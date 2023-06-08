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
    TableSchema cloudTableSchema;
    int ret = GetCloudTableSchema(tableName, cloudTableSchema);
    if (ret != E_OK) {
        LOGE("Cloud schema does not contain certain table:%d", -E_SCHEMA_MISMATCH);
        return -E_SCHEMA_MISMATCH;
    }
    TableInfo tableInfo = localSchema.GetTable(tableName);
    if (tableInfo.Empty()) {
        LOGE("Local schema does not contain certain table:%d", -E_SCHEMA_MISMATCH);
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
            LOGE("Column name mismatch between local and cloud schema: %d", -E_SCHEMA_MISMATCH);
            return -E_SCHEMA_MISMATCH;
        }
        FieldInfo &localField = localFields[cloudField.colName];
        if (!CompareType(localField, cloudField)) {
            LOGE("Type mismatch between local and cloud schema : %d", -E_SCHEMA_MISMATCH);
            return -E_SCHEMA_MISMATCH;
        }
        if (!CompareNullable(localField, cloudField)) {
            LOGE("The nullable property is mismatched between local and cloud schema : %d", -E_SCHEMA_MISMATCH);
            return -E_SCHEMA_MISMATCH;
        }
        if (!CompareIsPrimary(primaryKeys, cloudField)) {
            LOGE("The primary key property is mismatched between local and cloud schema : %d", -E_SCHEMA_MISMATCH);
            return -E_SCHEMA_MISMATCH;
        }
        cloudColNames.emplace(cloudField.colName);
    }
    if (primaryKeys.size() > 0 && !(primaryKeys.size() == 1 && primaryKeys[0] == CloudDbConstant::ROW_ID_FIELD_NAME)) {
        LOGE("Local schema contain extra primary key:%d", -E_SCHEMA_MISMATCH);
        return -E_SCHEMA_MISMATCH;
    }
    for (const auto &[fieldName, fieldInfo] : localFields) {
        if (!fieldInfo.HasDefaultValue() &&
            fieldInfo.IsNotNull() &&
            cloudColNames.find(fieldName) == cloudColNames.end()) {
            LOGE("Column from local schema is not within cloud schema but doesn't have default value : %d",
                -E_SCHEMA_MISMATCH);
            return -E_SCHEMA_MISMATCH;
        }
    }
    return E_OK;
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
    return false;
}

bool SchemaMgr::CompareNullable(const FieldInfo &localField, const Field &cloudField)
{
    return localField.IsNotNull() == !cloudField.nullable;
}

bool SchemaMgr::CompareIsPrimary(std::map<int, FieldName> &localPrimaryKeys, const Field &cloudField)
{
    // whether the corresponding field in local schema is primary key
    bool isLocalFieldPrimary = false;
    for (const auto &kvPair : localPrimaryKeys) {
        if (kvPair.second == cloudField.colName) {
            isLocalFieldPrimary = true;
            localPrimaryKeys.erase(kvPair.first);
            break;
        }
    }
    return isLocalFieldPrimary == cloudField.primary;
}

void SchemaMgr::SetCloudDbSchema(const DataBaseSchema &schema)
{
    cloudSchema_ = std::make_shared<DataBaseSchema>(schema);
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
        if (tableSchema.name == tableName) {
            retSchema = tableSchema;
            return E_OK;
        }
    }
    return -E_NOT_FOUND;
}

} // namespace DistributedDB