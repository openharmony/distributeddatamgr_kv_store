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
class DistributedDBRDBComplexCloudTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    static constexpr const char *CLOUD_SYNC_TABLE_A = "CLOUD_SYNC_TABLE_A";
    void InitTables(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitSchema1(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitSchema2(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitDistributedTable(const std::vector<std::string> &tables = {CLOUD_SYNC_TABLE_A});
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBComplexCloudTest::SetUp()
{
    RDBGeneralUt::SetUp();
    // create db first
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1_, "dev1"), E_OK);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2_, "dev2"), E_OK);
    EXPECT_NO_FATAL_FAILURE(InitTables());
    InitSchema1(info1_);
    InitSchema2(info2_);
    InitDistributedTable();
}

void DistributedDBRDBComplexCloudTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBComplexCloudTest::InitTables(const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT, uuidCol TEXT UNIQUE)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
}

void DistributedDBRDBComplexCloudTest::InitSchema1(const StoreInfo &info, const std::string &table)
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
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBRDBComplexCloudTest::InitSchema2(const StoreInfo &info, const std::string &table)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
        {{"stringCol2", TYPE_INDEX<std::string>, false, true}, false},
        {{"uuidCol", TYPE_INDEX<std::string>, true, true, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBRDBComplexCloudTest::InitDistributedTable(const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1_);
    RDBGeneralUt::SetCloudDbConfig(info2_);
    ASSERT_EQ(SetDistributedTables(info1_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
}

/**
 * @tc.name: SimpleSync001
 * @tc.desc: Test update date when sync schema diff.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, UpdateSync001, TestSize.Level0)
{
    SetCloudConflictHandler(info2_, [](const std::string &, const VBucket &, const VBucket &, VBucket &) {
        return ConflictRet::UPSERT;
    });
    /**
     * @tc.steps:step1. store1 and store2 insert same data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text3', 'uuid1')", info2_);
    ASSERT_EQ(ret, E_OK);
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='text2'"), 1);
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2='text3'"), 1);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success and stringCol2 is null because store1 don't sync stringCol2.
     */
    Query pushQuery = Query::Select().From(CLOUD_SYNC_TABLE_A).EqualTo("stringCol1", "text1");
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, pushQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PUSH, OK, OK));
    Query pullQuery = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, pullQuery, SyncMode::SYNC_MODE_CLOUD_CUSTOM_PULL, OK, OK));
    EXPECT_EQ(CountTableData(info2_, CLOUD_SYNC_TABLE_A,
        "intCol=1 AND stringCol1='text1' AND uuidCol='uuid1' AND stringCol2 is null"), 1);
}
}
#endif