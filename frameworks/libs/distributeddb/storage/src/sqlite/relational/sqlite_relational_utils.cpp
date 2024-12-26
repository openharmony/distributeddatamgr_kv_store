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
#include "db_common.h"
#include "db_errno.h"
#include "cloud/cloud_db_types.h"
#include "sqlite_utils.h"
#include "cloud/cloud_storage_utils.h"
#include "res_finalizer.h"
#include "runtime_context.h"
#include "cloud/cloud_db_constant.h"

namespace DistributedDB {
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
            if (strcasecmp(declType, SchemaConstant::KEYWORD_TYPE_BOOL.c_str()) == 0 ||
                strcasecmp(declType, SchemaConstant::KEYWORD_TYPE_BOOLEAN.c_str()) == 0) { // LCOV_EXCL_BR_LINE
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

int SQLiteRelationalUtils::AnalysisTrackerTable(sqlite3 *db, const TrackerTable &trackerTable, TableInfo &tableInfo)
{
    int errCode = SQLiteUtils::AnalysisSchema(db, trackerTable.GetTableName(), tableInfo, true);
    if (errCode != E_OK) {
        LOGE("analysis table schema failed %d.", errCode);
        return errCode;
    }
    tableInfo.SetTrackerTable(trackerTable);
    errCode = tableInfo.CheckTrackerTable();
    if (errCode != E_OK) {
        LOGE("check tracker table schema failed %d.", errCode);
    }
    return errCode;
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
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
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
    std::set<std::string> &fieldsNeedContain)
{
    for (const auto &syncField : syncFields) {
        fieldsNeedContain.insert(syncField.GetFieldName());
    }
    for (const auto &primaryKey : tableInfo.GetPrimaryKey()) {
        fieldsNeedContain.insert(primaryKey.second);
    }
    const std::vector<CompositeFields> &uniqueDefines = tableInfo.GetUniqueDefine();
    for (const auto &compositeFields : uniqueDefines) {
        for (const auto &fieldName : compositeFields) {
            fieldsNeedContain.insert(fieldName);
        }
    }
    const FieldInfoMap &fieldInfoMap = tableInfo.GetFields();
    for (const auto &entry : fieldInfoMap) {
        const FieldInfo &fieldInfo = entry.second;
        if (fieldInfo.IsNotNull() && fieldInfo.GetDefaultValue().empty()) {
            fieldsNeedContain.insert(fieldInfo.GetFieldName());
        }
    }
}

int CheckDistributedSchemaFields(const TableInfo &tableInfo, const std::vector<FieldInfo> &syncFields,
    const std::vector<DistributedField> &fields)
{
    if (fields.empty()) {
        LOGE("fields cannot be empty");
        return -E_SCHEMA_MISMATCH;
    }
    for (const auto &field : fields) {
        if (!tableInfo.IsFieldExist(field.colName)) {
            LOGE("Column[%s [%zu]] not found in table", DBCommon::StringMiddleMasking(field.colName).c_str(),
                 field.colName.size());
            return -E_SCHEMA_MISMATCH;
        }
    }
    std::set<std::string> fieldsNeedContain;
    GetFieldsNeedContain(tableInfo, syncFields, fieldsNeedContain);
    std::set<std::string> fieldsMap;
    for (auto &field : fields) {
        fieldsMap.insert(field.colName);
    }
    for (auto &fieldNeedContain : fieldsNeedContain) {
        if (fieldsMap.find(fieldNeedContain) == fieldsMap.end()) {
            LOGE("Required column[%s [%zu]] not found", DBCommon::StringMiddleMasking(fieldNeedContain).c_str(),
                fieldNeedContain.size());
            return -E_DISTRIBUTED_FIELD_DECREASE;
        }
    }
    return E_OK;
}

int SQLiteRelationalUtils::CheckDistributedSchemaValid(const RelationalSchemaObject &schemaObj,
    const DistributedSchema &schema, SQLiteSingleVerRelationalStorageExecutor *executor)
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
        errCode = CheckDistributedSchemaFields(tableInfo, schemaObj.GetSyncFieldInfo(table.tableName, false),
            table.fields);
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

int SQLiteRelationalUtils::GetLogInfoPre(sqlite3_stmt *queryStmt, DistributedTableMode mode,
    const DataItem &dataItem, LogInfo &logInfoGet)
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
} // namespace DistributedDB