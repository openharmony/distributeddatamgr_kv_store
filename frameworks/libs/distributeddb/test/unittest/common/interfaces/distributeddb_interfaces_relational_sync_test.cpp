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

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_store_manager.h"
#include "relational_virtual_device.h"
#include "runtime_config.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char* DB_SUFFIX = ".db";
    constexpr const char* STORE_ID = "Relational_Store_ID";
    const std::string DEVICE_A = "DEVICE_A";
    const std::string DEVICE_B = "DEVICE_B";
    std::string g_testDir;
    std::string g_dbDir;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
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
        "CREATE INDEX key_index ON sync_data (key, flag);";

    const std::string EMPTY_COLUMN_TYPE_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS student(" \
        "id         INTEGER NOT NULL UNIQUE," \
        "name       TEXT," \
        "field_1);";

    const std::string NORMAL_CREATE_TABLE_SQL_STUDENT = R""(create table student_1 (
            id      INTEGER PRIMARY KEY,
            name    STRING,
            level   INTEGER,
            score   INTEGER
        ))"";

    const std::string NORMAL_CREATE_TABLE_SQL_STUDENT_IN_ORDER = R""(create table student_1 (
            id      INTEGER PRIMARY KEY,
            name    STRING,
            score   INTEGER,
            level   INTEGER
        ))"";

    const std::string ALL_FIELD_TYPE_TABLE_SQL = R""(CREATE TABLE IF NOT EXISTS tbl_all_type(
        id INTEGER PRIMARY KEY,
        f_int INT,
        f_real REAL,
        f_text TEXT,
        f_blob BLOB,
        f_none
    ))"";

    void FakeOldVersionDB(sqlite3 *db)
    {
        std::string dropLogTable = "drop table naturalbase_rdb_aux_student_1_log;";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, dropLogTable), SQLITE_OK);
        dropLogTable = "drop table naturalbase_rdb_aux_sync_data_log;";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, dropLogTable), SQLITE_OK);

        std::string createLogTable = "CREATE TABLE naturalbase_rdb_aux_student_1_log(" \
            "data_key    INT NOT NULL," \
            "device      BLOB," \
            "ori_device  BLOB," \
            "timestamp   INT  NOT NULL," \
            "wtimestamp  INT  NOT NULL," \
            "flag        INT  NOT NULL," \
            "hash_key    BLOB NOT NULL," \
            "PRIMARY KEY(hash_key));";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, createLogTable), SQLITE_OK);

        createLogTable = "CREATE TABLE naturalbase_rdb_aux_sync_data_log(" \
            "data_key    INT NOT NULL," \
            "device      BLOB," \
            "ori_device  BLOB," \
            "timestamp   INT  NOT NULL," \
            "wtimestamp  INT  NOT NULL," \
            "flag        INT  NOT NULL," \
            "hash_key    BLOB NOT NULL," \
            "PRIMARY KEY(hash_key));";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, createLogTable), SQLITE_OK);

        std::string dropTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_student_1_ON_UPDATE;";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, dropTrigger), SQLITE_OK);

        std::string oldTrigger = "CREATE TRIGGER naturalbase_rdb_student_1_ON_UPDATE AFTER UPDATE \n"
            "ON student_1\n"
            "BEGIN\n"
            "\t UPDATE naturalbase_rdb_aux_student_1_log SET timestamp=get_sys_time(0), device='', "
            "flag=0x22 WHERE hash_key=calc_hash(OLD.id) AND flag&0x02=0x02;\n"
            "END;";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, oldTrigger), SQLITE_OK);
        Key key;
        DBCommon::StringToVector("log_table_version", key);
        Value val;
        DBCommon::StringToVector("1.0", val);
        EXPECT_EQ(RelationalTestUtils::SetMetaData(db, key, val), SQLITE_OK);
    }

    void AddDeviceSchema(RelationalVirtualDevice *device, sqlite3 *db, const std::string &name)
    {
        TableInfo table;
        SQLiteUtils::AnalysisSchema(db, name, table);

        std::vector<FieldInfo> fieldList;
        for (const auto &it : table.GetFields()) {
            fieldList.push_back(it.second);
        }
        device->SetLocalFieldInfo(table.GetFieldInfos());
        device->SetTableInfo(table);
    }
}

class DistributedDBInterfacesRelationalSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
protected:
    sqlite3 *db = nullptr;
    RelationalStoreDelegate *delegate = nullptr;
};

void DistributedDBInterfacesRelationalSyncTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBInterfacesRelationalSyncTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBInterfacesRelationalSyncTest::SetUp()
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

    db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", DEVICE_A);
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = delegate->CreateDistributedTable("sync_data");
    EXPECT_EQ(status, OK);

    AddDeviceSchema(g_deviceB, db, "sync_data");
}

void DistributedDBInterfacesRelationalSyncTest::TearDown()
{
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(RuntimeConfig::SetPermissionCheckCallback(nullCallback), OK);

    g_mgr.CloseStore(delegate);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

/**
  * @tc.name: RelationalSyncTest001
  * @tc.desc: Test with sync interface, table is not a distributed table
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest001, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_datb");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, DISTRIBUTED_SCHEMA_NOT_FOUND);
}

/**
  * @tc.name: RelationalSyncTest002
  * @tc.desc: Test with sync interface, query is not support
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest002, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data").Like("value", "abc");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, NOT_SUPPORT);
}

/**
  * @tc.name: RelationalSyncTest003
  * @tc.desc: Test with sync interface, query is invalid format
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest003, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data").And().Or().EqualTo("flag", 2);
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, INVALID_QUERY_FORMAT);
}

/**
  * @tc.name: RelationalSyncTest004
  * @tc.desc: Test with sync interface, query use invalid field
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest004, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data").EqualTo("fleg", 2);
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, INVALID_QUERY_FIELD);
}

/**
  * @tc.name: RelationalSyncTest005
  * @tc.desc: Test with sync interface, query table has been modified
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest005, TestSize.Level1)
{
    std::string modifySql = "ALTER TABLE sync_data ADD COLUMN add_field INTEGER;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, modifySql), SQLITE_OK);

    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, DISTRIBUTED_SCHEMA_CHANGED);
}

/**
  * @tc.name: RelationalSyncTest006
  * @tc.desc: Test with sync interface, query is not set table name
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest006, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select();
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, NOT_SUPPORT);
}

/**
  * @tc.name: RelationalSyncTest007
  * @tc.desc: Test with sync interface, distributed table has empty column type
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest007, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, EMPTY_COLUMN_TYPE_CREATE_TABLE_SQL), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "student", DEVICE_A);

    DBStatus status = delegate->CreateDistributedTable("student");
    EXPECT_EQ(status, OK);

    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("student");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, OK);
}

/**
  * @tc.name: RelationalSyncTest008
  * @tc.desc: Test sync with rebuilt table
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest008, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Drop sync_data
     * @tc.expected: step1. ok
     */
    std::string dropSql = "DROP TABLE IF EXISTS sync_data;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, dropSql), SQLITE_OK);

    /**
     * @tc.steps:step2. sync with sync_data
     * @tc.expected: step2. return INVALID_QUERY_FORMAT
     */
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, DISTRIBUTED_SCHEMA_CHANGED);

    /**
     * @tc.steps:step3. recreate sync_data
     * @tc.expected: step3. ok
     */
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    DBStatus status = delegate->CreateDistributedTable("sync_data");
    EXPECT_EQ(status, OK);

    /**
     * @tc.steps:step4. Check trigger
     * @tc.expected: step4. trigger exists
     */
    bool result = false;
    std::string checkSql = "select * from sqlite_master where type = 'trigger' and tbl_name = 'sync_data';";
    EXPECT_EQ(RelationalTestUtils::CheckSqlResult(db, checkSql, result), E_OK);
    EXPECT_EQ(result, true);

    /**
     * @tc.steps:step5. sync with sync_data
     * @tc.expected: step5. ok
     */
    errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, OK);
}

/**
  * @tc.name: RelationalSyncTest009
  * @tc.desc: Test sync with invalid query
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest009, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, EMPTY_COLUMN_TYPE_CREATE_TABLE_SQL), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "student", DEVICE_A);

    DBStatus status = delegate->CreateDistributedTable("student");
    EXPECT_EQ(status, OK);

    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("student").EqualTo("$id", 123);
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(status, INVALID_QUERY_FORMAT);

    query = Query::Select("student").EqualTo("A$id", 123);
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(status, INVALID_QUERY_FORMAT);

    query = Query::Select("student").EqualTo("$.id", 123);
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: RelationalSyncTest010
  * @tc.desc: Test sync with shcema changed
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RelationalSyncTest010, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, OK);

    std::string modifySql = "DROP TABLE IF EXISTS sync_data;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, modifySql), SQLITE_OK);

    errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, DISTRIBUTED_SCHEMA_CHANGED);

    errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, DISTRIBUTED_SCHEMA_CHANGED);
}

/**
  * @tc.name: UpdatePrimaryKeyTest001
  * @tc.desc: Test update data's primary key
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, UpdatePrimaryKeyTest001, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "student_1", DEVICE_A);

    DBStatus status = delegate->CreateDistributedTable("student_1");
    EXPECT_EQ(status, OK);

    std::string insertSql = "insert into student_1 (id, name, level, score) values (1001, 'xue', 2, 95);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, insertSql), SQLITE_OK);

    std::string updateSql = "update student_1 set id = 1002 where name = 'xue';";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, updateSql), SQLITE_OK);

    int cnt = RelationalTestUtils::CheckTableRecords(db, DBConstant::RELATIONAL_PREFIX + "student_1" + "_log");
    EXPECT_EQ(cnt, 2);
}

/**
  * @tc.name: UpgradeTriggerTest001
  * @tc.desc: Test upgrade from old version
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, UpgradeTriggerTest001, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "student_1", DEVICE_A);

    DBStatus status = delegate->CreateDistributedTable("student_1");
    EXPECT_EQ(status, OK);

    EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
    delegate = nullptr;

    FakeOldVersionDB(db);

    status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    // checkTrigger
    std::string resultTrigger;
    int errCode = RelationalTestUtils::ExecSql(db, "SELECT sql FROM sqlite_master WHERE type = ? AND name = ?",
        [](sqlite3_stmt *stmt) {
            (void)SQLiteUtils::BindTextToStatement(stmt, 1, "trigger"); // 1: bind index
            (void)SQLiteUtils::BindTextToStatement(stmt, 2, "naturalbase_rdb_student_1_ON_UPDATE"); // 2: bind index
            return E_OK;
        }, [&resultTrigger](sqlite3_stmt *stmt) {
            (void)SQLiteUtils::GetColumnTextValue(stmt, 0, resultTrigger);
            return E_OK;
        });
    EXPECT_EQ(errCode, E_OK);
    LOGD("result trigger: %s", resultTrigger.c_str());
    std::string expectTrigger = "CREATE TRIGGER naturalbase_rdb_student_1_ON_UPDATE AFTER UPDATE \n"
        "ON 'student_1'\n"
        "BEGIN\n"
        "\t UPDATE naturalbase_rdb_aux_student_1_log SET data_key=-1,timestamp=get_sys_time(0), device='',"
        " flag=0x03 WHERE hash_key=calc_hash(OLD.'id', 0) AND flag&0x02=0x02;\n"
        "\t INSERT OR REPLACE INTO naturalbase_rdb_aux_student_1_log VALUES (NEW._rowid_, '', '', get_sys_time(0), "
        "get_last_time(), CASE WHEN (calc_hash(NEW.'id', 0) != calc_hash(NEW.'id', 0)) " \
        "THEN 0x02 ELSE 0x22 END, calc_hash(NEW.'id', 0), '', '', '', '', '', 0);\n"
        "END";
    EXPECT_TRUE(resultTrigger == expectTrigger);
}


namespace {
void PrepareSyncData(sqlite3 *db, int dataSize)
{
    int i = 1;
    std::string insertSql = "INSERT INTO sync_data VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
    int ret = RelationalTestUtils::ExecSql(db, insertSql, [&i, dataSize] (sqlite3_stmt *stmt) {
        SQLiteUtils::BindTextToStatement(stmt, 1, "KEY_" + std::to_string(i));
        SQLiteUtils::BindTextToStatement(stmt, 2, "VAL_" + std::to_string(i));
        sqlite3_bind_int64(stmt, 3, 1000000 - (1000 + i));
        sqlite3_bind_int64(stmt, 4, i % 4);
        SQLiteUtils::BindTextToStatement(stmt, 5, "DEV_" + std::to_string(i));
        SQLiteUtils::BindTextToStatement(stmt, 6, "KEY_" + std::to_string(i));
        SQLiteUtils::BindTextToStatement(stmt, 7, "HASHKEY_" + std::to_string(i));
        sqlite3_bind_int64(stmt, 8, 1000 + i);
        return (i++ == dataSize) ? E_OK : -E_UNFINISHED;
    }, nullptr);
    EXPECT_EQ(ret, E_OK);
}

std::string GetKey(VirtualRowData rowData)
{
    DataValue dataVal;
    rowData.objectData.GetDataValue("key", dataVal);
    EXPECT_EQ(dataVal.GetType(), StorageType::STORAGE_TYPE_TEXT);
    std::string dataStr;
    EXPECT_EQ(dataVal.GetText(dataStr), E_OK);
    return dataStr;
}

void CheckSyncData(sqlite3 *db, const std::string &checkSql, const std::vector<VirtualRowData> &resultData)
{
    std::set<std::string> keySet;
    keySet.clear();
    for (size_t i = 0; i < resultData.size(); i++) {
        std::string ss = GetKey(resultData[i]);
        keySet.insert(ss);
    }
    EXPECT_EQ(keySet.size(), resultData.size());

    RelationalTestUtils::ExecSql(db, checkSql, nullptr, [keySet](sqlite3_stmt *stmt) {
        std::string val;
        SQLiteUtils::GetColumnTextValue(stmt, 0, val);
        EXPECT_NE(keySet.find(val), keySet.end());
        return E_OK;
    });
}
}

/**
  * @tc.name: SyncLimitTest001
  * @tc.desc: Sync device with limit query
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, SyncLimitTest001, TestSize.Level3)
{
    PrepareSyncData(db, 5000);
    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sync_data").Limit(4500, 100);
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, OK);

    std::vector<VirtualRowData> data;
    g_deviceB->GetAllSyncData("sync_data", data);
    EXPECT_EQ(data.size(), static_cast<size_t>(4500));
    std::string checkSql = "select * from sync_data limit 4500 offset 100;";
    CheckSyncData(db, checkSql, data);
}

/**
  * @tc.name: SyncLimitTest002
  * @tc.desc: Sync device with limit query
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, SyncLimitTest002, TestSize.Level3)
{
    PrepareSyncData(db, 5000);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sync_data").Limit(5000, 2000);
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, OK);

    std::vector<VirtualRowData> data;
    g_deviceB->GetAllSyncData("sync_data", data);
    EXPECT_EQ(data.size(), static_cast<size_t>(3000));
    std::string checkSql = "select * from sync_data limit 5000 offset 2000;";
    CheckSyncData(db, checkSql, data);
}

/**
  * @tc.name: SyncLimitTest003
  * @tc.desc: Sync device with limit query
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, SyncLimitTest003, TestSize.Level3)
{
    PrepareSyncData(db, 5000);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sync_data").OrderBy("timestamp").Limit(4500, 1500);
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, OK);

    std::vector<VirtualRowData> data;
    g_deviceB->GetAllSyncData("sync_data", data);
    EXPECT_EQ(data.size(), static_cast<size_t>(3500));
    std::string checkSql = "select * from sync_data order by timestamp limit 4500 offset 1500;";
    CheckSyncData(db, checkSql, data);
}

/**
  * @tc.name: SyncOrderByTest001
  * @tc.desc: Sync device with limit query
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, SyncOrderByTest001, TestSize.Level3)
{
    PrepareSyncData(db, 5000);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sync_data").OrderBy("timestamp");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(errCode, OK);

    std::vector<VirtualRowData> data;
    g_deviceB->GetAllSyncData("sync_data", data);
    EXPECT_EQ(data.size(), static_cast<size_t>(5000));
    std::string checkSql = "select * from sync_data order by timestamp";
    CheckSyncData(db, checkSql, data);
}

HWTEST_F(DistributedDBInterfacesRelationalSyncTest, TableNameCaseInsensitiveTest001, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "student_1");

    DBStatus status = delegate->CreateDistributedTable("StUDent_1");
    EXPECT_EQ(status, OK);

    std::string insertSql = "insert into student_1 (id, name, level, score) values (1001, 'xue', 2, 95);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, insertSql), SQLITE_OK);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sTudENT_1");
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);
    EXPECT_EQ(status, OK);

    std::vector<VirtualRowData> data;
    g_deviceB->GetAllSyncData("student_1", data);
    EXPECT_EQ(data.size(), 1u);
}

namespace {
struct StudentInOrder {
    int id_;
    std::string name_;
    int level_;
    int score_;

    VirtualRowData operator() () const
    {
        VirtualRowData virtualRowData;
        DataValue d1;
        d1 = (int64_t)id_;
        virtualRowData.objectData.PutDataValue("id", d1);
        DataValue d2;
        d2.SetText(name_);
        virtualRowData.objectData.PutDataValue("name", d2);
        DataValue d3;
        d3 = (int64_t)level_;
        virtualRowData.objectData.PutDataValue("level", d3);
        DataValue d4;
        d4 = (int64_t)score_;
        virtualRowData.objectData.PutDataValue("score", d4);
        virtualRowData.logInfo.dataKey = 3; // 3 fake datakey
        virtualRowData.logInfo.device = DEVICE_B;
        virtualRowData.logInfo.originDev = DEVICE_B;
        virtualRowData.logInfo.timestamp = 3170194300890338180; // 3170194300890338180 fake timestamp
        virtualRowData.logInfo.wTimestamp = 3170194300890338180; // 3170194300890338180 fake timestamp
        virtualRowData.logInfo.flag = 2; // 2 fake flag

        std::vector<uint8_t> hashKey;
        DBCommon::CalcValueHash({}, hashKey);
        virtualRowData.logInfo.hashKey = hashKey;
        return virtualRowData;
    }
};
}

HWTEST_F(DistributedDBInterfacesRelationalSyncTest, TableNameCaseInsensitiveTest002, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "student_1");

    DBStatus status = delegate->CreateDistributedTable("StUDent_1");
    EXPECT_EQ(status, OK);

    g_deviceB->PutDeviceData("student_1", std::vector<StudentInOrder> {{1001, "xue", 4, 91}}); // 4, 91 fake data

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sTudENT_1");
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
            EXPECT_EQ(devicesMap.at(DEVICE_B)[0].status, OK);
        }, true);
    EXPECT_EQ(status, OK);

    std::string deviceTableName = g_mgr.GetDistributedTableName(DEVICE_B, "student_1");
    RelationalTestUtils::ExecSql(db, "select count(*) from " + deviceTableName + ";", nullptr, [] (sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), 1);
        return OK;
    });

    status = delegate->RemoveDeviceData(DEVICE_B, "sTudENT_1");
    EXPECT_EQ(status, OK);

    RelationalTestUtils::ExecSql(db, "select count(*) from " + deviceTableName + ";", nullptr, [] (sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), 0);
        return OK;
    });
}

HWTEST_F(DistributedDBInterfacesRelationalSyncTest, TableFieldsOrderTest001, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT_IN_ORDER), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "student_1");
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "DROP TABLE IF EXISTS student_1;"), SQLITE_OK);

    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "student_1");

    DBStatus status = delegate->CreateDistributedTable("StUDent_1");
    EXPECT_EQ(status, OK);

    std::string insertSql = "insert into student_1 (id, name, level, score) values (1001, 'xue', 4, 95);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, insertSql), SQLITE_OK);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sTudENT_1").EqualTo("ID", 1001); // 1001 : id
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
            EXPECT_EQ(devicesMap.at(DEVICE_B)[0].status, OK);
        }, true);
    EXPECT_EQ(status, OK);

    std::vector<VirtualRowData> data;
    g_deviceB->GetAllSyncData("student_1", data);
    EXPECT_EQ(data.size(), 1u);
    DataValue value;
    data[0].objectData.GetDataValue("id", value);
    EXPECT_EQ(value.GetType(), StorageType::STORAGE_TYPE_INTEGER);
    int64_t intVal;
    value.GetInt64(intVal);
    EXPECT_EQ(intVal, (int64_t)1001); // 1001 : id

    data[0].objectData.GetDataValue("name", value);
    EXPECT_EQ(value.GetType(), StorageType::STORAGE_TYPE_TEXT);
    std::string strVal;
    value.GetText(strVal);
    EXPECT_EQ(strVal, "xue");

    data[0].objectData.GetDataValue("level", value);
    EXPECT_EQ(value.GetType(), StorageType::STORAGE_TYPE_INTEGER);
    value.GetInt64(intVal);
    EXPECT_EQ(intVal, (int64_t)4); // 4 level

    data[0].objectData.GetDataValue("score", value);
    EXPECT_EQ(value.GetType(), StorageType::STORAGE_TYPE_INTEGER);
    value.GetInt64(intVal);
    EXPECT_EQ(intVal, (int64_t)95); // 95 score
}

HWTEST_F(DistributedDBInterfacesRelationalSyncTest, TableFieldsOrderTest002, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT_IN_ORDER), SQLITE_OK);
    AddDeviceSchema(g_deviceB, db, "student_1");
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "DROP TABLE IF EXISTS student_1;"), SQLITE_OK);

    g_deviceB->PutDeviceData("student_1", std::vector<StudentInOrder> {{1001, "xue", 4, 91}}); // 4, 91 fake data

    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL_STUDENT), SQLITE_OK);

    DBStatus status = delegate->CreateDistributedTable("StUDent_1");
    EXPECT_EQ(status, OK);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("sTudENT_1").EqualTo("ID", 1001); // 1001 id
    status = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
            EXPECT_EQ(devicesMap.at(DEVICE_B)[0].status, OK);
        }, true);
    EXPECT_EQ(status, OK);

    std::string deviceTableName = g_mgr.GetDistributedTableName(DEVICE_B, "student_1");
    RelationalTestUtils::ExecSql(db, "select * from " + deviceTableName + ";", nullptr, [] (sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), 1001); // 1001 id
        std::string value;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, value), E_OK);
        EXPECT_EQ(value, "xue");
        EXPECT_EQ(sqlite3_column_int64(stmt, 2), 4); // 4 level
        EXPECT_EQ(sqlite3_column_int64(stmt, 3), 91); // 91 score
        return OK;
    });
}

/**
  * @tc.name: SyncZeroBlobTest001
  * @tc.desc: Sync device with zero blob
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, SyncZeroBlobTest001, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, ALL_FIELD_TYPE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(delegate->CreateDistributedTable("tbl_all_type"), OK);
    AddDeviceSchema(g_deviceB, db, "tbl_all_type");

    // prepare with zero blob data
    std::string insertSql = "INSERT INTO tbl_all_type VALUES(?, ?, ?, ?, ?, ?)";
    int ret = RelationalTestUtils::ExecSql(db, insertSql, [] (sqlite3_stmt *stmt) {
        sqlite3_bind_int64(stmt, 1, 1001); // 1, 1001 bind index, bind value
        sqlite3_bind_int64(stmt, 2, 12344); // 2, 12344 bind index, bind value
        sqlite3_bind_double(stmt, 3, 1.234); // 3, 1.234 bind index, bind value
        SQLiteUtils::BindTextToStatement(stmt, 4, ""); // 4, bind index
        SQLiteUtils::BindBlobToStatement(stmt, 5, {}); // 5,bind index
        return E_OK;
    }, nullptr);
    EXPECT_EQ(ret, E_OK);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("tbl_all_type");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
            for (const auto &itDev : devicesMap) {
                for (const auto &itTbl : itDev.second) {
                    EXPECT_EQ(itTbl.status, OK);
                }
            }
        }, true);
    EXPECT_EQ(errCode, OK);

    std::vector<VirtualRowData> data;
    g_deviceB->GetAllSyncData("tbl_all_type", data);
    EXPECT_EQ(data.size(), 1U);
    for (const auto &it : data) {
        DataValue val;
        it.objectData.GetDataValue("id", val);
        EXPECT_EQ(val.GetType(), StorageType::STORAGE_TYPE_INTEGER);
        it.objectData.GetDataValue("f_int", val);
        EXPECT_EQ(val.GetType(), StorageType::STORAGE_TYPE_INTEGER);
        it.objectData.GetDataValue("f_real", val);
        EXPECT_EQ(val.GetType(), StorageType::STORAGE_TYPE_REAL);
        it.objectData.GetDataValue("f_text", val);
        EXPECT_EQ(val.GetType(), StorageType::STORAGE_TYPE_TEXT);
        it.objectData.GetDataValue("f_blob", val);
        EXPECT_EQ(val.GetType(), StorageType::STORAGE_TYPE_BLOB);
        it.objectData.GetDataValue("f_none", val);
        EXPECT_EQ(val.GetType(), StorageType::STORAGE_TYPE_NULL);
    }
}

namespace {
struct TblAllType {
    DataValue id_;
    DataValue fInt_;
    DataValue fReal_;
    DataValue fText_;
    DataValue fBlob_;
    DataValue fNone_;

    TblAllType(int64_t id, int64_t fInt, double fReal, const std::string &fText, const Blob &fBlob)
    {
        id_ = id;
        fInt_ = fInt;
        fReal_ = fReal;
        fText_ = fText;
        fBlob_ = fBlob;
    }

    VirtualRowData operator() () const
    {
        VirtualRowData virtualRowData;
        virtualRowData.objectData.PutDataValue("id", id_);
        virtualRowData.objectData.PutDataValue("f_int", fInt_);
        virtualRowData.objectData.PutDataValue("f_real", fReal_);
        virtualRowData.objectData.PutDataValue("f_text", fText_);
        virtualRowData.objectData.PutDataValue("f_blob", fBlob_);
        virtualRowData.objectData.PutDataValue("f_none", fNone_);

        virtualRowData.logInfo.dataKey = 4; // 4 fake datakey
        virtualRowData.logInfo.device = DEVICE_B;
        virtualRowData.logInfo.originDev = DEVICE_B;
        virtualRowData.logInfo.timestamp = 3170194300891338180; // 3170194300891338180 fake timestamp
        virtualRowData.logInfo.wTimestamp = 3170194300891338180; // 3170194300891338180 fake timestamp
        virtualRowData.logInfo.flag = 2; // 2 fake flag

        std::vector<uint8_t> hashKey;
        DBCommon::CalcValueHash({}, hashKey);
        virtualRowData.logInfo.hashKey = hashKey;
        return virtualRowData;
    }
};
}

/**
  * @tc.name: SyncZeroBlobTest002
  * @tc.desc: Sync device with zero blob
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, SyncZeroBlobTest002, TestSize.Level1)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, ALL_FIELD_TYPE_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(delegate->CreateDistributedTable("tbl_all_type"), OK);
    AddDeviceSchema(g_deviceB, db, "tbl_all_type");

    std::vector<VirtualRowData> dataList;
    g_deviceB->PutDeviceData("tbl_all_type",
        std::vector<TblAllType> {{1001, 12344, 1.234, "", {}}}); // 1001, 12344, 1.234 : fake data

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select("tbl_all_type");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PULL_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
            for (const auto &itDev : devicesMap) {
                for (const auto &itTbl : itDev.second) {
                    EXPECT_EQ(itTbl.status, OK);
                }
            }
        }, true);
    EXPECT_EQ(errCode, OK);

    std::string devictTbl = RelationalStoreManager::GetDistributedTableName(DEVICE_B, "tbl_all_type");
    std::string insertSql = "SELECT * FROM " + devictTbl;
    int resCnt = 0;
    int ret = RelationalTestUtils::ExecSql(db, insertSql, nullptr, [&resCnt](sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_type(stmt, 0), SQLITE_INTEGER);
        EXPECT_EQ(sqlite3_column_int(stmt, 0), 1001); // 1001: fake data

        EXPECT_EQ(sqlite3_column_type(stmt, 1), SQLITE_INTEGER); // 1: column index
        EXPECT_EQ(sqlite3_column_int(stmt, 1), 12344); // 1: column index; 12344: fake data

        EXPECT_EQ(sqlite3_column_type(stmt, 2), SQLITE_FLOAT); // 2: column index
        EXPECT_EQ(sqlite3_column_double(stmt, 2), 1.234); // 2: column index; 1.234: fake data

        EXPECT_EQ(sqlite3_column_type(stmt, 3), SQLITE_TEXT); // 3: column index
        std::string strVal;
        SQLiteUtils::GetColumnTextValue(stmt, 3, strVal); // 3: column index
        EXPECT_EQ(strVal, "");

        EXPECT_EQ(sqlite3_column_type(stmt, 4), SQLITE_BLOB); // 4: column index
        std::vector<uint8_t> blobVal;
        SQLiteUtils::GetColumnBlobValue(stmt, 4, blobVal); // 4: column index
        EXPECT_EQ(blobVal, std::vector<uint8_t> {});

        EXPECT_EQ(sqlite3_column_type(stmt, 5), SQLITE_NULL); // 5: column index
        resCnt++;
        return E_OK;
    });
    EXPECT_EQ(resCnt, 1);
    EXPECT_EQ(ret, E_OK);
}

/**
  * @tc.name: RuntimeConfig001
  * @tc.desc: Runtime config api
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesRelationalSyncTest, RuntimeConfig001, TestSize.Level1)
{
    DBStatus status = RuntimeConfig::SetProcessLabel("", "");
    EXPECT_EQ(status, INVALID_ARGS);
    status = RuntimeConfig::SetProcessLabel("DistributedDBInterfacesRelationalSyncTest", "RuntimeConfig001");
    EXPECT_EQ(status, OK);
    status = RuntimeConfig::SetProcessCommunicator(nullptr);
    if (!RuntimeContext::GetInstance()->IsCommunicatorAggregatorValid()) {
        EXPECT_EQ(status, OK);
    }
    EXPECT_EQ(RuntimeConfig::IsProcessSystemApiAdapterValid(),
        RuntimeContext::GetInstance()->IsProcessSystemApiAdapterValid());
}
