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
namespace DistributedDB {
int SQLiteSingleVerRelationalStorageExecutor::QuerySubscribeOutput(const DataDonationSchema::DdRelationsPath &path,
    const DBSubscribeCur &cursorIn, DBSubscribeCur &cursorOut, std::vector<VBucket> &dataOut)
{
    int errCode = E_OK;
    if (cursorIn.cursor == 0) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_reset_search_hwm_binlog(dbHandle_));
        if (errCode != E_OK) {
            LOGE("[QuerySubscribeOutput] sqlite3_reset_search_hwm_binlog failed: %d", errCode);
            return errCode;
        }
    }
    std::string sql;
    errCode = GetQuerySubscribeSql(path, cursorIn.cursor, sql);
    if (errCode != E_OK) {
        LOGE("[QuerySubscribeOutput] GetQuerySubscribeSql failed: %d", errCode);
        return errCode;
    }
    
    SqlCondition condition;
    condition.sql = sql;
    condition.readOnly = true;
    
    errCode = ExecuteSql(condition, dataOut);
    if (errCode != E_OK) {
        LOGE("[QuerySubscribeOutput] ExecuteSql failed: %d", errCode);
        return errCode;
    }
    for (auto& bucket : dataOut) {
        bucket.insert_or_assign(CloudDbConstant::SUB_DATA_OP_TYPE, static_cast<int64_t>(SubDataOpType::OP_INSERT));
    }
    
    cursorOut.queryType = cursorIn.queryType;
    cursorOut.cursor = cursorIn.cursor + static_cast<uint64_t>(dataOut.size());
    
    return dataOut.size() < CloudDbConstant::SUBSCRIBE_QUERY_LIMIT ? -E_SUBSCRIBE_QUERY_END : E_OK;
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
            (void)CloudStorageUtils::GetValueFromVBucket(it->second.pkColumn, bucket, pkValue);
            matchedPks.insert(std::to_string(pkValue));

            DdData dataRow(bucket);
            // match cursor and opType from binlog result
            ret = DataDonationUtils::GetCursorByPkColumn(bucket, it->second, dataRow);
            if (ret != E_OK) {
                LOGE("[QuerySubscribeOutput] Get cursor from binlog err: %d", ret);
                return ret;
            }
            DataDonationUtils::FilterNonOutputKeys(dataRow, keyOut);
            dataOut.emplace_back(dataRow);
        }
        SupplementUnmatchedDeletedRecords(it->second, matchedPks, dataOut);
    }
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
    std::vector<DdData> &dataOut) const
{
    for (const auto &dataField : changedData.changedData) {
        if (dataField.field.empty()) {
            continue;
        }
        auto pkValueStr = dataField.field.begin()->first;
        int64_t opType = std::get<int64_t>(dataField.field.begin()->second);
        if (matchedPks.find(pkValueStr) == matchedPks.end() &&
            opType == static_cast<int64_t>(SubDataOpType::OP_DELETE)) {
            VBucket deletedBucket;
            deletedBucket.insert_or_assign(DataDonationUtils::GetFieldName(changedData.tableName, changedData.pkColumn),
                pkValueStr);
            deletedBucket.insert_or_assign(CloudDbConstant::SUB_DATA_OP_TYPE, opType);
            DdData ddData(deletedBucket);
            ddData.opType = opType;
            ddData.fileIdx = dataField.binlogCursor.first;
            ddData.cursor = dataField.binlogCursor.second;
            dataOut.push_back(ddData);
        }
    }
}

int SQLiteSingleVerRelationalStorageExecutor::GetQuerySubscribeSql(const DataDonationSchema::DdRelationsPath &path,
    int64_t cursor, std::string &sql) const
{
    DataDonationSqlGenerator generator;
    return generator.GenerateQuerySql(path, cursor, sql);
}
} // namespace DistributedDB
#endif