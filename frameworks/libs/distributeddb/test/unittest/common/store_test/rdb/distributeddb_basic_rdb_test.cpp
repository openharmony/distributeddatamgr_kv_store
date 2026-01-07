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
 * @tc.desc: Test update data will change cursor.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. cloud insert data.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    auto ret = ExecuteSQL("INSERT INTO defaultTable1(id, name) VALUES(1, 'name1')", info1);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(g_defaultTable1), "cursor >= 1"), 1);
    ret = ExecuteSQL("UPDATE defaultTable1 SET name='name1' WHERE id=1", info1);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(g_defaultTable1), "cursor >= 2"), 1);
    ret = ExecuteSQL("UPDATE defaultTable1 SET name='name2' WHERE id=1", info1);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(g_defaultTable1), "cursor >= 3"), 1);
}

/**
 * @tc.name: RdbCloudSyncExample004
 * @tc.desc: Test upload failed, when return FILE_NOT_FOUND
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
 * @tc.name: RdbCloudSyncExample005
 * @tc.desc: Test upload when asset is abnormal
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
 * @tc.name: RdbCloudSyncExample006
 * @tc.desc: one table is normal and another is abnormal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample006, TestSize.Level0)
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
    
    // 实体类型1: 作业待办 - 语文作业 (所有必选字段都不同)
    const std::string CHINESE_HOMEWORK = R"({
        "subject": "语文",
        "assignment_name": "作文",
        "assignment_date": 1633123200,
        "completion_date": 1633209600,
        "assignment_description": "写一篇作文",
        "teacher_name": "李老师",
        "issue_date": 1633036800
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
    
    // 实体类型2: 活动任务待办
    const std::string ACTIVITY_ASSIGNMENT = R"({
        "activity_name": "篮球比赛",
        "activity_date": 1633027200,
        "location": "学校体育馆",
        "teacher_name": "王老师",
        "activity_description": "班级篮球赛事",
        "issue_date": 1632940800
    })";
    
    // 实体类型3: 取餐码
    const std::string MEAL_PICKUP = R"({
        "restaurant_name": "麦当劳",
        "meal_code": "MC001",
        "meal_description": "巨无霸套餐",
        "order_id": "ORDER123",
        "estimated_time": 1633027200,
        "reserved_time": 1633030800,
        "meal_link": "http://example.com"
    })";
    
    // 实体类型4: 医院预约
    const std::string HOSPITAL_APPOINTMENT = R"({
        "hospital_name": "人民医院",
        "department_name": "内科",
        "doctor_name": "张医生",
        "visit_date": 1633027200,
        "visit_time": "14:30",
        "visit_number": "A001",
        "patient_name": "张三"
    })";
    
    // 空JSON对象
    const std::string EMPTY_JSON = "{}";
    
    // 无效JSON
    const std::string INVALID_JSON = "{invalid json";
    
    // JSON缺少闭合括号
    const std::string UNCLOSED_JSON = R"({"subject": "数学")";
    
    // 缺少必选字段的JSON (缺少assignment_description)
    const std::string MISSING_REQUIRED_FIELD = R"({
        "subject": "数学",
        "assignment_name": "作业1",
        "assignment_date": 1633027200,
        "completion_date": 1633113600,
        "teacher_name": "张老师",
        "issue_date": 1632940800
    })";
    
    // 字段类型错误的JSON (assignment_date应该是int但是给了string)
    const std::string WRONG_TYPE_FIELD = R"({
        "subject": "数学",
        "assignment_name": "作业1",
        "assignment_date": "2023-10-01",
        "completion_date": 1633113600,
        "assignment_description": "完成练习",
        "teacher_name": "张老师",
        "issue_date": 1632940800
    })";
    
    // 包含中文字符的JSON
    const std::string CHINESE_CHARS = R"({
        "subject": "数学（高等代数）",
        "assignment_name": "作业1-第一章",
        "assignment_date": 1633027200,
        "completion_date": 1633113600,
        "assignment_description": "完成练习：第1-10题",
        "teacher_name": "张老师",
        "issue_date": 1632940800
    })";
    
    // 包含SQL注入尝试的JSON
    const std::string SQL_INJECTION = R"({
        "subject": "数学",
        "assignment_name": "作业1; DROP TABLE test; --",
        "assignment_date": 1633027200,
        "completion_date": 1633113600,
        "assignment_description": "完成练习",
        "teacher_name": "张老师",
        "issue_date": 1632940800
    })";
}

/**
 * @tc.name: RdbIsEntityDuplicate001
 * @tc.desc: 基础功能测试 - 完全相同的实体应该返回重复
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate001, TestSize.Level0)
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
    
    // 创建包含content_type和page_content的表
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + TEST_TABLE +
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    /**
     * @tc.steps: step2. 插入测试数据 (实体类型1)
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
     * @tc.steps: step3. 使用is_entity_duplicate查询重复实体
     * @tc.expected: step3. 返回1 (true)，表示重复
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    int result = sqlite3_column_int(stmt, 0);
    EXPECT_EQ(result, 1);  // 应该返回true
    sqlite3_finalize(stmt);
}

/**
 * @tc.name: RdbIsEntityDuplicate002
 * @tc.desc: 基础功能测试 - 完全不同的实体应该返回不重复
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate002, TestSize.Level0)
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
     * @tc.steps: step3. 使用is_entity_duplicate查询完全不同实体
     * @tc.expected: step3. 返回0 (false)，表示不重复
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::CHINESE_HOMEWORK.c_str(),
                      TestEntities::CHINESE_HOMEWORK.length(), SQLITE_TRANSIENT);
    
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    int result = sqlite3_column_int(stmt, 0);
    EXPECT_EQ(result, 0);  // 应该返回false
    sqlite3_finalize(stmt);
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

/**
 * @tc.name: RdbIsEntityDuplicate004
 * @tc.desc: 边界条件测试 - 空值输入处理
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate004, TestSize.Level0)
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
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    /**
     * @tc.steps: step2. 插入测试数据，page_content为NULL
     * @tc.expected: step2. 插入成功
     */
    std::string insertSql = "INSERT INTO " + TEST_TABLE + " (id, content_type, page_content) VALUES (1, 1, NULL)";
    EXPECT_EQ(ExecuteSQL(insertSql, info1), E_OK);
    
    /**
     * @tc.steps: step3. 测试第一个参数为NULL的情况
     * @tc.expected: step3. 返回NULL
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    sqlite3_stmt* stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_type(stmt, 0), SQLITE_NULL);  // 应该返回NULL
    sqlite3_finalize(stmt);
    
    /**
     * @tc.steps: step4. 插入page_content不为NULL的数据
     * @tc.expected: step4. 插入成功
     */
    insertSql = "INSERT INTO " + TEST_TABLE + " (id, content_type, page_content) VALUES (2, 1, ?)";
    stmt = PrepareStatement(db, insertSql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    /**
     * @tc.steps: step5. 测试第二个参数为NULL的情况
     * @tc.expected: step5. 返回NULL
     */
    querySql = "SELECT is_entity_duplicate(page_content, NULL) FROM " + TEST_TABLE + " WHERE id = 2";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_type(stmt, 0), SQLITE_NULL);
    sqlite3_finalize(stmt);
}

/**
 * @tc.name: RdbIsEntityDuplicate005
 * @tc.desc: Schema符合性测试 - 缺少必选字段
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate005, TestSize.Level0)
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
     * @tc.steps: step2. 插入完整的数学作业数据
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
     * @tc.steps: step3. 使用缺少必选字段的JSON进行查询
     * @tc.expected: step3. 抛出错误，指出缺少必选字段
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MISSING_REQUIRED_FIELD.c_str(),
                      TestEntities::MISSING_REQUIRED_FIELD.length(), SQLITE_TRANSIENT);
    
    // 这里期望函数抛出错误
    EXPECT_NE(sqlite3_step(stmt), SQLITE_ROW);  // 不应该返回行
    sqlite3_finalize(stmt);
}

/**
 * @tc.name: RdbIsEntityDuplicate006
 * @tc.desc: Schema符合性测试 - 字段类型不匹配
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate006, TestSize.Level0)
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
     * @tc.steps: step2. 插入正确的数学作业数据
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
     * @tc.steps: step3. 使用字段类型错误的JSON进行查询
     * @tc.expected: step3. 抛出类型不匹配错误
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::WRONG_TYPE_FIELD.c_str(),
                      TestEntities::WRONG_TYPE_FIELD.length(), SQLITE_TRANSIENT);
    
    // 这里期望函数抛出错误
    EXPECT_NE(sqlite3_step(stmt), SQLITE_ROW);  // 不应该返回行
    sqlite3_finalize(stmt);
}

/**
 * @tc.name: RdbIsEntityDuplicate007
 * @tc.desc: 跨实体类型比较测试
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate007, TestSize.Level0)
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
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    /**
     * @tc.steps: step2. 插入实体类型1的数据
     * @tc.expected: step2. 插入成功
     */
    std::string insertSql = "INSERT INTO " + TEST_TABLE + " (id, content_type, page_content) VALUES (1, 1, ?)";
    sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    /**
     * @tc.steps: step3. 插入实体类型2的数据
     * @tc.expected: step3. 插入成功
     */
    insertSql = "INSERT INTO " + TEST_TABLE + " (id, content_type, page_content) VALUES (2, 2, ?)";
    stmt = PrepareStatement(db, insertSql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::ACTIVITY_ASSIGNMENT.c_str(),
                      TestEntities::ACTIVITY_ASSIGNMENT.length(), SQLITE_TRANSIENT);
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    /**
     * @tc.steps: step4. 对不同实体类型进行查询
     * @tc.expected: step4. 返回false
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::ACTIVITY_ASSIGNMENT.c_str(),
                      TestEntities::ACTIVITY_ASSIGNMENT.length(), SQLITE_TRANSIENT);
    
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    int result = sqlite3_column_int(stmt, 0);
    EXPECT_EQ(result, 0);
    sqlite3_finalize(stmt);
}

/**
 * @tc.name: RdbIsEntityDuplicate008
 * @tc.desc: JSON格式验证 - 无效JSON格式
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate008, TestSize.Level0)
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
     * @tc.steps: step2. 插入有效JSON数据
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
     * @tc.steps: step3. 使用无效JSON进行查询
     * @tc.expected: step3. 抛出"无效JSON格式"错误
     */
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::INVALID_JSON.c_str(),
                      TestEntities::INVALID_JSON.length(), SQLITE_TRANSIENT);
    
    // 这里期望函数抛出错误
    EXPECT_NE(sqlite3_step(stmt), SQLITE_ROW);  // 不应该返回行
    sqlite3_finalize(stmt);
}

/**
 * @tc.name: RdbIsEntityDuplicate009
 * @tc.desc: SQL查询集成 - WHERE子句使用
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate009, TestSize.Level0)
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
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT, created_time INTEGER)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    /**
     * @tc.steps: step2. 插入多个测试数据
     * @tc.expected: step2. 插入成功
     */
    std::vector<std::pair<int, std::string>> testData = {
        {1, TestEntities::MATH_HOMEWORK},
        {2, TestEntities::CHINESE_HOMEWORK},        // 2 means second
        {3, TestEntities::MATH_HOMEWORK},  // 重复数据, 3 means the third
        {4, TestEntities::ACTIVITY_ASSIGNMENT}  // 4 means the fourth
    };
    
    for (size_t i = 0; i < testData.size(); i++) {
        std::string insertSql = "INSERT INTO " + TEST_TABLE +
            " (id, content_type, page_content, created_time) VALUES (?, ?, ?, ?)";
        sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
        ASSERT_NE(stmt, nullptr);
        
        sqlite3_bind_int(stmt, 1, i + 1);
        sqlite3_bind_int(stmt, 2, 1);  // content_type为1, // 2 means second
        // 3 means the third
        sqlite3_bind_text(stmt, 3, testData[i].second.c_str(),
                          testData[i].second.length(), SQLITE_TRANSIENT);
        // // 4 means the fourth, 1633027200 is a data
        sqlite3_bind_int64(stmt, 4, 1633027200 + i);
        EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    /**
     * @tc.steps: step3. 使用WHERE子句查询重复实体
     * @tc.expected: step3. 返回id为1和3的记录
     */
    std::string querySql = "SELECT id FROM " + TEST_TABLE +
        " WHERE is_entity_duplicate(page_content, ?) = 1 ORDER BY id";
    sqlite3_stmt* stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    
    std::vector<int> resultIds;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        resultIds.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    
    EXPECT_EQ(resultIds.size(), 2);
    if (resultIds.size() >= 2) {
        EXPECT_EQ(resultIds[0], 1);
        EXPECT_EQ(resultIds[1], 3);
    }
}

/**
 * @tc.name: RdbIsEntityDuplicate010
 * @tc.desc: SQL查询集成 - 复杂查询组合
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate010, TestSize.Level0)
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
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT, created_time INTEGER)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    /**
     * @tc.steps: step2. 插入多个测试数据
     * @tc.expected: step2. 插入成功
     */
    std::vector<std::tuple<int, int, std::string, int64_t>> testData = {
        {1, 1, TestEntities::MATH_HOMEWORK, 1633027200},  // 较早时间
        {2, 1, TestEntities::MATH_HOMEWORK, 1633027300},  // 较晚时间，重复
        {3, 1, TestEntities::CHINESE_HOMEWORK, 1633027400},  // 不同实体
        {4, 2, TestEntities::ACTIVITY_ASSIGNMENT, 1633027500}   // 不同实体类型
    };
    
    for (const auto& data : testData) {
        std::string insertSql = "INSERT INTO " + TEST_TABLE +
            " (id, content_type, page_content, created_time) VALUES (?, ?, ?, ?)";
        sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
        ASSERT_NE(stmt, nullptr);
        
        sqlite3_bind_int(stmt, 1, std::get<0>(data));
        sqlite3_bind_int(stmt, 2, std::get<1>(data));
        sqlite3_bind_text(stmt, 3, std::get<2>(data).c_str(),
                          std::get<2>(data).length(), SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 4, std::get<3>(data));
        EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    /**
     * @tc.steps: step3. 使用复杂查询组合
     * @tc.expected: step3. 返回id为2的记录（重复且created_time > 1633027200）
     */
    std::string querySql = "SELECT id FROM " + TEST_TABLE +
        " WHERE is_entity_duplicate(page_content, ?) = 1 AND created_time > ? ORDER BY id";
    sqlite3_stmt* stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, 1633027200);
    
    std::vector<int> resultIds;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        resultIds.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    
    EXPECT_EQ(resultIds.size(), 1);
    if (resultIds.size() >= 1) {
        EXPECT_EQ(resultIds[0], 2);
    }
}

/**
 * @tc.name: RdbIsEntityDuplicate011
 * @tc.desc: 所有实体类型遍历测试
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate011, TestSize.Level0)
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
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    /**
     * @tc.steps: step2. 插入所有实体类型的测试数据
     * @tc.expected: step2. 插入成功
     */
    std::vector<std::pair<int, std::string>> testData = {
        {1, TestEntities::MATH_HOMEWORK},        // 实体类型1
        {2, TestEntities::ACTIVITY_ASSIGNMENT},        // 实体类型2
        {3, TestEntities::MEAL_PICKUP},          // 实体类型3
        {4, TestEntities::HOSPITAL_APPOINTMENT}  // 实体类型4
    };
    
    for (size_t i = 0; i < testData.size(); i++) {
        std::string insertSql = "INSERT INTO " + TEST_TABLE +
            " (id, content_type, page_content) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
        ASSERT_NE(stmt, nullptr);
        
        sqlite3_bind_int(stmt, 1, i + 1);
        sqlite3_bind_int(stmt, 2, i + 1);  // content_type从1到4
        sqlite3_bind_text(stmt, 3, testData[i].second.c_str(),
                          testData[i].second.length(), SQLITE_TRANSIENT);
        EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    /**
     * @tc.steps: step3. 对每个实体类型进行重复检测
     * @tc.expected: step3. 所有实体类型都能正常工作
     */
    for (size_t i = 0; i < testData.size(); i++) {
        std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " +
            TEST_TABLE + " WHERE id = ?";
        sqlite3_stmt* stmt = PrepareStatement(db, querySql);
        ASSERT_NE(stmt, nullptr);
        
        sqlite3_bind_text(stmt, 1, testData[i].second.c_str(),
                          testData[i].second.length(), SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, i + 1);
        
        EXPECT_EQ(sqlite3_step(stmt), SQLITE_ROW);
        int result = sqlite3_column_int(stmt, 0);
        EXPECT_EQ(result, 1);  // 应该返回true（相同实体）
        sqlite3_finalize(stmt);
    }
}

/**
 * @tc.name: RdbIsEntityDuplicate012
 * @tc.desc: 错误处理测试 - 错误信息明确性
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate012, TestSize.Level0)
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
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    /**
     * @tc.steps: step2. 插入有效数据
     * @tc.expected: step2. 插入成功
     */
    std::string insertSql = "INSERT INTO " + TEST_TABLE + " (id, content_type, page_content) VALUES (1, 1, ?)";
    sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MATH_HOMEWORK.c_str(),
                      TestEntities::MATH_HOMEWORK.length(), SQLITE_TRANSIENT);
    EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    /**
     * @tc.steps: step3. 测试各种错误条件
     * @tc.expected: step3. 每个错误都有明确、易于理解的错误信息
     */
    
    // 测试1: 缺少必选字段
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::MISSING_REQUIRED_FIELD.c_str(),
                      TestEntities::MISSING_REQUIRED_FIELD.length(), SQLITE_TRANSIENT);
    
    int stepResult = sqlite3_step(stmt);
    EXPECT_NE(stepResult, SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    // 测试2: 无效JSON格式
    querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::INVALID_JSON.c_str(),
                      TestEntities::INVALID_JSON.length(), SQLITE_TRANSIENT);
    
    stepResult = sqlite3_step(stmt);
    EXPECT_NE(stepResult, SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    // 测试3: 字段类型不匹配
    querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " + TEST_TABLE + " WHERE id = 1";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::WRONG_TYPE_FIELD.c_str(),
                      TestEntities::WRONG_TYPE_FIELD.length(), SQLITE_TRANSIENT);
    
    stepResult = sqlite3_step(stmt);
    EXPECT_NE(stepResult, SQLITE_ROW);
    sqlite3_finalize(stmt);
}

/**
 * @tc.name: RdbIsEntityDuplicate013
 * @tc.desc: 错误传播测试 - 复杂查询中的错误传播
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicate013, TestSize.Level0)
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
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT, value INTEGER)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    /**
     * @tc.steps: step2. 插入多个测试数据
     * @tc.expected: step2. 插入成功
     */
    std::vector<std::tuple<int, std::string, int>> testData = {
        {1, TestEntities::MATH_HOMEWORK, 100},
        {2, TestEntities::CHINESE_HOMEWORK, 200},
        {3, TestEntities::MATH_HOMEWORK, 300}
    };
    
    for (const auto& data : testData) {
        std::string insertSql = "INSERT INTO " + TEST_TABLE +
            " (id, content_type, page_content, value) VALUES (?, ?, ?, ?)";
        sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
        ASSERT_NE(stmt, nullptr);
        
        sqlite3_bind_int(stmt, 1, std::get<0>(data));
        sqlite3_bind_int(stmt, 2, 1);   // 2 means second
        sqlite3_bind_text(stmt, 3, std::get<1>(data).c_str(),
                          std::get<1>(data).length(), SQLITE_TRANSIENT);    // 3 means third
        sqlite3_bind_int(stmt, 4, std::get<2>(data));   // 4 means fourth
        EXPECT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    /**
     * @tc.steps: step3. 在复杂查询中触发UDF错误
     * @tc.expected: step3. 整个查询失败
     */
    
    // 测试1: 在WHERE子句中使用无效JSON
    std::string querySql = "SELECT SUM(value) FROM " + TEST_TABLE +
        " WHERE is_entity_duplicate(page_content, ?) = 1";
    sqlite3_stmt* stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, TestEntities::INVALID_JSON.c_str(),
                      TestEntities::INVALID_JSON.length(), SQLITE_TRANSIENT);
    
    int stepResult = sqlite3_step(stmt);
    EXPECT_NE(stepResult, SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    /**
     * @tc.steps: step4. 在子查询中触发UDF错误
     * @tc.expected: step4. 整个查询失败，错误正确传播
     */
    querySql = "SELECT * FROM " + TEST_TABLE +
        " WHERE id IN (SELECT id FROM " + TEST_TABLE +
        " WHERE is_entity_duplicate(page_content, ?) = 1)";
    stmt = PrepareStatement(db, querySql);
    ASSERT_NE(stmt, nullptr);

    sqlite3_bind_text(stmt, 1, TestEntities::WRONG_TYPE_FIELD.c_str(),
                      TestEntities::WRONG_TYPE_FIELD.length(), SQLITE_TRANSIENT);
    
    stepResult = sqlite3_step(stmt);
    EXPECT_NE(stepResult, SQLITE_ROW);
    sqlite3_finalize(stmt);
}

// 辅助函数：插入实体数据
void InsertEntityData(sqlite3* db, int id, int contentType, const std::string& entityJson)
{
    std::string insertSql = "INSERT INTO " + TEST_TABLE +
        " (id, content_type, page_content) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt = PrepareStatement(db, insertSql);
    if (!stmt) {
        return;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, contentType); // 2 means second
    sqlite3_bind_text(stmt, 3, entityJson.c_str(), entityJson.length(), SQLITE_TRANSIENT);  // 3 means the third param
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// 辅助函数：查询是否重复
int QueryIsDuplicate(sqlite3* db, int id, const std::string& entityJson)
{
    std::string querySql = "SELECT is_entity_duplicate(page_content, ?) FROM " +
        TEST_TABLE + " WHERE id = ?";
    sqlite3_stmt* stmt = PrepareStatement(db, querySql);
    if (!stmt) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, entityJson.c_str(), entityJson.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);  // 2 means second
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return result;
}

// 辅助函数：查询重复实体ID
std::vector<int> QueryDuplicateEntities(sqlite3* db, const std::string& entityJson, int contentType)
{
    std::vector<int> resultIds;
    std::string querySql = "SELECT id FROM " + TEST_TABLE +
        " WHERE content_type = ? AND is_entity_duplicate(page_content, ?) = 1 ORDER BY id";
    
    sqlite3_stmt* stmt = PrepareStatement(db, querySql);
    if (!stmt) return resultIds;
    
    sqlite3_bind_int(stmt, 1, contentType);
    sqlite3_bind_text(stmt, 2, entityJson.c_str(), entityJson.length(), SQLITE_TRANSIENT);  // 2 means second
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        resultIds.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return resultIds;
}

// 辅助函数：查询不重复实体ID
std::vector<int> QueryNonDuplicateEntities(sqlite3* db, const std::string& entityJson, int contentType)
{
    std::vector<int> resultIds;
    std::string querySql = "SELECT id FROM " + TEST_TABLE +
        " WHERE content_type = ? AND is_entity_duplicate(page_content, ?) = 0 ORDER BY id";
    
    sqlite3_stmt* stmt = PrepareStatement(db, querySql);
    if (!stmt) return resultIds;
    
    sqlite3_bind_int(stmt, 1, contentType);
    sqlite3_bind_text(stmt, 2, entityJson.c_str(), entityJson.length(), SQLITE_TRANSIENT);  // 2 means second
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        resultIds.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return resultIds;
}

// 辅助函数：检查是否包含
bool Contains(const std::vector<int>& vec, int value)
{
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

/**
 * @tc.name: RdbIsEntityDuplicateBarrier001
 * @tc.desc: 表中有多条对应重复的实体数据，查询与入参entity重复或不重复的全部数据
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicateBarrier001, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + TEST_TABLE +
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    // 插入4条数据：2条重复，2条不重复
    InsertEntityData(db, 1, 1, TestEntities::MATH_HOMEWORK);
    InsertEntityData(db, 2, 1, TestEntities::CHINESE_HOMEWORK);
    InsertEntityData(db, 3, 1, TestEntities::MATH_HOMEWORK);
    InsertEntityData(db, 4, 1, TestEntities::MATH_HOMEWORK_DIFFERENT_DATE);
    
    // 查询重复数据
    std::vector<int> duplicateIds = QueryDuplicateEntities(db, TestEntities::MATH_HOMEWORK, 1);
    EXPECT_EQ(duplicateIds.size(), 2);
    EXPECT_TRUE(Contains(duplicateIds, 1));
    EXPECT_TRUE(Contains(duplicateIds, 3));
    
    // 查询不重复数据
    std::vector<int> nonDuplicateIds = QueryNonDuplicateEntities(db, TestEntities::MATH_HOMEWORK, 1);
    EXPECT_EQ(nonDuplicateIds.size(), 2);
    EXPECT_TRUE(Contains(nonDuplicateIds, 2));
    EXPECT_TRUE(Contains(nonDuplicateIds, 4));
}

/**
 * @tc.name: RdbIsEntityDuplicateBarrier002
 * @tc.desc: 入参entity实体与db中数据实体必选字段和可选字段内容完全一致，识别为重复
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicateBarrier002, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + TEST_TABLE +
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    // 包含必选和可选字段的完整实体
    std::string fullEntity = R"({
        "subject": "数学",
        "assignment_name": "作业1",
        "assignment_date": 1633027200,
        "completion_date": 1633113600,
        "assignment_description": "完成练习",
        "teacher_name": "张老师",
        "issue_date": 1632940800,
        "attachments": "练习册第10页",
        "difficulty": 3,
        "score": 95,
        "status": "待完成"
    })";
    
    InsertEntityData(db, 1, 1, fullEntity);
    
    // 使用相同的完整实体查询
    int result = QueryIsDuplicate(db, 1, fullEntity);
    EXPECT_EQ(result, 1);  // 应该识别为重复
}

/**
 * @tc.name: RdbIsEntityDuplicateBarrier003
 * @tc.desc: 入参entity实体与db中数据实体字段内容完全一致，但字段顺序与schema不一致，识别为重复
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicateBarrier003, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + TEST_TABLE +
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    // Schema顺序的实体
    std::string schemaOrderEntity = TestEntities::MATH_HOMEWORK;
    
    // 字段顺序不同的相同实体
    std::string differentOrderEntity = R"({
        "teacher_name": "张老师",
        "issue_date": 1632940800,
        "subject": "数学",
        "assignment_description": "完成练习",
        "completion_date": 1633113600,
        "assignment_date": 1633027200,
        "assignment_name": "作业1"
    })";
    
    InsertEntityData(db, 1, 1, schemaOrderEntity);
    
    // 使用字段顺序不同的实体查询
    int result = QueryIsDuplicate(db, 1, differentOrderEntity);
    EXPECT_EQ(result, 1);  // 应该识别为重复
}

/**
 * @tc.name: RdbIsEntityDuplicateBarrier004
 * @tc.desc: 入参entity实体比schema少了个可选字段，识别为重复
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbIsEntityDuplicateBarrier004, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + TEST_TABLE +
        " (id INTEGER PRIMARY KEY, content_type INTEGER, page_content TEXT)";
    EXPECT_EQ(ExecuteSQL(createTableSql, info1), E_OK);
    
    sqlite3* db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    
    // 包含所有可选字段的实体
    std::string fullOptionalEntity = R"({
        "subject": "数学",
        "assignment_name": "作业1",
        "assignment_date": 1633027200,
        "completion_date": 1633113600,
        "assignment_description": "完成练习",
        "teacher_name": "张老师",
        "issue_date": 1632940800,
        "attachments": "练习册第10页",
        "difficulty": 3,
        "score": 95,
        "status": "待完成"
    })";
    
    // 缺少一个可选字段的实体
    std::string missingOptionalEntity = R"({
        "subject": "数学",
        "assignment_name": "作业1",
        "assignment_date": 1633027200,
        "completion_date": 1633113600,
        "assignment_description": "完成练习",
        "teacher_name": "张老师",
        "issue_date": 1632940800,
        "attachments": "练习册第10页",
        "difficulty": 3,
        "score": 95
        // 缺少status字段
    })";
    
    InsertEntityData(db, 1, 1, fullOptionalEntity);
    
    // 使用缺少可选字段的实体查询
    int result = QueryIsDuplicate(db, 1, missingOptionalEntity);
    EXPECT_EQ(result, 1);  // 应该识别为重复
}

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
}