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

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBRDBDataStatusTest : public RDBGeneralUt {
public:
    void SetUp() override;
protected:
    void PrepareTableBasicEnv(bool createWithTracker = false);
    void PrepareBasicTableByType(TableSyncType type);
    void PrepareBasicTableByType(const std::vector<std::string> &tables, TableSyncType type);
    void DataStatusComplexTest(bool testWithTracker);
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table, bool noPk = false);
    void InitCollaborationDelegate();
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    static constexpr const char *DEVICE_C = "DEVICE_C";
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBDataStatusTest::SetUp()
{
    RDBGeneralUt::SetUp();
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
}

UtDateBaseSchemaInfo DistributedDBRDBDataStatusTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBRDBDataStatusTest::GetTableSchema(const std::string &table, bool noPk)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo field;
    field.field.colName = "id";
    field.field.type = TYPE_INDEX<int64_t>;
    if (!noPk) {
        field.field.primary = true;
    }
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

void DistributedDBRDBDataStatusTest::PrepareTableBasicEnv(bool createWithTracker)
{
    /**
     * @tc.steps: step1. Call InitDelegate interface with default split table mode.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    /**
     * @tc.steps: step2. Set distributed tables.
     * @tc.expected: step2. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    if (createWithTracker) {
        ASSERT_EQ(SetTrackerTables(info1_, {DEVICE_SYNC_TABLE}), E_OK);
        ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    }
    /**
     * @tc.steps: step3. Insert local data.
     * @tc.expected: step3. Ok
     */
    InsertLocalDBData(0, 1, info1_);
    /**
     * @tc.steps: step4. DEV_A sync to DEV_B.
     * @tc.expected: step4. Ok
     */
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step5. DEV_A update time
     * @tc.expected: step5. Ok
     */
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store->OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME)), OK);
}

void DistributedDBRDBDataStatusTest::PrepareBasicTableByType(TableSyncType type)
{
    ASSERT_NO_FATAL_FAILURE(PrepareBasicTableByType({DEVICE_SYNC_TABLE}, type));
}

void DistributedDBRDBDataStatusTest::PrepareBasicTableByType(const std::vector<std::string> &tables,
    TableSyncType type)
{
    /**
     * @tc.steps: step1. Call InitDelegate interface with default split table mode.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    /**
     * @tc.steps: step2. Set distributed tables.
     * @tc.expected: step2. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, tables, type), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, tables, type), E_OK);
    ASSERT_EQ(SetTrackerTables(info1_, tables), E_OK);
    /**
     * @tc.steps: step3. Insert local data.
     * @tc.expected: step3. Ok
     */
    InsertLocalDBData(0, 1, info1_);
}

void DistributedDBRDBDataStatusTest::DataStatusComplexTest(bool testWithTracker)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_NO_FATAL_FAILURE(PrepareTableBasicEnv(testWithTracker));
    /**
     * @tc.steps: step6. Reopen store with DEV_C.
     * @tc.expected: step6. Ok
     */
    ASSERT_EQ(RDBGeneralUt::CloseDelegate(info1_), OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_C), E_OK);
    /**
     * @tc.steps: step7. DEV_C sync to DEV_B.
     * @tc.expected: step7. Ok
     */
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step8. Check 1 record with device_c.
     * @tc.expected: step8. Ok
     */
    EXPECT_EQ(CountTableDataByDev(info2_, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), DEVICE_C), 1);
}

void DistributedDBRDBDataStatusTest::InitCollaborationDelegate()
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
}

/**
 * @tc.name: SplitTable001
 * @tc.desc: Test split table sync after update time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDataStatusTest, SplitTable001, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareTableBasicEnv());
    /**
     * @tc.steps: step6. DEV_A sync to DEV_B.
     * @tc.expected: step6. Ok
     */
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE);
}


/**
 * @tc.name: SplitTable002
 * @tc.desc: Test split table sync with diff dev after update time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDataStatusTest, SplitTable002, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareTableBasicEnv());
    /**
     * @tc.steps: step6. Reopen store with DEV_C.
     * @tc.expected: step6. Ok
     */
    ASSERT_EQ(RDBGeneralUt::CloseDelegate(info1_), OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_C), E_OK);
    /**
     * @tc.steps: step7. DEV_C sync to DEV_B.
     * @tc.expected: step7. Ok
     */
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step8. Check 1 record in device_c table.
     * @tc.expected: step8. Ok
     */
    EXPECT_EQ(CountTableData(info2_, DBCommon::GetDistributedTableName(DEVICE_C, DEVICE_SYNC_TABLE)), 1);
}

/**
 * @tc.name: CollaborationTable001
 * @tc.desc: Test collaboration table sync after update time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable001, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_NO_FATAL_FAILURE(PrepareTableBasicEnv());
    /**
     * @tc.steps: step6. DEV_A sync to DEV_B.
     * @tc.expected: step6. Ok
     */
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE);
}

/**
 * @tc.name: CollaborationTable002
 * @tc.desc: Test collaboration table sync after update time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable002, TestSize.Level0)
{
    DataStatusComplexTest(false);
}

/**
 * @tc.name: CollaborationTable003
 * @tc.desc: Test collaboration search table sync after update time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable003, TestSize.Level0)
{
    DataStatusComplexTest(true);
}

/**
 * @tc.name: CollaborationTable004
 * @tc.desc: Test collaboration search table sync after update time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable004, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_NO_FATAL_FAILURE(PrepareBasicTableByType(DEVICE_COOPERATION));
    /**
     * @tc.steps: step1. DEV_A sync to DEV_B.
     * @tc.expected: step1. Ok
     */
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE);
}

/**
 * @tc.name: CollaborationTable005
 * @tc.desc: Test collaboration no pk table set schema twice.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create no pk table and set as distributed table.
     * @tc.expected: step1. Ok
     */
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema("new_table", true));
    SetSchemaInfo(info1_, info);
    SetSchemaInfo(info2_, info);
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_NO_FATAL_FAILURE(PrepareBasicTableByType({"new_table"}, DEVICE_COOPERATION));
    /**
     * @tc.steps: step2. Update distributed table by create more table.
     * @tc.expected: step2. Ok
     */
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    SetSchemaInfo(info1_, info);
    SetSchemaInfo(info2_, info);
    ASSERT_NO_FATAL_FAILURE(PrepareBasicTableByType({"new_table", DEVICE_SYNC_TABLE}, DEVICE_COOPERATION));
}

/**
 * @tc.name: CollaborationTable006
 * @tc.desc: Test device collaboration and search table sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable006, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, DEVICE_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. SCHEMA_MISMATCH
     */
    InsertLocalDBData(0, 1, info1_);
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE, SCHEMA_MISMATCH);
}

/**
 * @tc.name: CollaborationTable007
 * @tc.desc: Test device collaboration search table and search table sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable007, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, DEVICE_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info1_, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. SCHEMA_MISMATCH
     */
    InsertLocalDBData(0, 1, info1_);
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE, SCHEMA_MISMATCH);
}

/**
 * @tc.name: CollaborationTable008
 * @tc.desc: Test device collaboration and cloud collaboration search table sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable008, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, DEVICE_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {DEVICE_SYNC_TABLE}, CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. NOT_SUPPORT
     */
    InsertLocalDBData(0, 1, info1_);
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE, NOT_SUPPORT);
}

/**
 * @tc.name: CollaborationTable009
 * @tc.desc: Test device collaboration search table and cloud collaboration search table sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable009, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, DEVICE_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info1_, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {DEVICE_SYNC_TABLE}, CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. NOT_SUPPORT
     */
    InsertLocalDBData(0, 1, info1_);
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE, NOT_SUPPORT);
}

/**
 * @tc.name: CollaborationTable010
 * @tc.desc: Test device collaboration and cloud collaboration able sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable010, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {DEVICE_SYNC_TABLE}, DEVICE_COOPERATION), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. NOT_SUPPORT
     */
    InsertLocalDBData(0, 1, info1_);
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    auto toDevice  = GetDevice(info2_);
    ASSERT_FALSE(toDevice.empty());
    Query query = Query::Select(DEVICE_SYNC_TABLE);
    DBStatus callStatus = store->Sync({toDevice}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
    EXPECT_EQ(callStatus, NOT_SUPPORT);
}

/**
 * @tc.name: CollaborationTable011
 * @tc.desc: Test cloud collaboration table and device collaboration search table sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable011, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {DEVICE_SYNC_TABLE}, DEVICE_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. NOT_SUPPORT
     */
    InsertLocalDBData(0, 1, info1_);
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    auto toDevice  = GetDevice(info2_);
    ASSERT_FALSE(toDevice.empty());
    Query query = Query::Select(DEVICE_SYNC_TABLE);
    DBStatus callStatus = store->Sync({toDevice}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
    EXPECT_EQ(callStatus, NOT_SUPPORT);
}

/**
 * @tc.name: CollaborationTable012
 * @tc.desc: Test cloud collaboration and search table sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable012, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. NOT_SUPPORT
     */
    InsertLocalDBData(0, 1, info1_);
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    auto toDevice  = GetDevice(info2_);
    ASSERT_FALSE(toDevice.empty());
    Query query = Query::Select(DEVICE_SYNC_TABLE);
    DBStatus callStatus = store->Sync({toDevice}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
    EXPECT_EQ(callStatus, NOT_SUPPORT);
}

/**
 * @tc.name: CollaborationTable013
 * @tc.desc: Test cloud collaboration search table and search table sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CollaborationTable013, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    /**
     * @tc.steps: step1. Set distributed tables and Tracker tables.
     * @tc.expected: step1. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}, CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetTrackerTables(info1_, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetTrackerTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert data and sync.
     * @tc.expected: step2. NOT_SUPPORT
     */
    InsertLocalDBData(0, 1, info1_);
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    auto toDevice  = GetDevice(info2_);
    ASSERT_FALSE(toDevice.empty());
    Query query = Query::Select(DEVICE_SYNC_TABLE);
    DBStatus callStatus = store->Sync({toDevice}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
    EXPECT_EQ(callStatus, NOT_SUPPORT);
}

/**
 * @tc.name: CreateDistributedTableTest001
 * @tc.desc: Test CreateDistributedTable interface after re-create table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBDataStatusTest, CreateDistributedTableTest001, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    ASSERT_NO_FATAL_FAILURE(PrepareTableBasicEnv());
    EXPECT_EQ(RDBGeneralUt::CreateDistributedTable(info1_, {DEVICE_SYNC_TABLE}), E_OK);

    /**
     * @tc.steps: step1. Drop table, and create same table that unique field are added.
     * @tc.expected: step1. Ok
     */
    sqlite3 *db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    std::string newSql = "DROP TABLE DEVICE_SYNC_TABLE;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, newSql), SQLITE_OK);
    newSql = "CREATE TABLE IF NOT EXISTS DEVICE_SYNC_TABLE('id' INTEGER PRIMARY KEY, new_field INTEGER UNIQUE);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, newSql), SQLITE_OK);

    /**
     * @tc.steps: step2. Create distributed table.
     * @tc.expected: step2. OK
     */
    EXPECT_EQ(RDBGeneralUt::CreateDistributedTable(info1_, {DEVICE_SYNC_TABLE}), E_OK);
}

/**
 * @tc.name: TimeCheck001
 * @tc.desc: Test operate data status after modify time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBDataStatusTest, TimeCheck001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create distributed table and insert local data.
     */
    ASSERT_NO_FATAL_FAILURE(InitCollaborationDelegate());
    EXPECT_EQ(RDBGeneralUt::CreateDistributedTable(info1_, {DEVICE_SYNC_TABLE}), E_OK);
    InsertLocalDBData(0, 1, info1_);
    /**
     * @tc.steps: step2. Get last max timestamp.
     * @tc.expected: step2. Ok
     */
    sqlite3 *db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    auto [errCode, t1] = RelationalTestUtils::GetMaxTimestamp(db, DEVICE_SYNC_TABLE);
    EXPECT_EQ(errCode, E_OK);
    /**
     * @tc.steps: step3. Modify time offset and operator data status.
     * @tc.expected: step3. Ok
     */
    std::string sql = "INSERT OR REPLACE INTO " + std::string(DBConstant::RELATIONAL_PREFIX) +
        "metadata VALUES('localTimeOffset', '100000')";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store->OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME)), OK);
    /**
     * @tc.steps: step4. Check timestamp should not decrease.
     * @tc.expected: step4. Ok
     */
    auto [ret, t2] = RelationalTestUtils::GetMaxTimestamp(db, DEVICE_SYNC_TABLE);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GE(t2, t1);
}
}