/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef SQLITE_CLOUD_KV_EXECUTOR_UTILS_H
#define SQLITE_CLOUD_KV_EXECUTOR_UTILS_H

#include "cloud/cloud_db_types.h"

#include "cloud/cloud_db_types.h"
#include "cloud/cloud_upload_recorder.h"
#include "data_transformer.h"
#include "icloud_sync_storage_interface.h"
#include "sqlite_single_ver_continue_token.h"
#include "sqlite_utils.h"

namespace DistributedDB {
class SqliteCloudKvExecutorUtils {
public:
    using DBParam = std::pair<sqlite3 *, bool>;
    using FillGidParam = std::pair<sqlite3 *, bool>;
    static int GetCloudData(const CloudSyncConfig &config, const DBParam &param, const CloudUploadRecorder &recorder,
        SQLiteSingleVerContinueToken &token, CloudSyncData &data);

    static std::pair<int, DataInfoWithLog> GetLogInfo(sqlite3 *db, bool isMemory, const VBucket &cloudData);

    static int PutCloudData(sqlite3 *db, bool isMemory, DownloadData &downloadData);

    static int FillCloudLog(const FillGidParam &param, OpType opType, const CloudSyncData &data,
        const std::string &user, CloudUploadRecorder &recorder);

    static std::pair<int, int64_t> CountCloudData(sqlite3 *db, bool isMemory, const Timestamp &timestamp,
        const std::string &user, bool forcePush);

    static std::pair<int, int64_t> CountAllCloudData(const DBParam &param, const std::vector<Timestamp> &timestampVec,
        const std::string &user, bool forcePush, QuerySyncObject &querySyncObject);

    static std::pair<int, CloudSyncData> GetLocalCloudVersion(sqlite3 *db, bool isMemory, const std::string &user);

    static int GetCloudVersionFromCloud(sqlite3 *db, bool isMemory, const std::string &user,
        const std::string &device, std::vector<VBucket> &dataVector);
private:
    using UploadDetail = std::pair<uint32_t, uint32_t>;
    static int GetCloudDataForSync(const CloudSyncConfig &config, const CloudUploadRecorder &recorder,
        sqlite3_stmt *statement, CloudSyncData &cloudDataResult, UploadDetail &detail);

    static void GetCloudLog(sqlite3_stmt *stmt, VBucket &logInfo, uint32_t &totalSize);

    static void GetCloudExtraLog(sqlite3_stmt *stmt, VBucket &flags);

    static int GetCloudKvData(sqlite3_stmt *stmt, VBucket &data, uint32_t &totalSize);

    static int GetCloudKvBlobData(const std::string &keyStr, int index, sqlite3_stmt *stmt,
        VBucket &data, uint32_t &totalSize);

    static int CheckIgnoreData(const CloudSyncConfig &config, VBucket &data, VBucket &flags);

    static std::pair<int, sqlite3_stmt*> GetLogInfoStmt(sqlite3 *db, const VBucket &cloudData, bool existKey);

    static std::pair<int, DataInfoWithLog> GetLogInfoInner(sqlite3_stmt *stmt, bool isMemory, const std::string &gid,
        const Bytes &key);

    static DataInfoWithLog FillLogInfoWithStmt(sqlite3_stmt *stmt);

    static int ExecutePutCloudData(sqlite3 *db, bool isMemory, DownloadData &downloadData,
        std::map<int, int> &statisticMap);

    static int OperateCloudData(sqlite3 *db, bool isMemory, int index, OpType opType,
        DownloadData &downloadData);

    static std::string GetOperateDataSql(OpType opType);

    static std::string GetOperateLogSql(OpType opType);

    static OpType TransToOpType(const CloudWaterType type);

    static int BindOnlyUpdateLogStmt(sqlite3_stmt *logStmt, const std::string &user, const DataItem &dataItem);

    static int BindStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, int index, OpType opType,
        DownloadData &downloadData);

    static int BindInsertStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, const std::string &user,
        const DataItem &dataItem);

    static int BindInsertLogStmt(sqlite3_stmt *logStmt, const std::string &user,
        const DataItem &dataItem);

    static int BindUpdateStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, const std::string &user,
        const DataItem &dataItem);

    static int BindUpdateLogStmt(sqlite3_stmt *logStmt, const std::string &user, const DataItem &dataItem);

    static int BindDeleteStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, const std::string &user,
        DataItem &dataItem);

    static int BindDataStmt(sqlite3_stmt *dataStmt, const DataItem &dataItem, bool isInsert);

    static int BindSyncDataStmt(sqlite3_stmt *dataStmt, const DataItem &dataItem, bool isInsert, int &index);

    static int BindCloudDataStmt(sqlite3_stmt *dataStmt, const DataItem &dataItem, int &index);

    static int StepStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, bool isMemory);

    static int OnlyUpdateLogTable(sqlite3 *db, bool isMemory, int index, OpType op, DownloadData &downloadData);

    static int OnlyUpdateSyncData(sqlite3 *db, bool isMemory, int index, OpType opType, DownloadData &downloadData);

    static int BindUpdateSyncDataStmt(sqlite3_stmt *dataStmt, int index, OpType opType, DownloadData &downloadData);

    static int BindUpdateTimestampStmt(sqlite3_stmt *dataStmt, int index, DownloadData &downloadData);

    static int FillCloudGid(const FillGidParam &param, const CloudSyncBatch &data, const std::string &user,
        const CloudWaterType &type, CloudUploadRecorder &recorder);

    static std::pair<int, DataItem> GetDataItem(int index, DownloadData &downloadData);

    static std::pair<int, int64_t> CountCloudDataInner(sqlite3 *db, bool isMemory, const Timestamp &timestamp,
        const std::string &user, std::string &sql);

    static int FillCloudVersionRecord(sqlite3 *db, OpType opType, const CloudSyncData &data);

    static std::pair<int, CloudSyncData> GetLocalCloudVersionInner(sqlite3 *db, bool isMemory,
        const std::string &user);

    static int GetCloudVersionRecord(bool isMemory, sqlite3_stmt *stmt, CloudSyncData &syncData);

    static void InitDefaultCloudVersionRecord(const std::string &key, const std::string &dev, CloudSyncData &syncData);

    static int BindVersionStmt(const std::string &device, const std::string &user, sqlite3_stmt *dataStmt);

    static int GetCloudVersionRecordData(sqlite3_stmt *stmt, VBucket &data, uint32_t &totalSize);

    static Timestamp GetMaxTimeStamp(std::vector<VBucket> &dataExtend);

    static bool UpdateBeginTimeForMemoryDB(SQLiteSingleVerContinueToken &token, CloudSyncData &data);

    static int BindFillGidLogStmt(sqlite3_stmt *logStmt, const std::string &user,
        const DataItem &dataItem, const VBucket &uploadExtend, const CloudWaterType &type);
private:
    static void MarkUploadSuccess(const FillGidParam &param, const CloudSyncBatch &data, const std::string &user,
        size_t dataIndex);

    static bool CheckDataChanged(const FillGidParam &param, const CloudSyncBatch &data, size_t dataIndex);

    static void MarkUploadSuccessInner(const FillGidParam &param, const CloudSyncBatch &data, const std::string &user,
        size_t dataIndex);
};
}
#endif // SQLITE_CLOUD_KV_EXECUTOR_UTILS_H