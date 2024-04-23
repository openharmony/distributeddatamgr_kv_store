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
#include "tracker_table.h"
#include "schema_constant.h"

namespace DistributedDB {
void TrackerTable::Init(const TrackerSchema &schema)
{
    tableName_ = schema.tableName;
    extendColName_ = schema.extendColName;
    trackerColNames_ = schema.trackerColNames;
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
    if (trackerColNames_.empty()) {
        return "0";
    }
    std::string sql = " case when (";
    size_t index = 0;
    for (const auto &colName: trackerColNames_) {
        sql += "(NEW." + colName + " IS NOT OLD." + colName + ")";
        if (index < trackerColNames_.size() - 1) {
            sql += " or ";
        }
        index++;
    }
    sql += ") then 1 else 0 end";
    return sql;
}

const std::string TrackerTable::GetDiffIncCursorSql(const std::string &tableName) const
{
    if (trackerColNames_.empty()) {
        return "";
    }
    std::string sql = ", cursor = case when (";
    size_t index = 0;
    for (const auto &colName: trackerColNames_) {
        sql += "(NEW." + colName + " IS NOT OLD." + colName + ")";
        if (index < trackerColNames_.size() - 1) {
            sql += " or ";
        }
        index++;
    }
    sql += ") then (SELECT case when (MAX(cursor) is null) then 1 else (MAX(cursor) + 1) END ";
    sql += " from " + tableName + ") ";
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
    attrStr.pop_back();
    attrStr += "]}";
    return attrStr;
}

const std::vector<std::string> TrackerTable::GetDropTempTriggerSql() const
{
    if (IsEmpty()) {
        return {};
    }
    std::vector<std::string> dropSql;
    dropSql.push_back("DROP TRIGGER IF EXISTS " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_ON_INSERT_TEMP;");
    dropSql.push_back("DROP TRIGGER IF EXISTS " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_ON_UPDATE_TEMP;");
    dropSql.push_back("DROP TRIGGER IF EXISTS " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_ON_DELETE_TEMP;");
    return dropSql;
}

const std::string TrackerTable::GetTempInsertTriggerSql() const
{
    // This trigger is built on the log table
    std::string sql = "CREATE TEMP TRIGGER IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + tableName_;
    sql += "_ON_INSERT_TEMP AFTER INSERT ON " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log" +
        " WHEN (SELECT count(1) FROM " + DBConstant::RELATIONAL_PREFIX + "metadata" +
        " WHERE key = 'log_trigger_switch' AND value = 'false')\n";
    sql += "BEGIN\n";
    sql += "UPDATE " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log" + " SET ";
    sql += "cursor = (SELECT case when (MAX(cursor) is null) then 1 else MAX(cursor) + 1 END ";
    sql += "FROM " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log" + ") WHERE ";
    sql += " hash_key = NEW.hash_key;\n";
    if (!IsEmpty()) {
        sql += "SELECT server_observer('" + tableName_ + "', 1);";
    }
    sql += "\nEND;";
    return sql;
}

const std::string TrackerTable::GetTempUpdateTriggerSql() const
{
    std::string sql = "CREATE TEMP TRIGGER IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + tableName_;
    sql += "_ON_UPDATE_TEMP AFTER UPDATE ON " + tableName_ +
        " WHEN (SELECT count(1) FROM " + DBConstant::RELATIONAL_PREFIX + "metadata" +
        " WHERE key = 'log_trigger_switch' AND value = 'false')\n";
    sql += "BEGIN\n";
    sql += "UPDATE " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log" + " SET ";
    if (!IsEmpty()) {
        sql += "extend_field=" + GetAssignValSql() + ",";
    }
    sql += " cursor = (SELECT case when (MAX(cursor) is null) then 1 else MAX(cursor) + 1 END ";
    sql += "FROM " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log" + ") WHERE ";
    sql += " data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    if (!IsEmpty()) {
        sql += "SELECT server_observer('" + tableName_ + "', " + GetDiffTrackerValSql() + ");";
    }
    sql += "\nEND;";
    return sql;
}

const std::string TrackerTable::GetTempDeleteTriggerSql() const
{
    std::string sql = "CREATE TEMP TRIGGER IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + tableName_;
    sql += "_ON_DELETE_TEMP AFTER DELETE ON " + tableName_ +
        " WHEN (SELECT count(1) FROM " + DBConstant::RELATIONAL_PREFIX + "metadata" +
        " WHERE key = 'log_trigger_switch' AND value = 'false')\n";
    sql += "BEGIN\n";
    sql += "UPDATE " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log" + " SET ";
    if (!IsEmpty()) {
        sql += "extend_field=" + GetAssignValSql(true) + ",";
    }
    sql += " cursor = (SELECT case when (MAX(cursor) is null) then 1 else MAX(cursor) + 1 END ";
    sql += "FROM " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log" + ") WHERE ";
    sql += " data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    if (!IsEmpty()) {
        sql += "SELECT server_observer('" + tableName_ + "', 1);";
    }
    sql += "\nEND;";
    return sql;
}

const std::string TrackerTable::GetUpgradedExtendValSql() const
{
    if (IsEmpty() || extendColName_.empty()) {
        return "''";
    }
    std::string sql = " (SELECT " + extendColName_ + " from " + tableName_ + " WHERE " + tableName_ + "." +
        std::string(DBConstant::SQLITE_INNER_ROWID) +
        " = " + DBConstant::RELATIONAL_PREFIX + tableName_ + "_log.data_key) ";
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
    return trackerColNames_.empty();
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
    return false;
}
}
#endif