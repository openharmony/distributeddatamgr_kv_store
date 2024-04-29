/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef I_STORAGE_HANDLE_H
#define I_STORAGE_HANDLE_H

#include "sqlite_single_ver_storage_executor.h"

namespace DistributedDB {
class KvStorageHandle {
public:
    KvStorageHandle() = default;
    virtual ~KvStorageHandle() = default;

    virtual std::pair<int, SQLiteSingleVerStorageExecutor*> GetStorageExecutor(bool isWrite) = 0;

    virtual void RecycleStorageExecutor(SQLiteSingleVerStorageExecutor *executor) = 0;

    virtual int GetMetaData(const Key &key, Value &value) const = 0;

    virtual int PutMetaData(const Key &key, const Value &value, bool isInTransaction) = 0;

    virtual TimeOffset GetLocalTimeOffsetForCloud() = 0;

    virtual Timestamp GetCurrentTimestamp() = 0;
};
}
#endif // I_STORAGE_HANDLE_H
