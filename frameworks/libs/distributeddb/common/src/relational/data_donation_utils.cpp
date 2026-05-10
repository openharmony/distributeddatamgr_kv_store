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
#include "data_donation_utils.h"

#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "db_common.h"
#include "db_constant.h"
#include "res_finalizer.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "sqlite_utils.h"
#include "cloud/cloud_storage_utils.h"

namespace DistributedDB {

constexpr size_t MAX_SLOT_NUM = 100;   // Max tables of each matrix file
constexpr uint16_t BINLOG_DATA_PAIR_SIZE = 2;

namespace {
std::mutex g_matrixInfoMutex;
std::map<std::string, MatrixFileInfo> g_matrixInfoMap;
}

std::string DataDonationUtils::JoinPrimaryKey(const std::vector<DonateDataField> &changedData)
{
    std::string out = "(";
    for (auto &data : changedData) {
        for (const auto &pair : data.field) {
            out += pair.first + ",";
        }
    }
    out.pop_back();
    out += ")";

    return out;
}

std::string DataDonationUtils::GetFieldName(const std::string &tableName, const std::string &columnName)
{
    return tableName + "." + columnName;
}

std::string DataDonationUtils::GetSelectFieldName(const std::string &tableName, const std::string &columnName)
{
    std::string fieldName = GetFieldName(tableName, columnName);
    return fieldName + " AS [" + fieldName + "]";
}

int DataDonationUtils::MapCloudOpType(int opType, uint32_t &cloudOpType)
{
    for (int i = 0; i < OP_TYPE_NUM; i++) {
        if (opType == OP_TYPE_MAPPING[i].opType) {
            cloudOpType = OP_TYPE_MAPPING[i].cloudOpType;
            return E_OK;
        }
    }
    return -E_NOT_FOUND;
}

BinlogChangedData *DataDonationUtils::EnsureTableInChangedDatas(DataDonationSchema &schema,
    std::unordered_map<std::string, BinlogChangedData> &changedDatas, const std::string &tableName)
{
    auto it = changedDatas.find(tableName);
    if (it != changedDatas.end()) {
        return &it->second;
    }

    DataDonationSchema::DdRelationsPath &path = schema.GetRelationPath(tableName);
    if (path.relations.empty()) {
        return nullptr; // skip this row
    }

    BinlogChangedData data = {
        .tableName = tableName,
        .pkColumn = path.relations.front().key.localField.field,
        .changedData = {}
    };
    changedDatas.insert({data.tableName, data});
    return &changedDatas[tableName];
}

void DataDonationUtils::ExtractPkValueFromRow(const BinlogSearchResult &row, const std::string &pkColumn,
    uint32_t cloudOpType, const std::pair<int, uint64_t> &batchCursor, BinlogChangedData &changedData)
{
    for (sqlite3_uint64 j = 0; j < BINLOG_DATA_PAIR_SIZE * row.nCol; j += BINLOG_DATA_PAIR_SIZE) {
        if (std::string(row.nameAndValues[j]) == pkColumn) {
            VBucket bucket;
            bucket.insert_or_assign(std::string(row.nameAndValues[j + 1]), static_cast<int64_t>(cloudOpType));

            DonateDataField field = {.field = bucket, .binlogCursor = batchCursor};
            changedData.changedData.emplace_back(std::move(field));
            break;
        }
    }
}

int DataDonationUtils::GetPrimaryKeysFromBinlog(sqlite3* db, DataDonationSchema &schema,
    std::unordered_map<std::string, BinlogChangedData> &changedDatas)
{
    if (db == nullptr) {
        LOGE("[GetPrimaryKeysFromBinlog] db is null");
        return -E_INVALID_DB;
    }

    BinlogSearchResultSet *binlogResult = nullptr;
    int errCode = sqlite3_get_search_data_binlog(db, db, &binlogResult);
    if (errCode == SQLITE_DONE) {
        errCode = -E_SUBSCRIBE_QUERY_END;
    } else {
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
    }
    if (binlogResult == nullptr || binlogResult->results == nullptr) {
        LOGE("[GetPrimaryKeysFromBinlog] Get search data from binlog err: %d", errCode);
        return errCode;
    }

    std::pair<int, uint64_t> batchCursor = {0, 0};
    if (binlogResult->row_count > 0) {
        BinlogSearchResult row = binlogResult->results[binlogResult->row_count - 1];
        batchCursor = {row.fileIndex, static_cast<uint64_t>(row.readPos)};
    }
    for (int i = 0; i < binlogResult->row_count; i++) {
        BinlogSearchResult row = binlogResult->results[i];
        std::string tableName = std::string(row.tableName);

        BinlogChangedData *entry = EnsureTableInChangedDatas(schema, changedDatas, tableName);
        if (entry == nullptr) {
            continue;
        }

        uint32_t cloudOpType = 0;
        int ret = MapCloudOpType(row.op, cloudOpType);
        if (ret == -E_NOT_FOUND) {
            LOGW("[GetPrimaryKeysFromBinlog] Op type not found: %d", row.op);
            continue;
        }

        ExtractPkValueFromRow(row, entry->pkColumn, cloudOpType, batchCursor, *entry);
    }
    sqlite3_free_search_data_binlog(db, &binlogResult);
    return errCode;
}

std::string DataDonationUtils::BuildWhereClause(const std::string &tableName, const BinlogChangedData &changedData)
{
    std::string whereClause = std::string("WHERE ") + tableName + "." + changedData.pkColumn + " IN " +
        JoinPrimaryKey(changedData.changedData);
    return whereClause;
}

std::string DataDonationUtils::GenerateSqlByTableName(const std::string &tableName,
    const DataDonationSchema::DdRelationsPath &path, const BinlogChangedData &changedData)
{
    std::string selectClause = "SELECT " + GetSelectFieldName(changedData.tableName, changedData.pkColumn) + ",";
    std::string fromClause = "FROM ";

    // fields already add to select
    std::unordered_set<std::string> fieldsAdded = {GetFieldName(changedData.tableName, changedData.pkColumn)};

    bool isFirst = true;
    for (auto &relation : path.relations) {
        std::string selectField = GetFieldName(relation.foreignField.table, relation.foreignField.field);
        if (!relation.foreignField.field.empty() &&
            fieldsAdded.find(selectField) == fieldsAdded.end()) {
            fieldsAdded.insert(selectField);
            selectClause += (GetSelectFieldName(relation.foreignField.table, relation.foreignField.field) + ",");
        }

        // join local table and foreign table
        std::string joinStatement;
        if (isFirst) {
            joinStatement = relation.key.localField.table;
            isFirst = false;
        }
        joinStatement += (" LEFT JOIN " + relation.key.foreignField.table + " ON " +
            GetFieldName(relation.key.localField.table, relation.key.localField.field) + " = " +
            GetFieldName(relation.key.foreignField.table, relation.key.foreignField.field) + " ");

        fromClause += joinStatement;
    }

    selectClause.pop_back();
    return selectClause + " " + fromClause;
}

int DataDonationUtils::GenerateQuerySql(sqlite3* db, DataDonationSchema &schema,
    std::unordered_map<std::string, BinlogChangedData> &changedDatas,
    std::unordered_map<std::string, std::string> &sqls)
{
    // Get values from binlog for all tables
    int errCode = GetPrimaryKeysFromBinlog(db, schema, changedDatas);
    if (errCode != E_OK && errCode != -E_SUBSCRIBE_QUERY_END) {
        LOGE("[GenerateQuerySql] Read binlog err: %d", errCode);
        return errCode;
    }

    for (const auto &[tableName, changedData] : changedDatas) {
        DataDonationSchema::DdRelationsPath &path = schema.GetRelationPath(tableName);
        if (path.relations.empty()) {
            LOGW("[GenerateQuerySql] Relation path is empty");
            continue;
        }
        std::string sql = GenerateSqlByTableName(tableName, path, changedData);

        if (!changedData.changedData.empty()) {
            sql += BuildWhereClause(tableName, changedData);
        }
        sql += ";";
        sqls.insert({tableName, sql});
    }
    return errCode;
}

void DataDonationUtils::FilterNonOutputKeys(DdData &dataRow, const std::vector<DataDonationSchema::DdKeyOut> &keyOut)
{
    bool found = false;
    for (auto it = dataRow.data.cbegin(); it != dataRow.data.cend();) {
        if (it->first == CloudDbConstant::SUB_DATA_OP_TYPE) {
            it++;
            continue;
        }
        found = false;
        for (const auto &key : keyOut) {
            std::string realKey = GetFieldName(key.item.table, key.item.field);
            if (it->first == realKey) {
                found = true;
                break;
            }
        }
        if (!found) {
            it = dataRow.data.erase(it);
        } else {
            it++;
        }
    }
}

int DataDonationUtils::GetCursorByPkColumn(const VBucket &bucket, const BinlogChangedData &data,
    DdData &dataRow)
{
    int64_t pkValue = 0;
    int errCode = CloudStorageUtils::GetValueFromVBucket(GetFieldName(data.tableName, data.pkColumn), bucket, pkValue);
    if (errCode != E_OK) {
        LOGE("[GetCursorByPkColumn] Primary key column not found");
        return errCode;
    }

    bool found = false;
    for (const auto &dataField : data.changedData) {
        int64_t opType = 0;
        std::string changedPkValue = std::to_string(pkValue);
        errCode = CloudStorageUtils::GetValueFromVBucket(changedPkValue, dataField.field, opType);
        if (errCode != E_OK) {
            continue;
        }
        found = true;
        dataRow.data.insert_or_assign(CloudDbConstant::SUB_DATA_OP_TYPE, opType);
        dataRow.opType = static_cast<int16_t>(opType);
        dataRow.fileIdx = dataField.binlogCursor.first;
        dataRow.cursor = dataField.binlogCursor.second;
        break;
    }
    return found ? E_OK : -E_NOT_FOUND;
}

uint64_t *DataDonationUtils::MmapMatrixFile(const std::string &path, size_t mmapSize, int32_t &fileFd)
{
    (void)path;
    (void)mmapSize;
    fileFd = -1;
    return nullptr;
}

std::vector<uint64_t> DataDonationUtils::GetMatrixTableIndexs(const MatrixFileInfo &matrixFileInfo,
    const std::vector<std::string> &changedData)
{
    std::vector<uint64_t> indexList;
    for (const auto &tableName : changedData) {
        auto it = matrixFileInfo.matrixTables.find(tableName);
        if (it == matrixFileInfo.matrixTables.end()) {
            LOGW("[DataDonationUtils] Table not registered, %s",
                DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
            continue;
        }
        uint64_t index = it->second;
        if (index >= MAX_SLOT_NUM) {
            LOGW("[DataDonationUtils] Table index out of range %zu, limit: %zu, table: %s", index, MAX_SLOT_NUM,
                DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
            continue;
        }
        indexList.push_back(index);
    }
    return indexList;
}

int DataDonationUtils::UpdateMatrixFile(const MatrixFileInfo &fileInfo,
    const std::vector<std::string> &changedData, const MatrixFileUpdateConfig &config)
{
    (void)fileInfo;
    (void)changedData;
    (void)config;
    return E_OK;
}

bool DataDonationUtils::GetDbFileName(sqlite3 *db, std::string &fileName)
{
    if (db == nullptr) {
        return false;
    }

    auto dbFilePath = sqlite3_db_filename(db, nullptr);
    if (dbFilePath == nullptr) {
        return false;
    }
    fileName = std::string(dbFilePath);
    return true;
}

bool DataDonationUtils::EndsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.size() > str.size()) {
        return false;
    }
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool DataDonationUtils::GetSchemaPathByDbPath(const std::string &dbPath, std::string &output)
{
    char separator = '/';
    size_t lastPos = dbPath.rfind(separator);
    if (lastPos == std::string::npos) {
        return false;
    }

    output = dbPath.substr(0, lastPos) + "/" + DataDonationUtils::DATA_DONATION_SCHEMA_FILE;
    return true;
}

int DataDonationUtils::SaveSubscribeSchema(sqlite3 *db, const std::string &schema)
{
    std::string fullName;
    if (!DataDonationUtils::GetDbFileName(db, fullName)) {
        return -E_INVALID_DB;
    }

    std::string filePath;
    if (!DataDonationUtils::GetSchemaPathByDbPath(fullName, filePath)) {
        return -E_INVALID_DB;
    }
    std::ofstream file(filePath);
    if (!file.is_open()) {
        LOGE("[SaveSubscribeSchema] Open file failed errno: %d", errno);
        return -E_INVALID_FILE;
    }

    file << schema << std::endl;
    file.close();
    return E_OK;
}

int DataDonationUtils::SetTrackerMatrixInfo(sqlite3 *db, const MatrixFileInfo &info)
{
    if (info.matrixFilePath.empty() || info.matrixTables.empty()) {
        LOGE("Matrix info invalid, path empty: %d, matrix table size: %zu",
            info.matrixFilePath.empty(), info.matrixTables.size());
        return -E_INVALID_ARGS;
    }

    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        LOGE("Get db fileName failed.");
        return -E_INVALID_DB;
    }

    {
        std::lock_guard<std::mutex> autoLock(g_matrixInfoMutex);
        g_matrixInfoMap[fileName] = info;
    }
    return E_OK;
}

int DataDonationUtils::GetTrackerMatrixInfo(const std::string dbPath, const MatrixFileInfo &info)
{
    (void)dbPath;
    (void)info;
    return E_OK;
}

void DataDonationUtils::DataChangedObserver(const char *dbPath, char *tableName)
{
    MatrixFileInfo fileInfo;
    {
        std::lock_guard<std::mutex> autoLock(g_matrixInfoMutex);
        auto it = g_matrixInfoMap.find(std::string(dbPath));
        if (it == g_matrixInfoMap.end()) {
            LOGE("[UpdateMatrixFileCallback] Matrix file not registered");
            return;
        }
        fileInfo = it->second;
    }

    std::vector<std::string> changedData = {std::string(tableName)};
    MatrixFileUpdateConfig config = {.isFullSync = false};
    int errCode = DataDonationUtils::UpdateMatrixFile(fileInfo, changedData, config);
    if (errCode != DistributedDB::E_OK) {
        LOGE("[UpdateMatrixFileCallback] Update matrix file err: %d", errCode);
    }
}

void DataDonationUtils::SetDataChangedObserver(sqlite3 *db)
{
    if (db == nullptr) {
        LOGE("[SetDataChangedObserver] db is null");
        return;
    }
    sqlite3_set_xChange_callback_binlog(db, &DataChangedObserver);
}
}   // namespace DistributedDB
#endif