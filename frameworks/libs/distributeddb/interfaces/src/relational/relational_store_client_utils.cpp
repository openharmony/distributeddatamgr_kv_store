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
#include "dfx_helper.h"
#include "log_print.h"
#include "res_finalizer.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
int RelationalStoreClientUtils::UpdateDataLog(sqlite3 *db, const DistributedDB::UpdateOption &option)
{
    auto errCode = CheckUpdateOption(db, option);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = CheckTable(db, option.tableName, true);
    if (errCode != E_OK) {
        return errCode;
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
        LOGD("[RDBClientUtils] Not found rdb schema[%d] in db", static_cast<int>(isTracker));
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

int RelationalStoreClientUtils::ArchiveSyncedData(sqlite3 *db, const std::string &tableName, uint64_t cursor)
{
    if (db == nullptr) {
        LOGE("[RDBClientUtils] Archive synced data failed, db is null");
        return -E_INVALID_ARGS;
    }
    std::string tag = std::string("archive synced data for [")
        .append(DBCommon::StringMiddleMaskingWithLen(tableName)).append("]");
    auto helper = DFXHelper::GetCostTimeHelper(tag);
    auto errCode = CheckTable(db, tableName, false);
    if (errCode != E_OK) {
        return errCode;
    }
    auto [ret, rdbSchema] = GetRDBSchema(db, true);
    if (ret != E_OK) {
        return ret;
    }
    errCode = CheckTable(db, tableName, rdbSchema, false, true);
    bool isTracker = false;
    TrackerTable table = rdbSchema.GetTrackerTable(tableName);
    table.SetTableName(tableName);
    table.SetTriggerObserver(false);
    if (errCode == -E_DISTRIBUTED_SCHEMA_NOT_FOUND) {
        errCode = E_OK;
    } else if (errCode == E_OK) {
        isTracker = true;
    } else {
        LOGE("[RDBClientUtils] Analyze tracer[%s] failed %d",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
        return errCode;
    }
    return SQLiteUtils::TransactionProcess(db, TransactType::IMMEDIATE,
        [&db, &tableName, &cursor, &table, isTracker]() {
        return ArchiveSyncedDataInner(db, tableName, table, cursor, isTracker);
    });
}

int RelationalStoreClientUtils::DeleteSyncedData(sqlite3 *db, const std::string &tableName,
    const std::vector<std::vector<Type>> &keys)
{
    if (db == nullptr) {
        LOGE("[RDBClientUtils] Delete synced data failed, db is null");
        return -E_INVALID_ARGS;
    }
    std::string tag = std::string("delete synced data for [")
        .append(DBCommon::StringMiddleMaskingWithLen(tableName)).append("]");
    auto helper = DFXHelper::GetCostTimeHelper(tag);
    auto errCode = CheckTable(db, tableName, false);
    if (errCode != E_OK) {
        return errCode;
    }
    return SQLiteUtils::TransactionProcess(db, TransactType::IMMEDIATE, [&db, &tableName, &keys]() {
        return DeleteSyncedDataInner(db, tableName, keys);
    });
}

int RelationalStoreClientUtils::CheckTable(sqlite3 *db, const std::string &tableName, bool isCheckTableMode,
    bool isTracker)
{
    bool isCreate = false;
    auto errCode = SQLiteUtils::CheckTableExists(db, DBCommon::GetMetaTableName(), isCreate);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCreate) {
        LOGE("[RDBClientUtils] Meta[%s] not found",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    auto [ret, rdbSchema] = GetRDBSchema(db, isTracker);
    if (ret != E_OK) {
        return ret;
    }
    return CheckTable(db, tableName, rdbSchema, isCheckTableMode, isTracker);
}

int RelationalStoreClientUtils::CheckTable(sqlite3 *db, const std::string &tableName,
    const RelationalSchemaObject &rdbSchema, bool isCheckTableMode, bool isTracker)
{
    bool isCreate = false;
    int errCode = SQLiteUtils::CheckTableExists(db, tableName, isCreate);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCreate) {
        LOGE("[RDBClientUtils] Table[%s] not found",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
        return -E_TABLE_NOT_FOUND;
    }
    auto distributedTable = isTracker ? rdbSchema.GetTrackerTable(tableName).GetTableName() :
        rdbSchema.GetTable(tableName).GetTableName();
    if (distributedTable.empty()) {
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    auto tableMode = rdbSchema.GetTableMode();
    if (isCheckTableMode && tableMode != DistributedTableMode::COLLABORATION) {
        LOGE("[RDBClientUtils] Table[%s] mode[%d] not collaboration",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), static_cast<int>(tableMode));
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

int RelationalStoreClientUtils::ArchiveSyncedDataInner(sqlite3 *db, const std::string &tableName,
    const TrackerTable &table, uint64_t cursor, bool isTracker)
{
    std::vector<std::pair<std::string, std::function<void()>>> executeSQL;
    executeSQL.push_back({SQLiteRelationalUtils::GetLogTriggerStatusSQL(false), nullptr});
    std::string logTable = DBCommon::GetLogTableName(tableName);
    std::string sql = "UPDATE " + logTable + " SET flag=flag|" +
        DBCommon::FlagToStr(LogInfoFlag::FLAG_ARCHIVED) +
        " WHERE " + DBCommon::IsNotSameFlagSQL(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY) + " ";
    // mark cloud data is archived, skip logic deleted data which is reserved for DropLogicDeletedData
    auto updateSQL = sql + "AND flag&0x2=0 AND flag&" + DBCommon::FlagToStr(LogInfoFlag::FLAG_LOGIC_DELETE) +
        "=0 AND cursor<=" + std::to_string(cursor);
    executeSQL.push_back({updateSQL, nullptr});
    // mark local data is archived, skip logic deleted data which is reserved for DropLogicDeletedData
    updateSQL = sql + "AND " + DBCommon::IsSameFlagSQL(LogInfoFlag::FLAG_LOCAL) + " AND " +
        DBCommon::IsNotSameFlagSQL(LogInfoFlag::FLAG_LOGIC_DELETE) + " AND " +
        DBCommon::IsSameFlagSQL(LogInfoFlag::FLAG_UPLOAD_FINISHED);
    executeSQL.push_back({updateSQL, nullptr});
    auto deleteSQL = "DELETE FROM " + logTable + " WHERE flag&" + DBCommon::FlagToStr(LogInfoFlag::FLAG_DELETE) + "!=0"
        " AND flag&" + DBCommon::FlagToStr(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY) + "=0"
        " AND flag&" + DBCommon::FlagToStr(LogInfoFlag::FLAG_LOGIC_DELETE) + "=0";
    if (isTracker) {
        deleteSQL += " AND (extend_field='{}' OR extend_field IS NULL)";
    }
    executeSQL.push_back({deleteSQL, nullptr});
    executeSQL.push_back({table.GetTempDeleteTriggerSql(false), nullptr});
    // delete archived data
    deleteSQL = "DELETE FROM " + tableName + " WHERE _rowid_ IN ("
        "SELECT " + tableName + "._rowid_ FROM " + tableName + ", " + logTable + " WHERE " + tableName + "._rowid_=" +
        logTable + ".data_key AND " + logTable + ".flag&" + DBCommon::FlagToStr(LogInfoFlag::FLAG_ARCHIVED) + "!=0)";
    executeSQL.push_back({deleteSQL, [db, &tableName]() {
        LOGI("[RDBClientUtils] Table[%s] archived[%d]",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), sqlite3_changes(db));
    }});
    if (isTracker) {
        executeSQL.push_back({table.GetDropTempTriggerSql(TriggerMode::TriggerModeEnum::DELETE), nullptr});
    }
    // mark archived data log's data key to -1
    updateSQL = "UPDATE " + logTable + " SET data_key=-1 WHERE data_key > -1 AND flag&" +
        DBCommon::FlagToStr(LogInfoFlag::FLAG_ARCHIVED) + "!=0";
    executeSQL.push_back({updateSQL, nullptr});
    executeSQL.push_back({SQLiteRelationalUtils::GetLogTriggerStatusSQL(true), nullptr});
    return SQLiteUtils::ExecuteRawSQL(db, executeSQL);
}

int RelationalStoreClientUtils::DeleteSyncedDataInner(sqlite3 *db, const std::string &tableName,
    const std::vector<std::vector<Type>> &keys)
{
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET flag=" +
        DBCommon::FlagToStr(LogInfoFlag::FLAG_DELETE) + "|" + DBCommon::FlagToStr(LogInfoFlag::FLAG_LOCAL) +
        "|"+ DBCommon::FlagToStr(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY) +
        ", timestamp=get_raw_sys_time() WHERE hash_key=? AND flag&" +
        DBCommon::FlagToStr(LogInfoFlag::FLAG_ARCHIVED) + "!=0";
    sqlite3_stmt *stmt = nullptr;
    auto errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *release = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(release, true, ret);
        if (ret != E_OK) {
            LOGW("[RDBClientUtils]DeleteSyncedData release failed[%d]", ret);
        }
    });
    int64_t change = 0;
    for (const auto &row : keys) {
        auto [ret, hash] = GetHashKey(row);
        if (ret != E_OK) {
            return ret;
        }
        ret = SQLiteUtils::BindBlobToStatement(stmt, 1, hash);
        if (ret != E_OK) {
            LOGE("[RDBClientUtils]DeleteSyncedData bind failed[%d]", ret);
            return ret;
        }
        ret = SQLiteUtils::StepNext(stmt);
        if (ret != -E_FINISHED) {
            LOGE("[RDBClientUtils]DeleteSyncedData step failed[%d]", ret);
            return ret;
        }
        change += sqlite3_changes(db);
        SQLiteUtils::ResetStatement(stmt, false, errCode);
        if (errCode != E_OK) {
            LOGE("[RDBClientUtils]DeleteSyncedData reset failed[%d]", ret);
            return errCode;
        }
    }
    LOGI("[RDBClientUtils] Table[%s] pk[%zu] delete[%d]",
         DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), keys.size(), change);
    return E_OK;
}

std::pair<int, std::vector<uint8_t>> RelationalStoreClientUtils::GetHashKey(const std::vector<Type> &keys)
{
    std::pair<int, std::vector<uint8_t>> res;
    auto &[errCode, hash] = res;
    std::vector<uint8_t> hashValue;
    for (const auto &key : keys) {
        if (key.index() != TYPE_INDEX<std::string> && key.index() != TYPE_INDEX<int64_t>) {
            LOGE("[RDBClientUtils] Not support pk type[%zu]", key.index());
            errCode = -E_NOT_SUPPORT;
            return res;
        }
        std::string keyStr;
        if (key.index() == TYPE_INDEX<std::string>) {
            keyStr = std::get<std::string>(key);
        } else {
            keyStr = std::to_string(std::get<int64_t>(key));
        }
        std::vector<uint8_t> tmpHashValue;
        std::vector<uint8_t> oriValue(keyStr.begin(), keyStr.end());
        errCode = DBCommon::CalcValueHash(oriValue, tmpHashValue);
        if (errCode != E_OK) {
            LOGE("[RDBClientUtils] Cal hash failed[%d]", errCode);
            return res;
        }
        hashValue.insert(hashValue.end(), tmpHashValue.begin(), tmpHashValue.end());
    }
    if (keys.size() == 1u) {
        hash = hashValue;
    } else if (keys.empty()) {
        auto empty = std::string("");
        hash = std::vector<uint8_t>(empty.begin(), empty.end());
    } else {
        errCode = DBCommon::CalcValueHash(hashValue, hash);
        if (errCode != E_OK) {
            LOGE("[RDBClientUtils] Cal final hash failed[%d]", errCode);
            return res;
        }
    }
    return res;
}
} // namespace DistributedDB