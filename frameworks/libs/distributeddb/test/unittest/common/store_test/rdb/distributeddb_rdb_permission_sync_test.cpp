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

#ifdef USE_DISTRIBUTEDDB_DEVICE
#include "rdb_general_ut.h"
#include "relational_store_delegate_impl.h"
#include "runtime_config.h"
#include "sqlite_relational_store.h"
#include "sqlite_relational_store_connection.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBRDBPermissionSyncTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    void PrepareData();
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table);

    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    const StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    const StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBPermissionSyncTest::SetUp()
{
    RDBGeneralUt::SetUp();
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1_, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2_, DEVICE_B), E_OK);
    ASSERT_EQ(SetDistributedTables(info1_, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_NO_FATAL_FAILURE(PrepareData());
    RuntimeConfig::SetDataFlowCheckCallback([storeInfo1 = info1_, storeInfo2 = info2_](
        const PermissionCheckParam &param, const Property &property) {
        if (param.storeId == storeInfo2.storeId) {
            EXPECT_EQ(property.size(), 1);
            return DataFlowCheckRet::DEFAULT;
        }
        EXPECT_EQ(property.size(), 0);
        return DataFlowCheckRet::DENIED_SEND;
    });
}

void DistributedDBRDBPermissionSyncTest::TearDown()
{
    RuntimeConfig::SetDataFlowCheckCallback(nullptr);
    RDBGeneralUt::TearDown();
}

UtDateBaseSchemaInfo DistributedDBRDBPermissionSyncTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBRDBPermissionSyncTest::GetTableSchema(const std::string &table)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo field;
    field.field.colName = "id";
    field.field.type = TYPE_INDEX<int64_t>;
    field.field.primary = true;
    tableSchema.fieldInfo.push_back(field);
    field.field.colName = "val";
    field.field.type = TYPE_INDEX<std::string>;
    field.field.primary = false;
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

void DistributedDBRDBPermissionSyncTest::PrepareData()
{
    EXPECT_EQ(InsertLocalDBData(0, 1, info1_), E_OK);
    EXPECT_EQ(CountTableData(info1_, DEVICE_SYNC_TABLE, " id = 0 "), 1);
    EXPECT_EQ(InsertLocalDBData(1, 1, info2_), E_OK);
    EXPECT_EQ(CountTableData(info2_, DEVICE_SYNC_TABLE, " id = 1 "), 1);
    auto store = GetDelegate(info1_);
    ASSERT_NE(store, nullptr);
    Property property;
    EXPECT_EQ(store->SetProperty(property), OK);
    store = GetDelegate(info2_);
    ASSERT_NE(store, nullptr);
    property["field"] = std::string("field");
    EXPECT_EQ(store->SetProperty(property), OK);
}

/**
 * @tc.name: RDBPermissionSync001
 * @tc.desc: Test push OK by permission callback denied.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, RDBPermissionSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 sync to dev2
     * @tc.expected: step1. sync OK
     */
    BlockPush(info1_, info2_, DEVICE_SYNC_TABLE, OK);
    EXPECT_EQ(CountTableData(info2_, DEVICE_SYNC_TABLE, " id = 0 "), 0);
}

/**
 * @tc.name: RDBPermissionSync002
 * @tc.desc: Test pull ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, RDBPermissionSync002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 sync to dev2
     * @tc.expected: step1. sync ok
     */
    BlockPull(info1_, info2_, DEVICE_SYNC_TABLE, OK);
    EXPECT_EQ(CountTableData(info1_, DEVICE_SYNC_TABLE, " id = 1 "), 1);
}

/**
 * @tc.name: RDBPermissionSync003
 * @tc.desc: Test push ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, RDBPermissionSync003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev2 sync to dev1
     * @tc.expected: step1. sync ok
     */
    BlockPush(info2_, info1_, DEVICE_SYNC_TABLE, OK);
    EXPECT_EQ(CountTableData(info1_, DEVICE_SYNC_TABLE, " id = 1 "), 1);
}

/**
 * @tc.name: RDBPermissionSync004
 * @tc.desc: Test pull OK by permission callback denied.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, RDBPermissionSync004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 sync to dev2
     * @tc.expected: step1. sync OK
     */
    BlockPull(info2_, info1_, DEVICE_SYNC_TABLE, OK);
    EXPECT_EQ(CountTableData(info2_, DEVICE_SYNC_TABLE, " id = 0 "), 0);
}

/**
 * @tc.name: RDBPermissionQuery001
 * @tc.desc: Test remote query ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, RDBPermissionQuery001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 remote query to dev2
     * @tc.expected: step1. sync ok
     */
    std::shared_ptr<ResultSet> resultSet = nullptr;
    ASSERT_NO_FATAL_FAILURE(RemoteQuery(info1_, info2_, "SELECT * FROM DEVICE_SYNC_TABLE WHERE id = 1", OK, resultSet));
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GetCount(), 1);
    resultSet->Close();
}

/**
 * @tc.name: RDBPermissionQuery002
 * @tc.desc: Test remote query ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, RDBPermissionQuery002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 remote query to dev2
     * @tc.expected: step1. sync ok
     */
    std::shared_ptr<ResultSet> resultSet = nullptr;
    ASSERT_NO_FATAL_FAILURE(RemoteQuery(info2_, info1_, "SELECT * FROM DEVICE_SYNC_TABLE WHERE id = 0",
        OK, resultSet));
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GetCount(), 0);
    resultSet->Close();
}

/**
 * @tc.name: MockAPITest001
 * @tc.desc: Test rdbStoreNbDelegate api.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, MockAPITest001, TestSize.Level0)
{
    class MockRelationalStoreDelegate : public RelationalStoreDelegateImpl {
    public:
        MockRelationalStoreDelegate() : RelationalStoreDelegateImpl(nullptr, "store")
        {
        }

        DBStatus SetProperty(const Property &property) override
        {
            return RelationalStoreDelegate::SetProperty(property);
        }
    };
    MockRelationalStoreDelegate store;
    EXPECT_EQ(store.SetProperty({}), OK);
}

/**
 * @tc.name: InvalidArgs001
 * @tc.desc: Test invalid use api.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBPermissionSyncTest, InvalidArgs001, TestSize.Level0)
{
    RelationalStoreDelegateImpl store(nullptr, "store");
    EXPECT_EQ(store.SetProperty({}), DB_ERROR);
    SQLiteRelationalStore relationalStore;
    EXPECT_EQ(relationalStore.SetProperty({}), -E_INVALID_DB);
    SQLiteRelationalStoreConnection connection(nullptr);
    EXPECT_EQ(connection.SetProperty({}), -E_INVALID_CONNECTION);
}
}
#endif