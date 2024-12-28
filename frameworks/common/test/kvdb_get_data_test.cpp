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

#include "data_transformer.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "db_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "kvdb_properties.h"
#include "log_print.h"
#include "relational_schema_object.h"
#include "kv_store_delegate.h"
#include "kv_store_instance.h"
#include "kv_store_manager.h"
#include "kv_store_sqlite_ext.h"
#include "relational_sync_able_storage.h"
#include "runtime_config.h"
#include "sqlite_kv_store.h"
#include "sqlite_utils.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace KvDB;
using namespace KvDBUnitTest;
using namespace std;

namespace {
string g_testDirVirtual;
string g_storeTestPathVirtual;
string g_storeTestVir = "dftStoreID";
string g_tableNameVirtual { "data" };
KvDB::KvStoreManager g_mgrTest(APP_ID, USER_ID);
KvStoreDelegate *g_delegateTest = nullptr;
IKvStore *g_storeTest = nullptr;

void CreateDBTable()
{
    sqlite3 *database = nullptr;
    int errCodeTest = sqlite3_open(g_storeTestPathVirtual.c_str(), &database);
    if (errCodeTest != SQLITE_OK) {
        LOGE("open database failed:%d", errCodeTest);
        sqlite3_close(database);
        return;
    }

    const string sql =
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableNameVirtual + "(key INTEGER PRIMARY KEY AUTOINCREMENT, value INTEGER);";
    char *zErrMsgTest = nullptr;
    errCodeTest = sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &zErrMsgTest);
    if (errCodeTest != SQLITE_OK) {
        LOGE("sql error:%s", zErrMsgTest);
        sqlite3_free(zErrMsgTest);
    }
    sqlite3_close(database);
}

int AddOrUpdate(int64_t key, int64_t value)
{
    sqlite3 *database = nullptr;
    int errCodeTest = sqlite3_open(g_storeTestPathVirtual.c_str(), &database);
    if (errCodeTest == SQLITE_OK) {
        errCodeTest = sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr);
    }
    errCodeTest = SQLiteUtils::MapSQLiteErr(errCodeTest);
    sqlite3_close(database);
    return errCodeTest;
}

int GetLogDataTest(int key, uint64_t &flag, Timestamp &timestamp, const DeviceID &device = "")
{
    if (!device.empty()) {
    }

    sqlite3 *database = nullptr;
    sqlite3_stmt *stmt = nullptr;
    int errCodeTest = sqlite3_open(g_storeTestPathVirtual.c_str(), &database);
    if (errCodeTest != SQLITE_OK) {
        LOGE("open database failed:%d", errCodeTest);
        errCodeTest = SQLiteUtils::MapSQLiteErr(errCodeTest);
        goto END;
    }
    errCodeTest = SQLiteUtils::GetStatement(database, sql, stmt);
    if (errCodeTest != E_OK) {
        goto END;
    }
    errCodeTest = SQLiteUtils::BindInt64ToStatement(stmt, 1, key); // 1 means key's index
    if (errCodeTest != E_OK) {
        goto END;
    }
    errCodeTest = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCodeTest == SQLiteUtils::MapSQLiteErr(SQLITE_ROW)) {
        timestamp = static_cast<Timestamp>(sqlite3_column_int64(stmt, 0));
        flag = static_cast<Timestamp>(sqlite3_column_int64(stmt, 1));
        errCodeTest = E_OK;
    } else if (errCodeTest == SQLiteUtils::MapSQLiteErr(SQLITE_DONE)) {
        errCodeTest = -E_NOT_FOUND;
    }

END:
    SQLiteUtils::ResetStatement(stmt, true, errCodeTest);
    sqlite3_close(database);
    return errCodeTest;
}

void InitStorePropVirtual(const std::string &storePath, const std::string &appId, const std::string &userId,
    KvDBProperties &proper)
{
    proper.SetStringProp(KvDBProperties::DATA_DIR, storePath);
    proper.SetStringProp(KvDBProperties::APP_ID, appId);
    proper.SetStringProp(KvDBProperties::USER_ID, userId);
    proper.SetStringProp(KvDBProperties::STORE_ID, g_storeTestVir);
    std::string ident = userId + "-" + appId + "-" + g_storeTestVir;
    std::string hashIdenti = DBCommon::TransferHashString(ident);
    proper.SetStringProp(KvDBProperties::IDENTIFIER_DATA, hashIdenti);
}

const KvSyncAbleStorage *GetKvStore()
{
    KvDBProperties proper;
    InitStorePropVirtual(g_storeTestPathVirtual, APP_ID, USER_ID, proper);
    int errCodeTest = E_OK;
    g_storeTest = KvStoreInstance::GetDataBase(proper, errCodeTest);
    if (g_storeTest == nullptr) {
        LOGE("Get database failed:%d", errCodeTest);
        return nullptr;
    }
    return static_cast<SQLiteKvStore *>(g_storeTest)->GetStorageEngine();
}

void ExpectCount(sqlite3 *database, const string &sql, size_t expectCount)
{
    size_t countTest = 0;
    ASSERT_EQ(GetCount(database, sql, countTest), E_OK);
    EXPECT_EQ(countTest, expectCount);
}

int GetCount(sqlite3 *database, const string &sql, size_t &countTest)
{
    sqlite3_stmt *stmTest = nullptr;
    int errCodeTest = SQLiteUtils::GetStatement(database, sql, stmTest);
    if (errCodeTest != E_OK) {
        return errCodeTest;
    }
    errCodeTest = SQLiteUtils::StepWithRetry(stmTest, false);
    if (errCodeTest == SQLiteUtils::MapSQLiteErr(SQLITE_ROW)) {
        countTest = static_cast<size_t>(sqlite3_column_int64(stmTest, 0));
        errCodeTest = E_OK;
    }
    SQLiteUtils::ResetStatement(stmTest, true, errCodeTest);
    return errCodeTest;
}

std::string GetText(sqlite3 *database, const string &sql)
{
    std::string res;
    sqlite3_stmt *stmTest = nullptr;
    int errCodeTest = SQLiteUtils::GetStatement(database, sql, stmTest);
    if (errCodeTest != E_OK) {
        return res;
    }
    errCodeTest = SQLiteUtils::StepWithRetry(stmTest, false);
    if (errCodeTest == SQLiteUtils::MapSQLiteErr(SQLITE_ROW)) {
        SQLiteUtils::GetColumnTextValue(stmTest, 0, res);
    }
    SQLiteUtils::ResetStatement(stmTest, true, errCodeTest);
    return res;
}

int PutBatch(uint32_t totalCount, uint32_t valueSize)
{
    sqlite3 *database = nullptr;
    sqlite3_stmt *stmTest = nullptr;
    const string sql = "INSERT INTO " + g_tableNameVirtual + " VALUES(?,?);";
    int errCodeTest = sqlite3_open(g_storeTestPathVirtual.c_str(), &database);
    if (errCodeTest != SQLITE_OK) {
        goto ERROR;
    }
    for (uint32_t i = 0; i < totalCount; i++) {
        if (errCodeTest != E_OK) {
            break;
        }
        errCodeTest = SQLiteUtils::StepWithRetry(stmTest);
        if (errCodeTest != SQLiteUtils::MapSQLiteErr(SQLITE_DONE)) {
            break;
        }
        errCodeTest = E_OK;
        SQLiteUtils::ResetStatement(stmTest, false, errCodeTest);
    }
ERROR:
    if (errCodeTest == E_OK) {
        EXPECT_EQ(sqlite3_exec(database, "COMMIT TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    } else {
        EXPECT_EQ(sqlite3_exec(database, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    }
    SQLiteUtils::ResetStatement(stmTest, true, errCodeTest);
    errCodeTest = SQLiteUtils::MapSQLiteErr(errCodeTest);
    sqlite3_close(database);
    return errCodeTest;
}

void ExecAndAssertOK(sqlite3 *database, const initializer_list<std::string> &sqlList)
{
    for (const auto &sql : sqlList) {
        ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }
}

void ExecAndAssertOK(sqlite3 *database, const std::string &sql)
{
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
}

void ExpectMissQueryCnt(const std::vector<SingleVerKvEntry *> &entriesItem, size_t expectCount)
{
    size_t countTest = 0;
    for (auto i = entriesItem.begin(); i != entriesItem.end(); ++i) {
        if (((*i)->GetFlag() & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) == 0) {
            countTest++;
        }
        auto next = std::next(i, 1);
        if (next != entriesItem.end()) {
            EXPECT_LT((*i)->GetTimestamp(), (*next)->GetTimestamp());
        }
    }
    EXPECT_EQ(countTest, expectCount);
}

void SetRemoteSchema(const KvSyncAbleStorage *storeTest, const std::string &deviceID)
{
    std::string remSchema = storeTest->GetSchema().ToSchemaString();
    uint8_t reSchemaType = static_cast<uint8_t>(storeTest->GetSchema().GetSchemaType());
    const_cast<KvSyncAbleStorage *>(storeTest)->SaveRemoteDeviceSchema(deviceID, remSchema, reSchemaType);
}

void SetNextBeginTimeTest001()
{
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::unique_ptr<SQLiteSingleVerKvContinueToken> tokenTest =
        std::make_unique<SQLiteSingleVerKvContinueToken>(SyncTimeRange {}, queryTest);
    EXPECT_TRUE(tokenTest != nullptr);

    DataItem dataItemTest;
    dataItemTest.timestamp = INT64_MAX;
    tokenTest->SetNextBeginTime(dataItemTest);

    dataItemTest.flag = DataItem::DELETE_FLAG;
    tokenTest->FinishGetData();
    EXPECT_EQ(tokenTest->IsGetAllDataFinished(), false);
    tokenTest->SetNextBeginTime(dataItemTest);
}

class KvDBKvGetDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBKvGetDataTest::SetUpTestCase(void)
{
    KvDBToolsUnitTest::TestDirInit(g_testDirVirtual);
    g_storeTestPathVirtual = g_testDirVirtual + "/getDataTest.database";
    LOGI("The test database is:%s", g_testDirVirtual.c_str());

    auto communicate = new (std::nothrow) VirtualCommunicatorAggregator();
    EXPECT_TRUE(communicate != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicate);
}

void KvDBKvGetDataTest::TearDownTestCase(void)
{}

void KvDBKvGetDataTest::SetUp(void)
{
    g_tableNameVirtual = "data";
    KvDBToolsUnitTest::PrintTestCaseInfo();
    CreateDBTable();
}

void KvDBKvGetDataTest::TearDown(void)
{
    if (g_delegateTest != nullptr) {
        EXPECT_EQ(g_mgrTest.CloseStore(g_delegateTest), DBStatus::OK);
        g_delegateTest = nullptr;
    }
    if (KvDBToolsUnitTest::RemoveTestDbFiles(g_testDirVirtual) != 0) {
        LOGE("database files error.");
    }
    return;
}

/**
 * @tc.name: LogTbl1
 * @tc.desc: When put sync data to relational storeTest, trigger generate log.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 */
HWTEST_F(KvDBKvGetDataTest, LogTbl1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    /**
     * @tc.steps: step1. Put data.
     * @tc.expected: Succeed, return OK.
     */
    int insertKey = 1;
    int insertValue = 1;
    EXPECT_EQ(AddOrUpdate(insertKey, insertValue), E_OK);

    /**
     * @tc.steps: step2. Check log record.
     * @tc.expected: Record exists.
     */
    uint64_t flag = 0;
    Timestamp timestamp1 = 0;
    EXPECT_EQ(GetLogDataTest(insertKey, flag, timestamp1), E_OK);
    EXPECT_EQ(flag, DataItem::LOCAL_FLAG);
    EXPECT_NE(timestamp1, 0ULL);
}

/**
 * @tc.name: GetSyncData1
 * @tc.desc: GetSyncData interface
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
  */
HWTEST_F(KvDBKvGetDataTest, GetSyncData1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 500 records.
     * @tc.expected: Succeed, return OK.
     */
    const size_t COUNT = 500;
    for (size_t i = 0; i < COUNT; ++i) {
        EXPECT_EQ(AddOrUpdate(i, i), E_OK);
    }

    /**
     * @tc.steps: step2. Get all data.
     * @tc.expected: Succeed and the countTest is right.
     */
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;
    DataSizeSpec sizeInfo {MTU_SIZE, 50};

    int errCodeTest = storeTest->GetSyncData(queryTest, SyncTimeRange {}, sizeInfo, tokenTest, entriesItem);
    auto countTest = entriesItem.size();
    SingleVerKvEntry::Release(entriesItem);
    EXPECT_EQ(errCodeTest, -E_UNFINISHED);
    while (tokenTest != nullptr) {
        errCodeTest = storeTest->GetSyncDataNext(entriesItem, tokenTest, sizeInfo);
        countTest += entriesItem.size();
        SingleVerKvEntry::Release(entriesItem);
        EXPECT_TRUE(errCodeTest == E_OK || errCodeTest == -E_UNFINISHED);
    }
    EXPECT_EQ(countTest, COUNT);

    KvDBProperties proper;
    InitStorePropVirtual(g_storeTestPathVirtual, APP_ID, USER_ID, proper);
    auto database = KvStoreInstance::GetDataBase(proper, errCodeTest, false);
    EXPECT_EQ(database, nullptr);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetSyncData2
 * @tc.desc: GetSyncData interface. For overlarge data(over 4M), ignore it.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
  */
HWTEST_F(KvDBKvGetDataTest, GetSyncData2Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 10 records.(1M + 2M + 3M + 4M + 5M) * 2.
     * @tc.expected: Succeed, return OK.
     */
    for (int i = 1; i <= 5; ++i) {
        EXPECT_EQ(PutBatch(1, i * 1024 * 1024), E_OK);  // 1024*1024 equals 1M.
    }
    for (int i = 1; i <= 5; ++i) {
        EXPECT_EQ(PutBatch(1, i * 1024 * 1024), E_OK);  // 1024*1024 equals 1M.
    }

    /**
     * @tc.steps: step2. Get all data.
     * @tc.expected: Succeed and the countTest is 6.
     */
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;

    const size_t EXPECTCOUNT = 6;  // expect 6 records.
    DataSizeSpec sizeInfo;
    sizeInfo.blockSize = 100 * 1024 * 1024;  // permit 100M.
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, sizeInfo, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), EXPECTCOUNT);
    SingleVerKvEntry::Release(entriesItem);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetSyncData3
 * @tc.desc: GetSyncData interface. For deleted data.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
  */
HWTEST_F(KvDBKvGetDataTest, GetSyncData3Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    const string tableName = g_tableNameVirtual + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT, value INTEGER);";
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);

    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), COUNT);

    QueryObject gQuery(Query::Select(g_tableNameVirtual));
    DeviceID device1 = "device1";
    DeviceID device2 = "device2";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableNameVirtual),
        DBCommon::GetDistributedTableName(device1, g_tableNameVirtual)));
    SetRemoteSchema(storeTest, device1);
    SetRemoteSchema(storeTest, device2);
    auto rEntries = std::vector<SingleVerKvEntry *>(entriesItem.rbegin(), entriesItem.rend());
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableNameVirtual),
        DBCommon::GetDistributedTableName(device2, g_tableNameVirtual)));
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(gQuery, rEntries, device2), E_OK);
    rEntries.clear();
    SingleVerKvEntry::Release(entriesItem);

    ExecAndAssertOK(database, "DELETE FROM " + tableName + " WHERE rowid<=2;");
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), COUNT);
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(gQuery, entriesItem, device1), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetQuerySyncData1
 * @tc.desc: GetSyncData interface.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(KvDBKvGetDataTest, GetQuerySyncData1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 100 records.
     * @tc.expected: Succeed, return OK.
     */
    const size_t COUNT = 100; // 100 records.
    for (size_t i = 0; i < COUNT; ++i) {
        EXPECT_EQ(AddOrUpdate(i, i), E_OK);
    }

    /**
     * @tc.steps: step2. Get data limit 80, offset 30.
     * @tc.expected: Get 70 records.
     */
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual).Limit(LIM, OFFSET));
    std::vector<SingleVerKvEntry *> entriesItem;

    int errCodeTest = storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem);
    EXPECT_EQ(entriesItem.size(), EXPECTCOUNT);
    EXPECT_EQ(errCodeTest, E_OK);
    EXPECT_EQ(tokenTest, nullptr);
    SingleVerKvEntry::Release(entriesItem);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetQuerySyncData2
 * @tc.desc: GetSyncData interface.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, GetQuerySyncData2Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    const size_t COUNT = 100; // 100 records.
    for (size_t i = 0; i < COUNT; ++i) {
        EXPECT_EQ(AddOrUpdate(i, i), E_OK);
    }
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;

    Query queryTest = Query::Select(g_tableNameVirtual).NotEqualTo("key", 10).And().OrderBy("key", false);
    QueryObject queryObj(queryTest);
    queryObj.SetSchema(storeTest->GetSchema());

    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(tokenTest, nullptr);
    size_t expectCount = 98;  // expect 98 records.
    EXPECT_EQ(entriesItem.size(), expectCount);
    for (auto i = entriesItem.begin(); i != entriesItem.end(); ++i) {
        auto next = std::next(i, 1);
        if (next != entriesItem.end()) {
            EXPECT_LT((*i)->GetTimestamp(), (*next)->GetTimestamp());
        }
    }
    SingleVerKvEntry::Release(entriesItem);
    queryTest = Query::Select(g_tableNameVirtual).EqualTo("key", 10).Or().EqualTo("value", 20).OrderBy("key", true);
    queryObj = QueryObject(queryTest);
    queryObj.SetSchema(storeTest->GetSchema());

    EXPECT_EQ(storeTest->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(tokenTest, nullptr);
    expectCount = 2;  // expect 2 records.
    EXPECT_EQ(entriesItem.size(), expectCount);
    for (auto i = entriesItem.begin(); i != entriesItem.end(); ++i) {
        auto next = std::next(i, 1);
        if (next != entriesItem.end()) {
            EXPECT_LT((*i)->GetTimestamp(), (*next)->GetTimestamp());
        }
    }
    SingleVerKvEntry::Release(entriesItem);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetIncorrectTypeData1
 * @tc.desc: GetSyncData and PutSyncDataWithQuery interface.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, GetIncorrectTypeData1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    /**
     * @tc.steps: step1. Create 2 index for table "data".
     * @tc.expected: Succeed, return OK.
     */
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);

    /**
     * @tc.steps: step2. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableNameVirtual + "Plus";
    string sql = "CREATE TABLE " + tableName + "(LIMkey INTEGER PRIMARY KEY AUTOINCREMENT , value INTEGER);";
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);

    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), COUNT);

    /**
     * @tc.steps: step5. Put data into "data" table from device1.
     * @tc.expected: Succeed, return OK.
     */
    QueryObject queryPlus(Query::Select(g_tableNameVirtual));
    const DeviceID deviceID = "device1";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableNameVirtual)));
    ASSERT_EQ(E_OK, SQLiteUtils::CloneIndexes(database, g_tableNameVirtual,
        DBCommon::GetDistributedTableName(deviceID, g_tableNameVirtual)));

    SetRemoteSchema(storeTest, deviceID);
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryPlus, deviceID), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: UpdateData1
 * @tc.desc: UpdateData succeed when the table has primary key.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, UpdateData1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    const string tableName = g_tableNameVirtual + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(LIMkey INTEGER PRIMARY KEY AUTOINCREMENT , value INTEGER);";
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);

    const size_t COUNT = sqls.size();
    for (const auto &item : sqls) {
        ASSERT_EQ(sqlite3_exec(database, item.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }

    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), COUNT);

    queryTest = QueryObject(Query::Select(g_tableNameVirtual));
    const DeviceID deviceID = "device1";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableNameVir),
        DBCommon::GetDistributedTableName(deviceID, g_tableNameVirtual)));

    SetRemoteSchema(storeTest, deviceID);
    for (uint32_t i = 0; i < 10; ++i) {  // 10 for test.
        EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutDataWithQuery(queryTest, entries, deviceID), E_OK);
    }
    SingleVerKvEntry::Release(entriesItem);

    size_t countTest = 0;
    EXPECT_EQ(GetCount(database, sql, countTest), E_OK);
    EXPECT_EQ(countTest, COUNT);
    sql = "SELECT countTest(*) FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + g_tableNameVirtual + "_log;";
    countTest = 0;
    EXPECT_EQ(GetCount(database, sql, countTest), E_OK);
    EXPECT_EQ(countTest, COUNT);
    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: UpdateDataWithMulDevData1
 * @tc.desc: UpdateData succeed when there is multiple devices data exists.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, UpdateDataWithMulDevData1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    const string tableName = g_tableNameVirtual + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(LIMkey INTEGER PRIMARY KEY AUTOINCREMENT , value INTEGER);";
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);

    sql = "INSERT INTO " + tableName + " VALUES(1, 1);"; // k1v1
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);

    queryTest = QueryObject(Query::Select(g_tableNameVirtual));
    const DeviceID deviceID = "device1";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableNameVir),
        DBCommon::GetDistributedTableName(deviceID, g_tableNameVirtual)));

    SetRemoteSchema(storeTest, deviceID);
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryTest, deviceID), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    EXPECT_EQ(AddOrUpdate(1, 1), E_OK); // k1v1

    sql = "UPDATE " + g_tableNameVirtual + " SET value=2 WHERE key=1;"; // k1v1
    EXPECT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK); // change k1v1 to k1v2

    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: MissQuery1
 * @tc.desc: Check REMOTE_DEVICE_DATA_MISS_QUERY flag succeed.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, MissQuery1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);
    const string tableName = g_tableNameVirtual + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(LIMkey INTEGER PRIMARY KEY AUTOINCREMENT , value INTEGER);";
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    SyncTimeRange timeRange;

    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, timeRange, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    timeRange.lastQueryTime = (*(entriesItem.rbegin()))->GetTimestamp();
    EXPECT_EQ(entriesItem.size(), 3U);  // 3 for test

    queryTest = QueryObject(Query::Select(g_tableNameVirtual));
    const DeviceID deviceID = "device1";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableNameVir),
        DBCommon::GetDistributedTableName(deviceID, g_tableNameVirtual)));
    SetRemoteSchema(storeTest, deviceID);
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryTest, deviceID), E_OK);
    SingleVerKvEntry::Release(entriesItem);
    ExpectCount(database, getLogSql, 3);  // 2,3,4
    EXPECT_EQ(storeTest->GetSyncData(queryTest, timeRange, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), 3U);  // 3 miss queryTest data.

    SetRemoteSchema(storeTest, deviceID);
    queryTest = QueryObject(Query::Select(g_tableNameVirtual));
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryTest, deviceID), E_OK);
    SingleVerKvEntry::Release(entriesItem);
    ExpectCount(database, getDataSql, 0U);  // 0 data exists
    ExpectCount(database, getLogSql, 0U);  // 0 data exists
    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: CompatibleData1
 * @tc.desc: Check compatibility.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(KvDBKvGetDataTest, CompatibleData1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    const string tableName = g_tableNameVirtual + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER, value INTEGER NOT NULL, \
        extra_field TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);

    ASSERT_EQ(AddOrUpdate(1, 101), E_OK);
    sql = "INSERT INTO " + tableName + " VALUES(2, 102, 'f3');"; // k2v102
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest), E_OK);
    EXPECT_EQ(entriesItem.size(), 1UL);

    QueryObject queryPlus(Query::Select(tableName));
    const DeviceID deviceID = "device1";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(tableName),
        DBCommon::GetDistributedTableName(deviceID, tableName)));

    SetRemoteSchema(storeTest, deviceID);
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryPlus, deviceID), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    EXPECT_EQ(storeTest->GetSyncData(queryPlus, SyncTimeRange {}, DataSizeSpec {}, tokenTest), E_OK);
    EXPECT_EQ(entriesItem.size(), 1UL);

    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableNameVir),
        DBCommon::GetDistributedTableName(deviceID, g_tableNameVirtual)));
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryTest, entriesItem), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    countTest = 0;
    EXPECT_EQ(GetCount(database, sql, countTest), E_OK);
    EXPECT_EQ(countTest, 1UL);
    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetDataSortByTime1
 * @tc.desc: All queryTest get data sort by time asc.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, GetDataSortByTime1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    std::string sql = "INSERT INTO " + g_tableNameVirtual + " VALUES(1, 101);"; // k1v101
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "INSERT INTO " + g_tableNameVirtual + " VALUES(2, 102);"; // k2v102
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "INSERT INTO " + g_tableNameVirtual + " VALUES(3, 103);"; // k3v103
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "UPDATE " + g_tableNameVirtual + " SET value=104 WHERE key=2;"; // k2v104
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "UPDATE " + g_tableNameVirtual + " SET value=105 WHERE key=1;"; // k1v105
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    ExpectMissQueryCnt(entriesItem, 3UL);  // 3 data
    SingleVerKvEntry::Release(entriesItem);
    queryTest = QueryObject(Query::Select(g_tableNameVirtual).EqualTo("key", 1).Or().EqualTo("key", 3));
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    ExpectMissQueryCnt(entriesItem, 2UL);  // 2 data
    SingleVerKvEntry::Release(entriesItem);
    queryTest = QueryObject(Query::Select(g_tableNameVirtual).OrderBy("key", false));
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    ExpectMissQueryCnt(entriesItem, 3UL);  // 3 data
    SingleVerKvEntry::Release(entriesItem);
    queryTest = QueryObject(Query::Select(g_tableNameVirtual).OrderBy("value", false));
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    ExpectMissQueryCnt(entriesItem, 3UL);  // 3 data
    SingleVerKvEntry::Release(entriesItem);
    queryTest = QueryObject(Query::Select(g_tableNameVirtual).Limit(2));
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    ExpectMissQueryCnt(entriesItem, 2UL);  // 2 data
    SingleVerKvEntry::Release(entriesItem);
    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: SameFieldWithLogTable1
 * @tc.desc: Get queryTest data OK when the table has same field with log table.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, SameFieldWithLogTable1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);

    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);
    sql = "INSERT INTO " + tableName + " VALUES(1, 101, 'f3');"; // k1v101
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(tableName).EqualTo("flag", 101).OrderBy("device", false));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), 1UL);
    SingleVerKvEntry::Release(entriesItem);
    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: CompatibleData2
 * @tc.desc: Check compatibility.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, CompatibleData2Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);

    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    const DeviceID deviceID = "device1";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(g_tableNameVir),
        DBCommon::GetDistributedTableName(deviceID, g_tableNameVirtual)));

    /**
     * @tc.steps: step2. Alter "data" table and create distributed table again.
     * @tc.expected: Succeed.
     */
    std::string sql = "ALTER TABLE " + g_tableNameVirtual + " ADD COLUMN integer_type INTEGER DEFAULT 123 not null;"
        "ALTER TABLE " + g_tableNameVirtual + " ADD COLUMN text_type TEXT DEFAULT 'high_version' not null;"
        "ALTER TABLE " + g_tableNameVirtual + " ADD COLUMN real_type REAL DEFAULT 123.123456 not null;"
        "ALTER TABLE " + g_tableNameVirtual + " ADD COLUMN blob_type BLOB DEFAULT 123 not null;";
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    EXPECT_EQ(GetText(database, sql), expectSql);

    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: PutSyncDataConflictDataTest001
 * @tc.desc: Check put with conflict sync data.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lianhuix
 */
HWTEST_F(KvDBKvGetDataTest, PutSyncDataConflictDataTest001Virtual, TestSize.Level1)
{
    const DeviceID deviceID_A = "device1";
    const DeviceID deviceID_B = "device2";
    sqlite3 *database = KvTestUtils::CreateDataBase(g_storeTestPathVirtual);
    KvTestUtils::CreateDeviceTable(database, g_tableNameVirtual, deviceID_B);

    DBStatus status = g_mgrTest.OpenStore(g_storeTestPathVirtual, queryTest g_delegateTest);
    EXPECT_EQ(status, DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    EXPECT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    auto storeTest = const_cast<KvSyncAbleStorage *>(GetKvStore());
    ASSERT_NE(storeTest, nullptr);

    DataSizeSpec sizeInfo {MTU_SIZE, 50};
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;
    int errCodeTest = storeTest->GetSyncData(queryTest, {}, sizeInfo, tokenTest, entriesItem);
    EXPECT_EQ(errCodeTest, E_OK);

    SetRemoteSchema(storeTest, deviceID_B);
    errCodeTest = storeTest->PutSyncDataWithQuery(queryTest, entriesItem, deviceID_B);
    EXPECT_EQ(errCodeTest, E_OK);
    GenericSingleVerKvEntry::Release(entriesItem);

    QueryObject query2(Query::Select(g_tableNameVirtual).EqualTo("key", 1001));
    std::vector<SingleVerKvEntry *> entries2;
    storeTest->GetSyncData(query2, {}, sizeInfo, tokenTest, entries2);

    errCodeTest = storeTest->PutSyncDataWithQuery(queryTest, entries2, deviceID_B);
    EXPECT_EQ(errCodeTest, E_OK);
    GenericSingleVerKvEntry::Release(entries2);

    RefObject::DecObjRef(g_storeTest);

    std::string deviceTable = DBCommon::GetDistributedTableName(deviceID_B, g_tableNameVirtual);
    EXPECT_EQ(KvTestUtils::CheckTableRecords(database, deviceTable), 3);
    sqlite3_close_v2(database);
}

/**
 * @tc.name: SaveNonexistDevdata1
 * @tc.desc: Save non-exist device data and check errCodeTest.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(KvDBKvGetDataTest, SaveNonexistDevdata1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);
    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableNameVirtual + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER, value INTEGER NOT NULL, \
        extra_field TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);
    /**
     * @tc.steps: step2. Put 1 record into data table.
     * @tc.expected: Succeed, return OK.
     */
    ASSERT_EQ(AddOrUpdate(1, 101), E_OK);

    /**
     * @tc.steps: step3. Get all data from "data" table.
     * @tc.expected: Succeed and the countTest is right.
     */
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, SyncTimeRange {}, DataSizeSpec {}, tokenTest), E_OK);
    EXPECT_EQ(entriesItem.size(), 1UL);

    /**
     * @tc.steps: step4. Put data into "data_plus" table from device1 and device1 does not exist.
     * @tc.expected: Succeed, return OK.
     */
    queryTest = QueryObject(Query::Select(tableName));
    const DeviceID deviceID = "device1";
    SetRemoteSchema(storeTest, deviceID);
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryTest, entriesItem), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetMaxTimestamp1
 * @tc.desc: Get max timestamp.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 */
HWTEST_F(KvDBKvGetDataTest, GetMaxTimestamp1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    const string tableName = g_tableNameVirtual + "Plus";
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);

    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);

    Timestamp time1 = 0;
    storeTest->GetMaxTimestamp(time1);
    EXPECT_EQ(time1, 0uLL);

    ASSERT_EQ(AddOrUpdate(1, 101), E_OK);
    Timestamp time2 = 0;
    storeTest->GetMaxTimestamp(time2);
    EXPECT_GT(time2, time1);

    ASSERT_EQ(AddOrUpdate(2, 102), E_OK);
    Timestamp time3 = 0;
    storeTest->GetMaxTimestamp(time3);
    EXPECT_GT(time3, time2);

    Timestamp time4 = 0;
    storeTest->GetMaxTimestamp(g_tableNameVirtual, time4);
    EXPECT_EQ(time4, time3);

    Timestamp time5 = 0;
    storeTest->GetMaxTimestamp(tableName, time5);
    EXPECT_EQ(time5, 0uLL);

    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: NoPkData1
 * @tc.desc: For no pk data.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(KvDBKvGetDataTest, NoPkData1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);

    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableNameVirtual + "Plus";
    ExecAndAssertOK(database, "CREATE TABLE " + tableName + "(key INTEGER NOT NULL, value INTEGER);");
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(tableName), DBStatus::OK);

    /**
     * @tc.steps: step2. Put 2 data into "data" table.
     * @tc.expected: Succeed.
     */
    ASSERT_EQ(AddOrUpdate(1, 1), E_OK);
    ASSERT_EQ(AddOrUpdate(2, 2), E_OK);

    /**
     * @tc.steps: step3. Get data from "data" table.
     * @tc.expected: Succeed.
     */
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);

    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;
    EXPECT_EQ(storeTest->GetSyncData(queryTest, {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), 2U);  // expect 2 data.

    /**
     * @tc.steps: step4. Put data into "data" table from device1.
     * @tc.expected: Succeed, return OK.
     */
    QueryObject queryPlus(Query::Select(tableName));
    const DeviceID deviceID = "device1";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(database, storeTest->GetSchema().GetTable(tableName),
        DBCommon::GetDistributedTableName(deviceID, tableName)));
    SetRemoteSchema(storeTest, deviceID);
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryPlus, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    EXPECT_EQ(storeTest->GetSyncData(queryTest, {}, DataSizeSpec {}, tokenTest, entriesItem), E_OK);
    EXPECT_EQ(entriesItem.size(), 2U);  // expect 2 data.

    /**
     * @tc.steps: step7. Put data into "data" table from device1.
     * @tc.expected: Succeed, return OK.
     */
    EXPECT_EQ(const_cast<KvSyncAbleStorage *>(storeTest)->PutSyncDataWithQuery(queryPlus, deviceID), E_OK);
    SingleVerKvEntry::Release(entriesItem);

    /**
     * @tc.steps: step8. Check data.
     * @tc.expected: There is 2 data.
     */
    size_t countTest = 0;
    EXPECT_EQ(GetCount(database, sql, countTest), E_OK);
    EXPECT_EQ(countTest, 2U); // expect 2 data.

    sqlite3_close(database);
    RefObject::DecObjRef(g_storeTest);
}

/**
 * @tc.name: GetAfterDropTable1
 * @tc.desc: Get data after drop table.
 * @tc.type: FUNC
 * @tc.require: AR000H2QPN
 */
HWTEST_F(KvDBKvGetDataTest, GetAfterDropTable1Virtual, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create distributed table.
     * @tc.expected: Succeed, return OK.
     */
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);
    /**
     * @tc.steps: step2. Insert several data.
     * @tc.expected: Succeed, return OK.
     */
    ASSERT_EQ(AddOrUpdate(1, 1), E_OK);
    ASSERT_EQ(AddOrUpdate(2, 2), E_OK);
    ASSERT_EQ(AddOrUpdate(3, 3), E_OK);

    /**
     * @tc.steps: step3. Check data in distributed log table.
     * @tc.expected: The data in log table is in expect. All the flag in log table is 1.
     */
    sqlite3 *database = nullptr;
    ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);

    ExpectCount(database, getLogSql, 0u);  // 0 means no deleted data.

    /**
     * @tc.steps: step4. Drop the table in another connection.
     * @tc.expected: Succeed.
     */
    std::thread t1([] {
        sqlite3 *database = nullptr;
        ASSERT_EQ(sqlite3_open(g_storeTestPathVirtual.c_str(), &database), SQLITE_OK);
        ExecAndAssertOK(database, "DROP TABLE " + g_tableNameVirtual);
        sqlite3_close(database);
    });
    t1.join();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step5. Check data in distributed log table.
     * @tc.expected: The data in log table is in expect. All the flag in log table is 3.
     */
    ExpectCount(database, getLogSql, 3u);  // 3 means all deleted data.
    sqlite3_close(database);
}

/**
  * @tc.name: SetSchema1
  * @tc.desc: Test invalid parameters of query_object.cpp
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(KvDBKvGetDataTest, SetSchema1Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);
    auto storeTest = GetKvStore();
    ASSERT_NE(storeTest, nullptr);
    Query queryTest = Query::Select().OrderBy("errDevice", false);
    QueryObject queryObj1(queryTest);
    int errorNo = E_OK;
    errorNo = queryObj1.SetSchema(storeTest->GetSchema());
    EXPECT_EQ(errorNo, -E_INVALID_ARGS);
    EXPECT_FALSE(queryObj1.IsQueryForKvDB());
    errorNo = queryObj1.Init();
    EXPECT_EQ(errorNo, -E_NOT_SUPPORT);
    QueryObject queryObj2(queryTest);
    queryObj2.SetTableName(g_tableNameVirtual);
    errorNo = queryObj2.SetSchema(storeTest->GetSchema());
    EXPECT_EQ(errorNo, E_OK);
    errorNo = queryObj2.Init();
    EXPECT_EQ(errorNo, -E_INVALID_QUERY_FIELD);
    RefObject::DecObjRef(g_storeTest);
}

/**
  * @tc.name: SetNextBeginTimeTest001
  * @tc.desc: Test invalid parameters of query_object.cpp
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(KvDBKvGetDataTest, SetNextBeginTime001Virtual, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(SetNextBeginTimeTest001());
}

/**
 * @tc.name: CloseStore001
 * @tc.desc: Test Kv Store Close Action.
 * @tc.type: FUNC
 * @tc.require: AR000H2QPN
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBKvGetDataTest, CloseStore001Virtual, TestSize.Level1)
{
    /**
     * @tc.steps: step1. new storeTest and get connection now ref countTest is 2.
     * @tc.expected: Succeed.
     */
    auto storeTest = new (std::nothrow) SQLiteKvStore();
    ASSERT_NE(storeTest, nullptr);
    KvDBProperties proper;
    InitStorePropVirtual(g_storeTestPathVirtual, APP_ID, USER_ID, proper);
    EXPECT_EQ(storeTest->Open(proper), E_OK);
    int errCodeTest = E_OK;
    auto connection = storeTest->GetDBConnection(errCodeTest);
    EXPECT_EQ(errCodeTest, E_OK);
    ASSERT_NE(connection, nullptr);
    errCodeTest = storeTest->RegisterLifeCycleCallback([](const std::string &, const std::string &) {
    });
    EXPECT_EQ(errCodeTest, E_OK);
    errCodeTest = storeTest->RegisterLifeCycleCallback(nullptr);
    EXPECT_EQ(errCodeTest, E_OK);
    auto syncInterface = storeTest->GetStorageEngine();
    ASSERT_NE(syncInterface, nullptr);
    RefObject::IncObjRef(syncInterface);
    /**
     * @tc.steps: step2. release storeTest.
     * @tc.expected: Succeed.
     */
    storeTest->ReleaseDBConnection(1, connection); // 1 is connection id
    RefObject::DecObjRef(storeTest);
    storeTest = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    /**
     * @tc.steps: step3. try trigger heart beat after storeTest release.
     * @tc.expected: No crash.
     */
    Timestamp maxTimestamp = 0u;
    syncInterface->GetMaxTimestamp(maxTimestamp);
    RefObject::DecObjRef(syncInterface);
}

/**
 * @tc.name: ReleaseContinueTokenTest001
 * @tc.desc: Test relaese continue tokenTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(KvDBKvGetDataTest, ReleaseContinueTokenTest001Virtual, TestSize.Level1)
{
    ASSERT_EQ(g_mgrTest.OpenStore(g_storeTestPathVirtual, g_storeTestVir, queryTest g_delegateTest), DBStatus::OK);
    ASSERT_NE(g_delegateTest, nullptr);
    ASSERT_EQ(g_delegateTest->CreateDistributedTable(g_tableNameVirtual), DBStatus::OK);

    for (int i = 1; i <= 5; ++i) { // 5 is loop countTest
        EXPECT_EQ(PutBatch(1, i * 1024 * 1024), E_OK);  // 1024*1024 equals 1M.
    }

    EXPECT_EQ(g_mgrTest.CloseStore(g_delegateTest), DBStatus::OK);
    g_delegateTest = nullptr;
    auto storeTest = new(std::nothrow) SQLiteKvStore();
    ASSERT_NE(storeTest, nullptr);
    KvDBProperties proper;
    InitStorePropVirtual(g_storeTestPathVirtual, APP_ID, USER_ID, proper);
    EXPECT_EQ(storeTest->Open(proper), E_OK);
    int errCodeTest = E_OK;
    auto connection = storeTest->GetDBConnection(errCodeTest);
    EXPECT_EQ(errCodeTest, E_OK);
    ASSERT_NE(connection, nullptr);
    errCodeTest = storeTest->RegisterLifeCycleCallback([](const std::string &, const std::string &) {
    });
    EXPECT_EQ(errCodeTest, E_OK);
    errCodeTest = storeTest->RegisterLifeCycleCallback(nullptr);
    EXPECT_EQ(errCodeTest, E_OK);
    auto syncInterface = storeTest->GetStorageEngine();
    ASSERT_NE(syncInterface, nullptr);

    ContinueToken tokenTest = nullptr;
    QueryObject queryTest(Query::Select(g_tableNameVirtual));
    std::vector<SingleVerKvEntry *> entriesItem;
    DataSizeSpec sizeInfo;
    sizeInfo.blockSize = 1 * 1024 * 1024;  // permit 1M.
    EXPECT_EQ(syncInterface->GetSyncData(queryTest, SyncTimeRange {}, size, tokenTest, entriesItem), -E_UNFINISHED);
    EXPECT_NE(tokenTest, nullptr);
    syncInterface->ReleaseContinueToken(tokenTest);
    RefObject::IncObjRef(syncInterface);
    SingleVerKvEntry::Release(entriesItem);
    storeTest->ReleaseDBConnection(1, connection); // 1 is connection id
    RefObject::DecObjRef(storeTest);
    storeTest = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RefObject::DecObjRef(syncInterface);
}

/**
 * @tc.name: StopSync001
 * @tc.desc: Test Kv Stop Sync Action.
 * @tc.type: FUNC
 * @tc.require: AR000H2QPN
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBKvGetDataTest, StopSync001Virtual, TestSize.Level1)
{
    KvDBProperties proper;
    InitStorePropVirtual(g_storeTestPathVirtual, APP_ID, USER_ID, proper);
    proper.SetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, true);
    const int loopCount = 1000;
    std::thread userChangeThread([]() {
        for (int i = 0; i < loopCount; ++i) {
            RuntimeConfig::NotifyUserChanged();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    std::string hashIdenti = proper.GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    proper.SetStringProp(DBProperties::DUAL_TUPLE_IDENTIFIER_DATA, hashIdenti);
    OpenDbProperties option;
    option.uri = proper.GetStringProp(DBProperties::DATA_DIR, "");
    option.createIfNecessary = true;
    for (int i = 0; i < loopCount; ++i) {
        auto sqliteStorageEngine = std::make_shared<SQLiteSingleKvStorageEngine>(proper);
        ASSERT_NE(sqliteStorageEngine, nullptr);
        StorageEngineAttr poolSize = {1, 1, 0, 16}; // at most 1 write 16 read.
        int errCodeTest = sqliteStorageEngine->InitSQLiteStorageEngine(poolSize, option, hashIdenti);
        EXPECT_EQ(errCodeTest, E_OK);
        auto storageEngine = new (std::nothrow) KvSyncAbleStorage(sqliteStorageEngine);
        ASSERT_NE(storageEngine, nullptr);
        auto syncAbleEngine = std::make_unique<SyncAbleEngine>(storageEngine);
        ASSERT_NE(syncAbleEngine, nullptr);
        syncAbleEngine->WakeUpSyncer();
        syncAbleEngine->Close();
        RefObject::KillAndDecObjRef(storageEngine);
    }
    userChangeThread.join();
}

/**
 * @tc.name: EraseDeviceWaterMark001
 * @tc.desc: Test Kv erase water mark.
 * @tc.type: FUNC
 * @tc.require: AR000H2QPN
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBKvGetDataTest, EraseDeviceWaterMark001Virtual, TestSize.Level1)
{
    auto syncAbleEngine = std::make_unique<SyncAbleEngine>(nullptr);
    ASSERT_NE(syncAbleEngine, nullptr);
    EXPECT_EQ(syncAbleEngine->EraseDeviceWaterMark("", true), -E_INVALID_ARGS);
}
}
