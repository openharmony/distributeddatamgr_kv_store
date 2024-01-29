/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef MOCK_SINGLE_VER_KV_SYNCER_H
#define MOCK_SINGLE_VER_KV_SYNCER_H
#include <gmock/gmock.h>
#include "single_ver_kv_syncer.h"

namespace DistributedDB {
class MockSingleVerKVSyncer : public SingleVerKVSyncer {
public:
    void CallRecordTimeChangeOffset(void *changedOffset)
    {
        RecordTimeChangeOffset(changedOffset);
    }

    void CallTriggerAddSubscribeAsync(ISyncInterface *syncInterface)
    {
        SingleVerKVSyncer::TriggerAddSubscribeAsync(syncInterface);
    }

    void SetSyncEngine(ISyncEngine *engine)
    {
        syncEngine_ = engine;
    }

    void CallQueryAutoSync(const InternalSyncParma &param)
    {
        SingleVerKVSyncer::QueryAutoSync(param);
    }

    void Init(ISyncEngine *engine, ISyncInterface *storage, bool init)
    {
        syncEngine_ = engine;
        syncInterface_ = storage;
        initialized_ = init;
    }

    int CallStatusCheck() const
    {
        return SingleVerKVSyncer::StatusCheck();
    }

    void SetMetadata(const std::shared_ptr<Metadata> &metadata)
    {
        metadata_ = metadata;
    }

    void TestSyncerLock()
    {
        LOGD("begin get lock");
        std::lock_guard<std::mutex> autoLock(syncerLock_);
        LOGD("get lock success");
    }
};
}
#endif // MOCK_SINGLE_VER_KV_SYNCER_H
