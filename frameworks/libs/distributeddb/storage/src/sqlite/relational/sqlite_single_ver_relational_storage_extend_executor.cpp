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

#include "cloud/asset_operation_utils.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "log_table_manager_factory.h"
#include "runtime_context.h"
#include "simple_tracker_log_table_manager.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
static constexpr const int ROW_ID_INDEX = 1;
static constexpr const char *HASH_KEY = "HASH_KEY";
static constexpr const char *FLAG_NOT_LOGIC_DELETE = "FLAG & 0x08 = 0"; // see if 3th bit of a flag is not logic delete

using PairStringVector = std::pair<std::vector<std::string>, std::vector<std::string>>;

int SQLiteSingleVerRelationalStorageExecutor::GetQueryInfoSql(const std::string &tableName, const VBucket &vBucket,
    std::set<std::string> &pkSet, std::vector<Field> &assetFields, std::string &querySql)
{
    if (assetFields.empty() && pkSet.empty()) {
        return GetQueryLogSql(tableName, vBucket, pkSet, querySql);
    }
    std::string gid;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, vBucket, gid);
    if (putDataMode_ == PutDataMode::SYNC && errCode != E_OK) {
        LOGE("Get cloud gid fail when query log table.");
        return errCode;
    }

    if (pkSet.empty() && gid.empty()) {
        LOGE("query log table failed because of both primary key and gid are empty.");
        return -E_CLOUD_ERROR;
    }
    std::string sql = "select a.data_key, a.device, a.ori_device, a.timestamp, a.wtimestamp, a.flag, a.hash_key,"
        " a.cloud_gid, a.sharing_resource, a.status, a.version";
    for (const auto &field : assetFields) {
        sql += ", b." + field.colName;
    }
    for (const auto &pk : pkSet) {
        sql += ", b." + pk;
    }
    sql += CloudStorageUtils::GetLeftJoinLogSql(tableName) + " WHERE ";
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

    Bytes hashKey;
    (void)CloudStorageUtils::GetValueFromVBucket<Bytes>(HASH_KEY, vBucket, hashKey);
    VBucket dbAssets;
    std::tie(errCode, std::ignore) = GetAssetsByGidOrHashKey(tableSchema, cloudGid, hashKey, dbAssets);
    if (errCode != E_OK && errCode != -E_NOT_FOUND && errCode != -E_CLOUD_GID_MISMATCH) {
        LOGE("get assets by gid or hashkey failed %d.", errCode);
        return errCode;
    }
    AssetOperationUtils::RecordAssetOpType assetOpType = AssetOperationUtils::CalAssetOperation(vBucket, dbAssets,
        AssetOperationUtils::CloudSyncAction::END_DOWNLOAD);

    if (isDownloadSuccess) {
        CloudStorageUtils::FillAssetFromVBucketFinish(assetOpType, vBucket, dbAssets,
            CloudStorageUtils::FillAssetAfterDownload, CloudStorageUtils::FillAssetsAfterDownload);
    } else {
        CloudStorageUtils::FillAssetFromVBucketFinish(assetOpType, vBucket, dbAssets,
            CloudStorageUtils::FillAssetAfterDownloadFail, CloudStorageUtils::FillAssetsAfterDownloadFail);
    }

    sqlite3_stmt *stmt = nullptr;
    errCode = GetFillDownloadAssetStatement(tableSchema.name, dbAssets, assetsField, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ExecuteFillDownloadAssetStatement(stmt, assetsField.size() + 1, cloudGid);
    int ret = CleanDownloadChangedAssets(vBucket, assetOpType);
    return errCode == E_OK ? ret : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::FillCloudAssetForUpload(OpType opType, const TableSchema &tableSchema,
    const CloudSyncBatch &data)
{
    int errCode = E_OK;
    if (CloudStorageUtils::ChkFillCloudAssetParam(data, errCode)) {
        return errCode;
    }
    errCode = SetLogTriggerStatus(false);
    if (errCode != E_OK) {
        LOGE("Fail to set log trigger off, %d", errCode);
        return errCode;
    }
    sqlite3_stmt *stmt = nullptr;
    for (size_t i = 0; i < data.assets.size(); ++i) {
        if (data.assets.at(i).empty()) {
            continue;
        }
        if (DBCommon::IsRecordIgnored(data.extend[i]) || DBCommon::IsRecordVersionConflict(data.extend[i])) {
            continue;
        }
        errCode = InitFillUploadAssetStatement(opType, tableSchema, data, i, stmt);
        if (errCode != E_OK) {
            if (errCode == -E_NOT_FOUND) {
                errCode = E_OK;
                continue;
            }
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

int SQLiteSingleVerRelationalStorageExecutor::FillCloudVersionForUpload(const OpType opType, const CloudSyncData &data)
{
    switch (opType) {
        case OpType::UPDATE_VERSION:
            return SQLiteSingleVerRelationalStorageExecutor::FillCloudVersionForUpload(data.tableName, data.updData);
        case OpType::INSERT_VERSION:
            return SQLiteSingleVerRelationalStorageExecutor::FillCloudVersionForUpload(data.tableName, data.insData);
        default:
            LOGE("Fill version with unknown type %d", static_cast<int>(opType));
            return -E_INVALID_ARGS;
    }
}

int SQLiteSingleVerRelationalStorageExecutor::BindUpdateVersionStatement(const VBucket &vBucket, const Bytes &hashKey,
    sqlite3_stmt *&stmt)
{
    int errCode = E_OK;
    std::string version;
    if (CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::VERSION_FIELD,
        vBucket, version) != E_OK) {
        LOGW("get version from vBucket failed.");
    }
    if (hashKey.empty()) {
        LOGE("hash key is empty when update version");
        return -E_CLOUD_ERROR;
    }
    errCode = SQLiteUtils::BindTextToStatement(stmt, 1, version);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, 2, hashKey); // 2 means the second bind args
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
        SQLiteUtils::ResetStatement(stmt, false, errCode);
    } else {
        LOGE("step version stmt failed:%d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::InitFillUploadAssetStatement(OpType opType,
    const TableSchema &tableSchema, const CloudSyncBatch &data, const int &index, sqlite3_stmt *&statement)
{
    VBucket vBucket = data.assets.at(index);
    VBucket dbAssets;
    std::string cloudGid;
    int errCode;
    (void)CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::GID_FIELD, vBucket, cloudGid);
    std::tie(errCode, std::ignore) = GetAssetsByGidOrHashKey(tableSchema, cloudGid, data.hashKey.at(index), dbAssets);
    if (errCode != E_OK && errCode != -E_CLOUD_GID_MISMATCH) {
        return errCode;
    }
    AssetOperationUtils::CloudSyncAction action = opType == OpType::SET_UPLOADING ?
        AssetOperationUtils::CloudSyncAction::START_UPLOAD : AssetOperationUtils::CloudSyncAction::END_UPLOAD;
    AssetOperationUtils::RecordAssetOpType assetOpType = AssetOperationUtils::CalAssetOperation(vBucket, dbAssets,
        action);
    if (action == AssetOperationUtils::CloudSyncAction::START_UPLOAD) {
        CloudStorageUtils::FillAssetFromVBucketFinish(assetOpType, vBucket, dbAssets,
            CloudStorageUtils::FillAssetBeforeUpload, CloudStorageUtils::FillAssetsBeforeUpload);
    } else {
        if (DBCommon::IsRecordError(data.extend.at(index))) {
            CloudStorageUtils::FillAssetFromVBucketFinish(assetOpType, vBucket, dbAssets,
                CloudStorageUtils::FillAssetForUploadFailed, CloudStorageUtils::FillAssetsForUploadFailed);
        } else {
            CloudStorageUtils::FillAssetFromVBucketFinish(assetOpType, vBucket, dbAssets,
                CloudStorageUtils::FillAssetForUpload, CloudStorageUtils::FillAssetsForUpload);
        }
    }

    errCode = GetAndBindFillUploadAssetStatement(tableSchema.name, dbAssets, statement);
    if (errCode != E_OK) {
        return errCode;
    }
    int64_t rowid = data.rowid[index];
    return SQLiteUtils::BindInt64ToStatement(statement, dbAssets.size() + ROW_ID_INDEX, rowid);
}

int SQLiteSingleVerRelationalStorageExecutor::AnalysisTrackerTable(const TrackerTable &trackerTable,
    TableInfo &tableInfo)
{
    return SQLiteRelationalUtils::AnalysisTrackerTable(dbHandle_, trackerTable, tableInfo);
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
    if (!trackerTable.GetTrackerColNames().empty() && !isUpgrade) {
        return CheckInventoryData(DBCommon::GetLogTableName(table.GetTableName()));
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
    if (schemaVal.empty()) {
        return -E_NOT_FOUND;
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
    while ((errCode = SQLiteUtils::StepNext(statement, isMemDb_)) == E_OK) {
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

int SQLiteSingleVerRelationalStorageExecutor::GetClearWaterMarkTables(
    const std::vector<TableReferenceProperty> &tableReferenceProperty, const RelationalSchemaObject &schema,
    std::set<std::string> &clearWaterMarkTables)
{
    std::set<std::string> changeTables = schema.CompareReferenceProperty(tableReferenceProperty);
    for (const auto &table : changeTables) {
        std::string logTableName = DBCommon::GetLogTableName(table);
        bool isExists = false;
        int errCode = SQLiteUtils::CheckTableExists(dbHandle_, logTableName, isExists);
        if (errCode != E_OK) {
            LOGE("[GetClearWaterMarkTables] check table exists failed, errCode = %d", errCode);
            return errCode;
        }
        if (!isExists) { // table maybe dropped after set reference
            LOGI("[GetClearWaterMarkTables] log table not exists, skip this table.");
            continue;
        }

        bool isEmpty = true;
        errCode = SQLiteUtils::CheckTableEmpty(dbHandle_, logTableName, isEmpty);
        if (errCode != E_OK) {
            LOGE("[GetClearWaterMarkTables] check table empty failed, errCode = %d", errCode);
            clearWaterMarkTables.clear();
            return errCode;
        }
        if (!isEmpty) {
            clearWaterMarkTables.insert(table);
        }
    }
    LOGI("[GetClearWaterMarkTables] clearWaterMarkTables size = %zu", clearWaterMarkTables.size());
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::UpgradedLogForExistedData(TableInfo &tableInfo, bool schemaChanged)
{
    if (tableInfo.GetTableSyncType() == TableSyncType::DEVICE_COOPERATION) {
        return E_OK;
    }
    std::string logTable = DBCommon::GetLogTableName(tableInfo.GetTableName());
    if (schemaChanged) {
        std::string markAsInconsistent = "UPDATE " + logTable + " SET flag=" +
            "(CASE WHEN (cloud_gid='' and data_key=-1 and flag&0x02=0x02) then flag else flag|0x20 END)";
        int ret = SQLiteUtils::ExecuteRawSQL(dbHandle_, markAsInconsistent);
        if (ret != E_OK) {
            LOGE("Mark upgrade log info as inconsistent failed:%d", ret);
            return ret;
        }
    }
    if (tableInfo.GetTrackerTable().IsEmpty()) {
        return E_OK;
    }
    LOGI("Upgrade tracker table log, schemaChanged:%d.", schemaChanged);
    int64_t timeOffset = 0;
    std::string timeOffsetStr = std::to_string(timeOffset);
    std::string sql = "UPDATE " + logTable + " SET extend_field = " +
        tableInfo.GetTrackerTable().GetUpgradedExtendValSql();
    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, sql);
    if (errCode != E_OK) {
        LOGE("Upgrade log for extend field failed.");
        return errCode;
    }
    sql = "UPDATE " + logTable + " SET cursor = (SELECT (SELECT CASE WHEN MAX(cursor) IS NULL THEN 0 ELSE"
        " MAX(cursor) END from " + logTable + ") + " + std::string(DBConstant::SQLITE_INNER_ROWID) +
        " FROM " + tableInfo.GetTableName() + " WHERE " + tableInfo.GetTableName() + "." +
        std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + logTable + ".data_key);";
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, sql);
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
            LOGE("[RDBExecutor] execute drop sql failed %d", errCode);
            return errCode;
        }
    }
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, trackerTable.GetTempInsertTriggerSql());
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] create temp insert trigger failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, trackerTable.GetTempUpdateTriggerSql());
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] create temp update trigger failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, trackerTable.GetTempDeleteTriggerSql());
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] create temp delete trigger failed %d", errCode);
    }
    return errCode;
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
    while ((errCode = SQLiteUtils::StepNext(stmt, isMemDb_)) == E_OK) {
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

int SQLiteSingleVerRelationalStorageExecutor::CreateSharedTable(const TableSchema &tableSchema)
{
    std::map<int32_t, std::string> cloudFieldTypeMap;
    cloudFieldTypeMap[TYPE_INDEX<Nil>] = "NULL";
    cloudFieldTypeMap[TYPE_INDEX<int64_t>] = "INT";
    cloudFieldTypeMap[TYPE_INDEX<double>] = "REAL";
    cloudFieldTypeMap[TYPE_INDEX<std::string>] = "TEXT";
    cloudFieldTypeMap[TYPE_INDEX<bool>] = "BOOLEAN";
    cloudFieldTypeMap[TYPE_INDEX<Bytes>] = "BLOB";
    cloudFieldTypeMap[TYPE_INDEX<Asset>] = "ASSET";
    cloudFieldTypeMap[TYPE_INDEX<Assets>] = "ASSETS";

    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + tableSchema.sharedTableName + "(";
    std::string primaryKey = ", PRIMARY KEY (";
    createTableSql += CloudDbConstant::CLOUD_OWNER;
    createTableSql += " TEXT, ";
    createTableSql += CloudDbConstant::CLOUD_PRIVILEGE;
    createTableSql += " TEXT";
    primaryKey += CloudDbConstant::CLOUD_OWNER;
    bool hasPrimaryKey = false;
    for (const auto &field : tableSchema.fields) {
        createTableSql += ", " + field.colName + " ";
        createTableSql += cloudFieldTypeMap[field.type];
        createTableSql += field.nullable ? "" : " NOT NULL";
        if (field.primary) {
            primaryKey += ", " + field.colName;
            hasPrimaryKey = true;
        }
    }
    if (hasPrimaryKey) {
        createTableSql += primaryKey + ")";
    }
    createTableSql += ");";
    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, createTableSql);
    if (errCode != E_OK) {
        LOGE("Create shared table failed, %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteTable(const std::vector<std::string> &tableNames)
{
    for (const auto &tableName : tableNames) {
        std::string deleteTableSql = "DROP TABLE IF EXISTS " + tableName + ";";
        int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, deleteTableSql);
        if (errCode != E_OK) {
            LOGE("Delete table failed, %d", errCode);
            return errCode;
        }
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::UpdateSharedTable(
    const std::map<std::string, std::vector<Field>> &updateTableNames)
{
    int errCode = E_OK;
    std::map<int32_t, std::string> fieldTypeMap;
    fieldTypeMap[TYPE_INDEX<Nil>] = "NULL";
    fieldTypeMap[TYPE_INDEX<int64_t>] = "INT";
    fieldTypeMap[TYPE_INDEX<double>] = "REAL";
    fieldTypeMap[TYPE_INDEX<std::string>] = "TEXT";
    fieldTypeMap[TYPE_INDEX<bool>] = "BOOLEAN";
    fieldTypeMap[TYPE_INDEX<Bytes>] = "BLOB";
    fieldTypeMap[TYPE_INDEX<Asset>] = "ASSET";
    fieldTypeMap[TYPE_INDEX<Assets>] = "ASSETS";
    for (const auto &table : updateTableNames) {
        if (table.second.empty()) {
            continue;
        }
        std::string addColumnSql = "";
        for (const auto &field : table.second) {
            addColumnSql += "ALTER TABLE " + table.first + " ADD ";
            addColumnSql += field.colName + " ";
            addColumnSql += fieldTypeMap[field.type];
            addColumnSql += field.primary ? " PRIMARY KEY" : "";
            addColumnSql += field.nullable ? ";" : " NOT NULL;";
        }
        errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, addColumnSql);
        if (errCode != E_OK) {
            LOGE("Shared table add column failed, %d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::AlterTableName(const std::map<std::string, std::string> &tableNames)
{
    for (const auto &tableName : tableNames) {
        std::string alterTableSql = "ALTER TABLE " + tableName.first + " RENAME TO " + tableName.second + ";";
        int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, alterTableSql);
        if (errCode != E_OK) {
            LOGE("Alter table name failed, %d", errCode);
            return errCode;
        }
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::AppendUpdateLogRecordWhereSqlCondition(const TableSchema &tableSchema,
    const VBucket &vBucket, std::string &sql)
{
    std::string gidStr;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, vBucket, gidStr);
    if (errCode != E_OK) {
        LOGE("Get gid from cloud data fail when construct update log sql, errCode = %d", errCode);
        return errCode;
    }

    sql += " WHERE ";
    if (!gidStr.empty()) {
        sql += "cloud_gid = '" + gidStr + "'";
    }
    std::map<std::string, Field> pkMap = CloudStorageUtils::GetCloudPrimaryKeyFieldMap(tableSchema);
    if (!pkMap.empty()) {
        if (!gidStr.empty()) {
            sql += " OR ";
        }
        sql += "(hash_key = ?);";
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::DoCleanShareTableDataAndLog(const std::vector<std::string> &tableNameList)
{
    int ret = E_OK;
    int errCode = E_OK;
    for (const auto &tableName: tableNameList) {
        std::string delDataSql = "DELETE FROM '" + tableName + "';";
        sqlite3_stmt *statement = nullptr;
        errCode = SQLiteUtils::GetStatement(dbHandle_, delDataSql, statement);
        if (errCode != E_OK) {
            LOGE("get clean shared data stmt failed. %d", errCode);
            return errCode;
        }
        errCode = SQLiteUtils::StepWithRetry(statement);
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        } else {
            LOGE("clean shared data failed:%d", errCode);
            break;
        }
        statement = nullptr;
        std::string delLogSql = "DELETE FROM '" + DBConstant::RELATIONAL_PREFIX + tableName + "_log';";
        errCode = SQLiteUtils::GetStatement(dbHandle_, delLogSql, statement);
        if (errCode != E_OK) {
            LOGE("get clean shared log stmt failed. %d", errCode);
            return errCode;
        }
        errCode = SQLiteUtils::StepWithRetry(statement);
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        } else {
            LOGE("clean shared log failed:%d", errCode);
            break;
        }
    }
    return errCode == E_OK ? ret : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetReferenceGid(const std::string &tableName,
    const CloudSyncBatch &syncBatch, const std::map<std::string, std::vector<TableReferenceProperty>> &tableReference,
    std::map<int64_t, Entries> &referenceGid)
{
    int errCode = E_OK;
    for (const auto &[targetTable, targetReference] : tableReference) {
        errCode = GetReferenceGidInner(tableName, targetTable, syncBatch, targetReference, referenceGid);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] get reference gid inner failed %d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetReferenceGidInner(const std::string &sourceTable,
    const std::string &targetTable, const CloudSyncBatch &syncBatch,
    const std::vector<TableReferenceProperty> &targetTableReference, std::map<int64_t, Entries> &referenceGid)
{
    auto [sourceFields, targetFields] = SplitReferenceByField(targetTableReference);
    if (sourceFields.empty()) {
        LOGD("[RDBExecutor] source field is empty");
        return E_OK;
    }
    if (sourceFields.size() != targetFields.size()) {
        LOGE("[RDBExecutor] reference field size not equal.");
        return -E_INTERNAL_ERROR;
    }
    std::string sql = GetReferenceGidSql(sourceTable, targetTable, sourceFields, targetFields);
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] get ref gid data stmt failed. %d", errCode);
        return errCode;
    }
    errCode = GetReferenceGidByStmt(statement, syncBatch, targetTable, referenceGid);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode == E_OK ? ret : errCode;
}

std::string SQLiteSingleVerRelationalStorageExecutor::GetReferenceGidSql(const std::string &sourceTable,
    const std::string &targetTable, const std::vector<std::string> &sourceFields,
    const std::vector<std::string> &targetFields)
{
    // sql like this:
    // SELECT naturalbase_rdb_aux_parent_log.cloud_gid FROM naturalbase_rdb_aux_parent_log,
    //   (SELECT parent._rowid_ AS rowid_b FROM parent,
    //     (SELECT child._rowid_, name FROM child, naturalbase_rdb_aux_child_log
    //     WHERE child._rowid_ = ? AND naturalbase_rdb_aux_child_log.timestamp = ? ) source_a
    //   WHERE parent.name = source_a.name ) temp_table
    // WHERE naturalbase_rdb_aux_parent_log.data_key = temp_table.rowid_b
    std::string logTargetTable = DBCommon::GetLogTableName(targetTable);
    std::string logSourceTable = DBCommon::GetLogTableName(sourceTable);
    std::string sql;
    sql += "SELECT " + logTargetTable + ".cloud_gid" + " FROM " + logTargetTable + ", ";
    sql += "(";
    sql += "SELECT " + targetTable + "._rowid_ AS rowid_b FROM " + targetTable
           + ", ";
    sql += "(SELECT " + sourceTable + "._rowid_,";
    std::set<std::string> sourceFieldSet;
    for (const auto &item : sourceFields) {
        sourceFieldSet.insert(item);
    }
    for (const auto &sourceField : sourceFieldSet) {
        sql += sourceField + ",";
    }
    sql.pop_back();
    sql += " FROM " + sourceTable + ", " + logSourceTable;
    sql +=" WHERE " + sourceTable + "._rowid_ = ? AND " + logSourceTable + ".timestamp = ? ";
    sql += " AND " + logSourceTable + ".flag&0x08=0x00) source_a";
    sql += " WHERE ";
    for (size_t i = 0u; i < sourceFields.size(); ++i) {
        if (i != 0u) {
            sql += " AND ";
        }
        sql += targetTable + "." + targetFields[i] + " = source_a." + sourceFields[i];
    }
    sql += ") temp_table ";
    sql += "WHERE " + logTargetTable + ".data_key = temp_table.rowid_b";
    sql += " AND " + logTargetTable + ".flag&0x08=0x00";
    return sql;
}

int SQLiteSingleVerRelationalStorageExecutor::GetReferenceGidByStmt(sqlite3_stmt *statement,
    const CloudSyncBatch &syncBatch, const std::string &targetTable, std::map<int64_t, Entries> &referenceGid)
{
    int errCode = E_OK;
    if (syncBatch.rowid.size() != syncBatch.timestamp.size()) {
        LOGE("[RDBExecutor] rowid size[%zu] not equal to timestamp size[%zu]", syncBatch.rowid.size(),
            syncBatch.timestamp.size());
        return -E_INVALID_ARGS;
    }
    int matchCount = 0;
    for (size_t i = 0u; i < syncBatch.rowid.size(); i++) {
        errCode = SQLiteUtils::BindInt64ToStatement(statement, 1, syncBatch.rowid[i]); // 1 is rowid index
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] bind rowid to stmt failed. %d", errCode);
            break;
        }
        errCode = SQLiteUtils::BindInt64ToStatement(statement, 2, syncBatch.timestamp[i]); // 2 is timestamp index
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] bind timestamp to stmt failed. %d", errCode);
            break;
        }
        while ((errCode = SQLiteUtils::StepNext(statement, isMemDb_)) == E_OK) {
            std::string gid;
            (void)SQLiteUtils::GetColumnTextValue(statement, 0, gid);
            if (gid.empty()) {
                LOGE("[RDBExecutor] reference data don't contain gid.");
                errCode = -E_CLOUD_ERROR;
                break;
            }
            referenceGid[syncBatch.rowid[i]][targetTable] = gid;
            matchCount++;
        }
        if (errCode == -E_FINISHED) {
            errCode = E_OK;
        }
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] step stmt failed. %d", errCode);
            break;
        }
        SQLiteUtils::ResetStatement(statement, false, errCode);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] reset stmt failed. %d", errCode);
            break;
        }
    }
    if (matchCount != 0) {
        LOGD("[RDBExecutor] get reference gid match %d", matchCount);
    }
    return errCode;
}

PairStringVector SQLiteSingleVerRelationalStorageExecutor::SplitReferenceByField(
    const std::vector<TableReferenceProperty> &targetTableReference)
{
    PairStringVector sourceTargetFiled;
    for (const auto &reference : targetTableReference) {
        for (const auto &column : reference.columns) {
            sourceTargetFiled.first.push_back(column.first);
            sourceTargetFiled.second.push_back(column.second);
        }
    }
    return sourceTargetFiled;
}

int SQLiteSingleVerRelationalStorageExecutor::BindStmtWithCloudGid(const CloudSyncData &cloudDataResult,
    bool ignoreEmptyGid, sqlite3_stmt *&stmt)
{
    int fillGidCount = 0;
    int errCode = E_OK;
    for (size_t i = 0; i < cloudDataResult.insData.extend.size(); ++i) {
        auto gidEntry = cloudDataResult.insData.extend[i].find(CloudDbConstant::GID_FIELD);
        if (gidEntry == cloudDataResult.insData.extend[i].end()) {
            if (ignoreEmptyGid) {
                continue;
            }
            errCode = -E_INVALID_ARGS;
            LOGE("[RDBExecutor] Extend not contain gid");
            break;
        }
        bool containError = DBCommon::IsRecordError(cloudDataResult.insData.extend[i]);
        if (ignoreEmptyGid && containError) {
            continue;
        }
        std::string val;
        if (CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::GID_FIELD,
            cloudDataResult.insData.extend[i], val) != E_OK) {
            errCode = -E_INVALID_DATA;
            LOGE("[RDBExecutor] Can't get string gid from extend");
            break;
        }
        if (val.empty()) {
            errCode = -E_CLOUD_ERROR;
            LOGE("[RDBExecutor] Get empty gid from extend");
            break;
        }
        errCode = BindStmtWithCloudGidInner(val, cloudDataResult.insData.rowid[i], stmt, fillGidCount);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] Bind stmt error %d", errCode);
            break;
        }
    }
    LOGD("[RDBExecutor] Fill gid count %d", fillGidCount);
    return errCode;
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

int SQLiteSingleVerRelationalStorageExecutor::CheckIfExistUserTable(const std::string &tableName)
{
    std::string sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = ?";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] Prepare the sql statement error:%d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(statement, 1, tableName);
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] Bind table name failed:%d", errCode);
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return errCode;
    }
    if (SQLiteUtils::StepWithRetry(statement) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("[RDBExecutor] local exists user table which shared table name is same as");
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return -E_INVALID_ARGS;
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return E_OK;
}

std::string SQLiteSingleVerRelationalStorageExecutor::GetCloudDeleteSql(const std::string &logTable)
{
    std::string sql;
    if (isLogicDelete_) {
        // 1001 which is logicDelete|cloudForcePush|local|delete
        sql += "flag = flag&" + std::string(CONSISTENT_FLAG) + "|0x09, cursor = (SELECT case when "
            "(MAX(cursor) is null) then 1 else MAX(cursor) + 1 END FROM " + logTable + ")";
    } else {
        sql += "data_key = -1,  flag = flag&" + std::string(CONSISTENT_FLAG) + "|0x01";
    }
    sql += ", cloud_gid = '', version = ''";
    if (!isLogicDelete_) {
        sql += ", sharing_resource = ''";
    }
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

int SQLiteSingleVerRelationalStorageExecutor::BindStmtWithCloudGidInner(const std::string &gid, int64_t rowid,
    sqlite3_stmt *&stmt, int &fillGidCount)
{
    int errCode = SQLiteUtils::BindTextToStatement(stmt, 1, gid); // 1 means the gid index
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, 2, rowid); // 2 means rowid
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
        fillGidCount++;
        SQLiteUtils::ResetStatement(stmt, false, errCode);
    } else {
        LOGE("[RDBExecutor] Update cloud log failed:%d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::RenewTableTrigger(DistributedTableMode mode,
    const TableInfo &tableInfo, TableSyncType syncType)
{
    auto tableManager = LogTableManagerFactory::GetTableManager(mode, syncType);
    return tableManager->AddRelationalLogTableTrigger(dbHandle_, tableInfo, "");
}

int SQLiteSingleVerRelationalStorageExecutor::DoCleanAssetId(const std::string &tableName,
    const RelationalSchemaObject &localSchema)
{
    std::vector<int64_t> dataKeys;
    std::string logTableName = DBCommon::GetLogTableName(tableName);
    int errCode = GetCleanCloudDataKeys(logTableName, dataKeys, false);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] Failed to get clean cloud data keys, %d.", errCode);
        return errCode;
    }
    std::vector<FieldInfo> fieldInfos = localSchema.GetTable(tableName).GetFieldInfos();
    errCode = CleanAssetId(tableName, fieldInfos, dataKeys);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] failed to clean asset id when clean cloud data, %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CleanAssetId(const std::string &tableName,
    const std::vector<FieldInfo> &fieldInfos, const std::vector<int64_t> &dataKeys)
{
    int errCode = E_OK;
    for (const auto &fieldInfo : fieldInfos) {
        if (fieldInfo.IsAssetType()) {
            Assets assets;
            errCode = GetAssetOnTable(tableName, fieldInfo.GetFieldName(), dataKeys, assets);
            if (errCode != E_OK) {
                LOGE("[Storage Executor] failed to get cloud asset on table, %d.", errCode);
                return errCode;
            }
            errCode = UpdateAssetIdOnUserTable(tableName, fieldInfo.GetFieldName(), dataKeys, assets);
            if (errCode != E_OK) {
                LOGE("[Storage Executor] failed to save clean asset id on table, %d.", errCode);
                return errCode;
            }
        } else if (fieldInfo.IsAssetsType()) {
            errCode = GetAssetsAndUpdateAssetsId(tableName, fieldInfo.GetFieldName(), dataKeys);
            if (errCode != E_OK) {
                LOGE("[Storage Executor] failed to get cloud assets on table, %d.", errCode);
                return errCode;
            }
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::UpdateAssetIdOnUserTable(const std::string &tableName,
    const std::string &fieldName, const std::vector<int64_t> &dataKeys, std::vector<Asset> &assets)
{
    if (assets.empty()) {
        return E_OK;
    }
    int errCode = E_OK;
    int ret = E_OK;
    sqlite3_stmt *stmt = nullptr;
    size_t index = 0;
    for (const auto &rowId : dataKeys) {
        if (rowId == -1) { // -1 means data is deleted
            continue;
        }
        if (assets[index].name.empty()) {
            index++;
            continue;
        }
        std::string cleanAssetIdSql = "UPDATE " + tableName  + " SET " + fieldName + " = ? WHERE " +
            std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + std::to_string(rowId) + ";";
        errCode = SQLiteUtils::GetStatement(dbHandle_, cleanAssetIdSql, stmt);
        if (errCode != E_OK) {
            LOGE("Get statement failed, %d", errCode);
            return errCode;
        }
        assets[index].assetId = "";
        assets[index].status &= ~AssetStatus::UPLOADING;
        errCode = BindAssetToBlobStatement(assets[index], 1, stmt); // 1 means sqlite statement index
        index++;
        if (errCode != E_OK) {
            LOGE("Bind asset to blob statement failed, %d", errCode);
            goto END;
        }
        errCode = SQLiteUtils::StepWithRetry(stmt);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        } else {
            LOGE("Step statement failed, %d", errCode);
            goto END;
        }
        SQLiteUtils::ResetStatement(stmt, true, ret);
    }
    return errCode != E_OK ? errCode : ret;
END:
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAssetsAndUpdateAssetsId(const std::string &tableName,
    const std::string &fieldName, const std::vector<int64_t> &dataKeys)
{
    int errCode = E_OK;
    int ret = E_OK;
    sqlite3_stmt *selectStmt = nullptr;
    for (const auto &rowId : dataKeys) {
        std::string queryAssetsSql = "SELECT " + fieldName + " FROM '" + tableName +
            "' WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + std::to_string(rowId) + ";";
        errCode = SQLiteUtils::GetStatement(dbHandle_, queryAssetsSql, selectStmt);
        if (errCode != E_OK) {
            LOGE("Get select assets statement failed, %d", errCode);
            goto END;
        }
        Assets assets;
        errCode = GetAssetsByRowId(selectStmt, assets);
        if (errCode != E_OK) {
            LOGE("Get assets by rowId failed, %d", errCode);
            goto END;
        }
        SQLiteUtils::ResetStatement(selectStmt, true, ret);
        if (assets.empty()) {
            continue;
        }
        for (auto &asset : assets) {
            asset.assetId = "";
            asset.status &= ~AssetStatus::UPLOADING;
        }
        std::vector<uint8_t> assetsValue;
        errCode = RuntimeContext::GetInstance()->AssetsToBlob(assets, assetsValue);
        if (errCode != E_OK) {
            LOGE("[CleanAssetsIdOnUserTable] failed to transfer assets to blob, %d", errCode);
            return errCode;
        }
        errCode = CleanAssetsIdOnUserTable(tableName, fieldName, rowId, assetsValue);
        if (errCode != E_OK) {
            LOGE("[CleanAssetsIdOnUserTable] clean assets id on user table failed, %d", errCode);
            return errCode;
        }
    }
    return errCode != E_OK ? errCode : ret;
END:
    SQLiteUtils::ResetStatement(selectStmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::CleanAssetsIdOnUserTable(const std::string &tableName,
    const std::string &fieldName, const int64_t rowId, const std::vector<uint8_t> assetsValue)
{
    std::string cleanAssetIdSql = "UPDATE " + tableName  + " SET " + fieldName + " = ? WHERE " +
        std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + std::to_string(rowId) + ";";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, cleanAssetIdSql, stmt);
    if (errCode != E_OK) {
        LOGE("Get statement failed, %d", errCode);
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, assetsValue, false);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

std::pair<int, uint32_t> SQLiteSingleVerRelationalStorageExecutor::GetAssetsByGidOrHashKey(
    const TableSchema &tableSchema, const std::string &gid, const Bytes &hashKey, VBucket &assets)
{
    std::pair<int, uint32_t> res = { E_OK, static_cast<uint32_t>(LockStatus::UNLOCK) };
    auto &[errCode, status] = res;
    std::vector<Field> assetFields;
    std::string sql = "SELECT";
    for (const auto &field: tableSchema.fields) {
        if (field.type == TYPE_INDEX<Asset> || field.type == TYPE_INDEX<Assets>) {
            assetFields.emplace_back(field);
            sql += " b." + field.colName + ",";
        }
    }
    if (assetFields.empty()) {
        return { -E_NOT_FOUND, status };
    }
    sql += "a.cloud_gid, a.status ";
    sql += CloudStorageUtils::GetLeftJoinLogSql(tableSchema.name) + " WHERE (a." + FLAG_NOT_LOGIC_DELETE + ") AND (" +
        (gid.empty() ? "a.hash_key = ?);" : " a.cloud_gid = ? OR  a.hash_key = ?);");
    sqlite3_stmt *stmt = nullptr;
    errCode = InitGetAssetStmt(sql, gid, hashKey, stmt);
    if (errCode != E_OK) {
        return res;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    int index = 0;
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        for (const auto &field: assetFields) {
            Type cloudValue;
            errCode = SQLiteRelationalUtils::GetCloudValueByType(stmt, field.type, index++, cloudValue);
            if (errCode != E_OK) {
                break;
            }
            errCode = PutVBucketByType(assets, field, cloudValue);
            if (errCode != E_OK) {
                break;
            }
        }
        std::string curGid;
        errCode = SQLiteUtils::GetColumnTextValue(stmt, index++, curGid);
        if (errCode == E_OK && CloudStorageUtils::IsCloudGidMismatch(gid, curGid)) {
            // Gid is different, there may be duplicate primary keys in the cloud
            errCode = -E_CLOUD_GID_MISMATCH;
        }
        status = sqlite3_column_int(stmt, index++);
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
    } else {
        LOGE("step get asset stmt failed. %d", errCode);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return res;
}

int SQLiteSingleVerRelationalStorageExecutor::InitGetAssetStmt(const std::string &sql, const std::string &gid,
    const Bytes &hashKey, sqlite3_stmt *&stmt)
{
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get asset statement failed, %d", errCode);
        return errCode;
    }
    int index = 1;
    if (!gid.empty()) {
        errCode = SQLiteUtils::BindTextToStatement(stmt, index++, gid);
        if (errCode != E_OK) {
            LOGE("bind gid failed. %d", errCode);
            SQLiteUtils::ResetStatement(stmt, true, errCode);
            return errCode;
        }
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, index, hashKey);
    if (errCode != E_OK) {
        LOGE("bind hash failed. %d", errCode);
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::FillHandleWithOpType(const OpType opType, const CloudSyncData &data,
    bool fillAsset, bool ignoreEmptyGid, const TableSchema &tableSchema)
{
    int errCode = E_OK;
    switch (opType) {
        case OpType::UPDATE_VERSION: // fallthrough
        case OpType::INSERT_VERSION: {
            errCode = FillCloudVersionForUpload(opType, data);
            break;
        }
        case OpType::SET_UPLOADING: {
            errCode = FillCloudAssetForUpload(opType, tableSchema, data.insData);
            if (errCode != E_OK) {
                LOGE("Failed to set uploading for ins data, %d.", errCode);
                return errCode;
            }
            errCode = FillCloudAssetForUpload(opType, tableSchema, data.updData);
            break;
        }
        case OpType::INSERT: {
            errCode = UpdateCloudLogGid(data, ignoreEmptyGid);
            if (errCode != E_OK) {
                LOGE("Failed to fill cloud log gid, %d.", errCode);
                return errCode;
            }
            if (fillAsset) {
                errCode = FillCloudAssetForUpload(opType, tableSchema, data.insData);
                if (errCode != E_OK) {
                    LOGE("Failed to fill asset for ins, %d.", errCode);
                    return errCode;
                }
            }
            errCode = FillCloudVersionForUpload(OpType::INSERT_VERSION, data);
            break;
        }
        case OpType::UPDATE: {
            if (fillAsset && !data.updData.assets.empty()) {
                errCode = FillCloudAssetForUpload(opType, tableSchema, data.updData);
                if (errCode != E_OK) {
                    LOGE("Failed to fill asset for upd, %d.", errCode);
                    return errCode;
                }
            }
            errCode = FillCloudVersionForUpload(OpType::UPDATE_VERSION, data);
            break;
        }
        default:
            break;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAssetsByRowId(sqlite3_stmt *&selectStmt, Assets &assets)
{
    int errCode = SQLiteUtils::StepWithRetry(selectStmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(selectStmt, 0, blobValue);
        if (errCode != E_OK) {
            LOGE("Get column blob value failed. %d", errCode);
            return errCode;
        }
        errCode = RuntimeContext::GetInstance()->BlobToAssets(blobValue, assets);
        if (errCode != E_OK) {
            LOGE("Transfer blob to assets failed. %d", errCode);
        }
        return errCode;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    } else {
        LOGE("Step select statement failed. %d", errCode);
        return errCode;
    }
}

void SQLiteSingleVerRelationalStorageExecutor::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    assetLoader_ = loader;
}

int SQLiteSingleVerRelationalStorageExecutor::ExecuteFillDownloadAssetStatement(sqlite3_stmt *&stmt,
    int beginIndex, const std::string &cloudGid)
{
    int errCode = SQLiteUtils::BindTextToStatement(stmt, beginIndex, cloudGid);
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

int SQLiteSingleVerRelationalStorageExecutor::CleanDownloadChangedAssets(
    const VBucket &vBucket, const AssetOperationUtils::RecordAssetOpType &assetOpType)
{
    if (assetLoader_ == nullptr) {
        LOGE("assetLoader may be not set.");
        return -E_NOT_SET;
    }
    std::vector<Asset> toDeleteAssets;
    CloudStorageUtils::GetToBeRemoveAssets(vBucket, assetOpType, toDeleteAssets);
    if (toDeleteAssets.empty()) {
        return E_OK;
    }
    int ret = assetLoader_->RemoveLocalAssets(toDeleteAssets);
    if (ret != OK) {
        LOGE("remove local assets failed. %d", ret);
        return -E_REMOVE_ASSETS_FAILED;
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAndBindFillUploadAssetStatement(const std::string &tableName,
    const VBucket &assets, sqlite3_stmt *&statement)
{
    std::string sql = "UPDATE '" + tableName + "' SET ";
    for (const auto &item: assets) {
        sql += item.first + " = ?,";
    }
    sql.pop_back();
    sql += " WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " = ?;";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }
    int bindIndex = 1;
    for (const auto &item: assets) {
        Field field = {
            .colName = item.first, .type = static_cast<int32_t>(item.second.index())
        };
        errCode = bindCloudFieldFuncMap_[TYPE_INDEX<Assets>](bindIndex++, assets, field, statement);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::OnlyUpdateAssetId(const std::string &tableName,
    const TableSchema &tableSchema, const VBucket &vBucket, int64_t dataKey, OpType opType)
{
    if (opType != OpType::ONLY_UPDATE_GID && opType != OpType::NOT_HANDLE &&
        opType != OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO) {
        return E_OK;
    }
    if (CloudStorageUtils::IsSharedTable(tableSchema)) {
        // this is shared table, not need to update asset id.
        return E_OK;
    }
    int errCode = UpdateAssetId(tableSchema, dataKey, vBucket);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] failed to update assetId on table, %d.", errCode);
    }
    return errCode;
}

void SQLiteSingleVerRelationalStorageExecutor::UpdateLocalAssetId(const VBucket &vBucket, const std::string &fieldName,
    Asset &asset)
{
    for (const auto &[col, value] : vBucket) {
        if (value.index() == TYPE_INDEX<Asset> && col == fieldName) {
            asset = std::get<Asset>(value);
        }
    }
}

void SQLiteSingleVerRelationalStorageExecutor::UpdateLocalAssetsId(const VBucket &vBucket, const std::string &fieldName,
    Assets &assets)
{
    for (const auto &[col, value] : vBucket) {
        if (value.index() == TYPE_INDEX<Assets> && col == fieldName) {
            assets = std::get<Assets>(value);
        }
    }
}

void SQLiteSingleVerRelationalStorageExecutor::UpdateLocalAssetsIdInner(const Assets &cloudAssets, Assets &assets)
{
    for (const auto &cloudAsset : cloudAssets) {
        for (auto &asset : assets) {
            if (asset.name == cloudAsset.name) {
                asset.assetId = cloudAsset.assetId;
            }
        }
    }
}

int SQLiteSingleVerRelationalStorageExecutor::BindAssetToBlobStatement(const Asset &asset, int index,
    sqlite3_stmt *&stmt)
{
    std::vector<uint8_t> blobValue;
    int errCode = RuntimeContext::GetInstance()->AssetToBlob(asset, blobValue);
    if (errCode != E_OK) {
        LOGE("Transfer asset to blob failed, %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, index, blobValue, false);
    if (errCode != E_OK) {
        LOGE("Bind asset blob to statement failed, %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::BindAssetsToBlobStatement(const Assets &assets, int index,
    sqlite3_stmt *&stmt)
{
    std::vector<uint8_t> blobValue;
    int errCode = RuntimeContext::GetInstance()->AssetsToBlob(assets, blobValue);
    if (errCode != E_OK) {
        LOGE("Transfer asset to blob failed, %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, index, blobValue, false);
    if (errCode != E_OK) {
        LOGE("Bind asset blob to statement failed, %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAssetOnTableInner(sqlite3_stmt *&stmt, Asset &asset)
{
    int errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, 0, blobValue);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] Get column blob value failed, %d", errCode);
            return errCode;
        }
        errCode = RuntimeContext::GetInstance()->BlobToAsset(blobValue, asset);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] Transfer blob to asset failed, %d", errCode);
        }
        return errCode;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    } else {
        LOGE("[RDBExecutor] Step failed when get asset from table, errCode = %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAssetOnTable(const std::string &tableName,
    const std::string &fieldName, const int64_t dataKey, Asset &asset)
{
    sqlite3_stmt *selectStmt = nullptr;
    std::string queryAssetSql = "SELECT " + fieldName + " FROM '" + tableName +
        "' WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + std::to_string(dataKey) + ";";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, queryAssetSql, selectStmt);
    if (errCode != E_OK) {
        LOGE("Get select asset statement failed, %d", errCode);
        return errCode;
    }
    errCode = GetAssetOnTableInner(selectStmt, asset);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(selectStmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAssetsOnTableInner(sqlite3_stmt *&stmt, Assets &assets)
{
    int errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::vector<uint8_t> blobValue;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, 0, blobValue);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] Get column blob value failed, %d", errCode);
            return errCode;
        }
        errCode = RuntimeContext::GetInstance()->BlobToAssets(blobValue, assets);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor] Transfer blob to assets failed, %d", errCode);
        }
        return errCode;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    } else {
        LOGE("[RDBExecutor] Step failed when get assets from table, errCode = %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAssetsOnTable(const std::string &tableName,
    const std::string &fieldName, const int64_t dataKey, Assets &assets)
{
    sqlite3_stmt *selectStmt = nullptr;
    std::string queryAssetsSql = "SELECT " + fieldName + " FROM '" + tableName +
        "' WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + std::to_string(dataKey) + ";";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, queryAssetsSql, selectStmt);
    if (errCode != E_OK) {
        LOGE("Get select assets statement failed, %d", errCode);
        return errCode;
    }
    errCode = GetAssetsOnTableInner(selectStmt, assets);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(selectStmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::BindAssetFiledToBlobStatement(const TableSchema &tableSchema,
    const std::vector<Asset> &assetOfOneRecord, const std::vector<Assets> &assetsOfOneRecord, sqlite3_stmt *&stmt)
{
    int assetIndex = 0;
    int assetsIndex = 0;
    for (const auto &field : tableSchema.fields) {
        if (field.type == TYPE_INDEX<Asset>) {
            if (assetOfOneRecord[assetIndex].name.empty()) {
                continue;
            }
            int errCode = BindAssetToBlobStatement(assetOfOneRecord[assetIndex], assetIndex + assetsIndex + 1, stmt);
            if (errCode != E_OK) {
                LOGE("Bind asset to blob statement failed, %d", errCode);
                return errCode;
            }
            assetIndex++;
        } else if (field.type == TYPE_INDEX<Assets>) {
            if (assetsOfOneRecord[assetsIndex].empty()) {
                continue;
            }
            int errCode = BindAssetsToBlobStatement(assetsOfOneRecord[assetsIndex], assetIndex + assetsIndex + 1, stmt);
            if (errCode != E_OK) {
                LOGE("Bind assets to blob statement failed, %d", errCode);
                return errCode;
            }
            assetsIndex++;
        }
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::UpdateAssetsIdForOneRecord(const TableSchema &tableSchema,
    const std::string &sql, const std::vector<Asset> &assetOfOneRecord, const std::vector<Assets> &assetsOfOneRecord)
{
    int errCode = E_OK;
    int ret = E_OK;
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get update asset statement failed, %d", errCode);
        return errCode;
    }
    errCode = BindAssetFiledToBlobStatement(tableSchema, assetOfOneRecord, assetsOfOneRecord, stmt);
    if (errCode != E_OK) {
        LOGE("Asset field Bind asset to blob statement failed, %d", errCode);
        SQLiteUtils::ResetStatement(stmt, true, ret);
        return errCode != E_OK ? errCode : ret;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        LOGE("Step statement failed, %d", errCode);
    }
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteSingleVerRelationalStorageExecutor::UpdateAssetId(const TableSchema &tableSchema, int64_t dataKey,
    const VBucket &vBucket)
{
    int errCode = E_OK;
    std::vector<Asset> assetOfOneRecord;
    std::vector<Assets> assetsOfOneRecord;
    std::string updateAssetIdSql = "UPDATE " + tableSchema.name  + " SET";
    for (const auto &field : tableSchema.fields) {
        if (field.type == TYPE_INDEX<Asset>) {
            Asset asset;
            UpdateLocalAssetId(vBucket, field.colName, asset);
            assetOfOneRecord.push_back(asset);
            if (!asset.name.empty()) {
                updateAssetIdSql += " " + field.colName + " = ?,";
            }
        }
        if (field.type == TYPE_INDEX<Assets>) {
            Assets assets;
            UpdateLocalAssetsId(vBucket, field.colName, assets);
            assetsOfOneRecord.push_back(assets);
            if (!assets.empty()) {
                updateAssetIdSql += " " + field.colName + " = ?,";
            }
        }
    }
    if (updateAssetIdSql == "UPDATE " + tableSchema.name  + " SET") {
        return E_OK;
    }
    updateAssetIdSql.pop_back();
    updateAssetIdSql += " WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) + " = " + std::to_string(dataKey) + ";";
    errCode = UpdateAssetsIdForOneRecord(tableSchema, updateAssetIdSql, assetOfOneRecord, assetsOfOneRecord);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] failed to update asset id on table, %d.", errCode);
    }
    return errCode;
}

void SQLiteSingleVerRelationalStorageExecutor::SetPutDataMode(PutDataMode mode)
{
    putDataMode_ = mode;
}

void SQLiteSingleVerRelationalStorageExecutor::SetMarkFlagOption(MarkFlagOption option)
{
    markFlagOption_ = option;
}

int64_t SQLiteSingleVerRelationalStorageExecutor::GetDataFlag()
{
    if (putDataMode_ != PutDataMode::USER) {
        return static_cast<int64_t>(LogInfoFlag::FLAG_CLOUD) |
            static_cast<int64_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_CONSISTENCY);
    }
    uint32_t flag = static_cast<uint32_t>(LogInfoFlag::FLAG_LOCAL);
    if (markFlagOption_ == MarkFlagOption::SET_WAIT_COMPENSATED_SYNC) {
        flag |= static_cast<uint32_t>(LogInfoFlag::FLAG_WAIT_COMPENSATED_SYNC);
    }
    flag |= static_cast<int64_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_CONSISTENCY);
    return static_cast<int64_t>(flag);
}

std::string SQLiteSingleVerRelationalStorageExecutor::GetUpdateDataFlagSql()
{
    if (putDataMode_ == PutDataMode::SYNC) {
        return UPDATE_FLAG_CLOUD;
    }
    if (markFlagOption_ == MarkFlagOption::SET_WAIT_COMPENSATED_SYNC) {
        return UPDATE_FLAG_WAIT_COMPENSATED_SYNC;
    }
    return UPDATE_FLAG_CLOUD;
}

std::string SQLiteSingleVerRelationalStorageExecutor::GetDev()
{
    return putDataMode_ == PutDataMode::SYNC ? "cloud" : "";
}

std::vector<Field> SQLiteSingleVerRelationalStorageExecutor::GetUpdateField(const VBucket &vBucket,
    const TableSchema &tableSchema)
{
    std::set<std::string> useFields;
    std::vector<Field> fields;
    if (putDataMode_ == PutDataMode::SYNC) {
        for (const auto &field : tableSchema.fields) {
            useFields.insert(field.colName);
        }
        fields = tableSchema.fields;
    } else {
        for (const auto &field : vBucket) {
            if (field.first.empty() || field.first[0] == '#') {
                continue;
            }
            useFields.insert(field.first);
        }
        for (const auto &field : tableSchema.fields) {
            if (useFields.find(field.colName) == useFields.end()) {
                continue;
            }
            fields.push_back(field);
        }
    }
    return fields;
}

int SQLiteSingleVerRelationalStorageExecutor::UpdateRecordFlag(const std::string &tableName, bool recordConflict,
    const LogInfo &logInfo)
{
    bool useHashKey = false;
    if (logInfo.cloudGid.empty() && logInfo.dataKey == DBConstant::DEFAULT_ROW_ID) {
        if (logInfo.hashKey.empty()) {
            LOGE("[RDBExecutor] Update record flag failed with invalid args!");
            return -E_INVALID_ARGS;
        }
        useHashKey = true;
    }
    std::string sql = CloudStorageUtils::GetUpdateRecordFlagSql(tableName, recordConflict, logInfo);
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] Get stmt failed when update record flag, %d", errCode);
        return errCode;
    }
    int ret = E_OK;
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, 1, logInfo.timestamp); // 1 is timestamp
    if (errCode != E_OK) {
        LOGE("[Storage Executor] Bind timestamp to update record flag stmt failed, %d", errCode);
        SQLiteUtils::ResetStatement(stmt, true, ret);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, 2, logInfo.timestamp); // 2 is timestamp
    if (errCode != E_OK) {
        LOGE("[Storage Executor] Bind timestamp to update record status stmt failed, %d", errCode);
        SQLiteUtils::ResetStatement(stmt, true, ret);
        return errCode;
    }
    if (useHashKey) {
        errCode = SQLiteUtils::BindBlobToStatement(stmt, 3, logInfo.hashKey); // 3 is hash_key
        if (errCode != E_OK) {
            LOGE("[Storage Executor] Bind hashKey to update record flag stmt failed, %d", errCode);
            SQLiteUtils::ResetStatement(stmt, true, ret);
            return errCode;
        }
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        LOGE("[Storage Executor]Step update record flag stmt failed, %d", errCode);
    }
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode == E_OK ? ret : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetWaitCompensatedSyncDataPk(const TableSchema &table,
    std::vector<VBucket> &data)
{
    std::string sql = "SELECT ";
    std::vector<Field> pkFields;
    for (const auto &field : table.fields) {
        if (!field.primary) {
            continue;
        }
        sql += "b." + field.colName + ",";
        pkFields.push_back(field);
    }
    if (pkFields.empty()) {
        // ignore no pk table
        return E_OK;
    }
    sql.pop_back();
    sql += CloudStorageUtils::GetLeftJoinLogSql(table.name) + " WHERE " + FLAG_IS_WAIT_COMPENSATED_SYNC;
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[RDBExecutor] Get stmt failed when get wait compensated sync pk! errCode = %d", errCode);
        return errCode;
    }
    do {
        errCode = SQLiteUtils::StepWithRetry(stmt);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            VBucket pkData;
            errCode = GetRecordFromStmt(stmt, pkFields, 0, pkData);
            if (errCode != E_OK) {
                LOGE("[RDBExecutor] Get record failed when get wait compensated sync pk! errCode = %d", errCode);
                break;
            }
            data.push_back(pkData);
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else {
            LOGE("[RDBExecutor] Step failed when get wait compensated sync pk! errCode = %d", errCode);
            break;
        }
    } while (errCode == E_OK);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode == E_OK ? ret : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetRecordFromStmt(sqlite3_stmt *stmt, const std::vector<Field> fields,
    int startIndex, VBucket &record)
{
    int errCode = E_OK;
    for (const auto &field : fields) {
        Type cloudValue;
        errCode = SQLiteRelationalUtils::GetCloudValueByType(stmt, field.type, startIndex, cloudValue);
        if (errCode != E_OK) {
            break;
        }
        errCode = PutVBucketByType(record, field, cloudValue);
        if (errCode != E_OK) {
            break;
        }
        startIndex++;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::BindShareValueToInsertLogStatement(const VBucket &vBucket,
    const TableSchema &tableSchema, sqlite3_stmt *insertLogStmt)
{
    int errCode = E_OK;
    std::string version;
    if (putDataMode_ == PutDataMode::SYNC) {
        errCode = CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::VERSION_FIELD, vBucket, version);
        if ((errCode != E_OK && errCode != -E_NOT_FOUND)) {
            LOGE("get version for insert log statement failed, %d", errCode);
            return -E_CLOUD_ERROR;
        }
    }
    errCode = SQLiteUtils::BindTextToStatement(insertLogStmt, 10, version); // 10 is version
    if (errCode != E_OK) {
        LOGE("Bind version to insert log statement failed, %d", errCode);
        return errCode;
    }

    std::string shareUri;
    if (putDataMode_ == PutDataMode::SYNC) {
        errCode = CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::SHARING_RESOURCE_FIELD,
            vBucket, shareUri);
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            LOGE("get shareUri for insert log statement failed, %d", errCode);
            return -E_CLOUD_ERROR;
        }
    }

    errCode = SQLiteUtils::BindTextToStatement(insertLogStmt, 11, shareUri); // 11 is sharing_resource
    if (errCode != E_OK) {
        LOGE("Bind shareUri to insert log statement failed, %d", errCode);
        return errCode;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::MarkFlagAsConsistent(const std::string &tableName,
    const DownloadData &downloadData, const std::set<std::string> &gidFilters)
{
    if (downloadData.data.size() != downloadData.opType.size()) {
        LOGE("The num of data:%zu an opType:%zu is not equal.", downloadData.data.size(), downloadData.opType.size());
        return -E_CLOUD_ERROR;
    }
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(tableName) +
        " SET flag=flag&(~0x20), " + CloudDbConstant::UNLOCKING_TO_UNLOCK + " WHERE cloud_gid=? and timestamp=?;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get mark flag as consistent stmt failed, %d.", errCode);
        return errCode;
    }
    int ret = E_OK;
    int index = 0;
    for (const auto &data: downloadData.data) {
        SQLiteUtils::ResetStatement(stmt, false, ret);
        OpType opType = downloadData.opType[index++];
        if (opType == OpType::NOT_HANDLE || opType == OpType::LOCKED_NOT_HANDLE) {
            continue;
        }
        errCode = CloudStorageUtils::BindStepConsistentFlagStmt(stmt, data, gidFilters);
        if (errCode != E_OK) {
            break;
        }
    }
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode == E_OK ? ret : errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::QueryCount(const std::string &tableName, int64_t &count)
{
    return SQLiteRelationalUtils::QueryCount(dbHandle_, tableName, count);
}

int SQLiteSingleVerRelationalStorageExecutor::CheckInventoryData(const std::string &tableName)
{
    int64_t dataCount = 0;
    int errCode = SQLiteRelationalUtils::QueryCount(dbHandle_, tableName, dataCount);
    if (errCode != E_OK) {
        LOGE("Query count failed.", errCode);
        return errCode;
    }
    return dataCount > 0 ? -E_WITH_INVENTORY_DATA : E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::FillCloudVersionForUpload(const std::string &tableName,
    const CloudSyncBatch &batchData)
{
    if (batchData.extend.empty()) {
        return E_OK;
    }
    if (batchData.hashKey.empty() || batchData.extend.size() != batchData.hashKey.size()) {
        LOGE("invalid sync data for filling version.");
        return -E_INVALID_ARGS;
    }
    std::string sql = "UPDATE '" + DBCommon::GetLogTableName(tableName) +
        "' SET version = ? WHERE hash_key = ? ";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    int ret = E_OK;
    for (size_t i = 0; i < batchData.extend.size(); ++i) {
        errCode = BindUpdateVersionStatement(batchData.extend[i], batchData.hashKey[i], stmt);
        if (errCode != E_OK) {
            LOGE("bind update version stmt failed.");
            SQLiteUtils::ResetStatement(stmt, true, ret);
            return errCode;
        }
    }
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return ret;
}
} // namespace DistributedDB
#endif