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
#include "sqlite3sym.h"

namespace DistributedDB {

struct DonateDataCursor {
    uint64_t cursor = 0;
    int64_t cloudOpType = 0;
};

struct DonateDataField {
    VBucket field;
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
    static int GetTrackerMatrixInfo(const std::string dbPath, const MatrixFileInfo &info);
    static uint64_t *MmapMatrixFile(const std::string &path, size_t mmapSize, int32_t &fileFd);
    static int UpdateMatrixFile(const MatrixFileInfo &fileInfo, const std::vector<std::string> &changedData,
        const MatrixFileUpdateConfig &config);

    static int MapCloudOpType(int opType, uint32_t &cloudOpType);

    static bool EndsWith(const std::string &str, const std::string &suffix);
    static bool GetSchemaPathByDbPath(const std::string &dbPath, std::string &output);

    static void FilterNonOutputKeys(DdData &dataRow, const std::vector<DataDonationSchema::DdKeyOut> &keyOut);

private:
    static std::string JoinPrimaryKey(const std::vector<DonateDataField> &changedData);

    static std::string BuildWhereClause(const std::string &tableName, const BinlogChangedData &changedData);

    static int GetPrimaryKeysFromBinlog(sqlite3* db, DataDonationSchema &schema,
        std::unordered_map<std::string, BinlogChangedData> &changedDatas);

    static std::string GetSelectFieldName(const std::string &tableName, const std::string &columnName);

    static std::vector<uint64_t> GetMatrixTableIndexs(const MatrixFileInfo &matrixFileInfo,
        const std::vector<std::string> &changedData);

    static void DataChangedObserver(const char *dbPath, char *tableName);

    static bool GetDbFileName(sqlite3 *db, std::string &fileName);
    static int GetHashString(const std::string &str, std::string &dst);

    static std::string GenerateSqlByTableName(const std::string &tableName,
        const DataDonationSchema::DdRelationsPath &path, const BinlogChangedData &changedData);

    static BinlogChangedData *EnsureTableInChangedDatas(DataDonationSchema &schema,
        std::unordered_map<std::string, BinlogChangedData> &changedDatas, const std::string &tableName);

    static void ExtractPkValueFromRow(const BinlogSearchResult &row, const std::string &pkColumn,
        uint32_t cloudOpType, const std::pair<int, uint64_t> &batchCursor, BinlogChangedData &changedData);

    static constexpr const char *DATA_DONATION_SCHEMA_FILE = "data_donation_schema.json";
};

}
#endif  // RELATIONAL_STORE
#endif  // DATA_DONATION_UTILS_H

