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
#ifdef RELATIONAL_STORE
#include "sqlite_single_ver_relational_storage_executor.h"

#include "cloud/cloud_storage_utils.h"
#include "data_donation_sql_generator.h"
#include "data_donation_utils.h"
#include "db_common.h"
#include "sqlite_relational_utils.h"
namespace DistributedDB {
int SQLiteSingleVerRelationalStorageExecutor::QuerySubscribeOutputWithCursor(
    const DataDonationSchema::DdRelationsPath &path,
    const std::vector<std::pair<std::string, int64_t>> &inputCursorValues,
    const std::vector<std::pair<std::string, int64_t>> &inputMaxRowids,
    GetAllQueryResult &result)
{
    std::string mainTable = DataDonationSqlGenerator::BuildFromTableName(path);
    std::vector<std::string> tableNames = DataDonationSqlGenerator::GetJoinedTableNames(path);

    if (!SQLiteRelationalUtils::GetDbFileName(dbHandle_, result.dbPath)) {
        LOGE("[QuerySubscribeOutputWithCursor] Get db path failed");
        return -E_INVALID_DB;
    }

    result.mainTable = mainTable;
    result.maxRowids = inputMaxRowids;

    std::string sql;
    int errCode = GetQuerySubscribeSql(path, inputCursorValues, inputMaxRowids, sql);
    if (errCode != E_OK) {
        LOGE("[QuerySubscribeOutputWithCursor] GetQuerySubscribeSql failed: %d", errCode);
        return errCode;
    }

    SqlCondition condition;
    condition.sql = sql;
    errCode = ExecuteSql(condition, result.dataOut);
    if (errCode != E_OK) {
        LOGE("[QuerySubscribeOutputWithCursor] ExecuteSql failed: %d", errCode);
        return errCode;
    }

    result.cursorValues.clear();
    ExtractAndRemoveRowid(tableNames, result.dataOut, result.cursorValues);

    for (auto &bucket : result.dataOut) {
        bucket.insert_or_assign(CloudDbConstant::SUB_DATA_OP_TYPE,
            static_cast<int64_t>(SubDataOpType::OP_INSERT));
    }

    if (!result.maxRowids.empty()) {
        result.sessionCursor = static_cast<uint64_t>(result.maxRowids[0].second);
    }

    if (result.dataOut.size() < CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL) {
        return -E_SUBSCRIBE_QUERY_END;
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::InitFullQuery(const std::string &mainTable,
    const std::string &dbPath, const std::vector<std::string> &tableNames, int64_t &maxRowid,
    std::vector<std::pair<std::string, int64_t>> &maxRowids)
{
    int errCode = DataDonationUtils::CheckBinlogDirExist(dbPath);
    if (errCode != E_OK) {
        LOGE("[InitFullQuery] Binlog dir not exist: %d", errCode);
        return errCode;
    }

    for (const auto &tableName : tableNames) {
        int64_t tableMaxRowid = 0;
        errCode = GetMaxRowid(tableName, tableMaxRowid);
        if (errCode != E_OK) {
            LOGE("[InitFullQuery] GetMaxRowid for %s failed: %d", tableName.c_str(), errCode);
            return errCode;
        }
        maxRowids.emplace_back(tableName, tableMaxRowid);
    }

    if (!maxRowids.empty()) {
        maxRowid = maxRowids[0].second;
    }
    if (maxRowid == 0) {
        return -E_SUBSCRIBE_QUERY_END;
    }

    errCode = DataDonationUtils::SaveRowidHwm(dbPath, mainTable, {}, maxRowids);
    if (errCode != E_OK) {
        LOGE("[InitFullQuery] SaveRowidHwm failed: %d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_reset_search_hwm_binlog(dbHandle_));
    if (errCode != E_OK) {
        LOGE("[InitFullQuery] sqlite3_reset_search_hwm_binlog failed: %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::ResumeFullQuery(const std::string &mainTable,
    const std::string &dbPath, int64_t &maxRowid,
    std::vector<std::pair<std::string, int64_t>> &cursorValues,
    std::vector<std::pair<std::string, int64_t>> &maxRowids) const
{
    int errCode = DataDonationUtils::LoadRowidHwm(dbPath, mainTable, maxRowid, cursorValues, maxRowids);
    if (errCode != E_OK) {
        LOGE("[ResumeFullQuery] LoadRowidHwm failed: %d", errCode);
    }
    return errCode;
}

void SQLiteSingleVerRelationalStorageExecutor::ExtractAndRemoveRowid(const std::vector<std::string> &tableNames,
    std::vector<VBucket> &dataOut, std::vector<std::pair<std::string, int64_t>> &cursorValues) const
{
    if (dataOut.empty()) {
        return;
    }

    const VBucket &lastRow = dataOut.back();
    for (const auto &tableName : tableNames) {
        std::string rowidKey = DataDonationUtils::GetFieldName(tableName, DBConstant::SQLITE_INNER_ROWID);
        auto it = lastRow.find(rowidKey);
        if (it != lastRow.end()) {
            if (it->second.index() == TYPE_INDEX<int64_t>) {
                cursorValues.emplace_back(tableName, std::get<int64_t>(it->second));
            } else {
                cursorValues.emplace_back(tableName, -1);
            }
        } else {
            cursorValues.emplace_back(tableName, -1);
        }
    }

    for (auto &bucket : dataOut) {
        for (const auto &tableName : tableNames) {
            std::string rowidKey = DataDonationUtils::GetFieldName(tableName, DBConstant::SQLITE_INNER_ROWID);
            bucket.erase(rowidKey);
        }
    }
}

int SQLiteSingleVerRelationalStorageExecutor::QuerySubscribeOutput(DataDonationSchema &schema,
    std::vector<DdData> &dataOut)
{
    std::unordered_map<std::string, std::string> sqls;
    std::unordered_map<std::string, BinlogChangedData> changedDatas;
    int errCode = DataDonationUtils::GenerateQuerySql(dbHandle_, schema, changedDatas, sqls);
    if (errCode != E_OK && errCode != -E_SUBSCRIBE_QUERY_END) {
        LOGE("[QuerySubscribeOutput] GenerateQuerySql failed: %d", errCode);
        return errCode;
    }

    for (const auto &[tableName, sql] : sqls) {
        auto it = changedDatas.find(tableName);
        if (it == changedDatas.end()) {
            LOGW("[QuerySubscribeOutput] No changed data for table");
            continue;
        }

        std::vector<VBucket> queryResult;
        int ret = ExecuteTableQuery(sql, queryResult);
        if (ret != E_OK) {
            LOGE("[QuerySubscribeOutput] ExecuteSql failed: %d", ret);
            return ret;
        }

        std::vector<DataDonationSchema::DdKeyOut> keyOut = schema.GetKeyOut();
        std::unordered_set<std::string> matchedPks;
        for (auto& bucket : queryResult) {
            int64_t pkValue = 0;
            if (CloudStorageUtils::GetValueFromVBucket(it->second.pkColumn, bucket, pkValue) == E_OK) {
                matchedPks.insert(std::to_string(pkValue));
            }

            DdData dataRow(bucket);
            ret = DataDonationUtils::GetCursorByPkColumn(bucket, it->second, dataRow);
            if (ret != E_OK) {
                LOGE("[QuerySubscribeOutput] Get cursor from binlog err: %d", ret);
                return ret;
            }
            DataDonationUtils::FilterNonOutputKeys(dataRow, keyOut);
            dataOut.emplace_back(dataRow);
        }
        SupplementUnmatchedDeletedRecords(it->second, matchedPks, keyOut, dataOut);
    }
    FilterEmptyData(dataOut);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::ExecuteTableQuery(const std::string &sql,
    std::vector<VBucket> &queryResult)
{
    SqlCondition condition;
    condition.sql = sql;
    condition.readOnly = true;
    int errCode = ExecuteSql(condition, queryResult);
    if (errCode != E_OK) {
        LOGE("[QuerySubscribeOutput] ExecuteSql failed: %d", errCode);
    }
    return errCode;
}

void SQLiteSingleVerRelationalStorageExecutor::SupplementUnmatchedDeletedRecords(
    const BinlogChangedData &changedData, const std::unordered_set<std::string> &matchedPks,
    std::vector<DataDonationSchema::DdKeyOut> keyOut, std::vector<DdData> &dataOut) const
{
    for (const auto &dataField : changedData.changedData) {
        if (dataField.field.empty()) {
            continue;
        }
        if (dataField.field.size() != dataField.opType.size() || dataField.field.size() != dataField.colType.size()) {
            LOGE("[SupplementUnmatchedDeletedRecords] Invalid field size");
            continue;
        }
        for (size_t i = 0; i < dataField.field.size(); i++) {
            std::string pkValueStr = dataField.field[i];
            int64_t opType = static_cast<int64_t>(dataField.opType[i]);
            int colType = dataField.colType[i];
            if (matchedPks.find(pkValueStr) == matchedPks.end() &&
                opType == static_cast<int64_t>(SubDataOpType::OP_DELETE) &&
                DataDonationUtils::IsTableInKeyOut(changedData.tableName, keyOut)) {
                VBucket deletedBucket;
                Type pkValue = DataDonationUtils::ConvertStrToType(pkValueStr, colType);
                deletedBucket.insert_or_assign(DataDonationUtils::GetFieldName(
                    changedData.tableName, changedData.pkColumn), pkValue);
                FillAllKeyOutPks(changedData, keyOut, deletedBucket);
                deletedBucket.insert_or_assign(CloudDbConstant::SUB_DATA_OP_TYPE, opType);
                DdData ddData(deletedBucket);
                ddData.opType = opType;
                ddData.fileIdx = dataField.binlogCursor.first;
                ddData.cursor = dataField.binlogCursor.second;
                dataOut.push_back(ddData);
            }
        }
    }
}

void SQLiteSingleVerRelationalStorageExecutor::FillAllKeyOutPks(const BinlogChangedData &changedData,
    std::vector<DataDonationSchema::DdKeyOut> keyOut, VBucket &deletedBucket) const
{
    for (const auto &key : keyOut) {
        if (changedData.tableName == key.item.table && changedData.pkColumn == key.item.field) {
            continue;
        }
        deletedBucket.insert_or_assign(DataDonationUtils::GetFieldName(
            key.item.table, key.item.field), Nil{});
    }
}

int SQLiteSingleVerRelationalStorageExecutor::GetQuerySubscribeSql(const DataDonationSchema::DdRelationsPath &path,
    const std::vector<std::pair<std::string, int64_t>> &cursorValues,
    const std::vector<std::pair<std::string, int64_t>> &maxRowids, std::string &sql) const
{
    DataDonationSqlGenerator generator;
    return generator.GenerateQuerySql(path, cursorValues, maxRowids, sql);
}

int SQLiteSingleVerRelationalStorageExecutor::GetMaxRowid(const std::string &tableName, int64_t &maxRowid)
{
    std::string sql = "SELECT MAX(rowid) FROM " + tableName;
    SqlCondition condition;
    condition.sql = sql;
    condition.readOnly = true;
    std::vector<VBucket> result;
    int errCode = ExecuteSql(condition, result);
    if (errCode != E_OK) {
        LOGE("[GetMaxRowid] ExecuteSql failed: %d", errCode);
        return errCode;
    }
    if (result.empty()) {
        maxRowid = 0;
        return E_OK;
    }
    auto it = result[0].begin();
    if (it == result[0].end()) {
        maxRowid = 0;
        return E_OK;
    }
    if (it->second.index() == TYPE_INDEX<Nil>) {
        maxRowid = 0;
        return E_OK;
    }
    if (it->second.index() == TYPE_INDEX<int64_t>) {
        maxRowid = std::get<int64_t>(it->second);
    } else {
        maxRowid = 0;
    }
    return E_OK;
}

void SQLiteSingleVerRelationalStorageExecutor::FilterEmptyData(std::vector<DdData> &dataRows) const
{
    for (auto it = dataRows.begin(); it != dataRows.end();) {
        if (DataDonationUtils::IsDonationDataEmpty(it->data)) {
            it = dataRows.erase(it);
        } else {
            it++;
        }
    }
}
} // namespace DistributedDB
#endif
