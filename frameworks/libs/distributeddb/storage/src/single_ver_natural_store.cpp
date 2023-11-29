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

#include "single_ver_natural_store.h"

#include "db_common.h"
#include "db_errno.h"
#include "kvdb_utils.h"
#include "log_print.h"
#include "sqlite_single_ver_storage_engine.h"
#include "storage_engine_manager.h"

namespace DistributedDB {
SingleVerNaturalStore::SingleVerNaturalStore()
{
}

SingleVerNaturalStore::~SingleVerNaturalStore()
{
}

void SingleVerNaturalStore::EnableHandle()
{
    return;
}

void SingleVerNaturalStore::AbortHandle()
{
    return;
}

int SingleVerNaturalStore::RemoveKvDB(const KvDBProperties &properties)
{
    // To avoid leakage, the engine resources are forced to be released
    const std::string identifier = properties.GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    (void)StorageEngineManager::ForceReleaseStorageEngine(identifier);

    // Only care the data directory and the db name.
    std::string storeOnlyDir;
    std::string storeDir;
    GenericKvDB::GetStoreDirectory(properties, KvDBProperties::SINGLE_VER_TYPE_SQLITE, storeDir, storeOnlyDir);

    const std::vector<std::pair<const std::string &, const std::string &>> dbDir {
        { DBConstant::MAINDB_DIR, DBConstant::SINGLE_VER_DATA_STORE },
        { DBConstant::METADB_DIR, DBConstant::SINGLE_VER_META_STORE },
        { DBConstant::CACHEDB_DIR, DBConstant::SINGLE_VER_CACHE_STORE }
    };

    bool isAllNotFound = true;
    for (const auto &item : dbDir) {
        std::string currentDir = storeDir + item.first + "/";
        std::string currentOnlyDir = storeOnlyDir + item.first + "/";
        int errCode = KvDBUtils::RemoveKvDB(currentDir, currentOnlyDir, item.second);
        if (errCode != -E_NOT_FOUND) {
            if (errCode != E_OK) {
                return errCode;
            }
            isAllNotFound = false;
        }
    };
    if (isAllNotFound) {
        return -E_NOT_FOUND;
    }

    int errCode = DBCommon::RemoveAllFilesOfDirectory(storeDir, true);
    if (errCode != E_OK) {
        return errCode;
    }
    return DBCommon::RemoveAllFilesOfDirectory(storeOnlyDir, true);
}

RegisterFuncType SingleVerNaturalStore::GetFuncType(int index, const TransPair *transMap, int32_t len)
{
    int32_t head = 0;
    int32_t end = len - 1;
    while (head <= end) {
        int32_t mid = head + (end - head) / 2;
        if (transMap[mid].index < index) {
            head = mid + 1;
            continue;
        }
        if (transMap[mid].index > index) {
            end = mid - 1;
            continue;
        }
        return transMap[mid].funcType;
    }
    return RegisterFuncType::REGISTER_FUNC_TYPE_MAX;
}

void SingleVerNaturalStore::CommitAndReleaseNotifyData(SingleVerNaturalStoreCommitNotifyData *&committedData,
    bool isNeedCommit, int eventType)
{
    if (isNeedCommit) {
        if (committedData != nullptr) {
            if (!committedData->IsChangedDataEmpty()) {
                CommitNotify(eventType, committedData);
            }
            if (!committedData->IsConflictedDataEmpty()) {
                CommitNotify(static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_CONFLICT_EVENT),
                    committedData);
            }
        }
    }

    if (committedData != nullptr) {
        committedData->DecObjRef(committedData);
        committedData = nullptr;
    }
}

} // namespace DistributedDB