/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "DataMgrServiceProxy"

#include "datamgr_service_proxy.h"
#include <ipc_skeleton.h>
#include "itypes_util.h"
#include "message_parcel.h"
#include "types.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
DataMgrServiceProxy::DataMgrServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IKvStoreDataService>(impl)
{
    ZLOGI("init data service proxy.");
}

sptr<IRemoteObject> DataMgrServiceProxy::GetFeatureInterface(const std::string &name)
{
    ZLOGI("%s", name.c_str());
    MessageParcel data;
    if (!data.WriteInterfaceToken(DataMgrServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return nullptr;
    }

    if (!ITypesUtil::Marshal(data, name)) {
        ZLOGE("write name failed, name is %{public}s", name.c_str());
        return nullptr;
    }

    MessageParcel reply;
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(KvStoreDataServiceInterfaceCode::GET_FEATURE_INTERFACE), data, reply, mo);
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

Status DataMgrServiceProxy::RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(DataMgrServiceProxy::GetDescriptor())) {
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
        ZLOGE("observer is null");
        return Status::INVALID_ARGUMENT;
    }

    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(KvStoreDataServiceInterfaceCode::REGISTERCLIENTDEATHOBSERVER), data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}

int32_t DataMgrServiceProxy::ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex,
    int32_t tokenId)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(DataMgrServiceProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    if (!ITypesUtil::Marshal(data, bundleName, userId, appIndex, tokenId)) {
        ZLOGW("failed to write bundleName:%{public}s, user:%{public}d, appIndex:%{public}d, tokenID:%{public}d",
            bundleName.c_str(), userId, appIndex, tokenId);
        return Status::IPC_ERROR;
    }

    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(
        static_cast<uint32_t>(KvStoreDataServiceInterfaceCode::CLEAR_APP_STORAGE), data, reply, mo);
    if (error != 0) {
        ZLOGW("failed during IPC. errCode %d", error);
        return Status::IPC_ERROR;
    }
    return static_cast<Status>(reply.ReadInt32());
}
}  // namespace DistributedKv
}  // namespace OHOS
