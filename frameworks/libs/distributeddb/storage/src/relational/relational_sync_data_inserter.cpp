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

#include "relational_sync_data_inserter.h"
#include "cloud/cloud_storage_utils.h"
#include "data_transformer.h"
#include "db_common.h"
#include "sqlite_relational_utils.h"
#include "sqlite_utils.h"

namespace DistributedDB {
int SaveSyncDataStmt::ResetStatements(bool isNeedFinalize)
{
    int errCode = E_OK;
    if (insertDataStmt != nullptr) {
        SQLiteUtils::ResetStatement(insertDataStmt, isNeedFinalize, errCode);
    }
    if (updateDataStmt != nullptr) {
        SQLiteUtils::ResetStatement(updateDataStmt, isNeedFinalize, errCode);
    }
    if (saveLogStmt != nullptr) {
        SQLiteUtils::ResetStatement(saveLogStmt, isNeedFinalize, errCode);
    }
    if (queryStmt != nullptr) {
        SQLiteUtils::ResetStatement(queryStmt, isNeedFinalize, errCode);
    }
    if (rmDataStmt != nullptr) {
        SQLiteUtils::ResetStatement(rmDataStmt, isNeedFinalize, errCode);
    }
    if (rmLogStmt != nullptr) {
        SQLiteUtils::ResetStatement(rmLogStmt, isNeedFinalize, errCode);
    }
    if (queryByFieldStmt != nullptr) {
        SQLiteUtils::ResetStatement(queryByFieldStmt, isNeedFinalize, errCode);
    }
    return errCode;
}

RelationalSyncDataInserter RelationalSyncDataInserter::CreateInserter(const std::string &deviceName,
    const QueryObject &query, const SchemaInfo &schemaInfo, const std::vector<FieldInfo> &remoteFields,
    const StoreInfo &info)
{
    RelationalSyncDataInserter inserter;
    inserter.SetHashDevId(DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceName)));
    inserter.SetRemoteFields(remoteFields);
    inserter.SetQuery(query);
    TableInfo localTable = schemaInfo.localSchema.GetTable(query.GetTableName());
    localTable.SetTrackerTable(schemaInfo.trackerSchema.GetTrackerTable(localTable.GetTableName()));
    localTable.SetDistributedTable(schemaInfo.localSchema.GetDistributedTable(query.GetTableName()));
    inserter.SetLocalTable(localTable);
    inserter.SetTableMode(schemaInfo.localSchema.GetTableMode());
    if (schemaInfo.localSchema.GetTableMode() == DistributedTableMode::COLLABORATION) {
        inserter.SetInsertTableName(localTable.GetTableName());
    } else {
        inserter.SetInsertTableName(DBCommon::GetDistributedTableName(deviceName, localTable.GetTableName(), info));
    }
    return inserter;
}

void RelationalSyncDataInserter::SetHashDevId(const std::string &hashDevId)
{
    hashDevId_ = hashDevId;
}

void RelationalSyncDataInserter::SetRemoteFields(std::vector<FieldInfo> remoteFields)
{
    remoteFields_ = std::move(remoteFields);
}

void RelationalSyncDataInserter::SetEntries(std::vector<DataItem> entries)
{
    entries_ = std::move(entries);
}

void RelationalSyncDataInserter::SetLocalTable(TableInfo localTable)
{
    localTable_ = std::move(localTable);
}

const TableInfo &RelationalSyncDataInserter::GetLocalTable() const
{
    return localTable_;
}

void RelationalSyncDataInserter::SetQuery(QueryObject query)
{
    query_ = std::move(query);
}

void RelationalSyncDataInserter::SetInsertTableName(std::string tableName)
{
    insertTableName_ = std::move(tableName);
}

void RelationalSyncDataInserter::SetTableMode(DistributedTableMode mode)
{
    mode_ = mode;
}

int RelationalSyncDataInserter::GetInsertStatement(sqlite3 *db, sqlite3_stmt *&stmt)
{
    if (stmt != nullptr) {
        return -E_INVALID_ARGS;
    }

    const auto &localTableFields = localTable_.GetFields();
    std::string colName;
    std::string dataFormat;
    for (const auto &it : remoteFields_) {
        if (localTableFields.find(it.GetFieldName()) == localTableFields.end()) {
            continue; // skip fields which is orphaned in remote
        }
        colName += "'" + it.GetFieldName() + "',";
        dataFormat += "?,";
    }
    colName.pop_back();
    dataFormat.pop_back();
    std::string sql = "INSERT ";
    if (mode_ == DistributedTableMode::SPLIT_BY_DEVICE) {
        sql += "OR REPLACE ";
    }
    sql += "INTO '" + insertTableName_ + "'" +
        "(" + colName + ") VALUES(" + dataFormat + ");";
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get insert data statement fail! errCode:%d", errCode);
    }
    return errCode;
}

int RelationalSyncDataInserter::GetDbValueByRowId(sqlite3 *db, const std::vector<std::string> &fieldList,
    const int64_t rowid, std::vector<Type> &values)
{
    if (fieldList.empty()) {
        LOGW("[RelationalSyncDataInserter][GetDbValueByRowId] fieldList is empty");
        return E_OK;
    }
    sqlite3_stmt *getValueStmt = nullptr;
    std::string sql = "SELECT ";
    for (const auto &col : fieldList) {
        sql += "data.'" + col + "',";
    }
    sql.pop_back();
    sql += " FROM '" + localTable_.GetTableName() + "' as data WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) +
        " = ?;";
    int errCode = SQLiteUtils::GetStatement(db, sql, getValueStmt);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncDataInserter][GetDbValueByRowId] failed to prepare statmement");
        return errCode;
    }

    SQLiteUtils::BindInt64ToStatement(getValueStmt, 1, rowid);
    errCode = SQLiteUtils::StepWithRetry(getValueStmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        VBucket bucket;
        errCode = SQLiteRelationalUtils::GetSelectVBucket(getValueStmt, bucket);
        if (errCode != E_OK) {
            LOGE("[RelationalSyncDataInserter][GetDbValueByRowId] failed to convert sql result to values");
            return SQLiteUtils::ProcessStatementErrCode(getValueStmt, true, errCode);
        }
        for (auto value : bucket) {
            values.push_back(value.second);
        }
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        LOGW("[RelationalSyncDataInserter][GetDbValueByRowId] found no data in db");
        errCode = E_OK;
    }
    return SQLiteUtils::ProcessStatementErrCode(getValueStmt, true, errCode);
}

int RelationalSyncDataInserter::GetObserverDataByRowId(sqlite3 *db, int64_t rowid, ChangeType type)
{
    std::vector<Type> primaryValues;
    std::vector<std::string> primaryKeys;

    if (localTable_.IsMultiPkTable() || localTable_.IsNoPkTable()) {
        primaryKeys.push_back(DBConstant::ROWID);
        primaryValues.push_back(rowid);
    }
    std::vector<std::string> pkList;
    for (const auto &primaryKey : localTable_.GetPrimaryKey()) {
        if (primaryKey.second != DBConstant::ROWID) {
            pkList.push_back(primaryKey.second);
        }
    }

    data_.field = primaryKeys;
    if (pkList.empty()) {
        data_.primaryData[type].push_back(primaryValues);
        return E_OK;
    }

    std::vector<Type> dbValues;
    int errCode = GetDbValueByRowId(db, pkList, rowid, dbValues);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncDataInserter][GetObserverDataByRowId] failed to get db values");
        data_.primaryData[type].push_back(primaryValues);
        return errCode;
    }
    data_.field.insert(data_.field.end(), pkList.begin(), pkList.end());
    primaryValues.insert(primaryValues.end(), dbValues.begin(), dbValues.end());
    data_.primaryData[type].push_back(primaryValues);
    return errCode;
}

int RelationalSyncDataInserter::SaveData(bool isUpdate, const DataItem &dataItem,
    SaveSyncDataStmt &saveSyncDataStmt, std::map<std::string, Type> &saveVals)
{
    sqlite3_stmt *&stmt = isUpdate ? saveSyncDataStmt.updateDataStmt : saveSyncDataStmt.insertDataStmt;
    std::set<std::string> filterSet;
    if (isUpdate) {
        for (const auto &primaryKey : localTable_.GetIdentifyKey()) {
            filterSet.insert(primaryKey);
        }
        auto distributedPk = localTable_.GetSyncDistributedPk();
        if (!distributedPk.empty()) {
            filterSet.insert(distributedPk.begin(), distributedPk.end());
        }
    }
    if (stmt == nullptr) {
        LOGW("skip save data %s", DBCommon::StringMiddleMasking(DBCommon::VectorToHexString(dataItem.hashKey)).c_str());
        return E_OK;
    }

    int errCode = BindSaveDataStatement(isUpdate, dataItem, filterSet, stmt, saveVals);
    if (errCode != E_OK) {
        LOGE("Bind data failed, errCode=%d.", errCode);
        return SQLiteUtils::ProcessStatementErrCode(stmt, false, errCode);
    }

    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, false, true, ret);
    return errCode;
}

int RelationalSyncDataInserter::BindSaveDataStatement(bool isExist, const DataItem &dataItem,
    const std::set<std::string> &filterSet, sqlite3_stmt *stmt, std::map<std::string, Type> &saveVals)
{
    OptRowDataWithLog data;
    // deserialize by remote field info
    int errCode = DataTransformer::DeSerializeDataItem(dataItem, data, remoteFields_);
    if (errCode != E_OK) {
        LOGE("DeSerialize dataItem failed! errCode = [%d]", errCode);
        return errCode;
    }

    size_t dataIdx = 0;
    int bindIdx = 1;
    const auto &localTableFields = localTable_.GetFields();
    for (const auto &it : remoteFields_) {
        if (localTableFields.find(it.GetFieldName()) == localTableFields.end()) {
            LOGD("field %s[%zu] not found in local schema.", DBCommon::StringMiddleMasking(it.GetFieldName()).c_str(),
                it.GetFieldName().size());
            dataIdx++;
            continue; // skip fields which is orphaned in remote
        }
        if (dataIdx >= data.optionalData.size()) {
            LOGD("field over size. cnt:%d, data size:%d", dataIdx, data.optionalData.size());
            break; // cnt should less than optionalData size.
        }
        Type saveVal;
        CloudStorageUtils::SaveChangedDataByType(data.optionalData[dataIdx], saveVal);
        saveVals[it.GetFieldName()] = saveVal;
        if (filterSet.find(it.GetFieldName()) != filterSet.end()) {
            dataIdx++;
            continue; // skip fields when update
        }
        errCode = SQLiteUtils::BindDataValueByType(stmt, data.optionalData[dataIdx], bindIdx++);
        if (errCode != E_OK) {
            LOGE("Bind data failed, errCode:%d, cid:%zu.", errCode, dataIdx);
            return errCode;
        }
        dataIdx++;
    }
    return isExist ? BindHashKeyAndDev(dataItem, stmt, bindIdx) : E_OK;
}

int RelationalSyncDataInserter::GetDeleteLogStmt(sqlite3 *db, sqlite3_stmt *&stmt)
{
    std::string sql = "DELETE FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + localTable_.GetTableName() +
        "_log ";
    if (mode_ == DistributedTableMode::COLLABORATION) {
        sql += "WHERE hash_key=?";
    } else {
        sql += "WHERE hash_key=? AND device=?";
    }

    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[DeleteSyncLog] Get statement fail!");
    }
    return errCode;
}

int RelationalSyncDataInserter::GetDeleteSyncDataStmt(sqlite3 *db, sqlite3_stmt *&stmt)
{
    std::string sql = "DELETE FROM '" + insertTableName_ + "' WHERE " + std::string(DBConstant::SQLITE_INNER_ROWID) +
        " IN (SELECT data_key FROM " + DBConstant::RELATIONAL_PREFIX + localTable_.GetTableName() + "_log ";
    if (mode_ == DistributedTableMode::COLLABORATION) {
        sql += "WHERE hash_key=?);";
    } else {
        sql += "WHERE hash_key=? AND device=? AND flag&0x01=0);";
    }
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[DeleteSyncDataItem] Get statement fail!, errCode:%d", errCode);
    }
    return errCode;
}

int RelationalSyncDataInserter::GetSaveLogStatement(sqlite3 *db, sqlite3_stmt *&logStmt, sqlite3_stmt *&queryStmt)
{
    const std::string tableName = DBConstant::RELATIONAL_PREFIX + query_.GetTableName() + "_log";
    std::string dataFormat = "?, '" + hashDevId_ + "', ?, ?, ?, ?, ?";
    TrackerTable trackerTable = localTable_.GetTrackerTable();
    if (trackerTable.GetExtendNames().empty()) {
        dataFormat += ", ?";
    } else {
        dataFormat += ", (select json_object(";
        for (const auto &extendColName : trackerTable.GetExtendNames()) {
            dataFormat += "'" + extendColName + "'," + extendColName + ",";
        }
        dataFormat.pop_back();
        dataFormat += ") from ";
        dataFormat += localTable_.GetTableName() + " where _rowid_ = ?)";
    }
    std::string columnList = "data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, extend_field";
    std::string sql = "INSERT OR REPLACE INTO " + tableName +
        " (" + columnList + ", cursor) VALUES (" + dataFormat + "," +
        CloudStorageUtils::GetSelectIncCursorSql(query_.GetTableName()) +");";
    int errCode = SQLiteUtils::GetStatement(db, sql, logStmt);
    if (errCode != E_OK) {
        LOGE("[info statement] Get log statement fail! errCode:%d", errCode);
        return errCode;
    }
    std::string selectSql = "select " + columnList + " from " + tableName;
    if (mode_ == DistributedTableMode::COLLABORATION) {
        selectSql += " where hash_key = ?;";
    } else {
        selectSql += " where hash_key = ? and device = ?;";
    }
    errCode = SQLiteUtils::GetStatement(db, selectSql, queryStmt);
    if (errCode != E_OK) {
        int ret = E_OK;
        SQLiteUtils::ResetStatement(logStmt, true, ret);
        LOGE("[info statement] Get query statement fail! errCode:%d, reset stmt ret: %d", errCode, ret);
    }
    return errCode;
}

int RelationalSyncDataInserter::PrepareStatement(sqlite3 *db, SaveSyncDataStmt &stmt)
{
    int errCode = GetSaveLogStatement(db, stmt.saveLogStmt, stmt.queryStmt);
    if (errCode != E_OK) {
        LOGE("Get save log statement failed. err=%d", errCode);
        return errCode;
    }
    errCode = GetInsertStatement(db, stmt.insertDataStmt);
    if (errCode != E_OK) {
        LOGE("Get insert statement failed. err=%d", errCode);
        return errCode;
    }
    errCode = GetUpdateStatement(db, stmt.updateDataStmt);
    if (errCode != E_OK) {
        LOGE("Get update statement failed. err=%d", errCode);
        return errCode;
    }
    errCode = GetQueryLogByFieldStmt(db, stmt.queryByFieldStmt);
    if (errCode != E_OK) {
        LOGE("Get query by field statement failed. err=%d", errCode);
    }
    return errCode;
}

int RelationalSyncDataInserter::Iterate(const std::function<int (DataItem &)> &saveSyncDataItem)
{
    int errCode = E_OK;
    for (auto &it : entries_) {
        it.dev = hashDevId_;
        int ret = saveSyncDataItem(it);
        errCode = errCode == E_OK ? ret : errCode;
    }
    return errCode;
}

int RelationalSyncDataInserter::GetUpdateStatement(sqlite3 *db, sqlite3_stmt *&stmt)
{
    if (stmt != nullptr) {
        return -E_INVALID_ARGS;
    }

    std::set<std::string> identifyKeySet;
    for (const auto &primaryKey : localTable_.GetIdentifyKey()) {
        identifyKeySet.insert(primaryKey);
    }
    auto distributedPk = localTable_.GetSyncDistributedPk();
    if (!distributedPk.empty()) {
        identifyKeySet.insert(distributedPk.begin(), distributedPk.end());
    }
    std::string updateValue;
    const auto &localTableFields = localTable_.GetFields();
    for (const auto &it : remoteFields_) {
        if (localTableFields.find(it.GetFieldName()) == localTableFields.end()) {
            continue; // skip fields which is orphaned in remote
        }
        if (identifyKeySet.find(it.GetFieldName()) == identifyKeySet.end()) {
            if (updateValue.empty()) {
                updateValue.append(" SET ");
            } else {
                updateValue.append(", ");
            }
            updateValue.append("'").append(it.GetFieldName()).append("'=?");
        }
    }
    if (updateValue.empty()) {
        // only sync pk no need update
        return E_OK;
    }
    std::string sql = "UPDATE '" + insertTableName_ + "'" + updateValue + " WHERE " +
        std::string(DBConstant::SQLITE_INNER_ROWID) + " IN (SELECT data_key FROM " +
        DBConstant::RELATIONAL_PREFIX + localTable_.GetTableName() + "_log ";
    if (mode_ == DistributedTableMode::COLLABORATION) {
        sql += "WHERE hash_key=?);";
    } else {
        sql += "WHERE hash_key=? AND device=? AND flag&0x01=0);";
    }
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get update data statement fail! errCode:%d", errCode);
    }
    return errCode;
}

int RelationalSyncDataInserter::BindHashKeyAndDev(const DataItem &dataItem, sqlite3_stmt *stmt,
    int beginIndex)
{
    int errCode = SQLiteUtils::BindBlobToStatement(stmt, beginIndex++, dataItem.hashKey);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncDataInserter] bind hash key failed %d", errCode);
        return errCode;
    }
    if (mode_ != DistributedTableMode::COLLABORATION) {
        errCode = SQLiteUtils::BindTextToStatement(stmt, beginIndex, dataItem.dev);
        if (errCode != E_OK) {
            LOGE("[RelationalSyncDataInserter] bind dev failed %d", errCode);
        }
    }
    return errCode;
}

int RelationalSyncDataInserter::SaveSyncLog(sqlite3 *db, const DataItem &dataItem,
    const DeviceSyncSaveDataInfo &deviceSyncSaveDataInfo, std::map<std::string, Type> &saveVals,
    SaveSyncDataStmt &saveStmt)
{
    if (std::get_if<int64_t>(&saveVals[DBConstant::ROWID]) == nullptr) {
        LOGE("[RelationalSyncDataInserter] Invalid args because of no rowid!");
        return -E_INVALID_ARGS;
    }
    auto updateCursor = CloudStorageUtils::GetCursorIncSql(query_.GetTableName());
    int errCode = SQLiteUtils::ExecuteRawSQL(db, updateCursor);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncDataInserter] update cursor failed %d", errCode);
        return errCode;
    }
    LogInfo logInfoBind;
    logInfoBind.hashKey = dataItem.hashKey;
    logInfoBind.device = dataItem.dev;
    logInfoBind.timestamp = dataItem.timestamp;
    logInfoBind.flag = dataItem.flag;

    if (!deviceSyncSaveDataInfo.isExist) { // insert
        logInfoBind.wTimestamp = dataItem.writeTimestamp;
        logInfoBind.originDev = dataItem.dev;
    } else { // update
        logInfoBind.wTimestamp = deviceSyncSaveDataInfo.localLogInfo.wTimestamp;
        logInfoBind.originDev = deviceSyncSaveDataInfo.localLogInfo.originDev;
    }
    auto statement = saveStmt.saveLogStmt;
    // bind
    int bindIndex = 0;
    // 1 means dataKey index
    SQLiteUtils::BindInt64ToStatement(statement, ++bindIndex, std::get<int64_t>(saveVals[DBConstant::ROWID]));
    std::vector<uint8_t> originDev(logInfoBind.originDev.begin(), logInfoBind.originDev.end());
    SQLiteUtils::BindBlobToStatement(statement, ++bindIndex, originDev); // 2 means ori_dev index
    SQLiteUtils::BindInt64ToStatement(statement, ++bindIndex, logInfoBind.timestamp); // 3 means timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, ++bindIndex, logInfoBind.wTimestamp); // 4 means w_timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, ++bindIndex, logInfoBind.flag); // 5 means flag index
    SQLiteUtils::BindBlobToStatement(statement, ++bindIndex, logInfoBind.hashKey); // 6 means hashKey index
    BindExtendFieldOrRowid(statement, saveVals, bindIndex); // bind extend_field or rowid
    errCode = SQLiteUtils::StepWithRetry(statement, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    }
    return errCode;
}

ChangedData &RelationalSyncDataInserter::GetChangedData()
{
    return data_;
}

void RelationalSyncDataInserter::BindExtendFieldOrRowid(sqlite3_stmt *&stmt, std::map<std::string, Type> &saveVals,
    int bindIndex)
{
    TrackerTable trackerTable = localTable_.GetTrackerTable();
    if (trackerTable.GetExtendNames().empty()) {
        SQLiteUtils::BindTextToStatement(stmt, ++bindIndex, "");
    } else {
        SQLiteUtils::BindInt64ToStatement(stmt, ++bindIndex, std::get<int64_t>(saveVals[DBConstant::ROWID]));
    }
}

std::vector<FieldInfo> RelationalSyncDataInserter::GetRemoteFields() const
{
    return remoteFields_;
}

int RelationalSyncDataInserter::GetQueryLogByFieldStmt(sqlite3 *db, sqlite3_stmt *&stmt)
{
    if (mode_ != DistributedTableMode::COLLABORATION) {
        stmt = nullptr;
        return E_OK;
    }
    auto syncPk = localTable_.GetSyncDistributedPk();
    if (syncPk.empty()) {
        stmt = nullptr;
        return E_OK;
    }
    std::string sql("SELECT ");
    std::string columnList = "log.data_key, log.device, log.ori_device, log.timestamp, log.wtimestamp, log.flag,"
        " log.hash_key, log.extend_field";
    auto table = localTable_.GetTableName();
    sql.append(columnList).append(" FROM ").append(DBCommon::GetLogTableName(table))
        .append(" AS log, ").append(table)
        .append(" AS data WHERE log.data_key = data._rowid_ AND ");
    for (size_t i = 0; i < syncPk.size(); ++i) {
        if (i != 0) {
            sql.append(", ");
        }
        sql.append("data.'").append(syncPk[i]).append("' = ?");
    }
    auto errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncDataInserter] Get query [%s][%zu] log stmt failed[%d]",
            DBCommon::StringMiddleMasking(table).c_str(), table.size(), errCode);
    }
    return errCode;
}

void RelationalSyncDataInserter::IncNonExistDelCnt()
{
    nonExistDelCnt_++;
}

void RelationalSyncDataInserter::DfxPrintLog() const
{
    if (nonExistDelCnt_ > 0) {
        LOGI("[RelationalStorageExecutor][SaveSyncDataItem] Delete non-exist data. Nothing to save, cnt:" PRIu32 ".",
            nonExistDelCnt_);
    }
}
} // namespace DistributedDB