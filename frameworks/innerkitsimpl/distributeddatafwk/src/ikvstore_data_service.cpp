/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "KvStoreDataServiceProxy"

#include "ikvstore_data_service.h"
#include <ipc_skeleton.h>
#include "irdb_service.h"
#include "rdb_service_proxy.h"
#include "itypes_util.h"
#include "message_parcel.h"
#include "types.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
constexpr KvStoreDataServiceStub::RequestHandler KvStoreDataServiceStub::HANDLERS[SERVICE_CMD_LAST];
KvStoreDataServiceProxy::KvStoreDataServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IKvStoreDataService>(impl)
{
    ZLOGI("init data service proxy.");
}

sptr<IRemoteObject> KvStoreDataServiceProxy::GetFeatureInterface(const std::string &name)
{
    ZLOGI("%s", name.c_str());
    MessageParcel data;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return nullptr;
    }

    if (!ITypesUtil::Marshal(data, name)) {
        ZLOGE("write descriptor failed");
        return nullptr;
    }

    MessageParcel reply;
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(GET_FEATURE_INTERFACE, data, reply, mo);
    if (error != 0) {
        ZLOGE("SendRequest returned %{public}d", error);
        return nullptr;
    }

    sptr<IRemoteObject> remoteObject;
    if (!ITypesUtil::Unmarshal(reply, remoteObject)) {
        ZLOGE("remote object is nullptr");
        return nullptr;
    }
    return remoteObject;
}

Status KvStoreDataServiceProxy::GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
                                                 std::function<void(sptr<ISingleKvStore>)> callback)
{
    ZLOGI("%s %s", appId.appId.c_str(), storeId.storeId.c_str());
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.SetMaxCapacity(MAX_IPC_CAPACITY)) {
        ZLOGW("SetMaxCapacity failed.");
        return Status::IPC_ERROR;
    }
    // Passing a struct with an std::string field is a potential security exploit.
    OptionsIpc optionsIpc;
    optionsIpc.createIfMissing = options.createIfMissing;
    optionsIpc.encrypt = options.encrypt;
    optionsIpc.persistent = options.persistent;
    optionsIpc.backup = options.backup;
    optionsIpc.autoSync = options.autoSync;
    optionsIpc.securityLevel = options.securityLevel;
    optionsIpc.kvStoreType = options.kvStoreType;
    optionsIpc.syncable = options.syncable;
    std::string schemaString = options.schema;

    if (!data.WriteBuffer(&optionsIpc, sizeof(OptionsIpc)) ||
        !data.WriteString(appId.appId) ||
        !data.WriteString(storeId.storeId) ||
        !data.WriteString(schemaString)) {
        ZLOGW("failed to write parcel.");
        return Status::IPC_ERROR;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(GETSINGLEKVSTORE, data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    Status status = static_cast<Status>(reply.ReadInt32());
    if (status == Status::SUCCESS) {
        sptr<IRemoteObject> remote = reply.ReadRemoteObject();
        if (remote != nullptr) {
            sptr<ISingleKvStore> kvstoreImplProxy = iface_cast<ISingleKvStore>(remote);
            callback(std::move(kvstoreImplProxy));
        }
    } else {
        callback(nullptr);
    }
    return status;
}

void KvStoreDataServiceProxy::GetAllKvStoreId(const AppId &appId,
                                              std::function<void(Status, std::vector<StoreId> &)> callback)
{
}

Status KvStoreDataServiceProxy::CloseKvStore(const AppId &appId, const StoreId &storeId)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.WriteString(appId.appId) ||
        !data.WriteString(storeId.storeId)) {
        ZLOGW("failed to write parcel.");
        return Status::IPC_ERROR;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(CLOSEKVSTORE, data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}

/* close all opened kvstore */
Status KvStoreDataServiceProxy::CloseAllKvStore(const AppId &appId)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.WriteString(appId.appId)) {
        ZLOGW("failed to write parcel.");
        return Status::IPC_ERROR;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(CLOSEALLKVSTORE, data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}

Status KvStoreDataServiceProxy::DeleteKvStore(const AppId &appId, const StoreId &storeId)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.WriteString(appId.appId) ||
        !data.WriteString(storeId.storeId)) {
        ZLOGW("failed to write parcel.");
        return Status::IPC_ERROR;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(DELETEKVSTORE, data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}

/* delete all kv store */
Status KvStoreDataServiceProxy::DeleteAllKvStore(const AppId &appId)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.WriteString(appId.appId)) {
        ZLOGW("failed to write parcel.");
        return Status::IPC_ERROR;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(DELETEALLKVSTORE, data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}

Status KvStoreDataServiceProxy::RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.WriteString(appId.appId)) {
        ZLOGW("failed to write string.");
        return Status::IPC_ERROR;
    }
    if (observer != nullptr) {
        if (!data.WriteRemoteObject(observer)) {
            ZLOGW("failed to write parcel.");
            return Status::IPC_ERROR;
        }
    } else {
        return Status::INVALID_ARGUMENT;
    }

    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(REGISTERCLIENTDEATHOBSERVER, data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}

int32_t KvStoreDataServiceStub::NoSupport(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    (void)reply;
    return NOT_SUPPORT;
}

int32_t KvStoreDataServiceStub::GetSingleKvStoreOnRemote(MessageParcel &data, MessageParcel &reply)
{
    const OptionsIpc *optionIpcPtr = reinterpret_cast<const OptionsIpc *>(data.ReadBuffer(sizeof(OptionsIpc)));
    if (optionIpcPtr == nullptr) {
        ZLOGW("optionPtr is nullptr");
        if (!reply.WriteInt32(static_cast<int>(Status::INVALID_ARGUMENT))) {
            return -1;
        }
        return 0;
    }
    OptionsIpc optionsIpc = *optionIpcPtr;
    AppId appId = { data.ReadString() };
    StoreId storeId = { data.ReadString() };
    Options options;
    options.createIfMissing = optionsIpc.createIfMissing;
    options.encrypt = optionsIpc.encrypt;
    options.persistent = optionsIpc.persistent;
    options.backup = optionsIpc.backup;
    options.autoSync = optionsIpc.autoSync;
    options.securityLevel = optionsIpc.securityLevel;
    options.kvStoreType = optionsIpc.kvStoreType;
    options.syncable = optionsIpc.syncable;
    options.schema = data.ReadString();
    sptr<ISingleKvStore> proxyTmp;
    Status status = GetSingleKvStore(options, appId, storeId,
                                     [&](sptr<ISingleKvStore> proxy) { proxyTmp = std::move(proxy); });
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    if (status == Status::SUCCESS && proxyTmp != nullptr) {
        if (!reply.WriteRemoteObject(proxyTmp->AsObject().GetRefPtr())) {
            return -1;
        }
    }
    return 0;
}

int32_t KvStoreDataServiceStub::CloseKvStoreOnRemote(MessageParcel &data, MessageParcel &reply)
{
    AppId appId = { data.ReadString() };
    StoreId storeId = { data.ReadString() };
    Status status = CloseKvStore(appId, storeId);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::CloseAllKvStoreOnRemote(MessageParcel &data, MessageParcel &reply)
{
    AppId appId = { data.ReadString() };
    Status status = CloseAllKvStore(appId);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::DeleteKvStoreOnRemote(MessageParcel &data, MessageParcel &reply)
{
    AppId appId = { data.ReadString() };
    StoreId storeId = { data.ReadString() };
    Status status = DeleteKvStore(appId, storeId);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::DeleteAllKvStoreOnRemote(MessageParcel &data, MessageParcel &reply)
{
    AppId appId = { data.ReadString() };
    Status status = DeleteAllKvStore(appId);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::RegisterClientDeathObserverOnRemote(MessageParcel &data, MessageParcel &reply)
{
    AppId appId = { data.ReadString() };
    sptr<IRemoteObject> kvStoreClientDeathObserverProxy = data.ReadRemoteObject();
    if (kvStoreClientDeathObserverProxy == nullptr) {
        return -1;
    }
    Status status = RegisterClientDeathObserver(appId, std::move(kvStoreClientDeathObserverProxy));
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::GetFeatureInterfaceOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string name;
    if (!ITypesUtil::Unmarshal(data, name)) {
        return -1;
    }
    auto remoteObject = GetFeatureInterface(name);
    if (!ITypesUtil::Marshal(reply, remoteObject)) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
                                                MessageParcel &reply, MessageOption &option)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    std::u16string descriptor = KvStoreDataServiceStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        ZLOGE("local descriptor is not equal to remote");
        return -1;
    }
    if (code >= 0 && code < SERVICE_CMD_LAST) {
        return (this->*HANDLERS[code])(data, reply);
    } else {
        MessageOption mo { MessageOption::TF_SYNC };
        return IPCObjectStub::OnRemoteRequest(code, data, reply, mo);
    }
}
}  // namespace DistributedKv
}  // namespace OHOS
