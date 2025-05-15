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
#include <variant>

#include "cloud_db_sync_utils_test.h"
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
    static TableSchema GetTableSchema(bool upgrade = false, bool pkInStr = false);
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
    static constexpr const char *DEVICE_SYNC_TABLE_AUTOINCREMENT = "DEVICE_SYNC_TABLE_AUTOINCREMENT";
    static constexpr const char *CLOUD_SYNC_TABLE = "CLOUD_SYNC_TABLE";
    static constexpr const char *BIG_COLUMNS_TABLE = "BIG_COLUMNS_TABLE";
    static constexpr const char *INVALID_TABLE = "INVALID_TABLE";
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

TableSchema DistributedDBRDBCollaborationTest::GetTableSchema(bool upgrade, bool pkInStr)
{
    TableSchema tableSchema;
    tableSchema.name = upgrade ? DEVICE_SYNC_TABLE_UPGRADE : DEVICE_SYNC_TABLE;
    Field field;
    if (!pkInStr) {
        field.primary = true;
    }
    field.type = TYPE_INDEX<int64_t>;
    field.colName = "pk";
    tableSchema.fields.push_back(field);
    field.primary = false;
    field.colName = "int_field1";
    tableSchema.fields.push_back(field);
    field.colName = "int_field2";
    tableSchema.fields.push_back(field);
    field.colName = "123";
    field.type = TYPE_INDEX<std::string>;
    if (pkInStr) {
        field.primary = true;
    }
    tableSchema.fields.push_back(field);
    if (upgrade) {
        field.primary = false;
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
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    EXPECT_EQ(delegate_->CreateDistributedTable(CLOUD_SYNC_TABLE, TableSyncType::CLOUD_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", CLOUD_SYNC_TABLE);
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema);
    for (auto &table : distributedSchema.tables) {
        for (auto &field : table.fields) {
            field.isSpecified = false;
        }
    }
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema002, TestSize.Level1)
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
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema)), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema, true)), OK);
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

    /**
     * @tc.steps: step4. Test set distributed schema with int_field1 but isP2pSync is false
     * @tc.expected: step4. return DISTRIBUTED_FIELD_DECREASE
     */
    DistributedSchema distributedSchema1 = {0, {{DEVICE_SYNC_TABLE, {{"int_field1", false}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema1), DISTRIBUTED_FIELD_DECREASE);
}

/**
 * @tc.name: SetSchema006
 * @tc.desc: Test register client observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    DistributedSchema distributedSchema = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk", "int_field1"});
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
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

    /**
     * @tc.steps: step3. Insert data that pk = 0 and sync to real device
     * @tc.expected: step3.ok
     */
    auto schema = GetSchema();
    deviceB_->SetDistributedSchema(distributedSchema);
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});

    /**
     * @tc.steps: step4. Check extend_field
     * @tc.expected: step4.ok
     */
    sql = "select count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + std::string(DEVICE_SYNC_TABLE) + "_log"
        " where json_extract(extend_field, '$.int_field1')=0";
    EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1u), nullptr), SQLITE_OK);
    UnRegisterClientObserver(db_);
}

std::string GetTriggerSql(const std::string &tableName, const std::string &triggerTypeName, sqlite3 *db) {
    if (db == nullptr) {
        return "";
    }
    std::string sql = "select sql from sqlite_master where type = 'trigger' and tbl_name = '" + tableName +
        "' and name = 'naturalbase_rdb_" + tableName + "_ON_" + triggerTypeName + "';";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("[GetTriggerSql] prepare statement failed(%d), sys(%d), errmsg(%s)", errCode, errno, sqlite3_errmsg(db));
        return "";
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("[GetTriggerSql] execute statement failed(%d), sys(%d), errmsg(%s)", errCode, errno, sqlite3_errmsg(db));
        return "";
    }
    const std::string triggerSql = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return triggerSql;
}

/**
 * @tc.name: SetSchema007
 * @tc.desc: Test whether setting the schema multiple times will refresh the trigger
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema007, TestSize.Level1)
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
    std::string oldInsertTriggerSql = GetTriggerSql(DEVICE_SYNC_TABLE, "INSERT", db_);
    std::string oldUpdateTriggerSql = GetTriggerSql(DEVICE_SYNC_TABLE, "UPDATE", db_);
    std::string oldDeleteTriggerSql = GetTriggerSql(DEVICE_SYNC_TABLE, "DELETE", db_);
    EXPECT_FALSE(oldInsertTriggerSql.empty() || oldUpdateTriggerSql.empty() || oldDeleteTriggerSql.empty());
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
    std::string newInsertTriggerSql = GetTriggerSql(DEVICE_SYNC_TABLE, "INSERT", db_);
    std::string newUpdateTriggerSql = GetTriggerSql(DEVICE_SYNC_TABLE, "UPDATE", db_);
    std::string newDeleteTriggerSql = GetTriggerSql(DEVICE_SYNC_TABLE, "DELETE", db_);
    EXPECT_FALSE(newInsertTriggerSql.empty() || newUpdateTriggerSql.empty() || newDeleteTriggerSql.empty());
    EXPECT_TRUE(oldInsertTriggerSql == newInsertTriggerSql);
    EXPECT_TRUE(oldUpdateTriggerSql != newUpdateTriggerSql);
    EXPECT_TRUE(oldDeleteTriggerSql == newDeleteTriggerSql);
}

/**
 * @tc.name: SetSchema008
 * @tc.desc: Test set distributed schema with pk but pk isP2pSync is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Prepare db
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Test set distributed schema without pk
     * @tc.expected: step2. return OK
     */
    DistributedSchema distributedSchema = {0, {{DEVICE_SYNC_TABLE, {{"int_field1", true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step3. Test set distributed schema with pk but pk isP2pSync is false
     * @tc.expected: step3. return SCHEMA_MISMATCH
     */
    DistributedSchema distributedSchema1 = {0, {{DEVICE_SYNC_TABLE, {{"pk", false}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema1), SCHEMA_MISMATCH);
    /**
     * @tc.steps: step4. Test set same distributed schema
     * @tc.expected: step4. return OK
     */
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
}

/**
 * @tc.name: SetSchema009
 * @tc.desc: Test tableMode is SPLIT_BY_DEVICE then SetDistributedSchema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema009, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db, tableMode is SPLIT_BY_DEVICE
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::SPLIT_BY_DEVICE));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Test set distributed schema without pk
     * @tc.expected: step2. return NOT_SUPPORT
     */
    DistributedSchema distributedSchema = {0, {{DEVICE_SYNC_TABLE, {{"int_field1", true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), NOT_SUPPORT);
}

/**
 * @tc.name: SetSchema010
 * @tc.desc: Test create distributed table without communicator.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema010, TestSize.Level0)
{
    if (deviceB_ != nullptr) {
        delete deviceB_;
        deviceB_ = nullptr;
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
}

/**
 * @tc.name: SetSchema011
 * @tc.desc: Test set schema with not null field but isP2pSync is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema011, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Test set distributed schema
     * @tc.expected: step2. return SCHEMA_MISMATCH
     */
    DistributedSchema schema1 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk"});
    DistributedField &field1 = schema1.tables.front().fields.front();
    field1.isP2pSync = false;
    EXPECT_EQ(delegate_->SetDistributedSchema(schema1), SCHEMA_MISMATCH);
}

/**
 * @tc.name: SetSchema012
 * @tc.desc: Test call SetDistributedSchema with empty tables
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema012, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db, tableMode is SPLIT_BY_DEVICE
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::SPLIT_BY_DEVICE));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Test set schema with empty tables vector
     * @tc.expected: step2. return SCHEMA_MISMATCH
     */
    DistributedSchema distributedSchema = {0, {}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
    /**
     * @tc.steps: step3. Test set schema with a list of empty tables
     * @tc.expected: step3. return SCHEMA_MISMATCH
     */
    distributedSchema = {0, {{}, {}, {}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
    /**
     * @tc.steps: step4. Test set schema with a mix of empty and non-empty tables
     * @tc.expected: step4. return SCHEMA_MISMATCH
     */
    distributedSchema = {0, {{DEVICE_SYNC_TABLE, {{"int_field1", true}}}, {}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
}

/**
 * @tc.name: SetSchema013
 * @tc.desc: Test set tracker table for device table and check if timestamp has changed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema013, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    DistributedSchema distributedSchema = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk", "int_field1"});
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    TrackerSchema trackerSchema = {
        .tableName = DEVICE_SYNC_TABLE, .extendColNames = {"int_field1"}, .trackerColNames = {"int_field1"}
    };
    /**
     * @tc.steps: step2. Insert one data and query timestamp
     * @tc.expected: step2.ok
     */
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    sqlite3_stmt *stmt = nullptr;
    std::string sql = "select timestamp from " + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) +
        " where data_key=0";
    EXPECT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
    EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t timestamp1 = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    /**
     * @tc.steps: step3. Set tracker table and query timestamp
     * @tc.expected: step3.Equal
     */
    EXPECT_EQ(delegate_->SetTrackerTable(trackerSchema), WITH_INVENTORY_DATA);
    EXPECT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
    EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t timestamp2 = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    SQLiteUtils::ResetStatement(stmt, true, ret);
    EXPECT_EQ(timestamp1, timestamp2);
}

/**
 * @tc.name: SetSchema014
 * @tc.desc: Test set tracker table for device table and check if timestamp has changed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema014, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create auto increment table and specified pk, distributed schema without not null field
     * @tc.expected: step1. create distributed table ok but set schema failed
     */
    auto tableSchema = GetTableSchema();
    tableSchema.name = DEVICE_SYNC_TABLE_AUTOINCREMENT;
    ASSERT_EQ(RDBDataGenerator::InitTable(tableSchema, true, true, *db_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    DistributedSchema distributedSchema;
    DistributedTable distributedTable;
    distributedTable.tableName = tableSchema.name;
    DistributedField distributedField;
    distributedField.colName = "pk";
    distributedField.isSpecified = true;
    distributedField.isP2pSync = true;
    distributedTable.fields.push_back(distributedField);
    distributedSchema.tables.push_back(distributedTable);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableSchema.name, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
    /**
     * @tc.steps: step2. Distributed schema with not null field
     * @tc.expected: step2. ok
     */
    distributedSchema.tables.clear();
    distributedField.colName = "123";
    distributedField.isSpecified = false;
    distributedTable.fields.push_back(distributedField);
    distributedSchema.tables.push_back(distributedTable);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step3. Distributed schema with error specified
     * @tc.expected: step3. SCHEMA_MISMATCH
     */
    distributedSchema.tables.clear();
    distributedField.colName = "123";
    distributedField.isSpecified = true;
    distributedTable.fields.push_back(distributedField);
    distributedField.colName = "pk";
    distributedField.isP2pSync = false;
    distributedTable.fields.push_back(distributedField);
    distributedSchema.tables.push_back(distributedTable);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
}

/**
 * @tc.name: SetSchema015
 * @tc.desc: Test call SetDistributedSchema with mark more than one unique col isSpecified true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema015, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Prepare db, tableMode is COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string createSql = "CREATE TABLE IF NOT EXISTS table_pk_int(integer_field INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "int_field INT, char_field CHARACTER(20) UNIQUE, clob_field CLOB UNIQUE, UNIQUE(char_field, clob_field));";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, createSql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable("table_pk_int", TableSyncType::DEVICE_COOPERATION), OK);
 
    /**
     * @tc.steps: step2. Test mark more than one unique col isSpecified true
     * @tc.expected: step2. return SCHEMA_MISMATCH
     */
    DistributedSchema distributedSchema = {0, {{"table_pk_int", {
                                                    {"integer_field", false, false},
                                                    {"int_field", true, false},
                                                    {"char_field", true, true},
                                                    {"clob_field", true, true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
}

/**
 * @tc.name: SetSchema016
 * @tc.desc: Test set isSpecified to false after isSpecified was set to true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema016, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string tableName = "multiPriKeyTable";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName +
        "(pk1 INTEGER, pk2 INT, PRIMARY KEY (pk1, pk2));";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Test set distributed schema
     * @tc.expected: step2. return OK
     */
    DistributedSchema schema1 = GetDistributedSchema(tableName, {"pk1", "pk2"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema1), OK);
    /**
     * @tc.steps: step3. Test set distributed schema
     * @tc.expected: step3. return SCHEMA_MISMATCH
     */
    DistributedSchema schema2 = GetDistributedSchema(tableName, {"pk1", "pk2"});
    DistributedField &field2 = schema2.tables.front().fields.front();
    field2.isSpecified = true;
    EXPECT_EQ(delegate_->SetDistributedSchema(schema2), SCHEMA_MISMATCH);
}

int GetHashKey(sqlite3 *db, const std::string &tableName, std::vector<std::string> &hashKeys) {
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    std::string sql = "select cast(hash_key as text) from " + DBCommon::GetLogTableName(tableName) +
        " order by timestamp;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("prepare statement failed(%d), sys(%d), msg(%s)", errCode, errno, sqlite3_errmsg(db));
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(stmt);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            LOGE("[SQLiteUtils][ExecuteSQL] execute statement failed(%d), sys(%d), msg(%s)",
                errCode, errno, sqlite3_errmsg(db));
        } else {
            const unsigned char *result = sqlite3_column_text(stmt, 0);
            hashKeys.push_back(reinterpret_cast<const std::string::value_type *>(result));
        }
    } while (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));

    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    return errCode;
}

/**
 * @tc.name: SetSchema017
 * @tc.desc: Test whether to update hash_key after setting up distributed schema
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema017, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Prepare db
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string tableName = "multiPriKeyTable";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName +
        "(pk1 INTEGER PRIMARY KEY AUTOINCREMENT, pk2 INT UNIQUE);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Insert a record and get hash_key
     * @tc.expected: step2.ok
     */
    int dataCount = 10;
    for (int i = 0; i < dataCount; i++) {
        sql = "insert into " + tableName + " values (" + std::to_string(i) + ", " + std::to_string(i + 1) + ");";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    }
    std::vector<std::string> oldHashKeys;
    EXPECT_EQ(GetHashKey(db_, tableName, oldHashKeys), E_OK);
    ASSERT_EQ(oldHashKeys.size(), static_cast<size_t>(dataCount));
    /**
     * @tc.steps: step3. Set distributed schema and get old hash_key
     * @tc.expected: step3.ok
     */
    DistributedSchema schema1 = GetDistributedSchema(tableName, {"pk1", "pk2"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema1), OK);
    std::vector<std::string> newHashKeys1;
    EXPECT_EQ(GetHashKey(db_, tableName, newHashKeys1), E_OK);
    ASSERT_EQ(newHashKeys1.size(), static_cast<size_t>(dataCount));
    for (int i = 0; i < dataCount; i++) {
        EXPECT_EQ(oldHashKeys[i], newHashKeys1[i]);
    }
    /**
     * @tc.steps: step4. Set another distributed schema and get old hash_key
     * @tc.expected: step4.ok
     */
    DistributedSchema schema2 = {0, {{"multiPriKeyTable", {
        {"pk1", false, false},
        {"pk2", true, true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(schema2, true), OK);
    std::vector<std::string> newHashKeys2;
    EXPECT_EQ(GetHashKey(db_, tableName, newHashKeys2), E_OK);
    ASSERT_EQ(newHashKeys2.size(), static_cast<size_t>(dataCount));
    for (int i = 0; i < dataCount; i++) {
        EXPECT_NE(newHashKeys1[i], newHashKeys2[i]);
    }
    EXPECT_NE(newHashKeys2, oldHashKeys);
}

/**
 * @tc.name: SetSchema018
 * @tc.desc: Test no primary key table setting isSpecified
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema018, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string tableName = "noPriKeyTable";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName +
                      "(field_int1 INTEGER, field_int2 INT);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Test set distributed schema
     * @tc.expected: step2. return SCHEMA_MISMATCH
     */
    DistributedSchema schema = GetDistributedSchema(tableName, {"field_int1"});
    DistributedField &field = schema.tables.front().fields.front();
    field.isSpecified = true;
    EXPECT_EQ(delegate_->SetDistributedSchema(schema), SCHEMA_MISMATCH);
}

/**
 * @tc.name: SetSchema019
 * @tc.desc: Test call SetDistributedSchema when unique col not set isP2pSync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema019, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db, tableMode is COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string createSql = "CREATE TABLE IF NOT EXISTS table_pk_integer(integer_field INTEGER UNIQUE,"
                 "int_field INT, char_field CHARACTER(20), clob_field CLOB);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, createSql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable("table_pk_integer", TableSyncType::DEVICE_COOPERATION), OK);

    /**
     * @tc.steps: step2. Test mark unique col isP2pSync true
     * @tc.expected: step2. return SCHEMA_MISMATCH
     */
    DistributedSchema distributedSchema = {1, {{"table_pk_integer", {
                                                    {"int_field", true},
                                                    {"char_field", true},
                                                    {"clob_field", true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
}

/**
 * @tc.name: SetSchema020
 * @tc.desc: Test call SetDistributedSchema when unique col and pk isP2pSync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema020, TestSize.Level0)
{

    /**
     * @tc.steps: step1. Prepare db, tableMode is COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string createSql = "CREATE TABLE IF NOT EXISTS table_pk_int(integer_field INTEGER PRIMARY KEY AUTOINCREMENT,"
        "int_field INT UNIQUE, char_field CHARACTER(20), clob_field CLOB);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, createSql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable("table_pk_int", TableSyncType::DEVICE_COOPERATION), OK);

    /**
     * @tc.steps: step2. Test mark unique col and pk isP2pSync true, specified unique col
     * @tc.expected: step2. return NOT_SUPPORT
     */
    DistributedSchema distributedSchema = {1, {
        {"table_pk_int", {
            {"integer_field", true},
            {"int_field", true, true},
            {"char_field", true},
            {"clob_field", true}
        }}}
    };
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), NOT_SUPPORT);
}

/**
 * @tc.name: SetSchema021
 * @tc.desc: Test call SetDistributedSchema when table contains 1000 columns
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema021, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create table which contains 1000 columns
     * @tc.expected: step1. create table ok
     */
    TableSchema tableSchema;
    tableSchema.name = BIG_COLUMNS_TABLE;
    Field field;
    field.primary = true;
    field.type = TYPE_INDEX<int64_t>;
    field.colName = "pk";
    tableSchema.fields.push_back(field);
    field.primary = false;
    field.colName = "123";
    field.type = TYPE_INDEX<std::string>;
    tableSchema.fields.push_back(field);
    const uint16_t fieldNum = 1000;
    for (int i = 0; i < fieldNum; i++) {
        field.colName = "field" + to_string(i);
        tableSchema.fields.push_back(field);
    }
    ASSERT_EQ(RDBDataGenerator::InitTable(tableSchema, true, true, *db_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    EXPECT_EQ(delegate_->CreateDistributedTable(tableSchema.name, TableSyncType::DEVICE_COOPERATION), OK);

    /**
     * @tc.steps: step2. set schema
     * @tc.expected: step2. ok
     */
    DistributedSchema distributedSchema;
    DistributedTable distributedTable;
    distributedTable.tableName = tableSchema.name;
    DistributedField distributedField;
    distributedField.isP2pSync = true;
    distributedField.colName = "123";
    distributedField.isSpecified = false;
    distributedTable.fields.push_back(distributedField);
    for (int i = 0; i < fieldNum; i++) {
        distributedField.colName = "field" + to_string(i);
        distributedTable.fields.push_back(distributedField);
    }
    distributedSchema.tables.push_back(distributedTable);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
}

/**
 * @tc.name: SetSchema022
 * @tc.desc: Test setting the distributed schema with table names named keywords.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema022, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table named keywords.
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    std::string tableName = "except";
    std::string sql = "CREATE TABLE IF NOT EXISTS '" + tableName + "'(field1 INTEGER, field2 INT);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    /**
     * @tc.steps: step2. Create distributed table and set distributed schema
     * @tc.expected: step2.ok
     */
    auto distributedSchema = GetDistributedSchema(tableName, {"field1", "field2"});
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
}

/**
 * @tc.name: SetSchema023
 * @tc.desc: Test SetDistributedSchema interface when tables decrease.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema023, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set distributed schema for the first time
     * @tc.expected: step1. return OK
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    DistributedSchema distributedSchema = {1, {{DEVICE_SYNC_TABLE, {{"pk", true}, {"int_field1", true}}},
                                               {CLOUD_SYNC_TABLE, {{"pk", true}, {"int_field2", false}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);

    /**
     * @tc.steps: step2. decrease tables of input parameter to set distributed schema
     * @tc.expected: step2. return OK
     */
    distributedSchema = {2, {{CLOUD_SYNC_TABLE, {{"pk", true}, {"int_field2", false}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    distributedSchema = {3, {{DEVICE_SYNC_TABLE, {{"pk", true}, {"int_field1", true}}},
                             {DEVICE_SYNC_TABLE_UPGRADE, {{"pk", true}, {"int_field2", false}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
}

/**
 * @tc.name: SetSchema024
 * @tc.desc: Test SetDistributedSchema out of max schema size limit.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema024, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set distributed schema for the first time
     * @tc.expected: step1. return OK
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    /**
     * @tc.steps: step2. generate 32 tables with 1000 field
     * @tc.expected: step2. return OK
     */
    auto basicSchema = GetTableSchema();
    const int fieldCount = 1000;
    Field basicField;
    basicField.type = TYPE_INDEX<int64_t>;
    basicField.colName = "generate_field_";
    for (int i = 0; i < fieldCount; ++i) {
        Field field = basicField;
        field.colName  += std::to_string(i);
        basicSchema.fields.push_back(field);
    }
    /**
     * @tc.steps: step3. create distributed table
     * @tc.expected: step3. return OVER_MAX_LIMITS at last
     */
    const int tableCount = 32;
    DBStatus res = OK;
    for (int i = 0; i < tableCount; ++i) {
        TableSchema table = basicSchema;
        table.name += std::to_string(i);
        RDBDataGenerator::InitTable(table, false, *db_);
        res = delegate_->CreateDistributedTable(table.name);
        if (res != OK) {
            break;
        }
    }
    EXPECT_EQ(res, OVER_MAX_LIMITS);
}

/**
 * @tc.name: SetSchema025
 * @tc.desc: Test SetDistributedSchema with isForceUpgrade.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema025, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    DistributedSchema distributedSchema1 = {1, {{DEVICE_SYNC_TABLE, {{"pk", true}, {"int_field1", true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema1), OK);
    /**
     * @tc.steps: step2. Set distributed schema with isForceUpgrade
     * @tc.expected: step2.ok
     */
    DistributedSchema distributedSchema2 = {2, {{DEVICE_SYNC_TABLE, {{"pk", true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema2, true), OK);
}

/**
 * @tc.name: SetSchema026
 * @tc.desc: Test SetDistributedSchema with conflict log.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema026, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db, tableMode is COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string createSql = "CREATE TABLE IF NOT EXISTS table_pk_int(integer_field INTEGER PRIMARY KEY AUTOINCREMENT,"
        "int_field INT UNIQUE);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, createSql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable("table_pk_int", TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Set Distributed Schema
     * @tc.expected: step2. return OK
     */
    DistributedSchema distributedSchema = {1, {
        {"table_pk_int", {
            {"integer_field", true},
            {"int_field", true}
        }}}
    };
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step3. Local insert (1, 2), (2, 1) and delete (1, 2)
     * @tc.expected: step3. return OK
     */
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, "INSERT INTO table_pk_int VALUES(1, 2)"), E_OK);
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, "INSERT INTO table_pk_int VALUES(2, 1)"), E_OK);
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, "DELETE FROM table_pk_int WHERE integer_field = 1"), E_OK);
    /**
     * @tc.steps: step4. Upgrade Distributed Schema and mark int_field specified
     * @tc.expected: step4. return OK
     */
    distributedSchema = {2, {
            {"table_pk_int", {
                    {"integer_field", false},
                    {"int_field", true, true}
            }}}
    };
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema, true), OK);
}

/**
 * @tc.name: NormalSync001
 * @tc.desc: Test set distributed schema and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync002, TestSize.Level1)
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
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(GetSchema())), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync003, TestSize.Level1)
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
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE_UPGRADE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(GetSchema())), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    DistributedSchema schema = GetDistributedSchema(DEVICE_SYNC_TABLE, {"pk", "int_field1"});
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    deviceB_->SetDistributedSchema(schema);
    EXPECT_EQ(delegate_->SetDistributedSchema(schema), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
    /**
     * @tc.steps: step4. Change schema to non-existent table name, then sync
     * @tc.expected: step4.SCHEMA_MISMATCH
     */
    tableSchema.name = "not_config_table";
    ASSERT_EQ(RDBDataGenerator::InitTable(tableSchema, true, *db_), SQLITE_OK);
    ASSERT_EQ(delegate_->CreateDistributedTable(tableSchema.name), OK);
    Query query2 = Query::Select(tableSchema.name);
    std::map<std::string, std::vector<TableStatus>> statusMap;
    SyncStatusCallback callBack2;
    DBStatus callStatus = delegate_->Sync({deviceB_->GetDeviceId()}, SYNC_MODE_PUSH_ONLY, query2, callBack2, true);
    EXPECT_EQ(callStatus, SCHEMA_MISMATCH);
}

/**
 * @tc.name: NormalSync007
 * @tc.desc: Test change distributed schema and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
        .append(" where data_key=0 and cursor=1;");
    count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: NormalSync009
 * @tc.desc: Test if distributed table will be created when sync with COLLABORATION mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync009, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
    std::string checkSql = "select count(*) from sqlite_master where type='table' and name != '" +
        DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) + "' and name like 'naturalbase_rdb_aux_" +
        std::string(DEVICE_SYNC_TABLE) + "_%'";
    int count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkSql, count), E_OK);
    EXPECT_EQ(count, 0);
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. Check if the distributed table exists
     * @tc.expected: step4.ok
     */
    count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkSql, count), E_OK);
    EXPECT_EQ(count, 0);
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
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), SCHEMA_MISMATCH);
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
    DBStatus callStatus = delegate_->Sync({deviceB_->GetDeviceId()}, SYNC_MODE_PULL_ONLY, query, nullptr, true);
    EXPECT_EQ(callStatus, SCHEMA_MISMATCH);
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

/**
 * @tc.name: NormalSync011
 * @tc.desc: Test set the distributed schema first then sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync011, TestSize.Level1)
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
    Query query = Query::Select().FromTable({tableSchema.name});
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PUSH_ONLY, OK, {deviceB_->GetDeviceId()});
}

/**
 * @tc.name: NormalSync012
 * @tc.desc: Test sync with autoincrement table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync012, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create auto increment table and unique index
     * @tc.expected: step1.ok
     */
    auto tableSchema = GetTableSchema();
    tableSchema.name = DEVICE_SYNC_TABLE_AUTOINCREMENT;
    ASSERT_EQ(RDBDataGenerator::InitTable(tableSchema, true, true, *db_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    tableSchema = GetTableSchema(false, true);
    tableSchema.name = DEVICE_SYNC_TABLE_AUTOINCREMENT;
    auto schema = GetSchema();
    schema.tables.push_back(tableSchema);
    DistributedSchema distributedSchema = {0, {{tableSchema.name, {
        {"pk", false, false},
        {"int_field1", true, false},
        {"int_field2", true, false},
        {"123", true, true}}}}};
    deviceB_->SetDistributedSchema(distributedSchema);
    int errCode = SQLiteUtils::ExecuteRawSQL(db_, std::string("CREATE UNIQUE INDEX U_INDEX ON ")
        .append(tableSchema.name).append("('123')"));
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableSchema.name, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step2. Insert one data
     * @tc.expected: step2.ok
     */
    ASSERT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, tableSchema), E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100 ms
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 2, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    std::string sql = std::string("select count(*) from ").append(tableSchema.name);
    int count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 2);
    /**
     * @tc.steps: step4. Update date and sync again
     * @tc.expected: step4.ok
     */
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    sql = std::string("select count(*) from ").append(tableSchema.name);
    count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: NormalSync013
 * @tc.desc: Test chanage data after sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lg
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync013, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
    /**
     * @tc.steps: step4. check changData
     * @tc.expected: step4.ok
     */
    auto changeData = delegateObserver_->GetSavedChangedData();
    EXPECT_EQ(changeData[tableSchema.name].primaryData[0].size(), 10u);
    EXPECT_EQ(changeData[tableSchema.name].field.size(), 1u);
    /**
     * @tc.steps: step5. Remove the last item and a cloud-only item, then check changeData
     * @tc.expected: step5. Only the last item is in changeData and field is primary key
     */
    delegateObserver_->ClearChangedData();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(9, 11, deviceB_, tableSchema, true), E_OK);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    changeData = delegateObserver_->GetSavedChangedData();
    ASSERT_EQ(changeData[tableSchema.name].primaryData[OP_DELETE].size(), 1u);
    EXPECT_EQ(changeData[tableSchema.name].primaryData[OP_DELETE][0].size(), 1u);
    ASSERT_EQ(changeData[tableSchema.name].field.size(), 1u);
    EXPECT_EQ(changeData[tableSchema.name].field[0], "pk");
}

/**
 * @tc.name: NormalSync014
 * @tc.desc: Test chanage data after sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync014, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    DistributedSchema schema1 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"int_field1"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema1), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_COOPERATION);
    /**
     * @tc.steps: step2. Insert data
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
    /**
     * @tc.steps: step4. check changData
     * @tc.expected: step4.ok
     */
    auto changeData = delegateObserver_->GetSavedChangedData();
    EXPECT_EQ(changeData[tableSchema.name].primaryData[0].size(), 10u);
    EXPECT_EQ(changeData[tableSchema.name].field.size(), 1u);
    /**
     * @tc.steps: step5. SetDistributedSchema again
     * @tc.expected: step5.ok
     */
    DistributedSchema schema2 = GetDistributedSchema(DEVICE_SYNC_TABLE, {"int_field1", "int_field2"});
    EXPECT_EQ(delegate_->SetDistributedSchema(schema2), OK);
    /**
     * @tc.steps: step6. Sync to real device
     * @tc.expected: step6.ok
     */
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step7. check changData
     * @tc.expected: step7.ok
     */
    changeData = delegateObserver_->GetSavedChangedData();
    EXPECT_EQ(changeData[tableSchema.name].primaryData[1].size(), 10u);
    EXPECT_EQ(changeData[tableSchema.name].field.size(), 1u);
}

/**
 * @tc.name: NormalSync015
 * @tc.desc: Test sync with multi primary key table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync015, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    std::string tableName = "multiPriKeyTable";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName +
        "(pk1 INTEGER, pk2 INT, PRIMARY KEY (pk1, pk2));";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    /**
     * @tc.steps: step2. Create distributed table and set distributed schema
     * @tc.expected: step2.ok
     */
    auto distributedSchema = GetDistributedSchema(tableName, {"pk1", "pk2"});
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step3. Init data and sync to real device
     * @tc.expected: step3.ok
     */
    TableSchema tableSchema;
    tableSchema.name = tableName;
    Field field;
    field.primary = true;
    field.type = TYPE_INDEX<int64_t>;
    field.colName = "pk1";
    tableSchema.fields.push_back(field);
    field.colName = "pk2";
    tableSchema.fields.push_back(field);
    uint32_t dataCount = 10;
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableName, db_, deviceB_), E_OK);
    Query query = Query::Select(tableName);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. check changData
     * @tc.expected: step4.ok
     */
    auto changeData = delegateObserver_->GetSavedChangedData();
    ASSERT_EQ(changeData[tableName].primaryData[OP_INSERT].size(), dataCount);
    for (uint32_t i = 0; i < dataCount; i++) {
        EXPECT_EQ(changeData[tableName].primaryData[OP_INSERT][i].size(), 3u); // primary key (pk1, pk2) and rowid
    }
    EXPECT_EQ(changeData[tableName].field.size(), 3u); // primary key (pk1, pk2) and rowid
    /**
     * @tc.steps: step5. Remove the last item and a cloud-only item, then check changeData
     * @tc.expected: step5. Only the last item is in changeData and field is key (pk1, pk2) and rowid
     */
    delegateObserver_->ClearChangedData();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(dataCount - 1, dataCount + 1, deviceB_, tableSchema, true),
        E_OK);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    changeData = delegateObserver_->GetSavedChangedData();
    ASSERT_EQ(changeData[tableName].primaryData[OP_DELETE].size(), 1u);
    EXPECT_EQ(changeData[tableName].primaryData[OP_DELETE][0].size(), 3u); // primary key (pk1, pk2) and rowid
    EXPECT_EQ(changeData[tableName].field.size(), 3u); // primary key (pk1, pk2) and rowid
}

/**
 * @tc.name: NormalSync016
 * @tc.desc: Test sync with diff specified field.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync016, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Prepare db, tableMode is COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    std::string createSql = "CREATE TABLE IF NOT EXISTS table_int(integer_field INTEGER PRIMARY KEY AUTOINCREMENT,"
        "int_field1 INT UNIQUE, int_field2 INT UNIQUE);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, createSql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable("table_int", TableSyncType::DEVICE_COOPERATION), OK);

    /**
     * @tc.steps: step2. Test mark one specified one is field1 another is field2
     * @tc.expected: step2. sync return SCHEMA_MISMATCH
     */
    DistributedSchema distributedSchema = {0, {{"table_int", {
        {"integer_field", false, false},
        {"int_field1", true, false},
        {"int_field2", true, true}}}}};
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    distributedSchema = {0, {{"table_int", {
        {"integer_field", false, false},
        {"int_field1", true, true},
        {"int_field2", true, false}}}}};
    deviceB_->SetDistributedSchema(distributedSchema);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv("table_int", db_, deviceB_), E_OK);
    Query query = Query::Select("table_int");
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, SCHEMA_MISMATCH,
        {deviceB_->GetDeviceId()});
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PUSH_ONLY, SCHEMA_MISMATCH,
        {deviceB_->GetDeviceId()});
}

/**
 * @tc.name: NormalSync017
 * @tc.desc: Test delete other device's data and sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync017, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
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
    std::string sql = std::string("select count(*) from ").append(DEVICE_SYNC_TABLE);
    int count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 1);
    /**
     * @tc.steps: step4. Delete data and sync again
     * @tc.expected: step4.ok
     */
    EXPECT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100 ms
    sql = std::string("delete from ").append(DEVICE_SYNC_TABLE).append(" where 0 = 0");
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    sql = std::string("select count(*) from ").append(DEVICE_SYNC_TABLE);
    count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 0);
    auto changeData = delegateObserver_->GetSavedChangedData()[std::string(DEVICE_SYNC_TABLE)];
    EXPECT_TRUE(changeData.properties.isP2pSyncDataChange);
}

/**
 * @tc.name: NormalSync018
 * @tc.desc: Test sync with no primary key table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync018, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    std::string tableName = "noPriKeyTable";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + "(pk1 INTEGER, pk2 INT);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    /**
     * @tc.steps: step2. Create distributed table and set distributed schema
     * @tc.expected: step2.ok
     */
    auto distributedSchema = GetDistributedSchema(tableName, {"pk1", "pk2"});
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step3. Init data and sync to real device
     * @tc.expected: step3.ok
     */
    TableSchema tableSchema;
    tableSchema.name = tableName;
    Field field;
    field.primary = true;
    field.type = TYPE_INDEX<int64_t>;
    field.colName = "pk1";
    tableSchema.fields.push_back(field);
    field.colName = "pk2";
    tableSchema.fields.push_back(field);
    uint32_t dataCount = 10;
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableName, db_, deviceB_), E_OK);
    Query query = Query::Select(tableName);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. check changData
     * @tc.expected: step4.ok
     */
    auto changeData = delegateObserver_->GetSavedChangedData();
    ASSERT_EQ(changeData[tableName].primaryData[OP_INSERT].size(), dataCount);
    for (uint32_t i = 0; i < dataCount; i++) {
        EXPECT_EQ(changeData[tableName].primaryData[OP_INSERT][i].size(), 1u); // rowid
    }
    EXPECT_EQ(changeData[tableName].field.size(), 1u); // rowid
}

/**
 * @tc.name: NormalSync019
 * @tc.desc: Test whether there is an observer notification when local win.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync019, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step2. Insert a piece of data from the other end and then locally insert the same data.
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    std::string sql = std::string("insert into ").append(DEVICE_SYNC_TABLE).append("(pk) values (0)");
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. Check observer
     * @tc.expected: step4.No data change notification
     */
    auto onChangeCallCount = delegateObserver_->GetCloudCallCount();
    EXPECT_EQ(onChangeCallCount, 0u);
}

/**
 * @tc.name: NormalSync020
 * @tc.desc: Test set distributed schema after recreating the table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync020, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step2. Sync a piece of data
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step3. Recreate table and set distributed schema.
     * @tc.expected: step3.ok
     */
    std::string sql = std::string("drop table ").append(DEVICE_SYNC_TABLE);
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    RDBDataGenerator::InitTable(schema.tables.front(), false, *db_);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
}

/**
 * @tc.name: NormalSync021
 * @tc.desc: Test sync multi table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync021, TestSize.Level1)
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
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE_UPGRADE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Insert 10 data
     * @tc.expected: step2.ok
     */
    int dataCount = 10;
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    tableSchema = GetTableSchema(true);
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device and check data
     * @tc.expected: step3.ok
     */
    Query query = Query::Select().FromTable({DEVICE_SYNC_TABLE, DEVICE_SYNC_TABLE_UPGRADE});
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    std::string sql = std::string("select count(*) from ").append(DEVICE_SYNC_TABLE);
    int actualCount = 0;
    SQLiteUtils::GetCountBySql(db_, sql, actualCount);
    EXPECT_EQ(actualCount, dataCount);
    sql = std::string("select count(*) from ").append(DEVICE_SYNC_TABLE_UPGRADE);
    actualCount = 0;
    SQLiteUtils::GetCountBySql(db_, sql, actualCount);
    EXPECT_EQ(actualCount, dataCount);
    /**
     * @tc.steps: step4. Sync with invalid args
     * @tc.expected: step4.invalid args
     */
    query.And();
    DBStatus callStatus = delegate_->Sync({deviceB_->GetDeviceId()}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
    EXPECT_EQ(callStatus, NOT_SUPPORT);
}

/**
 * @tc.name: NormalSync022
 * @tc.desc: Test drop table after sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync022, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    std::string tableName = "noPriKeyTable";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + "(field1 INTEGER, field2 INT);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    auto distributedSchema = GetDistributedSchema(tableName, {"field1", "field2"});
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step2. Insert one piece of data at each end
     * @tc.expected: step2.ok
     */
    TableSchema tableSchema = {tableName, "", {{"field1", TYPE_INDEX<int64_t>}, {"field2", TYPE_INDEX<int64_t>}}};
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    sql = "insert into " + tableName + " values(1, 1)";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    std::string checkLogCountSql = "select count(*) from " + DBCommon::GetLogTableName(tableName);
    std::string checkDataCountSql = "select count(*) from " + tableName;
    int logCount = 0;
    int dataCount = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkDataCountSql, dataCount), E_OK);
    EXPECT_EQ(logCount, 1);
    EXPECT_EQ(dataCount, 1);
    /**
     * @tc.steps: step3. Sync a piece of data
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkDataCountSql, dataCount), E_OK);
    EXPECT_EQ(dataCount, 2);
    EXPECT_EQ(logCount, 2);
    /**
     * @tc.steps: step4. Drop table and check log count
     * @tc.expected: step4.ok
     */
    checkLogCountSql.append(" where data_key = -1 and flag&0x01 = 0x1;");
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(logCount, 0);
    std::string dropTableSql = "drop table " + tableName;
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, dropTableSql), E_OK);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(logCount, 2);
}

/**
 * @tc.name: NormalSync023
 * @tc.desc: Test synchronization of autoIncremental primary key table with observer notices.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync023, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    std::string tableName = "autoIncPriKeyTable";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName +
        "(pk INTEGER PRIMARY KEY AUTOINCREMENT, field1 INT UNIQUE);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    DistributedSchema distributedSchema = {0u, {{tableName, {{"pk", false, false}, {"field1", true, true}}}}};
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step2. Insert data
     * @tc.expected: step2.ok
     */
    uint32_t dataCount = 10;
    TableSchema tableSchema = {tableName, "", {{"pk", TYPE_INDEX<int64_t>, true}, {"field1", TYPE_INDEX<int64_t>}}};
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device and check data
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableName);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. check changData
     * @tc.expected: step4.ok
     */
    auto changeData = delegateObserver_->GetSavedChangedData();
    ASSERT_EQ(changeData[tableName].primaryData[OP_INSERT].size(), dataCount);
    for (uint32_t i = 0; i < changeData[tableName].primaryData[OP_INSERT].size(); i++) {
        auto *data = std::get_if<int64_t>(&changeData[tableName].primaryData[OP_INSERT][i].front());
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(static_cast<uint32_t>(*data), i + 1);
    }
    ASSERT_EQ(changeData[tableName].field.size(), 1u);
    EXPECT_EQ(changeData[tableName].field.front(), "pk");
}

/**
 * @tc.name: NormalSync024
 * @tc.desc: Test set tracker table and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lg
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync024, TestSize.Level1)
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
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE_UPGRADE, TableSyncType::DEVICE_COOPERATION), OK);

    TrackerSchema trackerSchema = {
        .tableName = DEVICE_SYNC_TABLE, .extendColNames = {"int_field1"}, .trackerColNames = {"int_field1"}};
    delegate_->SetTrackerTable(trackerSchema);

    /**
     * @tc.steps: step2. close deviceA and reopen it
     * @tc.expected: step2.ok
     */
    RelationalStoreManager mgr(APP_ID, USER_ID);
    EXPECT_EQ(mgr.CloseStore(delegate_), OK);
    delegate_ = nullptr;
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    option.observer = delegateObserver_;
    ASSERT_EQ(mgr.OpenStore(storePath_, STORE_ID_1, option, delegate_), OK);
    ASSERT_NE(delegate_, nullptr);

    /**
     * @tc.steps: step3. Insert 10 data
     * @tc.expected: step3.ok
     */
    int dataCount = 10;
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    tableSchema = GetTableSchema(true);
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step4. Sync to real device and check data
     * @tc.expected: step4.ok
     */
    Query query = Query::Select().FromTable({DEVICE_SYNC_TABLE, DEVICE_SYNC_TABLE_UPGRADE});
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    std::string checkLogCountSql = std::string("SELECT COUNT(*) FROM ") + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) +
                                   " AS a LEFT JOIN " + DEVICE_SYNC_TABLE +
                                   " AS b  ON (a.data_key = b._rowid_) WHERE a.data_key = b._rowid_;";
    int logCount = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(logCount, 10);
}

/**
 * @tc.name: NormalSync025
 * @tc.desc: Test drop table in split_by_device mode will only mark local data as delete in log table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync025, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create SPLIT_BY_DEVICE tables
     * @tc.expected: step1. Ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::SPLIT_BY_DEVICE));
    std::string tableName = "noPKTable025";
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + tableName + "(field1 INTEGER, field2 INT);";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, createTableSql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Insert one piece of data at each end
     * @tc.expected: step2. Ok
     */
    TableSchema tableSchema = {tableName, "", {{"field1", TYPE_INDEX<int64_t>}, {"field2", TYPE_INDEX<int64_t>}}};
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    std::string sql = "insert into " + tableName + " values(1, 1)";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    std::string checkLogCountSql = "select count(*) from " + DBCommon::GetLogTableName(tableName);
    std::string checkDataCountSql = "select count(*) from " + tableName;
    int logCount = 0;
    int dataCount = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkDataCountSql, dataCount), E_OK);
    EXPECT_EQ(logCount, 1);
    EXPECT_EQ(dataCount, 1);
    /**
     * @tc.steps: step3. Sync to pull deviceB_ data
     * @tc.expected: step3. One data in device table and one data in local table
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkDataCountSql, dataCount), E_OK);
    EXPECT_EQ(dataCount, 1);
    EXPECT_EQ(logCount, 2);
    std::string checkDeviceTableCountSql = "select count(*) from " +
        DBCommon::GetDistributedTableName(deviceB_->GetDeviceId(), tableName);
    int deviceDataCount = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkDeviceTableCountSql, deviceDataCount), E_OK);
    EXPECT_EQ(deviceDataCount, 1);
    /**
     * @tc.steps: step4. Check delete count in log table before drop
     * @tc.expected: step4. No deleted records
     */
    checkLogCountSql.append(" where data_key = -1 and flag&0x01 = 0x1;");
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(logCount, 0);
    /**
     * @tc.steps: step5. Drop table and recreate
     * @tc.expected: step5. Ok
     */
    std::string dropTableSql = "drop table " + tableName;
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, dropTableSql), E_OK);
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, createTableSql), E_OK);
    EXPECT_EQ(delegate_->CreateDistributedTable(tableName, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step6. Check count
     * @tc.expected: step6. only the local record is marked as deleted
     */
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkLogCountSql, logCount), E_OK);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkDataCountSql, dataCount), E_OK);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, checkDeviceTableCountSql, deviceDataCount), E_OK);
    EXPECT_EQ(logCount, 1);
    EXPECT_EQ(dataCount, 0);
    EXPECT_EQ(deviceDataCount, 1);
}

/**
 * @tc.name: SetStoreConfig001
 * @tc.desc: Test set store config.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetStoreConfig001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in SPLIT_BY_DEVICE
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::SPLIT_BY_DEVICE));
    /**
     * @tc.steps: step2. Set store config.
     * @tc.expected: step2.ok
     */
    EXPECT_EQ(delegate_->SetStoreConfig({DistributedTableMode::COLLABORATION}), OK);
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.ok
     */
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 10, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
}

/**
 * @tc.name: SetStoreConfig002
 * @tc.desc: Test set store config after create distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetStoreConfig002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in SPLIT_BY_DEVICE
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::SPLIT_BY_DEVICE));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Set store config.
     * @tc.expected: step2. return not support
     */
    EXPECT_EQ(delegate_->SetStoreConfig({DistributedTableMode::COLLABORATION}), NOT_SUPPORT);
}

/**
 * @tc.name: SetStoreConfig003
 * @tc.desc: Test set store config after create distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lg
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetStoreConfig003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate(DistributedTableMode::COLLABORATION));
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    /**
     * @tc.steps: step2. Set store config.
     * @tc.expected: step2. return not support
     */
    EXPECT_EQ(delegate_->SetStoreConfig({DistributedTableMode::SPLIT_BY_DEVICE}), NOT_SUPPORT);
}

/**
 * @tc.name: InvalidSync001
 * @tc.desc: Test remote set empty distributed schema and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRDBCollaborationTest, InvalidSync001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Remote device set empty distributed schema
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    DistributedSchema emptySchema;
    deviceB_->SetDistributedSchema(emptySchema);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    /**
     * @tc.steps: step2. Insert one data
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.SCHEMA_MISMATCH
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PUSH_ONLY, SCHEMA_MISMATCH,
        {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. Remove device data
     * @tc.expected: step4. NOT_SUPPORT
     */
    EXPECT_EQ(delegate_->RemoveDeviceData("dev", DEVICE_SYNC_TABLE), NOT_SUPPORT);
    EXPECT_EQ(delegate_->RemoveDeviceData(), NOT_SUPPORT);
    EXPECT_EQ(delegate_->RemoveDeviceData("dev", ClearMode::DEFAULT), NOT_SUPPORT);
}

/**
 * @tc.name: InvalidSync002
 * @tc.desc: Test remote set distributed schema but table is not exist then sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBRDBCollaborationTest, InvalidSync002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Remote device set distributed schema but table is not exist
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    DistributedSchema distributedSchema1;
    deviceB_->SetDistributedSchema(distributedSchema1);
    /**
     * @tc.steps: step2. Insert one data
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(0, 1, db_, GetTableSchema()), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device
     * @tc.expected: step3.SCHEMA_MISMATCH
     */
    Query query = Query::Select(DEVICE_SYNC_TABLE);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PUSH_ONLY, SCHEMA_MISMATCH,
        {deviceB_->GetDeviceId()});
}

/**
 * @tc.name: InvalidSync003
 * @tc.desc: Test sync of deletion when the deleted data has log locally but not found in the actual data table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, InvalidSync003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create device table and cloud table in COLLABORATION
     * @tc.expected: step1.ok
     */
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    auto schema = GetSchema();
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
    deviceB_->SetDistributedSchema(distributedSchema);
    EXPECT_EQ(delegate_->CreateDistributedTable(DEVICE_SYNC_TABLE, TableSyncType::DEVICE_COOPERATION), OK);
    EXPECT_EQ(delegate_->SetDistributedSchema(distributedSchema), OK);
    LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", DEVICE_SYNC_TABLE);
    /**
     * @tc.steps: step2. Insert 10 data
     * @tc.expected: step2.ok
     */
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 10, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Sync to real device to make data consistent
     * @tc.expected: step3.ok
     */
    Query query = Query::Select(tableSchema.name);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step4. Stop writing into log table and remove the first record locally
     * @tc.expected: step4. ok and the target scenario is created
     */
    std::string sql = "INSERT OR REPLACE INTO " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata" +
        " VALUES ('log_trigger_switch', 'false');";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    sql = "DELETE FROM " + std::string(DEVICE_SYNC_TABLE) + " WHERE pk = 0;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    sql = "INSERT OR REPLACE INTO " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata" +
        " VALUES ('log_trigger_switch', 'true');";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    /**
     * @tc.steps: step5. Delete the first data in virtual device and sync to deviceB_
     * @tc.expected: step5. ok
     */
    delegateObserver_->ClearChangedData();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema, true), E_OK);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    /**
     * @tc.steps: step6. check changeData
     * @tc.expected: step6. OP_DELETE has one record but the record is empty
     */
    auto changeData = delegateObserver_->GetSavedChangedData();
    ASSERT_EQ(changeData[tableSchema.name].primaryData[OP_DELETE].size(), 1u);
    EXPECT_EQ(changeData[tableSchema.name].primaryData[OP_DELETE][0].size(), 0u);
    ASSERT_EQ(changeData[tableSchema.name].field.size(), 1u);
    EXPECT_EQ(changeData[tableSchema.name].field[0], "pk");
}

/**
 * @tc.name: InvalidSync004
 * @tc.desc: Test sync with empty tables
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, InvalidSync004, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(InitDelegate());
    Query query = Query::Select().FromTable({});
    DBStatus callStatus = delegate_->Sync({deviceB_->GetDeviceId()}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
    EXPECT_EQ(callStatus, INVALID_ARGS);
}

/**
 * @tc.name: InvalidSync005
 * @tc.desc: Test error returned by the other end during sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, InvalidSync005, TestSize.Level1)
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
    /**
     * @tc.steps: step2. Prepare device B
     * @tc.expected: step2.ok
     */
    int dataCount = 10;
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Set return E_DISTRIBUTED_SCHEMA_NOT_FOUND when get sync data in device B
     * @tc.expected: step3.ok
     */
    deviceB_->SetGetSyncDataResult(-E_DISTRIBUTED_SCHEMA_NOT_FOUND);
    /**
     * @tc.steps: step4. Sync
     * @tc.expected: step4. return SCHEMA_MISMATCH
     */
    Query query = Query::Select().FromTable({DEVICE_SYNC_TABLE});
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, SCHEMA_MISMATCH,
        {deviceB_->GetDeviceId()});
}

/**
 * @tc.name: InvalidSync006
 * @tc.desc: Test FromTable and other predicates are combined when sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBCollaborationTest, InvalidSync006, TestSize.Level0)
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
    /**
     * @tc.steps: step2. Prepare device B
     * @tc.expected: step2.ok
     */
    int dataCount = 10;
    auto tableSchema = GetTableSchema();
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, dataCount, deviceB_, tableSchema), E_OK);
    ASSERT_EQ(RDBDataGenerator::PrepareVirtualDeviceEnv(tableSchema.name, db_, deviceB_), E_OK);
    /**
     * @tc.steps: step3. Prepare query list
     * @tc.expected: step3.ok
     */
    std::set<Key> keys = {{1}, {2}, {3}};
    std::vector<int> values = {1, 2, 3};
    std::vector<Query> queryList = {
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).BeginGroup().EqualTo("pk", 1),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).EqualTo("pk", 1),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).GreaterThan("pk", 1),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).GreaterThanOrEqualTo("pk", 1),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).In("pk", values),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).NotIn("pk", values),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).IsNotNull("pk"),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).LessThan("pk", 1),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).LessThanOrEqualTo("pk", 1),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).Like("pk", "abc"),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).NotLike("pk", "abc"),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).OrderByWriteTime(false),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).SuggestIndex("pk"),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).PrefixKey({1, 2, 3}),
        Query::Select().FromTable({DEVICE_SYNC_TABLE}).InKeys(keys)
    };
    /**
     * @tc.steps: step4. Sync
     * @tc.expected: step4. return NOT_SUPPORT
     */
    for (const auto &query : queryList) {
        DBStatus callStatus = delegate_->Sync({deviceB_->GetDeviceId()}, SYNC_MODE_PUSH_ONLY, query, nullptr, true);
        EXPECT_EQ(callStatus, NOT_SUPPORT);
    }
}
}