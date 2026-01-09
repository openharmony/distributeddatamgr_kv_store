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

#define LOG_TAG "KvStoreObserverStub"

#include "ikvstore_observer.h"

#include <cinttypes>
#include <ipc_skeleton.h>
#include "kv_types_util.h"
#include "itypes_util.h"
#include "log_print.h"
#include "message_parcel.h"
namespace OHOS {
namespace DistributedKv {
using namespace std::chrono;

enum {
    CLOUD_ONCHANGE,
    ONCHANGE,
};

int32_t KvStoreObserverStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    const int errorResult = -1;
    if (KvStoreObserverStub::GetDescriptor() != data.ReadInterfaceToken()) {
        ZLOGE("Local descriptor is not equal to remote");
        return errorResult;
    }
    switch (code) {
        case ONCHANGE: {
            return OnRemoteChange(data, reply);
        }
        case CLOUD_ONCHANGE: {
            std::string store;
            Keys keys;
            if (!ITypesUtil::Unmarshal(data, store, keys[OP_INSERT], keys[OP_UPDATE], keys[OP_DELETE])) {
                ZLOGE("ReadChangeList from Parcel failed");
                return errorResult;
            }
            OnChange({ .store = store }, std::move(keys));
            return 0;
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
}

int32_t KvStoreObserverStub::OnRemoteChange(MessageParcel &data, MessageParcel &reply)
{
    int64_t totalSize = 0;
    if (!ITypesUtil::Unmarshal(data, totalSize)) {
        ZLOGE("unmarshall totalSize fail, totalSize:%{public}" PRIu64, totalSize);
        return -1;
    }
    if (totalSize < SWITCH_RAW_DATA_SIZE) {
        ChangeNotification notification({}, {}, {}, "", false);
        if (!ITypesUtil::Unmarshal(data, notification)) {
            ZLOGE("unmarshall ChangeNotification fail");
            return -1;
        }
        OnChange(notification);
    } else {
        std::string deviceId;
        bool isClear = false;
        uint64_t insertSize = 0;
        uint64_t updateSize = 0;
        uint64_t deleteSize = 0;
        if (!ITypesUtil::Unmarshal(data, deviceId, isClear, insertSize, updateSize, deleteSize)) {
            ZLOGE("unmarshall fail, I:%{public}" PRIu64 "U:%{public}" PRIu64 "D:%{public}" PRIu64,
                insertSize, updateSize, deleteSize);
            return -1;
        }
        std::vector<Entry> totalEntries;
        if (!ITypesUtil::UnmarshalFromBuffer(data, totalEntries)) {
            ZLOGE("unmarshall entries fail, size:%{public}zu", totalEntries.size());
            return -1;
        }
        ZLOGI("I:%{public}" PRIu64 "U:%{public}" PRIu64 "D:%{public}" PRIu64 "T:%{public}zu", insertSize, updateSize,
            deleteSize, totalEntries.size());
        if ((insertSize + updateSize + deleteSize) != static_cast<uint64_t>(totalEntries.size())) {
            return -1;
        }
        std::vector<Entry> insertEntries = std::vector<Entry>(totalEntries.begin(), totalEntries.begin() + insertSize);
        std::vector<Entry> updateEntries = std::vector<Entry>(totalEntries.begin() + insertSize,
            totalEntries.begin() + insertSize + updateSize);
        std::vector<Entry> deleteEntries = std::vector<Entry>(totalEntries.begin() + insertSize + updateSize,
            totalEntries.end());
        ChangeNotification notification(std::move(insertEntries), std::move(updateEntries), std::move(deleteEntries),
            deviceId, isClear);
        OnChange(notification);
    }
    return 0;
}
}  // namespace DistributedKv
}  // namespace OHOS
