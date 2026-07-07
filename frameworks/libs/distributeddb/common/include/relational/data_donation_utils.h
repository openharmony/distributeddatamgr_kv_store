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
#ifndef DATA_DONATION_UTILS_H
#define DATA_DONATION_UTILS_H

#ifdef RELATIONAL_STORE
#include <unordered_set>

#include "cloud/cloud_store_types.h"
#include "data_donation_types.h"
#include "data_donation_schema.h"
#include "matrix_file.h"
#include "sqlite3sym.h"

namespace DistributedDB {

struct DonateDataCursor {
    uint64_t cursor = 0;
    int64_t cloudOpType = 0;
};

struct DonateDataField {
    std::vector<std::string> field;
    std::vector<int> colType;
    std::vector<uint32_t> opType;
    std::pair<int, uint64_t> binlogCursor;
};

struct BinlogChangedData {
    std::string tableName;
    std::string pkColumn;
    std::vector<DonateDataField> changedData;
};

constexpr const char *FILE_INDEX = "fileIndex";
constexpr const char *READ_POS = "readPos";

constexpr const int OP_TYPE_NUM = 3;
constexpr struct {
    int opType;
    int64_t cloudOpType;
} OP_TYPE_MAPPING[] = {
    {SQLITE_INSERT, static_cast<int64_t>(SubDataOpType::OP_INSERT)},
    {SQLITE_UPDATE, static_cast<int64_t>(SubDataOpType::OP_UPDATE)},
    {SQLITE_DELETE, static_cast<int64_t>(SubDataOpType::OP_DELETE)},
};

class DataDonationUtils {
public:
    static int GenerateQuerySql(sqlite3* db, DataDonationSchema &schema,
        std::unordered_map<std::string, BinlogChangedData> &changedDatas,
        std::unordered_map<std::string, std::string> &sqls);
    static int GetCursorByPkColumn(const VBucket &bucket, const BinlogChangedData &data, DdData &dataRow);

    static int SaveSubscribeSchema(sqlite3 *db, const std::string &schema);

    static std::string GetFieldName(const std::string &tableName, const std::string &columnName);

    static void SetDataChangedObserver(sqlite3 *db);
    static int SetTrackerMatrixInfo(sqlite3 *db, const MatrixFileInfo &info);
    static int UnsetTrackerMatrixInfo(sqlite3 *db);
    static int FindMatrixFileInfo(const std::string &hashFileName, MatrixFileInfo &fileInfo);
    static std::pair<int, std::shared_ptr<MatrixFile>> MmapMatrixFile(const std::string &path);
    static int UpdateMatrixFile(const MatrixFileInfo &fileInfo, const std::vector<std::string> &changedData,
        const MatrixFileUpdateConfig &config);

    static int MapCloudOpType(int opType, uint32_t &cloudOpType);

    static bool EndsWith(const std::string &str, const std::string &suffix);
    static bool GetSchemaPathByDbPath(const std::string &dbPath, std::string &output);

    static void FilterNonOutputKeys(DdData &dataRow, const std::vector<DataDonationSchema::DdKeyOut> &keyOut);

    static bool IsTableInKeyOut(const std::string &tableName, const std::vector<DataDonationSchema::DdKeyOut> &keyOut);

    static bool IsDonationDataEmpty(const VBucket &bucket);

    static MonitorTablesConfig *BinlogSchemaGet(const char *dbPath);

    static int FreeMonitorConfig(MonitorTablesConfig *monitorConfig);

    static void SetGetSchemaCallback(sqlite3 *db);

    static Type ConvertStrToType(const std::string &str, int colType);

    static int CheckBinlogDirExist(const std::string &dbPath);
    static std::string GetRowidHwmFilePath(const std::string &dbPath);
    static int SaveRowidHwm(const std::string &dbPath, const std::string &tableName,
        const std::vector<std::pair<std::string, int64_t>> &cursorValues = {},
        const std::vector<std::pair<std::string, int64_t>> &maxRowids = {});
    static int LoadRowidHwm(const std::string &dbPath, const std::string &tableName, int64_t &maxRowid,
        std::vector<std::pair<std::string, int64_t>> &cursorValues,
        std::vector<std::pair<std::string, int64_t>> &maxRowids);

private:
    static std::string JoinPrimaryKey(const std::vector<DonateDataField> &changedData);

    static std::string BuildWhereClause(const std::string &tableName, const BinlogChangedData &changedData);

    static int GetPrimaryKeysFromBinlog(sqlite3* db, DataDonationSchema &schema,
        std::unordered_map<std::string, BinlogChangedData> &changedDatas);

    static std::string GetSelectFieldName(const std::string &tableName, const std::string &columnName);

    static std::vector<uint64_t> GetMatrixTableIndexs(const MatrixFileInfo &matrixFileInfo,
        const std::vector<std::string> &changedData, const MatrixFileUpdateConfig &config);
    
    static bool IsFilePathValid(const std::string &path);

    static void DataChangedObserver(const char *dbPath, char *tableName);

    static bool GetDbFileName(sqlite3 *db, std::string &fileName);

    static std::string GenerateSqlByTableName(const std::string &tableName,
        const DataDonationSchema::DdRelationsPath &path, const BinlogChangedData &changedData);

    static BinlogChangedData *EnsureTableInChangedDatas(DataDonationSchema &schema,
        std::unordered_map<std::string, BinlogChangedData> &changedDatas, const std::string &tableName);

    static void ExtractPkValueFromRow(const BinlogSearchResult &row, const std::string &pkColumn,
        uint32_t cloudOpType, const std::pair<int, uint64_t> &batchCursor, BinlogChangedData &changedData);

    static int GetTableAndColumnName(const JsonObject &jsonValue, std::string &tableName, std::string &columnName);
    
    static int InitNewTableEntry(MonitorTableCol &table, const std::string &tableName, const std::string &columnName);

    static int TryAddColumnToTable(MonitorTableCol &table, const std::string &columnName);

    static int AddColumnsToMonitor(const JsonObject &jsonValue, MonitorTablesConfig *monitorConfig);

    static int ReadJsonConfigFromFile(const std::string &dbPath, std::string &jsonStr);

    static int ParseSearchConfig(const std::string &jsonStr, JsonObject &searchConfig);

    static int ProcessMappings(const JsonObject &part, MonitorTablesConfig *monitorConfig);

    static int ProcessUTDMapping(const JsonObject &utdMapping, MonitorTablesConfig *monitorConfig);

    static int ExtractTableAndColumnName(const JsonObject &mapping, MonitorTablesConfig *monitorConfig);

    static int ExtractFunction(const JsonObject &mapping, MonitorTablesConfig *monitorConfig);

    static int GetMonitorConfigFromFile(MonitorTablesConfig *monitorConfig, const std::string &dbPath);

    static int ParseHwmFile(const std::string &filePath, JsonObject &root);
    static int UpdateOrInsertCursorEntry(JsonObject &tableEntry,
        const std::vector<std::pair<std::string, int64_t>> &cursorValues,
        const std::vector<std::pair<std::string, int64_t>> &maxRowids);
    static int ParseCursorFromHwm(const JsonObject &tableEntry,
        std::vector<std::pair<std::string, int64_t>> &cursorValues,
        std::vector<std::pair<std::string, int64_t>> &maxRowids);
    static int WriteHwmFile(const std::string &filePath, const JsonObject &root);

    static constexpr const char *DATA_DONATION_SCHEMA_FILE = "data_donation_schema.json";
    static constexpr const char *ROWID_HWM_FILE = "subscribe_rowid_hwm.json";
};

}
#endif  // RELATIONAL_STORE
#endif  // DATA_DONATION_UTILS_H

