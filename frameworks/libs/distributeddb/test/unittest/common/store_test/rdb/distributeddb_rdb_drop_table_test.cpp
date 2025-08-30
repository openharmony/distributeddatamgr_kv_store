/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "rdb_general_ut.h"
#include "sqlite_relational_utils.h"
#include "virtual_sqlite_relational_store.h"
namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBRDBDropTableTest : public RDBGeneralUt {
public:
    void SetUp() override;
protected:
    void PrepareEnv(const StoreInfo &info, bool createTracker = false, bool createDistributedTable = false);
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtDateBaseSchemaInfo GetUniqueSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table, bool isPkUuid);
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBDropTableTest::SetUp()
{
    RDBGeneralUt::SetUp();
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
}

void DistributedDBRDBDropTableTest::PrepareEnv(const StoreInfo &info, bool createTracker, bool createDistributedTable)
{
    ASSERT_EQ(ExecuteSQL("CREATE TABLE IF NOT EXISTS"
        " DEVICE_SYNC_TABLE(id INTEGER PRIMARY KEY AUTOINCREMENT, uuid INTEGER UNIQUE, value INTEGER)", info), E_OK);
    if (createTracker) {
        ASSERT_EQ(SetTrackerTables(info, {DEVICE_SYNC_TABLE}), E_OK);
    }
    if (createDistributedTable) {
        ASSERT_EQ(SetDistributedTables(info, {DEVICE_SYNC_TABLE}), E_OK);
    }
}

UtDateBaseSchemaInfo DistributedDBRDBDropTableTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE, false));
    return info;
}

UtDateBaseSchemaInfo DistributedDBRDBDropTableTest::GetUniqueSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE, true));
    return info;
}

UtTableSchemaInfo DistributedDBRDBDropTableTest::GetTableSchema(const std::string &table, bool isPkUuid)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo field;
    field.field.type = TYPE_INDEX<int64_t>;
    field.field.primary = isPkUuid;
    field.field.colName = "uuid";
    tableSchema.fieldInfo.push_back(field);
    field.field.primary = false;
    field.field.colName = "value";
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

/**
 * @tc.name: SyncAfterDrop001
 * @tc.desc: Test sync success after drop table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, SyncAfterDrop001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create table and set distributed tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    SetSchemaInfo(info1_, GetDefaultSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true));
    EXPECT_EQ(ExecuteSQL("INSERT OR REPLACE INTO DEVICE_SYNC_TABLE VALUES(1, 100, 0)", info1_), E_OK);
    EXPECT_EQ(CountTableData(info1_, DEVICE_SYNC_TABLE, "id=1"), 1);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    SetSchemaInfo(info2_, GetUniqueSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info2_, true, true));
    /**
     * @tc.steps: step2. Recreate table and set distributed tables.
     * @tc.expected: step2. Ok
     */
    EXPECT_EQ(ExecuteSQL("DROP TABLE DEVICE_SYNC_TABLE", info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_));
    EXPECT_EQ(ExecuteSQL("INSERT OR REPLACE INTO DEVICE_SYNC_TABLE VALUES(1, 100, 0)", info1_), E_OK);
    SetSchemaInfo(info1_, GetUniqueSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true, true));
    /**
     * @tc.steps: step3. Sync.
     * @tc.expected: step3. Ok
     */
    BlockSync(info1_, info2_, DEVICE_SYNC_TABLE, SyncMode::SYNC_MODE_PUSH_ONLY, OK);
}

/**
 * @tc.name: SyncAfterDrop002
 * @tc.desc: Test sync success after drop table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, SyncAfterDrop002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create table and set distributed tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    SetSchemaInfo(info1_, GetDefaultSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true));
    EXPECT_EQ(ExecuteSQL("INSERT OR REPLACE INTO DEVICE_SYNC_TABLE VALUES(1, 100, 0)", info1_), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    SetSchemaInfo(info2_, GetUniqueSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info2_, true, true));
    /**
     * @tc.steps: step2. Recreate table and set distributed tables and insert dirty data.
     * @tc.expected: step2. Ok
     */
    EXPECT_EQ(ExecuteSQL("DROP TABLE DEVICE_SYNC_TABLE", info1_), E_OK);
    std::string sql = "INSERT OR REPLACE INTO " + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) +
                      "(data_key, timestamp, wtimestamp, flag, hash_key, extend_field) "
                      "VALUES(1, 1, 1, 0x02, 'error_hash1', '100')";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    sql = "INSERT OR REPLACE INTO " + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) +
          "(data_key, timestamp, wtimestamp, flag, hash_key, extend_field) "
          "VALUES(1, 2, 2, 0x02, 'error_hash2', 200)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_));
    EXPECT_EQ(ExecuteSQL("INSERT OR REPLACE INTO DEVICE_SYNC_TABLE VALUES(1, 100, 0)", info1_), E_OK);
    SetSchemaInfo(info1_, GetUniqueSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, false, true));
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true, false));
    /**
     * @tc.steps: step3. Sync.
     * @tc.expected: step3. Ok
     */
    BlockSync(info1_, info2_, DEVICE_SYNC_TABLE, SyncMode::SYNC_MODE_PUSH_ONLY, OK);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "hash_key='error_hash1'"), 0);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "hash_key='error_hash2'"), 0);
}

/**
 * @tc.name: SetTrackerAfterDrop001
 * @tc.desc: Test set tracker success after drop table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, SetTrackerAfterDrop001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create table and set tracker table.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    SetSchemaInfo(info1_, GetDefaultSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true));
    /**
     * @tc.steps: step2. Recreate table and set tracker table with force.
     * @tc.expected: step2. Ok
     */
    EXPECT_EQ(ExecuteSQL("DROP TABLE DEVICE_SYNC_TABLE", info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_));
    ASSERT_EQ(SetTrackerTables(info1_, {DEVICE_SYNC_TABLE}, true), E_OK);
}

/**
 * @tc.name: InvalidCleanTracker001
 * @tc.desc: Test clean tracker with invalid obj.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, InvalidCleanTracker001, TestSize.Level0)
{
    VirtualSqliteRelationalStore store;
    auto schemaObj = store.CallGetSchemaObj();
    EXPECT_FALSE(schemaObj.IsSchemaValid());
    TrackerSchema schema;
    TableInfo info;
    bool isNoTableInSchema = false;
    bool isFirstCreate = false;
    RelationalDBProperties properties;
    auto engine = std::make_shared<SQLiteSingleRelationalStorageEngine>(properties);
    store.SetStorageEngine(engine);
    engine->SetEngineState(EngineState::ENGINE_BUSY);
    EXPECT_EQ(store.CallCheckTrackerTable(schema, info, isNoTableInSchema, isFirstCreate), -E_BUSY);
    EXPECT_FALSE(isNoTableInSchema);
    EXPECT_TRUE(isFirstCreate);
    store.CallCleanDirtyLogIfNeed(DEVICE_SYNC_TABLE);
}

/**
 * @tc.name: CleanDirtyLog001
 * @tc.desc: Test clean dirty log by utils.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, CleanDirtyLog001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create table and set distributed tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    SetSchemaInfo(info1_, GetUniqueSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true, true));
    /**
     * @tc.steps: step2. Execute sql with invalid db.
     * @tc.expected: step2. Fail
     */
    auto res = SQLiteRelationalUtils::CheckExistDirtyLog(nullptr, DEVICE_SYNC_TABLE);
    EXPECT_EQ(res.first, -E_INVALID_DB);
    EXPECT_FALSE(res.second);
    EXPECT_EQ(SQLiteRelationalUtils::ExecuteSql(nullptr, "", nullptr), -E_INVALID_DB);
    /**
     * @tc.steps: step3. Check dirty log with no exist table.
     * @tc.expected: step3. No exist dirty log
     */
    auto handle = GetSqliteHandle(info1_);
    ASSERT_NE(handle, nullptr);
    res = SQLiteRelationalUtils::CheckExistDirtyLog(handle, "NO_EXISTS_TABLE");
    EXPECT_EQ(res.first, E_OK);
    EXPECT_FALSE(res.second);
    /**
     * @tc.steps: step4. Insert data and dirty log.
     * @tc.expected: step4. Ok
     */
    EXPECT_EQ(ExecuteSQL("INSERT OR REPLACE INTO DEVICE_SYNC_TABLE VALUES(1, 100, 0)", info1_), E_OK);
    std::string sql = "INSERT OR REPLACE INTO " + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) +
                      "(data_key, timestamp, wtimestamp, flag, hash_key, extend_field) "
                      "VALUES(1, 1, 1, 0x02, 'error_hash1', '100')";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "hash_key='error_hash1'"), 1);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "data_key='1'"), 2); // 2 log
    /**
     * @tc.steps: step5. Clean dirty log.
     * @tc.expected: step5. Ok and no exist dirty log
     */
    RelationalSchemaObject obj;
    EXPECT_EQ(SQLiteRelationalUtils::CleanDirtyLog(handle, DEVICE_SYNC_TABLE, obj), E_OK);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "hash_key='error_hash1'"), 1);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "data_key='1'"), 2); // 2 log
    obj.SetDistributedSchema(RDBDataGenerator::ParseSchema(GetSchema(info1_)));
    EXPECT_EQ(SQLiteRelationalUtils::CleanDirtyLog(handle, DEVICE_SYNC_TABLE, obj), E_OK);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "hash_key='error_hash1'"), 0);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "data_key='1'"), 1);
    /**
     * @tc.steps: step6. Clean no exist table dirty log.
     * @tc.expected: step6. Fail
     */
    EXPECT_EQ(ExecuteSQL("DROP TABLE DEVICE_SYNC_TABLE", info1_), E_OK);
    EXPECT_EQ(SQLiteRelationalUtils::CleanDirtyLog(handle, DEVICE_SYNC_TABLE, obj), -1); // -1 is default error
}

/**
 * @tc.name: SyncWithDirtyLog001
 * @tc.desc: Test sync success when remote data log is dirty.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, SyncWithDirtyLog001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create table and set distributed tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    SetSchemaInfo(info1_, GetUniqueSchema());
    SetSchemaInfo(info2_, GetUniqueSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true, true));
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info2_, true, true));
    /**
     * @tc.steps: step2. Create table and set distributed tables.
     * @tc.expected: step2. Ok
     */
    EXPECT_EQ(ExecuteSQL("INSERT OR REPLACE INTO DEVICE_SYNC_TABLE VALUES(1, 100, 0)", info1_), E_OK);
    EXPECT_EQ(CountTableData(info1_, DEVICE_SYNC_TABLE, "id = 1"), 1);
    /**
     * @tc.steps: step3. DEV_A sync to DEV_B.
     * @tc.expected: step3. Ok
     */
    BlockSync(info1_, info2_, DEVICE_SYNC_TABLE, SYNC_MODE_PUSH_ONLY, OK);
    EXPECT_EQ(CountTableData(info1_, DEVICE_SYNC_TABLE, "id = 1"), 1);
    /**
     * @tc.steps: step4. Update log and set hash_key is dirty.
     * @tc.expected: step4. Ok
     */
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) + " SET hash_key=x'12'";
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
    ASSERT_EQ(CountTableData(info2_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), "hash_key=x'12'"), 1);
    /**
     * @tc.steps: step5. Update log and set hash_key is dirty.
     * @tc.expected: step5. Ok
     */
    EXPECT_EQ(ExecuteSQL("UPDATE DEVICE_SYNC_TABLE SET value = 200 WHERE uuid = 100", info1_), E_OK);
    ASSERT_EQ(CountTableData(info1_, DEVICE_SYNC_TABLE, "value = 200"), 1);
    BlockSync(info1_, info2_, DEVICE_SYNC_TABLE, SYNC_MODE_PUSH_ONLY, OK);
    ASSERT_EQ(CountTableData(info2_, DEVICE_SYNC_TABLE, "value = 200"), 1);
}

/**
 * @tc.name: GetLocalLog001
 * @tc.desc: Test get local log.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, GetLocalLog001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Get local log by invalid dataItem.
     * @tc.expected: step1. -E_NOT_FOUND
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    SetSchemaInfo(info1_, GetUniqueSchema());
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(info1_, true, true));
    auto handle = GetSqliteHandle(info1_);
    ASSERT_NE(handle, nullptr);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(handle, "SELECT * FROM DEVICE_SYNC_TABLE", stmt), E_OK);
    ASSERT_NE(stmt, nullptr);
    DataItem dataItem;
    RelationalSyncDataInserter inserter;
    LogInfo logInfo;
    EXPECT_EQ(SQLiteRelationalUtils::GetLocalLog(dataItem, inserter, stmt, logInfo), -E_NOT_FOUND);
    dataItem.flag = DataItem::DELETE_FLAG;
    EXPECT_EQ(SQLiteRelationalUtils::GetLocalLog(dataItem, inserter, stmt, logInfo), -E_NOT_FOUND);
    dataItem.flag = DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;
    EXPECT_EQ(SQLiteRelationalUtils::GetLocalLog(dataItem, inserter, stmt, logInfo), -E_NOT_FOUND);
    dataItem.flag = DataItem::DELETE_FLAG | DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;
    EXPECT_EQ(SQLiteRelationalUtils::GetLocalLog(dataItem, inserter, stmt, logInfo), -E_NOT_FOUND);
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(SQLiteRelationalUtils::GetDistributedPk(dataItem, inserter).empty());
    dataItem.value.resize(Parcel::GetUInt64Len());
    Parcel parcel(dataItem.value.data(), dataItem.value.size());
    parcel.WriteUInt64(0);
    /**
     * @tc.steps: step2. Get distributed pk when field is diff.
     * @tc.expected: step2. Get empty pk
     */
    std::vector<FieldInfo> remote;
    FieldInfo field;
    field.SetFieldName("field1");
    remote.push_back(field);
    field.SetFieldName("field2");
    remote.push_back(field);
    inserter.SetRemoteFields(remote);
    TableInfo table;
    table.AddField(field);
    inserter.SetLocalTable(table);
    DistributedTable distributedTable;
    DistributedField distributedField;
    distributedField.isP2pSync = true;
    distributedField.isSpecified = true;
    distributedField.colName = "field1";
    distributedTable.fields.push_back(distributedField);
    distributedField.colName = "field2";
    distributedTable.fields.push_back(distributedField);
    table.SetDistributedTable(distributedTable);
    inserter.SetLocalTable(table);
    EXPECT_TRUE(SQLiteRelationalUtils::GetDistributedPk(dataItem, inserter).empty());
    dataItem.value = {};
    EXPECT_TRUE(SQLiteRelationalUtils::GetDistributedPk(dataItem, inserter).empty());
}

/**
 * @tc.name: GetLocalLog002
 * @tc.desc: Test get local log by invalid pk.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, GetLocalLog002, TestSize.Level0)
{
    RelationalSyncDataInserter inserter;
    VBucket distributedPk;
    TableInfo tableInfo;
    DistributedTable distributedTable;
    DistributedField field;
    field.isP2pSync = true;
    field.isSpecified = true;
    field.colName = "field";
    distributedTable.fields.push_back(field);
    tableInfo.SetDistributedTable(distributedTable);
    inserter.SetLocalTable(tableInfo);
    EXPECT_EQ(SQLiteRelationalUtils::BindDistributedPk(nullptr, inserter, distributedPk), -E_INTERNAL_ERROR);
    FieldInfo fieldInfo;
    fieldInfo.SetFieldName(field.colName);
    tableInfo.AddField(fieldInfo);
    inserter.SetLocalTable(tableInfo);
    EXPECT_EQ(SQLiteRelationalUtils::BindDistributedPk(nullptr, inserter, distributedPk), -E_INTERNAL_ERROR);
}

/**
 * @tc.name: BindOneField001
 * @tc.desc: Test bind one field by invalid args.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDropTableTest, BindOneField001, TestSize.Level0)
{
    FieldInfo fieldInfo;
    VBucket distributedPk;
    fieldInfo.SetStorageType(StorageType::STORAGE_TYPE_INTEGER);
    EXPECT_EQ(SQLiteRelationalUtils::BindOneField(nullptr, 0, fieldInfo, distributedPk), -SQLITE_MISUSE);
    fieldInfo.SetStorageType(StorageType::STORAGE_TYPE_BLOB);
    EXPECT_EQ(SQLiteRelationalUtils::BindOneField(nullptr, 0, fieldInfo, distributedPk), -SQLITE_MISUSE);
    fieldInfo.SetStorageType(StorageType::STORAGE_TYPE_TEXT);
    EXPECT_EQ(SQLiteRelationalUtils::BindOneField(nullptr, 0, fieldInfo, distributedPk), -SQLITE_MISUSE);
    fieldInfo.SetStorageType(StorageType::STORAGE_TYPE_REAL);
    EXPECT_EQ(SQLiteRelationalUtils::BindOneField(nullptr, 0, fieldInfo, distributedPk), -SQLITE_MISUSE);
}
}