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

#include "sqlite_utils.h"

#include "db_common.h"
#include "platform_specific.h"

namespace DistributedDB {
namespace {
    const int BUSY_SLEEP_TIME = 50; // sleep for 50us
    const int NO_SIZE_LIMIT = -1;
    const int MAX_STEP_TIMES = 8000;
    const std::string BEGIN_SQL = "BEGIN TRANSACTION";
    const std::string BEGIN_IMMEDIATE_SQL = "BEGIN IMMEDIATE TRANSACTION";
    const std::string COMMIT_SQL = "COMMIT TRANSACTION";
    const std::string ROLLBACK_SQL = "ROLLBACK TRANSACTION";
    const int MAX_BLOB_READ_SIZE = 64 * 1024 * 1024; // 64M limit
    const int MAX_TEXT_READ_SIZE = 5 * 1024 * 1024; // 5M limit

    const constexpr char *CHECK_TABLE_CREATED = "SELECT EXISTS(SELECT 1 FROM sqlite_master WHERE " \
        "type='table' AND (tbl_name=? COLLATE NOCASE));";
    const constexpr char *CHECK_META_DB_TABLE_CREATED = "SELECT EXISTS(SELECT 1 FROM meta.sqlite_master WHERE " \
        "type='table' AND (tbl_name=? COLLATE NOCASE));";
}

namespace TriggerMode {
const std::map<TriggerModeEnum, std::string> TRIGGER_MODE_MAP = {
    {TriggerModeEnum::NONE,   ""},
    {TriggerModeEnum::INSERT, "INSERT"},
    {TriggerModeEnum::UPDATE, "UPDATE"},
    {TriggerModeEnum::DELETE, "DELETE"},
};

std::string GetTriggerModeString(TriggerModeEnum mode)
{
    auto it = TRIGGER_MODE_MAP.find(mode);
    return (it == TRIGGER_MODE_MAP.end()) ? "" : it->second;
}
}

int SQLiteUtils::StepWithRetry(sqlite3_stmt *statement, bool isMemDb)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    int retryCount = 0;
    do {
        errCode = sqlite3_step(statement);
        if ((errCode == SQLITE_LOCKED) && isMemDb) {
            std::this_thread::sleep_for(std::chrono::microseconds(BUSY_SLEEP_TIME));
            retryCount++;
        } else {
            break;
        }
    } while (retryCount <= MAX_STEP_TIMES);

    if (errCode != SQLITE_DONE && errCode != SQLITE_ROW) {
        LOGE("[SQLiteUtils] Step error:%d, sys:%d", errCode, errno);
    }

    return SQLiteUtils::MapSQLiteErrno(errCode);
}

int SQLiteUtils::BeginTransaction(sqlite3 *db, TransactType type)
{
    if (type == TransactType::IMMEDIATE) {
        return ExecuteRawSQL(db, BEGIN_IMMEDIATE_SQL, true);
    }

    return ExecuteRawSQL(db, BEGIN_SQL, true);
}

int SQLiteUtils::CommitTransaction(sqlite3 *db)
{
    return ExecuteRawSQL(db, COMMIT_SQL, true);
}

int SQLiteUtils::RollbackTransaction(sqlite3 *db)
{
    return ExecuteRawSQL(db, ROLLBACK_SQL, true);
}

int SQLiteUtils::ExecuteRawSQL(sqlite3 *db, const std::string &sql, bool ignoreResetFail)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("[SQLiteUtils][ExecuteSQL] prepare statement failed(%d), sys(%d)", errCode, errno);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(stmt);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            LOGE("[SQLiteUtils][ExecuteSQL] execute statement failed(%d), sys(%d)", errCode, errno);
            break;
        }
    } while (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));

    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    if (!ignoreResetFail && ret != E_OK) {
        return errCode != E_OK ? errCode : ret;
    }
    return errCode;
}

int SQLiteUtils::MapSQLiteErrno(int errCode)
{
    switch (errCode) {
        case SQLITE_OK:
            return E_OK;
        case SQLITE_IOERR:
            if (errno == EKEYREVOKED) {
                return -E_EKEYREVOKED;
            }
            break;
        case SQLITE_CORRUPT:
        case SQLITE_NOTADB:
            return -E_INVALID_PASSWD_OR_CORRUPTED_DB;
        case SQLITE_LOCKED:
        case SQLITE_BUSY:
            return -E_BUSY;
        case SQLITE_ERROR:
            if (errno == EKEYREVOKED) {
                return -E_EKEYREVOKED;
            }
            break;
        case SQLITE_AUTH:
            return -E_DENIED_SQL;
        case SQLITE_CONSTRAINT:
            return -E_CONSTRAINT;
        case SQLITE_CANTOPEN:
            return -E_SQLITE_CANT_OPEN;
        default:
            break;
    }
    return -errCode;
}

int SQLiteUtils::GetStatement(sqlite3 *db, const std::string &sql, sqlite3_stmt *&statement)
{
    if (db == nullptr) {
        LOGE("Invalid db for statement");
        return -E_INVALID_DB;
    }
    // Prepare the new statement only when the input parameter is not null
    if (statement != nullptr) {
        return E_OK;
    }
    int errCode = sqlite3_prepare_v2(db, sql.c_str(), NO_SIZE_LIMIT, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("Prepare SQLite statement failed:%d, sys:%d", errCode, errno);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return errCode;
    }

    if (statement == nullptr) {
        return -E_INVALID_DB;
    }

    return E_OK;
}

void SQLiteUtils::ResetStatement(sqlite3_stmt *&statement, bool isNeedFinalize, int &errCode)
{
    ResetStatement(statement, isNeedFinalize, false, errCode);
}

void SQLiteUtils::ResetStatement(sqlite3_stmt *&statement, bool isNeedFinalize, bool isIgnoreResetRet, int &errCode)
{
    if (statement == nullptr) {
        return;
    }

    int innerCode = SQLITE_OK;
    // if need finalize the statement, just goto finalize.
    if (!isNeedFinalize) {
        // reset the statement firstly.
        innerCode = sqlite3_reset(statement);
        if (innerCode != SQLITE_OK && !isIgnoreResetRet) {
            LOGE("[SQLiteUtils] reset statement error:%d, sys:%d", innerCode, errno);
            isNeedFinalize = true;
        } else {
            sqlite3_clear_bindings(statement);
        }
    }

    if (isNeedFinalize) {
        int finalizeResult = sqlite3_finalize(statement);
        if (finalizeResult != SQLITE_OK) {
            LOGE("[SQLiteUtils] finalize statement error:%d, sys:%d", finalizeResult, errno);
            innerCode = finalizeResult;
        }
        statement = nullptr;
    }

    if (innerCode != SQLITE_OK) { // the sqlite error code has higher priority.
        errCode = SQLiteUtils::MapSQLiteErrno(innerCode);
    }
}

#ifdef RELATIONAL_STORE
namespace { // anonymous namespace for schema analysis
int AnalysisSchemaSqlAndTrigger(sqlite3 *db, const std::string &tableName, TableInfo &table, bool caseSensitive)
{
    std::string sql = "SELECT type, sql FROM sqlite_master WHERE tbl_name = ? ";
    if (!caseSensitive) {
        sql += "COLLATE NOCASE";
    }
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the analysis schema sql and trigger statement error:%d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(statement, 1, tableName);
    int ret = E_OK;
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Bind table name failed:%d", errCode);
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }

    errCode = -E_NOT_FOUND;
    int err = SQLiteUtils::MapSQLiteErrno(SQLITE_ROW);
    do {
        err = SQLiteUtils::StepWithRetry(statement);
        if (err == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        } else if (err == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            errCode = E_OK;
            std::string type;
            (void) SQLiteUtils::GetColumnTextValue(statement, 0, type);
            if (type == "table") {
                std::string createTableSql;
                (void) SQLiteUtils::GetColumnTextValue(statement, 1, createTableSql); // 1 means create table sql
                table.SetCreateTableSql(createTableSql);
            }
            table.SetType(type);
        } else {
            LOGE("[AnalysisSchema] Step for the analysis create table sql and trigger failed:%d", err);
            errCode = err;
            break;
        }
    } while (err == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int AnalysisSchemaIndexDefine(sqlite3 *db, const std::string &indexName, CompositeFields &indexDefine)
{
    auto sql = "pragma index_info('" + indexName + "')";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the analysis schema index statement error:%d", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string indexField;
            (void) SQLiteUtils::GetColumnTextValue(statement, 2, indexField);  // 2 means index's column name.
            indexDefine.push_back(indexField);
        } else {
            LOGW("[AnalysisSchema] Step for the analysis schema index failed:%d", errCode);
            break;
        }
    } while (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));

    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int GetSchemaIndexList(sqlite3 *db, const std::string &tableName, std::vector<std::string> &indexList,
    std::vector<std::string> &uniqueList)
{
    std::string sql = "pragma index_list('" + tableName + "')";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the get schema index list statement error:%d", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string indexName;
            (void) SQLiteUtils::GetColumnTextValue(statement, 1, indexName);  // 1 means index name
            int unique = sqlite3_column_int64(statement, 2);  // 2 means index type, whether unique
            if (unique == 0) { // 0 means index created by user declare
                indexList.push_back(indexName);
            } else if (unique == 1) { // 1 means an unique define
                uniqueList.push_back(indexName);
            }
        } else {
            LOGW("[AnalysisSchema] Step for the get schema index list failed:%d", errCode);
            break;
        }
    } while (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int AnalysisSchemaIndex(sqlite3 *db, const std::string &tableName, TableInfo &table)
{
    std::vector<std::string> indexList;
    std::vector<std::string> uniqueList;
    int errCode = GetSchemaIndexList(db, tableName, indexList, uniqueList);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] get schema index list failed.");
        return errCode;
    }

    for (const auto &indexName : indexList) {
        CompositeFields indexDefine;
        errCode = AnalysisSchemaIndexDefine(db, indexName, indexDefine);
        if (errCode != E_OK) {
            LOGE("[AnalysisSchema] analysis schema index columns failed.");
            return errCode;
        }
        table.AddIndexDefine(indexName, indexDefine);
    }

    std::vector<CompositeFields> uniques;
    for (const auto &uniqueName : uniqueList) {
        CompositeFields uniqueDefine;
        errCode = AnalysisSchemaIndexDefine(db, uniqueName, uniqueDefine);
        if (errCode != E_OK) {
            LOGE("[AnalysisSchema] analysis schema unique columns failed.");
            return errCode;
        }
        uniques.push_back(uniqueDefine);
    }
    table.SetUniqueDefine(uniques);
    return E_OK;
}

void SetPrimaryKeyCollateType(const std::string &sql, FieldInfo &field)
{
    std::string upperFieldName = DBCommon::ToUpperCase(field.GetFieldName());
    if (DBCommon::HasConstraint(sql, "PRIMARY KEY COLLATE NOCASE", " ", " ,)") ||
        DBCommon::HasConstraint(sql, upperFieldName + " TEXT COLLATE NOCASE", " (,", " ,")) {
        field.SetCollateType(CollateType::COLLATE_NOCASE);
    } else if (DBCommon::HasConstraint(sql, "PRIMARY KEY COLLATE RTRIM", " ", " ,)") ||
        DBCommon::HasConstraint(sql, upperFieldName + " TEXT COLLATE RTRIM", " (,", " ,")) {
        field.SetCollateType(CollateType::COLLATE_RTRIM);
    }
}

int SetFieldInfo(sqlite3_stmt *statement, TableInfo &table)
{
    FieldInfo field;
    field.SetColumnId(sqlite3_column_int(statement, 0));  // 0 means column id index

    std::string tmpString;
    (void) SQLiteUtils::GetColumnTextValue(statement, 1, tmpString);  // 1 means column name index
    if (!DBCommon::CheckIsAlnumOrUnderscore(tmpString)) {
        LOGE("[AnalysisSchema] unsupported field name.");
        return -E_NOT_SUPPORT;
    }
    field.SetFieldName(tmpString);

    (void) SQLiteUtils::GetColumnTextValue(statement, 2, tmpString);  // 2 means datatype index
    field.SetDataType(tmpString);

    field.SetNotNull(static_cast<bool>(sqlite3_column_int64(statement, 3)));  // 3 means whether null index

    (void) SQLiteUtils::GetColumnTextValue(statement, 4, tmpString);  // 4 means default value index
    if (!tmpString.empty()) {
        field.SetDefaultValue(tmpString);
    }

    int keyIndex = sqlite3_column_int(statement, 5); // 5 means primary key index
    if (keyIndex != 0) {  // not 0 means is a primary key
        table.SetPrimaryKey(field.GetFieldName(), keyIndex);
        SetPrimaryKeyCollateType(table.GetCreateTableSql(), field);
    }
    table.AddField(field);
    return E_OK;
}
} // end of anonymous namespace for schema analysis

int SQLiteUtils::AnalysisSchemaFieldDefine(sqlite3 *db, const std::string &tableName, TableInfo &table)
{
    std::string sql = "pragma table_info('" + tableName + "')";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the analysis schema field statement error:%d", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            errCode = SetFieldInfo(statement, table);
            if (errCode != E_OK) {
                break;
            }
        } else {
            LOGW("[AnalysisSchema] Step for the analysis schema field failed:%d", errCode);
            break;
        }
    } while (errCode == E_OK);

    if (table.GetPrimaryKey().empty()) {
        table.SetPrimaryKey("rowid", 1);
    }

    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteUtils::AnalysisSchema(sqlite3 *db, const std::string &tableName, TableInfo &table, bool caseSensitive)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    if (!DBCommon::CheckIsAlnumOrUnderscore(tableName)) {
        LOGE("[AnalysisSchema] unsupported table name.");
        return -E_NOT_SUPPORT;
    }

    int errCode = AnalysisSchemaSqlAndTrigger(db, tableName, table, caseSensitive);
    if (errCode != E_OK) {
        LOGW("[AnalysisSchema] Analysis sql and trigger failed. table[%s] errCode[%d]",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
        return errCode;
    }

    errCode = AnalysisSchemaIndex(db, tableName, table);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Analysis index failed.");
        return errCode;
    }

    errCode = AnalysisSchemaFieldDefine(db, tableName, table);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Analysis field failed.");
        return errCode;
    }

    table.SetTableName(tableName);
    return E_OK;
}
#endif

int SQLiteUtils::BindTextToStatement(sqlite3_stmt *statement, int index, const std::string &str)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = sqlite3_bind_text(statement, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtil][Bind text]Failed to bind the value:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}

int SQLiteUtils::ProcessStatementErrCode(sqlite3_stmt *&statement, bool isNeedFinalize, int errCode)
{
    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, isNeedFinalize, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteUtils::CheckTableExists(sqlite3 *db, const std::string &tableName, bool &isCreated, bool isCheckMeta)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, isCheckMeta ? CHECK_META_DB_TABLE_CREATED : CHECK_TABLE_CREATED, stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("Get check table statement failed. err=%d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::BindTextToStatement(stmt, 1, tableName);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("Bind table name to statement failed. err=%d", errCode);
        return ProcessStatementErrCode(stmt, true, errCode);
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("Check table exists failed. err=%d", errCode); // should always return a row data
        return ProcessStatementErrCode(stmt, true, errCode);
    }
    errCode = E_OK;
    isCreated = (sqlite3_column_int(stmt, 0) == 1);
    return ProcessStatementErrCode(stmt, true, errCode);
}

int SQLiteUtils::BindBlobToStatement(sqlite3_stmt *statement, int index, const std::vector<uint8_t> &value,
    bool permEmpty)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    // Check empty value.
    if (value.empty() && !permEmpty) {
        LOGI("[SQLiteUtil][Bind blob]Invalid value");
        return -E_INVALID_ARGS;
    }

    int errCode;
    if (value.empty()) {
        errCode = sqlite3_bind_zeroblob(statement, index, -1); // -1 for zero-length blob.
    } else {
        errCode = sqlite3_bind_blob(statement, index, static_cast<const void *>(value.data()),
            value.size(), SQLITE_TRANSIENT);
    }

    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtil][Bind blob]Failed to bind the value:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}

int SQLiteUtils::GetColumnTextValue(sqlite3_stmt *statement, int index, std::string &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int valSize = sqlite3_column_bytes(statement, index);
    if (valSize < 0) {
        LOGW("[SQLiteUtils][Column Text] size less than zero:%d", valSize);
        value = {};
        return E_OK;
    }
    const unsigned char *val = sqlite3_column_text(statement, index);
    if (valSize == 0 || val == nullptr) {
        value = {};
        return E_OK;
    }
    value = std::string(reinterpret_cast<const char *>(val));
    if (valSize > MAX_TEXT_READ_SIZE) {
        LOGW("[SQLiteUtils][Column text] size over limit:%d", valSize);
        value.resize(MAX_TEXT_READ_SIZE + 1); // Reset value size to invalid
    }
    return E_OK;
}

int SQLiteUtils::GetColumnBlobValue(sqlite3_stmt *statement, int index, std::vector<uint8_t> &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int keySize = sqlite3_column_bytes(statement, index);
    if (keySize < 0) {
        LOGW("[SQLiteUtils][Column blob] size less than zero:%d", keySize);
        value.resize(0);
        return E_OK;
    }
    auto keyRead = static_cast<const uint8_t *>(sqlite3_column_blob(statement, index));
    if (keySize == 0 || keyRead == nullptr) {
        value.resize(0);
    } else {
        if (keySize > MAX_BLOB_READ_SIZE) {
            LOGW("[SQLiteUtils][Column blob] size over limit:%d", keySize);
            keySize = MAX_BLOB_READ_SIZE + 1;
        }
        value.resize(keySize);
        value.assign(keyRead, keyRead + keySize);
    }
    return E_OK;
}

int SQLiteUtils::GetCountBySql(sqlite3 *db, const std::string &sql, int &count)
{
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][GetCountBySql] Get stmt failed when get local data count: %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        count = static_cast<int>(sqlite3_column_int(stmt, 0));
        errCode = E_OK;
    } else {
        LOGE("[SQLiteUtils][GetCountBySql] Query local data count failed: %d", errCode);
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    if (ret != E_OK) {
        LOGE("[SQLiteUtils][GetCountBySql] Reset stmt failed when get local data count: %d", ret);
    }
    return errCode != E_OK ? errCode : ret;
}

int SQLiteUtils::BindInt64ToStatement(sqlite3_stmt *statement, int index, int64_t value)
{
    // statement check outSide
    int errCode = sqlite3_bind_int64(statement, index, value);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtil][Bind int64]Failed to bind the value:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}

int SQLiteUtils::StepNext(sqlite3_stmt *stmt, bool isMemDb)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_FINISHED;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = E_OK;
    }
    return errCode;
}
} // namespace DistributedDB
