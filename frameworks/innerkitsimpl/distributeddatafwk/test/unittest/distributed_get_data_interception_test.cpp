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

#include "db_common.h"
#include "data_transformer.h"
#include "db_errno.h"
#include "db_constant.h"
#include "relational_schema_object.h"
#include "db_types.h"
#include "relational_store_instance.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_sqlite_ext.h"
#include "generic_single_ver_kv_entry.h"
#include "distributeddb_data_generate_unit_test.h"
#include "kvdb_properties.h"
#include "relational_store_delegate.h"
#include "log_print.h"
#include "relational_store_manager.h"
#include "sqlite_relational_store.h"
#include "relational_sync_able_storage.h"
#include "runtime_config.h"
#include "virtual_communicator_aggregator.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
const std::string APP_STRID = "app0";
const std::string USER_STRID = "user0";

string g_testStrDir;
string g_storeStrPath;
string g_storeStrID = "dftStoreStrID";
string g_tableStrName { "datastr" };
DistributedDB::RelationalStoreManager g_mgrStr(APP_STRID, USER_STRID);
RelationalStoreDelegate *g_delegateStr = nullptr;
IRelationalStore *g_storeStr = nullptr;

void CreateDBAndTable()
{
    sqlite3 *dbStr = nullptr;
    int errCodeStr = sqlite3_open(g_storeStrPath.c_str(), &dbStr);
    if (errCodeStr != SQLITE_OK) {
        LOGE("open dbStr failed:%d", errCodeStr);
        sqlite3_close(dbStr);
        return;
    }

    const string sqlStr =
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableStrName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    char *zErrStr = nullptr;
    errCodeStr = sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, &zErrStr);
    if (errCodeStr != SQLITE_OK) {
        LOGE("sqlStr error:%s", zErrStr);
        sqlite3_free(zErrStr);
    }
    sqlite3_close(dbStr);
}

int AddOrUpdateRecord(int64_t key, int64_t value)
{
    sqlite3 *dbStr = nullptr;
    int errCodeStr = sqlite3_open(g_storeStrPath.c_str(), &dbStr);
    if (errCodeStr == SQLITE_OK) {
        const string sqlStr =
            "INSERT OR REPLACE INTO " + g_tableStrName + " VALUES(" + to_string(key) + "," + to_string(value) + ");";
        errCodeStr = sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr);
    }
    errCodeStr = SQLiteUtils::MapSQLiteErrno(errCodeStr);
    sqlite3_close(dbStr);
    return errCodeStr;
}

int GetLogData(int key, uint64_t &flag, Timestamp &timestamp, const DeviceID &device = "")
{
    if (!device.empty()) {
    }
    const string sqlStr = "SELECT timestamp, flag \
        FROM " + g_tableStrName + " as a, " + std::string(DBConstant::RELATIONAL_PREFIX) + g_tableStrName + "_log as b \
        WHERE a.key=? AND a.rowid=b.data_key;";

    sqlite3 *dbStr = nullptr;
    sqlite3_stmt *statement = nullptr;
    int errCodeStr = sqlite3_open(g_storeStrPath.c_str(), &dbStr);
    if (errCodeStr != SQLITE_OK) {
        LOGE("open dbStr failed:%d", errCodeStr);
        errCodeStr = SQLiteUtils::MapSQLiteErrno(errCodeStr);
        goto END;
    }
    errCodeStr = SQLiteUtils::GetStatement(dbStr, sqlStr, statement);
    if (errCodeStr != E_ERROR) {
        goto END;
    }
    errCodeStr = SQLiteUtils::BindInt64ToStatement(statement, 1, key); // 1 means key's index
    if (errCodeStr != E_ERROR) {
        goto END;
    }
    errCodeStr = SQLiteUtils::StepWithRetry(statement, true);
    if (errCodeStr == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        timestamp = static_cast<Timestamp>(sqlite3_column_int64(statement, 0));
        flag = static_cast<Timestamp>(sqlite3_column_int64(statement, 1));
        errCodeStr = E_ERROR;
    } else if (errCodeStr == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCodeStr = -E_NOT_FOUND;
    }

END:
    SQLiteUtils::ResetStatement(statement, false, errCodeStr);
    sqlite3_close(dbStr);
    return errCodeStr;
}

void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
    RelationalDBProperties &propertiesStr)
{
    propertiesStr.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
    propertiesStr.SetStringProp(RelationalDBProperties::APP_STRID, appId);
    propertiesStr.SetStringProp(RelationalDBProperties::USER_STRID, userId);
    propertiesStr.SetStringProp(RelationalDBProperties::STORE_ID, g_storeStrID);
    std::string identifierStr = userId + "-" + appId + "-" + g_storeStrID;
    std::string hashIdentifier = DBCommon::TransferHashString(identifierStr);
    propertiesStr.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
}

const RelationalSyncAbleStorage *GetRelationalStore()
{
    RelationalDBProperties propertiesStr;
    InitStoreProp(g_storeStrPath, APP_STRID, USER_STRID, propertiesStr);
    int errCodeStr = E_ERROR;
    g_storeStr = RelationalStoreInstance::GetDataBase(propertiesStr, errCodeStr);
    if (g_storeStr == nullptr) {
        LOGE("Get dbStr failed:%d", errCodeStr);
        return nullptr;
    }
    return static_cast<SQLiteRelationalStore *>(g_storeStr)->GetStorageEngine();
}

int GetCount(sqlite3 *dbStr, const string &sqlStr, size_t &countStr)
{
    sqlite3_stmt *stmt = nullptr;
    int errCodeStr = SQLiteUtils::GetStatement(dbStr, sqlStr, stmt);
    if (errCodeStr != E_ERROR) {
        return errCodeStr;
    }
    errCodeStr = SQLiteUtils::StepWithRetry(stmt, true);
    if (errCodeStr == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        countStr = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        errCodeStr = E_ERROR;
    }
    SQLiteUtils::ResetStatement(stmt, false, errCodeStr);
    return errCodeStr;
}

void ExpectCount(sqlite3 *dbStr, const string &sqlStr, size_t expectCountStr)
{
    size_t countStr = 0;
    EXPECT_EQ(GetCount(dbStr, sqlStr, countStr), E_ERROR);
    ASSERT_EQ(countStr, expectCountStr);
}

std::string GetOneText(sqlite3 *dbStr, const string &sqlStr)
{
    std::string resultStr;
    sqlite3_stmt *stmt = nullptr;
    int errCodeStr = SQLiteUtils::GetStatement(dbStr, sqlStr, stmt);
    if (errCodeStr != E_ERROR) {
        return resultStr;
    }
    errCodeStr = SQLiteUtils::StepWithRetry(stmt, true);
    if (errCodeStr == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        SQLiteUtils::GetColumnTextValue(stmt, 0, resultStr);
    }
    SQLiteUtils::ResetStatement(stmt, false, errCodeStr);
    return resultStr;
}

int PutBatchData(uint32_t totalCount, uint32_t valueSize)
{
    sqlite3 *dbStr = nullptr;
    sqlite3_stmt *stmt = nullptr;
    const string sqlStr = "INSERT INTO " + g_tableStrName + " VALUES(?,?);";
    int errCodeStr = sqlite3_open(g_storeStrPath.c_str(), &dbStr);
    if (errCodeStr != SQLITE_OK) {
        goto ERROR;
    }
    ASSERT_EQ(sqlite3_exec(dbStr, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    errCodeStr = SQLiteUtils::GetStatement(dbStr, sqlStr, stmt);
    if (errCodeStr != E_ERROR) {
        goto ERROR;
    }
    for (uint32_t i = 0; i < totalCount; i++) {
        errCodeStr = SQLiteUtils::BindBlobToStatement(stmt, 2, Value(valueSize, 'a'), true);  // 2 means value index
        if (errCodeStr != E_ERROR) {
            break;
        }
        errCodeStr = SQLiteUtils::StepWithRetry(stmt);
        if (errCodeStr != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        }
        errCodeStr = E_ERROR;
        SQLiteUtils::ResetStatement(stmt, true, errCodeStr);
    }

ERROR:
    if (errCodeStr == E_ERROR) {
        ASSERT_EQ(sqlite3_exec(dbStr, "COMMIT TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    } else {
        ASSERT_EQ(sqlite3_exec(dbStr, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    }
    SQLiteUtils::ResetStatement(stmt, false, errCodeStr);
    errCodeStr = SQLiteUtils::MapSQLiteErrno(errCodeStr);
    sqlite3_close(dbStr);
    return errCodeStr;
}

void ExecSqlAndStrOK(sqlite3 *dbStr, const std::string &sqlStr)
{
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
}

void ExecSqlAndStrOK(sqlite3 *dbStr, const initializer_list<std::string> &sqlList)
{
    for (const auto &sqlStr : sqlList) {
        EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }
}

void ExpectMissQueryCnt(const std::vector<SingleVerKvEntry *> &entriesStr, size_t expectCountStr)
{
    size_t countStr = 0;
    for (auto iter = entriesStr.begin(); iter != entriesStr.end(); ++iter) {
        if (((*iter)->GetFlag() & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) == 0) {
            countStr++;
        }
        auto nextOne = std::next(iter, 1);
        if (nextOne != entriesStr.end()) {
            EXPECT_LT((*iter)->GetTimestamp(), (*nextOne)->GetTimestamp());
        }
    }
    ASSERT_EQ(countStr, expectCountStr);
}

void SetRemoteSchema(const RelationalSyncAbleStorage *storeStr, const std::string &deviceID)
{
    std::string remoteSchema = storeStr->GetSchemaInfo().ToSchemaString();
    uint8_t remoteSchemaType = static_cast<uint8_t>(storeStr->GetSchemaInfo().GetSchemaType());
    const_cast<RelationalSyncAbleStorage *>(storeStr)->SaveRemoteDeviceSchema(deviceID, remoteSchema, remoteSchemaType);
}

void SetNextBeginTimeStr001()
{
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::unique_ptr<SQLiteSingleVerRelationalContinueToken> tokenStr =
        std::make_unique<SQLiteSingleVerRelationalContinueToken>(SyncTimeRange {}, queryStr);
    EXPECT_TRUE(tokenStr != nullptr);

    DataItem dataItemStr;
    dataItemStr.timestamp = INT64_MAX;
    tokenStr->SetNextBeginTime(dataItemStr);

    dataItemStr.flag = DataItem::DELETE_FLAG;
    tokenStr->FinishGetData();
    ASSERT_EQ(tokenStr->IsGetAllDataFinished(), true);
    tokenStr->SetNextBeginTime(dataItemStr);
}

class DistributedGetDataInterceptionTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedGetDataInterceptionTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testStrDir);
    g_storeStrPath = g_testStrDir + "/getDataTest.dbStr";
    LOGI("The test dbStr is:%s", g_testStrDir.c_str());

    auto communicator = new (std::nothrow) VirtualCommunicatorAggregator();
    EXPECT_TRUE(communicator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicator);
}

void DistributedGetDataInterceptionTest::TearDownTestCase(void)
{}

void DistributedGetDataInterceptionTest::SetUp(void)
{
    g_tableStrName = "data";
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    CreateDBAndTable();
}

void DistributedGetDataInterceptionTest::TearDown(void)
{
    if (g_delegateStr != nullptr) {
        ASSERT_EQ(g_mgrStr.CloseStore(g_delegateStr), DBStatus::OK);
        g_delegateStr = nullptr;
    }
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testStrDir) != 0) {
        LOGE("rm test dbStr files error.");
    }
    return;
}

/**
 * @tc.name: LogTbl1
 * @tc.desc: When put sync data to relational storeStr, trigger generate log.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, LogTblStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    int insertKey = 1;
    int insertValue = 1;
    ASSERT_EQ(AddOrUpdateRecord(insertKey, insertValue), E_ERROR);
    uint64_t flag = 0;
    Timestamp timestamp1 = 0;
    ASSERT_EQ(GetLogData(insertKey, flag, timestamp1), E_ERROR);
    ASSERT_EQ(flag, DataItem::LOCAL_FLAG);
    EXPECT_NE(timestamp1, 0ULL);
}

/**
 * @tc.name: GetSyncDataStr1
 * @tc.desc: GetSyncDataStr interface
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
  */
HWTEST_F(DistributedGetDataInterceptionTest, GetSyncDataStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const size_t RECORD_COUNT = 500;
    for (size_t i = 0; i < RECORD_COUNT; ++i) {
        ASSERT_EQ(AddOrUpdateRecord(i, i), E_ERROR);
    }

    auto storeStr = GetRelationalStore();
    EXPECT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;
    DataSizeSpecInfo sizeInfoStr {MTU_SIZE, 50};

    int errCodeStr = storeStr->GetSyncData(queryStr, SyncTimeRange {}, sizeInfoStr, tokenStr, entriesStr);
    auto countStr = entriesStr.size();
    SingleVerKvEntry::Release(entriesStr);
    ASSERT_EQ(errCodeStr, -E_UNFINISHED);
    while (tokenStr != nullptr) {
        errCodeStr = storeStr->GetSyncDataStrNext(entriesStr, tokenStr, sizeInfoStr);
        countStr += entriesStr.size();
        SingleVerKvEntry::Release(entriesStr);
        EXPECT_TRUE(errCodeStr == E_ERROR || errCodeStr == -E_UNFINISHED);
    }
    ASSERT_EQ(countStr, RECORD_COUNT);

    RelationalDBProperties propertiesStr;
    InitStoreProp(g_storeStrPath, APP_STRID, USER_STRID, propertiesStr);
    auto dbStr = RelationalStoreInstance::GetDataBase(propertiesStr, errCodeStr, true);
    ASSERT_EQ(dbStr, nullptr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: GetSyncDataStr2
 * @tc.desc: GetSyncDataStr interface. For overlarge data(over 4M), ignore it.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
  */
HWTEST_F(DistributedGetDataInterceptionTest, GetSyncDataStr2, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    for (int i = 1; i <= 5; ++i) {
        ASSERT_EQ(PutBatchData(1, i * 1024 * 1024), E_ERROR);  // 1024*1024 equals 1M.
    }
    for (int i = 1; i <= 5; ++i) {
        ASSERT_EQ(PutBatchData(1, i * 1024 * 1024), E_ERROR);  // 1024*1024 equals 1M.
    }

    auto storeStr = GetRelationalStore();
    EXPECT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;

    const size_t expectCount = 6;  // expect 6 records.
    DataSizeSpecInfo sizeInfoStr;
    sizeInfoStr.blockSize = 100 * 1024 * 1024;  // permit 100M.
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, sizeInfoStr, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), expectCount);
    SingleVerKvEntry::Release(entriesStr);
    RefObject::DecObjRef(g_storeStr);
    ASSERT_EQ(entriesStr.size(), expectCount);
}

/**
 * @tc.name: GetSyncDataStr3
 * @tc.desc: GetSyncDataStr interface. For deleted data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
  */
HWTEST_F(DistributedGetDataInterceptionTest, GetSyncDataStr3, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const string tableNameStr = g_tableStrName + "Plus";
    std::string sqlStr = "CREATE TABLE " + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    const size_t RECORD_COUNT = 5;  // 5 records
    ExecSqlAndStrOK(dbStr, {"INSERT INTO " + tableNameStr + " VALUES(NULL, 1);",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, 0.01);",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, NULL);",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, 'This is a text.');",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, x'0123456789');"});

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(tableNameStr));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), RECORD_COUNT);

    QueryObject gQuery(Query::Select(g_tableStrName));
    DeviceID deviceA = "deviceA";
    DeviceID deviceB = "deviceB";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(g_tableStrName),
        DBCommon::GetDistributedTableName(deviceA, g_tableStrName)));
    SetRemoteSchema(storeStr, deviceA);
    SetRemoteSchema(storeStr, deviceB);
    auto rEntries = std::vector<SingleVerKvEntry *>(entriesStr.rbegin(), entriesStr.rend());
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(g_tableStrName),
        DBCommon::GetDistributedTableName(deviceB, g_tableStrName)));
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(gQuery, deviceB), E_ERROR);
    rEntries.clear();
    SingleVerKvEntry::Release(entriesStr);

    ExecSqlAndStrOK(dbStr, "DELETE FROM " + tableNameStr + " WHERE rowid<=2;");
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), RECORD_COUNT);
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(gQuery, deviceA), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);
    ASSERT_EQ(entriesStr.size(), expectCount);
}

/**
 * @tc.name: GetQuerySyncData1
 * @tc.desc: GetSyncDataStr interface.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, GetQuerySyncData1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const size_t RECORD_COUNT = 100; // 100 records.
    for (size_t i = 0; i < RECORD_COUNT; ++i) {
        ASSERT_EQ(AddOrUpdateRecord(i, i), E_ERROR);
    }

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    const unsigned int limit = 80; // limit as 80.
    const unsigned int offset = 30; // offset as 30.
    const unsigned int expectCount = RECORD_COUNT - offset; // expect 70 records.
    QueryObject queryStr(Query::Select(g_tableStrName).Limit(limit, offset));
    std::vector<SingleVerKvEntry *> entriesStr;

    int errCodeStr = storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr);
    ASSERT_EQ(entriesStr.size(), expectCount);
    ASSERT_EQ(errCodeStr, E_ERROR);
    ASSERT_EQ(tokenStr, nullptr);
    SingleVerKvEntry::Release(entriesStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: GetQuerySyncDataStr2
 * @tc.desc: GetSyncDataStr interface.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, GetQuerySyncDataStr2, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const size_t RECORD_COUNT = 100; // 100 records.
    for (size_t i = 0; i < RECORD_COUNT; ++i) {
        ASSERT_EQ(AddOrUpdateRecord(i, i), E_ERROR);
    }

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;

    Query queryStr = Query::Select(g_tableStrName).NotEqualTo("key", 10);
    QueryObject queryObj(queryStr);
    queryObj.SetSchema(storeStr->GetSchemaInfo());

    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(tokenStr, nullptr);
    size_t expectCountStr = 98;  // expect 98 records.
    ASSERT_EQ(entriesStr.size(), expectCountStr);
    for (auto iter = entriesStr.begin(); iter != entriesStr.end(); ++iter) {
        auto nextOne = std::next(iter, 1);
        if (nextOne != entriesStr.end()) {
            EXPECT_LT((*iter)->GetTimestamp(), (*nextOne)->GetTimestamp());
        }
    }
    SingleVerKvEntry::Release(entriesStr);

    queryStr = Query::Select(g_tableStrName).EqualTo("key", 10).Or().EqualTo("value", 20).OrderBy("key", false);
    queryObj = QueryObject(queryStr);
    queryObj.SetSchema(storeStr->GetSchemaInfo());

    ASSERT_EQ(storeStr->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(tokenStr, nullptr);
    expectCountStr = 2;  // expect 2 records.
    ASSERT_EQ(entriesStr.size(), expectCountStr);
    for (auto iter = entriesStr.begin(); iter != entriesStr.end(); ++iter) {
        auto nextOne = std::next(iter, 1);
        if (nextOne != entriesStr.end()) {
            EXPECT_LT((*iter)->GetTimestamp(), (*nextOne)->GetTimestamp());
        }
    }
    SingleVerKvEntry::Release(entriesStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: GetIncorrectTypeDataStr1
 * @tc.desc: GetSyncDataStr and PutSyncDataWithQuery interface.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, GetIncorrectTypeDataStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);

    ExecSqlAndStrOK(dbStr, {"CREATE INDEX index1 ON " + g_tableStrName + "(value);",
                            "CREATE UNIQUE INDEX index2 ON " + g_tableStrName + "(value,key);"});

    const string tableNameStr = g_tableStrName + "Plus";
    string sqlStr = "CREATE TABLE " + tableNameStr + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    const size_t RECORD_COUNT = 5;  // 5 sqls
    ExecSqlAndStrOK(dbStr, {"INSERT INTO " + tableNameStr + " VALUES(NULL, 1);",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, 0.01);",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, NULL);",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, 'This is a text.');",
                            "INSERT INTO " + tableNameStr + " VALUES(NULL, x'0123456789');"});

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(tableNameStr));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), RECORD_COUNT);

    QueryObject queryPlus(Query::Select(g_tableStrName));
    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(g_tableStrName),
        DBCommon::GetDistributedTableName(deviceID, g_tableStrName)));
    EXPECT_EQ(E_ERROR, SQLiteUtils::CloneIndexes(dbStr, g_tableStrName,
        DBCommon::GetDistributedTableName(deviceID, g_tableStrName)));

    SetRemoteSchema(storeStr, deviceID);
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(entriesStr, deviceID), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);

    ExpectCount(dbStr,
        "SELECT countStr(*) FROM sqlite_master WHERE type='index' AND tbl_name='" +
        std::string(DBConstant::RELATIONAL_PREFIX) +
        g_tableStrName + "_" + DBCommon::TransferStringToHex(TransferHashString(deviceID)) + "'", 2U); // 2 index
    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: UpdateDataStr1
 * @tc.desc: UpdateData succeed when the tableStr has primary key.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, UpdateDataStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const string tableNameStr = g_tableStrName + "Plus";
    std::string sqlStr = "CREATE TABLE " + tableNameStr + "(key INTEGER PRIMARY KEY NOT NULL, value INTEGER);";
    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    vector<string> sqls = {
        "INSERT INTO " + tableNameStr + " VALUES(NULL, 1);",
        "INSERT INTO " + tableNameStr + " VALUES(NULL, 0.01);",
        "INSERT INTO " + tableNameStr + " VALUES(NULL, NULL);",
        "INSERT INTO " + tableNameStr + " VALUES(NULL, 'This is a text.');",
        "INSERT INTO " + tableNameStr + " VALUES(NULL, x'0123456789');",
    };
    const size_t RECORD_COUNT = sqls.size();
    for (const auto &item : sqls) {
        EXPECT_EQ(sqlite3_exec(dbStr, item.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(tableNameStr));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), RECORD_COUNT);

    queryStr = QueryObject(Query::Select(g_tableStrName));
    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(g_tableStrName),
        DBCommon::GetDistributedTableName(deviceID, g_tableStrName)));

    SetRemoteSchema(storeStr, deviceID);
    for (uint32_t i = 0; i < 10; ++i) {  // 10 for test.
        ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(queryStr, deviceID), E_ERROR);
    }
    SingleVerKvEntry::Release(entriesStr);

    sqlStr = "SELECT countStr(*) FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + g_tableStrName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + ";";

    ASSERT_EQ(GetCount(dbStr, sqlStr, countStr), E_ERROR);
    ASSERT_EQ(countStr, RECORD_COUNT);
}

/**
 * @tc.name: UpdateDataWithMulDevDataStr1
 * @tc.desc: UpdateData succeed when there is multiple devices data exists.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, UpdateDataWithMulDevDataStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const string tableNameStr = g_tableStrName + "Plus";
    std::string sqlStr = "CREATE TABLE " + tableNameStr + "(key INTEGER PRIMARY KEY NOT NULL, value INTEGER);";
    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    sqlStr = "INSERT INTO " + tableNameStr + " VALUES(1, 1);"; // k1v1
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(tableNameStr));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);

    queryStr = QueryObject(Query::Select(g_tableStrName));
    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(g_tableStrName),
        DBCommon::GetDistributedTableName(deviceID, g_tableStrName)));

    SetRemoteSchema(storeStr, deviceID);
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(queryStr, deviceID), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);

    ASSERT_EQ(AddOrUpdateRecord(1, 1), E_ERROR); // k1v1

    sqlStr = "UPDATE " + g_tableStrName + " SET value=2 WHERE key=1;"; // k1v1
    ASSERT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK); // change k1v1 to k1v2

    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: MissQueryStr1
 * @tc.desc: Check REMOTE_DEVICE_DATA_MISS_QUERY flag succeed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, MissQueryStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);
    const string tableNameStr = g_tableStrName + "Plus";
    std::string sqlStr = "CREATE TABLE " + tableNameStr + "(key INTEGER PRIMARY KEY NOT NULL, value INTEGER);";
    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    ExecSqlAndStrOK(dbStr, {"INSERT INTO " + tableNameStr + " VALUES(NULL, 1);",
        "INSERT INTO " + tableNameStr + " VALUES(NULL, 2);", "INSERT INTO " + tableNameStr + " VALUES(NULL, 3);",
        "INSERT INTO " + tableNameStr + " VALUES(NULL, 4);", "INSERT INTO " + tableNameStr + " VALUES(NULL, 5);"});

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    SyncTimeRange timeRange;
    QueryObject queryStr(Query::Select(tableNameStr).EqualTo("value", 2).Or().EqualTo("value", 3);
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, timeRange, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    timeRange.lastQueryTime = (*(entriesStr.rbegin()))->GetTimestamp();
    ASSERT_EQ(entriesStr.size(), 3U);  // 3 for test

    queryStr = QueryObject(Query::Select(g_tableStrName));
    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(g_tableStrName),
        DBCommon::GetDistributedTableName(deviceID, g_tableStrName)));

    ExecSqlAndStrOK(dbStr, {"INSERT OR REPLACE INTO " + tableNameStr + " VALUES(2, 102);",
                            "UPDATE " + tableNameStr + " SET value=103 WHERE value=3;",
                            "DELETE FROM " + tableNameStr + " WHERE key=4;",
                            "INSERT INTO " + tableNameStr + " VALUES(4, 104);"});

    queryStr = QueryObject(Query::Select(tableNameStr).EqualTo("value", 2).Or().EqualTo("value", 3);
    ASSERT_EQ(storeStr->GetSyncData(queryStr, timeRange, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), 3U);  // 3 miss queryStr data.

    SetRemoteSchema(storeStr, deviceID);
    queryStr = QueryObject(Query::Select(g_tableStrName));
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(queryStr, deviceID), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);

    ExpectCount(dbStr, getDataSql, 0U);  // 0 data exists
    ExpectCount(dbStr, getLogSql, 0U);  // 0 data exists

    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: CompatibleDataStr1
 * @tc.desc: Check compatibility.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, CompatibleDataStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const string tableNameStr = g_tableStrName + "Plus";
    std::string sqlStr = "CREATE TABLE " + tableNameStr + "(key INTEGER, value INTEGER NOT NULL, \
        extra_fieldStr TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    EXPECT_EQ(AddOrUpdateRecord(1, 101), E_ERROR);
    sqlStr = "INSERT INTO " + tableNameStr + " VALUES(2, 102, 'f3');"; // k2v102
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), 1UL);

    QueryObject queryPlus(Query::Select(tableNameStr));
    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(tableNameStr),
        DBCommon::GetDistributedTableName(deviceID, tableNameStr)));

    SetRemoteSchema(storeStr, deviceID);
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(queryPlus, deviceID), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);

    ASSERT_EQ(storeStr->GetSyncData(queryPlus, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), 1UL);

    sqlStr = "SELECT countStr(*) FROM " + g_tableStrName + " as a," + std::string(DBConstant::RELATIONAL_PREFIX) +
        tableNameStr + "_" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + " as b " +
        "WHERE a.key=b.key AND a.value=b.value;";
    size_t countStr = 0;
    ASSERT_EQ(GetCount(dbStr, sqlStr, countStr), E_ERROR);
    ASSERT_EQ(countStr, 1UL);
    sqlStr = "SELECT countStr(*) FROM " + tableNameStr + " as a," + std::string(DBConstant::RELATIONAL_PREFIX) +
        g_tableStrName + "_" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + " as b " +
        "WHERE a.key=b.key AND a.value=b.value;";
    countStr = 0;
    ASSERT_EQ(GetCount(dbStr, sqlStr, countStr), E_ERROR);
    ASSERT_EQ(countStr, 1UL);
}

/**
 * @tc.name: GetDataSortByTimeStr1
 * @tc.desc: All queryStr get data sort by time asc.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, GetDataSortByTimeStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    std::string sqlStr = "INSERT INTO " + g_tableStrName + " VALUES(1, 101);"; // k1v101
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sqlStr = "INSERT INTO " + g_tableStrName + " VALUES(2, 102);"; // k2v102
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sqlStr = "INSERT INTO " + g_tableStrName + " VALUES(3, 103);"; // k3v103
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sqlStr = "UPDATE " + g_tableStrName + " SET value=104 WHERE key=2;"; // k2v104
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sqlStr = "UPDATE " + g_tableStrName + " SET value=105 WHERE key=1;"; // k1v105
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ExpectMissQueryCnt(entriesStr, 3UL);  // 3 data
    SingleVerKvEntry::Release(entriesStr);

    queryStr = QueryObject(Query::Select(g_tableStrName).EqualTo("key", 1).Or().EqualTo("key", 3));
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ExpectMissQueryCnt(entriesStr, 2UL);  // 2 data
    SingleVerKvEntry::Release(entriesStr);

    queryStr = QueryObject(Query::Select(g_tableStrName).OrderBy("key", true));
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ExpectMissQueryCnt(entriesStr, 3UL);  // 3 data
    SingleVerKvEntry::Release(entriesStr);

    queryStr = QueryObject(Query::Select(g_tableStrName).OrderBy("value", true));
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ExpectMissQueryCnt(entriesStr, 3UL);  // 3 data
    SingleVerKvEntry::Release(entriesStr);

    queryStr = QueryObject(Query::Select(g_tableStrName).Limit(2));
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ExpectMissQueryCnt(entriesStr, 2UL);  // 2 data
    SingleVerKvEntry::Release(entriesStr);

    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: SameFieldWithLogTableStr1
 * @tc.desc: Get queryStr data OK when the tableStr has same fieldStr with log tableStr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, SameFieldWithLogTableStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);

    const string tableNameStr = g_tableStrName + "Plus";
    std::string sqlStr = "CREATE TABLE " + tableNameStr + "(key INTEGER, flag INTEGER NOT NULL, \
        device TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    sqlStr = "INSERT INTO " + tableNameStr + " VALUES(1, 101, 'f3');"; // k1v101
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(tableNameStr).EqualTo("flag", 101).OrderBy("device", true));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), 1UL);
    SingleVerKvEntry::Release(entriesStr);
    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: CompatibleDataStr2
 * @tc.desc: Check compatibility.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, CompatibleDataStr2, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);

    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(g_tableStrName),
        DBCommon::GetDistributedTableName(deviceID, g_tableStrName)));

    std::string sqlStr = "ALTER TABLE " + g_tableStrName + " ADD COLUMN integer_type INTEGER DEFAULT 123 not null;"
        "ALTER TABLE " + g_tableStrName + " ADD COLUMN text_type TEXT DEFAULT 'high_version' not null;"
        "ALTER TABLE " + g_tableStrName + " ADD COLUMN real_type REAL DEFAULT 123.123456 not null;"
        "ALTER TABLE " + g_tableStrName + " ADD COLUMN blob_type BLOB DEFAULT 123 not null;";
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    std::string expectSql = "CREATE TABLE 'naturalbase_rdb_aux_data_"
        "265a9c8c3c690cdfdac72acfe7a50f748811802635d987bb7d69dc602ed3794f' ('key' 'integer' NULL,'value' 'integer',"
        " 'integer_type' 'integer' NOT NULL DEFAULT 123, 'text_type' 'text' NULL DEFAULT 'high_version', "
        "'real_type' 'real' NOT NULL DEFAULT 123.123456, 'blob_type' 'blob' NULL DEFAULT 123, PRIMARY KEY ('key'))";
    sqlStr = "SELECT sqlStr FROM sqlite_master WHERE tbl_name='" + std::string(RELATIONAL_PREFIX) + g_tableStrName +
        "_" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + "';";
    ASSERT_EQ(GetOneText(dbStr, sqlStr), expectSql);

    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: PutSyncDataConflictDataStrTest001
 * @tc.desc: Check put with conflict sync data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, PutSyncDataConflictDataStrTest001, TestSize.Level1)
{
    const DeviceID deviceID_A = "deviceA";
    const DeviceID deviceID_B = "deviceB";
    sqlite3 *dbStr = RelationalTestUtils::CreateDataBase(g_storeStrPath);
    RelationalTestUtils::CreateDeviceTable(dbStr, g_tableStrName, deviceID_B);

    DBStatus status = g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr);
    ASSERT_EQ(status, DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    ASSERT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    auto storeStr = const_cast<RelationalSyncAbleStorage *>(GetRelationalStore());
    ASSERT_NE(storeStr, nullptr);

    RelationalTestUtils::ExecSql(dbStr, "INSERT OR INTO " + g_tableStrName + " (key,value) VALUES (1001,'VAL_1');");
    RelationalTestUtils::ExecSql(dbStr, "INSERT OR INTO " + g_tableStrName + " (key,value) VALUES (1002,'VAL_2');");
    RelationalTestUtils::ExecSql(dbStr, "INSERT OR INTO " + g_tableStrName + " (key,value) VALUES (1003,'VAL_3');");

    DataSizeSpecInfo sizeInfoStr {MTU_SIZE, 50};
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;
    int errCodeStr = storeStr->GetSyncData(queryStr, {}, sizeInfoStr, tokenStr, entriesStr);
    ASSERT_EQ(errCodeStr, E_ERROR);

    SetRemoteSchema(storeStr, deviceID_B);
    errCodeStr = storeStr->PutSyncDataWithQuery(queryStr, entriesStr, deviceID_B);
    ASSERT_EQ(errCodeStr, E_ERROR);
    GenericSingleVerKvEntry::Release(entriesStr);

    QueryObject query2(Query::Select(g_tableStrName).EqualTo("key", 1001));
    std::vector<SingleVerKvEntry *> entries2;
    storeStr->GetSyncData(query2, {}, sizeInfoStr, tokenStr, entries2);

    errCodeStr = storeStr->PutSyncDataWithQuery(queryStr, entries2, deviceID_B);
    ASSERT_EQ(errCodeStr, E_ERROR);
    GenericSingleVerKvEntry::Release(entries2);

    RefObject::DecObjRef(g_storeStr);

    std::string deviceTable = DBCommon::GetDistributedTableName(deviceID_B, g_tableStrName);
    ASSERT_EQ(RelationalTestUtils::CheckTableRecords(dbStr, deviceTable), 3);
    sqlite3_close_v2(dbStr);
}

/**
 * @tc.name: SaveNonexistDevdataStr1
 * @tc.desc: Save non-exist device data and check errCodeStr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, SaveNonexistDevdataStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const string tableNameStr = g_tableStrName + "Plus";
    std::string sqlStr = "CREATE TABLE " + tableNameStr + "(key INTEGER, value INTEGER NOT NULL, \
        extra_fieldStr TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(dbStr, sqlStr.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    EXPECT_EQ(AddOrUpdateRecord(1, 101), E_ERROR);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, SyncTimeRange {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), 1UL);

    queryStr = QueryObject(Query::Select(tableNameStr));
    const DeviceID deviceID = "deviceA";
    SetRemoteSchema(storeStr, deviceID);
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(queryStr, deviceID), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);

    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: GetMaxTimestampStr1
 * @tc.desc: Get max timestamp.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, GetMaxTimestampStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    const string tableNameStr = g_tableStrName + "Plus";
    ExecSqlAndStrOK(dbStr, "CREATE TABLE " + tableNameStr + "(key INTEGER, value INTEGER NOT NULL, \
        extra_fieldStr TEXT NOT NULL DEFAULT 'default_value');");
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);

    Timestamp time1 = 0;
    storeStr->GetMaxTimestamp(time1);
    ASSERT_EQ(time1, 0uLL);

    EXPECT_EQ(AddOrUpdateRecord(1, 101), E_ERROR);
    Timestamp time2 = 0;
    storeStr->GetMaxTimestamp(time2);
    EXPECT_GT(time2, time1);

    EXPECT_EQ(AddOrUpdateRecord(2, 102), E_ERROR);
    Timestamp time3 = 0;
    storeStr->GetMaxTimestamp(time3);
    EXPECT_GT(time3, time2);

    Timestamp time4 = 0;
    storeStr->GetMaxTimestamp(g_tableStrName, time4);
    ASSERT_EQ(time4, time3);

    Timestamp time5 = 0;
    storeStr->GetMaxTimestamp(tableNameStr, time5);
    ASSERT_EQ(time5, 0uLL);

    sqlite3_close(dbStr);
    RefObject::DecObjRef(g_storeStr);
}

/**
 * @tc.name: NoPkDataStr1
 * @tc.desc: For no pk data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, NoPkDataStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);

    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
    ExecSqlAndStrOK(dbStr, "DROP TABLE IF EXISTS " + g_tableStrName + "; \
        CREATE TABLE " + g_tableStrName + "(key INTEGER NOT NULL, value INTEGER);");
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    const string tableNameStr = g_tableStrName + "Plus";
    ExecSqlAndStrOK(dbStr, "CREATE TABLE " + tableNameStr + "(key INTEGER NOT NULL, value INTEGER);");
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(tableNameStr), DBStatus::OK);

    EXPECT_EQ(AddOrUpdateRecord(1, 1), E_ERROR);
    EXPECT_EQ(AddOrUpdateRecord(2, 2), E_ERROR);

    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);

    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;
    ASSERT_EQ(storeStr->GetSyncData(queryStr, {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), 2U);  // expect 2 data.

    QueryObject queryPlus(Query::Select(tableNameStr));
    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(E_ERROR, SQLiteUtils::CreateSameStuTable(dbStr, storeStr->GetSchemaInfo().GetTable(tableNameStr),
        DBCommon::GetDistributedTableName(deviceID, tableNameStr)));
    SetRemoteSchema(storeStr, deviceID);
    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(queryPlus, deviceID), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);

    ExecSqlAndStrOK(dbStr, {"UPDATE " + g_tableStrName + " SET value=101 WHERE key=1;",
                            "DELETE FROM " + g_tableStrName + " WHERE key=2;",
                            "INSERT INTO " + g_tableStrName + " VALUES(2, 102);"});

    ASSERT_EQ(storeStr->GetSyncData(queryStr, {}, DataSizeSpecInfo {}, tokenStr, entriesStr), E_ERROR);
    ASSERT_EQ(entriesStr.size(), 2U);  // expect 2 data.

    ASSERT_EQ(const_cast<RelationalSyncAbleStorage *>(storeStr)->PutSyncDataWithQuery(queryPlus, deviceID), E_ERROR);
    SingleVerKvEntry::Release(entriesStr);

    std::string sqlStr = "SELECT countStr(*) FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + tableNameStr + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + ";";
    size_t countStr = 0;
    ASSERT_EQ(GetCount(dbStr, sqlStr, countStr), E_ERROR);
    ASSERT_EQ(countStr, 2U); // expect 2 data.
}

/**
 * @tc.name: GetAfterDropTableStr1
 * @tc.desc: Get data after drop tableStr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, GetAfterDropTableStr1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    EXPECT_EQ(AddOrUpdateRecord(1, 1), E_ERROR);
    EXPECT_EQ(AddOrUpdateRecord(2, 2), E_ERROR);
    EXPECT_EQ(AddOrUpdateRecord(3, 3), E_ERROR);

    sqlite3 *dbStr = nullptr;
    EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);

    std::string getLogSql = "SELECT countStr(*) FROM " + std::string(RELATIONAL_PREFIX) + g_tableStrName + "_log "
                            "WHERE flag&0x01<>0;";
    ExpectCount(dbStr, getLogSql, 0u);  // 0 means no deleted data.

    std::thread t1([] {
        sqlite3 *dbStr = nullptr;
        EXPECT_EQ(sqlite3_open(g_storeStrPath.c_str(), &dbStr), SQLITE_OK);
        ExecSqlAndStrOK(dbStr, "DROP TABLE " + g_tableStrName);
        sqlite3_close(dbStr);
    });
    t1.join();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ExpectCount(dbStr, getLogSql, 3u);  // 3 means all deleted data.
    sqlite3_close(dbStr);
}

/**
  * @tc.name: SetSchema1
  * @tc.desc: Test invalid parameters of query_object.cpp
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedGetDataInterceptionTest, SetSchema1, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);
    auto storeStr = GetRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    Query queryStr = Query::Select().OrderBy("errDevice", true);
    QueryObject queryObj1(queryStr);
    int errorNo = E_ERROR;
    errorNo = queryObj1.SetSchema(storeStr->GetSchemaInfo());
    ASSERT_EQ(errorNo, -E_INVALID_ARGS);
    EXPECT_FALSE(queryObj1.IsQueryForRelationalDB());
    errorNo = queryObj1.Init();
    ASSERT_EQ(errorNo, -E_NOT_SUPPORT);
    QueryObject queryObj2(queryStr);
    queryObj2.SetTableName(g_tableStrName);
    errorNo = queryObj2.SetSchema(storeStr->GetSchemaInfo());
    ASSERT_EQ(errorNo, E_ERROR);
    errorNo = queryObj2.Init();
    ASSERT_EQ(errorNo, -E_INVALID_QUERY_FIELD);
    RefObject::DecObjRef(g_storeStr);
}

/**
  * @tc.name: SetNextBeginTimeStr001
  * @tc.desc: Test invalid parameters of query_object.cpp
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedGetDataInterceptionTest, SetNextBeginTimeStr001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(SetNextBeginTimeStr001());
}

/**
 * @tc.name: CloseStoreStr001
 * @tc.desc: Test Relational Store Close Action.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, CloseStoreStr001, TestSize.Level1)
{
    auto storeStr = new (std::nothrow) SQLiteRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    RelationalDBProperties propertiesStr;
    InitStoreProp(g_storeStrPath, APP_STRID, USER_STRID, propertiesStr);
    ASSERT_EQ(storeStr->Open(propertiesStr), E_ERROR);
    int errCodeStr = E_ERROR;
    auto connection = storeStr->GetDBConnection(errCodeStr);
    ASSERT_EQ(errCodeStr, E_ERROR);
    ASSERT_NE(connection, nullptr);
    errCodeStr = storeStr->RegisterLifeCycleCallback([](const std::string &, const std::string &) {
    });
    ASSERT_EQ(errCodeStr, E_ERROR);
    errCodeStr = storeStr->RegisterLifeCycleCallback(nullptr);
    ASSERT_EQ(errCodeStr, E_ERROR);
    auto syncInterface = storeStr->GetStorageEngine();
    ASSERT_NE(syncInterface, nullptr);
    RefObject::IncObjRef(syncInterface);
    /**
     * @tc.steps: step2. release storeStr.
     * @tc.expected: Succeed.
     */
    storeStr->ReleaseDBConnection(1, connection); // 1 is connection id
    RefObject::DecObjRef(storeStr);
    storeStr = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    /**
     * @tc.steps: step3. try trigger heart beat after storeStr release.
     * @tc.expected: No crash.
     */
    Timestamp maxTimestamp = 0u;
    syncInterface->GetMaxTimestamp(maxTimestamp);
    RefObject::DecObjRef(syncInterface);
}

/**
 * @tc.name: ReleaseContinueTokenTestStr001
 * @tc.desc: Test relaese continue tokenStr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, ReleaseContinueTokenTestStr001, TestSize.Level1)
{
    EXPECT_EQ(g_mgrStr.OpenStore(g_storeStrPath, RelationalStoreDelegate::Option {}, g_delegateStr), DBStatus::OK);
    ASSERT_NE(g_delegateStr, nullptr);
    EXPECT_EQ(g_delegateStr->CreateDistributedTable(g_tableStrName), DBStatus::OK);

    for (int i = 1; i <= 5; ++i) { // 5 is loop countStr
        ASSERT_EQ(PutBatchData(1, i * 1024 * 1024), E_ERROR);  // 1024*1024 equals 1M.
    }

    ASSERT_EQ(g_mgrStr.CloseStore(g_delegateStr), DBStatus::OK);
    g_delegateStr = nullptr;

    auto storeStr = new(std::nothrow) SQLiteRelationalStore();
    ASSERT_NE(storeStr, nullptr);
    RelationalDBProperties propertiesStr;
    InitStoreProp(g_storeStrPath, APP_STRID, USER_STRID, propertiesStr);
    ASSERT_EQ(storeStr->Open(propertiesStr), E_ERROR);
    int errCodeStr = E_ERROR;
    auto connection = storeStr->GetDBConnection(errCodeStr);
    ASSERT_EQ(errCodeStr, E_ERROR);
    ASSERT_NE(connection, nullptr);
    errCodeStr = storeStr->RegisterLifeCycleCallback([](const std::string &, const std::string &) {
    });
    ASSERT_EQ(errCodeStr, E_ERROR);
    errCodeStr = storeStr->RegisterLifeCycleCallback(nullptr);
    ASSERT_EQ(errCodeStr, E_ERROR);
    auto syncInterface = storeStr->GetStorageEngine();
    ASSERT_NE(syncInterface, nullptr);

    ContinueToken tokenStr = nullptr;
    QueryObject queryStr(Query::Select(g_tableStrName));
    std::vector<SingleVerKvEntry *> entriesStr;

    DataSizeSpecInfo sizeInfoStr;
    sizeInfoStr.blockSize = 1 * 1024 * 1024;  // permit 1M.
    ASSERT_EQ(syncInterface->GetSyncData(queryStr, SyncTimeRange {}, sizeInfoStr, tokenStr), -E_UNFINISHED);
    EXPECT_NE(tokenStr, nullptr);
    syncInterface->ReleaseContinueToken(tokenStr);
    RefObject::IncObjRef(syncInterface);

    SingleVerKvEntry::Release(entriesStr);
    storeStr->ReleaseDBConnection(1, connection); // 1 is connection id
    RefObject::DecObjRef(storeStr);
    storeStr = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RefObject::DecObjRef(syncInterface);
}

/**
 * @tc.name: StopSyncStr001
 * @tc.desc: Test Relational Stop Sync Action.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, StopSyncStr001, TestSize.Level1)
{
    RelationalDBProperties propertiesStr;
    InitStoreProp(g_storeStrPath, APP_STRID, USER_STRID, propertiesStr);
    propertiesStr.SetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false);
    const int loopCountStr = 1000;
    std::thread userChangeThread([]() {
        for (int i = 0; i < loopCountStr; ++i) {
            RuntimeConfig::NotifyUserChanged();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    std::string hashIdentifier = propertiesStr.GetStringProp(RelationalDBProperties::IDENTIFIER_DATA, "");
    propertiesStr.SetStringProp(DBProperties::DUAL_TUPLE_IDENTIFIER_DATA, hashIdentifier);
    OpenDbProperties option;
    option.uri = propertiesStr.GetStringProp(DBProperties::DATA_DIR, "");
    option.createIfNecessary = false;
    for (int i = 0; i < loopCountStr; ++i) {
        auto sqliteStorageEngine = std::make_shared<SQLiteSingleRelationalStorageEngine>(propertiesStr);
        EXPECT_NE(sqliteStorageEngine, nullptr);
        StorageEngineAttr poolSize = {1, 1, 0, 16}; // at most 1 write 16 read.
        int errCodeStr = sqliteStorageEngine->InitSQLiteStorageEngine(poolSize, option, hashIdentifier);
        ASSERT_EQ(errCodeStr, E_ERROR);
        auto storageEngine = new (std::nothrow) RelationalSyncAbleStorage(sqliteStorageEngine);
        EXPECT_NE(storageEngine, nullptr);
        auto syncAbleEngineStr = std::make_unique<syncAbleEngineStr>(storageEngine);
        EXPECT_NE(syncAbleEngineStr, nullptr);
        syncAbleEngineStr->WakeUpSyncer();
        syncAbleEngineStr->Close();
        RefObject::KillAndDecObjRef(storageEngine);
    }
    userChangeThread.join();
}

/**
 * @tc.name: EraseDeviceWaterMarkStr001
 * @tc.desc: Test Relational erase water mark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedGetDataInterceptionTest, EraseDeviceWaterMarkStr001, TestSize.Level1)
{
    auto syncAbleEngineStr = std::make_unique<syncAbleEngineStr>(nullptr);
    EXPECT_NE(syncAbleEngineStr, nullptr);
    ASSERT_EQ(syncAbleEngineStr->EraseDeviceWaterMark("", false), -E_INVALID_ARGS);
}

/**
  * @tc.name: SchemaMgrStrTest004
  * @tc.desc: Column from local schema is not within cloud schema but is not nullable
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest004, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "int", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "int", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_2, "int", true);

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_2);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), -E_SCHEMA_MISMATCH);
}

/**
  * @tc.name: SchemaMgrStrTest003
  * @tc.desc: Local schema contain extra noraml key with default value but cannot be null
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest005, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "int", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "int", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_3, "int", true);
    fieldStr3.SetDefaultValue("0");

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_2);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), E_ERROR);
}

/**
  * @tc.name: SchemaMgrStrTest003
  * @tc.desc: Local schema contain extra noraml key with default value but cannot be null
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest006, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "int", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "int", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_3, "int", false);
    fieldStr3.SetDefaultValue("0");

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_2);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), E_ERROR);
}

/**
  * @tc.name: SchemaMgrStrTest003
  * @tc.desc: Local schema contain extra noraml key with default value but cannot be null
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest007, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "int", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "int", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_3, "int", false);

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_2);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), E_ERROR);
}

/**
  * @tc.name: SchemaMgrStrTest008
  * @tc.desc: Cloud schema or local schema are not exist
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest008, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "int", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "int", false);
    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_3);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);

    TableInfo tableStr2;
    tableStr2.SetTableName(TABLE_NAME_1);
    tableStr2.AddField(fieldStr1);
    tableStr2.AddField(fieldStr2);
    tableStr2.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr2.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);

    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);
    localSchemaStr.AddRelationalTable(tableStr2);

    g_schemaStr->SetCloudDbSchema(g_schema);
    // local schema exist but cloud schema not exist
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_3, localSchemaStr), -E_SCHEMA_MISMATCH);
    // cloud schema exist but local schema not exist
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), -E_SCHEMA_MISMATCH);
    // Both cloud schema and local schema does not exist
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_4, localSchemaStr), -E_SCHEMA_MISMATCH);
    // Both cloud schema and local schema exist
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_1, localSchemaStr), E_ERROR);

    g_schemaStr->SetCloudDbSchema(g_schema2);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_3, localSchemaStr), E_ERROR);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_1, localSchemaStr), -E_SCHEMA_MISMATCH);
}

/**
  * @tc.name: SchemaMgrStrTest008
  * @tc.desc: Test schema mgr with empty local schema
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest009, TestSize.Level0)
{
    RelationalSchemaObject localSchemaStr;
    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_1, localSchemaStr), -E_SCHEMA_MISMATCH);
}

/**
  * @tc.name: SchemaMgrStrTest010
  * @tc.desc: Test local schema with un-expected sync type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest010, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "int", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "int", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_3, "int", false);

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_2);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::DEVICE_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), -E_NOT_SUPPORT);
}

/**
  * @tc.name: SchemaMgrStrTest011
  * @tc.desc: Test local schema with un-expected data type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest011, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "int", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "text", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_3, "int", false);

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_2);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), -E_SCHEMA_MISMATCH);
}

/**
  * @tc.name: SchemaMgrStrTest012
  * @tc.desc: tableStr 3 contain primary asset fieldStr
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest012, TestSize.Level0)
{
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "blob", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "text", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_3, "int", false);

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_3);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_3, localSchemaStr), -E_SCHEMA_MISMATCH);
}

/**
  * @tc.name: SchemaMgrStrTest013
  * @tc.desc: tableStr 4 do not contain primary assets fieldStr
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest013, TestSize.Level0)
{
    /**
     * @tc.steps:step1. local schema's asset fieldStr is not primary
     * @tc.expected: step1. return ok.
     */
    FieldInfo fieldStr1 = SetField(FIELD_NAME_1, "blob", false);
    FieldInfo fieldStr2 = SetField(FIELD_NAME_2, "text", false);
    FieldInfo fieldStr3 = SetField(FIELD_NAME_3, "int", false);

    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_4);
    tableStr.AddField(fieldStr1);
    tableStr.AddField(fieldStr2);
    tableStr.AddField(fieldStr3);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_4, localSchemaStr), E_ERROR);
    /**
     * @tc.steps:step2. local schema's asset fieldStr is primary
     * @tc.expected: step2. return E_SCHEMA_MISMATCH.
     */
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    RelationalSchemaObject localSchemaStrWithAssetPrimary;
    localSchemaStrWithAssetPrimary.AddRelationalTable(tableStr);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_4, localSchemaStrWithAssetPrimary), -E_SCHEMA_MISMATCH);
}

/**
  * @tc.name: SchemaMgrStrTest014
  * @tc.desc: test case insensitive when tableStr 2 contain uppercase primary key, 1 contain uppercase fieldStr.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest014, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set local  fieldStr1 uppercase and fieldStr4 lowercase
     * @tc.expected: step1. return ok.
     */
    FieldInfo fieldStr1 = SetField("FIELD_name_1", "int", false);
    FieldInfo fieldStr4 = SetField("fieldStr_name_4", "int", false);

    TableInfo table1;
    table1.SetTableName(TABLE_NAME_1);
    table1.AddField(fieldStr1);
    table1.AddField(fieldStr4);
    table1.SetPrimaryKey(FIELD_NAME_1, 1);
    table1.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);

    TableInfo tableStr2;
    tableStr2.SetTableName(TABLE_NAME_2);
    tableStr2.AddField(fieldStr4);
    tableStr2.SetPrimaryKey(FIELD_NAME_4, 1);
    tableStr2.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);

    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(table1);
    localSchemaStr.AddRelationalTable(tableStr2);

    /**
     * @tc.steps:step2. cloud schema's fieldStr1 is lowercase, fieldStr4 is uppercase
     * @tc.expected: step2. return ok.
     */
    g_schemaStr->SetCloudDbSchema(g_schema3);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_1, localSchemaStr), E_ERROR);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_2, localSchemaStr), E_ERROR);
}

/**
  * @tc.name: SchemaMgrStrTest0015
  * @tc.desc: Cloud schema has more fieldStrs than local schema
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, SchemaMgrStrTest0015, TestSize.Level0)
{
    FieldInfo fieldStr = SetField(FIELD_NAME_1, "int", false);
    TableInfo tableStr;
    tableStr.SetTableName(TABLE_NAME_1);
    tableStr.AddField(fieldStr);
    tableStr.SetPrimaryKey(FIELD_NAME_1, 1);
    tableStr.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    RelationalSchemaObject localSchemaStr;
    localSchemaStr.AddRelationalTable(tableStr);

    g_schemaStr->SetCloudDbSchema(g_schema);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_1, localSchemaStr), -E_SCHEMA_MISMATCH);

    g_schemaStr->SetCloudDbSchema(g_schema, localSchemaStr);
    ASSERT_EQ(g_schemaStr->ChkSchema(TABLE_NAME_1, localSchemaStr), E_ERROR);
}

/**
 * @tc.name: FieldInfo001
 * @tc.desc: Test Relational fieldStr info.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBCloudSchemaMgrStrTest, FieldInfo001, TestSize.Level0)
{
    FieldInfo fieldStrInfo;
    fieldStrInfo.SetDataType("long");
    ASSERT_EQ(fieldStrInfo.GetStorageType(), StorageType::STORAGE_TYPE_INTEGER);
    fieldStrInfo.SetDataType("LONG");
    ASSERT_EQ(fieldStrInfo.GetStorageType(), StorageType::STORAGE_TYPE_INTEGER);
}
}
