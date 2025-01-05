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

#define LOG_TAG "KvStoreObserverProxy"

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
            if (data.ReadInt32() < SWITCH_RAW_DATA_SIZE) {
                ChangeNotification notification({}, {}, {}, "", false);
                if (!ITypesUtil::Unmarshal(data, notification)) {
                    ZLOGE("ChangeNotification is nullptr");
                    return errorResult;
                }
                OnChange(notification);
            } else {
                std::string deviceId;
                uint32_t clear = 0;
                std::vector<Entry> inserts;
                std::vector<Entry> updates;
                std::vector<Entry> deletes;
                if (!ITypesUtil::Unmarshal(data, deviceId, clear) || !ITypesUtil::UnmarshalFromBuffer(data, inserts) ||
                    !ITypesUtil::UnmarshalFromBuffer(data, updates) ||
                    !ITypesUtil::UnmarshalFromBuffer(data, deletes)) {
                    ZLOGE("WriteChangeList to Parcel by buffer failed");
                    return errorResult;
                }
                ChangeNotification change(std::move(inserts), std::move(updates), std::move(deletes), deviceId,
                    clear != 0);
                OnChange(change);
            }
            return 0;
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
}  // namespace DistributedKv
}  // namespace OHOS
