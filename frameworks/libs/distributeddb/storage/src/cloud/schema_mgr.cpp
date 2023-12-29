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

#include "cloud/schema_mgr.h"

#include <unordered_set>

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_store_types.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "db_errno.h"
namespace DistributedDB {
SchemaMgr::SchemaMgr()
{
}

int SchemaMgr::ChkSchema(const TableName &tableName, RelationalSchemaObject &localSchema)
{
    if (cloudSchema_ == nullptr) {
        LOGE("Cloud schema has not been set");
        return -E_SCHEMA_MISMATCH;
    }
    TableInfo tableInfo = localSchema.GetTable(tableName);
    if (tableInfo.Empty()) {
        LOGE("Local schema does not contain certain table");
        return -E_SCHEMA_MISMATCH;
    }
    if (tableInfo.GetTableSyncType() != TableSyncType::CLOUD_COOPERATION) {
        LOGE("Sync type of local table is not CLOUD_COOPERATION");
        return -E_NOT_SUPPORT;
    }
    TableSchema cloudTableSchema;
    int ret = GetCloudTableSchema(tableName, cloudTableSchema);
    if (ret != E_OK) {
        LOGE("Cloud schema does not contain certain table:%d", ret);
        return -E_SCHEMA_MISMATCH;
    }
    std::map<int, FieldName> primaryKeys = tableInfo.GetPrimaryKey();
    FieldInfoMap localFields = tableInfo.GetFields();
    return CompareFieldSchema(primaryKeys, localFields, cloudTableSchema.fields);
}

int SchemaMgr::CompareFieldSchema(std::map<int, FieldName> &primaryKeys, FieldInfoMap &localFields,
    std::vector<Field> &cloudFields)
{
    std::unordered_set<std::string> cloudColNames;
    for (const Field &cloudField : cloudFields) {
        if (localFields.find(cloudField.colName) == localFields.end()) {
            LOGE("Column name mismatch between local and cloud schema");
            return -E_SCHEMA_MISMATCH;
        }
        if (IsAssetPrimaryField(cloudField)) {
            LOGE("Asset type can not be primary field");
            return -E_SCHEMA_MISMATCH;
        }
        FieldInfo &localField = localFields[cloudField.colName];
        if (!CompareType(localField, cloudField)) {
            LOGE("Type mismatch between local and cloud schema");
            return -E_SCHEMA_MISMATCH;
        }
        if (!CompareNullable(localField, cloudField)) {
            LOGE("The nullable property is mismatched between local and cloud schema");
            return -E_SCHEMA_MISMATCH;
        }
        if (!ComparePrimaryField(primaryKeys, cloudField)) {
            LOGE("The primary key property is mismatched between local and cloud schema");
            return -E_SCHEMA_MISMATCH;
        }
        cloudColNames.emplace(cloudField.colName);
    }
    if (!primaryKeys.empty() && !(primaryKeys.size() == 1 && primaryKeys[0] == CloudDbConstant::ROW_ID_FIELD_NAME)) {
        LOGE("Local schema contain extra primary key:%d", -E_SCHEMA_MISMATCH);
        return -E_SCHEMA_MISMATCH;
    }
    for (const auto &[fieldName, fieldInfo] : localFields) {
        if (!fieldInfo.HasDefaultValue() &&
            fieldInfo.IsNotNull() &&
            cloudColNames.find(fieldName) == cloudColNames.end()) {
            LOGE("Column from local schema is not within cloud schema but doesn't have default value");
            return -E_SCHEMA_MISMATCH;
        }
    }
    return E_OK;
}

bool SchemaMgr::IsAssetPrimaryField(const Field &cloudField)
{
    return cloudField.primary && (cloudField.type == TYPE_INDEX<Assets> || cloudField.type == TYPE_INDEX<Asset>);
}

bool SchemaMgr::CompareType(const FieldInfo &localField, const Field &cloudField)
{
    StorageType localType = localField.GetStorageType();
    switch (cloudField.type) {
        case TYPE_INDEX<std::monostate>:
        case TYPE_INDEX<bool>:
            // BOOL type should be stored as NUMERIC type,
            // but we regard it as NULL type for historic reason
            return localType == StorageType::STORAGE_TYPE_NULL;
        case TYPE_INDEX<int64_t>:
            return localType == StorageType::STORAGE_TYPE_INTEGER;
        case TYPE_INDEX<double>:
            return localType == StorageType::STORAGE_TYPE_REAL;
        case TYPE_INDEX<std::string>:
            return localType == StorageType::STORAGE_TYPE_TEXT;
        case TYPE_INDEX<Bytes>:
        case TYPE_INDEX<Asset>:
        case TYPE_INDEX<Assets>:
            return localType == StorageType::STORAGE_TYPE_BLOB;
        default:
            return false;
    }
}

bool SchemaMgr::CompareNullable(const FieldInfo &localField, const Field &cloudField)
{
    return localField.IsNotNull() == !cloudField.nullable;
}

bool SchemaMgr::ComparePrimaryField(std::map<int, FieldName> &localPrimaryKeys, const Field &cloudField)
{
    // whether the corresponding field in local schema is primary key
    bool isLocalFieldPrimary = false;
    for (const auto &kvPair : localPrimaryKeys) {
        if (DBCommon::CaseInsensitiveCompare(kvPair.second, cloudField.colName)) {
            isLocalFieldPrimary = true;
            localPrimaryKeys.erase(kvPair.first);
            break;
        }
    }
    return isLocalFieldPrimary == cloudField.primary;
}

void SchemaMgr::SetCloudDbSchema(const DataBaseSchema &schema)
{
    DataBaseSchema cloudSchema = schema;
    DataBaseSchema cloudSharedSchema;
    for (const auto &tableSchema : cloudSchema.tables) {
        if (tableSchema.name.empty() || tableSchema.sharedTableName.empty()) {
            continue;
        }
        bool hasPrimaryKey = DBCommon::HasPrimaryKey(tableSchema.fields);
        auto sharedTableFields = tableSchema.fields;
        Field ownerField = { CloudDbConstant::CLOUD_OWNER, TYPE_INDEX<std::string>, hasPrimaryKey, true };
        Field privilegeField = { CloudDbConstant::CLOUD_PRIVILEGE, TYPE_INDEX<std::string>, false, true };
        sharedTableFields.push_back(ownerField);
        sharedTableFields.push_back(privilegeField);
        TableSchema sharedTableSchema = { tableSchema.sharedTableName, tableSchema.sharedTableName, sharedTableFields };
        cloudSharedSchema.tables.push_back(sharedTableSchema);
    }
    for (const auto &sharedTableSchema : cloudSharedSchema.tables) {
        cloudSchema.tables.push_back(sharedTableSchema);
    }
    cloudSchema_ = std::make_shared<DataBaseSchema>(cloudSchema);
}

std::shared_ptr<DataBaseSchema> SchemaMgr::GetCloudDbSchema()
{
    return cloudSchema_;
}

int SchemaMgr::GetCloudTableSchema(const TableName &tableName, TableSchema &retSchema)
{
    if (cloudSchema_ == nullptr) {
        return -E_SCHEMA_MISMATCH;
    }
    for (const TableSchema &tableSchema : cloudSchema_->tables) {
        if (DBCommon::CaseInsensitiveCompare(tableSchema.name, tableName)) {
            retSchema = tableSchema;
            return E_OK;
        }
    }
    return -E_NOT_FOUND;
}

bool SchemaMgr::IsSharedTable(const std::string &tableName)
{
    if (cloudSchema_ == nullptr) {
        return false;
    }
    if (sharedTableMap_.find(tableName) != sharedTableMap_.end()) {
        return sharedTableMap_[tableName];
    }
    for (const auto &tableSchema : (*cloudSchema_).tables) {
        if (DBCommon::CaseInsensitiveCompare(tableName, tableSchema.sharedTableName)) {
            sharedTableMap_[tableName] = true;
            return true;
        }
    }
    sharedTableMap_[tableName] = false;
    return false;
}

std::map<std::string, std::string> SchemaMgr::GetSharedTableOriginNames()
{
    if (cloudSchema_ == nullptr) {
        LOGW("[SchemaMgr] Not found cloud schema!");
        return {};
    }
    std::map<std::string, std::string> res;
    for (const auto &item : cloudSchema_->tables) {
        if (item.sharedTableName.empty() || CloudStorageUtils::IsSharedTable(item)) {
            continue;
        }
        res[item.name] = item.sharedTableName;
    }
    return res;
}
} // namespace DistributedDB