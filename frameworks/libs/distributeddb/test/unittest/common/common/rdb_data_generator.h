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
#ifndef RDB_DATA_GENERATE_H
#define RDB_DATA_GENERATE_H

#include "cloud/cloud_store_types.h"
#include "relational_virtual_device.h"
#include "sqlite_utils.h"
#include "virtual_cloud_db.h"
namespace DistributedDBUnitTest {
class RDBDataGenerator {
public:
    static int InitDatabase(const DistributedDB::DataBaseSchema &schema, sqlite3 &db);

    static DistributedDB::DBStatus InsertCloudDBData(int64_t begin, int64_t count, int64_t gidStart,
        const DistributedDB::DataBaseSchema &schema,
        const std::shared_ptr<DistributedDB::VirtualCloudDb> &virtualCloudDb);

    static std::pair<std::vector<DistributedDB::VBucket>, std::vector<DistributedDB::VBucket>> GenerateDataRecords(
        int64_t begin, int64_t count, int64_t gidStart, const std::vector<DistributedDB::Field> &fields);

    static int InsertLocalDBData(int64_t begin, int64_t count, sqlite3 *db,
        const DistributedDB::DataBaseSchema &schema);

    static int InsertLocalDBData(int64_t begin, int64_t count, sqlite3 *db,
        const DistributedDB::TableSchema &schema);

    static int UpsertLocalDBData(int64_t begin, int64_t count, sqlite3 *db,
        const DistributedDB::TableSchema &schema);

    static int UpdateLocalDBData(int64_t begin, int64_t count, sqlite3 *db,
        const DistributedDB::TableSchema &schema);

    static int InsertVirtualLocalDBData(int64_t begin, int64_t count, DistributedDB::RelationalVirtualDevice *device,
        const DistributedDB::TableSchema &schema);

    static DistributedDB::DistributedSchema ParseSchema(const DistributedDB::DataBaseSchema &schema,
        bool syncOnlyPk = false);

    static int PrepareVirtualDeviceEnv(const std::string &tableName, sqlite3 *db,
        DistributedDB::RelationalVirtualDevice *device);

    static int PrepareVirtualDeviceEnv(const std::string &scanTable, const std::string &expectTable, sqlite3 *db,
        DistributedDB::RelationalVirtualDevice *device);

    static DistributedDB::TableSchema FlipTableSchema(const DistributedDB::TableSchema &origin);
private:
    static std::string GetTypeText(int type);

    static void FillColValueByType(int64_t index, const DistributedDB::Field &field, DistributedDB::VBucket &vBucket);

    static DistributedDB::Type GetColValueByType(int64_t index, const DistributedDB::Field &field);

    static DistributedDB::Asset GenerateAsset(int64_t index, const DistributedDB::Field &field);

    static DistributedDB::Assets GenerateAssets(int64_t index, const DistributedDB::Field &field);

    static int BindOneRowStmt(int64_t index, sqlite3_stmt *stmt, int cid, bool withoutPk,
        const std::vector<DistributedDB::Field> &fields);

    static int BindOneRowUpdateStmt(int64_t index, sqlite3_stmt *stmt, int cid,
        const std::vector<DistributedDB::Field> &fields);

    static int BindOneColStmt(int cid, sqlite3_stmt *stmt, const DistributedDB::Type &type);

    static std::string GetUpsertSQL(const DistributedDB::TableSchema &schema);

    static std::string GetUpdateSQL(const DistributedDB::TableSchema &schema);

    static void FillTypeIntoDataValue(const DistributedDB::Field &field, const DistributedDB::Type &type,
        DistributedDB::VirtualRowData &virtualRow);
};
}
#endif // RDB_DATA_GENERATE_H
