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

#include "virtual_sqlite_storage_engine.h"

namespace DistributedDB {
void VirtualSingleVerStorageEngine::ForkNewExecutorMethod(const NewExecutorMethod &method)
{
    std::lock_guard<std::mutex> autoLock(functionMutex_);
    forkNewFunc_ = method;
}

void VirtualSingleVerStorageEngine::ForkOpenMainDatabaseMethod(const OpenMainDatabaseMethod &method)
{
    std::lock_guard<std::mutex> autoLock(functionMutex_);
    forkOpenMainFunc_ = method;
}

void VirtualSingleVerStorageEngine::SetMaxNum(int maxRead, int maxWrite)
{
    engineAttr_.maxReadNum = maxRead;
    engineAttr_.maxWriteNum = maxWrite;
}

void VirtualSingleVerStorageEngine::SetEnhance(bool isEnhance)
{
    isEnhance_ = isEnhance;
}

void VirtualSingleVerStorageEngine::CallSetSQL(const std::vector<std::string> &sql)
{
    SetSQL(sql);
}

std::pair<int, sqlite3 *> VirtualSingleVerStorageEngine::GetCacheHandle(OpenDbProperties &option)
{
    std::pair<int, sqlite3 *> res;
    auto &[errCode, db] = res;
    errCode = SQLiteSingleVerStorageEngine::GetCacheDbHandle(db, option);
    return res;
}

int VirtualSingleVerStorageEngine::CreateNewExecutor(bool isWrite, StorageExecutor *&handle)
{
    {
        std::lock_guard<std::mutex> autoLock(functionMutex_);
        if (forkNewFunc_ != nullptr) {
            return forkNewFunc_(isWrite, handle);
        }
    }
    return SQLiteSingleVerStorageEngine::CreateNewExecutor(isWrite, handle);
}

int VirtualSingleVerStorageEngine::TryToOpenMainDatabase(bool isWrite, sqlite3 *&db, OpenDbProperties &option)
{
    {
        std::lock_guard<std::mutex> autoLock(functionMutex_);
        if (forkOpenMainFunc_ != nullptr) {
            return forkOpenMainFunc_(isWrite, db, option);
        }
    }
    return SQLiteSingleVerStorageEngine::TryToOpenMainDatabase(isWrite, db, option);
}
}