/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "relational_store_client_utils.h"

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "res_finalizer.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
constexpr int MAX_MONITOR_TABLE_COUNT = 10;
constexpr int MAX_MONITOR_COLUMN_COUNT = 100;
constexpr int E_ERROR = 1;
int RelationalStoreClientUtils::UpdateDataLog(sqlite3 *db, const DistributedDB::UpdateOption &option)
{
    auto errCode = CheckUpdateOption(db, option);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isCreate = false;
    errCode = SQLiteUtils::CheckTableExists(db, DBCommon::GetMetaTableName(), isCreate);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCreate) {
        LOGE("[RDBClientUtils] UpdateDataLog meta not found",
            DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str());
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    auto [ret, rdbSchema] = GetRDBSchema(db, false);
    if (ret != E_OK) {
        return ret;
    }
    isCreate = false;
    errCode = SQLiteUtils::CheckTableExists(db, option.tableName, isCreate);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCreate) {
        LOGE("[RDBClientUtils] UpdateDataLog table[%s] not found",
            DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str());
        return -E_TABLE_NOT_FOUND;
    }
    auto tableInfo = rdbSchema.GetTable(option.tableName);
    if (tableInfo.GetTableName().empty()) {
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    auto tableMode = rdbSchema.GetTableMode();
    if (tableMode != DistributedTableMode::COLLABORATION) {
        LOGE("[RDBClientUtils] UpdateDataLog table[%s] mode[%d] not collaboration",
            DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str(), static_cast<int>(tableMode));
        return -E_NOT_SUPPORT;
    }
    return UpdateDataLogInner(db, option);
}

std::pair<int, RelationalSchemaObject> RelationalStoreClientUtils::GetRDBSchema(sqlite3 *db, bool isTracker)
{
    std::pair<int, RelationalSchemaObject> res;
    auto &[errCode, rdbSchema] = res;
    std::string schemaKey = isTracker ? DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY
                                      : DBConstant::RELATIONAL_SCHEMA_KEY;
    const Key schema(schemaKey.begin(), schemaKey.end());
    Value schemaVal;
    errCode = SQLiteRelationalUtils::GetKvData(db, false, schema, schemaVal); // save schema to meta_data
    if (errCode == -E_NOT_FOUND) {
        LOGD("[RDBClientUtils] Not found rdb schema in db");
        errCode = E_OK;
        return res;
    }
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Get rdb schema from meta table failed. %d", errCode);
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

int RelationalStoreClientUtils::CheckUpdateOption(sqlite3 *db, const UpdateOption &option)
{
    if (db == nullptr) {
        LOGE("[RDBClientUtils] CheckUpdateOption db is nullptr");
        return -E_INVALID_ARGS;
    }
    if (option.tableName.empty()) {
        LOGE("[RDBClientUtils] CheckUpdateOption tableName is empty");
        return -E_INVALID_ARGS;
    }
    if (option.condition.logCondition.has_value() && option.condition.dataCondition.has_value()) {
        LOGE("[RDBClientUtils] CheckUpdateOption both condition exists");
        return -E_INVALID_ARGS;
    }
    if (!option.condition.logCondition.has_value() && !option.condition.dataCondition.has_value()) {
        LOGE("[RDBClientUtils] CheckUpdateOption both condition not exists");
        return -E_INVALID_ARGS;
    }
    int errCode = CheckSelectCondition(option.condition.logCondition, "log");
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = CheckSelectCondition(option.condition.dataCondition, "data");
    if (errCode != E_OK) {
        return errCode;
    }
    return CheckUpdateContent(option.content);
}

int RelationalStoreClientUtils::CheckSelectCondition(const std::optional<SelectCondition> &condition,
    const std::string &dfxLog)
{
    if (!condition.has_value()) {
        return E_OK;
    }
    auto &cd = condition.value();
    if (cd.sql.empty()) {
        LOGE("[RDBClientUtils] CheckSelectCondition %s sql is empty", dfxLog.c_str());
        return -E_INVALID_ARGS;
    }
    int count = std::count(cd.sql.begin(), cd.sql.end(), '?');
    if (static_cast<size_t>(count) != cd.args.size()) {
        LOGE("[RDBClientUtils] CheckSelectCondition %s args[%d] not match sql[%zu]", dfxLog.c_str(), count,
            cd.args.size());
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RelationalStoreClientUtils::CheckUpdateContent(const UpdateContent &content)
{
    if (!content.flag.has_value() && !content.oriDevice.has_value()) {
        LOGE("[RDBClientUtils] CheckUpdateContent update content not exists");
        return -E_INVALID_ARGS;
    }
    if (!content.flag.has_value()) {
        return E_OK;
    }
    if (static_cast<uint32_t>(content.flag.value()) >= static_cast<uint32_t>(LogFlag::BUTT)) {
        LOGE("[RDBClientUtils] CheckUpdateContent invalid flag[%" PRIu32 "]",
            static_cast<uint32_t>(content.flag.value()));
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RelationalStoreClientUtils::UpdateDataLogInner(sqlite3 *db, const UpdateOption &option)
{
    if (sqlite3_get_autocommit(db) == 0) {
        return UpdateDataLogInTransaction(db, option);
    }
    auto errCode = SQLiteUtils::BeginTransaction(db, TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Update data log begin transaction failed[%d]", errCode);
        return errCode;
    }
    errCode = UpdateDataLogInTransaction(db, option);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Update data log in transaction failed[%d]", errCode);
        int ret = SQLiteUtils::RollbackTransaction(db);
        if (ret != E_OK) {
            LOGE("[RDBClientUtils] Update data log rollback transaction failed[%d]", errCode);
        }
    } else {
        errCode = SQLiteUtils::CommitTransaction(db);
        if (errCode != E_OK) {
            LOGE("[RDBClientUtils] Update data log commit transaction failed[%d]", errCode);
            return errCode;
        }
    }
    return errCode;
}

int RelationalStoreClientUtils::UpdateDataLogInTransaction(sqlite3 *db, const UpdateOption &option)
{
    auto sql = GetUpdateSQL(option);
    sqlite3_stmt *stmt = nullptr;
    auto errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Get statement failed[%d]", errCode);
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *releaseStmt = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(releaseStmt, true, ret);
        if (ret != E_OK) {
            LOGE("[RDBClientUtils] Reset statement failed[%d]", ret);
        }
    });
    errCode = BindDataLogValue(stmt, option);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Bind data log value failed[%d]", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepNext(stmt);
    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Step statement failed[%d]", errCode);
        return errCode;
    }
    LOGI("[RDBClientUtils] Update data log[%s] count[%d] success",
        DBCommon::StringMiddleMaskingWithLen(option.tableName).c_str(), sqlite3_changes(db));
    return E_OK;
}

std::string RelationalStoreClientUtils::GetUpdateSQL(const UpdateOption &option)
{
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(option.tableName) + " SET " + GetUpdateLogSQL(option);
    sql += " WHERE ";
    if (option.condition.logCondition.has_value()) {
        sql += option.condition.logCondition.value().sql;
    } else {
        sql += " data_key IN (SELECT _rowid_ FROM " + option.tableName + " WHERE ";
        sql += option.condition.dataCondition.value().sql + ")";
    }
    return sql;
}

std::string RelationalStoreClientUtils::GetUpdateLogSQL(const UpdateOption &option)
{
    std::string sql;
    if (option.content.oriDevice.has_value()) {
        sql += "ori_device = ?";
    }
    if (!option.content.flag.has_value()) {
        return sql;
    }
    if (!sql.empty()) {
        sql += ", ";
    }
    sql += "flag = flag";
    if (option.content.flag.value() == LogFlag::REMOTE) {
        sql.append("&~").append(std::to_string(static_cast<int64_t>(LogFlag::LOCAL)));
    } else if (option.content.flag.value() == LogFlag::LOCAL) {
        sql.append("|").append(std::to_string(static_cast<int64_t>(LogFlag::LOCAL)));
    }
    return sql;
}

int RelationalStoreClientUtils::BindDataLogValue(sqlite3_stmt *stmt, const UpdateOption &option)
{
    int index = 1;
    if (option.content.oriDevice.has_value()) {
        auto hashDev = DBCommon::TransferHashString(option.content.oriDevice.value());
        int errCode;
        if (!hashDev.empty()) {
            errCode = SQLiteUtils::BindBlobToStatement(stmt, index++, Bytes(hashDev.begin(), hashDev.end()));
        } else {
            errCode = SQLiteUtils::BindTextToStatement(stmt, index++, "");
        }
        if (errCode != E_OK) {
            LOGE("[RDBClientUtils] Bind ori_device to statement failed[%d]", errCode);
            return errCode;
        }
    }
    auto errCode = BindDataLogCondition(stmt, option.condition.logCondition, true, index);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Bind log condition to statement failed[%d]", errCode);
        return errCode;
    }
    errCode = BindDataLogCondition(stmt, option.condition.dataCondition, false, index);
    if (errCode != E_OK) {
        LOGE("[RDBClientUtils] Bind data condition to statement failed[%d]", errCode);
    }
    return errCode;
}

int RelationalStoreClientUtils::BindDataLogCondition(sqlite3_stmt *stmt,
    const std::optional<SelectCondition> &condition, bool isLog, int &index)
{
    if (!condition.has_value()) {
        return E_OK;
    }
    auto &cd = condition.value();
    for (const auto &arg : cd.args) {
        auto type = arg;
        if (isLog && std::holds_alternative<std::string>(type)) {
            type = DBCommon::TransferHashString(std::get<std::string>(type));
            auto str = std::get<std::string>(type);
            if (!str.empty()) {
                type = Bytes(str.begin(), str.end());
            }
        }
        auto errCode = SQLiteUtils::BindType(stmt, type, index++);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int RelationalStoreClientUtils::GetTableAndColumnName(const JsonObject &jsonValue,
    std::string &tableName, std::string &columnName)
{
    FieldValue tableValue;
    int errCode = jsonValue.GetFieldValueByFieldPath(FieldPath {"tableName"}, tableValue);
    if (errCode != E_OK) {
        LOGE("get table failed %d", errCode);
        return errCode;
    }
    FieldValue columnValue;
    errCode = jsonValue.GetFieldValueByFieldPath(FieldPath {"columnName"}, columnValue);
    if (errCode != E_OK) {
        LOGE("get column failed %d", errCode);
        return errCode;
    }
    tableName = tableValue.stringValue;
    columnName = columnValue.stringValue;
    return E_OK;
}

int RelationalStoreClientUtils::InitNewTableEntry(MonitorTableCol &table, const std::string &tableName,
    const std::string &columnName)
{
    char *tableNameCpy = strdup(tableName.c_str());
    if (tableNameCpy == nullptr) {
        LOGE("[InitNewTableEntry] Copy table name err: %d", -E_OUT_OF_MEMORY);
        return -E_OUT_OF_MEMORY;
    }

    size_t colSize = sizeof(char *) * MAX_MONITOR_COLUMN_COUNT;
    char **tableCols = static_cast<char **>(malloc(colSize));
    if (tableCols == nullptr) {
        LOGE("[InitNewTableEntry] Allocate table columns err: %d", -E_OUT_OF_MEMORY);
        free(tableNameCpy);
        return -E_OUT_OF_MEMORY;
    }
    (void)memset_s(tableCols, colSize, 0, colSize);

    tableCols[0] = strdup(columnName.c_str());
    if (tableCols[0] == nullptr) {
        LOGE("[InitNewTableEntry] Copy column name err: %d", -E_OUT_OF_MEMORY);
        free(tableNameCpy);
        free(tableCols);
        return -E_OUT_OF_MEMORY;
    }

    table.tableName = tableNameCpy;
    table.cols = tableCols;
    table.colCount = 1;
    return E_OK;
}

int RelationalStoreClientUtils::TryAddColumnToTable(MonitorTableCol &table, const std::string &columnName)
{
    if (table.cols == nullptr) {
        LOGE("[TryAddColumnToTable] Column is null");
        return -E_UNEXPECTED_DATA;
    }

    if (table.colCount >= MAX_MONITOR_COLUMN_COUNT) {
        LOGE("[TryAddColumnToTable] Column count exceed limit: %d, max: %d", table.colCount, MAX_MONITOR_COLUMN_COUNT);
        return -E_LENGTH_ERROR;
    }

    for (int j = 0; j < table.colCount; j++) {
        if (table.cols[j] != nullptr && std::string(table.cols[j]) == columnName) {
            return E_OK;
        }
    }

    table.cols[table.colCount] = strdup(columnName.c_str());
    if (table.cols[table.colCount] == nullptr) {
        LOGE("[TryAddColumnToTable] Copy column name failed.");
        return -E_OUT_OF_MEMORY;
    }

    table.colCount++;
    return E_OK;
}

int RelationalStoreClientUtils::AddColumnsToMonitor(const JsonObject &jsonValue,
    MonitorTablesConfig *monitorConfig)
{
    std::string tableName;
    std::string columnName;
    int errCode = GetTableAndColumnName(jsonValue, tableName, columnName);
    if (errCode != E_OK) {
        LOGE("[AddColumnsToMonitor] Get table and column name err: %d", errCode);
        return errCode;
    }

    // add column if table already exist
    for (int i = 0; i < monitorConfig->tableCount; i++) {
        if (monitorConfig->tables[i].tableName == nullptr) {
            continue;
        }
        if (std::string(monitorConfig->tables[i].tableName) == tableName) {
            return TryAddColumnToTable(monitorConfig->tables[i], columnName);
        }
    }

    if (monitorConfig->tableCount >= MAX_MONITOR_TABLE_COUNT) {
        LOGE("[AddColumnsToMonitor] Table count exceed limit %d", monitorConfig->tableCount);
        return -E_LENGTH_ERROR;
    }

    errCode = InitNewTableEntry(monitorConfig->tables[monitorConfig->tableCount], tableName, columnName);
    if (errCode != E_OK) {
        LOGE("[AddColumnsToMonitor] Init table err: %d", errCode);
        return errCode;
    }
    monitorConfig->tableCount++;
    return E_OK;
}

int RelationalStoreClientUtils::ReadJsonConfigFromFile(const std::string &dbPath, std::string &jsonStr)
{
    std::string configPath;
    if (!DataDonationUtils::GetSchemaPathByDbPath(dbPath, configPath)) {
        return -E_INVALID_ARGS;
    }
    std::ifstream file(configPath);
    if (!file.is_open()) {
        LOGE("Failed to open config file: %s", DBCommon::StringMiddleMasking(configPath).c_str());
        return -E_ERROR;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    jsonStr = buffer.str();
    file.close();
    if (jsonStr.empty()) {
        LOGD("Config file is empty");
        return -E_ERROR;
    }
    return E_OK;
}

int RelationalStoreClientUtils::ParseSearchConfig(const std::string &jsonStr, JsonObject &searchConfig)
{
    JsonObject object;
    int errCode = object.Parse(jsonStr.c_str());
    if (errCode != E_OK) {
        LOGE("update Parsed failed");
        return errCode;
    }
    errCode = SchemaUtils::ExtractJsonObj(object, "searchConfig", searchConfig);
    if (errCode != E_OK) {
        LOGE("Plz check searchConfig. %d", errCode);
    }
    return errCode;
}

int RelationalStoreClientUtils::ProcessMappings(const JsonObject &part, MonitorTablesConfig *monitorConfig)
{
    std::vector<JsonObject> mappings;
    int errCode = SchemaUtils::ExtractJsonObjArray(part, "mappings", mappings);
    if (errCode != E_OK) {
        LOGE("Plz check mappings. %d", errCode);
        return errCode;
    }
    for (const auto &mapping : mappings) {
        JsonObject value;
        errCode = SchemaUtils::ExtractJsonObj(mapping, "value", value);
        if (errCode != E_OK) {
            LOGD("Extract value field err: %d", errCode);
            continue;
        }
        errCode = AddColumnsToMonitor(value, monitorConfig);
        if (errCode != E_OK) {
            LOGW("[ProcessMappings] Add column to monitor err: %d", errCode);
            continue;
        }
    }
    return E_OK;
}

int RelationalStoreClientUtils::ProcessUTDMapping(const JsonObject &utdMapping, MonitorTablesConfig *monitorConfig)
{
    std::vector<JsonObject> parts;
    int errCode = SchemaUtils::ExtractJsonObjArray(utdMapping, "parts", parts);
    if (errCode != E_OK) {
        LOGE("Plz check parts. %d", errCode);
        return errCode;
    }
    for (const auto &part : parts) {
        (void)ProcessMappings(part, monitorConfig);
    }
    return E_OK;
}

int RelationalStoreClientUtils::GetMonitorConfigFromFile(MonitorTablesConfig *monitorConfig, const std::string &dbPath)
{
    if (monitorConfig == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::string jsonStr;
    int errCode = ReadJsonConfigFromFile(dbPath, jsonStr);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject searchConfig;
    errCode = ParseSearchConfig(jsonStr, searchConfig);
    if (errCode != E_OK) {
        return errCode;
    }
    std::vector<JsonObject> utdMappings;
    errCode = SchemaUtils::ExtractJsonObjArray(searchConfig, "UTDMapping", utdMappings);
    if (errCode != E_OK) {
        LOGE("Plz check UTDMapping. %d", errCode);
        return errCode;
    }
    for (const auto &utdMapping : utdMappings) {
        (void)ProcessUTDMapping(utdMapping, monitorConfig);
    }
    return E_OK;
}

MonitorTablesConfig *RelationalStoreClientUtils::BinlogSchemaGet(const char *dbPath)
{
    if (dbPath == nullptr) {
        LOGE("[BinlogSchemaGet] db path is null");
        return nullptr;
    }

    LOGD("[BinlogSchemaGet] Start get schema.");
    MonitorTablesConfig *monitorConfig = static_cast<MonitorTablesConfig*>(malloc(sizeof(MonitorTablesConfig)));
    if (monitorConfig == nullptr) {
        LOGE("BinlogSchemaGet: malloc monitorConfig failed");
        return nullptr;
    }
    (void)memset_s(monitorConfig, sizeof(MonitorTablesConfig), 0, sizeof(MonitorTablesConfig));

    monitorConfig->tables = static_cast<MonitorTableCol*>(malloc(MAX_MONITOR_TABLE_COUNT * sizeof(MonitorTableCol)));
    if (monitorConfig->tables == nullptr) {
        LOGE("BinlogSchemaGet: malloc tables failed");
        free(monitorConfig);
        return nullptr;
    }
    (void)memset_s(monitorConfig->tables, MAX_MONITOR_TABLE_COUNT * sizeof(MonitorTableCol), 0,
        MAX_MONITOR_TABLE_COUNT * sizeof(MonitorTableCol));

    int errCode = GetMonitorConfigFromFile(monitorConfig, dbPath);
    if (errCode != E_OK) {
        LOGD("GetMonitorConfigFromFile failed. err=%d", errCode);
        free(monitorConfig->tables);
        free(monitorConfig);
        return nullptr;
    }
    return monitorConfig;
}

std::string RelationalStoreClientUtils::GetInsertTrigger(const std::string &tableName,
    bool isRowid, const std::string &primaryKey)
{
    std::string insertTrigger = "CREATE TEMP TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + tableName + "_local_ON_INSERT AFTER INSERT\n";
    insertTrigger += "ON '" + tableName + "'\n";
    insertTrigger += "BEGIN\n";
    if (isRowid || primaryKey.empty()) { // LCOV_EXCL_BR_LINE
        insertTrigger += "SELECT data_change('" + tableName + "', 'rowid', NEW._rowid_, 0);\n";
    } else {
        insertTrigger += "SELECT data_change('" + tableName + "', ";
        insertTrigger += "'" + primaryKey + "', ";
        insertTrigger += "NEW." + primaryKey + ", 0);\n";
    }
    insertTrigger += "END;";
    return insertTrigger;
}

std::string RelationalStoreClientUtils::GetUpdateTrigger(const std::string &tableName,
    bool isRowid, const std::string &primaryKey)
{
    std::string updateTrigger = "CREATE TEMP TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + tableName + "_local_ON_UPDATE AFTER UPDATE\n";
    updateTrigger += "ON '" + tableName + "'\n";
    updateTrigger += "BEGIN\n";
    if (isRowid || primaryKey.empty()) { // LCOV_EXCL_BR_LINE
        updateTrigger += "SELECT data_change('" + tableName + "', 'rowid', NEW._rowid_, 1);\n";
    } else {
        updateTrigger += "SELECT data_change('" + tableName + "', ";
        updateTrigger += "'" + primaryKey + "', ";
        updateTrigger += "NEW." + primaryKey + ", 1);\n";
    }
    updateTrigger += "END;";
    return updateTrigger;
}

std::string RelationalStoreClientUtils::GetDeleteTrigger(const std::string &tableName,
    bool isRowid, const std::string &primaryKey)
{
    std::string deleteTrigger = "CREATE TEMP TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + tableName + "_local_ON_DELETE AFTER DELETE\n";
    deleteTrigger += "ON '" + tableName + "'\n";
    deleteTrigger += "BEGIN\n";
    if (isRowid || primaryKey.empty()) { // LCOV_EXCL_BR_LINE
        deleteTrigger += "SELECT data_change('" + tableName + "', 'rowid', OLD._rowid_, 2);\n";
    } else {
        deleteTrigger += "SELECT data_change('" + tableName + "', ";
        deleteTrigger += "'" + primaryKey + "', ";
        deleteTrigger += "OLD." + primaryKey + ", 2);\n";
    }
    deleteTrigger += "END;";
    return deleteTrigger;
}

void RelationalStoreClientUtils::StringToUpper(std::string &str)
{
    std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
}
} // namespace DistributedDB