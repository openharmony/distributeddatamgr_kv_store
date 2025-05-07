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

#include "sqlite_relational_utils.h"
#include "db_common.h"

namespace DistributedDB {
int SQLiteRelationalUtils::CreateRelationalMetaTable(sqlite3 *db)
{
    std::string sql =
        "CREATE TABLE IF NOT EXISTS " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata(" \
        "key    BLOB PRIMARY KEY NOT NULL," \
        "value  BLOB);";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[SQLite] execute create table sql failed, err=%d", errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::GetKvData(sqlite3 *db, bool isMemory, const Key &key, Value &value)
{
    static const std::string SELECT_META_VALUE_SQL = "SELECT value FROM " + std::string(DBConstant::RELATIONAL_PREFIX) +
        "metadata WHERE key=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, SELECT_META_VALUE_SQL, statement);
    if (errCode != E_OK) {
        return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false); // first arg.
    if (errCode != E_OK) {
        return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
    }

    errCode = SQLiteUtils::StepWithRetry(statement, isMemory);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
        return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
    } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
    }

    errCode = SQLiteUtils::GetColumnBlobValue(statement, 0, value); // only one result.
    return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
}

int SQLiteRelationalUtils::PutKvData(sqlite3 *db, bool isMemory, const Key &key, const Value &value)
{
    static const std::string INSERT_META_SQL = "INSERT OR REPLACE INTO " + std::string(DBConstant::RELATIONAL_PREFIX) +
        "metadata VALUES(?,?);";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, INSERT_META_SQL, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false);  // 1 means key index
    if (errCode != E_OK) {
        LOGE("[SingleVerExe][BindPutKv]Bind key error:%d", errCode);
        return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 2, value, true);  // 2 means value index
    if (errCode != E_OK) {
        LOGE("[SingleVerExe][BindPutKv]Bind value error:%d", errCode);
        return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
    }
    errCode = SQLiteUtils::StepWithRetry(statement, isMemory);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
}

int SQLiteRelationalUtils::InitCursorToMeta(sqlite3 *db, bool isMemory, const std::string &tableName)
{
    Value key;
    Value cursor;
    DBCommon::StringToVector(DBCommon::GetCursorKey(tableName), key);
    int errCode = GetKvData(db, isMemory, key, cursor);
    if (errCode == -E_NOT_FOUND) {
        DBCommon::StringToVector(std::string("0"), cursor);
        errCode = PutKvData(db, isMemory, key, cursor);
        if (errCode != E_OK) {
            LOGE("Init cursor to meta table failed. %d", errCode);
        }
        return errCode;
    }
    if (errCode != E_OK) {
        LOGE("Get cursor from meta table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::SetLogTriggerStatus(sqlite3 *db, bool status)
{
    const std::string key = "log_trigger_switch";
    std::string val = status ? "true" : "false";
    std::string sql = "INSERT OR REPLACE INTO " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata" +
        " VALUES ('" + key + "', '" + val + "')";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("Set log trigger to %s failed. errCode=%d", val.c_str(), errCode);
    }
    return errCode;
}

int SQLiteRelationalUtils::GeneLogInfoForExistedData(const std::string &identity, const TableInfo &tableInfo,
    std::unique_ptr<SqliteLogTableManager> &logMgrPtr, GenLogParam &param)
{
    std::string tableName = tableInfo.GetTableName();
    int64_t timeOffset = 0;
    int errCode = GetExistedDataTimeOffset(param.db, tableName, param.isMemory, timeOffset);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SetLogTriggerStatus(param.db, false);
    if (errCode != E_OK) {
        return errCode;
    }
    std::string timeOffsetStr = std::to_string(timeOffset);
    std::string logTable = DBConstant::RELATIONAL_PREFIX + tableName + "_log";
    std::string rowid = std::string(DBConstant::SQLITE_INNER_ROWID);
    std::string flag = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_LOCAL) |
        static_cast<uint32_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY));
    TrackerTable trackerTable = tableInfo.GetTrackerTable();
    trackerTable.SetTableName(tableName);
    const std::string prefix = "a.";
    std::string calPrimaryKeyHash = logMgrPtr->CalcPrimaryKeyHash(prefix, tableInfo, identity);
    std::string sql = "INSERT OR REPLACE INTO " + logTable + " SELECT " + rowid +
        ", '', '', " + timeOffsetStr + " + " + rowid + ", " +
        timeOffsetStr + " + " + rowid + ", " + flag + ", " + calPrimaryKeyHash + ", '', ";
    sql += GetExtendValue(tableInfo.GetTrackerTable());
    sql += ", 0, '', '', 0 FROM '" + tableName + "' AS a ";
    if (param.isTrackerTable) {
        sql += " WHERE 1 = 1;";
    } else {
        sql += "WHERE NOT EXISTS (SELECT 1 FROM " + logTable + " WHERE data_key = a._rowid_);";
    }
    errCode = trackerTable.ReBuildTempTrigger(param.db, TriggerMode::TriggerModeEnum::INSERT, [db = param.db, &sql]() {
        int ret = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (ret != E_OK) {
            LOGE("Failed to initialize cloud type log data.%d", ret);
        }
        return ret;
    });
    return errCode;
}

int SQLiteRelationalUtils::GetExistedDataTimeOffset(sqlite3 *db, const std::string &tableName, bool isMem,
    int64_t &timeOffset)
{
    std::string sql = "SELECT get_sys_time(0) - max(" + std::string(DBConstant::SQLITE_INNER_ROWID) + ") - 1 FROM '" +
        tableName + "';";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, isMem);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        timeOffset = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
        errCode = E_OK;
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

std::string SQLiteRelationalUtils::GetExtendValue(const TrackerTable &trackerTable)
{
    std::string extendValue;
    const std::set<std::string> &extendNames = trackerTable.GetExtendNames();
    if (!extendNames.empty()) {
        extendValue += "json_object(";
        for (const auto &extendName : extendNames) {
            extendValue += "'" + extendName + "'," + extendName + ",";
        }
        extendValue.pop_back();
        extendValue += ")";
    } else {
        extendValue = "''";
    }
    return extendValue;
}

int SQLiteRelationalUtils::CleanTrackerData(sqlite3 *db, const std::string &tableName, int64_t cursor,
    bool isOnlyTrackTable)
{
    std::string sql;
    if (isOnlyTrackTable) {
        sql = "DELETE FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName + "_log";
    } else {
        sql = "UPDATE " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName + "_log SET extend_field = NULL";
    }
    sql += " where data_key = -1 and cursor <= ?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        LOGE("get clean tracker data stmt failed %d.", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(statement, 1, cursor);
    int ret = E_OK;
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        LOGE("bind clean tracker data stmt failed %d.", errCode);
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) { // LCOV_EXCL_BR_LINE
        errCode = E_OK;
    } else {
        LOGE("clean tracker step failed: %d.", errCode);
    }
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode == E_OK ? ret : errCode;
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
} // namespace DistributedDB