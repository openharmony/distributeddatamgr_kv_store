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

#include "db_common.h"
#include "db_errno.h"
#include "simple_tracker_log_table_manager.h"
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

int KnowledgeSourceUtils::SetKnowledgeSourceSchemaInner(sqlite3 *db, const KnowledgeSourceSchema &schema)
{
    bool isExist = false;
    auto errCode = SQLiteUtils::CheckTableExists(db, schema.tableName, isExist);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isExist) {
        LOGE("Set not exist table's knowledge schema");
        return -E_INVALID_ARGS;
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
        std::unique_ptr<SqliteLogTableManager> tableManager = std::make_unique<SimpleTrackerLogTableManager>();
        tableManager->CheckAndCreateTrigger(db, tableInfo, "");
        return E_OK;
    }
    errCode = InitLogTable(db, schema, tableInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    knowledgeSchema.InsertTrackerSchema(GetTrackerSchema(schema));
    return SaveKnowledgeSourceSchema(db, knowledgeSchema);
}

int KnowledgeSourceUtils::InitMeta(sqlite3 *db, const std::string &table)
{
    int errCode = SQLiteRelationalUtils::CreateRelationalMetaTable(db);
    if (errCode != E_OK) {
        LOGE("Create relational store meta table failed. err=%d", errCode);
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
    std::unique_ptr<SqliteLogTableManager> tableManager = std::make_unique<SimpleTrackerLogTableManager>();
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