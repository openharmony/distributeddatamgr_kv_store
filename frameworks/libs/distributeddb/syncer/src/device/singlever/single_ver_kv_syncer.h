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

#ifndef KV_SYNCER_H
#define KV_SYNCER_H

#include "single_ver_syncer.h"

namespace DistributedDB {
class SingleVerKVSyncer : public SingleVerSyncer {
public:
    SingleVerKVSyncer();
    ~SingleVerKVSyncer() override;

    // Enable auto sync function
    void EnableAutoSync(bool enable) override;

    // Local data changed callback
    void LocalDataChanged(int notifyEvent) override;

    // Remote data changed callback
    void RemoteDataChanged(const std::string &device) override;

    void TriggerSubscribe(const std::string &device, const QuerySyncObject &query);

    SyncerBasicInfo DumpSyncerBasicInfo() override;

protected:
    int SyncConditionCheck(const SyncParma &param, const ISyncEngine *engine, ISyncInterface *storage) const override;

    // Init the Sync engine
    int InitSyncEngine(ISyncInterface *syncInterface) override;

    void TriggerAddSubscribeAsync(ISyncInterface *syncInterface);
private:
    // if trigger full sync, no need to trigger query sync again
    bool TryFullSync(const std::vector<std::string> &devices);

    void TriggerSubQuerySync(const std::vector<std::string> &devices);

    bool autoSyncEnable_;
    std::atomic<bool> triggerSyncTask_;
};
} // namespace DistributedDB

#endif  // KV_SYNCER_H
