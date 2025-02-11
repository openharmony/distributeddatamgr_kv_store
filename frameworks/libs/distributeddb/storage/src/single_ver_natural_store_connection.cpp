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
    if (option.dataType != IOption::SYNC_DATA) {
        return -E_NOT_SUPPORT;
    }
    int errCode = CheckSyncEntriesValid(entries);
    if (errCode != E_OK) {
        return errCode;
    }
    return PutBatchInner(option, entries);
}

bool SingleVerNaturalStoreConnection::IsExtendedCacheDBMode() const
{
    return false;
}

bool SingleVerNaturalStoreConnection::CheckAndGetEntryLen(const std::vector<Entry> &entries, uint32_t limit,
    uint32_t &entryLen) const
{
    uint32_t len = 0;
    for (const auto &entry : entries) {
        len += (entry.key.size() + entry.value.size());
        if (len > limit) {
            return false; // stop calculate key len when it excced limit.
        }
    }
    entryLen = len;
    return true;
}

bool SingleVerNaturalStoreConnection::CheckAndGetKeyLen(const std::vector<Key> &keys, const uint32_t limit,
    uint32_t &keyLen) const
{
    uint32_t len = 0;
    for (const auto &key : keys) {
        len += key.size();
        if (len > limit) {
            return false; // stop calculate key len when it excced limit.
        }
    }
    keyLen = len;
    return true;
}

int SingleVerNaturalStoreConnection::CheckSyncEntriesValid(const std::vector<Entry> &entries) const
{
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

int SingleVerNaturalStoreConnection::Delete(const IOption &option, const Key &key)
{
    std::vector<Key> keys;
    keys.push_back(key);

    return DeleteBatch(option, keys);
}

int SingleVerNaturalStoreConnection::DeleteBatch(const IOption &option, const std::vector<Key> &keys)
{
    LOGD("[DeleteBatch] keys size is : %zu, dataType : %d", keys.size(), option.dataType);
    if (option.dataType != IOption::SYNC_DATA) {
        return -E_NOT_SUPPORT;
    }
    int errCode = CheckSyncKeysValid(keys);
    if (errCode != E_OK) {
        return errCode;
    }
    return DeleteBatchInner(option, keys);
}
} // namespace DistributedDB