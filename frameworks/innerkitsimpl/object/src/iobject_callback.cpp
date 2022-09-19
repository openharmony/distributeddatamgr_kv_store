 /* Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "IObjectSaveCallback"

#include "iobject_callback.h"

#include <ipc_skeleton.h>

#include "itypes_util.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedObject {
constexpr static int32_t COMPLETED = 0;

void ObjectSaveCallbackProxy::Completed(const std::map<std::string, int32_t> &results)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, results)) {
        ZLOGE("Marshalling failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}

ObjectSaveCallbackProxy::ObjectSaveCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IObjectSaveCallback>(impl)
{
}

ObjectRevokeSaveCallbackProxy::ObjectRevokeSaveCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IObjectRevokeSaveCallback>(impl)
{
}

ObjectRetrieveCallbackProxy::ObjectRetrieveCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IObjectRetrieveCallback>(impl)
{
}

ObjectChangeCallbackProxy::ObjectChangeCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IObjectChangeCallback>(impl)
{
}

int ObjectSaveCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    ZLOGI("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    auto localDescriptor = GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return -1;
    }
    switch (code) {
        case COMPLETED: {
            std::map<std::string, int32_t> results;
            if (!ITypesUtil::Unmarshal(data, results)) {
                ZLOGE("Unmarshalling failed");
                return -1;
            }
            ZLOGI("object start complete");
            Completed(results);
            ZLOGI("object end complete");
            return 0;
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}

void ObjectRevokeSaveCallbackProxy::Completed(int32_t status)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, status)) {
        ZLOGE("write descriptor failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}

int ObjectRevokeSaveCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    ZLOGI("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    auto localDescriptor = GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return -1;
    }
    switch (code) {
        case COMPLETED: {
            int32_t status;
            if (!ITypesUtil::Unmarshal(data, status)) {
                ZLOGE("write descriptor failed");
                return -1;
            }
            ZLOGI("object start complete");
            Completed(status);
            ZLOGI("object end complete");
            return 0;
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}

int ObjectRetrieveCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    ZLOGI("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    auto localDescriptor = GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return -1;
    }
    switch (code) {
        case COMPLETED: {
            std::map<std::string, std::vector<uint8_t>> results;
            if (!ITypesUtil::Unmarshal(data, results)) {
                ZLOGE("write descriptor failed");
                return -1;
            }
            ZLOGI("object start complete");
            Completed(results);
            ZLOGI("object end complete");
            return 0;
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}

void ObjectRetrieveCallbackProxy::Completed(const std::map<std::string, std::vector<uint8_t>> &results)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, results)) {
        ZLOGE("write descriptor failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}

int ObjectChangeCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    ZLOGI("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    auto localDescriptor = GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return -1;
    }
    switch (code) {
        case COMPLETED: {
            std::map<std::string, std::vector<uint8_t>> results;
            if (!ITypesUtil::Unmarshal(data, results)) {
                ZLOGE("write descriptor failed");
                return -1;
            }
            Completed(results);
            return 0;
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}

void ObjectChangeCallbackProxy::Completed(const std::map<std::string, std::vector<uint8_t>> &results)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, results)) {
        ZLOGE("write descriptor failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}
} // namespace DistributedObject
} // namespace OHOS
