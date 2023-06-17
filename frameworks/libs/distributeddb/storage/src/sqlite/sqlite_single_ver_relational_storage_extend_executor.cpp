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

namespace DistributedDB {
int SQLiteSingleVerRelationalStorageExecutor::GetQueryInfoSql(const std::string &tableName, const VBucket &vBucket,
    std::set<std::string> &pkSet, std::vector<Field> &assetFields, std::string &querySql)
{
    if (assetFields.empty() && pkSet.empty()) {
        return GetQueryLogSql(tableName, vBucket, pkSet, querySql);
    }
    std::string cloudGid;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, vBucket, cloudGid);
    if (errCode != E_OK) {
        LOGE("Get cloud gid fail when query log table.");
        return errCode;
    }

    if (pkSet.empty() && cloudGid.empty()) {
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
    sql += " ON (a.data_key = b.rowid) WHERE ";
    if (!cloudGid.empty()) {
        sql += " a.cloud_gid = ? or ";
    }
    sql += "a.hash_key = ?";
    querySql = sql;
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::GetQueryLogRowid(const std::string &tableName,
    const VBucket &vBucket, int64_t &rowId)
{
    std::string cloudGid;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, vBucket, cloudGid);
    if (errCode != E_OK) {
        LOGE("Miss gid when fill Asset");
        return errCode;
    }
    std::string sql = "SELECT data_key FROM " + DBCommon::GetLogTableName(tableName) + " where cloud_gid = ?;";
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get select rowid statement failed, %d", errCode);
        return errCode;
    }

    int index = 1;
    errCode = SQLiteUtils::BindTextToStatement(stmt, index, cloudGid);
    if (errCode != E_OK) {
        LOGE("Bind cloud gid to query log rowid statement failed. %d", errCode);
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("Not find rowid from log by cloud gid. %d", errCode);
        errCode = -E_NOT_FOUND;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        rowId = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetFillDownloadAssetStatement(const std::string &tableName,
    const VBucket &vBucket, const std::vector<Field> &fields, bool isFullReplace, sqlite3_stmt *&statement)
{
    std::string sql = "UPDATE " + tableName + " SET ";
    for (const auto &field: fields) {
        sql += field.colName + " = ?,";
    }
    sql.pop_back();
    sql += " WHERE rowid = ?;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get fill asset statement failed, %d", errCode);
        return errCode;
    }
    for (size_t i = 0; i < fields.size(); ++i) {
        if (isFullReplace) {
            errCode = bindCloudFieldFuncMap_[TYPE_INDEX<Bytes>](i+1, vBucket, fields[i], stmt);
        } else {
            errCode = BindOneField(i+1, vBucket, fields[i], stmt);
        }
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(stmt, true, errCode);
            return errCode;
        }
    }
    statement = stmt;
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::FillCloudAsset(const TableSchema &tableSchema,
    VBucket &vBucket, bool isFullReplace)
{
    sqlite3_stmt *stmt = nullptr;
    int errCode = SetLogTriggerStatus(false);
    if (errCode != E_OK) {
        LOGE("Fail to set log trigger off, %d", errCode);
        return errCode;
    }
    std::vector<Field> assetsField;
    errCode = CloudStorageUtils::CheckAssetFromSchema(tableSchema, vBucket, assetsField);
    if (errCode != E_OK) {
        goto END;
    }

    int64_t rowId;
    errCode = GetQueryLogRowid(tableSchema.name, vBucket, rowId);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = GetFillDownloadAssetStatement(tableSchema.name, vBucket, assetsField, isFullReplace, stmt);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, assetsField.size() + 1, rowId);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        goto END;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        LOGE("Fill cloud asset failed:%d", errCode);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);

END:
    errCode = SetLogTriggerStatus(true);
    if (errCode != E_OK) {
        LOGE("Fail to set log trigger off, %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::FillCloudAssetForUpload(const CloudSyncData &data)
{
    if (data.updData.assets.empty() || data.updData.rowid.empty() || data.updData.timestamp.empty()) {
        return -E_INVALID_ARGS;
    }
    if (data.updData.assets.size() != data.updData.rowid.size() ||
        data.updData.assets.size() != data.updData.timestamp.size()) {
        return -E_INVALID_ARGS;
    }
    int errCode = SetLogTriggerStatus(false);
    if (errCode != E_OK) {
        LOGE("Fail to set log trigger off, %d", errCode);
        return errCode;
    }
    sqlite3_stmt *stmt = nullptr;
    for (size_t i = 0; i < data.updData.assets.size(); ++i) {
        errCode = GetFillUploadAssetStatement(data.tableName, data.updData.assets[i], stmt);
        if (errCode != E_OK) {
            goto END;
        }
        int64_t rowid = data.updData.rowid[i];
        errCode = SQLiteUtils::BindInt64ToStatement(stmt, data.updData.assets[i].size() + 1, rowid);
        if (errCode != E_OK) {
            break;
        }
        int64_t timeStamp = data.updData.timestamp[i];
        errCode = SQLiteUtils::BindInt64ToStatement(stmt, data.updData.assets[i].size() + 2, timeStamp); // 2 is index
        if (errCode != E_OK) {
            break;
        }
        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            SQLiteUtils::ResetStatement(stmt, false, errCode);
        } else {
            LOGE("Fill upload asset failed:%d", errCode);
            break;
        }
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    END:
    int endCode = SetLogTriggerStatus(true);
    if (endCode != E_OK) {
        LOGE("Fail to set log trigger off, %d", endCode);
    }
    return errCode;
}
int SQLiteSingleVerRelationalStorageExecutor::GetFillUploadAssetStatement(const std::string &tableName,
    const VBucket &vBucket, sqlite3_stmt *&statement)
{
    std::string sql = "UPDATE " + tableName + " SET ";
    for (const auto &item: vBucket) {
        sql += item.first + " = ?,";
    }
    sql.pop_back();
    sql += " WHERE rowid = ? and (select 1 from " + DBCommon::GetLogTableName(tableName) + " WHERE timestamp = ?);";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }
    int index = 1;
    for (const auto &item: vBucket) {
        Field field = {
            .colName = item.first, .type = static_cast<int32_t>(item.second.index())
        };
        errCode = bindCloudFieldFuncMap_[item.second.index()](index++, vBucket, field, statement);
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(statement, true, errCode);
            return errCode;
        }
    }
    return errCode;
}
} // namespace DistributedDB
#endif