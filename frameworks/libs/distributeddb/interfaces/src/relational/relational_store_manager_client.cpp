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
#ifdef RELATIONAL_STORE
#include "relational_store_manager.h"

#include "db_common.h"
#include "cloud/cloud_storage_utils.h"

namespace DistributedDB {
DB_API std::string RelationalStoreManager::GetDistributedLogTableName(const std::string &tableName)
{
    return DBCommon::GetLogTableName(tableName);
}

static int GetCollateTypeByName(const std::map<std::string, CollateType> &collateTypeMap,
    const std::string &name, CollateType &collateType)
{
    auto it = collateTypeMap.find(name);
    if (it == collateTypeMap.end()) {
        LOGW("collate map doesn't contain primary key we need");
        collateType = CollateType::COLLATE_NONE;
        return E_OK;
    }
    if (static_cast<uint32_t>(it->second) >= static_cast<uint32_t>(CollateType::COLLATE_BUTT)) {
        LOGE("collate type is invalid");
        return -E_INVALID_ARGS;
    }
    collateType = it->second;
    return E_OK;
}

DB_API std::vector<uint8_t> RelationalStoreManager::CalcPrimaryKeyHash(const std::map<std::string, Type> &primaryKey,
    const std::map<std::string, CollateType> &collateTypeMap)
{
    std::vector<uint8_t> result;
    if (primaryKey.empty()) {
        LOGW("primaryKey is empty");
        return result;
    }
    int errCode = E_OK;
    CollateType collateType = CollateType::COLLATE_NONE;
    if (primaryKey.size() == 1) {
        auto iter = primaryKey.begin();
        Field field = {iter->first, static_cast<int32_t>(iter->second.index()), true, false};
        if (GetCollateTypeByName(collateTypeMap, iter->first, collateType) != E_OK) {
            return result;
        }
        errCode = CloudStorageUtils::CalculateHashKeyForOneField(field, primaryKey, false, collateType, result);
        if (errCode != E_OK) {
            // never happen
            LOGE("calc hash fail when there is one primary key errCode = %d", errCode);
        }
    } else {
        std::vector<uint8_t> tempRes;
        std::map<std::string, Type> pkOrderByUpperName;
        for (const auto &item : primaryKey) { // we sort by upper case name in log table when calculate hash
            pkOrderByUpperName[DBCommon::ToUpperCase(item.first)] = item.second;
        }

        for (const auto &item : pkOrderByUpperName) {
            std::vector<uint8_t> temp;
            Field field = {DBCommon::ToLowerCase(item.first), static_cast<int32_t>(item.second.index()), true, false};
            if (GetCollateTypeByName(collateTypeMap, DBCommon::ToLowerCase(item.first), collateType) != E_OK) {
                return result;
            }
            errCode = CloudStorageUtils::CalculateHashKeyForOneField(field, primaryKey, false, collateType, temp);
            if (errCode != E_OK) {
                // never happen
                LOGE("calc hash fail when there is more than one primary key errCode = %d", errCode);
                return result;
            }
            tempRes.insert(tempRes.end(), temp.begin(), temp.end());
        }
        errCode = DBCommon::CalcValueHash(tempRes, result);
        if (errCode != E_OK) {
            LOGE("calc hash fail when calc the composite primary key errCode = %d", errCode);
        }
    }
    return result;
}
} // namespace DistributedDB
#endif
