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
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "db_common.h"
#include "db_constant.h"
#include "platform_specific.h"
#include "res_finalizer.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "sqlite_utils.h"
#include "cloud/cloud_storage_utils.h"
#include "store_types.h"

namespace DistributedDB {

constexpr int MAX_MONITOR_TABLE_COUNT = 50;
constexpr int MAX_MONITOR_COLUMN_COUNT = 100;

constexpr uint16_t BINLOG_DATA_PAIR_SIZE = 2;

namespace {
std::mutex g_matrixOperateMutex;
std::mutex g_matrixInfoMutex;
std::map<std::string, MatrixFileInfo> g_matrixInfoMap;
}

std::string DataDonationUtils::JoinPrimaryKey(const std::vector<DonateDataField> &changedData)
{
    std::string out = "(";
    for (auto &data : changedData) {
        for (const auto &fieldVal : data.field) {
            out += fieldVal + ",";
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
        .pkColumn = schema.GetPrimaryKey(tableName),
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
            int colType = row.colTypes[j / BINLOG_DATA_PAIR_SIZE];
            DonateDataField dataField;
            dataField.field.push_back(std::string(row.nameAndValues[j + 1]));
            dataField.colType.push_back(colType);
            dataField.opType.push_back(cloudOpType);
            dataField.binlogCursor = batchCursor;
            changedData.changedData.emplace_back(std::move(dataField));
            break;
        }
    }
}

Type DataDonationUtils::ConvertStrToType(const std::string &str, int colType)
{
    switch (colType) {
        case SQLITE_INTEGER: {
            int64_t value = 0;
            if (DBCommon::ConvertToUInt64(str, value)) {
                return value;
            }
            return str;
        }
        case SQLITE_NULL:
            return Nil{};
        case SQLITE_TEXT:
        case SQLITE_BLOB:
        case SQLITE_FLOAT:
        default:
            return str;
    }
}

int DataDonationUtils::CheckBinlogDirExist(const std::string &dbPath)
{
    std::string binlogDir = dbPath + DBConstant::BINLOG_DIR_POSTFIX;
    if (!OS::CheckPathExistence(binlogDir)) {
        LOGE("[DataDonationUtils] Binlog directory does not exist");
        return -E_INVALID_DB;
    }
    return E_OK;
}

std::string DataDonationUtils::GetRowidHwmFilePath(const std::string &dbPath)
{
    return dbPath + DBConstant::BINLOG_DIR_POSTFIX + ROWID_HWM_FILE;
}

int DataDonationUtils::ParseHwmFile(const std::string &filePath, JsonObject &root)
{
    std::ifstream existFile(filePath);
    if (!existFile.is_open()) {
        return E_OK;
    }
    std::stringstream buffer;
    buffer << existFile.rdbuf();
    existFile.close();
    std::string existContent = buffer.str();
    if (existContent.empty()) {
        return E_OK;
    }
    int errCode = root.Parse(existContent);
    if (errCode != E_OK || !root.IsValid()) {
        LOGW("[DataDonationUtils] Parse existing hwm file failed, will overwrite");
        root = JsonObject();
    }
    return E_OK;
}

int DataDonationUtils::WriteHwmFile(const std::string &filePath, const JsonObject &root)
{
    std::string content = root.ToString();
    std::ofstream file(filePath);
    if (!file.is_open()) {
        LOGE("[DataDonationUtils] Open rowid hwm file failed, rdstate: %d, errno: %d",
            static_cast<int>(file.rdstate()), errno);
        return -E_INVALID_FILE;
    }
    file << content << std::endl;
    file.close();
    return E_OK;
}

int DataDonationUtils::UpdateOrInsertCursorEntry(JsonObject &tableEntry,
    const std::vector<std::pair<std::string, int64_t>> &cursorValues,
    const std::vector<std::pair<std::string, int64_t>> &maxRowids)
{
    tableEntry.DeleteField(FieldPath{"cursor"});

    for (size_t i = 0; i < maxRowids.size(); ++i) {
        JsonObject cursorEntry;
        cursorEntry.InsertField(FieldPath{"tableName"}, FieldType::LEAF_FIELD_STRING,
            FieldValue{.stringValue = maxRowids[i].first}, false);
        cursorEntry.InsertField(FieldPath{"maxRowid"}, FieldType::LEAF_FIELD_LONG,
            FieldValue{.longValue = maxRowids[i].second}, false);
        int64_t lastRowid = (i < cursorValues.size()) ? cursorValues[i].second : 0;
        cursorEntry.InsertField(FieldPath{"lastRowid"}, FieldType::LEAF_FIELD_LONG,
            FieldValue{.longValue = lastRowid}, false);
        tableEntry.InsertField(FieldPath{"cursor"}, cursorEntry, true);
    }
    return E_OK;
}

int DataDonationUtils::ParseCursorFromHwm(const JsonObject &tableEntry,
    std::vector<std::pair<std::string, int64_t>> &cursorValues,
    std::vector<std::pair<std::string, int64_t>> &maxRowids)
{
    std::vector<JsonObject> cursorArray;
    int errCode = tableEntry.GetObjectArrayByFieldPath(FieldPath{"cursor"}, cursorArray);
    if (errCode != E_OK || cursorArray.empty()) {
        return E_OK;
    }

    for (const auto &entry : cursorArray) {
        FieldValue nameVal;
        if (entry.GetFieldValueByFieldPath(FieldPath{"tableName"}, nameVal) != E_OK) {
            continue;
        }
        FieldValue maxRowidVal;
        FieldType maxRowidType;
        if (entry.GetFieldValueByFieldPath(FieldPath{"maxRowid"}, maxRowidVal) != E_OK ||
            entry.GetFieldTypeByFieldPath(FieldPath{"maxRowid"}, maxRowidType) != E_OK) {
            continue;
        }
        FieldValue lastRowidVal;
        FieldType lastRowidType;
        if (entry.GetFieldValueByFieldPath(FieldPath{"lastRowid"}, lastRowidVal) != E_OK ||
            entry.GetFieldTypeByFieldPath(FieldPath{"lastRowid"}, lastRowidType) != E_OK) {
            continue;
        }
        cursorValues.emplace_back(nameVal.stringValue, lastRowidType == FieldType::LEAF_FIELD_INTEGER ?
            lastRowidVal.integerValue : lastRowidVal.longValue);
        maxRowids.emplace_back(nameVal.stringValue, maxRowidType == FieldType::LEAF_FIELD_INTEGER ?
            maxRowidVal.integerValue : maxRowidVal.longValue);
    }
    return E_OK;
}

int DataDonationUtils::SaveRowidHwm(const std::string &dbPath, const std::string &tableName,
    const std::vector<std::pair<std::string, int64_t>> &cursorValues,
    const std::vector<std::pair<std::string, int64_t>> &maxRowids)
{
    std::string filePath = GetRowidHwmFilePath(dbPath);
    JsonObject root;
    ParseHwmFile(filePath, root);

    std::vector<JsonObject> tables;
    if (root.IsValid()) {
        root.GetObjectArrayByFieldPath(FieldPath{"tables"}, tables);
    }

    bool found = false;
    for (auto &table : tables) {
        FieldValue nameVal;
        if (table.GetFieldValueByFieldPath(FieldPath{"mainTable"}, nameVal) == E_OK &&
            nameVal.stringValue == tableName) {
            JsonObject newTableEntry;
            newTableEntry.InsertField(FieldPath{"mainTable"}, FieldType::LEAF_FIELD_STRING,
                FieldValue{.stringValue = tableName}, false);
            if (!maxRowids.empty()) {
                UpdateOrInsertCursorEntry(newTableEntry, cursorValues, maxRowids);
            }
            table = newTableEntry;
            found = true;
            break;
        }
    }

    if (!found) {
        JsonObject tableEntry;
        tableEntry.InsertField(FieldPath{"mainTable"}, FieldType::LEAF_FIELD_STRING,
            FieldValue{.stringValue = tableName}, false);
        if (!maxRowids.empty()) {
            UpdateOrInsertCursorEntry(tableEntry, cursorValues, maxRowids);
        }
        tables.push_back(tableEntry);
    }

    JsonObject newRoot;
    for (const auto &table : tables) {
        newRoot.InsertField(FieldPath{"tables"}, table, true);
    }

    return WriteHwmFile(filePath, newRoot);
}

int DataDonationUtils::LoadRowidHwm(const std::string &dbPath, const std::string &tableName,
    std::vector<std::pair<std::string, int64_t>> &cursorValues,
    std::vector<std::pair<std::string, int64_t>> &maxRowids)
{
    std::string filePath = GetRowidHwmFilePath(dbPath);
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOGE("[DataDonationUtils] Open rowid hwm file failed, errno: %d", errno);
        return -E_INVALID_FILE;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string content = buffer.str();
    if (content.empty()) {
        LOGE("[DataDonationUtils] Rowid hwm file is empty");
        return -E_INVALID_FILE;
    }

    JsonObject root;
    int errCode = root.Parse(content);
    if (errCode != E_OK || !root.IsValid()) {
        LOGE("[DataDonationUtils] Parse rowid hwm file failed");
        return -E_INVALID_FILE;
    }

    std::vector<JsonObject> tables;
    errCode = root.GetObjectArrayByFieldPath(FieldPath{"tables"}, tables);
    if (errCode != E_OK) {
        LOGE("[DataDonationUtils] Get tables array from hwm file failed");
        return -E_INVALID_FILE;
    }

    for (const auto &table : tables) {
        FieldValue nameVal;
        if (table.GetFieldValueByFieldPath(FieldPath{"mainTable"}, nameVal) != E_OK ||
            nameVal.stringValue != tableName) {
            continue;
        }
        ParseCursorFromHwm(table, cursorValues, maxRowids);
        for (const auto &maxRowidEntry : maxRowids) {
            if (maxRowidEntry.first == tableName) {
                return E_OK;
            }
        }
        LOGE("[DataDonationUtils] MaxRowid for table %s not found in cursor entries",
            DBCommon::StringMiddleMasking(tableName).c_str());
        return -E_INVALID_FILE;
    }

    LOGE("[DataDonationUtils] TableName %s not found in hwm file",
        DBCommon::StringMiddleMasking(tableName).c_str());
    return -E_INVALID_ARGS;
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
        if (errCode != -E_SUBSCRIBE_QUERY_END) {
            LOGE("[GetPrimaryKeysFromBinlog] Get search data from binlog err: %d", errCode);
        }
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

bool DataDonationUtils::IsTableInKeyOut(const std::string &tableName,
    const std::vector<DataDonationSchema::DdKeyOut> &keyOut)
{
    for (const auto &key : keyOut) {
        if (key.item.table == tableName) {
            return true;
        }
    }
    return false;
}

bool DataDonationUtils::IsDonationDataEmpty(const VBucket &bucket)
{
    for (auto it = bucket.begin(); it != bucket.end(); it++) {
        if (it->first == CloudDbConstant::SUB_DATA_OP_TYPE) {
            continue;
        }
        if (it->second.index() != TYPE_INDEX<Nil>) {
            return false;
        }
    }
    return true;
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
    std::string changedPkValue = std::to_string(pkValue);
    for (const auto &dataField : data.changedData) {
        for (size_t i = 0; i < dataField.field.size(); i++) {
            if (dataField.field[i] != changedPkValue) {
                continue;
            }
            found = true;
            int64_t opType = static_cast<int64_t>(dataField.opType[i]);
            dataRow.data.insert_or_assign(CloudDbConstant::SUB_DATA_OP_TYPE, opType);
            dataRow.opType = static_cast<int16_t>(opType);
            dataRow.fileIdx = dataField.binlogCursor.first;
            dataRow.cursor = dataField.binlogCursor.second;
        }
        if (found) {
            break;
        }
    }
    return found ? E_OK : -E_NOT_FOUND;
}

std::pair<int, std::shared_ptr<MatrixFile>> DataDonationUtils::MmapMatrixFile(const std::string &path)
{
    std::shared_ptr<MatrixFile> matrixFile = std::make_shared<MatrixFile>();
    int errCode = matrixFile->AcquireWithRetry(path);
    if (errCode != E_OK) {
        LOGE("[MmapMatrixFile] Acquire matrix file err: %d, errno: %d", errCode, errno);
        return std::make_pair(errCode, nullptr);
    }

    errCode = matrixFile->MapMatrixFile();
    if (errCode != E_OK) {
        LOGE("[MmapMatrixFile] Map matrix file err: %d", errCode);
        return std::make_pair(errCode, nullptr);
    }
    return std::make_pair(E_OK, matrixFile);
}

std::vector<uint64_t> DataDonationUtils::GetMatrixTableIndexs(const MatrixFileInfo &matrixFileInfo,
    const std::vector<std::string> &changedData, const MatrixFileUpdateConfig &config)
{
    std::vector<uint64_t> indexList;
    for (const auto &tableName : changedData) {
        auto it = matrixFileInfo.matrixTables.find(tableName);
        if (it == matrixFileInfo.matrixTables.end()) {
            LOGW("[GetMatrixTableIndexs] Table not registered, %s",
                DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
            continue;
        }
        uint64_t index = it->second;
        if (index >= MatrixFile::MAX_SLOT_NUM) {
            LOGW("[GetMatrixTableIndexs] Table index out of range %zu, limit: %zu, table: %s", index,
                MatrixFile::MAX_SLOT_NUM, DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
            continue;
        }
        indexList.push_back(index);
    }

    if (config.isFullSync) {
        if (matrixFileInfo.fullSyncOffset >= MatrixFile::MAX_SLOT_NUM) {
            LOGW("[GetMatrixTableIndexs] isFull offset: %zu out of range %zu",
                matrixFileInfo.fullSyncOffset, MatrixFile::MAX_SLOT_NUM);
        } else {
            indexList.push_back(matrixFileInfo.fullSyncOffset);
        }
    }
    return indexList;
}

int DataDonationUtils::FindMatrixFileInfo(const std::string &hashFileName, MatrixFileInfo &fileInfo)
{
    std::lock_guard<std::mutex> autoLock(g_matrixInfoMutex);
    auto it = g_matrixInfoMap.find(hashFileName);
    if (it == g_matrixInfoMap.end()) {
        return -E_NOT_FOUND;
    }
    fileInfo = it->second;
    return E_OK;
}

int DataDonationUtils::UpdateMatrixFile(const MatrixFileInfo &fileInfo,
    const std::vector<std::string> &changedData, const MatrixFileUpdateConfig &config)
{
    std::vector<uint64_t> indexList = DataDonationUtils::GetMatrixTableIndexs(fileInfo, changedData, config);
    if (indexList.empty()) {
        LOGI("[UpdateMatrixFile] No change, changed data size:%zu, isFull:%d", changedData.size(), config.isFullSync);
        return E_OK;
    }

    {
        std::lock_guard<std::mutex> autoLock(g_matrixOperateMutex);
        auto [errCode, matrixFile] = MmapMatrixFile(fileInfo.matrixFilePath);
        if (matrixFile == nullptr) {
            LOGE("[UpdateMatrixFile] Matrix map ptr is null, err: %d", errCode);
            return errCode;
        }

        errCode = matrixFile->WriteMatrixFile(indexList);
        if (errCode != E_OK) {
            LOGE("[UpdateMatrixFile] Sync to matrix file err: %d", errCode);
            return errCode;
        }
    }
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

    output = dbPath + DBConstant::BINLOG_DIR_POSTFIX + DataDonationUtils::DATA_DONATION_SCHEMA_FILE;
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

    std::string binlogDir = fullName + DBConstant::BINLOG_DIR_POSTFIX;
    int errCode = DBCommon::CreateDirectory(binlogDir);
    if (errCode != E_OK) {
        LOGE("[SaveSubscribeSchema] Create binlog directory failed, errCode: %d", errCode);
        return errCode;
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

bool DataDonationUtils::IsFilePathValid(const std::string &path)
{
    if (path.empty()) {
        LOGE("[IsFilePathValid] Path is empty");
        return false;
    }

    if (path.front() != DBConstant::SEPARATOR) {
        LOGE("[IsFilePathValid] Relative path not allowed");
        return false;
    }

    // continuous separator is not allowed
    bool lastIsSeparator = false;
    for (char ch : path) {
        if (ch == DBConstant::SEPARATOR) {
            if (lastIsSeparator) {
                LOGE("[IsFilePathValid] Duplicate path separator");
                return false;
            }
            lastIsSeparator = true;
        } else {
            lastIsSeparator = false;
        }
    }

    // "." or ".." path segments are not allowed
    size_t start = 1;
    while (start < path.size()) {
        size_t end = path.find(DBConstant::SEPARATOR, start);
        std::string part = path.substr(start, end - start);
        if (part == "." || part == "..") {
            LOGE("[IsFilePathValid] Path contains '.' or '..'");
            return false;
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    if (path.size() > 1 && path.back() == DBConstant::SEPARATOR) {
        LOGE("[IsFilePathValid] Path ends with separator");
        return false;
    }
    return true;
}

int DataDonationUtils::SetTrackerMatrixInfo(sqlite3 *db, const MatrixFileInfo &info)
{
    if (!IsFilePathValid(info.matrixFilePath) || info.matrixTables.empty()) {
        LOGE("[SetTrackerMatrixInfo] Matrix info invalid, path: %s, matrix table size: %zu",
            DBCommon::StringMiddleMaskingWithLen(info.matrixFilePath).c_str(), info.matrixTables.size());
        return -E_INVALID_ARGS;
    }

    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        LOGE("[SetTrackerMatrixInfo] Get db fileName failed.");
        return -E_INVALID_ARGS;
    }

    std::string hashFileName;
    int errCode = DBCommon::GetHashString(fileName, hashFileName);
    if (errCode != E_OK) {
        LOGE("[SetTrackerMatrixInfo] GetHashString err: %d", errCode);
        return -E_INVALID_ARGS;
    }

    {
        std::lock_guard<std::mutex> autoLock(g_matrixInfoMutex);
        g_matrixInfoMap[hashFileName] = info;
    }
    return E_OK;
}

int DataDonationUtils::UnsetTrackerMatrixInfo(sqlite3 *db)
{
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        LOGE("[UnsetTrackerMatrixInfo] Get db filename failed.");
        return -E_INVALID_ARGS;
    }
    std::string hashFileName;
    int errCode = DBCommon::GetHashString(fileName, hashFileName);
    if (errCode != E_OK) {
        LOGE("[UnsetTrackerMatrixInfo] GetHashString err: %d", errCode);
        return -E_INVALID_ARGS;
    }

    std::lock_guard<std::mutex> autoLock(g_matrixInfoMutex);
    g_matrixInfoMap.erase(hashFileName);
    return E_OK;
}

void DataDonationUtils::DataChangedObserver(const char *dbPath, char *tableName)
{
    std::string hashFileName;
    int errCode = DBCommon::GetHashString(dbPath, hashFileName);
    if (errCode != E_OK) {
        LOGE("[DataChangedObserver] GetHashString err: %d", errCode);
        return;
    }

    MatrixFileInfo fileInfo;
    {
        std::lock_guard<std::mutex> autoLock(g_matrixInfoMutex);
        auto it = g_matrixInfoMap.find(hashFileName);
        if (it == g_matrixInfoMap.end()) {
            LOGE("[DataChangedObserver] Matrix file not registered");
            return;
        }
        fileInfo = it->second;
    }

    std::vector<std::string> changedData = {std::string(tableName)};
    MatrixFileUpdateConfig config = {.isFullSync = false};
    errCode = DataDonationUtils::UpdateMatrixFile(fileInfo, changedData, config);
    if (errCode != E_OK) {
        LOGE("[DataChangedObserver] Update matrix file err: %d", errCode);
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

void DataDonationUtils::SetGetSchemaCallback(sqlite3 *db)
{
    if (db == nullptr) {
        LOGE("[SetGetSchemaCallback] db is null");
        return;
    }
    sqlite3_set_json_parse_callback_binlog(db, &DataDonationUtils::BinlogSchemaGet);
    sqlite3_free_json_parse_callback_binlog(db, &DataDonationUtils::FreeMonitorConfig);
}

int DataDonationUtils::GetTableAndColumnName(const JsonObject &jsonValue,
    std::string &tableName, std::string &columnName)
{
    FieldValue tableValue;
    int errCode = jsonValue.GetFieldValueByFieldPath(FieldPath {"tableName"}, tableValue);
    if (errCode != E_OK) {
        LOGE("get table failed %d", errCode);
        return errCode;
    }
    FieldValue columnValue;
    errCode = jsonValue.GetFieldValueByFieldPath(FieldPath {"columnName"}, columnValue);
    if (errCode != E_OK) {
        LOGE("get column failed %d", errCode);
        return errCode;
    }
    tableName = tableValue.stringValue;
    columnName = columnValue.stringValue;
    return E_OK;
}

int DataDonationUtils::InitNewTableEntry(MonitorTableCol &table, const std::string &tableName,
    const std::string &columnName)
{
    char *tableNameCpy = strdup(tableName.c_str());
    if (tableNameCpy == nullptr) {
        LOGE("[InitNewTableEntry] Copy table name err: %d", -E_OUT_OF_MEMORY);
        return -E_OUT_OF_MEMORY;
    }

    size_t colSize = sizeof(char *) * MAX_MONITOR_COLUMN_COUNT;
    char **tableCols = static_cast<char **>(malloc(colSize));
    if (tableCols == nullptr) {
        LOGE("[InitNewTableEntry] Allocate table columns err: %d", -E_OUT_OF_MEMORY);
        free(tableNameCpy);
        return -E_OUT_OF_MEMORY;
    }
    (void)memset_s(tableCols, colSize, 0, colSize);

    tableCols[0] = strdup(columnName.c_str());
    if (tableCols[0] == nullptr) {
        LOGE("[InitNewTableEntry] Copy column name err: %d", -E_OUT_OF_MEMORY);
        free(tableNameCpy);
        free(tableCols);
        return -E_OUT_OF_MEMORY;
    }

    table.tableName = tableNameCpy;
    table.cols = tableCols;
    table.colCount = 1;
    return E_OK;
}

int DataDonationUtils::TryAddColumnToTable(MonitorTableCol &table, const std::string &columnName)
{
    if (table.cols == nullptr) {
        LOGE("[TryAddColumnToTable] Column is null");
        return -E_UNEXPECTED_DATA;
    }

    if (table.colCount >= MAX_MONITOR_COLUMN_COUNT) {
        LOGE("[TryAddColumnToTable] Column count exceed limit: %d, max: %d", table.colCount, MAX_MONITOR_COLUMN_COUNT);
        return -E_LENGTH_ERROR;
    }

    for (int j = 0; j < table.colCount; j++) {
        if (table.cols[j] != nullptr && std::string(table.cols[j]) == columnName) {
            return E_OK;
        }
    }

    table.cols[table.colCount] = strdup(columnName.c_str());
    if (table.cols[table.colCount] == nullptr) {
        LOGE("[TryAddColumnToTable] Copy column name failed.");
        return -E_OUT_OF_MEMORY;
    }

    table.colCount++;
    return E_OK;
}

int DataDonationUtils::AddColumnsToMonitor(const JsonObject &jsonValue,
    MonitorTablesConfig *monitorConfig)
{
    if (monitorConfig == nullptr) {
        LOGE("[AddColumnsToMonitor] Monitor config is null");
        return -E_UNEXPECTED_DATA;
    }
    std::string tableName;
    std::string columnName;
    int errCode = GetTableAndColumnName(jsonValue, tableName, columnName);
    if (errCode != E_OK) {
        LOGE("[AddColumnsToMonitor] Get table and column name err: %d", errCode);
        return errCode;
    }

    // add column if table already exist
    for (int i = 0; i < monitorConfig->tableCount; i++) {
        if (monitorConfig->tables[i].tableName == nullptr) {
            continue;
        }
        if (std::string(monitorConfig->tables[i].tableName) == tableName) {
            return TryAddColumnToTable(monitorConfig->tables[i], columnName);
        }
    }

    if (monitorConfig->tableCount >= MAX_MONITOR_TABLE_COUNT) {
        LOGE("[AddColumnsToMonitor] Table count exceed limit %d", monitorConfig->tableCount);
        return -E_LENGTH_ERROR;
    }

    errCode = InitNewTableEntry(monitorConfig->tables[monitorConfig->tableCount], tableName, columnName);
    if (errCode != E_OK) {
        LOGE("[AddColumnsToMonitor] Init table err: %d", errCode);
        return errCode;
    }
    monitorConfig->tableCount++;
    return E_OK;
}

int DataDonationUtils::ReadJsonConfigFromFile(const std::string &dbPath, std::string &jsonStr)
{
    std::string configPath;
    if (!GetSchemaPathByDbPath(dbPath, configPath)) {
        return -E_INVALID_ARGS;
    }
    std::ifstream file(configPath);
    if (!file.is_open()) {
        LOGE("Failed to open config file: %s", DBCommon::StringMiddleMasking(configPath).c_str());
        return -E_INVALID_FILE;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    jsonStr = buffer.str();
    file.close();
    if (jsonStr.empty()) {
        LOGE("Config file is empty");
        return -E_INVALID_FILE;
    }
    return E_OK;
}

int DataDonationUtils::ExtractJsonObj(const JsonObject &inJsonObject, const std::string &field, JsonObject &out)
{
    FieldType fieldType;
    auto fieldPath = FieldPath {field};
    int errCode = inJsonObject.GetFieldTypeByFieldPath(fieldPath, fieldType);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }
    if (fieldType != FieldType::INTERNAL_FIELD_OBJECT) {
        return -E_INVALID_ARGS;
    }
    errCode = inJsonObject.GetObjectByFieldPath(fieldPath, out);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int DataDonationUtils::ExtractJsonObjArray(const JsonObject &inJsonObject, const std::string &field,
    std::vector<JsonObject> &out)
{
    FieldType fieldType;
    auto fieldPath = FieldPath {field};
    int errCode = inJsonObject.GetFieldTypeByFieldPath(fieldPath, fieldType);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }
    if (fieldType != FieldType::LEAF_FIELD_ARRAY) {
        return -E_INVALID_ARGS;
    }
    errCode = inJsonObject.GetObjectArrayByFieldPath(fieldPath, out);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int DataDonationUtils::ParseSearchConfig(const std::string &jsonStr, JsonObject &searchConfig)
{
    JsonObject object;
    int errCode = object.Parse(jsonStr.c_str());
    if (errCode != E_OK) {
        LOGE("update Parsed failed");
        return errCode;
    }
    errCode = SchemaUtils::ExtractJsonObj(object, "searchConfig", searchConfig);
    if (errCode != E_OK) {
        LOGE("Plz check searchConfig. %d", errCode);
    }
    return errCode;
}

int DataDonationUtils::ExtractTableAndColumnName(const JsonObject &mapping, MonitorTablesConfig *monitorConfig)
{
    JsonObject value;
    int errCode = ExtractJsonObj(mapping, "value", value);
    if (errCode == E_OK) {
        errCode = AddColumnsToMonitor(value, monitorConfig);
        if (errCode != E_OK) {
            LOGE("[ProcessMappings] Add value column to monitor err: %d", errCode);
        }
        return errCode;
    }
    std::vector<JsonObject> values;
    errCode = ExtractJsonObjArray(mapping, "values", values);
    if (errCode == E_OK) {
        for (const auto &valueInner : values) {
            errCode = AddColumnsToMonitor(valueInner, monitorConfig);
            if (errCode != E_OK) {
                LOGD("[ExtractTableAndColumnName] Add values column to monitor err: %d", errCode);
            }
        }
        return errCode;
    }
    errCode = ExtractJsonObjArray(mapping, "value", values);
    if (errCode == E_OK) {
        for (const auto &valueInner : values) {
            errCode = AddColumnsToMonitor(valueInner, monitorConfig);
            if (errCode != E_OK) {
                LOGD("[ExtractTableAndColumnName] Add value vector column to monitor err: %d", errCode);
            }
        }
        return errCode;
    }
    return ExtractFunction(mapping, monitorConfig);
}

int DataDonationUtils::ExtractFunction(const JsonObject &mapping, MonitorTablesConfig *monitorConfig)
{
    JsonObject function;
    int errCode = ExtractJsonObj(mapping, "function", function);
    if (errCode != E_OK) {
        LOGD("[ExtractFunction]function field not found: %d", errCode);
        return errCode;
    }
    std::vector<JsonObject> argLists;
    errCode = ExtractJsonObjArray(function, "argList", argLists);
    if (errCode != E_OK) {
        LOGD("[ExtractFunction] extract array from argList err: %d", errCode);
        return errCode;
    }
    for (const auto &argList : argLists) {
        errCode = AddColumnsToMonitor(argList, monitorConfig);
        if (errCode != E_OK) {
            LOGD("[ExtractFunction] Add argList column to monitor err: %d", errCode);
        }
    }
    return errCode;
}

int DataDonationUtils::ProcessMappings(const JsonObject &part, MonitorTablesConfig *monitorConfig)
{
    std::vector<JsonObject> mappings;
    int errCode = SchemaUtils::ExtractJsonObjArray(part, "mappings", mappings);
    if (errCode != E_OK) {
        LOGE("Plz check mappings. %d", errCode);
        return errCode;
    }
    for (const auto &mapping : mappings) {
        errCode = ExtractTableAndColumnName(mapping, monitorConfig);
        if (errCode != E_OK) {
            LOGD("Extract value field err: %d", errCode);
        }
    }
    return E_OK;
}

int DataDonationUtils::ProcessUTDMapping(const JsonObject &utdMapping, MonitorTablesConfig *monitorConfig)
{
    std::vector<JsonObject> parts;
    int errCode = SchemaUtils::ExtractJsonObjArray(utdMapping, "parts", parts);
    if (errCode != E_OK) {
        LOGE("Plz check parts. %d", errCode);
        return errCode;
    }
    for (const auto &part : parts) {
        (void)ProcessMappings(part, monitorConfig);
    }
    return E_OK;
}

int DataDonationUtils::GetMonitorConfigFromFile(MonitorTablesConfig *monitorConfig, const std::string &dbPath)
{
    if (monitorConfig == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::string jsonStr;
    int errCode = ReadJsonConfigFromFile(dbPath, jsonStr);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject searchConfig;
    errCode = ParseSearchConfig(jsonStr, searchConfig);
    if (errCode != E_OK) {
        return errCode;
    }
    std::vector<JsonObject> utdMappings;
    errCode = SchemaUtils::ExtractJsonObjArray(searchConfig, "UTDMapping", utdMappings);
    if (errCode != E_OK) {
        LOGE("Plz check UTDMapping. %d", errCode);
        return errCode;
    }
    for (const auto &utdMapping : utdMappings) {
        (void)ProcessUTDMapping(utdMapping, monitorConfig);
    }
    return E_OK;
}

MonitorTablesConfig *DataDonationUtils::BinlogSchemaGet(const char *dbPath)
{
    if (dbPath == nullptr) {
        LOGE("[BinlogSchemaGet] db path is null");
        return nullptr;
    }

    LOGI("[BinlogSchemaGet] Start get schema.");
    MonitorTablesConfig *monitorConfig = static_cast<MonitorTablesConfig*>(malloc(sizeof(MonitorTablesConfig)));
    if (monitorConfig == nullptr) {
        LOGE("BinlogSchemaGet: malloc monitorConfig failed");
        return nullptr;
    }
    (void)memset_s(monitorConfig, sizeof(MonitorTablesConfig), 0, sizeof(MonitorTablesConfig));

    monitorConfig->tables = static_cast<MonitorTableCol*>(malloc(MAX_MONITOR_TABLE_COUNT * sizeof(MonitorTableCol)));
    if (monitorConfig->tables == nullptr) {
        LOGE("BinlogSchemaGet: malloc tables failed");
        free(monitorConfig);
        return nullptr;
    }
    (void)memset_s(monitorConfig->tables, MAX_MONITOR_TABLE_COUNT * sizeof(MonitorTableCol), 0,
        MAX_MONITOR_TABLE_COUNT * sizeof(MonitorTableCol));

    int errCode = GetMonitorConfigFromFile(monitorConfig, dbPath);
    if (errCode != E_OK) {
        LOGE("GetMonitorConfigFromFile failed. err=%d", errCode);
        FreeMonitorConfig(monitorConfig);
        return nullptr;
    }
    return monitorConfig;
}

int DataDonationUtils::FreeMonitorConfig(MonitorTablesConfig *monitorConfig)
{
    if (monitorConfig == nullptr) {
        return SQLITE_OK;
    }
    for (int i = 0; i < monitorConfig->tableCount; i++) {
        MonitorTableCol table = monitorConfig->tables[i];
        if (table.cols == nullptr) {
            continue;
        }
        for (int j = 0; j < table.colCount; j++) {
            free(table.cols[j]);
            table.cols[j] = nullptr;
        }
        free(table.cols);
        free(const_cast<char *>(table.tableName));
    }
    free(monitorConfig->tables);
    free(monitorConfig);
    return SQLITE_OK;
}
}   // namespace DistributedDB
#endif