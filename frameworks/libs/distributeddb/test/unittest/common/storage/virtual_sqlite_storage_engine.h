/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef VIRTUAL_SQLITE_STORAGE_ENGINE_H
#define VIRTUAL_SQLITE_STORAGE_ENGINE_H

#include "sqlite_single_ver_storage_engine.h"

namespace DistributedDB {
class VirtualSingleVerStorageEngine : public SQLiteSingleVerStorageEngine {
public:
    using NewExecutorMethod = std::function<int(bool isWrite, StorageExecutor *&)>;
    using OpenMainDatabaseMethod = std::function<int(bool isWrite, sqlite3 *&, OpenDbProperties &)>;
    void ForkNewExecutorMethod(const NewExecutorMethod &method);
    void ForkOpenMainDatabaseMethod(const OpenMainDatabaseMethod &method);
    void SetMaxNum(int maxRead, int maxWrite);
    void SetEnhance(bool isEnhance);
    void CallSetSQL(const std::vector<std::string> &sql);
    std::pair<int, sqlite3 *> GetCacheHandle(OpenDbProperties &option);
protected:
    int CreateNewExecutor(bool isWrite, DistributedDB::StorageExecutor *&handle) override;
    int TryToOpenMainDatabase(bool isWrite, sqlite3 *&db, OpenDbProperties &option) override;
private:
    std::mutex functionMutex_;
    NewExecutorMethod forkNewFunc_;
    OpenMainDatabaseMethod forkOpenMainFunc_;
};
}
#endif // VIRTUAL_SQLITE_STORAGE_ENGINE_H
