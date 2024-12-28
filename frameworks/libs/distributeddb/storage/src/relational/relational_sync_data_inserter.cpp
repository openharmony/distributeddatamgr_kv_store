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
    return errCode;
}

RelationalSyncDataInserter RelationalSyncDataInserter::CreateInserter(const std::string &deviceName,
    const QueryObject &query, const RelationalSchemaObject &localSchema, const std::vector<FieldInfo> &remoteFields,
    const StoreInfo &info)
{
    RelationalSyncDataInserter inserter;
    inserter.SetHashDevId(DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceName)));
    inserter.SetRemoteFields(remoteFields);
    inserter.SetQuery(query);
    TableInfo localTable = localSchema.GetTable(query.GetTableName());
    inserter.SetLocalTable(localTable);
    inserter.SetTableMode(localSchema.GetTableMode());
    if (localSchema.GetTableMode() == DistributedTableMode::COLLABORATION) {
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

    std::string sql = "INSERT OR REPLACE INTO '" + insertTableName_ + "'" +
        "(" + colName + ") VALUES(" + dataFormat + ");";
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get insert data statement fail! errCode:%d", errCode);
    }
    return errCode;
}

int RelationalSyncDataInserter::SaveData(bool isUpdate, const DataItem &dataItem,
    SaveSyncDataStmt &saveSyncDataStmt)
{
    sqlite3_stmt *&stmt = isUpdate ? saveSyncDataStmt.updateDataStmt : saveSyncDataStmt.insertDataStmt;
    std::set<std::string> filterSet;
    if (isUpdate) {
        for (const auto &primaryKey : localTable_.GetIdentifyKey()) {
            filterSet.insert(primaryKey);
        }
    }
    if (stmt == nullptr) {
        LOGW("skip save data %s", DBCommon::StringMiddleMasking(DBCommon::VectorToHexString(dataItem.hashKey)).c_str());
        return E_OK;
    }

    int errCode = BindSaveDataStatement(isUpdate, dataItem, filterSet, stmt);
    if (errCode != E_OK) {
        LOGE("Bind data failed, errCode=%d.", errCode);
        int ret = E_OK;
        SQLiteUtils::ResetStatement(stmt, false, ret);
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, false, ret);
    return errCode;
}

int RelationalSyncDataInserter::BindSaveDataStatement(bool isExist, const DataItem &dataItem,
    const std::set<std::string> &filterSet, sqlite3_stmt *stmt)
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
        if (filterSet.find(it.GetFieldName()) != filterSet.end()) {
            dataIdx++;
            continue; // skip fields when update
        }
        if (dataIdx >= data.optionalData.size()) {
            LOGD("field over size. cnt:%d, data size:%d", dataIdx, data.optionalData.size());
            break; // cnt should less than optionalData size.
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
    std::string columnList = "data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key";
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
        SQLiteUtils::ResetStatement(logStmt, true, errCode);
        LOGE("[info statement] Get query statement fail! errCode:%d", errCode);
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
    }
    return errCode;
}

int RelationalSyncDataInserter::Iterate(const std::function<int (DataItem &)> &saveSyncDataItem)
{
    int errCode = E_OK;
    for (auto &it : entries_) {
        it.dev = hashDevId_;
        errCode = saveSyncDataItem(it);
        if (errCode != E_OK) {
            LOGE("Save sync data item failed. err=%d", errCode);
            break;
        }
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
            updateValue.append(it.GetFieldName()).append("=?");
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

int RelationalSyncDataInserter::SaveSyncLog(sqlite3 *db, sqlite3_stmt *statement, sqlite3_stmt *queryStmt,
    const DataItem &dataItem, int64_t rowid)
{
    auto updateCursor = CloudStorageUtils::GetCursorIncSql(query_.GetTableName());
    int errCode = SQLiteUtils::ExecuteRawSQL(db, updateCursor);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncDataInserter] update cursor failed %d", errCode);
        return errCode;
    }
    LogInfo logInfoGet;
    errCode = SQLiteRelationalUtils::GetLogInfoPre(queryStmt, mode_, dataItem, logInfoGet);
    LogInfo logInfoBind;
    logInfoBind.hashKey = dataItem.hashKey;
    logInfoBind.device = dataItem.dev;
    logInfoBind.timestamp = dataItem.timestamp;
    logInfoBind.flag = dataItem.flag;

    if (errCode == -E_NOT_FOUND) { // insert
        logInfoBind.wTimestamp = dataItem.writeTimestamp;
        logInfoBind.originDev = dataItem.dev;
    } else if (errCode == E_OK) { // update
        logInfoBind.wTimestamp = logInfoGet.wTimestamp;
        logInfoBind.originDev = logInfoGet.originDev;
    } else {
        LOGE("[RelationalSyncDataInserter] get log info failed %d", errCode);
        return errCode;
    }

    // bind
    SQLiteUtils::BindInt64ToStatement(statement, 1, rowid);  // 1 means dataKey index
    std::vector<uint8_t> originDev(logInfoBind.originDev.begin(), logInfoBind.originDev.end());
    SQLiteUtils::BindBlobToStatement(statement, 2, originDev);  // 2 means ori_dev index
    SQLiteUtils::BindInt64ToStatement(statement, 3, logInfoBind.timestamp);  // 3 means timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, 4, logInfoBind.wTimestamp);  // 4 means w_timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, 5, logInfoBind.flag);  // 5 means flag index
    SQLiteUtils::BindBlobToStatement(statement, 6, logInfoBind.hashKey);  // 6 means hashKey index
    errCode = SQLiteUtils::StepWithRetry(statement, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    }
    return errCode;
}
} // namespace DistributedDB