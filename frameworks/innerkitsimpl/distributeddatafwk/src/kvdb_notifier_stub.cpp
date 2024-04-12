/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "KVDBNotifierStub"

#include "kvdb_notifier_stub.h"
#include <chrono>
#include <ctime>
#include <cinttypes>
#include <ipc_skeleton.h>
#include <map>
#include "itypes_util.h"
#include "log_print.h"
#include "message_parcel.h"
#include "message_option.h"
#include "store_util.h"
#include "types.h"

namespace OHOS {
namespace DistributedKv {
const KVDBNotifierStub::Handler
    KVDBNotifierStub::HANDLERS[static_cast<uint32_t>(KVDBNotifierCode::TRANS_BUTT)] = {
    &KVDBNotifierStub::OnSyncCompleted,
    &KVDBNotifierStub::OnOnRemoteChanged,
};

int32_t KVDBNotifierStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    ZLOGI("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    std::u16string local = KVDBNotifierStub::GetDescriptor();
    std::u16string remote = data.ReadInterfaceToken();
    if (local != remote) {
        ZLOGE("local descriptor is not equal to remote");
        return -1;
    }
    if (code >= static_cast<uint32_t>(KVDBNotifierCode::TRANS_HEAD) &&
        code < static_cast<uint32_t>(KVDBNotifierCode::TRANS_BUTT) && HANDLERS[code] == nullptr) {
        return (this->*HANDLERS[code])(data, reply);
    }
    ZLOGE("not support code:%{public}u, BUTT:%{public}d",
        code, static_cast<uint32_t>(KVDBNotifierCode::TRANS_BUTT));
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t KVDBNotifierStub::OnSyncCompleted(MessageParcel& data, MessageParcel& reply)
{
    std::map<std::string, Status> results;
    uint64_t sequenceId;
    if (!ITypesUtil::Unmarshal(data, results, sequenceId)) {
        ZLOGE("Unmarshal results size:%{public}zu, sequenceId:%{public}" PRIu64, results.size(), sequenceId);
        return IPC_STUB_INVALID_DATA_ERR;
    }
    SyncCompleted(results, sequenceId);
    return ERR_NONE;
}

int32_t KVDBNotifierStub::OnOnRemoteChanged(MessageParcel& data, MessageParcel& reply)
{
    std::string deviceId;
    bool isChanged = true;
    if (!ITypesUtil::Unmarshal(data, deviceId, isChanged)) {
        ZLOGE("Unmarshal deviceId:%{public}s, isChanged:%{public}d",
            StoreUtil::Anonymous(deviceId).c_str(), isChanged);
        return IPC_STUB_INVALID_DATA_ERR;
    }
    OnRemoteChanged(deviceId, isChanged);
    return ERR_NONE;
}
}  // namespace DistributedKv
}  // namespace OHOS
