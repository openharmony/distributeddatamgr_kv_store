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
#include "sqlite_relational_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBBasicRDBTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *CLOUD_SYNC_TABLE = "CLOUD_SYNC_TABLE";
};

void DistributedDBBasicRDBTest::SetUp()
{
    RDBGeneralUt::SetUp();
}

void DistributedDBBasicRDBTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

/**
 * @tc.name: InitDelegateExample001
 * @tc.desc: Test InitDelegate interface of RDBGeneralUt.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, InitDelegateExample001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Call InitDelegate interface with default data.
     * @tc.expected: step1. Ok
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    DataBaseSchema actualSchemaInfo = RDBGeneralUt::GetSchema(info1);
    ASSERT_EQ(actualSchemaInfo.tables.size(), 2u);
    EXPECT_EQ(actualSchemaInfo.tables[0].name, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);

    /**
     * @tc.steps: step2. Call twice InitDelegate interface with the set data.
     * @tc.expected: step2. Ok
     */
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, true}, {{"name", TYPE_INDEX<std::string>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {{.name = DEVICE_SYNC_TABLE, .fieldInfo = filedInfo}}
    };
    RDBGeneralUt::SetSchemaInfo(info1, schemaInfo);
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);

    StoreInfo info2 = {USER_ID, APP_ID, STORE_ID_2};
    schemaInfo = {
        .tablesInfo = {
            {.name = DEVICE_SYNC_TABLE, .fieldInfo = filedInfo},
            {.name = CLOUD_SYNC_TABLE, .fieldInfo = filedInfo},
        }
    };
    RDBGeneralUt::SetSchemaInfo(info2, schemaInfo);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2, "dev2"), E_OK);
    actualSchemaInfo = RDBGeneralUt::GetSchema(info2);
    ASSERT_EQ(actualSchemaInfo.tables.size(), schemaInfo.tablesInfo.size());
    EXPECT_EQ(actualSchemaInfo.tables[1].name, CLOUD_SYNC_TABLE);
    TableSchema actualTableInfo = RDBGeneralUt::GetTableSchema(info2, CLOUD_SYNC_TABLE);
    EXPECT_EQ(actualTableInfo.fields.size(), filedInfo.size());
}

#ifdef USE_DISTRIBUTEDDB_DEVICE
/**
 * @tc.name: RdbSyncExample001
 * @tc.desc: Test insert data and sync from dev1 to dev2.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbSyncExample001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 insert data.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, "dev2"), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), 0);

    /**
     * @tc.steps: step2. create distributed tables and sync to dev1.
     * @tc.expected: step2. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BlockPush(info1, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), 2);

    /**
     * @tc.steps: step3. update name and sync to dev1.
     * @tc.expected: step3. Ok
     */
    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update'";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    ASSERT_NO_FATAL_FAILURE(BlockPush(info1, info2, g_defaultTable1));
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1, "name='update'"), 2);
}
#endif

#ifdef USE_DISTRIBUTEDDB_CLOUD
/**
 * @tc.name: RdbCloudSyncExample001
 * @tc.desc: Test cloud sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. sync dev1 data to cloud.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 2);
}

/**
 * @tc.name: RdbCloudSyncExample002
 * @tc.desc: Test cloud insert data and cloud sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. cloud insert data.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, 20, 0, RDBGeneralUt::GetSchema(info1), virtualCloudDb), OK);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 20);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 0);

    /**
     * @tc.steps: step2. cloud sync data to dev1.
     * @tc.expected: step2. Ok
     */
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 20);
}

/**
 * @tc.name: RdbCloudSyncExample003
 * @tc.desc: Test upload failed, when return FILE_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample003, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);

    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetLocalAssetNotFound(true);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, LOCAL_ASSET_NOT_FOUND);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 2);

    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update'";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    virtualCloudDb->SetLocalAssetNotFound(false);
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 0);
}

/**
 * @tc.name: RdbCloudSyncExample004
 * @tc.desc: Test upload when asset is abnormal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample004, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetLocalAssetNotFound(true);
    // sync failed and local asset abnormal
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, LOCAL_ASSET_NOT_FOUND);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 2);

    virtualCloudDb->ClearAllData();
    query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 0);
    // insert new local assert
    std::string sql = "DELETE FROM " + g_defaultTable1;
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    query = Query::Select().FromTable({g_defaultTable1});
    InsertLocalDBData(0, 4, info1);
    virtualCloudDb->SetLocalAssetNotFound(false);
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 2);
}

/**
 * @tc.name: RdbCloudSyncExample005
 * @tc.desc: one table is normal and another is abnormal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample005, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable2), 2);
    
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetLocalAssetNotFound(true);
    // sync failed and local asset abnormal
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1, g_defaultTable2}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, LOCAL_ASSET_NOT_FOUND);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 2);

    virtualCloudDb->ClearAllData();
    virtualCloudDb->SetLocalAssetNotFound(false);

    query = Query::Select().FromTable({g_defaultTable1, g_defaultTable2});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 0);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable2), 2);
}

/**
 * @tc.name: RdbCloudSyncExample008
 * @tc.desc: Test upload failed, when return SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample008, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    InsertLocalDBData(0, 1, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 1);

    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetUploadRecordStatus(DBStatus::SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT);

    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update'";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    virtualCloudDb->SetUploadRecordStatus(DBStatus::OK);
    RDBGeneralUt::CloudBlockSync(info1, query, OK, OK);
}
#endif // USE_DISTRIBUTEDDB_CLOUD

/**
 * @tc.name: RdbUtilsTest001
 * @tc.desc: Test rdb utils execute actions.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbUtilsTest001, TestSize.Level0)
{
    std::vector<std::function<int()>> actions;
    /**
     * @tc.steps: step1. execute null actions no effect ret.
     * @tc.expected: step1. E_OK
     */
    actions.emplace_back(nullptr);
    EXPECT_EQ(SQLiteRelationalUtils::ExecuteListAction(actions), E_OK);
    /**
     * @tc.steps: step2. execute abort when action return error.
     * @tc.expected: step2. -E_INVALID_ARGS
     */
    actions.clear();
    actions.emplace_back([]() {
        return -E_INVALID_ARGS;
    });
    actions.emplace_back([]() {
        return -E_NOT_SUPPORT;
    });
    EXPECT_EQ(SQLiteRelationalUtils::ExecuteListAction(actions), -E_INVALID_ARGS);
}

const std::string TEST_TABLE = "entity_test_table";

// 辅助函数：准备SQL语句
sqlite3_stmt* PrepareStatement(sqlite3* db, const std::string& sql)
{
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }
    return stmt;
}

// 测试实体JSON字符串定义
namespace TestEntities {
    // 实体类型1: 作业待办 - 数学作业
    const std::string MATH_HOMEWORK = R"({
        "subject": "数学",
        "assignment_name": "作业1",
        "assignment_date": 1633027200,
        "completion_date": 1633113600,
        "assignment_description": "完成练习",
        "teacher_name": "张老师",
        "issue_date": 1632940800
    })";

    // 实体类型1: 作业待办 - 数学作业不同日期 (只有一个必选字段不同)
    const std::string MATH_HOMEWORK_DIFFERENT_DATE = R"({
        "subject": "数学",
        "assignment_name": "作业1",
        "assignment_date": 1633027200,
        "completion_date": 1633123600,  // 这个字段不同
        "assignment_description": "完成练习",
        "teacher_name": "张老师",
        "issue_date": 1632940800
    })";
}

/**
 * @tc.name: RdbIsEntityDuplicate003
 * @tc.desc: 基础功能测试 - 部分字段相同应该返回不重复
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. 创建测试表
     * @tc.expected: step1. 创建成功
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);

    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + TEST_TABLE +
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);

    /**
     * @tc.steps: step2. 插入数学作业数据 (实体类型1)
     * @tc.expected: step2. 插入成功
     */
    std::string insertSql = "INSERT INTO " + TEST_TABLE + " (id, content_type, page_content) VALUES (1, 1, ?)";

    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
    ASSERT_NE(stmt, nullptr);

    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);

    /**
     * @tc.steps: step3. 使用is_entity_duplicate查询部分字段相同实体
     * @tc.expected: step3. 返回0 (false)，表示不重复
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";

    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);

    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK_DIFFERENT_DATE.c_str(),
                      TestEntities::MATH_HOMEWORK_DIFFERENT_DATE.length(), SQLITE_TRANSIENT);

    EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    int result = sqlite3_column_int(stmt, 0);
    EXPECT_EQ(result, 0);  // 应该返回false，因为有一个必选字段不同
    sqlite3_finalize(stmt);
}
}