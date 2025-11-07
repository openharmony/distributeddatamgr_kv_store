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
#include "knowledge_source_utils.h"

#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "db_errno.h"
#include "res_finalizer.h"
#include "knowledge_log_table_manager.h"
#include "sqlite_relational_utils.h"
#include "sqlite_utils.h"

namespace DistributedDB {
namespace {
int GenLogForExistData(sqlite3 *db, const TableInfo &table, std::unique_ptr<SqliteLogTableManager> &manager)
{
    SQLiteRelationalUtils::GenLogParam param = {db, false, true};
    return SQLiteRelationalUtils::GeneLogInfoForExistedData("", table, manager, param);
}

TrackerTable GetTrackerTable(const KnowledgeSourceSchema &schema)
{
    TrackerTable trackerTable;
    trackerTable.SetTableName(schema.tableName);
    trackerTable.SetExtendNames(schema.extendColNames);
    trackerTable.SetTrackerNames(schema.knowledgeColNames);
    trackerTable.SetTriggerObserver(false);
    trackerTable.SetKnowledgeTable(true);
    return trackerTable;
}

TrackerSchema GetTrackerSchema(const KnowledgeSourceSchema &schema)
{
    TrackerSchema trackerSchema;
    trackerSchema.tableName = schema.tableName;
    trackerSchema.extendColNames = schema.extendColNames;
    trackerSchema.trackerColNames = schema.knowledgeColNames;
    return trackerSchema;
}
}

constexpr const char *PROCESS_SEQ_KEY = "processSequence";
constexpr const char *KNOWLEDGE_CURSOR_PREFIX = "knowledge_cursor_";

int KnowledgeSourceUtils::CheckProcessSequence(sqlite3 *db, const std::string &tableName,
    const std::string &columnName)
{
    // empty column name is ok
    if (columnName.empty()) {
        return E_OK;
    }
    if (db == nullptr) {
        return -E_INVALID_DB;
    }
    std::string checkColumnSql = "SELECT COUNT(1) FROM pragma_table_info('" + tableName + "')" +
        " WHERE name = ? AND type LIKE '%int%'";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, checkColumnSql, stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("Get check column statement failed. err=%d", errCode);
        return errCode;
    }

    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("Reset stmt failed when check column type %d", ret);
        }
    });

    errCode = SQLiteUtils::BindTextToStatement(stmt, 1, columnName);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("Bind column name to statement failed. err=%d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    // should always return a row of data
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("Check column type failed. err=%d", errCode);
        return errCode;
    }
    bool isCheckSuccess = (sqlite3_column_int(stmt, 0) == 1);
    return isCheckSuccess ? E_OK : -E_INVALID_ARGS;
}

int KnowledgeSourceUtils::SetKnowledgeSourceSchema(sqlite3 *db, const KnowledgeSourceSchema &schema)
{
    int errCode = SQLiteUtils::BeginTransaction(db, TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("Begin transaction failed %d when set knowledge schema", errCode);
        return errCode;
    }
    errCode = SetKnowledgeSourceSchemaInner(db, schema);
    int ret;
    if (errCode != E_OK) {
        ret = SQLiteUtils::RollbackTransaction(db);
    } else {
        ret = SQLiteUtils::CommitTransaction(db);
    }
    LOGI("Set knowledge source schema res %d, commit or rollback ret %d", errCode, ret);
    return errCode == E_OK ? ret : errCode;
}

int KnowledgeSourceUtils::CleanDeletedData(sqlite3 *db, const std::string &tableName, uint64_t cursor)
{
    bool isExist = false;
    auto errCode = SQLiteUtils::CheckTableExists(db, DBCommon::GetLogTableName(tableName), isExist);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isExist) {
        LOGI("Clean delete data with no exist log, ori table %s len %zu",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
        return E_OK;
    }
    errCode = SQLiteRelationalUtils::CleanTrackerData(db, tableName, static_cast<int64_t>(cursor), true);
    if (errCode != E_OK) {
        return errCode;
    }
    auto count = sqlite3_changes64(db);
    LOGI("Clean delete data success, table %s len %zu cursor %" PRIu64 " delete count %" PRId64,
        DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size(), cursor, count);
    return errCode;
}

int KnowledgeSourceUtils::CheckSchemaFields(sqlite3 *db, const KnowledgeSourceSchema &schema)
{
    bool isCheckSuccess = false;
    auto errCode = SQLiteUtils::CheckTableExists(db, schema.tableName, isCheckSuccess);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCheckSuccess) {
        LOGE("Set not exist table's knowledge schema");
        return -E_INVALID_ARGS;
    }

    auto it = schema.columnsToVerify.find(PROCESS_SEQ_KEY);
    if (it == schema.columnsToVerify.end()) {
        return E_OK;
    }
    for (const auto &columnName : it->second) {
        errCode = CheckProcessSequence(db, schema.tableName, columnName);
        if (errCode != E_OK) {
            LOGE("column type not match, name %s, size %zu", DBCommon::StringMiddleMasking(columnName).c_str(),
                columnName.size());
            return errCode;
        }
    }
    return E_OK;
}

int KnowledgeSourceUtils::SetKnowledgeSourceSchemaInner(sqlite3 *db, const KnowledgeSourceSchema &schema)
{
    int errCode = CheckSchemaFields(db, schema);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = InitMeta(db, schema.tableName);
    if (errCode != E_OK) {
        return errCode;
    }
    RelationalSchemaObject knowledgeSchema;
    std::tie(errCode, knowledgeSchema) = GetKnowledgeSourceSchema(db);
    if (errCode != E_OK) {
        return errCode;
    }
    TableInfo tableInfo;
    errCode = SQLiteUtils::AnalysisSchema(db, schema.tableName, tableInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    tableInfo.SetTrackerTable(GetTrackerTable(schema));
    bool isChanged = false;
    std::tie(errCode, isChanged) = CheckSchemaValidAndChangeStatus(db, knowledgeSchema, schema, tableInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isChanged) {
        LOGI("Knowledge schema is no change, table %s len %zu",
            DBCommon::StringMiddleMasking(schema.tableName).c_str(), schema.tableName.size());
        return UpdateFlagAndTriggerIfNeeded(db, tableInfo);
    }
    errCode = InitLogTable(db, schema, tableInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    knowledgeSchema.InsertTrackerSchema(GetTrackerSchema(schema));
    return SaveKnowledgeSourceSchema(db, knowledgeSchema);
}

int KnowledgeSourceUtils::UpdateFlagAndTriggerIfNeeded(sqlite3 *db, const TableInfo &tableInfo)
{
    std::string updateTriggerName = "naturalbase_rdb_" + tableInfo.GetTableName() + "_ON_UPDATE";

    bool needUpdate = false;
    int errCode = CheckUpdateTriggerVersion(db, updateTriggerName, tableInfo.GetTableName(), needUpdate);
    if (errCode != E_OK) {
        LOGE("Check knowledge table trigger err %d", errCode);
        return errCode;
    }

    std::unique_ptr<SqliteLogTableManager> tableManager = std::make_unique<KnowledgeLogTableManager>();
    if (!needUpdate) {
        tableManager->CheckAndCreateTrigger(db, tableInfo, "");
        return E_OK;
    }

    LOGI("Knowledge table trigger needs update");
    errCode = UpdateKnowledgeFlag(db, tableInfo);
    if (errCode != E_OK) {
        LOGE("Update knowledge flag err %d", errCode);
        return errCode;
    }

    errCode = tableManager->AddRelationalLogTableTrigger(db, tableInfo, "");
    if (errCode != E_OK) {
        LOGE("Update existing trigger err %d", errCode);
    }
    return errCode;
}

int KnowledgeSourceUtils::CheckUpdateTriggerVersion(sqlite3 *db, const std::string &triggerName,
    const std::string &tableName, bool &needUpdate)
{
    std::string checkSql = "select sql from sqlite_master where type = 'trigger' and tbl_name = '" +
        tableName + "' and name = '" + triggerName + "';";

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, checkSql, stmt);
    if (errCode != E_OK) {
        LOGE("[CheckUpdateTriggerVersion] Get statement err:%d", errCode);
        return errCode;
    }

    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("Reset stmt failed when check trigger version %d", ret);
        }
    });

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    }
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("Get trigger err: %d", errCode);
        return errCode;
    }

    std::string trigger;
    errCode = SQLiteUtils::GetColumnTextValue(stmt, 0, trigger);
    if (errCode != E_OK) {
        LOGE("Get trigger statement err: %d", errCode);
        return errCode;
    }

    std::string newPattern = "cursor=" + CloudStorageUtils::GetSelectIncCursorSql(tableName);
    size_t pos = trigger.find(newPattern);
    // trigger is new version if pattern matches
    needUpdate = (pos == std::string::npos);
    return E_OK;
}

int KnowledgeSourceUtils::UpdateKnowledgeFlag(sqlite3 *db, const TableInfo &tableInfo)
{
    int64_t cursor = 0;
    int errCode = GetKnowledgeCursor(db, tableInfo, cursor);
    if (errCode != E_OK) {
        LOGE("Get knowledge cursor err:%d", errCode);
        return errCode;
    }

    std::string logTblName = DBConstant::RELATIONAL_PREFIX + tableInfo.GetTableName() + "_log";
    uint32_t newFlag = static_cast<uint32_t>(LogInfoFlag::FLAG_KNOWLEDGE_INVERTED_WRITE) |
        static_cast<uint32_t>(LogInfoFlag::FLAG_KNOWLEDGE_VECTOR_WRITE);
    std::string sql = "UPDATE " + logTblName + " SET flag = flag | " + std::to_string(newFlag) + " WHERE cursor <= ?;";

    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[UpdateKnowledgeFlag] Get statement err:%d", errCode);
        return errCode;
    }

    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("Reset stmt failed when update knowledge flag %d", ret);
        }
    });

    errCode = SQLiteUtils::BindInt64ToStatement(stmt, 1, cursor);
    if (errCode != E_OK) {
        LOGE("UpdateKnowledgeFlag bind arg err:%d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("Execute update flag statement err:%d", errCode);
        return errCode;
    }
    return E_OK;
}

int KnowledgeSourceUtils::GetKnowledgeCursor(sqlite3 *db, const TableInfo &tableInfo, int64_t &cursor)
{
    std::string cursorKey = std::string(KNOWLEDGE_CURSOR_PREFIX) + tableInfo.GetTableName();
    std::string cursorSql = "SELECT value FROM naturalbase_rdb_aux_metadata WHERE key = '" + cursorKey + "';";

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, cursorSql, stmt);
    if (errCode != E_OK) {
        LOGE("[GetKnowledgeCursor] Get statement err:%d", errCode);
        return errCode;
    }

    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("Reset stmt failed when get knowledge cursor %d", ret);
        }
    });

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        LOGI("Not found knowledge cursor");
        return E_OK;
    }
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("Get knowledge cursor err:%d", errCode);
        return errCode;
    }
    cursor = sqlite3_column_int64(stmt, 0);
    return E_OK;
}

int KnowledgeSourceUtils::RemoveKnowledgeTableSchema(sqlite3 *db, const std::string &tableName)
{
    int errCode = E_OK;
    RelationalSchemaObject knowledgeSchema;
    std::tie(errCode, knowledgeSchema) = GetKnowledgeSourceSchema(db);
    if (errCode != E_OK) {
        return errCode;
    }
    knowledgeSchema.RemoveRelationalTable(tableName);
    return SaveKnowledgeSourceSchema(db, knowledgeSchema);
}

int KnowledgeSourceUtils::InitMeta(sqlite3 *db, const std::string &table)
{
    int errCode = SQLiteRelationalUtils::CreateRelationalMetaTable(db);
    if (errCode != E_OK) {
        LOGE("Create relational store meta table failed. err=%d", errCode);
        return errCode;
    }
    errCode = SQLiteRelationalUtils::InitKnowledgeTableTypeToMeta(db, false, table);
    if (errCode != E_OK) {
        LOGE("Init table type to meta err:%d", errCode);
        return errCode;
    }
    return SQLiteRelationalUtils::InitCursorToMeta(db, false, table);
}

std::pair<int, bool> KnowledgeSourceUtils::CheckSchemaValidAndChangeStatus(sqlite3 *db,
    const RelationalSchemaObject &knowledgeSchema,
    const KnowledgeSourceSchema &schema, const TableInfo &tableInfo)
{
    std::pair<int, bool> res = {E_OK, false};
    auto &[errCode, isChange] = res;
    if (!IsSchemaValid(schema, tableInfo)) {
        errCode = -E_INVALID_ARGS;
        return res;
    }
    RelationalSchemaObject rdbSchema;
    std::tie(errCode, rdbSchema) = GetRDBSchema(db, false);
    if (errCode != E_OK) {
        return res;
    }
    if (IsTableInRDBSchema(schema.tableName, rdbSchema, false)) {
        errCode = -E_INVALID_ARGS;
        return res;
    }
    std::tie(errCode, rdbSchema) = GetRDBSchema(db, true);
    if (errCode != E_OK) {
        return res;
    }
    if (IsTableInRDBSchema(schema.tableName, rdbSchema, true)) {
        errCode = -E_INVALID_ARGS;
        return res;
    }
    isChange = IsSchemaChange(knowledgeSchema, schema);
    return res;
}

bool KnowledgeSourceUtils::IsSchemaValid(const KnowledgeSourceSchema &schema, const TableInfo &tableInfo)
{
    auto fields = tableInfo.GetFields();
    for (const auto &col : schema.knowledgeColNames) {
        if (fields.find(col) == fields.end()) {
            LOGE("Not exist knowledge col %s %zu table %s %zu",
                DBCommon::StringMiddleMasking(col).c_str(), col.size(),
                DBCommon::StringMiddleMasking(schema.tableName).c_str(), schema.tableName.size());
            return false;
        }
    }
    for (const auto &col : schema.extendColNames) {
        if (fields.find(col) == fields.end()) {
            LOGE("Not exist extend col %s %zu table %s %zu",
                 DBCommon::StringMiddleMasking(col).c_str(), col.size(),
                 DBCommon::StringMiddleMasking(schema.tableName).c_str(), schema.tableName.size());
            return false;
        }
    }
    return true;
}

bool KnowledgeSourceUtils::IsTableInRDBSchema(const std::string &table, const RelationalSchemaObject &rdbSchema,
    bool isTracker)
{
    if (isTracker) {
        auto trackerTable = rdbSchema.GetTrackerTable(table);
        if (!trackerTable.GetTableName().empty()) {
            LOGE("Table %s len %zu was tracker table", DBCommon::StringMiddleMasking(table).c_str(), table.size());
            return true;
        }
        return false;
    }
    auto tableInfo = rdbSchema.GetTable(table);
    if (!tableInfo.GetTableName().empty()) {
        LOGE("Table %s len %zu was distributed table, type %d", DBCommon::StringMiddleMasking(table).c_str(),
            table.size(), static_cast<int>(tableInfo.GetTableSyncType()));
        return true;
    }
    return false;
}

bool KnowledgeSourceUtils::IsSchemaChange(const RelationalSchemaObject &dbSchema, const KnowledgeSourceSchema &schema)
{
    auto trackerTable = dbSchema.GetTrackerTable(schema.tableName);
    if (trackerTable.IsEmpty()) {
        LOGI("Check knowledge schema was change by first set");
        return true;
    }
    const auto &dbExtendsNames = trackerTable.GetExtendNames();
    if (dbExtendsNames != schema.extendColNames) {
        LOGI("Check knowledge schema was change by extend col old %zu new %zu",
            dbExtendsNames.size(), schema.extendColNames.size());
        return true;
    }
    const auto &dbTrackerCols = trackerTable.GetTrackerColNames();
    if (dbTrackerCols != schema.knowledgeColNames) {
        LOGI("Check knowledge schema was change by knowledge col old %zu new %zu",
            dbTrackerCols.size(), schema.knowledgeColNames.size());
        return true;
    }
    return false;
}

int KnowledgeSourceUtils::InitLogTable(sqlite3 *db, const KnowledgeSourceSchema &schema, const TableInfo &tableInfo)
{
    std::unique_ptr<SqliteLogTableManager> tableManager = std::make_unique<KnowledgeLogTableManager>();
    auto errCode = tableManager->CreateRelationalLogTable(db, tableInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GenLogForExistData(db, tableInfo, tableManager);
    if (errCode != E_OK) {
        LOGE("Gen log failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteRelationalUtils::SetLogTriggerStatus(db, true);
    if (errCode != E_OK) {
        return errCode;
    }
    return tableManager->AddRelationalLogTableTrigger(db, tableInfo, "");
}

std::pair<int, RelationalSchemaObject> KnowledgeSourceUtils::GetKnowledgeSourceSchema(sqlite3 *db)
{
    std::pair<int, RelationalSchemaObject> res;
    auto &[errCode, rdbSchema] = res;
    const std::string knowledgeKey(DBConstant::RDB_KNOWLEDGE_SCHEMA_KEY);
    const Key schemaKey(knowledgeKey.begin(), knowledgeKey.end());
    Value schemaVal;
    errCode = SQLiteRelationalUtils::GetKvData(db, false, schemaKey, schemaVal); // save schema to meta_data
    if (errCode == -E_NOT_FOUND) {
        LOGD("Not found knowledge schema in db");
        errCode = E_OK;
        return res;
    }
    if (errCode != E_OK) {
        LOGE("Get knowledge schema from meta table failed. %d", errCode);
        return res;
    }
    std::string schemaJson(schemaVal.begin(), schemaVal.end());
    errCode = rdbSchema.ParseFromTrackerSchemaString(schemaJson);
    return res;
}

std::pair<int, RelationalSchemaObject> KnowledgeSourceUtils::GetRDBSchema(sqlite3 *db, bool isTracker)
{
    std::pair<int, RelationalSchemaObject> res;
    auto &[errCode, rdbSchema] = res;
    std::string schemaKey = isTracker ? DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY : DBConstant::RELATIONAL_SCHEMA_KEY;
    const Key schema(schemaKey.begin(), schemaKey.end());
    Value schemaVal;
    errCode = SQLiteRelationalUtils::GetKvData(db, false, schema, schemaVal); // save schema to meta_data
    if (errCode == -E_NOT_FOUND) {
        LOGD("Not found rdb schema in db");
        errCode = E_OK;
        return res;
    }
    if (errCode != E_OK) {
        LOGE("Get rdb schema from meta table failed. %d", errCode);
        return res;
    }
    std::string schemaJson(schemaVal.begin(), schemaVal.end());
    if (isTracker) {
        errCode = rdbSchema.ParseFromTrackerSchemaString(schemaJson);
    } else {
        errCode = rdbSchema.ParseFromSchemaString(schemaJson);
    }
    return res;
}

int KnowledgeSourceUtils::SaveKnowledgeSourceSchema(sqlite3 *db, const RelationalSchemaObject &schema)
{
    const std::string knowledgeKey(DBConstant::RDB_KNOWLEDGE_SCHEMA_KEY);
    const Key schemaKey(knowledgeKey.begin(), knowledgeKey.end());
    Value schemaVal;
    DBCommon::StringToVector(schema.ToSchemaString(), schemaVal);
    int errCode = SQLiteRelationalUtils::PutKvData(db, false, schemaKey, schemaVal); // save schema to meta_data
    if (errCode != E_OK) {
        LOGE("Save schema to meta table failed. %d", errCode);
    }
    return errCode;
}
}