/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>
#include <iostream>
#include "cloud/cloud_storage_utils.h"
#include "cloud/cloud_db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "process_system_api_adapter_impl.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "sqlite_relational_store.h"
#include "sqlite_relational_utils.h"
#include "store_observer.h"
#include "time_helper.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"
#include "mock_asset_loader.h"
#include "cloud_db_sync_utils_test.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_storeID = "Relational_Store_SYNC";
    const string g_tableName = "worker";
    const string DB_SUFFIX = ".db";
    const string CLOUD = "cloud";
    string g_testDir;
    string g_storePath;
    std::shared_ptr<VirtualCloudDb> g_virtualCloudDb;
    std::shared_ptr<VirtualAssetLoader> g_virtualAssetLoader;
    RelationalStoreObserverUnitTest *g_observer = nullptr;
    RelationalStoreDelegate *g_delegate = nullptr;
    const std::vector<std::string> g_tables = {g_tableName};
    const std::string CREATE_LOCAL_TABLE_COMPOUND_PRIMARY_KEY_SQL =
        "CREATE TABLE IF NOT EXISTS " + g_tableName + "(" \
        "name TEXT," \
        "height REAL ," \
        "married BOOLEAN ," \
        "photo BLOB NOT NULL," \
        "asset BLOB," \
        "age INT," \
        "PRIMARY KEY (" \
        "  name," \
        "  age)" \
        ");";
    const std::vector<Field> g_cloudFiledCompoundPrimaryKey = {
        {"name", TYPE_INDEX<std::string>, true}, {"height", TYPE_INDEX<double>},
        {"married", TYPE_INDEX<bool>}, {"photo", TYPE_INDEX<Bytes>, false, false},
        {"asset", TYPE_INDEX<Asset>}, {"age", TYPE_INDEX<int64_t>, true}
    };

    void InitExpectChangedData(ChangedDataType dataType, int64_t count, ChangeType changeType)
    {
        ChangedData changedDataForTable;
        changedDataForTable.tableName = g_tableName;
        changedDataForTable.type = dataType;
        changedDataForTable.field.push_back(std::string("rowid"));
        changedDataForTable.field.push_back(std::string("name"));
        changedDataForTable.field.push_back(std::string("age"));
        for (int64_t i = 0; i < count; ++i) {
            changedDataForTable.primaryData[changeType].push_back({i + 1,
                "Cloud" + to_string(i), 13L}); // 13 is expect age
        }
        g_observer->SetExpectedResult(changedDataForTable);
    }

    class DistributedDBCloudTableCompoundPrimaryKeySyncTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    protected:
        sqlite3 *db = nullptr;
    };

    void DistributedDBCloudTableCompoundPrimaryKeySyncTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        g_storePath = g_testDir + "/" + g_storeID + DB_SUFFIX;
        LOGI("The test db is:%s", g_testDir.c_str());
        RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
    }

    void DistributedDBCloudTableCompoundPrimaryKeySyncTest::TearDownTestCase(void)
    {}

    void DistributedDBCloudTableCompoundPrimaryKeySyncTest::SetUp(void)
    {
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
            LOGE("rm test db files error.");
        }
        DistributedDBToolsUnitTest::PrintTestCaseInfo();
        LOGD("Test dir is %s", g_testDir.c_str());
        db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        CloudDBSyncUtilsTest::CreateUserDBAndTable(db, CREATE_LOCAL_TABLE_COMPOUND_PRIMARY_KEY_SQL);
        CloudDBSyncUtilsTest::SetStorePath(g_storePath);
        CloudDBSyncUtilsTest::InitSyncUtils(g_cloudFiledCompoundPrimaryKey, g_observer, g_virtualCloudDb,
            g_virtualAssetLoader, g_delegate);
    }

    void DistributedDBCloudTableCompoundPrimaryKeySyncTest::TearDown(void)
    {
        CloudDBSyncUtilsTest::CloseDb(g_observer, g_virtualCloudDb, g_delegate);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
            LOGE("rm test db files error.");
        }
    }

/*
 * @tc.name: CloudSyncTest001
 * @tc.desc: test data sync when cloud insert
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudTableCompoundPrimaryKeySyncTest, CloudSyncTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud data and merge
     * @tc.expected: step1. check the changeddata and return ok
     */
    int64_t cloudCount = 10; // 10 is random cloud count
    int64_t paddingSize = 10; // 10 is padding size
    InitExpectChangedData(ChangedDataType::DATA, cloudCount, ChangeType::OP_INSERT);
    CloudDBSyncUtilsTest::InsertCloudTableRecord(0, cloudCount, paddingSize, true, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);
}

/*
 * @tc.name: CloudSyncTest002
 * @tc.desc: test data sync when cloud update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudTableCompoundPrimaryKeySyncTest, CloudSyncTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud data and merge
     * @tc.expected: step1. check the changeddata and return ok
     */
    int64_t cloudCount = 10; // 10 is random cloud count
    int64_t paddingSize = 10; // 10 is padding size
    CloudDBSyncUtilsTest::InsertCloudTableRecord(0, cloudCount, paddingSize, true, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);

    /**
     * @tc.steps:step2. update cloud data and merge
     * @tc.expected: step2. check the changeddata and return ok
     */
    InitExpectChangedData(ChangedDataType::DATA, cloudCount, ChangeType::OP_UPDATE);
    CloudDBSyncUtilsTest::UpdateCloudTableRecord(0, cloudCount, paddingSize, true, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);
}

/*
 * @tc.name: CloudSyncTest003
 * @tc.desc: test data sync when cloud delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudTableCompoundPrimaryKeySyncTest, CloudSyncTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud data and merge
     * @tc.expected: step1. check the changeddata and return ok
     */
    int64_t cloudCount = 10; // 10 is random cloud count
    int64_t paddingSize = 10; // 10 is padding size
    CloudDBSyncUtilsTest::InsertCloudTableRecord(0, cloudCount, paddingSize, true, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);

    /**
     * @tc.steps:step2. delete cloud data and merge
     * @tc.expected: step2. check the changeddata and return ok
     */
    InitExpectChangedData(ChangedDataType::DATA, cloudCount, ChangeType::OP_DELETE);
    CloudDBSyncUtilsTest::DeleteCloudTableRecordByGid(0, cloudCount, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({0L}, g_virtualCloudDb);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    CloudDBSyncUtilsTest::CheckLocalRecordNum(db, g_tableName, 0);
}

/*
 * @tc.name: CloudSyncTest004
 * @tc.desc: test asset when cloud insert
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudTableCompoundPrimaryKeySyncTest, CloudSyncTest004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud asset and merge
     * @tc.expected: step1. check the changeddata and return ok
     */
    int64_t cloudCount = 10; // 10 is random cloud count
    int64_t paddingSize = 10; // 10 is padding size
    InitExpectChangedData(ChangedDataType::ASSET, cloudCount, ChangeType::OP_INSERT);
    CloudDBSyncUtilsTest::InsertCloudTableRecord(0, cloudCount, paddingSize, false, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);
}

/*
 * @tc.name: CloudSyncTest005
 * @tc.desc: test asset sync when cloud insert
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudTableCompoundPrimaryKeySyncTest, CloudSyncTest005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud asset and merge
     * @tc.expected: step1. check the changeddata and return ok
     */
    int64_t cloudCount = 10; // 10 is random cloud count
    int64_t paddingSize = 10; // 10 is padding size
    CloudDBSyncUtilsTest::InsertCloudTableRecord(0, cloudCount, paddingSize, false, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);

    /**
     * @tc.steps:step2. update cloud asset and merge
     * @tc.expected: step2. check the changeddata and return ok
     */
    InitExpectChangedData(ChangedDataType::ASSET, cloudCount, ChangeType::OP_UPDATE);
    CloudDBSyncUtilsTest::UpdateCloudTableRecord(0, cloudCount, paddingSize, false, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);
}

/*
 * @tc.name: CloudSyncTest006
 * @tc.desc: test asset sync when cloud delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudTableCompoundPrimaryKeySyncTest, CloudSyncTest006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud asset and merge
     * @tc.expected: step1. check the changeddata and return ok
     */
    int64_t cloudCount = 10; // 10 is random cloud count
    int64_t paddingSize = 10; // 10 is padding size
    CloudDBSyncUtilsTest::InsertCloudTableRecord(0, cloudCount, paddingSize, false, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CloudDBSyncUtilsTest::CheckDownloadResult(db, {cloudCount}, CLOUD);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({cloudCount}, g_virtualCloudDb);

    /**
     * @tc.steps:step2. insert cloud asset and merge
     * @tc.expected: step2. check the changeddata and return ok
     */
    InitExpectChangedData(ChangedDataType::ASSET, cloudCount, ChangeType::OP_DELETE);
    CloudDBSyncUtilsTest::DeleteCloudTableRecordByGid(0, cloudCount, g_virtualCloudDb);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CloudDBSyncUtilsTest::CheckCloudTotalCount({0L}, g_virtualCloudDb);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    CloudDBSyncUtilsTest::CheckLocalRecordNum(db, g_tableName, 0);
}

}
#endif // RELATIONAL_STORE