/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "param_check_utils.h"

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "log_print.h"
#include "platform_specific.h"

namespace DistributedDB {
bool ParamCheckUtils::CheckDataDir(const std::string &dataDir, std::string &canonicalDir)
{
    if (dataDir.empty() || (dataDir.length() > DBConstant::MAX_DATA_DIR_LENGTH)) {
        LOGE("Invalid data directory[%zu]", dataDir.length());
        return false;
    }

    // After normalizing the path, determine whether the path is a legal path considered by the program.
    // There has been guaranteed by the upper layer, So there is no need trustlist set here.
    return (OS::GetRealPath(dataDir, canonicalDir) == E_OK);
}

bool ParamCheckUtils::IsStoreIdSafe(const std::string &storeId)
{
    if (storeId.empty() || (storeId.length() > DBConstant::MAX_STORE_ID_LENGTH)) {
        LOGE("Invalid store id[%zu]", storeId.length());
        return false;
    }

    auto iter = std::find_if_not(storeId.begin(), storeId.end(),
        [](char value) { return (std::isalnum(value) || value == '_'); });
    if (iter != storeId.end()) {
        LOGE("Invalid store id format");
        return false;
    }
    return true;
}

bool ParamCheckUtils::CheckStoreParameter(const std::string &storeId, const std::string &appId,
    const std::string &userId, bool isIgnoreUserIdCheck)
{
    if (!IsStoreIdSafe(storeId)) {
        return false;
    }
    if (!isIgnoreUserIdCheck) {
        if (userId.empty() || userId.length() > DBConstant::MAX_USER_ID_LENGTH) {
            LOGE("Invalid user info[%zu][%zu]", userId.length(), appId.length());
            return false;
        }
        if (userId.find(DBConstant::ID_CONNECTOR) != std::string::npos) {
            LOGE("Invalid userId character in the store para info.");
            return false;
        }
    }
    if (appId.empty() || appId.length() > DBConstant::MAX_APP_ID_LENGTH) {
        LOGE("Invalid app info[%zu][%zu]", userId.length(), appId.length());
        return false;
    }

    if ((appId.find(DBConstant::ID_CONNECTOR) != std::string::npos) ||
        (storeId.find(DBConstant::ID_CONNECTOR) != std::string::npos)) {
        LOGE("Invalid character in the store para info.");
        return false;
    }
    return true;
}

bool ParamCheckUtils::CheckEncryptedParameter(CipherType cipher, const CipherPassword &passwd)
{
    if (cipher != CipherType::DEFAULT && cipher != CipherType::AES_256_GCM) {
        LOGE("Invalid cipher type!");
        return false;
    }

    return (passwd.GetSize() != 0);
}

bool ParamCheckUtils::CheckConflictNotifierType(int conflictType)
{
    if (conflictType <= 0) {
        return false;
    }
    // Divide the type into different types.
    if (conflictType >= CONFLICT_NATIVE_ALL) {
        conflictType -= CONFLICT_NATIVE_ALL;
    }
    if (conflictType >= CONFLICT_FOREIGN_KEY_ORIG) {
        conflictType -= CONFLICT_FOREIGN_KEY_ORIG;
    }
    if (conflictType >= CONFLICT_FOREIGN_KEY_ONLY) {
        conflictType -= CONFLICT_FOREIGN_KEY_ONLY;
    }
    return (conflictType == 0);
}

bool ParamCheckUtils::CheckSecOption(const SecurityOption &secOption)
{
    if (secOption.securityLabel > S4 || secOption.securityLabel < NOT_SET) {
        LOGE("[DBCommon] SecurityLabel is invalid, label is [%d].", secOption.securityLabel);
        return false;
    }
    if (secOption.securityFlag != 0) {
        if ((secOption.securityLabel != S3 && secOption.securityLabel != S4) || secOption.securityFlag != SECE) {
            LOGE("[DBCommon] SecurityFlag is invalid.");
            return false;
        }
    }
    return true;
}

bool ParamCheckUtils::CheckObserver(const Key &key, unsigned int mode)
{
    if (key.size() > DBConstant::MAX_KEY_SIZE) {
        return false;
    }
    uint64_t rawMode = DBCommon::EraseBit(mode, DBConstant::OBSERVER_CHANGES_MASK);
    if (rawMode == OBSERVER_CHANGES_NATIVE || rawMode == OBSERVER_CHANGES_FOREIGN ||
        rawMode == OBSERVER_CHANGES_LOCAL_ONLY || rawMode == OBSERVER_CHANGES_CLOUD ||
        rawMode == (OBSERVER_CHANGES_NATIVE | OBSERVER_CHANGES_FOREIGN)) {
            return true;
    }
    return false;
}

bool ParamCheckUtils::IsS3SECEOpt(const SecurityOption &secOpt)
{
    SecurityOption S3SeceOpt = {SecurityLabel::S3, SecurityFlag::SECE};
    return (secOpt == S3SeceOpt);
}

int ParamCheckUtils::CheckAndTransferAutoLaunchParam(const AutoLaunchParam &param, bool checkDir,
    SchemaObject &schemaObject, std::string &canonicalDir)
{
    if ((param.option.notifier && !ParamCheckUtils::CheckConflictNotifierType(param.option.conflictType)) ||
        (!param.option.notifier && param.option.conflictType != 0)) {
        LOGE("[AutoLaunch] CheckConflictNotifierType is invalid.");
        return -E_INVALID_ARGS;
    }
    if (!ParamCheckUtils::CheckStoreParameter(param.storeId, param.appId, param.userId)) {
        LOGE("[AutoLaunch] CheckStoreParameter is invalid.");
        return -E_INVALID_ARGS;
    }

    const AutoLaunchOption &option = param.option;
    if (!ParamCheckUtils::CheckSecOption(option.secOption)) {
        LOGE("[AutoLaunch] CheckSecOption is invalid.");
        return -E_INVALID_ARGS;
    }

    if (option.isEncryptedDb) {
        if (!ParamCheckUtils::CheckEncryptedParameter(option.cipher, option.passwd)) {
            LOGE("[AutoLaunch] CheckEncryptedParameter is invalid.");
            return -E_INVALID_ARGS;
        }
    }

    if (!param.option.schema.empty()) {
        schemaObject.ParseFromSchemaString(param.option.schema);
        if (!schemaObject.IsSchemaValid()) {
            LOGE("[AutoLaunch] ParseFromSchemaString is invalid.");
            return -E_INVALID_SCHEMA;
        }
    }

    if (!checkDir) {
        canonicalDir = param.option.dataDir;
        return E_OK;
    }

    if (!ParamCheckUtils::CheckDataDir(param.option.dataDir, canonicalDir)) {
        LOGE("[AutoLaunch] CheckDataDir is invalid.");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

uint8_t ParamCheckUtils::GetValidCompressionRate(uint8_t compressionRate)
{
    // Valid when between 1 and 100. When compressionRate is invalid, change it to default rate.
    if (compressionRate < 1 || compressionRate > DBConstant::DEFAULT_COMPTRESS_RATE) {
        LOGD("Invalid compression rate:%" PRIu8, compressionRate);
        compressionRate = DBConstant::DEFAULT_COMPTRESS_RATE;
    }
    return compressionRate;
}

bool ParamCheckUtils::CheckRelationalTableName(const std::string &tableName)
{
    if (!DBCommon::CheckIsAlnumOrUnderscore(tableName)) {
        return false;
    }
    return tableName.compare(0, DBConstant::SYSTEM_TABLE_PREFIX.size(), DBConstant::SYSTEM_TABLE_PREFIX) != 0;
}

bool ParamCheckUtils::CheckTableReference(const std::vector<TableReferenceProperty> &tableReferenceProperty)
{
    if (tableReferenceProperty.empty()) {
        LOGI("[CheckTableReference] tableReferenceProperty is empty");
        return true;
    }

    std::vector<std::vector<int>> dependency;
    std::map<std::string, int, CaseInsensitiveComparator> tableName2Int;
    int index = 0;
    for (const auto &item : tableReferenceProperty) {
        if (item.sourceTableName.empty() || item.targetTableName.empty() || item.columns.empty()) {
            LOGE("[CheckTableReference] table name or column is empty");
            return false;
        }
        std::vector<int> vec;
        for (const auto &tableName : { item.sourceTableName, item.targetTableName }) {
            if (tableName2Int.find(tableName) != tableName2Int.end()) {
                vec.push_back(tableName2Int.at(tableName));
            } else {
                vec.push_back(index);
                tableName2Int[tableName] = index;
                index++;
            }
        }
        if (std::find(dependency.begin(), dependency.end(), vec) != dependency.end()) {
            LOGE("[CheckTableReference] set multiple reference for two tables is not support.");
            return false;
        }
        dependency.emplace_back(vec);
    }

    if (DBCommon::IsCircularDependency(index, dependency)) {
        LOGE("[CheckTableReference] circular reference is not support.");
        return false;
    }
    return true;
}

bool ParamCheckUtils::CheckSharedTableName(const DataBaseSchema &schema)
{
    DataBaseSchema lowerSchema = schema;
    TransferSchemaToLower(lowerSchema);
    std::set<std::string> tableNames;
    std::set<std::string> sharedTableNames;
    for (const auto &tableSchema : lowerSchema.tables) {
        if (tableSchema.sharedTableName.empty()) {
            continue;
        }
        if (tableSchema.sharedTableName == tableSchema.name) {
            LOGE("[CheckSharedTableName] Shared table name and table name are same.");
            return false;
        }
        if (sharedTableNames.find(tableSchema.sharedTableName) != sharedTableNames.end() ||
            sharedTableNames.find(tableSchema.name) != sharedTableNames.end() ||
            tableNames.find(tableSchema.sharedTableName) != tableNames.end() ||
            tableNames.find(tableSchema.name) != tableNames.end()) {
            LOGE("[CheckSharedTableName] Shared table names or table names are duplicate.");
            return false;
        }
        if (!CheckRelationalTableName(tableSchema.sharedTableName)) {
            return false;
        }
        tableNames.insert(tableSchema.name);
        sharedTableNames.insert(tableSchema.sharedTableName);
        std::set<std::string> fields;
        for (const auto &field : tableSchema.fields) {
            if (fields.find(field.colName) != fields.end() || field.colName == CloudDbConstant::CLOUD_OWNER ||
                field.colName == CloudDbConstant::CLOUD_PRIVILEGE) {
                LOGE("[CheckSharedTableName] fields are duplicate.");
                return false;
            }
            fields.insert(field.colName);
        }
    }
    return true;
}

void ParamCheckUtils::TransferSchemaToLower(DataBaseSchema &schema)
{
    for (auto &tableSchema : schema.tables) {
        std::transform(tableSchema.name.begin(), tableSchema.name.end(), tableSchema.name.begin(), tolower);
        std::transform(tableSchema.sharedTableName.begin(), tableSchema.sharedTableName.end(),
            tableSchema.sharedTableName.begin(), tolower);
        for (auto &field : tableSchema.fields) {
            std::transform(field.colName.begin(), field.colName.end(), field.colName.begin(), tolower);
        }
    }
}
} // namespace DistributedDB