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
#ifdef RELATIONAL_STORE
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "tracker_table.h"
#include "schema_constant.h"

namespace DistributedDB {
void TrackerTable::Init(const TrackerSchema &schema)
{
    tableName_ = schema.tableName;
    extendColName_ = schema.extendColName;
    trackerColNames_ = schema.trackerColNames;
    isTrackerAction_ = schema.isTrackAction;
}

std::string TrackerTable::GetTableName() const
{
    return tableName_;
}

const std::set<std::string> &TrackerTable::GetTrackerColNames() const
{
    return trackerColNames_;
}

const std::string TrackerTable::GetAssignValSql(bool isDelete) const
{
    if (extendColName_.empty()) {
        return "''";
    }
    return isDelete ? ("OLD." + extendColName_) : ("NEW." + extendColName_);
}

const std::string TrackerTable::GetExtendAssignValSql(bool isDelete) const
{
    if (extendColName_.empty()) {
        return "";
    }
    return isDelete ? (", extend_field = OLD." + extendColName_) : (", extend_field = NEW." + extendColName_);
}

const std::string TrackerTable::GetDiffTrackerValSql() const
{
    if (trackerColNames_.empty() || isTrackerAction_) {
        return isTrackerAction_ ? "1" : "0";
    }
    std::string sql = " CASE WHEN (";
    size_t index = 0;
    for (const auto &colName: trackerColNames_) {
        sql += "(NEW." + colName + " IS NOT OLD." + colName + ")";
        if (index < trackerColNames_.size() - 1) {
            sql += " OR ";
        }
        index++;
    }
    sql += ") THEN 1 ELSE 0 END";
    return sql;
}

const std::string TrackerTable::GetDiffIncCursorSql(const std::string &tableName) const
{
    if (IsEmpty()) {
        return "";
    }
    if (isTrackerAction_) {
        return std::string(", cursor = ").append(CloudStorageUtils::GetSelectIncCursorSql(tableName)).append(" ");
    }
    std::string sql = ", cursor = CASE WHEN (";
    size_t index = 0;
    for (const auto &colName: trackerColNames_) {
        sql += "(NEW." + colName + " IS NOT OLD." + colName + ")";
        if (index < trackerColNames_.size() - 1) {
            sql += " OR ";
        }
        index++;
    }
    sql += ") THEN " + CloudStorageUtils::GetSelectIncCursorSql(tableName);
    sql += " ELSE cursor END ";
    return sql;
}

const std::string TrackerTable::GetExtendName() const
{
    return extendColName_;
}

std::string TrackerTable::ToString() const
{
    std::string attrStr;
    attrStr += "{";
    attrStr += R"("NAME": ")" + tableName_ + "\",";
    attrStr += R"("EXTEND_NAME": ")" + extendColName_ + "\",";
    attrStr += R"("TRACKER_NAMES": [)";
    for (const auto &colName: trackerColNames_) {
        attrStr += "\"" + colName + "\",";
    }
    if (!trackerColNames_.empty()) {
        attrStr.pop_back();
    }
    attrStr += "],";
    attrStr += R"("TRACKER_ACTION": )";
    attrStr += isTrackerAction_ ? SchemaConstant::KEYWORD_ATTR_VALUE_TRUE : SchemaConstant::KEYWORD_ATTR_VALUE_FALSE;
    attrStr += "}";
    return attrStr;
}

const std::vector<std::string> TrackerTable::GetDropTempTriggerSql() const
{
    if (IsEmpty()) {
        return {};
    }
    std::vector<std::string> dropSql;
    dropSql.push_back(GetDropTempTriggerSql(TriggerMode::TriggerModeEnum::INSERT));
    dropSql.push_back(GetDropTempTriggerSql(TriggerMode::TriggerModeEnum::UPDATE));
    dropSql.push_back(GetDropTempTriggerSql(TriggerMode::TriggerModeEnum::DELETE));
    return dropSql;
}

const std::string TrackerTable::GetDropTempTriggerSql(TriggerMode::TriggerModeEnum mode) const
{
    return "DROP TRIGGER IF EXISTS " + GetTempTriggerName(mode);
}

const std::string TrackerTable::GetCreateTempTriggerSql(TriggerMode::TriggerModeEnum mode) const
{
    switch (mode) {
        case TriggerMode::TriggerModeEnum::INSERT:
            return GetTempInsertTriggerSql();
        case TriggerMode::TriggerModeEnum::UPDATE:
            return GetTempUpdateTriggerSql();
        case TriggerMode::TriggerModeEnum::DELETE:
            return GetTempDeleteTriggerSql();
        default:
            return {};
    }
}

const std::string TrackerTable::GetTempTriggerName(TriggerMode::TriggerModeEnum mode) const
{
    return DBConstant::RELATIONAL_PREFIX + tableName_ + "_ON_" + TriggerMode::GetTriggerModeString(mode) + "_TEMP";
}

const std::string TrackerTable::GetTempInsertTriggerSql(bool incFlag) const
{
    // This trigger is built on the log table
    std::string sql = "CREATE TEMP TRIGGER IF NOT EXISTS " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName_;
    sql += "_ON_INSERT_TEMP AFTER INSERT ON " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName_ + "_log";
    sql += " WHEN (SELECT 1 FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata" +
        " WHERE key = 'log_trigger_switch' AND value = 'false')\n";
    sql += "BEGIN\n";
    if (incFlag) {
        sql += CloudStorageUtils::GetCursorIncSqlWhenAllow(tableName_) + "\n";
    } else {
        sql += CloudStorageUtils::GetCursorIncSql(tableName_) + "\n";
    }
    sql += "UPDATE " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName_ + "_log" + " SET ";
    if (incFlag) {
        sql += "cursor= case when (select 1 from " + std::string(DBConstant::RELATIONAL_PREFIX) +
            "metadata where key='cursor_inc_flag' AND value = 'true') then " +
            CloudStorageUtils::GetSelectIncCursorSql(tableName_) + " else cursor end WHERE";
    } else {
        sql += "cursor=" + CloudStorageUtils::GetSelectIncCursorSql(tableName_) + " WHERE";
    }
    sql += " hash_key = NEW.hash_key;\n";
    if (!IsEmpty()) {
        sql += "SELECT server_observer('" + tableName_ + "', 1);";
    }
    sql += "\nEND;";
    return sql;
}

const std::string TrackerTable::GetTempUpdateTriggerSql(bool incFlag) const
{
    std::string sql = "CREATE TEMP TRIGGER IF NOT EXISTS " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName_;
    sql += "_ON_UPDATE_TEMP AFTER UPDATE ON " + tableName_;
    sql += " WHEN (SELECT 1 FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata" +
        " WHERE key = 'log_trigger_switch' AND value = 'false')\n";
    sql += "BEGIN\n";
    if (incFlag) {
        sql += CloudStorageUtils::GetCursorIncSqlWhenAllow(tableName_) + "\n";
    } else {
        sql += CloudStorageUtils::GetCursorIncSql(tableName_) + "\n";
    }
    sql += "UPDATE " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName_ + "_log" + " SET ";
    if (!IsEmpty()) {
        sql += "extend_field=" + GetAssignValSql() + ",";
    }
    if (incFlag) {
        sql += "cursor= case when (select 1 from " + std::string(DBConstant::RELATIONAL_PREFIX) +
            "metadata where key='cursor_inc_flag' AND value = 'true') then " +
            CloudStorageUtils::GetSelectIncCursorSql(tableName_) + " else cursor end WHERE";
    } else {
        sql += "cursor=" + CloudStorageUtils::GetSelectIncCursorSql(tableName_) + " WHERE";
    }
    sql += " data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    if (!IsEmpty()) {
        sql += "SELECT server_observer('" + tableName_ + "', " + GetDiffTrackerValSql() + ");";
    }
    sql += "\nEND;";
    return sql;
}

const std::string TrackerTable::GetTempDeleteTriggerSql(bool incFlag) const
{
    std::string sql = "CREATE TEMP TRIGGER IF NOT EXISTS " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName_;
    sql += "_ON_DELETE_TEMP AFTER DELETE ON " + tableName_ +
        " WHEN (SELECT 1 FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata" +
        " WHERE key = 'log_trigger_switch' AND value = 'false')\n";
    sql += "BEGIN\n";
    if (IsEmpty() && incFlag) {
        sql += "SELECT 1;\n";
        sql += "\nEND;";
        return sql;
    }
    if (!incFlag) {
        sql += CloudStorageUtils::GetCursorIncSql(tableName_) + "\n";
    }
    sql += "UPDATE " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName_ + "_log" + " SET ";
    if (!IsEmpty()) {
        sql += "extend_field=" + GetAssignValSql(true) + ",";
    }
    if (!incFlag) {
        sql += "cursor=" + CloudStorageUtils::GetSelectIncCursorSql(tableName_);
    }
    if (!IsEmpty() && incFlag) {
        sql.pop_back();
    }
    sql += " WHERE data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    if (!IsEmpty()) {
        sql += "SELECT server_observer('" + tableName_ + "', 1);";
    }
    sql += "\nEND;";
    return sql;
}

void TrackerTable::SetTableName(const std::string &tableName)
{
    tableName_ = tableName;
}

void TrackerTable::SetExtendName(const std::string &colName)
{
    extendColName_ = colName;
}

void TrackerTable::SetTrackerNames(const std::set<std::string> &trackerNames)
{
    trackerColNames_ = std::move(trackerNames);
}

bool TrackerTable::IsEmpty() const
{
    return trackerColNames_.empty() && !isTrackerAction_;
}

bool TrackerTable::IsTableNameEmpty() const
{
    return tableName_.empty();
}

bool TrackerTable::IsChanging(const TrackerSchema &schema)
{
    if (tableName_ != schema.tableName || extendColName_ != schema.extendColName) {
        return true;
    }
    if (trackerColNames_.size() != schema.trackerColNames.size()) {
        return true;
    }
    for (const auto &col: trackerColNames_) {
        if (schema.trackerColNames.find(col) == schema.trackerColNames.end()) {
            return true;
        }
    }
    return isTrackerAction_ != schema.isTrackAction;
}

int TrackerTable::ReBuildTempTrigger(sqlite3 *db, TriggerMode::TriggerModeEnum mode, const AfterBuildAction &action)
{
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, "SELECT 1 FROM sqlite_temp_master where name='" +
        GetTempTriggerName(mode) + "' and type='trigger' COLLATE NOCASE;", stmt);
    if (errCode != E_OK) {
        LOGE("Failed to select temp trigger mode:%d err:%d", mode, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    bool isExists = (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) ? true : false;
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW) && errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("Select temp trigger step mode:%d err:%d", mode, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, GetDropTempTriggerSql(mode));
    if (errCode != E_OK) {
        LOGE("Failed to drop temp trigger mode:%d err:%d", mode, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, GetCreateTempTriggerSql(mode));
    if (errCode != E_OK) {
        LOGE("Failed to create temp trigger mode:%d err:%d", mode, errCode);
        return errCode;
    }
    if (action != nullptr) {
        errCode = action();
        if (errCode != E_OK) {
            return errCode;
        }
    }
    if (!isExists) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, GetDropTempTriggerSql(mode));
        if (errCode != E_OK) {
            LOGE("Failed to clear temp trigger mode:%d err:%d", mode, errCode);
        }
    }
    return errCode;
}

void TrackerTable::SetTrackerAction(bool isTrackerAction)
{
    isTrackerAction_ = isTrackerAction;
}
}
#endif