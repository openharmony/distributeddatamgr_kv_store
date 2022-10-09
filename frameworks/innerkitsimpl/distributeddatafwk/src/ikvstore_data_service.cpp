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

Status KvStoreDataServiceProxy::GetLocalDevice(OHOS::DistributedKv::DeviceInfo &device)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(GETLOCALDEVICE, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest returned %d", error);
        return Status::IPC_ERROR;
    }
    Status status = static_cast<Status>(reply.ReadInt32());
    if (status == Status::SUCCESS) {
        device = {reply.ReadString(), reply.ReadString(), reply.ReadString()};
    }
    return status;
}

Status KvStoreDataServiceProxy::GetRemoteDevices(std::vector<DeviceInfo> &deviceInfoList, DeviceFilterStrategy strategy)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.WriteInt32(static_cast<int>(strategy))) {
        ZLOGW("write int failed.");
        return Status::IPC_ERROR;
    }
    MessageParcel reply;
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(GETREMOTEDEVICES, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest returned %d", error);
        return Status::IPC_ERROR;
    }
    Status status = static_cast<Status>(reply.ReadInt32());
    if (status == Status::SUCCESS) {
        int len = reply.ReadInt32();
        for (int i = 0; i < len; i++) {
            DeviceInfo deviceInfo = {
                .deviceId = reply.ReadString(),
                .deviceName = reply.ReadString(),
                .deviceType = reply.ReadString()
            };
            deviceInfoList.push_back(std::move(deviceInfo));
        }
    }
    return status;
}

Status KvStoreDataServiceProxy::StartWatchDeviceChange(sptr<IDeviceStatusChangeListener> observer,
                                                       DeviceFilterStrategy strategy)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!data.WriteInt32(static_cast<int>(strategy))) {
        ZLOGW("write int failed.");
        return Status::IPC_ERROR;
    }
    if (observer != nullptr) {
        if (!data.WriteRemoteObject(observer->AsObject().GetRefPtr())) {
            return Status::IPC_ERROR;
        }
    } else {
        return Status::INVALID_ARGUMENT;
    }
    MessageParcel reply;
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(STARTWATCHDEVICECHANGE, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest returned %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}

Status KvStoreDataServiceProxy::StopWatchDeviceChange(sptr<IDeviceStatusChangeListener> observer)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(KvStoreDataServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (observer != nullptr) {
        if (!data.WriteRemoteObject(observer->AsObject().GetRefPtr())) {
            return Status::IPC_ERROR;
        }
    } else {
        return Status::INVALID_ARGUMENT;
    }
    MessageParcel reply;
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(STOPWATCHDEVICECHANGE, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest returned %d", error);
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

int32_t KvStoreDataServiceStub::GetRemoteDevicesOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::vector<DeviceInfo> infos;
    DeviceFilterStrategy strategy = static_cast<DeviceFilterStrategy>(data.ReadInt32());
    Status status = GetRemoteDevices(infos, strategy);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    if (status == Status::SUCCESS) {
        if (!reply.WriteInt32(infos.size())) {
            return -1;
        }
        for (DeviceInfo const &info : infos) {
            if (!reply.WriteString(info.deviceId) || !reply.WriteString(info.deviceName) ||
                !reply.WriteString(info.deviceType)) {
                return -1;
            }
        }
    }
    return 0;
}

int32_t KvStoreDataServiceStub::StartWatchDeviceChangeOnRemote(MessageParcel &data, MessageParcel &reply)
{
    DeviceFilterStrategy strategy = static_cast<DeviceFilterStrategy>(data.ReadInt32());
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    if (remote == nullptr) {
        ZLOGW("observerProxy nullptr after ipc");
        if (!reply.WriteInt32(static_cast<int>(Status::IPC_ERROR))) {
            return -1;
        }
        return 0;
    }
    sptr<IDeviceStatusChangeListener> observerProxy = iface_cast<IDeviceStatusChangeListener>(remote);
    Status status = StartWatchDeviceChange(std::move(observerProxy), strategy);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::StopWatchDeviceChangeOnRemote(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    if (remote == nullptr) {
        ZLOGW("observerProxy nullptr after ipc");
        if (!reply.WriteInt32(static_cast<int>(Status::IPC_ERROR))) {
            return -1;
        }
        return 0;
    }
    sptr<IDeviceStatusChangeListener> observerProxy = iface_cast<IDeviceStatusChangeListener>(remote);
    Status status = StopWatchDeviceChange(std::move(observerProxy));
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

int32_t KvStoreDataServiceStub::GetLocalDeviceOnRemote(MessageParcel &data, MessageParcel &reply)
{
    DeviceInfo info;
    Status status = GetLocalDevice(info);
    if (!reply.WriteInt32(static_cast<int>(status)) || !reply.WriteString(info.deviceId) ||
        !reply.WriteString(info.deviceName) || !reply.WriteString(info.deviceType)) {
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
