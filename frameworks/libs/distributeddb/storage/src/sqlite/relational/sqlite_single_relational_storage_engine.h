/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef SQLITE_RELATIONAL_ENGINE_H
#define SQLITE_RELATIONAL_ENGINE_H
#ifdef RELATIONAL_STORE

#include "macro_utils.h"
#include "relationaldb_properties.h"
#include "sqlite_relational_utils.h"
#include "sqlite_storage_engine.h"
#include "sqlite_single_ver_relational_storage_executor.h"
#include "tracker_table.h"

namespace DistributedDB {
enum class GenLogTaskStatus {
    IDLE = 0,
    RUNNING,
    RUNNING_BEFORE_SYNC,
    RUNNING_APPENDED,
    INTERRUPTED,
    DB_CLOSED,
};

// Once waiting time for log generation task before cloud sync
constexpr std::chrono::milliseconds SYNC_WAIT_GEN_LOG_ONCE_TIME = std::chrono::milliseconds(10 * 1000);
// Max waiting time for log generation task before cloud sync
constexpr std::chrono::milliseconds SYNC_WAIT_GEN_LOG_MAX_TIME = std::chrono::milliseconds(20 * 60 * 1000);

struct GenerateLogInfo {
    std::string tableName;
    std::string identity;
};

struct CreateDistributedTableParam {
    bool isTrackerSchemaChanged = false;
    TableSyncType syncType = TableSyncType::DEVICE_COOPERATION;
    std::string tableName;
    std::string identity;
    std::optional<TableSchema> cloudTable;
    bool isAsync = false;
};

class SQLiteSingleRelationalStorageEngine : public SQLiteStorageEngine {
public:
    explicit SQLiteSingleRelationalStorageEngine(RelationalDBProperties properties);
    ~SQLiteSingleRelationalStorageEngine() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleRelationalStorageEngine);

    void SetSchema(const RelationalSchemaObject &schema);

    RelationalSchemaObject GetSchema() const;

    int CreateDistributedTable(const CreateDistributedTableParam &param, bool &schemaChanged);

    int CleanDistributedDeviceTable(std::vector<std::string> &missingTables);

    const RelationalDBProperties &GetProperties() const;
    const RelationalDBProperties GetRelationalProperties() const;
    void SetProperties(const RelationalDBProperties &properties);

    int SetTrackerTable(const TrackerSchema &schema, const TableInfo &tableInfo, bool isFirstCreate);
    void CacheTrackerSchema(const TrackerSchema &schema);
    int GetOrInitTrackerSchemaFromMeta();
    int SaveTrackerSchema(const std::string &tableName, bool isFirstCreate);

    int ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records);
    RelationalSchemaObject GetTrackerSchema() const;
    void SetTrackerSchema(const RelationalSchemaObject &trackerSchema);
    int CleanTrackerData(const std::string &tableName, int64_t cursor);

    int SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty,
        SQLiteSingleVerRelationalStorageExecutor *handle, std::set<std::string> &clearWaterMarkTables,
        RelationalSchemaObject &schema);
    int UpgradeSharedTable(const DataBaseSchema &cloudSchema, const std::vector<std::string> &deleteTableNames,
        const std::map<std::string, std::vector<Field>> &updateTableNames,
        const std::map<std::string, std::string> &alterTableNames);
    std::pair<std::vector<std::string>, int> CalTableRef(const std::vector<std::string> &tableNames,
        const std::map<std::string, std::string> &sharedTableOriginNames);
    int UpdateExtendField(const TrackerSchema &schema);

    std::pair<int, bool> SetDistributedSchema(const DistributedSchema &schema, const std::string &localIdentity,
        bool isForceUpgrade);

    void ResetGenLogTaskStatus();
    void StopGenLogTask(bool isCloseDb = false);
    int StopGenLogTaskWithTables(const std::vector<std::string> &tables);
    bool IsNeedStopWaitGenLogTask(const std::vector<std::string> &tables,
        std::vector<std::pair<int, std::string>> &asyncGenLogTasks, int &errCode);
    int WaitAsyncGenLogTaskFinished(const std::vector<std::string> &tables, const std::string &identity);
    int GetAsyncGenLogTasks(std::vector<std::pair<int, std::string>> &asyncGenLogTasks);
    int GetAsyncGenLogTasksWithTables(const std::set<std::string> &tables,
        std::vector<std::pair<int, std::string>> &tasks);

    std::pair<int, TableInfo> AnalyzeTable(const std::string &tableName);

#ifdef USE_DISTRIBUTEDDB_CLOUD
    int PutCloudGid(const std::string &tableName, std::vector<VBucket> &data);
#endif
protected:
    StorageExecutor *NewSQLiteStorageExecutor(sqlite3 *dbHandle, bool isWrite, bool isMemDb) override;
    int Upgrade(sqlite3 *db) override;
    int CreateNewExecutor(bool isWrite, StorageExecutor *&handle) override;
private:
    // For executor.
    void ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&handle, bool isExternal = false);

    // For db.
    int RegisterFunction(sqlite3 *db) const;

    int UpgradeDistributedTable(const TableInfo &tableInfo, bool &schemaChanged, TableSyncType syncType);

    int CreateDistributedTable(SQLiteSingleVerRelationalStorageExecutor *&handle, bool isUpgraded,
        const CreateDistributedTableParam &param, TableInfo &table, RelationalSchemaObject &schema);

    int CreateDistributedTable(const CreateDistributedTableParam &param, bool isUpgraded,
        RelationalSchemaObject &schema);

    int CreateDistributedSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle, const std::string &tableName,
        const std::string &sharedTableName, TableSyncType syncType, RelationalSchemaObject &schema);

    int CleanTrackerDeviceTable(const std::vector<std::string> &tableNames, RelationalSchemaObject &trackerSchemaObj,
        SQLiteSingleVerRelationalStorageExecutor *&handle);

    void SetGenLogTaskStatus(GenLogTaskStatus status);
    int GetAsyncGenLogTasks(const SQLiteSingleVerRelationalStorageExecutor *handle,
        std::vector<std::pair<int, std::string>> &asyncGenLogTasks);
    int AddAsyncGenLogTask(const SQLiteSingleVerRelationalStorageExecutor *handle,
        const std::string &tableName);
    int GenCloudLogInfo(const std::string &identity);
    int GenCloudLogInfoWithTables(const std::string &identity,
        const std::vector<std::pair<int, std::string>> &taskTables);
    int GenCloudLogInfoIfNeeded(const std::string &tableName, const std::string &identity);
    int GenLogInfoForUpgrade(const std::string &tableName, bool schemaChanged);
    int GeneLogInfoForExistedDataInBatch(const std::string &identity, const TableInfo &tableInfo,
        std::unique_ptr<SqliteLogTableManager> &logMgrPtr);
    int GeneLogInfoForExistedData(const std::string &identity, const TableInfo &tableInfo,
        std::unique_ptr<SqliteLogTableManager> &logMgrPtr, uint32_t limitNum, int &changedCount);
    int TriggerGenLogTask(const std::string &identity);
    int RemoveAsyncGenLogTask(int taskId);
    bool IsNeedStopGenLogTask();

    static std::map<std::string, std::map<std::string, bool>> GetReachableWithShared(
        const std::map<std::string, std::map<std::string, bool>> &reachableReference,
        const std::map<std::string, std::string> &tableToShared);

    static std::map<std::string, int> GetTableWeightWithShared(const std::map<std::string, int> &tableWeight,
        const std::map<std::string, std::string> &tableToShared);

    int UpgradeSharedTableInner(SQLiteSingleVerRelationalStorageExecutor *&handle,
        const DataBaseSchema &cloudSchema, const std::vector<std::string> &deleteTableNames,
        const std::map<std::string, std::vector<Field>> &updateTableNames,
        const std::map<std::string, std::string> &alterTableNames);

    int DoDeleteSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
        const std::vector<std::string> &deleteTableNames, RelationalSchemaObject &schema);

    int DoUpdateSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
        const std::map<std::string, std::vector<Field>> &updateTableNames, const DataBaseSchema &cloudSchema,
        RelationalSchemaObject &localSchema);

    int DoAlterSharedTableName(SQLiteSingleVerRelationalStorageExecutor *&handle,
        const std::map<std::string, std::string> &alterTableNames, RelationalSchemaObject &schema);

    int DoCreateSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
        const DataBaseSchema &cloudSchema, const std::map<std::string, std::vector<Field>> &updateTableNames,
        const std::map<std::string, std::string> &alterTableNames, RelationalSchemaObject &schema);

    int UpdateKvData(SQLiteSingleVerRelationalStorageExecutor *&handle,
        const std::map<std::string, std::string> &alterTableNames);

    int CheckIfExistUserTable(SQLiteSingleVerRelationalStorageExecutor *&handle, const DataBaseSchema &cloudSchema,
        const std::map<std::string, std::string> &alterTableNames, const RelationalSchemaObject &schema);

    int SetDistributedSchemaInner(RelationalSchemaObject &schemaObj, const DistributedSchema &schema,
        const std::string &localIdentity, bool isForceUpgrade);

    int SetDistributedSchemaInTraction(RelationalSchemaObject &schemaObj, const DistributedSchema &schema,
        const std::string &localIdentity, bool isForceUpgrade, SQLiteSingleVerRelationalStorageExecutor &handle);

    RelationalSchemaObject schema_;
    RelationalSchemaObject trackerSchema_;
    mutable std::mutex schemaMutex_;
    mutable std::mutex trackerSchemaMutex_;

    RelationalDBProperties properties_;
    std::mutex createDistributedTableMutex_;
    mutable std::mutex propertiesMutex_;

    mutable std::mutex genLogTaskStatusMutex_;
    GenLogTaskStatus genLogTaskStatus_ = GenLogTaskStatus::IDLE;
    mutable std::mutex genLogTaskCvMutex_;
    std::condition_variable genLogTaskCv_;
};
} // namespace DistributedDB
#endif
#endif // SQLITE_RELATIONAL_ENGINE_H