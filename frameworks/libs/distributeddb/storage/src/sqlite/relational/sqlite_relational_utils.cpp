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
#include "db_errno.h"
#include "cloud/cloud_db_types.h"
#include "sqlite_utils.h"
#include "cloud/cloud_storage_utils.h"
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
        case SQLITE_INTEGER: {
            value = static_cast<int64_t>(sqlite3_column_int64(statement, cid));
            break;
        }
        case SQLITE_FLOAT: {
            value = sqlite3_column_double(statement, cid);
            break;
        }
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
            break;
        }
        case SQLITE_NULL: {
            break;
        }
        case SQLITE3_TEXT: {
            std::string str;
            (void)SQLiteUtils::GetColumnTextValue(statement, cid, str);
            value = str;
            if (value.GetType() != StorageType::STORAGE_TYPE_TEXT) {
                errCode = -E_OUT_OF_MEMORY;
            }
            break;
        }
        default: {
            break;
        }
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
            break;
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
            if (declType == nullptr) {
                typeVal = static_cast<int64_t>(sqlite3_column_int64(stmt, cid));
                break;
            }
            if (strcasecmp(declType, SchemaConstant::KEYWORD_TYPE_BOOL.c_str()) == 0 ||
                strcasecmp(declType, SchemaConstant::KEYWORD_TYPE_BOOLEAN.c_str()) == 0) {
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
            if (errCode != E_OK) {
                break;
            }
        }
        case SQLITE3_TEXT: {
            errCode = GetBlobByStatement(stmt, cid, typeVal);
            if (errCode != E_OK) {
                break;
            }
            if (typeVal.index() != TYPE_INDEX<Nil>) {
                break;
            }
            std::string str;
            (void)SQLiteUtils::GetColumnTextValue(stmt, cid, str);
            typeVal = str;
            break;
        }
        default: {
            typeVal = Nil();
            break;
        }
    }
    return errCode;
}

int SQLiteRelationalUtils::GetBlobByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal)
{
    const char *declType = sqlite3_column_decltype(stmt, cid);
    int errCode = E_OK;
    if (declType != nullptr && strcasecmp(declType, CloudDbConstant::ASSET) == 0) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, cid, blobValue);
        if (errCode != E_OK) {
            return errCode;
        }
        Asset asset;
        errCode = RuntimeContext::GetInstance()->BlobToAsset(blobValue, asset);
        if (errCode != E_OK) {
            return errCode;
        }
        typeVal = asset;
    } else if (declType != nullptr && strcasecmp(declType, CloudDbConstant::ASSETS) == 0) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, cid, blobValue);
        if (errCode != E_OK) {
            return errCode;
        }
        Assets assets;
        errCode = RuntimeContext::GetInstance()->BlobToAssets(blobValue, assets);
        if (errCode != E_OK) {
            return errCode;
        }
        typeVal = assets;
    } else if (sqlite3_column_type(stmt, cid) == SQLITE_BLOB) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, cid, blobValue);
        if (errCode != E_OK) {
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
} // namespace DistributedDB