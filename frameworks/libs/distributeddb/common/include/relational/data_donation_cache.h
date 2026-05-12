/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef DATA_DONATION_CACHE_H
#define DATA_DONATION_CACHE_H

#include <vector>
#include <memory>
#include <cstring>
#include <securec.h>
#include "store_types.h"
#include "db_types.h"
#include "data_donation_types.h"
#include "data_donation_schema.h"
#include "unique_queue.h"
#include "sqlite_single_ver_relational_storage_executor.h"

#ifdef RELATIONAL_STORE
namespace DistributedDB {
using namespace std;

constexpr size_t GET_ALL_BATCH_NUM = 1000;
constexpr size_t GET_NEW_BATCH_NUM = 100;

class DataDonationCache : public UniqueQueue<DdData, DdDataHash, std::equal_to<DdData>> {
public:
    int SetSchema(const std::string &schema);
    int Query(SQLiteSingleVerRelationalStorageExecutor *handle, const DBSubscribeCur &cursorIn,
        DBSubscribeCur &cursorOut, std::vector<VBucket> &data);
    int UpdateCursor(const DdCursor &cursorIn, DdData &ddData);

    DdData cacheRead[GET_ALL_BATCH_NUM]{};
    DdData cacheWrite[GET_ALL_BATCH_NUM]{};

private:
    uint64_t cursor = UINT64_MAX; // The water level value set externally，cursor % capacity = front
    DataDonationSchema ddSchema;

    int QueryStorage(SQLiteSingleVerRelationalStorageExecutor *handle, const DBSubscribeCur &cursorIn,
        DBSubscribeCur &cursorOut, std::vector<VBucket> &data);
    int QueryBinlog(SQLiteSingleVerRelationalStorageExecutor *handle, const DBSubscribeCur &cursorIn,
        DBSubscribeCur &cursorOut, std::vector<VBucket> &data);
};

}  // namespace DistributedDB
#endif  // RELATIONAL_STORE
#endif  // DATA_DONATION_CACHE_H