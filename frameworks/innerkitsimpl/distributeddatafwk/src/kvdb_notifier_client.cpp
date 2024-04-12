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

#define LOG_TAG "KVDBNotifierClient"

#include "kvdb_notifier_client.h"
#include <cinttypes>
#include <atomic>
#include "dds_trace.h"
#include "log_print.h"
#include "store_util.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::DistributedDataDfx;
KVDBNotifierClient::~KVDBNotifierClient()
{
    syncCallbackInfo_.Clear();
}

void KVDBNotifierClient::SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto finded = syncCallbackInfo_.Find(sequenceId);
    if (finded.first) {
        finded.second->SyncCompleted(results);
        finded.second->SyncCompleted(results, sequenceId);
        DeleteSyncCallback(sequenceId);
    }
}

void KVDBNotifierClient::OnRemoteChanged(const std::string &deviceId, bool isChanged)
{
    ZLOGD("remote changed deviceId:%{public}s, isChanged:%{public}d",
        StoreUtil::Anonymous(deviceId).c_str(), isChanged);
    if (deviceId.empty()) {
        return;
    }
    remotes_.Compute(deviceId, [isChanged](auto &, bool &value) {
        value = isChanged;
        return true;
    });
}

bool KVDBNotifierClient::IsChanged(const std::string &deviceId)
{
    auto [exist, value] = remotes_.Find(deviceId);
    ZLOGD("exist:%{public}d, changed:%{public}d", exist, value);
    return exist ? value : true; // no exist, need to sync.
}

void KVDBNotifierClient::AddSyncCallback(
    const std::shared_ptr<KvStoreSyncCallback> callback, uint64_t sequenceId)
{
    if (callback == nullptr) {
        ZLOGE("callback is nullptr");
        return;
    }
    auto inserted = syncCallbackInfo_.Insert(sequenceId, callback);
    if (!inserted) {
        ZLOGE("The sequeuceId %{public}" PRIu64 "is repeat!", sequenceId);
    }
}

void KVDBNotifierClient::DeleteSyncCallback(uint64_t sequenceId)
{
    syncCallbackInfo_.Erase(sequenceId);
}
}  // namespace DistributedKv
}  // namespace OHOS
