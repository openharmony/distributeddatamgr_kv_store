/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_SINGLE_VER_SYNC_ENGINE_H
#define MOCK_SINGLE_VER_SYNC_ENGINE_H

#include <gmock/gmock.h>
#include "sync_engine.h"
#include "single_ver_sync_engine.h"

namespace DistributedDB {
class MockSingleVerSyncEngine : public SingleVerSyncEngine {
public:
    MOCK_METHOD1(CreateSyncTaskContext, ISyncTaskContext *(const ISyncInterface &syncInterface));

    void InitSubscribeManager()
    {
        subManager_ = std::make_shared<SubscribeManager>();
    }

    ISyncTaskContext *CallGetSyncTaskContext(const DeviceSyncTarget &target, int &errCode)
    {
        return SyncEngine::GetSyncTaskContext(target, errCode);
    }

    void SetSyncTaskContextMap(const std::map<DeviceSyncTarget, ISyncTaskContext *> &syncTaskContextMap)
    {
        std::unique_lock<std::mutex> lock(contextMapLock_);
        syncTaskContextMap_ = syncTaskContextMap;
    }

    void SetQueryAutoSyncCallbackNull(bool isNeed, const InitCallbackParam &callbackParam)
    {
        if (!isNeed) {
            this->queryAutoSyncCallback_ = nullptr;
        } else {
            this->queryAutoSyncCallback_ = callbackParam.queryAutoSyncCallback;
        }
        return;
    }
};
} // namespace DistributedDB
#endif  // #define MOCK_SINGLE_VER_SYNC_ENGINE_H