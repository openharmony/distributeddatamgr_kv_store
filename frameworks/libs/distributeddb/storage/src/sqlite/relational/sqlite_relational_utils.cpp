/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "sqlite_relational_utils.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "db_errno.h"
#include "res_finalizer.h"
#include "runtime_context.h"
#include "sqlite_utils.h"
#include "time_helper.h"

namespace DistributedDB {
namespace {
#ifdef USE_DISTRIBUTEDDB_CLOUD
    constexpr const char *UPLOAD_CLOUD_UNFINISHED = "~0x400";
#endif
}
int SQLiteRelationalUtils::GetDataValueByType(sqlite3_stmt *statement, int cid, DataValue &value)
{
    if (statement == nullptr || cid < 0 || cid >= sqlite3_column_count(statement)) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    int storageType = sqlite3_column_type(statement, cid);
    switch (storageType) {
        case SQLITE_INTEGER:
            value = static_cast<int64_t>(sqlite3_column_int64(statement, cid));
            break;
        case SQLITE_FLOAT:
            value = sqlite3_column_double(statement, cid);
            break;
        case SQLITE_BLOB: {
            std::vector<uint8_t> blobValue;
            errCode = SQLiteUtils::GetColumnBlobValue(statement, cid, blobValue);
            if (errCode != E_OK) {
                return errCode;
            }
            auto blob = new (std::nothrow) Blob;
            if (blob == nullptr) {
                return -E_OUT_OF_MEMORY;
            }
            blob->WriteBlob(blobValue.data(), static_cast<uint32_t>(blobValue.size()));
            errCode = value.Set(blob);
            if (errCode != E_OK) {
                delete blob;
                blob = nullptr;
            }
            break;
        }
        case SQLITE_NULL:
            break;
        case SQLITE3_TEXT: {
            std::string str;
            (void)SQLiteUtils::GetColumnTextValue(statement, cid, str);
            value = str;
            if (value.GetType() != StorageType::STORAGE_TYPE_TEXT) {
                errCode = -E_OUT_OF_MEMORY;
            }
            break;
        }
        default:
            break;
    }
    return errCode;
}

std::vector<DataValue> SQLiteRelationalUtils::GetSelectValues(sqlite3_stmt *stmt)
{
    std::vector<DataValue> values;
    for (int cid = 0, colCount = sqlite3_column_count(stmt); cid < colCount; ++cid) {
        DataValue value;
        (void)GetDataValueByType(stmt, cid, value);
        values.emplace_back(std::move(value));
    }
    return values;
}

int SQLiteRelationalUtils::GetCloudValueByType(sqlite3_stmt *statement, int type, int cid, Type &cloudValue)
{
    if (statement == nullptr || cid < 0 || cid >= sqlite3_column_count(statement)) {
        return -E_INVALID_ARGS;
    }
    switch (sqlite3_column_type(statement, cid)) {
        case SQLITE_INTEGER: {
            if (type == TYPE_INDEX<bool>) {
                cloudValue = static_cast<bool>(sqlite3_column_int(statement, cid));
                break;
            }
            cloudValue = static_cast<int64_t>(sqlite3_column_int64(statement, cid));
            break;
        }
        case SQLITE_FLOAT: {
            cloudValue = sqlite3_column_double(statement, cid);
            break;
        }
        case SQLITE_BLOB: {
            std::vector<uint8_t> blobValue;
            int errCode = SQLiteUtils::GetColumnBlobValue(statement, cid, blobValue);
            if (errCode != E_OK) {
                return errCode;
            }
            cloudValue = blobValue;
            break;
        }
        case SQLITE3_TEXT: {
            bool isBlob = (type == TYPE_INDEX<Bytes> || type == TYPE_INDEX<Asset> || type == TYPE_INDEX<Assets>);
            if (isBlob) {
                std::vector<uint8_t> blobValue;
                int errCode = SQLiteUtils::GetColumnBlobValue(statement, cid, blobValue);
                if (errCode != E_OK) {
                    return errCode;
                }
                cloudValue = blobValue;
                break;
            }
            std::string str;
            (void)SQLiteUtils::GetColumnTextValue(statement, cid, str);
            cloudValue = str;
            break;
        }
        default: {
            cloudValue = Nil();
        }
    }
    return E_OK;
}

void SQLiteRelationalUtils::CalCloudValueLen(Type &cloudValue, uint32_t &totalSize)
{
    switch (cloudValue.index()) {
        case TYPE_INDEX<int64_t>:
            totalSize += sizeof(int64_t);
            break;
        case TYPE_INDEX<double>:
            totalSize += sizeof(double);
            break;
        case TYPE_INDEX<std::string>:
            totalSize += std::get<std::string>(cloudValue).size();
            break;
        case TYPE_INDEX<bool>:
            totalSize += sizeof(int32_t);
            break;
        case TYPE_INDEX<Bytes>:
        case TYPE_INDEX<Asset>:
        case TYPE_INDEX<Assets>:
            totalSize += std::get<Bytes>(cloudValue).size();
            break;
        default: {
            break;
        }
    }
}

int SQLiteRelationalUtils::BindStatementByType(sqlite3_stmt *statement, int cid, Type &typeVal)
{
    int errCode = E_OK;
    switch (typeVal.index()) {
        case TYPE_INDEX<int64_t>: {
            int64_t value = 0;
            (void)CloudStorageUtils::GetValueFromType(typeVal, value);
            errCode = SQLiteUtils::BindInt64ToStatement(statement, cid, value);
            break;
        }
        case TYPE_INDEX<bool>: {
            bool value = false;
            (void)CloudStorageUtils::GetValueFromType<bool>(typeVal, value);
            errCode = SQLiteUtils::BindInt64ToStatement(statement, cid, value);
            break;
        }
        case TYPE_INDEX<double>: {
            double value = 0.0;
            (void)CloudStorageUtils::GetValueFromType<double>(typeVal, value);
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_double(statement, cid, value));
            break;
        }
        case TYPE_INDEX<std::string>: {
            std::string value;
            (void)CloudStorageUtils::GetValueFromType<std::string>(typeVal, value);
            errCode = SQLiteUtils::BindTextToStatement(statement, cid, value);
            break;
        }
        default: {
            errCode = BindExtendStatementByType(statement, cid, typeVal);
            break;
        }
    }
    return errCode;
}

int SQLiteRelationalUtils::BindExtendStatementByType(sqlite3_stmt *statement, int cid, Type &typeVal)
{
    int errCode = E_OK;
    switch (typeVal.index()) {
        case TYPE_INDEX<Bytes>: {
            Bytes value;
            (void)CloudStorageUtils::GetValueFromType<Bytes>(typeVal, value);
            errCode = SQLiteUtils::BindBlobToStatement(statement, cid, value);
            break;
        }
        case TYPE_INDEX<Asset>: {
            Asset value;
            (void)CloudStorageUtils::GetValueFromType<Asset>(typeVal, value);
            Bytes val;
            errCode = RuntimeContext::GetInstance()->AssetToBlob(value, val);
            if (errCode != E_OK) {
                break;
            }
            errCode = SQLiteUtils::BindBlobToStatement(statement, cid, val);
            break;
        }
        case TYPE_INDEX<Assets>: {
            Assets value;
            (void)CloudStorageUtils::GetValueFromType<Assets>(typeVal, value);
            Bytes val;
            errCode = RuntimeContext::GetInstance()->AssetsToBlob(value, val);
            if (errCode != E_OK) {
                break;
            }
            errCode = SQLiteUtils::BindBlobToStatement(statement, cid, val);
            break;
        }
        default: {
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(statement, cid));
            break;
        }
    }
    return errCode;
}

int SQLiteRelationalUtils::GetSelectVBucket(sqlite3_stmt *stmt, VBucket &bucket)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    for (int cid = 0, colCount = sqlite3_column_count(stmt); cid < colCount; ++cid) {
        Type typeVal;
        int errCode = GetTypeValByStatement(stmt, cid, typeVal);
        if (errCode != E_OK) {
            LOGE("get typeVal from stmt failed");
            return errCode;
        }
        const char *colName = sqlite3_column_name(stmt, cid);
        bucket.insert_or_assign(colName, std::move(typeVal));
    }
    return E_OK;
}

bool SQLiteRelationalUtils::GetDbFileName(sqlite3 *db, std::string &fileName)
{
    if (db == nullptr) {
        return false;
    }

    auto dbFilePath = sqlite3_db_filename(db, nullptr);
    if (dbFilePath == nullptr) {
        return false;
    }
    fileName = std::string(dbFilePath);
    return true;
}

int SQLiteRelationalUtils::GetTypeValByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal)
{
    if (stmt == nullptr || cid < 0 || cid >= sqlite3_column_count(stmt)) {
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    switch (sqlite3_column_type(stmt, cid)) {
        case SQLITE_INTEGER: {
            const char *declType = sqlite3_column_decltype(stmt, cid);
            if (declType == nullptr) { // LCOV_EXCL_BR_LINE
                typeVal = static_cast<int64_t>(sqlite3_column_int64(stmt, cid));
                break;
            }
            if (strcasecmp(declType, SchemaConstant::KEYWORD_TYPE_BOOL) == 0 ||
                strcasecmp(declType, SchemaConstant::KEYWORD_TYPE_BOOLEAN) == 0) { // LCOV_EXCL_BR_LINE
                typeVal = static_cast<bool>(sqlite3_column_int(stmt, cid));
                break;
            }
            typeVal = static_cast<int64_t>(sqlite3_column_int64(stmt, cid));
            break;
        }
        case SQLITE_FLOAT: {
            typeVal = sqlite3_column_double(stmt, cid);
            break;
        }
        case SQLITE_BLOB: {
            errCode = GetBlobByStatement(stmt, cid, typeVal);
            break;
        }
        case SQLITE3_TEXT: {
            errCode = GetBlobByStatement(stmt, cid, typeVal);
            if (errCode != E_OK || typeVal.index() != TYPE_INDEX<Nil>) { // LCOV_EXCL_BR_LINE
                break;
            }
            std::string str;
            (void)SQLiteUtils::GetColumnTextValue(stmt, cid, str);
            typeVal = str;
            break;
        }
        default: {
            typeVal = Nil();
        }
    }
    return errCode;
}

int SQLiteRelationalUtils::GetBlobByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal)
{
    const char *declType = sqlite3_column_decltype(stmt, cid);
    int errCode = E_OK;
    if (declType != nullptr && strcasecmp(declType, CloudDbConstant::ASSET) == 0) { // LCOV_EXCL_BR_LINE
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, cid, blobValue);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            return errCode;
        }
        Asset asset;
        errCode = RuntimeContext::GetInstance()->BlobToAsset(blobValue, asset);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            return errCode;
        }
        typeVal = asset;
    } else if (declType != nullptr && strcasecmp(declType, CloudDbConstant::ASSETS) == 0) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, cid, blobValue);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            return errCode;
        }
        Assets assets;
        errCode = RuntimeContext::GetInstance()->BlobToAssets(blobValue, assets);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            return errCode;
        }
        typeVal = assets;
    } else if (sqlite3_column_type(stmt, cid) == SQLITE_BLOB) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, cid, blobValue);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            return errCode;
        }
        typeVal = blobValue;
    }
    return E_OK;
}

int SQLiteRelationalUtils::SelectServerObserver(sqlite3 *db, const std::string &tableName, bool isChanged)
{
    if (db == nullptr || tableName.empty()) {
        return -E_INVALID_ARGS;
    }
    std::string sql;
    if (isChanged) {
        sql = "SELECT server_observer('" + tableName + "', 1);";
    } else {
        sql = "SELECT server_observer('" + tableName + "', 0);";
    }
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("get select server observer stmt failed. %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("select server observer failed. %d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }
    return ret == E_OK ? E_OK : ret;
}

void SQLiteRelationalUtils::AddUpgradeSqlToList(const TableInfo &tableInfo,
    const std::vector<std::pair<std::string, std::string>> &fieldList, std::vector<std::string> &sqlList)
{
    for (const auto &[colName, colType] : fieldList) {
        auto it = tableInfo.GetFields().find(colName);
        if (it != tableInfo.GetFields().end()) {
            continue;
        }
        sqlList.push_back("alter table " + tableInfo.GetTableName() + " add " + colName +
            " " + colType + ";");
    }
}

int SQLiteRelationalUtils::QueryCount(sqlite3 *db, const std::string &tableName, int64_t &count)
{
    std::string sql = "SELECT COUNT(1) FROM " + tableName ;
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Query count failed. %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        count = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
        errCode = E_OK;
    } else {
        LOGE("Failed to get the count. %d", errCode);
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteRelationalUtils::GetCursor(sqlite3 *db, const std::string &tableName, uint64_t &cursor)
{
    cursor = DBConstant::INVALID_CURSOR;
    std::string sql = "SELECT value FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata where key = ?;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] Get cursor of table[%s length[%u]] failed=%d",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.length(), errCode);
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("Reset stmt failed %d when get cursor", ret);
        }
    });
    Key key;
    DBCommon::StringToVector(DBCommon::GetCursorKey(tableName), key);
    errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, key, false); // first arg.
    if (errCode != E_OK) {
        LOGE("[Storage Executor] Bind failed when get cursor of table[%s length[%u]] failed=%d",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.length(), errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        int64_t tmpCursor = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
        if (tmpCursor >= 0) {
            cursor = static_cast<uint64_t>(tmpCursor);
        }
    }
    return cursor == DBConstant::INVALID_CURSOR ? errCode : E_OK;
}

void GetFieldsNeedContain(const TableInfo &tableInfo, const std::vector<FieldInfo> &syncFields,
    std::set<std::string> &fieldsNeedContain, std::set<std::string> &fieldsNotDecrease,
    std::set<std::string> &requiredNotNullFields)
{
    // should not decrease distributed field
    for (const auto &field : tableInfo.GetSyncField()) {
        fieldsNotDecrease.insert(field);
    }
    const std::vector<CompositeFields> &uniqueDefines = tableInfo.GetUniqueDefine();
    for (const auto &compositeFields : uniqueDefines) {
        for (const auto &fieldName : compositeFields) {
            if (tableInfo.IsPrimaryKey(fieldName)) {
                continue;
            }
            fieldsNeedContain.insert(fieldName);
        }
    }
    const FieldInfoMap &fieldInfoMap = tableInfo.GetFields();
    for (const auto &entry : fieldInfoMap) {
        const FieldInfo &fieldInfo = entry.second;
        if (fieldInfo.IsNotNull() && fieldInfo.GetDefaultValue().empty()) {
            requiredNotNullFields.insert(fieldInfo.GetFieldName());
        }
    }
}

bool CheckRequireFieldsInMap(const std::set<std::string> &fieldsNeedContain,
    const std::map<std::string, bool> &fieldsMap)
{
    for (auto &fieldNeedContain : fieldsNeedContain) {
        if (fieldsMap.find(fieldNeedContain) == fieldsMap.end()) {
            LOGE("Required column[%s [%zu]] not found", DBCommon::StringMiddleMasking(fieldNeedContain).c_str(),
                fieldNeedContain.size());
            return false;
        }
        if (!fieldsMap.at(fieldNeedContain)) {
            LOGE("The isP2pSync of required column[%s [%zu]] is false",
                DBCommon::StringMiddleMasking(fieldNeedContain).c_str(), fieldNeedContain.size());
            return false;
        }
    }
    return true;
}

bool IsMarkUniqueColumnInvalid(const TableInfo &tableInfo, const std::vector<DistributedField> &originFields)
{
    int count = 0;
    for (const auto &field : originFields) {
        if (field.isSpecified && tableInfo.IsUniqueField(field.colName)) {
            count++;
            if (count > 1) {
                return true;
            }
        }
    }
    return false;
}

bool IsDistributedPKInvalidInAutoIncrementTable(const TableInfo &tableInfo,
    const std::set<std::string, CaseInsensitiveComparator> &distributedPk,
    const std::vector<DistributedField> &originFields)
{
    if (distributedPk.empty()) {
        return false;
    }
    auto uniqueAndPkDefine = tableInfo.GetUniqueAndPkDefine();
    if (IsMarkUniqueColumnInvalid(tableInfo, originFields)) {
        LOGE("Mark more than one unique column specified in auto increment table: %s, tableName len: %zu",
            DBCommon::StringMiddleMasking(tableInfo.GetTableName()).c_str(), tableInfo.GetTableName().length());
        return true;
    }
    auto find = std::any_of(uniqueAndPkDefine.begin(), uniqueAndPkDefine.end(), [&distributedPk](const auto &item) {
        // unique index field count should be same
        return item.size() == distributedPk.size() && distributedPk == DBCommon::TransformToCaseInsensitive(item);
    });
    bool isMissMatch = !find;
    if (isMissMatch) {
        LOGE("Miss match distributed pk size %zu in auto increment table %s", distributedPk.size(),
            DBCommon::StringMiddleMasking(tableInfo.GetTableName()).c_str());
    }
    return isMissMatch;
}

bool IsDistributedPkInvalid(const TableInfo &tableInfo,
    const std::set<std::string, CaseInsensitiveComparator> &distributedPk,
    const std::vector<DistributedField> &originFields, bool isForceUpgrade)
{
    auto lastDistributedPk = DBCommon::TransformToCaseInsensitive(tableInfo.GetSyncDistributedPk());
    if (!isForceUpgrade && !lastDistributedPk.empty() && distributedPk != lastDistributedPk) {
        LOGE("distributed pk has change last %zu now %zu", lastDistributedPk.size(), distributedPk.size());
        return true;
    }
    // check pk is same or local pk is auto increase and set pk is unique
    if (tableInfo.IsNoPkTable()) {
        return false;
    }
    if (!distributedPk.empty() && distributedPk.size() != tableInfo.GetPrimaryKey().size()) {
        return true;
    }
    if (tableInfo.GetAutoIncrement()) {
        return IsDistributedPKInvalidInAutoIncrementTable(tableInfo, distributedPk, originFields);
    }
    for (const auto &field : originFields) {
        bool isLocalPk = tableInfo.IsPrimaryKey(field.colName);
        if (field.isSpecified && !isLocalPk) {
            LOGE("Column[%s [%zu]] is not primary key but mark specified",
                DBCommon::StringMiddleMasking(field.colName).c_str(), field.colName.size());
            return true;
        }
        if (isLocalPk && !field.isP2pSync) {
            LOGE("Column[%s [%zu]] is primary key but set isP2pSync false",
                DBCommon::StringMiddleMasking(field.colName).c_str(), field.colName.size());
            return true;
        }
    }
    return false;
}

bool IsDistributedSchemaSupport(const TableInfo &tableInfo, const std::vector<DistributedField> &fields)
{
    if (!tableInfo.GetAutoIncrement()) {
        return true;
    }
    bool isSyncPk = false;
    bool isSyncOtherSpecified = false;
    for (const auto &item : fields) {
        if (tableInfo.IsPrimaryKey(item.colName) && item.isP2pSync) {
            isSyncPk = true;
        } else if (item.isSpecified && item.isP2pSync) {
            isSyncOtherSpecified = true;
        }
    }
    if (isSyncPk && isSyncOtherSpecified) {
        LOGE("Not support sync with auto increment pk and other specified col");
        return false;
    }
    return true;
}

int CheckDistributedSchemaFields(const TableInfo &tableInfo, const std::vector<FieldInfo> &syncFields,
    const std::vector<DistributedField> &fields, bool isForceUpgrade)
{
    if (fields.empty()) {
        LOGE("fields cannot be empty");
        return -E_SCHEMA_MISMATCH;
    }
    if (!IsDistributedSchemaSupport(tableInfo, fields)) {
        return -E_NOT_SUPPORT;
    }
    std::set<std::string, CaseInsensitiveComparator> distributedPk;
    bool isNoPrimaryKeyTable = tableInfo.IsNoPkTable();
    for (const auto &field : fields) {
        if (!tableInfo.IsFieldExist(field.colName)) {
            LOGE("Column[%s [%zu]] not found in table", DBCommon::StringMiddleMasking(field.colName).c_str(),
                 field.colName.size());
            return -E_SCHEMA_MISMATCH;
        }
        if (isNoPrimaryKeyTable && field.isSpecified) {
            return -E_SCHEMA_MISMATCH;
        }
        if (field.isSpecified && field.isP2pSync) {
            distributedPk.insert(field.colName);
        }
    }
    if (IsDistributedPkInvalid(tableInfo, distributedPk, fields, isForceUpgrade)) {
        return -E_SCHEMA_MISMATCH;
    }
    std::set<std::string> fieldsNeedContain;
    std::set<std::string> fieldsNotDecrease;
    std::set<std::string> requiredNotNullFields;
    GetFieldsNeedContain(tableInfo, syncFields, fieldsNeedContain, fieldsNotDecrease, requiredNotNullFields);
    std::map<std::string, bool> fieldsMap;
    for (auto &field : fields) {
        fieldsMap.insert({field.colName, field.isP2pSync});
    }
    if (!CheckRequireFieldsInMap(fieldsNeedContain, fieldsMap)) {
        LOGE("The required fields are not found in fieldsMap");
        return -E_SCHEMA_MISMATCH;
    }
    if (!isForceUpgrade && !CheckRequireFieldsInMap(fieldsNotDecrease, fieldsMap)) {
        LOGE("The fields should not decrease");
        return -E_DISTRIBUTED_FIELD_DECREASE;
    }
    if (!CheckRequireFieldsInMap(requiredNotNullFields, fieldsMap)) {
        LOGE("The required not-null fields are not found in fieldsMap");
        return -E_SCHEMA_MISMATCH;
    }
    return E_OK;
}

int SQLiteRelationalUtils::CheckDistributedSchemaValid(const RelationalSchemaObject &schemaObj,
    const DistributedSchema &schema, bool isForceUpgrade, SQLiteSingleVerRelationalStorageExecutor *executor)
{
    if (executor == nullptr) {
        LOGE("[RDBUtils][CheckDistributedSchemaValid] executor is null");
        return -E_INVALID_ARGS;
    }
    sqlite3 *db;
    int errCode = executor->GetDbHandle(db);
    if (errCode != E_OK) {
        LOGE("[RDBUtils][CheckDistributedSchemaValid] sqlite handle failed %d", errCode);
        return errCode;
    }
    for (const auto &table : schema.tables) {
        if (table.tableName.empty()) {
            LOGE("[RDBUtils][CheckDistributedSchemaValid] Table name cannot be empty");
            return -E_SCHEMA_MISMATCH;
        }
        TableInfo tableInfo;
        errCode = SQLiteUtils::AnalysisSchema(db, table.tableName, tableInfo);
        if (errCode != E_OK) {
            LOGE("[RDBUtils][CheckDistributedSchemaValid] analyze table %s failed %d",
                DBCommon::StringMiddleMasking(table.tableName).c_str(), errCode);
            return errCode == -E_NOT_FOUND ? -E_SCHEMA_MISMATCH : errCode;
        }
        tableInfo.SetDistributedTable(schemaObj.GetDistributedTable(table.tableName));
        errCode = CheckDistributedSchemaFields(tableInfo, schemaObj.GetSyncFieldInfo(table.tableName, false),
            table.fields, isForceUpgrade);
        if (errCode != E_OK) {
            LOGE("[CheckDistributedSchema] Check fields of [%s [%zu]] fail",
                DBCommon::StringMiddleMasking(table.tableName).c_str(), table.tableName.size());
            return errCode;
        }
    }
    return E_OK;
}

DistributedSchema SQLiteRelationalUtils::FilterRepeatDefine(const DistributedSchema &schema)
{
    DistributedSchema res;
    res.version = schema.version;
    std::set<std::string> tableName;
    std::list<DistributedTable> tableList;
    for (auto it = schema.tables.rbegin();it != schema.tables.rend(); it++) {
        if (tableName.find(it->tableName) != tableName.end()) {
            continue;
        }
        tableName.insert(it->tableName);
        tableList.push_front(FilterRepeatDefine(*it));
    }
    for (auto &item : tableList) {
        res.tables.push_back(std::move(item));
    }
    return res;
}

DistributedTable SQLiteRelationalUtils::FilterRepeatDefine(const DistributedTable &table)
{
    DistributedTable res;
    res.tableName = table.tableName;
    std::set<std::string> fieldName;
    std::list<DistributedField> fieldList;
    for (auto it = table.fields.rbegin();it != table.fields.rend(); it++) {
        if (fieldName.find(it->colName) != fieldName.end()) {
            continue;
        }
        fieldName.insert(it->colName);
        fieldList.push_front(*it);
    }
    for (auto &item : fieldList) {
        res.fields.push_back(std::move(item));
    }
    return res;
}

int SQLiteRelationalUtils::GetLogData(sqlite3_stmt *logStatement, LogInfo &logInfo)
{
    logInfo.dataKey = sqlite3_column_int64(logStatement, 0);  // 0 means dataKey index

    std::vector<uint8_t> dev;
    int errCode = SQLiteUtils::GetColumnBlobValue(logStatement, 1, dev);  // 1 means dev index
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Get dev failed %d", errCode);
        return errCode;
    }
    logInfo.device = std::string(dev.begin(), dev.end());

    std::vector<uint8_t> oriDev;
    errCode = SQLiteUtils::GetColumnBlobValue(logStatement, 2, oriDev);  // 2 means ori_dev index
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Get ori dev failed %d", errCode);
        return errCode;
    }
    logInfo.originDev = std::string(oriDev.begin(), oriDev.end());
    logInfo.timestamp = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 3));  // 3 means timestamp index
    logInfo.wTimestamp = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 4));  // 4 means w_timestamp index
    logInfo.flag = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 5));  // 5 means flag index
    logInfo.flag &= (~DataItem::LOCAL_FLAG);
    logInfo.flag &= (~DataItem::UPDATE_FLAG);
    errCode = SQLiteUtils::GetColumnBlobValue(logStatement, 6, logInfo.hashKey);  // 6 means hashKey index
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Get hashKey failed %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::GetLogInfoPre(sqlite3_stmt *queryStmt, DistributedTableMode mode, const DataItem &dataItem,
    LogInfo &logInfoGet)
{
    if (queryStmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = SQLiteUtils::BindBlobToStatement(queryStmt, 1, dataItem.hashKey);  // 1 means hash key index.
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Bind hashKey failed %d", errCode);
        return errCode;
    }
    if (mode != DistributedTableMode::COLLABORATION) {
        errCode = SQLiteUtils::BindTextToStatement(queryStmt, 2, dataItem.dev);  // 2 means device index.
        if (errCode != E_OK) {
            LOGE("[SQLiteRDBUtils] Bind dev failed %d", errCode);
            return errCode;
        }
    }

    errCode = SQLiteUtils::StepWithRetry(queryStmt, false); // rdb not exist mem db
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = -E_NOT_FOUND;
    } else {
        errCode = SQLiteRelationalUtils::GetLogData(queryStmt, logInfoGet);
    }
    return errCode;
}

int SQLiteRelationalUtils::OperateDataStatus(sqlite3 *db, const std::vector<std::string> &tables, uint64_t virtualTime)
{
    auto time = std::to_string(virtualTime);
    LOGI("[SQLiteRDBUtils] %zu tables wait for update time to %s", tables.size(), time.c_str());
    int errCode = E_OK;
    for (const auto &table : tables) {
        errCode = UpdateLocalDataModifyTime(db, table, time);
        if (errCode != E_OK) {
            LOGE("[SQLiteRDBUtils] %s table len %zu operate failed %d", DBCommon::StringMiddleMasking(table).c_str(),
                table.size(), errCode);
            break;
        }
    }
    if (errCode == E_OK) {
        LOGI("[SQLiteRDBUtils] Operate data all success");
    }
    return errCode;
}

int SQLiteRelationalUtils::UpdateLocalDataModifyTime(sqlite3 *db, const std::string &table,
    const std::string &modifyTime)
{
    auto logTable = DBCommon::GetLogTableName(table);
    bool isCreate = false;
    auto errCode = SQLiteUtils::CheckTableExists(db, logTable, isCreate);
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Check table exist failed %d when update time", errCode);
        return errCode;
    }
    if (!isCreate) {
        LOGW("[SQLiteRDBUtils] Skip non exist log table %s len %zu when update time",
            DBCommon::StringMiddleMasking(table).c_str(), table.size());
        return E_OK;
    }
    std::string operateSql = "UPDATE " + logTable +
        " SET timestamp= _rowid_ + " + modifyTime + " WHERE flag & 0x02 != 0";
    errCode = SQLiteUtils::ExecuteRawSQL(db, operateSql);
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Update %s len %zu modify time failed %d", DBCommon::StringMiddleMasking(table).c_str(),
            table.size(), errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::CleanDirtyLog(sqlite3 *db, const std::string &oriTable, const RelationalSchemaObject &obj)
{
    TableInfo tableInfo;
    tableInfo.SetDistributedTable(obj.GetDistributedTable(oriTable));
    auto distributedPk = tableInfo.GetSyncDistributedPk();
    if (distributedPk.empty()) {
        return E_OK;
    }
    auto logTable = DBCommon::GetLogTableName(oriTable);
    int errCode = E_OK;
    std::string sql = "DELETE FROM " + logTable + " WHERE hash_key IN "
          "(SELECT hash_key FROM (SELECT data_key, hash_key, extend_field FROM " + logTable +
          " WHERE data_key IN(SELECT data_key FROM " + logTable + " GROUP BY data_key HAVING count(1)>1)"
          " AND data_key != -1) AS log, '" + oriTable +
          "' WHERE log.data_key = '" + oriTable + "'.rowid AND log.hash_key != " +
          SqliteLogTableManager::CalcPkHash(oriTable + ".", distributedPk) + ")";
    errCode = ExecuteSql(db, sql, nullptr);
    if (errCode == E_OK) {
        LOGI("[SQLiteRDBUtils] Clean %d dirty hash log", sqlite3_changes(db));
    } else {
        LOGW("[SQLiteRDBUtils][ClearDirtyLog] failed %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::ExecuteListAction(const std::vector<std::function<int()>> &actions)
{
    int errCode = E_OK;
    for (const auto &action : actions) {
        if (action == nullptr) {
            continue;
        }
        errCode = action();
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return errCode;
}

int SQLiteRelationalUtils::GetLocalLogInfo(const RelationalSyncDataInserter &inserter, const DataItem &dataItem,
    DistributedTableMode mode, LogInfo &logInfoGet, SaveSyncDataStmt &saveStmt)
{
    int errCode = SQLiteRelationalUtils::GetLogInfoPre(saveStmt.queryStmt, mode, dataItem, logInfoGet);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(saveStmt.queryStmt, false, ret);
    if (errCode == -E_NOT_FOUND) {
        int res = GetLocalLog(dataItem, inserter, saveStmt.queryByFieldStmt, logInfoGet);
        SQLiteUtils::ResetStatement(saveStmt.queryByFieldStmt, false, ret);
        errCode = (res == E_OK) ? E_OK : errCode;
    }
    return errCode;
}

int SQLiteRelationalUtils::GetLocalLog(const DataItem &dataItem, const RelationalSyncDataInserter &inserter,
    sqlite3_stmt *stmt, LogInfo &logInfo)
{
    if (stmt == nullptr) {
        // no need to get log by distributed pk
        return -E_NOT_FOUND;
    }
    if ((dataItem.flag & DataItem::DELETE_FLAG) != 0 ||
        (dataItem.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) != 0) {
        return -E_NOT_FOUND;
    }
    auto [errCode, distributedPk] = SQLiteRelationalUtils::GetDistributedPk(dataItem, inserter);
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Get distributed pk failed[%d]", errCode);
        return errCode;
    }
    errCode = BindDistributedPk(stmt, inserter, distributedPk);
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] Bind distributed pk stmt failed[%d]", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false); // rdb not exist mem db
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = -E_NOT_FOUND;
    } else {
        errCode = SQLiteRelationalUtils::GetLogData(stmt, logInfo);
    }
    return errCode;
}

std::pair<int, VBucket> SQLiteRelationalUtils::GetDistributedPk(const DataItem &dataItem,
    const RelationalSyncDataInserter &inserter)
{
    std::pair<int, VBucket> res;
    auto &[errCode, distributedPk] = res;
    OptRowDataWithLog data;
    // deserialize by remote field info
    errCode = DataTransformer::DeSerializeDataItem(dataItem, data, inserter.GetRemoteFields());
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] DeSerialize dataItem failed! errCode[%d]", errCode);
        return res;
    }
    size_t dataIdx = 0;
    const auto &localTableFields = inserter.GetLocalTable().GetFields();
    auto pkFields = inserter.GetLocalTable().GetSyncDistributedPk();
    std::set<std::string> distributedPkFields(pkFields.begin(), pkFields.end());
    for (const auto &it : inserter.GetRemoteFields()) {
        if (distributedPkFields.find(it.GetFieldName()) == distributedPkFields.end()) {
            dataIdx++;
            continue;
        }
        if (localTableFields.find(it.GetFieldName()) == localTableFields.end()) {
            LOGW("[SQLiteRDBUtils] field[%s][%zu] not found in local schema.",
                 DBCommon::StringMiddleMasking(it.GetFieldName()).c_str(),
                 it.GetFieldName().size());
            dataIdx++;
            continue; // skip fields which is orphaned in remote
        }
        if (dataIdx >= data.optionalData.size()) {
            LOGE("[SQLiteRDBUtils] field over size. cnt[%d] data size[%d]", dataIdx, data.optionalData.size());
            errCode = -E_INTERNAL_ERROR;
            break; // cnt should less than optionalData size.
        }
        Type saveVal;
        CloudStorageUtils::SaveChangedDataByType(data.optionalData[dataIdx], saveVal);
        distributedPk[it.GetFieldName()] = std::move(saveVal);
        dataIdx++;
    }
    return res;
}

int SQLiteRelationalUtils::BindDistributedPk(sqlite3_stmt *stmt, const RelationalSyncDataInserter &inserter,
    VBucket &distributedPk)
{
    const auto &table = inserter.GetLocalTable();
    const auto distributedPkFields = table.GetSyncDistributedPk();
    auto &fields = table.GetFields();
    int bindIdx = 1;
    for (const auto &fieldName : distributedPkFields) {
        auto field = fields.find(fieldName);
        if (field == fields.end()) {
            LOGE("[SQLiteRDBUtils] bind no exist field[%s][%zu]", DBCommon::StringMiddleMasking(fieldName).c_str(),
                fieldName.size());
            return -E_INTERNAL_ERROR;
        }
        auto errCode = BindOneField(stmt, bindIdx, field->second, distributedPk);
        if (errCode != E_OK) {
            LOGE("[SQLiteRDBUtils] bind field[%s][%zu] type[%d] failed[%d]",
                DBCommon::StringMiddleMasking(fieldName).c_str(), fieldName.size(),
                static_cast<int>(field->second.GetStorageType()), errCode);
            return errCode;
        }
        bindIdx++;
    }
    return E_OK;
}

int SQLiteRelationalUtils::BindOneField(sqlite3_stmt *stmt, int bindIdx, const FieldInfo &fieldInfo,
    VBucket &distributedPk)
{
    Field field;
    field.colName = fieldInfo.GetFieldName();
    field.nullable = !fieldInfo.IsNotNull();
    switch (fieldInfo.GetStorageType()) {
        case StorageType::STORAGE_TYPE_INTEGER:
            field.type = TYPE_INDEX<int64_t>;
            return CloudStorageUtils::BindInt64(bindIdx, distributedPk, field, stmt);
        case StorageType::STORAGE_TYPE_BLOB:
            field.type = TYPE_INDEX<Bytes>;
            return CloudStorageUtils::BindBlob(bindIdx, distributedPk, field, stmt);
        case StorageType::STORAGE_TYPE_TEXT:
            field.type = TYPE_INDEX<std::string>;
            return CloudStorageUtils::BindText(bindIdx, distributedPk, field, stmt);
        case StorageType::STORAGE_TYPE_REAL:
            field.type = TYPE_INDEX<double>;
            return CloudStorageUtils::BindDouble(bindIdx, distributedPk, field, stmt);
        default:
            return -E_INTERNAL_ERROR;
    }
}

void SQLiteRelationalUtils::DeleteMismatchLog(sqlite3 *db, const std::string &tableName, const std::string &misDataKeys)
{
    std::string delSql = "DELETE FROM " + DBCommon::GetLogTableName(tableName) + " WHERE data_key IN " +
        misDataKeys;
    int errCode = SQLiteUtils::ExecuteRawSQL(db, delSql);
    if (errCode != E_OK) {
        LOGW("[%s [%zu]] Failed to del mismatch log, errCode=%d", DBCommon::StringMiddleMasking(tableName).c_str(),
            tableName.size(), errCode);
    }
}

const std::string SQLiteRelationalUtils::GetTempUpdateLogCursorTriggerSql(const std::string &tableName)
{
    std::string sql = "CREATE TEMP TRIGGER IF NOT EXISTS " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName;
    sql += "LOG_ON_UPDATE_TEMP AFTER UPDATE ON " + DBCommon::GetLogTableName(tableName);
    sql += " WHEN (SELECT 1 FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata" +
           " WHERE key = 'log_trigger_switch' AND value = 'false')\n";
    sql += "BEGIN\n";
    sql += CloudStorageUtils::GetCursorIncSql(tableName) + "\n";
    sql += "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET ";
    sql += "cursor=" + CloudStorageUtils::GetSelectIncCursorSql(tableName) + " WHERE data_key = OLD.data_key;\n";
    sql += "END;";
    return sql;
}

std::pair<int, TableInfo> SQLiteRelationalUtils::AnalyzeTable(sqlite3 *db, const std::string &tableName)
{
    std::pair<int, TableInfo> res;
    auto &[errCode, tableInfo] = res;
    errCode = SQLiteUtils::AnalysisSchema(db, tableName, tableInfo);
    return res;
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
int SQLiteRelationalUtils::PutCloudGid(sqlite3 *db, const std::string &tableName, std::vector<VBucket> &data)
{
    // create tmp table if table not exists
    std::string sql = "CREATE TABLE IF NOT EXISTS " + DBCommon::GetTmpLogTableName(tableName) +
        "(ID integer primary key autoincrement, cloud_gid TEXT UNIQUE ON CONFLICT IGNORE)";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[RDBUtils] Create gid table failed[%d]", errCode);
        return errCode;
    }
    // insert all gid
    sql = "INSERT INTO " + DBCommon::GetTmpLogTableName(tableName) + "(cloud_gid) VALUES(?)";
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (stmt == nullptr) {
        LOGE("[RDBUtils] Get insert gid stmt failed[%d]", errCode);
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *releaseStmt = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(releaseStmt, true, ret);
        if (ret != E_OK) {
            LOGW("[RDBUtils] Reset cloud gid stmt failed[%d]", ret);
        }
    });
    return PutCloudGidInner(stmt, data);
}

int SQLiteRelationalUtils::PutCloudGidInner(sqlite3_stmt *stmt, std::vector<VBucket> &data)
{
    int errCode = E_OK;
    for (const auto &item : data) {
        auto iter = item.find(CloudDbConstant::GID_FIELD);
        if (iter == item.end()) {
            LOGW("[RDBUtils] Cloud gid info not contain gid field");
            continue;
        }
        auto gid = std::get_if<std::string>(&iter->second);
        if (gid == nullptr) {
            LOGW("[RDBUtils] Cloud gid info not contain wrong type[%zu]", iter->second.index());
            continue;
        }
        errCode = SQLiteUtils::BindTextToStatement(stmt, 1, *gid);
        if (errCode != E_OK) {
            LOGE("[RDBUtils] Bind cloud gid failed[%d]", errCode);
            return errCode;
        }
        errCode = SQLiteUtils::StepNext(stmt);
        if (errCode != -E_FINISHED) {
            LOGE("[RDBUtils] Step cloud gid failed[%d]", errCode);
            return errCode;
        }
        errCode = E_OK;
        SQLiteUtils::ResetStatement(stmt, false, errCode);
        if (errCode != E_OK) {
            LOGE("[RDBUtils] Reset cloud gid stmt failed[%d]", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SQLiteRelationalUtils::GetOneBatchCloudNotExistRecord(const std::string &tableName, sqlite3 *db,
    std::vector<CloudNotExistRecord> &records, const std::string &dataPk)
{
    std::string sql = "SELECT a._rowid_, a.data_key, b." + dataPk + ", a.flag FROM " +
        DBCommon::GetLogTableName(tableName) + " AS a LEFT JOIN " + tableName + " AS b ON (a.data_key = b._rowid_) "
        "WHERE a.cloud_gid IS NOT '' AND a.cloud_gid NOT IN (SELECT cloud_gid FROM naturalbase_rdb_aux_" + tableName +
        "_log_tmp) limit 100;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[RDBUtils][GetOneBatchCloudNotExistRecord] Get statement failed, %d.", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(stmt);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            CloudNotExistRecord record;
            record.logRowid = sqlite3_column_int64(stmt, 0); // col 0 is _rowid_ of log table
            record.dataRowid = sqlite3_column_int64(stmt, 1); // col 1 is _rowid_ of data table
            errCode = GetTypeValByStatement(stmt, 2, record.pkValue); // col 2 is pk of data table
            if (errCode != E_OK) {
                LOGE("[RDBUtils][GetOneBatchCloudNotExistRecord] Get pk value failed, errCode = %d.", errCode);
                break;
            }
            record.flag = sqlite3_column_int(stmt, 3); // col 3 is flag
            records.push_back(record);
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else {
            LOGE("[RDBUtils][GetOneBatchCloudNotExistRecord] Step failed, errCode = %d.", errCode);
            break;
        }
    } while (true);

    return SQLiteUtils::ProcessStatementErrCode(stmt, true, errCode);
}

int SQLiteRelationalUtils::DeleteOneRecord(const std::string &tableName, sqlite3 *db, const CloudNotExistRecord &record,
    bool isLogicDelete, std::vector<Type> &changePk)
{
    if ((record.flag & static_cast<uint32_t>(LogInfoFlag::FLAG_LOCAL)) != 0) {
        std::string sql = "UPDATE " + DBCommon::GetLogTableName(tableName) +
            " SET cloud_gid = '', version = '', flag=flag&" + UPLOAD_CLOUD_UNFINISHED + " where _rowid_ = " +
            std::to_string(record.logRowid) + ";";
        return SQLiteUtils::ExecuteRawSQL(db, sql);
    }
    changePk.emplace_back(record.pkValue);
    std::string sql;
    int errCode = E_OK;
    if (!isLogicDelete) {
        sql = "DELETE FROM " + tableName + " WHERE _rowid_ = " + std::to_string(record.dataRowid) + ";";
        errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    uint32_t recordFlag = static_cast<uint32_t>(record.flag);
    int32_t newFlag = isLogicDelete ?
        ((recordFlag & (static_cast<uint32_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY) |
            static_cast<uint32_t>(LogInfoFlag::FLAG_DELETE) |
            static_cast<uint32_t>(LogInfoFlag::FLAG_LOGIC_DELETE))) &
            (~static_cast<uint32_t>(LogInfoFlag::FLAG_CLOUD_UPDATE_LOCAL))) :
        ((recordFlag & (static_cast<uint32_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY) |
            static_cast<uint32_t>(LogInfoFlag::FLAG_DELETE))) &
            (~static_cast<uint32_t>(LogInfoFlag::FLAG_CLOUD_UPDATE_LOCAL)));
    sql = "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET cloud_gid = '', version = '', flag = " +
          std::to_string(newFlag) + " where _rowid_ = " + std::to_string(record.logRowid) + ";";
    return SQLiteUtils::ExecuteRawSQL(db, sql);
}

int SQLiteRelationalUtils::DropTempTable(const std::string &tableName, sqlite3 *db)
{
    std::string sql = "DROP TABLE IF EXISTS " + DBCommon::GetTmpLogTableName(tableName);
    return SQLiteUtils::ExecuteRawSQL(db, sql);
}

int SQLiteRelationalUtils::CheckUserCreateSharedTable(sqlite3 *db, const TableSchema &oriTable,
    const std::string &sharedTable)
{
    auto [errCode, sharedTableInfo] = AnalyzeTable(db, sharedTable);
    if (errCode == -E_NOT_FOUND) {
        return E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[RDBUtils] Check shared table[%s] failed[%d]",
            DBCommon::StringMiddleMaskingWithLen(sharedTable).c_str(), errCode);
        return errCode;
    }
    if (sharedTableInfo.IsView()) {
        LOGE("[RDBUtils] Not support view for shared table[%s] ",
            DBCommon::StringMiddleMaskingWithLen(sharedTable).c_str());
        return -E_NOT_SUPPORT;
    }
    return CheckUserCreateSharedTableInner(oriTable, sharedTableInfo);
}

int SQLiteRelationalUtils::CheckUserCreateSharedTableInner(const TableSchema &oriTable,
    const TableInfo &sharedTableInfo)
{
    if (!sharedTableInfo.IsFieldExist(CloudDbConstant::CLOUD_OWNER)) {
        return -E_INVALID_ARGS;
    }
    if (!sharedTableInfo.IsFieldExist(CloudDbConstant::CLOUD_PRIVILEGE)) {
        return -E_INVALID_ARGS;
    }
    auto sharedFields = sharedTableInfo.GetFieldInfos();
    auto cloudFieldDataTypes = GetCloudFieldDataType();
    std::map<std::string, std::pair<std::string, bool>> cloudFieldType;
    for (const auto &field : oriTable.fields) {
        auto iter = cloudFieldDataTypes.find(field.type);
        if (iter == cloudFieldDataTypes.end()) {
            LOGE("[RDBUtils] Check shared table[%s] failed by miss field[%s] type[%" PRId32 "]",
                DBCommon::StringMiddleMaskingWithLen(sharedTableInfo.GetTableName()).c_str(),
                DBCommon::StringMiddleMaskingWithLen(field.colName).c_str(),
                field.type);
            return -E_INVALID_ARGS;
        }
        cloudFieldType[field.colName] = {iter->second, field.nullable};
    }
    for (const auto &sharedField : sharedFields) {
        if (sharedField.GetFieldName() == CloudDbConstant::CLOUD_OWNER ||
            sharedField.GetFieldName() == CloudDbConstant::CLOUD_PRIVILEGE) {
            continue;
        }
        auto oriField = cloudFieldType.find(sharedField.GetFieldName());
        if (oriField == cloudFieldType.end()) {
            LOGE("[RDBUtils] Check shared table[%s] failed by miss field[%s]",
                DBCommon::StringMiddleMaskingWithLen(sharedTableInfo.GetTableName()).c_str(),
                DBCommon::StringMiddleMaskingWithLen(sharedField.GetFieldName()).c_str());
            return -E_INVALID_ARGS;
        }
        FieldInfo checkField = sharedField;
        auto [dataType, nullable] = cloudFieldType[sharedField.GetFieldName()];
        checkField.SetDataType(dataType);
        checkField.SetNotNull(!nullable);
        if (!sharedField.CompareWithField(checkField)) {
            return -E_INVALID_ARGS;
        }
    }
    return E_OK;
}

std::map<int32_t, std::string> SQLiteRelationalUtils::GetCloudFieldDataType()
{
    std::map<int32_t, std::string> cloudFieldTypeMap;
    cloudFieldTypeMap[TYPE_INDEX<Nil>] = "NULL";
    cloudFieldTypeMap[TYPE_INDEX<int64_t>] = "INT";
    cloudFieldTypeMap[TYPE_INDEX<double>] = "REAL";
    cloudFieldTypeMap[TYPE_INDEX<std::string>] = "TEXT";
    cloudFieldTypeMap[TYPE_INDEX<bool>] = "BOOLEAN";
    cloudFieldTypeMap[TYPE_INDEX<Bytes>] = "BLOB";
    cloudFieldTypeMap[TYPE_INDEX<Asset>] = "ASSET";
    cloudFieldTypeMap[TYPE_INDEX<Assets>] = "ASSETS";
    return cloudFieldTypeMap;
}
#endif

int SQLiteRelationalUtils::BindAndStepDevicesToStatement(sqlite3_stmt *stmt,
    const std::vector<std::string> &keepDevices)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    for (size_t i = 0; i < keepDevices.size(); i++) {
        std::string hashDevice = DBCommon::TransferHashString(keepDevices[i]);
        if (hashDevice.empty()) {
            errCode = SQLiteUtils::BindTextToStatement(stmt, i + 1, hashDevice);
        } else {
            std::vector<uint8_t> originDev(hashDevice.begin(), hashDevice.end());
            errCode = SQLiteUtils::BindBlobToStatement(stmt, i + 1, originDev);
        }
        if (errCode != E_OK) {
            LOGE("[SQLiteRDBUtils] Bind hashKey failed %d", errCode);
            return errCode;
        }
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
        (void) SQLiteUtils::PrintChangeRows(stmt);
    }
    return errCode;
}

int SQLiteRelationalUtils::DeleteDistributedExceptDeviceTable(sqlite3 *db, const std::string &removedTable,
    const std::vector<std::string> &keepDevices)
{
    std::string selectSql = "SELECT data_key FROM " + DBCommon::GetLogTableName(removedTable) +
        " WHERE ori_device NOT IN (";
    for (size_t i = 0; i < keepDevices.size(); i++) {
        selectSql += "?";
        if (i + 1 < keepDevices.size()) {
            selectSql += ", ";
        }
    }
    selectSql += ") AND data_key <> -1";
    std::string deleteSql = "DELETE FROM " + removedTable + " WHERE _rowid_ in (" + selectSql + ");";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, deleteSql, stmt);
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] get delete distributed table stmt failed, %d", errCode);
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset stmt failed %d when delete data", ret);
        }
    });
    errCode = BindAndStepDevicesToStatement(stmt, keepDevices);
    if (errCode != E_OK) {
        LOGE("[DeleteDistributedExceptDeviceTable] delete table failed, %s, %d",
            DBCommon::StringMiddleMaskingWithLen(removedTable).c_str(), errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::DeleteDistributedExceptDeviceTableLog(sqlite3 *db, const std::string &removedTable,
    const std::vector<std::string> &keepDevices)
{
    std::string deleteSql = "DELETE FROM " + DBCommon::GetLogTableName(removedTable) +
        " WHERE ori_device NOT IN (";
    for (size_t i = 0; i < keepDevices.size(); i++) {
        deleteSql += "?";
        if (i + 1 < keepDevices.size()) {
            deleteSql += ", ";
        }
    }
    deleteSql += ");";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, deleteSql, stmt);
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] get delete distributed log table stmt failed, %d", errCode);
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset stmt failed %d when delete log data", ret);
        }
    });
    errCode = BindAndStepDevicesToStatement(stmt, keepDevices);
    if (errCode != E_OK) {
        LOGE("[DeleteDistributedExceptDeviceTableLog] delete log table failed, %s, %d",
            DBCommon::StringMiddleMaskingWithLen(DBCommon::GetLogTableName(removedTable)).c_str(), errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::UpdateTrackerTableSyncDelete(sqlite3 *db, const std::string &removedTable,
    const std::vector<std::string> &keepDevices)
{
    std::string deleteSql = "UPDATE " + DBCommon::GetLogTableName(removedTable) +
        " SET flag = 0x01 WHERE ori_device NOT IN (";
    for (size_t i = 0; i < keepDevices.size(); i++) {
        deleteSql += "?";
        if (i + 1 < keepDevices.size()) {
            deleteSql += ", ";
        }
    }
    deleteSql += ") AND data_key <> -1;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, deleteSql, stmt);
    if (errCode != E_OK) {
        LOGE("[SQLiteRDBUtils] get update tracker sync delete table stmt failed, %d", errCode);
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset stmt failed %d when updatetracker data", ret);
        }
    });
    errCode = BindAndStepDevicesToStatement(stmt, keepDevices);
    if (errCode != E_OK) {
        LOGE("[UpdateTrackerTableSyncDelete] delete log table failed, %s, %d",
            DBCommon::StringMiddleMaskingWithLen(DBCommon::GetLogTableName(removedTable)).c_str(), errCode);
    }
    return errCode;
}
} // namespace DistributedDB