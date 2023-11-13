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

#include "single_ver_natural_store_connection.h"


#include "db_errno.h"
#include "log_print.h"
#include "single_ver_natural_store.h"

namespace DistributedDB {
SingleVerNaturalStoreConnection::SingleVerNaturalStoreConnection(SyncAbleKvDB *kvDB) : SyncAbleKvDBConnection(kvDB)
{
}

SingleVerNaturalStoreConnection::~SingleVerNaturalStoreConnection()
{
}

int SingleVerNaturalStoreConnection::Put(const IOption &option, const Key &key, const Value &value)
{
    std::vector<Entry> entries;
    Entry entry{key, value};
    entries.emplace_back(std::move(entry));

    return PutBatch(option, entries);
}

int SingleVerNaturalStoreConnection::PutBatch(const IOption &option, const std::vector<Entry> &entries)
{
    LOGD("[PutBatch] entries size is : %zu, dataType : %d", entries.size(), option.dataType);
    if (option.dataType == IOption::SYNC_DATA) {
        int errCode = CheckSyncEntriesValid(entries);
        if (errCode != E_OK) {
            return errCode;
        }
        return PutBatchInner(option, entries);
    }

    return -E_NOT_SUPPORT;
}

bool SingleVerNaturalStoreConnection::IsExtendedCacheDBMode() const
{
    return false;
}

int SingleVerNaturalStoreConnection::CheckSyncEntriesValid(const std::vector<Entry> &entries) const
{
    if (entries.size() > DBConstant::MAX_BATCH_SIZE) {
        return -E_INVALID_ARGS;
    }

    SingleVerNaturalStore *naturalStore = GetDB<SingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = CheckWritePermission();
    if (errCode != E_OK) {
        return errCode;
    }

    for (const auto &entry : entries) {
        errCode = naturalStore->CheckDataStatus(entry.key, entry.value, false);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int SingleVerNaturalStoreConnection::CheckWritePermission() const
{
    return E_OK;
}

int SingleVerNaturalStoreConnection::Get(const IOption &option, const Key &key, Value &value) const
{
    return E_OK;
}

// Delete the value from the database
int SingleVerNaturalStoreConnection::Delete(const IOption &option, const Key &key)
{
    std::vector<Key> keys;
    keys.push_back(key);

    return DeleteBatch(option, keys);
}

// Delete the batch values from the database.
int SingleVerNaturalStoreConnection::DeleteBatch(const IOption &option, const std::vector<Key> &keys)
{
    LOGD("[DeleteBatch] keys size is : %zu, dataType : %d", keys.size(), option.dataType);
    if (option.dataType == IOption::SYNC_DATA) {
        int errCode = CheckSyncKeysValid(keys);
        if (errCode != E_OK) {
            return errCode;
        }
        return DeleteBatchInner(option, keys);
    }

    return -E_NOT_SUPPORT;
}

} // namespace DistributedDB