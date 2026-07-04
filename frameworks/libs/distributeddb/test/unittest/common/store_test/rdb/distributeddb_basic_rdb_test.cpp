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

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "data_donation_utils.h"
#include "rdb_general_ut.h"
#include "sqlite_relational_utils.h"
#include "relational_store_client.h"
#include "relational_store_client_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
const std::string g_deviceA = "dev1";
const std::string g_deviceB = "dev2";
const std::string g_deviceC = "dev3";

constexpr size_t MAX_SLOT_NUM = 100;
constexpr size_t MATRIX_FILE_SLOT_SIZE = sizeof(uint64_t);
constexpr size_t MATRIX_FILE_SIZE = MAX_SLOT_NUM * MATRIX_FILE_SLOT_SIZE;

class DistributedDBBasicRDBTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table, bool noPk = false);
    void PrepareRemoveDataStore(StoreInfo &info1, StoreInfo &info2, StoreInfo &info3, int count);
    std::string InitMatrixFile();
    std::string InitZeroSizeMatrixFile();
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

UtDateBaseSchemaInfo DistributedDBBasicRDBTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBBasicRDBTest::GetTableSchema(const std::string &table, bool noPk)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo field;
    field.field.colName = "id";
    field.field.type = TYPE_INDEX<int64_t>;
    if (!noPk) {
        field.field.primary = true;
    }
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

std::string DistributedDBBasicRDBTest::InitMatrixFile()
{
    std::string matrixFilePath = GetTestDir() + "/matrixFile";

    int fd = open(matrixFilePath.c_str(), O_RDWR | O_CREAT, 0660);
    if (fd == -1) {
        return "";
    }

    int ret = ftruncate(fd, MATRIX_FILE_SIZE);
    close(fd);
    if (ret != 0) {
        unlink(matrixFilePath.c_str());
        return "";
    }
    return matrixFilePath;
}

std::string DistributedDBBasicRDBTest::InitZeroSizeMatrixFile()
{
    std::string matrixFilePath = GetTestDir() + "/matrixFile";

    int fd = open(matrixFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0660);
    if (fd == -1) {
        return "";
    }

    close(fd);
    return matrixFilePath;
}

void DistributedDBBasicRDBTest::PrepareRemoveDataStore(StoreInfo &info1, StoreInfo &info2, StoreInfo &info3, int count)
{
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info3, g_deviceC), E_OK);
    /**
    * @tc.steps: step1. dev1 insert data
    * @tc.expected: step1. Ok
    */
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    InsertLocalDBData(count, 0, info3);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);

    /**
    * @tc.steps: step2. create distributed tables and sync to dev2
    * @tc.expected: step2. Ok
    */
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info3, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId("dev1");
    BlockPush(info1, info2, g_defaultTable1);
    BasicUnitTest::SetLocalDeviceId("dev3");
    BlockPush(info3, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count + count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
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
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);

    StoreInfo info2 = {USER_ID, APP_ID, STORE_ID_2};
    schemaInfo = {
        .tablesInfo = {
            {.name = DEVICE_SYNC_TABLE, .fieldInfo = filedInfo},
            {.name = CLOUD_SYNC_TABLE, .fieldInfo = filedInfo},
        }
    };
    RDBGeneralUt::SetSchemaInfo(info2, schemaInfo);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
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
#endif // USE_DISTRIBUTEDDB_DEVICE

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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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
 * @tc.name: RdbCloudSyncExample007
 * @tc.desc: sync when table have field "timestamp"
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample007, TestSize.Level0)
{
    // step1: init local table
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, false},
        {{"timestamp", TYPE_INDEX<int64_t>, false, true}, false},
    };
    std::string tableName = "test_table";
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = tableName, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info1, schemaInfo);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 30, info1);
    // step2: do sync
    ASSERT_EQ(SetDistributedTables(info1, {tableName}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({tableName});
    RDBGeneralUt::CloudBlockSync(info1, query);
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
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
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

/**
 * @tc.name: UpdateDataLog001
 * @tc.desc: Test error update data log
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicRDBTest, UpdateDataLog001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);
    UpdateOption updateOption;
    updateOption.tableName = "non_distributed_table";
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;
    EXPECT_EQ(UpdateDataLog(db, updateOption), DISTRIBUTED_SCHEMA_NOT_FOUND);
}

/**
 * @tc.name: SetTrackerMatrixInfoTest001
 * @tc.desc: Test set tracker matrix info on success
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, SetTrackerMatrixInfoTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. SetTrackerMatrixInfo with normal params.
     * @tc.expected: step2. Ok
     */
    MatrixFileInfo info = {.matrixFilePath = "/filePath", .matrixTables = {{"table1", 0u}}};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), OK);
}

/**
 * @tc.name: SetTrackerMatrixInfoTest002
 * @tc.desc: Test set tracker matrix info when params invalid
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, SetTrackerMatrixInfoTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. SetTrackerMatrixInfo with empty file path.
     * @tc.expected: step2. INVALID_ARGS
     */
    MatrixFileInfo info = {.matrixFilePath = "", .matrixTables = {{"table1", 0u}}};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), INVALID_ARGS);

    /**
     * @tc.steps: step3. SetTrackerMatrixInfo with empty db.
     * @tc.expected: step3. INVALID_ARGS
     */
    info = {.matrixFilePath = "/db", .matrixTables = {{"table1", 0u}}};
    EXPECT_EQ(SetTrackerMatrixInfo(nullptr, info), INVALID_ARGS);

    /**
     * @tc.steps: step4. SetTrackerMatrixInfo with empty tables.
     * @tc.expected: step4. INVALID_ARGS
     */
    info = {.matrixFilePath = "/db", .matrixTables = {}};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), INVALID_ARGS);
}

/**
 * @tc.name: SetTrackerMatrixInfoTest003
 * @tc.desc: Test set tracker matrix info when path invalid
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, SetTrackerMatrixInfoTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. SetTrackerMatrixInfo with relative path.
     * @tc.expected: step2. INVALID_ARGS
     */
    MatrixFileInfo info = {.matrixFilePath = "./db", .matrixTables = {{"table1", 0u}}};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), INVALID_ARGS);

    /**
     * @tc.steps: step3. SetTrackerMatrixInfo with relative path.
     * @tc.expected: step3. INVALID_ARGS
     */
    info = {.matrixFilePath = "~/db", .matrixTables = {{"table1", 0u}}};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), INVALID_ARGS);

    /**
     * @tc.steps: step4. SetTrackerMatrixInfo with relative path.
     * @tc.expected: step4. INVALID_ARGS
     */
    info = {.matrixFilePath = "/path/to/../db", .matrixTables = {{"table1", 0u}}};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), INVALID_ARGS);

    /**
     * @tc.steps: step5. SetTrackerMatrixInfo with relative path.
     * @tc.expected: step5. INVALID_ARGS
     */
    info = {.matrixFilePath = "../relative/path", .matrixTables = {{"table1", 0u}}};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), INVALID_ARGS);
}

/**
 * @tc.name: UpdateMatrixFileTest001
 * @tc.desc: Test update matrix file
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, UpdateMatrixFileTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. Init matrix file.
     * @tc.expected: step2. OK
     */
    std::string matrixFilePath = InitMatrixFile();
    ASSERT_FALSE(matrixFilePath.empty());

    /**
     * @tc.steps: step3. Set matrix info.
     * @tc.expected: step3. OK
     */
    std::map<std::string, uint64_t> matrixTables = {
        {"table1", 0u},
        {"table2", 1u}
    };
    MatrixFileInfo info = {.matrixFilePath = matrixFilePath, .matrixTables = matrixTables, .fullSyncOffset = 2};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), OK);

    /**
     * @tc.steps: step4. Update matrix file for table1.
     * @tc.expected: step4. OK
     */
    MatrixFileUpdateConfig config = {.isFullSync = false};
    EXPECT_EQ(UpdateMatrixFile(db, {"table1"}, config), OK);

    auto [errCode, filePtr] = DataDonationUtils::MmapMatrixFile(info.matrixFilePath);
    ASSERT_NE(filePtr, nullptr);
    EXPECT_EQ(filePtr->GetValueByIndex(0), 1u);
    EXPECT_EQ(filePtr->GetValueByIndex(1), 0u);
    EXPECT_EQ(filePtr->GetValueByIndex(2), 0u);

    /**
     * @tc.steps: step5. Clean up.
     * @tc.expected: step5. OK
     */
    filePtr = nullptr;
    unlink(matrixFilePath.c_str());
}

/**
 * @tc.name: UpdateMatrixFileTest002
 * @tc.desc: Test update matrix file and set full sync
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, UpdateMatrixFileTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. Init matrix file.
     * @tc.expected: step2. OK
     */
    std::string matrixFilePath = InitMatrixFile();
    ASSERT_FALSE(matrixFilePath.empty());

    /**
     * @tc.steps: step3. Set matrix info.
     * @tc.expected: step3. OK
     */
    std::map<std::string, uint64_t> matrixTables = {
        {"table1", 0u},
        {"table2", 1u}
    };
    MatrixFileInfo info = {.matrixFilePath = matrixFilePath, .matrixTables = matrixTables, .fullSyncOffset = 2};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), OK);

    /**
     * @tc.steps: step4. Update matrix file for both tables.
     * @tc.expected: step4. OK
     */
    MatrixFileUpdateConfig config = {.isFullSync = true};
    EXPECT_EQ(UpdateMatrixFile(db, {"table1", "table2"}, config), OK);

    auto [errCode, filePtr] = DataDonationUtils::MmapMatrixFile(info.matrixFilePath);
    ASSERT_NE(filePtr, nullptr);
    EXPECT_EQ(filePtr->GetValueByIndex(0), 1u);
    EXPECT_EQ(filePtr->GetValueByIndex(1), 1u);
    EXPECT_EQ(filePtr->GetValueByIndex(2), 1u);

    /**
     * @tc.steps: step5. Clean up.
     * @tc.expected: step5. OK
     */
    filePtr = nullptr;
    unlink(matrixFilePath.c_str());
}

/**
 * @tc.name: UpdateMatrixFileTest003
 * @tc.desc: Test update matrix file when params invalid
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, UpdateMatrixFileTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. Update matrix file.
     * @tc.expected: step2. NOT FOUND
     */
    EXPECT_EQ(UnsetTrackerMatrixInfo(db), OK);
    MatrixFileUpdateConfig config = {.isFullSync = false};
    EXPECT_EQ(UpdateMatrixFile(db, {"table1", "table2"}, config), NOT_FOUND);

    /**
     * @tc.steps: step3. Update matrix file when db is null.
     * @tc.expected: step3. INVALID ARGS
     */
    EXPECT_EQ(UpdateMatrixFile(nullptr, {"table1", "table2"}, config), INVALID_ARGS);
}

/**
 * @tc.name: UpdateMatrixFileTest004
 * @tc.desc: Test update matrix file when file size is 0
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, UpdateMatrixFileTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init database.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. Init zero size matrix file.
     * @tc.expected: step2. OK
     */
    std::string matrixFilePath = InitZeroSizeMatrixFile();
    ASSERT_FALSE(matrixFilePath.empty());

    /**
     * @tc.steps: step3. Set matrix info.
     * @tc.expected: step3. OK
     */
    std::map<std::string, uint64_t> matrixTables = {{"table1", 0u}};
    MatrixFileInfo info = {.matrixFilePath = matrixFilePath, .matrixTables = matrixTables, .fullSyncOffset = 1u};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), OK);

    /**
     * @tc.steps: step4. Update matrix file for table1.
     * @tc.expected: step4. INVALID_FILE
     */
    MatrixFileUpdateConfig config = {.isFullSync = true};
    EXPECT_EQ(UpdateMatrixFile(db, {"table1"}, config), INVALID_FILE);

    unlink(matrixFilePath.c_str());
}

/**
 * @tc.name: UpdateMatrixFileTest005
 * @tc.desc: Test update matrix file when path error
 * @tc.type: FUNC
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBBasicRDBTest, UpdateMatrixFileTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InitDatabase(info1), E_OK);
    auto db = GetSqliteHandle(info1);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step2. Init matrix file and then delete file.
     * @tc.expected: step2. OK
     */
    std::string matrixFilePath = InitMatrixFile();
    ASSERT_FALSE(matrixFilePath.empty());
    unlink(matrixFilePath.c_str());

    /**
     * @tc.steps: step3. Set matrix info.
     * @tc.expected: step3. OK
     */
    std::map<std::string, uint64_t> matrixTables = {
        {"table1", 0u},
        {"table2", 1u}
    };
    MatrixFileInfo info = {.matrixFilePath = matrixFilePath, .matrixTables = matrixTables, .fullSyncOffset = 2};
    EXPECT_EQ(SetTrackerMatrixInfo(db, info), OK);

    /**
     * @tc.steps: step4. Update matrix file for both tables.
     * @tc.expected: step4. INVALID_FILE
     */
    MatrixFileUpdateConfig config = {.isFullSync = true};
    EXPECT_EQ(UpdateMatrixFile(db, {"table1", "table2"}, config), INVALID_FILE);
}

/**
 * @tc.name: SetBinlogEnabled002
 * @tc.desc: Test disable binlog.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DistributedDBBasicRDBTest, SetBinlogEnabled002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Init delegate and disable binlog.
     * @tc.expected: step1. Return OK.
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(false), OK);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);
}

/**
 * @tc.name: SetBinlogEnabled003
 * @tc.desc: Test enable then disable binlog.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DistributedDBBasicRDBTest, SetBinlogEnabled003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Init delegate, enable then disable binlog.
     * @tc.expected: step1. Both return OK.
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(delegate->SetBinlogEnabled(false), OK);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);
}

/**
 * @tc.name: SetBinlogEnabled004
 * @tc.desc: Test repeatedly enable binlog.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DistributedDBBasicRDBTest, SetBinlogEnabled004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Init delegate, enable binlog twice.
     * @tc.expected: step1. Both return OK.
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);
}

/**
 * @tc.name: SetBinlogEnabled005
 * @tc.desc: Test enable binlog then insert data works normally.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DistributedDBBasicRDBTest, SetBinlogEnabled005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. cloud insert data.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
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
} // namespace