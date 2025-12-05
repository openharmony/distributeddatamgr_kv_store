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

#ifdef USE_DISTRIBUTEDDB_CLOUD
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBRDBConflictHandlerTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    static constexpr const char *CLOUD_SYNC_TABLE_A = "CLOUD_SYNC_TABLE_A";
    void InitTables(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitSchema(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitDistributedTable(const std::vector<std::string> &tables = {CLOUD_SYNC_TABLE_A});
    void Store1InsertStore2Pull();
    void Store1InsertStore2Pull(const Query &pushQuery);
    void Store1DeleteStore2Pull();
    void AbortCloudSyncTest(const std::function<void(void)> &forkFunc, const std::function<void(void)> &cancelForkFunc,
        SyncMode mode);
    void PrepareEnv(const UtDateBaseSchemaInfo &info);
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBConflictHandlerTest::SetUp()
{
    RDBGeneralUt::SetUp();
    // create db first
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1_, "dev1"), E_OK);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2_, "dev2"), E_OK);
    EXPECT_NO_FATAL_FAILURE(InitTables());
    InitSchema();
    InitDistributedTable();
}

void DistributedDBRDBConflictHandlerTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBConflictHandlerTest::InitTables(const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT, uuidCol TEXT UNIQUE)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
}

void DistributedDBRDBConflictHandlerTest::InitSchema(const std::string &table)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
        {{"uuidCol", TYPE_INDEX<std::string>, true, true, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info1_, schemaInfo);
    RDBGeneralUt::SetSchemaInfo(info2_, schemaInfo);
}

void DistributedDBRDBConflictHandlerTest::InitDistributedTable(const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1_);
    RDBGeneralUt::SetCloudDbConfig(info2_);
    ASSERT_EQ(SetDistributedTables(info1_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
}

void DistributedDBRDBConflictHandlerTest::Store1InsertStore2Pull()
{
    Query pushQuery = Query::Select().From(CLOUD_SYNC_TABLE_A).EqualTo("stringCol2", "text2");
    Store1InsertStore2Pull(pushQuery);
}

void DistributedDBRDBConflictHandlerTest::Store1InsertStore2Pull(const Query &pushQuery)
{
    // step1 store1 insert (id=1, intCol=1, stringCol1='text1', stringCol2='text2', uuidCol='uuid1')
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    // step2 check insert success
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='text2'"), 1);
    // step3 store1 push (intCol=1, stringCol1='text1', uuidCol='uuid1') to cloud
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, pushQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PUSH, OK, OK));
    // step4 store2 pull (intCol=1, stringCol1='text1', uuidCol='uuid1') from cloud
    Query pullQuery = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, pullQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PULL, OK, OK));
}

void DistributedDBRDBConflictHandlerTest::Store1DeleteStore2Pull()
{
    // step1 store1 delete (id=1, intCol=1, stringCol1='text1', stringCol2='text2', uuidCol='uuid1')
    auto ret = ExecuteSQL("DELETE FROM CLOUD_SYNC_TABLE_A WHERE uuidCol='uuid1'", info1_);
    ASSERT_EQ(ret, E_OK);
    // step2 check delete success
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A, "uuidCol='uuid1'"), 0);
    // step3 store1 push and store2 pull
    Query pushQuery = Query::Select().From(CLOUD_SYNC_TABLE_A).EqualTo("stringCol2", "text2");
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, pushQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PUSH, OK, OK));
    Query pullQuery = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, pullQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PULL, OK, OK));
}

void DistributedDBRDBConflictHandlerTest::PrepareEnv(const UtDateBaseSchemaInfo &info)
{
    RDBGeneralUt::SetSchemaInfo(info1_, info);
    RDBGeneralUt::SetSchemaInfo(info2_, info);
    RDBGeneralUt::SetCloudDbConfig(info1_);
    RDBGeneralUt::SetCloudDbConfig(info2_);
    std::vector<std::string> tables;
    for (const auto &item : info.tablesInfo) {
        tables.push_back(item.name);
    }
    ASSERT_EQ(SetDistributedTables(info1_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
}

/**
 * @tc.name: SimpleSync001
 * @tc.desc: Test store1 insert/delete local and custom push, store2 custom pull.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync001, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &old, const VBucket &, VBucket &) {
        EXPECT_EQ(old.find(CloudDbConstant::CREATE_FIELD), old.end());
        EXPECT_EQ(old.find(CloudDbConstant::MODIFY_FIELD), old.end());
        EXPECT_EQ(old.find(CloudDbConstant::VERSION_FIELD), old.end());
        return ConflictRet::UPSERT;
    });
    // step1 store1 push one row and store2 pull
    Store1InsertStore2Pull();
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2 IS NULL"), 1);
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &old, const VBucket &, VBucket &) {
        EXPECT_NE(old.find(CloudDbConstant::CREATE_FIELD), old.end());
        EXPECT_NE(old.find(CloudDbConstant::MODIFY_FIELD), old.end());
        return ConflictRet::DELETE;
    });
    // step2 store1 delete one row and store2 pull
    Store1DeleteStore2Pull();
    // step3 store2 check not exist uuidCol='uuid1'
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A, "uuidCol='uuid1'"), 0);
}

/**
 * @tc.name: SimpleSync002
 * @tc.desc: Test store1 insert local and custom push, store2 custom pull to update it.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync002, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        upsert.insert({"stringCol2", std::string("upsert")});
        return ConflictRet::UPSERT;
    });
    // step1 store1 insert (id=1, intCol=1, stringCol1='text1', stringCol2='text2', uuidCol='uuid1')
    //       store2 insert (id=2, intCol=1, stringCol1='text1', stringCol2='text3', uuidCol='uuid1')
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(2, 1, 'text1', 'text3', 'uuid1')", info2_);
    ASSERT_EQ(ret, E_OK);
    // step2 check insert success
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='text2'"), 1);
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='text3'"), 1);
    // step3 store1 push (intCol=1, stringCol1='text1', uuidCol='uuid1') to cloud
    Query query = Query::Select().From(CLOUD_SYNC_TABLE_A).EqualTo("stringCol2", "text2");
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PUSH, OK, OK));
    // step4 store2 pull (intCol=1, stringCol1='text1', uuidCol='uuid1') from cloud
    query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PULL, OK, OK));
    // step5 store2 check (intCol=1, stringCol1='text1', uuidCol='uuid1') exist and stringCol2 is 'text3'
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='upsert'"), 1);
    // make sure store2 only exist one row
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A), 1);
}

/**
 * @tc.name: SimpleSync003
 * @tc.desc: Test store1 insert and custom push, store2 custom pull with not handle.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync003, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        upsert.insert({"stringCol2", std::string("upsert")});
        return ConflictRet::NOT_HANDLE;
    });
    // step1 store1 push one row and store2 pull
    Store1InsertStore2Pull();
    // step2 store2 has no data
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A), 0);
}

/**
 * @tc.name: SimpleSync004
 * @tc.desc: Test store2 custom pull delete data with not handle.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync004, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        upsert.insert({"stringCol2", std::string("upsert")});
        return ConflictRet::UPSERT;
    });
    // step1 store1 push one row and store2 pull
    Store1InsertStore2Pull();
    // step2 store2 has one row and exist gid
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        upsert.insert({"stringCol2", std::string("upsert")});
        return ConflictRet::NOT_HANDLE;
    });
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A, "uuidCol='uuid1'"), 1);
    EXPECT_EQ(CountTableData(info2_, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE_A), "cloud_gid != ''"), 1);
    // step3 store1 delete one row and store2 pull
    Store1DeleteStore2Pull();
    // step4 store2 check not exist uuidCol='uuid1'
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A, "uuidCol='uuid1'"), 1);
    EXPECT_EQ(CountTableData(info2_, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE_A),
        "cloud_gid = '' AND version = '' "), 1);
}

/**
 * @tc.name: SimpleSync005
 * @tc.desc: Test store1 custom push with not equal to.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync005, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        upsert.insert({"stringCol2", std::string("upsert")});
        upsert.insert({"stringCol1", std::string("upsert")});
        return ConflictRet::UPSERT;
    });
    // step1 store1 insert (id=2, intCol=2, stringCol1='text2', stringCol2='text3', uuidCol='uuid2')
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(2, 2, 'text2', 'text3', 'uuid2')", info1_);
    ASSERT_EQ(ret, E_OK);
    // step2 check insert success
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A,
        "intCol=2 AND stringCol1='text2' AND uuidCol='uuid2' AND stringCol2='text3'"), 1);
    // step3 store1 insert (id=1, intCol=1, stringCol1='text1', stringCol2='text2', uuidCol='uuid2')
    //       store1 push data with stringCol2 not equal to 'text3'
    Query pushQuery = Query::Select().From(CLOUD_SYNC_TABLE_A).NotEqualTo("stringCol2", "text2");
    Store1InsertStore2Pull(pushQuery);
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2 IS NULL"), 0);
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=2 AND stringCol1='upsert' AND uuidCol='uuid2' AND stringCol2='upsert'"), 1);
}

/**
 * @tc.name: SimpleSync006
 * @tc.desc: Test set both primary key and unique column in the schema and sync.
 * @tc.type: FUNC
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare table
     * @tc.expected: step1. Return OK.
    */
    std::string tableName = "CLOUD_SYNC_TABLE_B";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + "("
        "id INTEGER PRIMARY KEY NOT NULL, intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT, uuidCol TEXT UNIQUE)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, false},
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
        {{"uuidCol", TYPE_INDEX<std::string>, false, true, true}, false},
        {{"nonExist", TYPE_INDEX<std::string>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = tableName, .fieldInfo = filedInfo}
        }
    };
    PrepareEnv(schemaInfo);
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &) {
        return ConflictRet::UPSERT;
    });

    /**
     * @tc.steps:step2. Prepare data and sync
     * @tc.expected: step2. Return OK.
    */
    sql = "INSERT INTO " + tableName + " VALUES(1, 1, 'text1', 'text2', 'uuid1')";
    auto ret = ExecuteSQL(sql, info1_);
    ASSERT_EQ(ret, E_OK);
    sql = "INSERT INTO " + tableName + " VALUES(1, 1, 'text3', 'text4', 'uuid1')";
    ret = ExecuteSQL(sql, info2_);
    ASSERT_EQ(ret, E_OK);

    Query query = Query::Select().FromTable({tableName});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PUSH, OK, OK));
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    EXPECT_EQ(CountTableData(info1_, tableName,
        "id = 1 AND intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='text2'"), 0);
    EXPECT_EQ(CountTableData(info1_, tableName,
        "id = 1 AND intCol=1 AND stringCol1='text3' AND uuidCol='uuid1' AND stringCol2='text2'"), 1);
}

/**
 * @tc.name: SimpleSync007
 * @tc.desc: Test unique col with null data.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync007, TestSize.Level0)
{
    const std::string table = "SimpleSync007";
    InitTables(table);
    InitSchema(table);
    /**
     * @tc.steps:step1. Prepare data which unique col is null
     * @tc.expected: step1. Return OK.
     */
    auto sql = "INSERT INTO " + table + " VALUES(1, 1, 'text1', 'text2', null)";
    auto ret = ExecuteSQL(sql, info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. Create distributed table with null data
     * @tc.expected: step2. Create OK.
     */
    ASSERT_NO_FATAL_FAILURE(InitDistributedTable({table}));
    /**
     * @tc.steps:step3. Update data and set uuid
     * @tc.expected: step3. Update OK.
     */
    sql = "UPDATE " + table + " SET uuidCol='123' WHERE intCol=1";
    ret = ExecuteSQL(sql, info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step4. Store1 sync to store2
     * @tc.expected: step4. Sync OK.
     */
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        return ConflictRet::UPSERT;
    });
    Query pushQuery = Query::Select().From(table).EqualTo("intCol", 1);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, pushQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PUSH, OK, OK));
    Query pullQuery = Query::Select().FromTable({table});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, pullQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PULL, OK, OK));
    EXPECT_EQ(CountTableData(info2_, table,
        "id = 1 AND intCol=1 AND stringCol1='text1' AND uuidCol='123' AND stringCol2 IS NULL"), 1);
}

/**
 * @tc.name: SimpleSync008
 * @tc.desc: Test save sync data when local has null data.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync008, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &) {
        return ConflictRet::UPSERT;
    });
    /**
     * @tc.steps:step1. Prepare data which unique col is null
     * @tc.expected: step1. Return OK.
     */
    auto sql = "INSERT INTO " + std::string(CLOUD_SYNC_TABLE_A) + " VALUES(1, 1, 'text1', 'text2', null)";
    auto ret = ExecuteSQL(sql, info2_);
    ASSERT_EQ(ret, E_OK);
    /**
    * @tc.steps:step2. Store1 push one row and store2 pull
    * @tc.expected: step2. Sync OK.
    */
    Store1InsertStore2Pull();
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2 IS NULL"), 1);
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND stringCol2='text2' AND uuidCol IS NULL"), 1);
}

/**
 * @tc.name: SimpleSync009
 * @tc.desc: Test save sync data with integrate.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, SimpleSync009, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &update) {
        update["stringCol1"] = std::string("INTEGRATE");
        return ConflictRet::INTEGRATE;
    });
    SetCloudConflictHandler(info1_, [](const std::string &, const VBucket &, const VBucket &, VBucket &) {
        return ConflictRet::UPSERT;
    });
    /**
     * @tc.steps:step1. Prepare data which unique col is uuid1
     * @tc.expected: step1. Return OK.
     */
    auto sql = "INSERT INTO " + std::string(CLOUD_SYNC_TABLE_A) + " VALUES(1, 1, 'text1', 'text2', 'uuid1')";
    auto ret = ExecuteSQL(sql, info2_);
    ASSERT_EQ(ret, E_OK);
    /**
    * @tc.steps:step2. Store1 push one row and store2 pull
    * @tc.expected: step2. Sync OK.
    */
    Store1InsertStore2Pull();
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='INTEGRATE' AND uuidCol='uuid1'"), 1);
    /**
    * @tc.steps:step3. Store2 push and store1 pull
    * @tc.expected: step3. Sync OK.
    */
    Query pushQuery = Query::Select().From({CLOUD_SYNC_TABLE_A}).EqualTo("stringCol1", "INTEGRATE");
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, pushQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PUSH, OK, OK));
    Query pullQuery = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, pullQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PULL, OK, OK));
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='INTEGRATE' AND uuidCol='uuid1'"), 1);
}

void DistributedDBRDBConflictHandlerTest::AbortCloudSyncTest(const std::function<void(void)> &forkFunc,
    const std::function<void(void)> &cancelForkFunc, SyncMode mode)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 2, 'text1', 'text3', 'uuid1')", info2_);
    ASSERT_EQ(ret, E_OK);
    ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(2, 1, 'text1', 'text3', 'uuid2')", info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. Stop task when Query cloud data.
     * @tc.expected: step2. Return OK.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SYNC_MODE_CLOUD_MERGE, OK, OK));
    forkFunc();
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, mode, OK, TASK_INTERRUPTED));
    /**
     * @tc.steps:step3. Sync again.
     * @tc.expected: step3. Return OK.
     */
    cancelForkFunc();
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, mode, OK, OK));
}

/**
  * @tc.name: AbortCloudSyncTest001
  * @tc.desc: Test abort cloud sync task.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBRDBConflictHandlerTest, AbortCloudSyncTest001, TestSize.Level0)
{
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    auto delegate = GetDelegate(info1_);
    std::function<void(void)> forkFunc = [&virtualCloudDb, &delegate]() {
        virtualCloudDb->ForkQuery([&delegate](const std::string &, VBucket &) {
            std::thread subThread([&delegate]() {
                EXPECT_EQ(delegate->StopTask(TaskType::BACKGROUND_TASK), OK);
            });
            subThread.detach();
        });
    };
    std::function<void(void)> cancelForkFunc = [&virtualCloudDb]() {
        virtualCloudDb->ForkQuery(nullptr);
    };
    AbortCloudSyncTest(forkFunc, cancelForkFunc, SYNC_MODE_CLOUD_MERGE);
}

/**
  * @tc.name: AbortCloudSyncTest002
  * @tc.desc: Test abort cloud sync task.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBRDBConflictHandlerTest, AbortCloudSyncTest002, TestSize.Level0)
{
    auto delegate = GetDelegate(info1_);
    auto handler = std::make_shared<TestCloudConflictHandler>();
    handler->SetCallback([](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        upsert.insert({"stringCol2", std::string("upsert")});
        return ConflictRet::UPSERT;
    });
    EXPECT_EQ(delegate->SetCloudConflictHandler(handler), OK);

    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    std::function<void(void)> forkFunc = [&virtualCloudDb, &delegate]() {
        virtualCloudDb->ForkQuery([&delegate](const std::string &, VBucket &) {
            std::thread subThread([&delegate]() {
                EXPECT_EQ(delegate->StopTask(TaskType::BACKGROUND_TASK), OK);
            });
            subThread.detach();
        });
    };
    std::function<void(void)> cancelForkFunc = [&virtualCloudDb]() {
        virtualCloudDb->ForkQuery(nullptr);
    };
    AbortCloudSyncTest(forkFunc, cancelForkFunc, SYNC_MODE_CLOUD_CUSTOM_PULL);
}

/**
  * @tc.name: AbortCloudSyncTest003
  * @tc.desc: Test abort cloud sync task.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBRDBConflictHandlerTest, AbortCloudSyncTest003, TestSize.Level0)
{
    auto delegate = GetDelegate(info1_);
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    bool isStopTask = false;
    std::function<void(void)> forkFunc = [&virtualCloudDb, &isStopTask, &delegate]() {
        virtualCloudDb->ForkUpload([&isStopTask, &delegate](const std::string &, VBucket &) {
            if (isStopTask) {
                return;
            }
            std::thread subThread([&]() {
                EXPECT_EQ(delegate->StopTask(TaskType::BACKGROUND_TASK), OK);
            });
            subThread.detach();
            isStopTask = true;
        });
    };
    std::function<void(void)> cancelForkFunc = [&virtualCloudDb]() {
        virtualCloudDb->ForkUpload(nullptr);
    };
    AbortCloudSyncTest(forkFunc, cancelForkFunc, SYNC_MODE_CLOUD_MERGE);
}

/**
  * @tc.name: AbortCloudSyncTest004
  * @tc.desc: Test abort cloud sync task.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBRDBConflictHandlerTest, AbortCloudSyncTest004, TestSize.Level0)
{
    auto delegate = GetDelegate(info1_);
    auto handler = std::make_shared<TestCloudConflictHandler>();
    handler->SetCallback([](const std::string &, const VBucket &, const VBucket &, VBucket &upsert) {
        upsert.insert({"stringCol2", std::string("upsert")});
        return ConflictRet::UPSERT;
    });
    EXPECT_EQ(delegate->SetCloudConflictHandler(handler), OK);

    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    bool isStopTask = false;
    std::function<void(void)> forkFunc = [&virtualCloudDb, &isStopTask, &delegate]() {
        virtualCloudDb->ForkUpload([&isStopTask, &delegate](const std::string &, VBucket &) {
            if (isStopTask) {
                return;
            }
            std::thread subThread([&]() {
                EXPECT_EQ(delegate->StopTask(TaskType::BACKGROUND_TASK), OK);
            });
            subThread.detach();
            isStopTask = true;
        });
    };
    std::function<void(void)> cancelForkFunc = [&virtualCloudDb]() {
        virtualCloudDb->ForkUpload(nullptr);
    };
    AbortCloudSyncTest(forkFunc, cancelForkFunc, SYNC_MODE_CLOUD_CUSTOM_PUSH);
}

/**
  * @tc.name: AbortCloudSyncTest005
  * @tc.desc: Test abort cloud sync task.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBRDBConflictHandlerTest, AbortCloudSyncTest005, TestSize.Level0)
{
    auto delegate = GetDelegate(info1_);
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    std::function<void(void)> forkFunc = [&virtualCloudDb, &delegate]() {
        virtualCloudDb->ForkBeforeBatchUpdate([&delegate](const std::string &, std::vector<VBucket> &,
            std::vector<VBucket> &, bool) {
            std::thread subThread([&]() {
                EXPECT_EQ(delegate->StopTask(TaskType::BACKGROUND_TASK), OK);
            });
            subThread.detach();
        });
    };
    std::function<void(void)> cancelForkFunc = [&virtualCloudDb]() {
        virtualCloudDb->ForkUpload(nullptr);
    };
    AbortCloudSyncTest(forkFunc, cancelForkFunc, SYNC_MODE_CLOUD_MERGE);
}

/**
 * @tc.name: LogTrigger001
 * @tc.desc: Test trigger update log after col change to null.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, LogTrigger001, TestSize.Level0)
{
    // step1 store1 insert (id=1, intCol=1, stringCol1='text1', stringCol2='text2', uuidCol='uuid1')
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    // step2 check insert success
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='text2'"), 1);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE_A),
        "data_key=1 AND cursor=1"), 1);

    ret = ExecuteSQL("UPDATE CLOUD_SYNC_TABLE_A SET stringCol1='text2' WHERE uuidCol='uuid1'", info1_);
    ASSERT_EQ(ret, E_OK);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE_A),
        "data_key=1 AND cursor=2"), 1);

    ret = ExecuteSQL("UPDATE CLOUD_SYNC_TABLE_A SET stringCol1=null WHERE uuidCol='uuid1'", info1_);
    ASSERT_EQ(ret, E_OK);
    EXPECT_EQ(CountTableData(info1_, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE_A),
        "data_key=1 AND cursor=3"), 1);
}

/**
 * @tc.name: LogTrigger002
 * @tc.desc: Test change pk with no dup check table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, LogTrigger002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare table
     * @tc.expected: step1. Return OK.
     */
    std::string tableName = "CLOUD_SYNC_TABLE_B";
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + "("
        "id INTEGER PRIMARY KEY NOT NULL, intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, false},
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = tableName, .fieldInfo = filedInfo}
        }
    };
    PrepareEnv(schemaInfo);
    /**
     * @tc.steps:step2. store2 insert data and sync to cloud
     * @tc.expected: step2. Sync OK.
     */
    sql = "INSERT INTO " + tableName + "(rowid, id, intCol, stringCol1, stringCol2) VALUES(1, 2, 1, 'text1', 'text2')";
    auto ret = ExecuteSQL(sql, info2_);
    ASSERT_EQ(ret, E_OK);
    sql = "UPDATE " + tableName + " SET id=1 WHERE intCol=1";
    ret = ExecuteSQL(sql, info2_);
    ASSERT_EQ(ret, E_OK);
    EXPECT_EQ(CountTableData(info2_, DBCommon::GetLogTableName(tableName)), 1);
}

/**
 * @tc.name: UpgradeTest001
 * @tc.desc: Test upgrade distributed table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBConflictHandlerTest, UpgradeTest001, TestSize.Level0)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
        {{"stringCol2", TYPE_INDEX<std::string>, false, true}, false},
        {{"uuidCol", TYPE_INDEX<std::string>, false, true, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = CLOUD_SYNC_TABLE_A, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info1_, schemaInfo);
    EXPECT_EQ(SetDistributedTables(info1_, {CLOUD_SYNC_TABLE_A}, TableSyncType::CLOUD_COOPERATION), E_OK);
}
}
#endif // USE_DISTRIBUTEDDB_CLOUD