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

#ifndef SINGLE_VER_NATURAL_STORE_CONNECTION_H
#define SINGLE_VER_NATURAL_STORE_CONNECTION_H
#include "sync_able_kvdb_connection.h"

namespace DistributedDB {
class SingleVerNaturalStoreConnection : public SyncAbleKvDBConnection {
public:
    explicit SingleVerNaturalStoreConnection(SyncAbleKvDB *kvDB);

    virtual ~SingleVerNaturalStoreConnection();

    int Put(const IOption &option, const Key &key, const Value &value) override;

    int PutBatch(const IOption &option, const std::vector<Entry> &entries) override;

    virtual bool IsExtendedCacheDBMode() const;

    int Get(const IOption &option, const Key &key, Value &value) const override;

    // Delete the value from the database
    int Delete(const IOption &option, const Key &key) override;

    // Delete the batch values from the database.
    int DeleteBatch(const IOption &option, const std::vector<Key> &keys) override;
protected:
    virtual int CheckSyncEntriesValid(const std::vector<Entry> &entries) const;

    virtual int CheckWritePermission() const;

    virtual int PutBatchInner(const IOption &option, const std::vector<Entry> &entries) = 0;

    virtual int CheckSyncKeysValid(const std::vector<Key> &keys) const = 0;

    virtual int DeleteBatchInner(const IOption &option, const std::vector<Key> &keys) = 0;

    bool CheckAndGetEntryLen(const std::vector<Entry> &entries, uint32_t limit, uint32_t &keyLen) const;

    bool CheckAndGetKeyLen(const std::vector<Key> &keys, const uint32_t limit, uint32_t &keyLen) const;
};
} // namespace DistributedDB

#endif // SINGLE_VER_NATURAL_STORE_CONNECTION_H