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
#include <gtest/gtest.h>

#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "rdb_data_generator.h"
#include "relational_store_client.h"
#include "relational_store_manager.h"
#include "relational_virtual_device.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;

class DistributedDBRDBCollaborationTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
protected:
    static DataBaseSchema GetSchema();
    static TableSchema GetTableSchema(bool upgrade = false);
    void InitStore();
    void InitDelegate(DistributedTableMode mode = DistributedTableMode::COLLABORATION);
    void CloseDb();
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
    RelationalVirtualDevice *deviceB_ = nullptr;
    RelationalStoreObserverUnitTest *delegateObserver_ = nullptr;

    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *DEVICE_SYNC_TABLE_UPGRADE = "DEVICE_SYNC_TABLE_UPGRADE";
    static constexpr const char *CLOUD_SYNC_TABLE = "CLOUD_SYNC_TABLE";
};

void DistributedDBRDBCollaborationTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBRDBCollaborationTest::TearDownTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBRDBCollaborationTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    InitStore();
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);

    deviceB_ = new (std::nothrow) RelationalVirtualDevice(UnitTestCommonConstant::DEVICE_B);
    ASSERT_NE(deviceB_, nullptr);
    auto syncInterfaceB = new (std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_NE(syncInterfaceB, nullptr);
    ASSERT_EQ(deviceB_->Initialize(communicatorAggregator_, syncInterfaceB), E_OK);
}

void DistributedDBRDBCollaborationTest::TearDown()
{
    CloseDb();
    if (deviceB_ != nullptr) {
        delete deviceB_;
        deviceB_ = nullptr;
    }
    if (delegateObserver_ != nullptr) {
        delete delegateObserver_;
        delegateObserver_ = nullptr;
    }
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    communicatorAggregator_ = nullptr;
}

DataBaseSchema DistributedDBRDBCollaborationTest::GetSchema()
{
    DataBaseSchema schema;
    auto table = GetTableSchema();
    schema.tables.push_back(table);
    table.name = CLOUD_SYNC_TABLE;
    schema.tables.push_back(table);
    schema.tables.push_back(GetTableSchema(true));
    return schema;
}

TableSchema DistributedDBRDBCollaborationTest::GetTableSchema(bool upgrade)
{
    TableSchema tableSchema;
    tableSchema.name = upgrade ? DEVICE_SYNC_TABLE_UPGRADE : DEVICE_SYNC_TABLE;
    Field field;
    field.primary = true;
    field.type = TYPE_INDEX<int64_t>;
    field.colName = "pk";
    tableSchema.fields.push_back(field);
    field.primary = false;
    field.colName = "int_field1";
    tableSchema.fields.push_back(field);
    field.colName = "int_field2";
    tableSchema.fields.push_back(field);
    field.colName = "str_field";
    field.type = TYPE_INDEX<std::string>;
    tableSchema.fields.push_back(field);
    if (upgrade) {
        field.colName = "int_field_upgrade";
        field.type = TYPE_INDEX<int64_t>;
        tableSchema.fields.push_back(field);
    }
    return tableSchema;
}

void DistributedDBRDBCollaborationTest::InitStore()
{
    if (storePath_.empty()) {
        storePath_ = g_testDir + "/" + STORE_ID_1 + ".db";
    }
    db_ = RelationalTestUtils::CreateDataBase(storePath_);
    ASSERT_NE(db_, nullptr);
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InitDatabase(schema, *db_), SQLITE_OK);
}

void DistributedDBRDBCollaborationTest::InitDelegate(DistributedTableMode mode)
{
    if (delegateObserver_ == nullptr) {
        delegateObserver_ = new (std::nothrow) RelationalStoreObserverUnitTest();
        ASSERT_NE(delegateObserver_, nullptr);
        delegateObserver_->SetCallbackDetailsType(static_cast<uint32_t>(CallbackDetailsType::DETAILED));
    }
    RelationalStoreManager mgr(APP_ID, USER_ID);
    RelationalStoreDelegate::Option option;
    option.tableMode = mode;
    option.observer = delegateObserver_;
    ASSERT_EQ(mgr.OpenStore(storePath_, STORE_ID_1, option, delegate_), OK);
    ASSERT_NE(delegate_, nullptr);
}

void DistributedDBRDBCollaborationTest::CloseDb()
{
    if (db_ != nullptr) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
    if (delegate_ != nullptr) {
        RelationalStoreManager mgr(APP_ID, USER_ID);
        EXPECT_EQ(mgr.CloseStore(delegate_), OK);
        delegate_ = nullptr;
    }
}

/**
 * @tc.name: SetSchema001
 * @tc.desc: Test set distributed schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema)), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    EXPECT_EQ(delegate_->CreateDistributedTable(CLOUD_SYNC_TABLE, TableSyncType::CLOUD_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", CLOUD_SYNC_TABLE);
    /**
     * @tc.steps: step2. Insert update delete local
     * @tc.expected: step2.ok
     */
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    EXPECT_EQ(RegisterClientObserver(db_, [](ClientChangedData &changedData) {
        for (const auto &table : changedData.tableData) {
            EXPECT_FALSE(table.second.isTrackedDataChange);
            EXPECT_TRUE(table.second.isP2pSyncDataChange);
        }
    }), OK);
    EXPECT_EQ(RDBDataGenerator::UpdateLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    UnRegisterClientObserver(db_);
}

/**
 * @tc.name: SetSchema002
 * @tc.desc: Test upgrade table mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create cloud table in SPLIT_BY_DEVICE
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::SPLIT_BY_DEVICE));
    EXPECT_EQ(delegate_->CreateDistributedTable(CLOUD_SYNC_TABLE, TableSyncType::CLOUD_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", CLOUD_SYNC_TABLE);
    CloseDb();
    /**
     * @tc.steps: step2. Create device table in COLLABORATION
     * @tc.expected: step2.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema)), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    CloseDb();
}

/**
 * @tc.name: SetSchema003
 * @tc.desc: Test set distributed schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema, true)), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step2. Insert update delete local
     * @tc.expected: step2.ok
     */
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    EXPECT_EQ(RegisterClientObserver(db_, [](ClientChangedData &changedData) {
        for (const auto &table : changedData.tableData) {
            EXPECT_FALSE(table.second.isTrackedDataChange);
            EXPECT_FALSE(table.second.isP2pSyncDataChange);
        }
    }), OK);
    EXPECT_EQ(RDBDataGenerator::UpdateLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    UnRegisterClientObserver(db_);
}

/**
 * @tc.name: SetSchema004
 * @tc.desc: Test create distributed table without schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
}

DistributedSchema GetDistributedSchema(const std::string &tableName, const std::vector<std::string> &fields)
{
    DistributedTable table;
    table.tableName = tableName;
    for (const auto &fieldName : fields) {
        DistributedField field;
        field.isP2pSync = true;
        field.colName = fieldName;
        table.fields.push_back(field);
    }
    DistributedSchema schema;
    schema.tables.push_back(table);
    return schema;
}

/**
 * @tc.name: SetSchema005
 * @tc.desc: Test set distributed schema with invalid schema
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Test set distributed schema with invalid table name
     * @tc.expected: step2. return SCHEMA_MISMATCH
     */
    DistributedSchema schema1 = GetDistributedSchema("", {});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema1), SCHEMA_MISMATCH);
    DistributedSchema schema2 = GetDistributedSchema("xxx", {});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema2), SCHEMA_MISMATCH);
    /**
     * @tc.steps: step3. Test set distributed schema with invalid fields
     * @tc.expected: step3. return SCHEMA_MISMATCH OR DISTRIBUTED_FIELD_DECREASE
     */
    DistributedSchema schema3 = GetDistributedSchema(DEVICE_SYNC_TABLE, {});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema3), SCHEMA_MISMATCH);
    DistributedSchema schema4 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"xxx"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema4), SCHEMA_MISMATCH);
    DistributedSchema schema5 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk", "int_field1"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema5), OK);
    DistributedSchema schema6 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema6), DISTRIBUTED_FIELD_DECREASE);
}

/**
 * @tc.name: SetSchema006
 * @tc.desc: Test register client observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    DistributedSchema distributedSchema = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk", "int_field1"});
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    TrackerSchema trackerSchema = {
        .tableName = DEVICE_SYNC_TABLE, .extendColNames = {"int_field1"}, .trackerColNames = {"int_field1"}
    };
    EXPECT_EQ(delegate_->SetTrackerTable(trackerSchema), OK);
    /**
     * @tc.steps: step2. Insert update local
     * @tc.expected: step2.ok
     */
    EXPECT_EQ(RegisterClientObserver(db_, [](ClientChangedData &changedData) {
        for (const auto &table : changedData.tableData) {
            EXPECT_TRUE(table.second.isTrackedDataChange);
            EXPECT_TRUE(table.second.isP2pSyncDataChange);
        }
    }), OK);
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    std::string sql = "update " + std::string(DEVICE_SYNC_TABLE) + " set int_field1 = int_field1 + 1 where pk = 0";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    UnRegisterClientObserver(db_);
}

/**
 * @tc.name: SetSchema007
 * @tc.desc: Test register client observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    DistributedSchema schema1 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema1), OK);
    /**
     * @tc.steps:step2. delete triggers
     * @tc.expected: step2. Return OK.
     */
    std::vector<std::string> triggerTypes = {"INSERT", "UPDATE", "DELETE"};
    for (const auto &triggerType : triggerTypes) {
        std::string sql = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + std::string(DEVICE_SYNC_TABLE) + "_ON_" +
            triggerType;
        SQLiteUtils::ExecuteRawSQL(db_, sql);
    }
    /**
     * @tc.steps:step3. Set distributed schema and check if the trigger exists
     * @tc.expected: step3. Check OK.
     */
    DistributedSchema schema2 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk", "int_field1"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema2), OK);
    for (const auto &triggerType : triggerTypes) {
        std::string sql = "select count(*) from sqlite_master where type = 'trigger' and tbl_name = '" +
            std::string(DEVICE_SYNC_TABLE) + "' and name = 'naturalbase_rdb_" + std::string(DEVICE_SYNC_TABLE) +
            "_ON_" + triggerType + "';";
        int count = 0;
        EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
        EXPECT_EQ(count, 1);
    }
}

/**
 * @tc.name: NormalSync001
 * @tc.desc: Test set distributed schema and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step2. Insert one data
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 10, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
}

/**
 * @tc.name: NormalSync002
 * @tc.desc: Test sync with diff distributed schema [high version -> low version].
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    /**
     * @tc.steps: step2. DeviceB set schema [pk, int_field1, int_field2, int_field_upgrade]
     * @tc.expected: step2.ok
     */
    DataBaseSchema virtualSchema;
    auto tableSchema = GetTableSchema(true);
    tableSchema.name = DEVICE_SYNC_TABLE;
    virtualSchema.tables.push_back(tableSchema);
    auto distributedSchema = RDBDataGenerator::ParseSchema(virtualSchema);
    deviceB_->SetDistributedSchema(distributedSchema);
    /**
     * @tc.steps: step3. Real device set schema [pk, int_field1, int_field2]
     * @tc.expected: step3.ok
     */
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(GetSchema())), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step4. Insert table info and virtual data into deviceB
     * @tc.expected: step4.ok
     */
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 10, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(DEVICE_SYNC_TABLE_UPGRADE, DEVICE_SYNC_TABLE, db_, deviceB_),
        E_OK);
    /**
     * @tc.steps: step5. Sync to real device
     * @tc.expected: step5.ok
     */
    Query query = Query::Select(tableSchema.name);
    EXPECT_EQ(deviceB_->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
}

/**
 * @tc.name: NormalSync003
 * @tc.desc: Test sync with diff distributed schema [low version -> high version].
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    /**
     * @tc.steps: step2. DeviceB set schema [pk, int_field1, int_field2]
     * @tc.expected: step2.ok
     */
    DataBaseSchema virtualSchema;
    auto tableSchema = GetTableSchema();
    tableSchema.name = DEVICE_SYNC_TABLE_UPGRADE;
    virtualSchema.tables.push_back(tableSchema);
    auto distributedSchema = RDBDataGenerator::ParseSchema(virtualSchema);
    deviceB_->SetDistributedSchema(distributedSchema);
    /**
     * @tc.steps: step3. Real device set schema [pk, int_field1, int_field2, int_field_upgrade]
     * @tc.expected: step3.ok
     */
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(GetSchema())), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE_UPGRADE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step4. Insert table info and virtual data into deviceB
     * @tc.expected: step4.ok
     */
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 10, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(DEVICE_SYNC_TABLE, DEVICE_SYNC_TABLE_UPGRADE, db_, deviceB_),
        E_OK);
    /**
     * @tc.steps: step5. Sync to real device
     * @tc.expected: step5.ok
     */
    Query query = Query::Select(tableSchema.name);
    EXPECT_EQ(deviceB_->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
}

/**
 * @tc.name: NormalSync004
 * @tc.desc: Test sync when distributed schema was not set.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Sync to real device
     * @tc.expected: step2. return SCHEMA_MISMATCH.
     */
    Query query = Query::Select(tableSchema.name);
    DBStatus status = delegate_->Sync({UnitTestCommonConstant::DEVICE_B}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
    EXPECT_EQ(status, SCHEMA_MISMATCH);
}

/**
 * @tc.name: NormalSync005
 * @tc.desc: Test sync with specified columns
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    DistributedSchema schema = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk", "int_field1"});
    deviceB_->SetDistributedSchema(schema);
    EXPECT_EQ(delegate_->SetDistributedSchema(schema), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Init some data
     * @tc.expected: step2.ok
     */
    int64_t dataNum = 10;
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, dataNum / 2, db_, GetTableSchema()), E_OK);
    std::string sql = "update " + std::string(DEVICE_SYNC_TABLE) + " set int_field1 = 1, int_field2 = 2 where pk >= 0";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    auto tableSchema = GetTableSchema();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataNum, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device and check data
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    EXPECT_EQ(deviceB_->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_PULL, query, true), E_OK);
    sql = "select int_field1, int_field2 from " + std::string(DEVICE_SYNC_TABLE) + " order by pk;";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
    int dataIndex = 0;
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        int64_t intField1Value = sqlite3_column_int64(stmt, 0);
        int64_t intField2Value = sqlite3_column_int64(stmt, 1);
        EXPECT_EQ(intField1Value, dataIndex);
        dataIndex++;
        if (dataIndex <= dataNum / 2) {
            EXPECT_EQ(intField2Value, 2);
        } else {
            EXPECT_EQ(intField2Value, 0);
        }
    }
    EXPECT_EQ(dataIndex, dataNum);
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

/**
 * @tc.name: NormalSync006
 * @tc.desc: Test set distributed schema and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step2. Insert one data
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PUSH_ONLY, OK, {deviceB_->GetDeviceId()});
}

/**
 * @tc.name: NormalSync007
 * @tc.desc: Test change distributed schema and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step2. Insert one data
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    std::string sql = std::string("select count(*) from ").append(DEVICE_SYNC_TABLE)
            .append(" where pk=0 and int_field1 is null and int_field2 is null");
    int count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 1);
    /**
     * @tc.steps: step4. Change schema and sync again
     * @tc.expected: step4.ok
     */
    distributedSchema = RDBDataGenerator::ParseSchema(schema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    sql = std::string("select count(*) from ").append(DEVICE_SYNC_TABLE)
        .append(" where pk=0 and int_field1=0 and int_field2=0");
    count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 1);
    auto changeData = delegateObserver_->GetSavedChangedData()[std::string(DEVICE_SYNC_TABLE)];
    EXPECT_TRUE(changeData.properties.isP2pSyncDataChange);
}

/**
 * @tc.name: NormalSync008
 * @tc.desc: Test set distributed schema and sync with diff sort.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step2. Insert one data with diff sort col
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_,
        RDBDataGenerator::FlipTableSchema(tableSchema)), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    std::string sql = std::string("select count(*) from ").append(DEVICE_SYNC_TABLE).append(" where pk=0;");
    int count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 1);
    sql = std::string("select count(*) from ")
        .append(RelationalStoreManager::GetDistributedLogTableName(DEVICE_SYNC_TABLE))
        .append(" where data_key=1 and cursor=1;");
    count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: NormalSync010
 * @tc.desc: Test sync without not null col.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync010, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Recreate not null table
     * @tc.expected: step1.ok
     */
    std::string sql = std::string("DROP TABLE ").append(DEVICE_SYNC_TABLE);
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InitTable(tableSchema, true, *db_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step2. Insert one data
     * @tc.expected: step2.ok
     */
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync without str col
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, DB_ERROR, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. Check if the distributed table exists
     * @tc.expected: step4.ok
     */
    int count = 0;
    sql = std::string("select count(*) from ")
            .append(RelationalStoreManager::GetDistributedLogTableName(DEVICE_SYNC_TABLE));
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 0);
}
}