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

#ifndef MOCK_RELATIONAL_SYNC_ABLE_STORAGE_H
#define MOCK_RELATIONAL_SYNC_ABLE_STORAGE_H
#include <gmock/gmock.h>
#include "relational_sync_able_storage.h"

namespace DistributedDB {
class MockRelationalSyncAbleStorage : public RelationalSyncAbleStorage {
public:
    MockRelationalSyncAbleStorage() : RelationalSyncAbleStorage(nullptr)
    {
    }

    int CallFillReferenceData(CloudSyncData &syncData)
    {
        return RelationalSyncAbleStorage::FillReferenceData(syncData);
    }

    MOCK_METHOD3(GetReferenceGid, int(const std::string &, const CloudSyncBatch &, std::map<int64_t, Entries> &));
};
}
#endif // MOCK_RELATIONAL_SYNC_ABLE_STORAGE_H
