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

#include "relational_store_client.h"

#include "db_common.h"
#include "kv_store_errno.h"
#include "relational_store_client_utils.h"

using namespace DistributedDB;

DistributedDB::DBStatus ArchiveSyncedData(sqlite3 *db, const std::string &tableName, uint64_t cursor)
{
#ifdef USE_DISTRIBUTEDDB_CLOUD
    auto errCode = RelationalStoreClientUtils::ArchiveSyncedData(db, tableName, cursor);
    if (errCode != DistributedDB::E_OK) {
        LOGE("ArchiveSyncedData[%s] failed %d", DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
    }
    return TransferDBErrno(errCode);
#else
    return DistributedDB::OK;
#endif
}

DistributedDB::DBStatus DeleteSyncedData(sqlite3 *db, const std::string &tableName,
    const std::vector<std::vector<DistributedDB::Type>> &keys)
{
#ifdef USE_DISTRIBUTEDDB_CLOUD
    auto errCode = RelationalStoreClientUtils::DeleteSyncedData(db, tableName, keys);
    if (errCode != DistributedDB::E_OK) {
        LOGE("DeleteSyncedData[%s] failed %d", DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
    }
    return TransferDBErrno(errCode);
#else
    return DistributedDB::OK;
#endif
} // namespace DistributedDB
