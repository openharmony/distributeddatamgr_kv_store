/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <queue>
#include <random>

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "platform_specific.h"
#include "relational_store_manager.h"
#include "relational_store_sqlite_ext.h"
#include "relational_virtual_device.h"
#include "runtime_config.h"
#ifdef DB_DEBUG_ENV
#include "system_time.h"
#endif // DB_DEBUG_ENV
#include "virtual_relational_ver_sync_db_interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
constexpr const char* DB_SUFFIX = ".db";
constexpr const char* STORE_ID = "Relational_Store_ID";
std::string g_testDir;
std::string g_dbDir;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);

const std::string DEVICE_A = "real_device";
const std::string DEVICE_B = "deviceB";
VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
RelationalVirtualDevice *g_deviceB = nullptr;

const std::string NORMAL_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS sync_data(" \
    "key         BLOB NOT NULL UNIQUE," \
    "value       BLOB," \
    "timestamp   INT  NOT NULL," \
    "flag        INT  NOT NULL," \
    "device      BLOB," \
    "ori_device  BLOB," \
    "hash_key    BLOB PRIMARY KEY NOT NULL," \
    "w_timestamp INT," \
    "UNIQUE(device, ori_device));" \
    "CREATE INDEX key_index ON sync_data(key, flag);";

const std::string NORMAL_CREATE_NO_UNIQUE = "CREATE TABLE IF NOT EXISTS sync_data(" \
    "key         BLOB NOT NULL," \
    "value       BLOB," \
    "timestamp   INT  NOT NULL," \
    "flag        INT  NOT NULL," \
    "device      BLOB," \
    "ori_device  BLOB," \
    "hash_key    BLOB PRIMARY KEY NOT NULL," \
    "w_timestamp INT);" \
    "CREATE INDEX key_index ON sync_data(key, flag);";

const std::string SIMPLE_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS t1(a INT, b TEXT)";

const std::string CREATE_TABLE_SQL_NO_PRIMARY_KEY = "CREATE TABLE IF NOT EXISTS sync_data(" \
    "key         BLOB NOT NULL UNIQUE," \
    "value       BLOB," \
    "timestamp   INT  NOT NULL," \
    "flag        INT  NOT NULL," \
    "device      BLOB," \
    "ori_device  BLOB," \
    "hash_key    BLOB NOT NULL," \
    "w_timestamp INT," \
    "UNIQUE(device, ori_device));" \
    "CREATE INDEX key_index ON sync_data (key, flag);";

const std::string CREATE_TABLE_SQL_NO_PRIMARY_KEY_NO_UNIQUE = "CREATE TABLE IF NOT EXISTS sync_data(" \
    "key         BLOB NOT NULL," \
    "value       BLOB," \
    "timestamp   INT  NOT NULL," \
    "flag        INT  NOT NULL," \
    "device      BLOB," \
    "ori_device  BLOB," \
    "hash_key    BLOB NOT NULL," \
    "w_timestamp INT);" \
    "CREATE INDEX key_index ON sync_data (key, flag);";

const std::string UNSUPPORTED_FIELD_TABLE_SQL = "CREATE TABLE IF NOT EXISTS test('$.ID' INT, val BLOB);";

const std::string COMPOSITE_PRIMARY_KEY_TABLE_SQL = R"(CREATE TABLE workers (
        worker_id INTEGER,
        last_name VARCHAR NOT NULL,
        first_name VARCHAR,
        join_date DATE,
        PRIMARY KEY (last_name, first_name)
    );)";

const std::string INSERT_SYNC_DATA_SQL = "INSERT OR REPLACE INTO sync_data (key, timestamp, flag, hash_key) "
    "VALUES('KEY', 123456789, 1, 'HASH_KEY');";

const std::string INVALID_TABLE_FIELD_SQL = "create table if not exists t1 ('1 = 1; --' int primary key, b blob)";


void PrepareVirtualDeviceEnv(const std::string &tableName, const std::string &dbPath,
    const std::vector<RelationalVirtualDevice *> &remoteDeviceVec)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(dbPath);
    ASSERT_NE(db, nullptr);
    TableInfo tableInfo;
    SQLiteUtils::AnalysisSchema(db, tableName, tableInfo);
    for (const auto &dev : remoteDeviceVec) {
        std::vector<FieldInfo> fieldInfoList = tableInfo.GetFieldInfos();
        dev->SetLocalFieldInfo(fieldInfoList);
        dev->SetTableInfo(tableInfo);
    }
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

class DistributedDBInterfacesRelationalTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBInterfacesRelationalTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBInterfacesRelationalTest::TearDownTestCase(void)
{
}

void DistributedDBInterfacesRelationalTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();

    g_deviceB = new (std::nothrow) RelationalVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    auto *syncInterfaceB = new (std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
        return true;
    };
    EXPECT_EQ(RuntimeConfig::SetPermissionCheckCallback(permissionCheckCallback), OK);
}

void DistributedDBInterfacesRelationalTest::TearDown(void)
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(RuntimeConfig::SetPermissionCheckCallback(nullCallback), OK);
    if (g_communicatorAggregator != nullptr) {
        g_communicatorAggregator->RegOnDispatch(nullptr);
    }
}

void NoramlCreateDistributedTableTest(TableSyncType tableSyncType)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    if (tableSyncType == DistributedDB::DEVICE_COOPERATION) {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    } else {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_NO_UNIQUE), SQLITE_OK);
    }

    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_A");
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step2. open relational store, create distributed table, close store
     * @tc.expected: step2. Return OK.
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = delegate->CreateDistributedTable("sync_data", tableSyncType);
    EXPECT_EQ(status, OK);

    // test create same table again
    status = delegate->CreateDistributedTable("sync_data", tableSyncType);
    EXPECT_EQ(status, OK);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);

    /**
     * @tc.steps:step3. drop sync_data table
     * @tc.expected: step3. Return OK.
     */
    db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "drop table sync_data;"), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step4. open again, check auxiliary should be delete
     * @tc.expected: step4. Return OK.
     */
    delegate = nullptr;
    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: RelationalStoreTest001
  * @tc.desc: Test open store and create distributed db with DEVICE_COOPERATION type
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest001, TestSize.Level1)
{
    NoramlCreateDistributedTableTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: RelationalStoreTest001
  * @tc.desc: Test open store and create distributed db with CLOUD_COOPERATION type
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest001_1, TestSize.Level1)
{
    NoramlCreateDistributedTableTest(DistributedDB::CLOUD_COOPERATION);
}

/**
  * @tc.name: RelationalStoreTest002
  * @tc.desc: Test open store with invalid path or store ID
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step2. Test open store with invalid path or store ID
     * @tc.expected: step2. open store failed.
     */
    RelationalStoreDelegate *delegate = nullptr;

    // test open store with path not exist
    DBStatus status = g_mgr.OpenStore(g_dbDir + "tmp/" + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_NE(status, OK);
    ASSERT_EQ(delegate, nullptr);

    // test open store with empty store_id
    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, {}, {}, delegate);
    EXPECT_NE(status, OK);
    ASSERT_EQ(delegate, nullptr);

    // test open store with path has invalid character
    status = g_mgr.OpenStore(g_dbDir + "t&m$p/" + STORE_ID + DB_SUFFIX, {}, {}, delegate);
    EXPECT_NE(status, OK);
    ASSERT_EQ(delegate, nullptr);

    // test open store with store_id has invalid character
    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, "Relation@al_S$tore_ID", {}, delegate);
    EXPECT_NE(status, OK);
    ASSERT_EQ(delegate, nullptr);

    // test open store with store_id length over MAX_STORE_ID_LENGTH
    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX,
        std::string(DBConstant::MAX_STORE_ID_LENGTH + 1, 'a'), {}, delegate);
    EXPECT_NE(status, OK);
    ASSERT_EQ(delegate, nullptr);
}

/**
  * @tc.name: RelationalStoreTest003
  * @tc.desc: Test open store with journal_mode is not WAL
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare db file with string is not WAL
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=PERSIST;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step2. Test open store
     * @tc.expected: step2. Open store failed.
     */
    RelationalStoreDelegate *delegate = nullptr;

    // test open store with journal mode is not WAL
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_NE(status, OK);
    ASSERT_EQ(delegate, nullptr);
}

void CreateDistributedTableOverLimitTest(TableSyncType tableSyncTpe)
{
    /**
     * @tc.steps:step1. Prepare db file with multiple tables
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    const int tableCount = DBConstant::MAX_DISTRIBUTED_TABLE_COUNT + 10; // 10: additional size for test abnormal scene
    for (int i=0; i<tableCount; i++) {
        std::string sql = "CREATE TABLE TEST_" + std::to_string(i) + "(id INT PRIMARY KEY, value TEXT);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    }
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    RelationalStoreDelegate *delegate = nullptr;

    /**
     * @tc.steps:step2. Open store and create multiple distributed table
     * @tc.expected: step2. The tables in limited quantity were created successfully, the others failed.
     */
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    for (int i=0; i<tableCount; i++) {
        if (i < DBConstant::MAX_DISTRIBUTED_TABLE_COUNT) {
            EXPECT_EQ(delegate->CreateDistributedTable("TEST_" + std::to_string(i), tableSyncTpe), OK);
        } else {
            EXPECT_NE(delegate->CreateDistributedTable("TEST_" + std::to_string(i), tableSyncTpe), OK);
        }
    }

    /**
     * @tc.steps:step3. Close store
     * @tc.expected: step3. Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: RelationalStoreTest004
  * @tc.desc: Test create distributed table with over limit for DEVICE_COOPERATION type
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest004, TestSize.Level1)
{
    CreateDistributedTableOverLimitTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: RelationalStoreTest004
  * @tc.desc: Test create distributed table with over limit for CLOUD_COOPERATION type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest004_1, TestSize.Level1)
{
    CreateDistributedTableOverLimitTest(DistributedDB::CLOUD_COOPERATION);
}

void CreateDistributedTableInvalidArgsTest(TableSyncType tableSyncType)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);

    /**
     * @tc.steps:step2. Open store
     * @tc.expected: step2. return OK
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    /**
     * @tc.steps:step3. Create distributed table with invalid table name
     * @tc.expected: step3. Create distributed table failed.
     */
    EXPECT_NE(delegate->CreateDistributedTable(DBConstant::SYSTEM_TABLE_PREFIX + "_tmp", tableSyncType), OK);

    EXPECT_EQ(delegate->CreateDistributedTable("Handle-J@^.", tableSyncType), INVALID_ARGS);
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data",
        static_cast<TableSyncType>(DistributedDB::DEVICE_COOPERATION - 1)), INVALID_ARGS);
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data",
        static_cast<TableSyncType>(DistributedDB::CLOUD_COOPERATION + 1)), INVALID_ARGS);

    EXPECT_EQ(RelationalTestUtils::ExecSql(db, INVALID_TABLE_FIELD_SQL), SQLITE_OK);
    EXPECT_EQ(delegate->CreateDistributedTable("t1", tableSyncType), NOT_SUPPORT);

    /**
     * @tc.steps:step4. Create distributed table temp table or not exist table
     * @tc.expected: step4. Create distributed table failed.
     */
    EXPECT_EQ(delegate->CreateDistributedTable("child", tableSyncType), NOT_FOUND);
    std::string tempTableSql = "CREATE TEMP TABLE child(x, y, z)";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, tempTableSql), SQLITE_OK);
    EXPECT_EQ(delegate->CreateDistributedTable("child", tableSyncType), NOT_FOUND);

    /**
     * @tc.steps:step5. Close store
     * @tc.expected: step5. Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
  * @tc.name: RelationalStoreTest005
  * @tc.desc: Test create distributed table with invalid table name or invalid table sync type
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest005, TestSize.Level1)
{
    CreateDistributedTableInvalidArgsTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: RelationalStoreTest005
  * @tc.desc: Test create distributed table with invalid table name or invalid table sync type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest005_1, TestSize.Level1)
{
    CreateDistributedTableInvalidArgsTest(DistributedDB::CLOUD_COOPERATION);
}

void CreateDistributedTableNonPrimaryKeyTest(TableSyncType tableSyncType)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    if (tableSyncType == DistributedDB::DEVICE_COOPERATION) {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_TABLE_SQL_NO_PRIMARY_KEY), SQLITE_OK);
    } else {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_TABLE_SQL_NO_PRIMARY_KEY_NO_UNIQUE), SQLITE_OK);
    }
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step2. Open store
     * @tc.expected: step2. return OK
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    /**
     * @tc.steps:step3. Create distributed table with valid table name
     * @tc.expected: step3. Create distributed table success.
     */
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data", tableSyncType), OK);

    /**
     * @tc.steps:step4. Close store
     * @tc.expected: step4. Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    delegate = nullptr;

    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: RelationalStoreTest006
  * @tc.desc: Test create distributed table with non primary key schema
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest006, TestSize.Level1)
{
    CreateDistributedTableNonPrimaryKeyTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: RelationalStoreTest006
  * @tc.desc: Test create distributed table with non primary key schema
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest006_1, TestSize.Level1)
{
    CreateDistributedTableNonPrimaryKeyTest(DistributedDB::CLOUD_COOPERATION);
}

void CreateDistributedTableInvalidFieldTest(TableSyncType tableSyncType)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, UNSUPPORTED_FIELD_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    EXPECT_EQ(delegate->CreateDistributedTable("test", tableSyncType), NOT_SUPPORT);
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: RelationalStoreTest007
  * @tc.desc: Test create distributed table with table has invalid field name
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest007, TestSize.Level1)
{
    CreateDistributedTableInvalidFieldTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: RelationalStoreTest007
  * @tc.desc: Test create distributed table with table has invalid field name
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest007_1, TestSize.Level1)
{
    CreateDistributedTableInvalidFieldTest(DistributedDB::CLOUD_COOPERATION);
}

void CreateDistributedTableCompositePKTest(TableSyncType tableSyncType, int expectCode)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, COMPOSITE_PRIMARY_KEY_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    EXPECT_EQ(delegate->CreateDistributedTable("workers", tableSyncType), expectCode);
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: RelationalStoreTest008
  * @tc.desc: Test create distributed table with table has composite primary keys for DEVICE_COOPERATION
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest008, TestSize.Level1)
{
    CreateDistributedTableCompositePKTest(DistributedDB::DEVICE_COOPERATION, NOT_SUPPORT);
}

/**
  * @tc.name: RelationalStoreTest008
  * @tc.desc: Test create distributed table with table has composite primary keys for CLOUD_COOPERATION
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest008_1, TestSize.Level1)
{
    CreateDistributedTableCompositePKTest(DistributedDB::CLOUD_COOPERATION, OK);
}

void CreateDistributedTableWithHistoryDataTest(TableSyncType tableSyncType)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, INSERT_SYNC_DATA_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    EXPECT_EQ(delegate->CreateDistributedTable("sync_data"), OK);
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: RelationalStoreTest009
  * @tc.desc: Test create distributed table with table has history data for DEVICE_COOPERATION
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest009, TestSize.Level1)
{
    CreateDistributedTableWithHistoryDataTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: RelationalStoreTest009
  * @tc.desc: Test create distributed table with table has history data for CLOUD_COOPERATION
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalStoreTest009_1, TestSize.Level1)
{
    CreateDistributedTableWithHistoryDataTest(DistributedDB::CLOUD_COOPERATION);
}

void TableModifyTest(const std::string &modifySql, TableSyncType tableSyncType, DBStatus expect)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    if (tableSyncType == DistributedDB::DEVICE_COOPERATION) {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    } else {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_NO_UNIQUE), SQLITE_OK);
    }

    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_A");
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_B");
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_C");

    /**
     * @tc.steps:step2. Open store
     * @tc.expected: step2. return OK
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    /**
     * @tc.steps:step3. Create distributed table
     * @tc.expected: step3. Create distributed table OK.
     */
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data", tableSyncType), OK);

    /**
     * @tc.steps:step4. Upgrade table with modifySql
     * @tc.expected: step4. return OK
     */
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, modifySql), SQLITE_OK);

    /**
     * @tc.steps:step5. Create distributed table again
     * @tc.expected: step5. Create distributed table return expect.
     */
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data", tableSyncType), expect);

    /**
     * @tc.steps:step6. Close store
     * @tc.expected: step6 Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
  * @tc.name: RelationalTableModifyTest001
  * @tc.desc: Test modify distributed table with compatible upgrade
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest001, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data ADD COLUMN add_field INTEGER NOT NULL DEFAULT 123;",
        DistributedDB::DEVICE_COOPERATION, OK);
}

/**
  * @tc.name: RelationalTableModifyTest002
  * @tc.desc: Test modify distributed table with incompatible upgrade
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest002, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data ADD COLUMN add_field INTEGER NOT NULL;",
        DistributedDB::DEVICE_COOPERATION, SCHEMA_MISMATCH);
}

/**
  * @tc.name: RelationalTableModifyTest003
  * @tc.desc: Test modify distributed table with incompatible upgrade
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest003, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data DROP COLUMN w_timestamp;",
        DistributedDB::DEVICE_COOPERATION, SCHEMA_MISMATCH);
}

/**
  * @tc.name: RelationalTableModifyTest001
  * @tc.desc: Test modify distributed table with compatible upgrade
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest001_1, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data ADD COLUMN add_field INTEGER NOT NULL DEFAULT 123;",
        DistributedDB::CLOUD_COOPERATION, OK);
}

/**
  * @tc.name: RelationalTableModifyTest002
  * @tc.desc: Test modify distributed table with incompatible upgrade
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest002_1, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data ADD COLUMN add_field INTEGER NOT NULL;",
        DistributedDB::CLOUD_COOPERATION, SCHEMA_MISMATCH);
}

/**
  * @tc.name: RelationalTableModifyTest003
  * @tc.desc: Test modify distributed table with incompatible upgrade
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest003_1, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data DROP COLUMN w_timestamp;",
        DistributedDB::CLOUD_COOPERATION, SCHEMA_MISMATCH);
}

void UpgradeDistributedTableTest(TableSyncType tableSyncType)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    if (tableSyncType == DistributedDB::DEVICE_COOPERATION) {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    } else {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_NO_UNIQUE), SQLITE_OK);
    }

    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_A");
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_B");
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_C");

    /**
     * @tc.steps:step2. Open store
     * @tc.expected: step2. return OK
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    /**
     * @tc.steps:step3. Create distributed table
     * @tc.expected: step3. Create distributed table OK.
     */
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data", tableSyncType), OK);

    /**
     * @tc.steps:step4. Upgrade table
     * @tc.expected: step4. return OK
     */
    std::string modifySql = "ALTER TABLE sync_data ADD COLUMN add_field INTEGER;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, modifySql), SQLITE_OK);
    std::string indexSql = "CREATE INDEX add_index ON sync_data (add_field);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, indexSql), SQLITE_OK);
    std::string deleteIndexSql = "DROP INDEX IF EXISTS key_index";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, deleteIndexSql), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, INSERT_SYNC_DATA_SQL), SQLITE_OK);

    /**
     * @tc.steps:step5. Create distributed table again
     * @tc.expected: step5. Create distributed table return expect.
     */
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data", tableSyncType), OK);

    /**
     * @tc.steps:step6. Close store
     * @tc.expected: step6 Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
  * @tc.name: RelationalTableModifyTest004
  * @tc.desc: Test upgrade distributed table with device table exists
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest004, TestSize.Level1)
{
    UpgradeDistributedTableTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: RelationalTableModifyTest004
  * @tc.desc: Test upgrade distributed table with device table exists
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest004_1, TestSize.Level1)
{
    UpgradeDistributedTableTest(DistributedDB::CLOUD_COOPERATION);
}

/**
  * @tc.name: RelationalTableModifyTest005
  * @tc.desc: Test modify distributed table with compatible upgrade
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest005, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data ADD COLUMN add_field STRING NOT NULL DEFAULT 'asdf';",
        DistributedDB::DEVICE_COOPERATION, OK);
}

/**
  * @tc.name: RelationalTableModifyTest005
  * @tc.desc: Test modify distributed table with compatible upgrade
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalTableModifyTest005_1, TestSize.Level1)
{
    TableModifyTest("ALTER TABLE sync_data ADD COLUMN add_field STRING NOT NULL DEFAULT 'asdf';",
        DistributedDB::CLOUD_COOPERATION, OK);
}

void CheckTable(TableSyncType tableSyncType, RelationalStoreDelegate *delegate, sqlite3 *db)
{
    /**
     * @tc.steps:step4. Create distributed table with a table with "UNIQUE"
     * @tc.expected: step4. return OK or NOT_SUPPORT.
     */
    std::string tableName4 = "t4";
    std::string createSql = "create table " + tableName4 + "(id int UNIQUE);";
    createSql += "create table t4_1(id int primary key, value text UNIQUE, name int);";
    createSql += "create table t4_2(id int primary key, value text UNIQUE , name int);";
    createSql += "create table t4_3(id int primary key, value text UNIQUE );";
    createSql += "create table t4_4(id int primary key, value text UNIQUE , name int);";
    createSql += "create table t4_5(id int unique);";
    createSql += "create table t4_6(id int primary key, value text UniqUe, name int);";
    createSql += "create table t4_7(id int primary key, uniquekey text , name int);";
    createSql += "create table t4_8(id int , name text, UNIQUE(id, name));";
    createSql += "create table t4_9(id int , name text,UNIQUE(id, name));";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSql), SQLITE_OK);
    DBStatus expectCode;
    if (tableSyncType == DEVICE_COOPERATION) {
        expectCode = OK;
    } else {
        expectCode = NOT_SUPPORT;
    }
    EXPECT_EQ(delegate->CreateDistributedTable(tableName4, tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_1", tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_2", tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_3", tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_4", tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_5", tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_6", tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_7", tableSyncType), OK);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_8", tableSyncType), expectCode);
    EXPECT_EQ(delegate->CreateDistributedTable("t4_9", tableSyncType), expectCode);
}

void TableConstraintsCheck(TableSyncType tableSyncType, RelationalStoreDelegate *delegate, sqlite3 *db)
{
    /**
     * @tc.steps:step1. Create distributed table with a table with "CHECK" key word
     * @tc.expected: step1. return NOT_SUPPORT or OK.
     */
    std::string tableName1 = "t1";
    std::string createSql = "create table " + tableName1 + "(id int CHECK(id > 5));";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSql), SQLITE_OK);
    int expectCode = OK;
    if (tableSyncType == DistributedDB::CLOUD_COOPERATION) {
        expectCode = NOT_SUPPORT;
    }
    EXPECT_EQ(delegate->CreateDistributedTable(tableName1, tableSyncType), expectCode);

    /**
     * @tc.steps:step2. Create distributed table with a table with foreign key
     * @tc.expected: step2. return OK.
     */
    std::string tableName2 = "t2";
    createSql = "create table " + tableName2 + "(name text, t1_id int, FOREIGN KEY (t1_id) REFERENCES " + tableName1 +
                " (id));";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSql), SQLITE_OK);
    EXPECT_EQ(delegate->CreateDistributedTable(tableName2, tableSyncType), OK);

    CheckTable(tableSyncType, delegate, db);

    /**
     * @tc.steps:step3. Create distributed table with a table with "WITHOUT ROWID"
     * @tc.expected: step3. return NOT_SUPPORT.
     */
    std::string tableName3 = "t3";
    createSql = "create table " + tableName3 + "(id int primary key) WITHOUT ROWID;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSql), SQLITE_OK);
    EXPECT_EQ(delegate->CreateDistributedTable(tableName3, tableSyncType), NOT_SUPPORT);

    /**
     * @tc.steps:step5. Create distributed table with a table with primary key which is real
     * @tc.expected: step5. return OK or NOT_SUPPORT.
     */
    std::string tableName5 = "t5";
    createSql = "create table " + tableName5 + "(id REAL primary key);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSql), SQLITE_OK);
    if (tableSyncType == DEVICE_COOPERATION) {
        expectCode = OK;
    } else {
        expectCode = NOT_SUPPORT;
    }
    EXPECT_EQ(delegate->CreateDistributedTable(tableName5, tableSyncType), expectCode);

    /**
     * @tc.steps:step6. Create distributed table with a table with primary key which is ASSET
     * @tc.expected: step6. return OK or NOT_SUPPORT.
     */
    std::string tableName6 = "t6";
    createSql = "create table " + tableName6 + "(id ASSET primary key);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSql), SQLITE_OK);
    if (tableSyncType == DEVICE_COOPERATION) {
        expectCode = OK;
    } else {
        expectCode = NOT_SUPPORT;
    }
    EXPECT_EQ(delegate->CreateDistributedTable(tableName6, tableSyncType), expectCode);

    /**
     * @tc.steps:step7. Create distributed table with a table with primary key which is ASSETS
     * @tc.expected: step7. return OK or NOT_SUPPORT.
     */
    std::string tableName7 = "t7";
    createSql = "create table " + tableName7 + "(id assets primary key);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, createSql), SQLITE_OK);
    if (tableSyncType == DEVICE_COOPERATION) {
        expectCode = OK;
    } else {
        expectCode = NOT_SUPPORT;
    }
    EXPECT_EQ(delegate->CreateDistributedTable(tableName7, tableSyncType), expectCode);
}

void TableConstraintsTest(TableSyncType tableSyncType)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);

    /**
     * @tc.steps:step2. Open store
     * @tc.expected: step2. return OK
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    /**
     * @tc.steps:step3. check constraints
     */
    TableConstraintsCheck(tableSyncType, delegate, db);

    /**
     * @tc.steps:step4. Close store
     * @tc.expected: step4 Return OK.
     */
    EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
  * @tc.name: TableConstraintsTest001
  * @tc.desc: Test table constraints when create distributed table with DistributedDB::DEVICE_COOPERATION
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, TableConstraintsTest001, TestSize.Level1)
{
    TableConstraintsTest(DistributedDB::DEVICE_COOPERATION);
}

/**
  * @tc.name: TableConstraintsTest002
  * @tc.desc: Test table constraints when create distributed table with DistributedDB::CLOUD_COOPERATION
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, TableConstraintsTest002, TestSize.Level1)
{
    TableConstraintsTest(DistributedDB::CLOUD_COOPERATION);
}

/**
  * @tc.name: RelationalRemoveDeviceDataTest001
  * @tc.desc: Test remove device data
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalRemoveDeviceDataTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_A");
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_B");
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_C");

    /**
     * @tc.steps:step2. Open store
     * @tc.expected: step2. return OK
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    /**
     * @tc.steps:step3. Remove device data
     * @tc.expected: step3. ok
     */
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_A"), DISTRIBUTED_SCHEMA_NOT_FOUND);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_D"), DISTRIBUTED_SCHEMA_NOT_FOUND);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_A", "sync_data"), DISTRIBUTED_SCHEMA_NOT_FOUND);
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data"), OK);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_A"), OK);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_B"), OK);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_C", "sync_data"), OK);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_D"), OK);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_A", "sync_data_A"), DISTRIBUTED_SCHEMA_NOT_FOUND);

    /**
     * @tc.steps:step4. Remove device data with invalid args
     * @tc.expected: step4. invalid
     */
    EXPECT_EQ(delegate->RemoveDeviceData(""), INVALID_ARGS);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_A", "Handle-J@^."), INVALID_ARGS);

    /**
     * @tc.steps:step5. Close store
     * @tc.expected: step5 Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

struct TableT1 {
    int a;
    std::string b;
    int rowid;
    int flag;
    int timestamp;

    VirtualRowData operator() () const
    {
        VirtualRowData rowData;
        DataValue dA;
        dA = static_cast<int64_t>(a);
        rowData.objectData.PutDataValue("a", dA);
        DataValue dB;
        dB.SetText(b);
        rowData.objectData.PutDataValue("b", dB);
        rowData.logInfo.dataKey = rowid;
        rowData.logInfo.device = DEVICE_B;
        rowData.logInfo.originDev = DEVICE_B;
        rowData.logInfo.timestamp = timestamp;
        rowData.logInfo.wTimestamp = timestamp;
        rowData.logInfo.flag = flag;
        Key key;
        DBCommon::StringToVector(std::to_string(rowid), key);
        std::vector<uint8_t> hashKey;
        DBCommon::CalcValueHash(key, hashKey);
        rowData.logInfo.hashKey = hashKey;
        return rowData;
    }
};

void AddDeviceSchema(RelationalVirtualDevice *device, sqlite3 *db, const std::string &name)
{
    TableInfo table;
    SQLiteUtils::AnalysisSchema(db, name, table);
    device->SetLocalFieldInfo(table.GetFieldInfos());
    device->SetTableInfo(table);
}

void AddErrorTrigger(sqlite3 *db, const std::string &name)
{
    ASSERT_NE(db, nullptr);
    ASSERT_NE(name, "");
    std::string sql = "CREATE TRIGGER IF NOT EXISTS "
        "naturalbase_rdb_aux_" + name + "_log_ON_DELETE BEFORE DELETE \n"
        "ON naturalbase_rdb_aux_" + name + "_log \n"
        "BEGIN \n"
        "\t INSERT INTO naturalbase_rdb_aux_" + name + "_log VALUES(no_exist_func(), '', '', '', '', '', ''); \n"
        "END;";
    char *errMsg = nullptr;
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg), SQLITE_OK);
    if (errMsg != nullptr) {
        LOGE("sql error %s", errMsg);
        LOGE("sql %s", sql.c_str());
    }
}

/**
  * @tc.name: RelationalRemoveDeviceDataTest002
  * @tc.desc: Test remove device data and syn
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalRemoveDeviceDataTest002, TestSize.Level1)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, SIMPLE_CREATE_TABLE_SQL), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "t1");

    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    EXPECT_EQ(delegate->CreateDistributedTable("t1"), OK);

    g_deviceB->PutDeviceData("t1", std::vector<TableT1>{
        {1, "111", 1, 2, 1}, // test data
        {2, "222", 2, 2, 2}, // test data
        {3, "333", 3, 2, 3}, // test data
        {4, "444", 4, 2, 4} // test data
    });
    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("t1").NotEqualTo("a", 0);
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
            EXPECT_EQ(devicesMap.at(DEVICE_B)[0].status, OK);
        }, true);
    EXPECT_EQ(status, OK);

    EXPECT_EQ(delegate->RemoveDeviceData(DEVICE_B), OK);

    int logCnt = -1;
    std::string checkLogSql = "SELECT count(*) FROM naturalbase_rdb_aux_t1_log";
    RelationalTestUtils::ExecSql(db, checkLogSql, nullptr, [&logCnt](sqlite3_stmt *stmt) {
        logCnt = sqlite3_column_int(stmt, 0);
        return E_OK;
    });
    EXPECT_EQ(logCnt, 0);

    int dataCnt = -1;
    std::string deviceTable = g_mgr.GetDistributedTableName(DEVICE_B, "t1");
    std::string checkDataSql = "SELECT count(*) FROM " + deviceTable;
    RelationalTestUtils::ExecSql(db, checkDataSql, nullptr, [&dataCnt](sqlite3_stmt *stmt) {
        dataCnt = sqlite3_column_int(stmt, 0);
        return E_OK;
    });
    EXPECT_EQ(logCnt, 0);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

void TestRemoveDeviceDataWithCallback(bool removeAll)
{
    /**
     * @tc.steps:step1. Prepare db and data
     * @tc.expected: step1. Return OK.
     */
    RuntimeConfig::SetTranslateToDeviceIdCallback([](const std::string &oriDevId, const StoreInfo &info) {
        return oriDevId + "_" + info.appId;
    });
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, SIMPLE_CREATE_TABLE_SQL), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "t1");
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->CreateDistributedTable("t1"), OK);
    g_deviceB->PutDeviceData("t1", std::vector<TableT1> {
        {1, "111", 1, 0, 1} // test data
    });
    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("t1").EqualTo("a", 1);
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            ASSERT_EQ(devicesMap.size(), devices.size());
            EXPECT_EQ(devicesMap.at(DEVICE_B)[0].status, OK);
        }, true);
    EXPECT_EQ(status, OK);
    /**
     * @tc.steps:step2. remove device data and check table
     * @tc.expected: step2. dev table not exist and log not exist device b.
     */
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (removeAll) {
        EXPECT_EQ(delegate->RemoveDeviceData(), OK);
    } else {
        EXPECT_EQ(delegate->RemoveDeviceData(DEVICE_B + "_" + APP_ID), OK);
    }
    int logCnt = -1;
    std::string checkLogSql = "SELECT count(*) FROM naturalbase_rdb_aux_t1_log";
    RelationalTestUtils::ExecSql(db, checkLogSql, nullptr, [&logCnt](sqlite3_stmt *stmt) {
        logCnt = sqlite3_column_int(stmt, 0);
        return E_OK;
    });
    EXPECT_EQ(logCnt, 0);
    std::string deviceTable = RelationalStoreManager::GetDistributedTableName(DEVICE_B + "_" + APP_ID, "t1");
    std::string checkDataSql = "SELECT count(*) FROM " + deviceTable;
    EXPECT_NE(RelationalTestUtils::ExecSql(db, checkDataSql, nullptr, nullptr), SQLITE_OK);
    /**
     * @tc.steps:step3. close db
     * @tc.expected: step3. Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    RuntimeConfig::SetTranslateToDeviceIdCallback(nullptr);
}

/**
  * @tc.name: RelationalRemoveDeviceDataTest003
  * @tc.desc: Test remove all device data and sync again
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalRemoveDeviceDataTest003, TestSize.Level1)
{
    TestRemoveDeviceDataWithCallback(true);
}

/**
  * @tc.name: RelationalRemoveDeviceDataTest004
  * @tc.desc: Test remove one device data and sync again
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalRemoveDeviceDataTest004, TestSize.Level1)
{
    TestRemoveDeviceDataWithCallback(false);
}

/**
  * @tc.name: RelationalRemoveDeviceDataTest005
  * @tc.desc: Test remove device data with invalid param
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalRemoveDeviceDataTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_A");
    AddDeviceSchema(g_deviceB, db, "sync_data");
    /**
     * @tc.steps:step2. Open store
     * @tc.expected: step2. return OK
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);
    int count = 0;
    RuntimeConfig::SetTranslateToDeviceIdCallback([&count](const std::string &oriDevId, const StoreInfo &info) {
        count++;
        return oriDevId + "_" + info.appId;
    });
    /**
     * @tc.steps:step3. Remove not exist device data
     * @tc.expected: step3. ok
     */
    EXPECT_EQ(delegate->CreateDistributedTable("sync_data"), OK);
    EXPECT_EQ(delegate->RemoveDeviceData("DEVICE_C", "sync_data"), OK);
    EXPECT_EQ(count, 0);
    /**
     * @tc.steps:step4. Close store
     * @tc.expected: step4 Return OK.
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    RuntimeConfig::SetTranslateToDeviceIdCallback(nullptr);
}

/**
  * @tc.name: RelationalRemoveDeviceDataTest006
  * @tc.desc: Test remove device data with busy
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalRemoveDeviceDataTest006, TestSize.Level1)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "CREATE TABLE IF NOT EXISTS t2(a INT, b TEXT)"), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "t2");

    RelationalStoreDelegate *delegate = nullptr;
    auto observer = new (std::nothrow) RelationalStoreObserverUnitTest();
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID,
        { .observer = observer }, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    EXPECT_EQ(delegate->CreateDistributedTable("t2"), OK);

    g_deviceB->PutDeviceData("t2", std::vector<TableT1>{
        {1, "111", 1, 2, 1}, // test data
    });
    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("t2");
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query, nullptr, true);
    EXPECT_EQ(status, OK);

    auto beforeCallCount = observer->GetCallCount();
    AddErrorTrigger(db, "t2");
    EXPECT_EQ(delegate->RemoveDeviceData(DEVICE_B), DB_ERROR);

    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query, nullptr, true);
    EXPECT_EQ(status, OK);
    auto afterCallCount = observer->GetCallCount();
    EXPECT_NE(beforeCallCount, afterCallCount);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    delete observer;
}

/**
  * @tc.name: RelationalOpenStorePathCheckTest001
  * @tc.desc: Test open store with same label but different path.
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalOpenStorePathCheckTest001, TestSize.Level1)
{
    std::string dir1 = g_dbDir + "dbDir1";
    EXPECT_EQ(OS::MakeDBDirectory(dir1), E_OK);
    sqlite3 *db1 = RelationalTestUtils::CreateDataBase(dir1 + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db1, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db1, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db1, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db1), SQLITE_OK);

    std::string dir2 = g_dbDir + "dbDir2";
    EXPECT_EQ(OS::MakeDBDirectory(dir2), E_OK);
    sqlite3 *db2 = RelationalTestUtils::CreateDataBase(dir2 + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db2, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db2, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db2, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db2), SQLITE_OK);

    DBStatus status = OK;
    RelationalStoreDelegate *delegate1 = nullptr;
    status = g_mgr.OpenStore(dir1 + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate1);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate1, nullptr);

    RelationalStoreDelegate *delegate2 = nullptr;
    status = g_mgr.OpenStore(dir2 + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate2);
    EXPECT_EQ(status, INVALID_ARGS);
    ASSERT_EQ(delegate2, nullptr);

    status = g_mgr.CloseStore(delegate1);
    EXPECT_EQ(status, OK);

    status = g_mgr.CloseStore(delegate2);
    EXPECT_EQ(status, INVALID_ARGS);
}

HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalOpenStorePressureTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    DBStatus status = OK;
    for (int i = 0; i < 1000; i++) {
        RelationalStoreDelegate *delegate = nullptr;
        status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
        EXPECT_EQ(status, OK);
        ASSERT_NE(delegate, nullptr);

        status = g_mgr.CloseStore(delegate);
        EXPECT_EQ(status, OK);
        delegate = nullptr;
    }
}

HWTEST_F(DistributedDBInterfacesRelationalTest, RelationalOpenStorePressureTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    std::queue<RelationalStoreDelegate *> delegateQueue;
    std::mutex queueLock;
    std::random_device rd;
    default_random_engine e(rd());
    uniform_int_distribution<unsigned> u(0, 9);

    std::thread openStoreThread([&, this]() {
        for (int i = 0; i < 1000; i++) {
            LOGD("++++> open store delegate: %d", i);
            RelationalStoreDelegate *delegate = nullptr;
            DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
            EXPECT_EQ(status, OK);
            ASSERT_NE(delegate, nullptr);
            {
                std::lock_guard<std::mutex> lock(queueLock);
                delegateQueue.push(delegate);
            }
            LOGD("++++< open store delegate: %d", i);
        }
    });

    int cnt = 0;
    while (cnt < 1000) {
        RelationalStoreDelegate *delegate = nullptr;
        {
            std::lock_guard<std::mutex> lock(queueLock);
            if (delegateQueue.empty()) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            delegate = delegateQueue.front();
            delegateQueue.pop();
        }
        LOGD("++++> close store delegate: %d", cnt);
        DBStatus status = g_mgr.CloseStore(delegate);
        LOGD("++++< close store delegate: %d", cnt);
        EXPECT_EQ(status, OK);
        delegate = nullptr;
        cnt++;
        std::this_thread::sleep_for(std::chrono::microseconds(100 * u(e)));
    }
    openStoreThread.join();
}

namespace {
    void ProcessSync(RelationalStoreDelegate *delegate)
    {
        std::vector<std::string> devices = {DEVICE_B};
        Query query = Query::Select("create").EqualTo("create", 1);
        DBStatus status = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
            [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
                EXPECT_EQ(devicesMap.size(), devices.size());
                EXPECT_EQ(devicesMap.at(DEVICE_B)[0].status, OK);
            }, true);
        EXPECT_EQ(status, OK);

        std::vector<VirtualRowData> data;
        g_deviceB->GetAllSyncData("create", data);
        EXPECT_EQ(data.size(), 1u);

        VirtualRowData virtualRowData;
        DataValue d1;
        d1 = static_cast<int64_t>(2); // 2: test data
        virtualRowData.objectData.PutDataValue("create", d1);
        DataValue d2;
        d2.SetText("hello");
        virtualRowData.objectData.PutDataValue("ddd", d2);
        DataValue d3;
        d3.SetText("hello");
        virtualRowData.objectData.PutDataValue("eee", d3);
        virtualRowData.logInfo.timestamp = 1;
        g_deviceB->PutData("create", {virtualRowData});
        status = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query,
            [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
                EXPECT_EQ(devicesMap.size(), devices.size());
                EXPECT_EQ(devicesMap.at(DEVICE_B)[0].status, OK);
            }, true);
        EXPECT_EQ(status, OK);
    }
}

/**
  * @tc.name: SqliteKeyWord001
  * @tc.desc: Test relational table with sqlite key world.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, SqliteKeyWordTest001, TestSize.Level1)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);

    std::string tableSql = "CREATE TABLE IF NOT EXISTS 'create' ('create' INTEGER PRIMARY KEY, b 'CREATE', " \
    "c TEXT DEFAULT 'DEFAULT', UNIQUE(b, c))";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, tableSql), SQLITE_OK);
    std::string indexSql = "CREATE INDEX IF NOT EXISTS 'index' on 'create' (b)";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, indexSql), SQLITE_OK);

    PrepareVirtualDeviceEnv("create", g_dbDir + STORE_ID + DB_SUFFIX, {g_deviceB});

    DBStatus status = OK;
    RelationalStoreDelegate *delegate = nullptr;
    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = delegate->CreateDistributedTable("create");
    EXPECT_EQ(status, OK);

    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "INSERT INTO 'create' values(1, 'bbb', 'ccc')"), SQLITE_OK);

    std::string insertSql = "INSERT INTO 'create' values(1001, 'create', 'text')";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, insertSql), SQLITE_OK);

    std::string updateSql = "UPDATE 'create' SET 'create' = 1002 WHERE b = 'create'";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, updateSql), SQLITE_OK);

    std::string deleteSql = "DELETE FROM 'create' WHERE 'create' = 1002";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, deleteSql), SQLITE_OK);

    ProcessSync(delegate);

    std::string alterSql = "ALTER TABLE 'create' ADD COLUMN 'table' 'insert' DEFAULT NULL";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, alterSql), SQLITE_OK);
    status = delegate->CreateDistributedTable("create");
    EXPECT_EQ(status, OK);

    status = delegate->RemoveDeviceData("DEVICES_B", "create");
    EXPECT_EQ(status, OK);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    delegate = nullptr;
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
  * @tc.name: GetDistributedTableName001
  * @tc.desc: Test get distributed table name
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, GetDistributedTableName001, TestSize.Level1)
{
    const std::string deviceName = "DEVICES_A";
    const std::string tableName = "TABLE";
    const std::string hashDev = DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceName));

    std::string devTableName = RelationalStoreManager::GetDistributedTableName(deviceName, tableName);
    EXPECT_EQ(devTableName, DBConstant::RELATIONAL_PREFIX + tableName + "_" + hashDev);
    RuntimeConfig::SetTranslateToDeviceIdCallback([](const std::string &oriDevId, const StoreInfo &info) {
        EXPECT_EQ(info.appId, "");
        return oriDevId;
    });
    devTableName = RelationalStoreManager::GetDistributedTableName(deviceName, tableName);
    EXPECT_EQ(devTableName, DBConstant::RELATIONAL_PREFIX + tableName + "_" + deviceName);
    devTableName = RelationalStoreManager::GetDistributedTableName("", tableName);
    EXPECT_EQ(devTableName, DBConstant::RELATIONAL_PREFIX + tableName + "_");
    RuntimeConfig::SetTranslateToDeviceIdCallback(nullptr);
}

/**
  * @tc.name: CloudRelationalStoreTest001
  * @tc.desc: Test create distributed table in cloud table sync type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshjie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, CreateDistributedTableTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_NO_UNIQUE), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step2. open relational store, create distributed table with CLOUD_COOPERATION
     * @tc.expected: step2. Return OK.
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = delegate->CreateDistributedTable("sync_data", DistributedDB::CLOUD_COOPERATION);
    EXPECT_EQ(status, OK);

    /**
     * @tc.steps:step3. open relational store, create distributed table with CLOUD_COOPERATION again
     * @tc.expected: step3. Return OK.
     */
    status = delegate->CreateDistributedTable("sync_data", DistributedDB::CLOUD_COOPERATION);
    EXPECT_EQ(status, OK);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: CloudRelationalStoreTest002
  * @tc.desc: Test create distributed table in diff table sync type for the same table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshjie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, CreateDistributedTableTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare db file
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_NO_UNIQUE), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step2. open relational store, create distributed table with DEVICE_COOPERATION
     * @tc.expected: step2. Return OK.
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = delegate->CreateDistributedTable("sync_data", DistributedDB::DEVICE_COOPERATION);
    EXPECT_EQ(status, OK);

    /**
     * @tc.steps:step3. create distributed table with CLOUD_COOPERATION again
     * @tc.expected: step3. Return TYPE_MISMATCH.
     */
    status = delegate->CreateDistributedTable("sync_data", DistributedDB::CLOUD_COOPERATION);
    EXPECT_EQ(status, TYPE_MISMATCH);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    delegate = nullptr;

    /**
     * @tc.steps:step4. drop table sync_data and create again
     * @tc.expected: step4. Return OK.
     */
    db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    const std::string dropSql = "drop table sync_data;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, dropSql), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step5. open relational store, create distributed table with CLOUD_COOPERATION
     * @tc.expected: step5. Return OK.
     */
    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_NO_UNIQUE), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    EXPECT_EQ(delegate->CreateDistributedTable("sync_data", DistributedDB::CLOUD_COOPERATION), OK);

    /**
     * @tc.steps:step6. create distributed table with DEVICE_COOPERATION again
     * @tc.expected: step6. Return TYPE_MISMATCH.
     */
    status = delegate->CreateDistributedTable("sync_data", DistributedDB::DEVICE_COOPERATION);
    EXPECT_EQ(status, TYPE_MISMATCH);

    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    delegate = nullptr;
}

/**
  * @tc.name: CreateDistributedTableTest003
  * @tc.desc: Test create distributed table after rename table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, CreateDistributedTableTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare db file
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, SIMPLE_CREATE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    /**
     * @tc.steps:step2. open relational store, create distributed table with default mode
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = delegate->CreateDistributedTable("t1");
    EXPECT_EQ(status, OK);
    /**
     * @tc.steps:step3. rename table
     */
    db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "alter table t1 rename to t2;"), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    /**
     * @tc.steps:step4. reopen delegate
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    delegate = nullptr;
    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);
    /**
     * @tc.steps:step5. create distributed table and insert data ok
     */
    status = delegate->CreateDistributedTable("t2");
    EXPECT_EQ(status, OK);

    db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "insert into t2 values(1, '2');"), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    /**
     * @tc.steps:step6. close store
     */
    status = g_mgr.CloseStore(delegate);
    EXPECT_EQ(status, OK);
    delegate = nullptr;
}

/**
  * @tc.name: CreateDistributedTableTest004
  * @tc.desc: Test create distributed table will violate the constraint when table contains "_rowid_" column
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, CreateDistributedTableTest004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare db and table
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string t1 = "t1";
    std::string sql = "create table " + t1 + "(key text, _rowid_ int);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    std::string t2 = "t2";
    sql = "create table " + t2 + "(rowid int);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    std::string t3 = "t3";
    sql = "create table " + t3 + "(oid int);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    std::string t4 = "t4";
    sql = "create table " + t4 + "(rowid int, oid int, _rowid_ text);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    std::string t5 = "t5";
    sql = "create table " + t5 + "(_RoWiD_ int);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

    /**
     * @tc.steps:step2. open relational store, create distributed table
     */
    RelationalStoreDelegate *delegate = nullptr;
    EXPECT_EQ(g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate), OK);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->CreateDistributedTable(t1), NOT_SUPPORT);
    EXPECT_EQ(delegate->CreateDistributedTable(t1, DistributedDB::CLOUD_COOPERATION), NOT_SUPPORT);
    EXPECT_EQ(delegate->CreateDistributedTable(t2), OK);
    EXPECT_EQ(delegate->CreateDistributedTable(t3), OK);
    EXPECT_EQ(delegate->CreateDistributedTable(t4), NOT_SUPPORT);
    EXPECT_EQ(delegate->CreateDistributedTable(t5), NOT_SUPPORT);

    EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
    delegate = nullptr;
}

/**
  * @tc.name: CreateDistributedTableTest005
  * @tc.desc: Test create distributed table again will return ok when rebuild table(miss one field)
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesRelationalTest, CreateDistributedTableTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare db and table
     * @tc.expected: step1. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string t1 = "t1";
    std::string sql = "create table " + t1 + "(key int, value text);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);

    /**
     * @tc.steps:step2. open relational store, create distributed table with default mode
     * @tc.expected: step2. Return OK.
     */
    RelationalStoreDelegate *delegate = nullptr;
    EXPECT_EQ(g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate), OK);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->CreateDistributedTable("t1"), OK);

    /**
     * @tc.steps:step3. drop t1, rebuild t1(miss one column), then reopen store, create distributed table
     * @tc.expected: step3. Return OK.
     */
    sql = "drop table " + t1;
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    sql = "create table " + t1 + "(key int);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
    delegate = nullptr;
    EXPECT_EQ(g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate), OK);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->CreateDistributedTable("t1"), OK);

    /**
     * @tc.steps:step4. close store
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
    delegate = nullptr;
}
}
