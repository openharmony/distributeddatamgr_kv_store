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

#ifndef SINGLE_VER_NATURAL_STORE_H
#define SINGLE_VER_NATURAL_STORE_H
#include "single_ver_kvdb_sync_interface.h"
#include "single_ver_natural_store_commit_notify_data.h"
#include "sync_able_kvdb.h"

namespace DistributedDB {
class SingleVerNaturalStore : public SyncAbleKvDB, public SingleVerKvDBSyncInterface {
public:
    SingleVerNaturalStore();
    ~SingleVerNaturalStore() override;

    virtual void EnableHandle();

    virtual void AbortHandle();

    int RemoveKvDB(const KvDBProperties &properties) override;

protected:
    struct TransPair {
        int index;
        RegisterFuncType funcType;
    };
    static RegisterFuncType GetFuncType(int index, const TransPair *transMap, int32_t len);

    void CommitAndReleaseNotifyData(SingleVerNaturalStoreCommitNotifyData *&committedData,
        bool isNeedCommit, int eventType);
};
} // namespace DistributedDB

#endif // SINGLE_VER_NATURAL_STORE_H