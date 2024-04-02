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
#include "sqlite_storage_engine.h"
#include "sqlite_single_ver_relational_storage_executor.h"
#include "tracker_table.h"

namespace DistributedDB {
class SQLiteSingleRelationalStorageEngine : public SQLiteStorageEngine {
public:
    explicit SQLiteSingleRelationalStorageEngine(RelationalDBProperties properties);
    ~SQLiteSingleRelationalStorageEngine() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleRelationalStorageEngine);

    void SetSchema(const RelationalSchemaObject &schema);

    RelationalSchemaObject GetSchema() const;

    int CreateDistributedTable(const std::string &tableName, const std::string &identity, bool &schemaChanged,
        TableSyncType syncType, bool trackerSchemaChanged);

    int CleanDistributedDeviceTable(std::vector<std::string> &missingTables);

    const RelationalDBProperties &GetProperties() const;
    void SetProperties(const RelationalDBProperties &properties);

    int SetTrackerTable(const TrackerSchema &schema);
    int CheckAndCacheTrackerSchema(const TrackerSchema &schema, TableInfo &tableInfo, bool &isFirstCreate);
    int GetOrInitTrackerSchemaFromMeta();
    int SaveTrackerSchema(const std::string &tableName, bool isFirstCreate);

    int ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records);
    RelationalSchemaObject GetTrackerSchema() const;
    int CleanTrackerData(const std::string &tableName, int64_t cursor);

    int SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty,
        SQLiteSingleVerRelationalStorageExecutor *handle, std::set<std::string> &clearWaterMarkTables,
        RelationalSchemaObject &schema);
    int UpgradeSharedTable(const DataBaseSchema &cloudSchema, const std::vector<std::string> &deleteTableNames,
        const std::map<std::string, std::vector<Field>> &updateTableNames,
        const std::map<std::string, std::string> &alterTableNames);
    std::pair<std::vector<std::string>, int> CalTableRef(const std::vector<std::string> &tableNames,
        const std::map<std::string, std::string> &sharedTableOriginNames);
protected:
    StorageExecutor *NewSQLiteStorageExecutor(sqlite3 *dbHandle, bool isWrite, bool isMemDb) override;
    int Upgrade(sqlite3 *db) override;
    int CreateNewExecutor(bool isWrite, StorageExecutor *&handle) override;
private:
    // For executor.
    int ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&handle);

    // For db.
    int RegisterFunction(sqlite3 *db) const;

    int UpgradeDistributedTable(const std::string &tableName, bool &schemaChanged, TableSyncType syncType);

    int CreateDistributedTable(SQLiteSingleVerRelationalStorageExecutor *&handle, bool isUpgraded,
        const std::string &identity, TableInfo &table, RelationalSchemaObject &schema);

    int CreateDistributedTable(const std::string &tableName, bool isUpgraded, const std::string &identity,
        RelationalSchemaObject &schema, TableSyncType tableSyncType);

    int CreateDistributedSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle, const std::string &tableName,
        const std::string &sharedTableName, TableSyncType syncType, RelationalSchemaObject &schema);

    int CreateRelationalMetaTable(sqlite3 *db);

    int CleanTrackerDeviceTable(const std::vector<std::string> &tableNames, RelationalSchemaObject &trackerSchemaObj,
        SQLiteSingleVerRelationalStorageExecutor *&handle);

    int GenLogInfoForUpgrade(const std::string &tableName, RelationalSchemaObject &schema, bool schemaChanged);

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

    RelationalSchemaObject schema_;
    RelationalSchemaObject trackerSchema_;
    mutable std::mutex schemaMutex_;

    RelationalDBProperties properties_;
    std::mutex createDistributedTableMutex_;
};
} // namespace DistributedDB
#endif
#endif // SQLITE_RELATIONAL_ENGINE_H