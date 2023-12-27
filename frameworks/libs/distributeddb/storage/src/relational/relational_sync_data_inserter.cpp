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
#include "data_transformer.h"
#include "db_common.h"
#include "sqlite_utils.h"

namespace DistributedDB {
int SaveSyncDataStmt::ResetStatements(bool isNeedFinalize)
{
    int errCode = E_OK;
    if (saveDataStmt != nullptr) {
        SQLiteUtils::ResetStatement(saveDataStmt, isNeedFinalize, errCode);
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

RelationalSyncDataInserter::RelationalSyncDataInserter()
{
}

RelationalSyncDataInserter::~RelationalSyncDataInserter()
{
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

    const std::string sql = "INSERT OR REPLACE INTO '" + insertTableName_ + "'" +
        " (" + colName + ") VALUES (" + dataFormat + ");";
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get saving data statement fail! errCode:%d", errCode);
    }
    return errCode;
}

int RelationalSyncDataInserter::BindInsertStatement(sqlite3_stmt *stmt, const DataItem &dataItem)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }

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
            LOGD("field %s not found in local schema.", it.GetFieldName().c_str());
            dataIdx++;
            continue; // skip fields which is orphaned in remote
        }
        if (dataIdx >= data.optionalData.size()) {
            LOGD("field over size. cnt:%d, data size:%d", dataIdx, data.optionalData.size());
            break; // cnt should less then optionalData size.
        }
        errCode = SQLiteUtils::BindDataValueByType(stmt, data.optionalData[dataIdx], bindIdx++);
        if (errCode != E_OK) {
            LOGE("Bind data failed, errCode:%d, cid:%zu.", errCode, dataIdx);
            return errCode;
        }
        dataIdx++;
    }

    return E_OK;
}

int RelationalSyncDataInserter::GetDeleteLogStmt(sqlite3 *db, sqlite3_stmt *&stmt)
{
    std::string sql = "DELETE FROM " + DBConstant::RELATIONAL_PREFIX + localTable_.GetTableName() + "_log ";
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
        " (" + columnList + ") VALUES (" + dataFormat + ");";
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
    errCode = GetInsertStatement(db, stmt.saveDataStmt);
    if (errCode != E_OK) {
        LOGE("Get insert statement failed. err=%d", errCode);
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
} // namespace DistributedDB