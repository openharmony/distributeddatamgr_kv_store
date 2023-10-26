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
#include "sqlite_single_ver_relational_storage_executor.h"

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "simple_tracker_log_table_manager.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
static constexpr const int ROW_ID_INDEX = 1;
static constexpr const int TIMESTAMP_INDEX = 2;

int SQLiteSingleVerRelationalStorageExecutor::GetQueryInfoSql(const std::string &tableName, const VBucket &vBucket,
    std::set<std::string> &pkSet, std::vector<Field> &assetFields, std::string &querySql)
{
    if (assetFields.empty() && pkSet.empty()) {
        return GetQueryLogSql(tableName, vBucket, pkSet, querySql);
    }
    std::string gid;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, vBucket, gid);
    if (errCode != E_OK) {
        LOGE("Get cloud gid fail when query log table.");
        return errCode;
    }

    if (pkSet.empty() && gid.empty()) {
        LOGE("query log table failed because of both primary key and gid are empty.");
        return -E_CLOUD_ERROR;
    }
    std::string sql = "select a.data_key, a.device, a.ori_device, a.timestamp, a.wtimestamp, a.flag, a.hash_key,"
        " a.cloud_gid";
    for (const auto &field : assetFields) {
        sql += ", b." + field.colName;
    }
    for (const auto &pk : pkSet) {
        sql += ", b." + pk;
    }
    sql += " from '" + DBCommon::GetLogTableName(tableName) + "' AS a LEFT JOIN '" + tableName + "' AS b ";
    sql += " ON (a.data_key = b." + std::string(DBConstant::SQLITE_INNER_ROWID) + ") WHERE ";
    if (!gid.empty()) {
        sql += " a.cloud_gid = ? or ";
    }
    sql += "a.hash_key = ?";
    querySql = sql;
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::GetFillDownloadAssetStatement(const std::string &tableName,
    const VBucket &vBucket, const std::vector<Field> &fields, sqlite3_stmt *&statement)
{
    std::string sql = "UPDATE " + tableName + " SET ";
    for (const auto &field: fields) {
        sql += field.colName + " = ?,";
    }
    sql.pop_back();
    sql += " WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " = (";
    sql += "SELECT data_key FROM " + DBCommon::GetLogTableName(tableName) + " where cloud_gid = ?);";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get fill asset statement failed, %d", errCode);
        return errCode;
    }
    for (size_t i = 0; i < fields.size(); ++i) {
        errCode = BindOneField(i + 1, vBucket, fields[i], stmt);
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(stmt, true, errCode);
            return errCode;
        }
    }
    statement = stmt;
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::FillCloudAssetForDownload(const TableSchema &tableSchema,
    VBucket &vBucket, bool isDownloadSuccess)
{
    std::string cloudGid;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, vBucket, cloudGid);
    if (errCode != E_OK) {
        LOGE("Miss gid when fill Asset");
        return errCode;
    }
    std::vector<Field> assetsField;
    errCode = CloudStorageUtils::GetAssetFieldsFromSchema(tableSchema, vBucket, assetsField);
    if (errCode != E_OK) {
        LOGE("No assets need to be filled.");
        return errCode;
    }
    CloudStorageUtils::ChangeAssetsOnVBucketToAsset(vBucket, assetsField);

    if (isDownloadSuccess) {
        CloudStorageUtils::FillAssetFromVBucketFinish(vBucket, CloudStorageUtils::FillAssetAfterDownload,
            CloudStorageUtils::FillAssetsAfterDownload);
    } else {
        CloudStorageUtils::PrepareToFillAssetFromVBucket(vBucket, CloudStorageUtils::FillAssetAfterDownloadFail);
    }
    sqlite3_stmt *stmt = nullptr;
    errCode = GetFillDownloadAssetStatement(tableSchema.name, vBucket, assetsField, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(stmt, assetsField.size() + 1, cloudGid);
    if (errCode != E_OK) {
        LOGE("Bind cloud gid to statement failed. %d", errCode);
        int ret = E_OK;
        SQLiteUtils::ResetStatement(stmt, true, ret);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        LOGE("Fill cloud asset failed:%d", errCode);
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::FillCloudAssetForUpload(const std::string &tableName,
    const CloudSyncBatch &data)
{
    if (data.rowid.empty() || data.timestamp.empty()) {
        return -E_INVALID_ARGS;
    }
    if (data.assets.size() != data.rowid.size() || data.assets.size() != data.timestamp.size()) {
        return -E_INVALID_ARGS;
    }
    int errCode = SetLogTriggerStatus(false);
    if (errCode != E_OK) {
        LOGE("Fail to set log trigger off, %d", errCode);
        return errCode;
    }
    sqlite3_stmt *stmt = nullptr;
    for (size_t i = 0; i < data.assets.size(); ++i) {
        if (data.assets.at(i).empty()) {
            continue;
        }
        errCode = InitFillUploadAssetStatement(tableName, data, i, stmt);
        if (errCode != E_OK) {
            break;
        }
        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            LOGE("Fill upload asset failed:%d", errCode);
            break;
        }
        errCode = E_OK;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        stmt = nullptr;
        if (errCode != E_OK) {
            break;
        }
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    int endCode = SetLogTriggerStatus(true);
    if (endCode != E_OK) {
        LOGE("Fail to set log trigger off, %d", endCode);
        return endCode;
    }
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::InitFillUploadAssetStatement(const std::string &tableName,
    const CloudSyncBatch &data, const int &index, sqlite3_stmt *&statement)
{
    VBucket vBucket = data.assets.at(index);
    CloudStorageUtils::FillAssetFromVBucketFinish(vBucket, CloudStorageUtils::FillAssetForUpload,
        CloudStorageUtils::FillAssetsForUpload);
    std::string sql = "UPDATE " + tableName + " SET ";
    for (const auto &item: vBucket) {
        sql += item.first + " = ?,";
    }
    sql.pop_back();
    sql += " WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " = ? and (select 1 from " +
        DBCommon::GetLogTableName(tableName) + " WHERE timestamp = ?);";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }
    int batchIndex = 1;
    for (const auto &item: vBucket) {
        Field field = {
            .colName = item.first, .type = static_cast<int32_t>(item.second.index())
        };
        errCode = bindCloudFieldFuncMap_[TYPE_INDEX<Assets>](batchIndex++, vBucket, field, statement);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    int64_t rowid = data.rowid[index];
    errCode = SQLiteUtils::BindInt64ToStatement(statement, vBucket.size() + ROW_ID_INDEX, rowid);
    if (errCode != E_OK) {
        return errCode;
    }
    int64_t timeStamp = data.timestamp[index];
    return SQLiteUtils::BindInt64ToStatement(statement, vBucket.size() + TIMESTAMP_INDEX, timeStamp);
}

bool SQLiteSingleVerRelationalStorageExecutor::IsGetCloudDataContinue(uint32_t curNum, uint32_t curSize,
    uint32_t maxSize)
{
    if (curNum == 0) {
        return true;
    }
#ifdef MAX_UPLOAD_COUNT
    if (curSize < maxSize && curNum < MAX_UPLOAD_COUNT) {
        return true;
    }
#else
    if (curSize < maxSize) {
        return true;
    }
#endif
    return false;
}

int SQLiteSingleVerRelationalStorageExecutor::AnalysisTrackerTable(const TrackerTable &trackerTable,
    TableInfo &tableInfo)
{
    int errCode = SQLiteUtils::AnalysisSchema(dbHandle_, trackerTable.GetTableName(), tableInfo);
    if (errCode != E_OK) {
        LOGW("analysis table schema failed. %d", errCode);
        return errCode;
    }
    tableInfo.SetTrackerTable(trackerTable);
    errCode = tableInfo.CheckTrackerTable();
    if (errCode != E_OK) {
        LOGE("check tracker table schema failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CreateTrackerTable(const TrackerTable &trackerTable, bool isUpgrade)
{
    TableInfo table;
    table.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    int errCode = AnalysisTrackerTable(trackerTable, table);
    if (errCode != E_OK) {
        return errCode;
    }
    auto tableManager = std::make_unique<SimpleTrackerLogTableManager>();
    if (!trackerTable.GetTrackerColNames().empty()) {
        // create log table
        errCode = tableManager->CreateRelationalLogTable(dbHandle_, table);
        if (errCode != E_OK) {
            return errCode;
        }
        std::string calPrimaryKeyHash = tableManager->CalcPrimaryKeyHash("a.", table, "");
        if (isUpgrade) {
            errCode = CleanExtendAndCursorForDeleteData(dbHandle_, table.GetTableName());
            if (errCode != E_OK) {
                LOGE("clean tracker log info for deleted data failed. %d", errCode);
                return errCode;
            }
        }
        errCode = GeneLogInfoForExistedData(dbHandle_, trackerTable.GetTableName(), calPrimaryKeyHash, table);
        if (errCode != E_OK) {
            LOGE("general tracker log info for existed data failed. %d", errCode);
            return errCode;
        }
    }
    errCode = tableManager->AddRelationalLogTableTrigger(dbHandle_, table, "");
    if (errCode != E_OK) {
        return errCode;
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::GetOrInitTrackerSchemaFromMeta(RelationalSchemaObject &schema)
{
    if (!schema.ToSchemaString().empty()) {
        return E_OK;
    }
    const Key schemaKey(DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY.begin(),
        DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY.end());
    Value schemaVal;
    int errCode = GetKvData(schemaKey, schemaVal); // save schema to meta_data
    if (errCode != E_OK) {
        return errCode;
    }
    std::string schemaStr;
    DBCommon::VectorToString(schemaVal, schemaStr);
    errCode = schema.ParseFromTrackerSchemaString(schemaStr);
    if (errCode != E_OK) {
        LOGE("Parse from tracker schema string err");
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records)
{
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, condition.sql, statement);
    if (errCode != E_OK) {
        LOGE("Execute sql failed when prepare stmt");
        return errCode;
    }
    size_t bindCount = static_cast<size_t>(sqlite3_bind_parameter_count(statement));
    if (bindCount > condition.bindArgs.size() || bindCount < condition.bindArgs.size()) {
        LOGE("sql bind args mismatch");
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return -E_INVALID_ARGS;
    }
    for (size_t i = 0; i < condition.bindArgs.size(); i++) {
        Type type = condition.bindArgs[i];
        errCode = SQLiteRelationalUtils::BindStatementByType(statement, i + 1, type);
        if (errCode != E_OK) {
            int ret = E_OK;
            SQLiteUtils::ResetStatement(statement, true, ret);
            return errCode;
        }
    }
    while ((errCode = SQLiteRelationalUtils::StepNext(isMemDb_, statement)) == E_OK) {
        VBucket bucket;
        errCode = SQLiteRelationalUtils::GetSelectVBucket(statement, bucket);
        if (errCode != E_OK) {
            int ret = E_OK;
            SQLiteUtils::ResetStatement(statement, true, ret);
            return errCode;
        }
        records.push_back(std::move(bucket));
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode == -E_FINISHED ? (ret == E_OK ? E_OK : ret) : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::UpgradedLogForExistedData(sqlite3 *db, TableInfo &tableInfo)
{
    if (tableInfo.GetTrackerTable().IsEmpty() || tableInfo.GetTableSyncType() == TableSyncType::DEVICE_COOPERATION) {
        return E_OK;
    }
    int64_t timeOffset = 0;
    std::string timeOffsetStr = std::to_string(timeOffset);
    std::string logTable = DBConstant::RELATIONAL_PREFIX + tableInfo.GetTableName() + "_log";
    std::string sql = "UPDATE " + logTable + " SET extend_field = " +
        tableInfo.GetTrackerTable().GetUpgradedExtendValSql();
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("Upgrade log for extend field failed.");
        return errCode;
    }
    sql = "UPDATE " + logTable + " SET cursor = (SELECT (SELECT CASE WHEN MAX(cursor) IS NULL THEN 0 ELSE"
        " MAX(cursor) END from " + logTable + ") + " + std::string(DBConstant::SQLITE_INNER_ROWID) +
        " FROM " + tableInfo.GetTableName() + " WHERE " + tableInfo.GetTableName() + "." +
        std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + logTable + ".data_key);";
    errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("Upgrade log for cursor failed.");
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CreateTempSyncTrigger(const TrackerTable &trackerTable)
{
    int errCode = E_OK;
    std::vector<std::string> dropSql = trackerTable.GetDropTempTriggerSql();
    for (const auto &sql: dropSql) {
        errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, sql);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, trackerTable.GetTempInsertTriggerSql());
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, trackerTable.GetTempUpdateTriggerSql());
    if (errCode != E_OK) {
        return errCode;
    }
    return SQLiteUtils::ExecuteRawSQL(dbHandle_, trackerTable.GetTempDeleteTriggerSql());
}

int SQLiteSingleVerRelationalStorageExecutor::GetAndResetServerObserverData(const std::string &tableName,
    ChangeProperties &changeProperties)
{
    std::string fileName;
    if (!SQLiteRelationalUtils::GetDbFileName(dbHandle_, fileName)) {
        LOGE("get db file name failed.");
        return -E_INVALID_DB;
    }
    SQLiteUtils::GetAndResetServerObserverData(fileName, tableName, changeProperties);
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::ClearAllTempSyncTrigger()
{
    sqlite3_stmt *stmt = nullptr;
    static const std::string sql = "SELECT name FROM sqlite_temp_master WHERE type = 'trigger';";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("get clear all temp trigger stmt failed. %d", errCode);
        return errCode;
    }
    int ret = E_OK;
    while ((errCode = SQLiteRelationalUtils::StepNext(isMemDb_, stmt)) == E_OK) {
        std::string str;
        (void)SQLiteUtils::GetColumnTextValue(stmt, 0, str);
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(stmt, true, ret);
            return errCode;
        }
        std::string dropSql = "DROP TRIGGER IF EXISTS '" + str + "';";
        errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, dropSql);
        if (errCode != E_OK) {
            LOGE("drop temp trigger failed. %d", errCode);
            SQLiteUtils::ResetStatement(stmt, true, ret);
            return errCode;
        }
    }
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode == -E_FINISHED ? (ret == E_OK ? E_OK : ret) : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CleanTrackerData(const std::string &tableName, int64_t cursor)
{
    std::string sql = "UPDATE " + DBConstant::RELATIONAL_PREFIX + tableName + "_log";
    sql += " SET extend_field = NULL where data_key = -1 and cursor <= ?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        LOGE("get clean tracker data stmt failed. %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(statement, 1, cursor);
    int ret = E_OK;
    if (errCode != E_OK) {
        LOGE("bind clean tracker data stmt failed. %d", errCode);
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        LOGE("clean tracker step failed:%d", errCode);
    }
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode == E_OK ? ret : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CleanExtendAndCursorForDeleteData(sqlite3 *db,
    const std::string &tableName)
{
    std::string logTable = DBConstant::RELATIONAL_PREFIX + tableName + "_log";
    std::string sql = "UPDATE " + logTable + " SET extend_field = NULL, cursor = NULL where flag&0x01=0x01;";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("update extend field and cursor failed. %d", errCode);
    }
    return errCode;
}

std::string SQLiteSingleVerRelationalStorageExecutor::GetCloudDeleteSql()
{
    std::string sql;
    if (isLogicDelete_) {
        sql += "flag = 9"; // 1001 which is logicDelete|cloudForcePush|local|delete
    } else {
        sql += "data_key = -1,  flag = 1";
    }
    sql += ", cloud_gid = '', version = '', ";
    return sql;
}

int SQLiteSingleVerRelationalStorageExecutor::RemoveDataAndLog(const std::string &tableName, int64_t dataKey)
{
    int errCode = E_OK;
    std::string removeDataSql = "DELETE FROM " + tableName + " WHERE " + DBConstant::SQLITE_INNER_ROWID + " = " +
        std::to_string(dataKey);
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, removeDataSql);
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] remove data failed %d", errCode);
        return errCode;
    }
    std::string removeLogSql = "DELETE FROM " + DBCommon::GetLogTableName(tableName) + " WHERE data_key = " +
        std::to_string(dataKey);
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, removeLogSql);
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] remove log failed %d", errCode);
    }
    return errCode;
}

int64_t SQLiteSingleVerRelationalStorageExecutor::GetLocalDataKey(size_t index,
    const DownloadData &downloadData)
{
    if (index >= downloadData.existDataKey.size()) {
        LOGW("[RDBExecutor] index out of range when get local data key"); // should not happen
        return -1; // -1 means not exist
    }
    return downloadData.existDataKey[index];
}
} // namespace DistributedDB
#endif