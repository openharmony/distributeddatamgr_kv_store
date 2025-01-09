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
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema001, TestSize.Level0)
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
    EXPECT_EQ(delegate_->SetDistributedSchema(RDBDataGenerator::ParseSchema(schema)), OK);
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
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema003, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema006, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema008, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync005, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync006, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync007, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync008, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync009, TestSize.Level0)
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
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync011, TestSize.Level0)
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
 * @tc.name: NormalSync012
 * @tc.desc: Test sync with autoincrement table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCollaborationTest, NormalSync012, TestSize.Level0)
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
    auto distributedSchema = RDBDataGenerator::ParseSchema(schema, true);
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
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
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
    EXPECT_EQ(count, 1);
    /**
     * @tc.steps: step4. Update date and sync again
     * @tc.expected: step4.ok
     */
    ASSERT_EQ(RDBDataGenerator::InsertVirtualLocalDBData(0, 1, deviceB_, tableSchema), E_OK);
    DistributedDBToolsUnitTest::BlockSync(*delegate_, query, SYNC_MODE_PULL_ONLY, OK, {deviceB_->GetDeviceId()});
    sql = std::string("select count(*) from ").append(tableSchema.name);
    count = 0;
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db_, sql, count), E_OK);
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: SetSchema013
 * @tc.desc: Test set tracker table for device table and check if timestamp has changed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRDBCollaborationTest, SetSchema013, TestSize.Level0)
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
}