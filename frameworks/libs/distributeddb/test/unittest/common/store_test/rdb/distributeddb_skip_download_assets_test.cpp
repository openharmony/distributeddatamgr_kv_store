/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "cloud/asset_operation_utils.h"

#ifdef USE_DISTRIBUTEDDB_CLOUD
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBSkipDownloadAssetsTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    static constexpr const char *SYNC_TABLE_A = "SYNC_TABLE_A";
    void InitTables(const std::string &table = SYNC_TABLE_A);
    void InitSchema(const StoreInfo &info, const std::string &table = SYNC_TABLE_A);
    void InitDistributedTable(const StoreInfo &info1, const StoreInfo &info2,
        const std::vector<std::string> &tables = {SYNC_TABLE_A});
    void PrepareToDownloadEnv();
    static CloudSyncOption GetSyncOption();
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBSkipDownloadAssetsTest::SetUp()
{
    RDBGeneralUt::SetUp();
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1_, "dev1"), E_OK);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2_, "dev2"), E_OK);
    EXPECT_NO_FATAL_FAILURE(InitTables());
    InitSchema(info1_);
    InitSchema(info2_);
    InitDistributedTable(info1_, info2_);
}

void DistributedDBSkipDownloadAssetsTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBSkipDownloadAssetsTest::InitTables(const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "intCol INTEGER, stringCol TEXT, assets BLOB)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
}

void DistributedDBSkipDownloadAssetsTest::InitSchema(const StoreInfo &info, const std::string &table)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, true}, false},
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol", TYPE_INDEX<std::string>, false, true}, false},
        {{"assets", TYPE_INDEX<Assets>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBSkipDownloadAssetsTest::InitDistributedTable(const StoreInfo &info1, const StoreInfo &info2,
    const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1);
    RDBGeneralUt::SetCloudDbConfig(info2);
    auto ret1 = SetDistributedTables(info1, tables, TableSyncType::CLOUD_COOPERATION);
    auto ret2 = SetDistributedTables(info2, tables, TableSyncType::CLOUD_COOPERATION);
    EXPECT_EQ(ret1, E_OK);
    EXPECT_EQ(ret2, E_OK);
}

/**
 * @tc.name: IsAssetNotDownloadTest001
 * @tc.desc: Test IsAssetNotDownload with different asset statuses.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, IsAssetNotDownloadTest001, TestSize.Level0)
{
    // Test ABNORMAL status - should return true
    uint32_t status1 = static_cast<uint32_t>(AssetStatus::ABNORMAL);
    EXPECT_TRUE(AssetOperationUtils::IsAssetNotDownload(status1));

    // Test DOWNLOADING status - should return true
    uint32_t status2 = static_cast<uint32_t>(AssetStatus::DOWNLOADING);
    EXPECT_TRUE(AssetOperationUtils::IsAssetNotDownload(status2));

    // Test TO_DOWNLOAD status - should return true
    uint32_t status3 = static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD);
    EXPECT_TRUE(AssetOperationUtils::IsAssetNotDownload(status3));

    // Test NORMAL status - should return false
    uint32_t status4 = static_cast<uint32_t>(AssetStatus::NORMAL);
    EXPECT_FALSE(AssetOperationUtils::IsAssetNotDownload(status4));

    // Test INSERT status - should return false
    uint32_t status5 = static_cast<uint32_t>(AssetStatus::INSERT);
    EXPECT_FALSE(AssetOperationUtils::IsAssetNotDownload(status5));

    // Test DELETE status - should return false
    uint32_t status6 = static_cast<uint32_t>(AssetStatus::DELETE);
    EXPECT_FALSE(AssetOperationUtils::IsAssetNotDownload(status6));
}

/**
 * @tc.name: IsAssetsNeedDownloadTest001
 * @tc.desc: Test IsAssetsNeedDownload with single asset.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, IsAssetsNeedDownloadTest001, TestSize.Level0)
{
    // Test empty assets - should return false
    Assets emptyAssets;
    EXPECT_FALSE(AssetOperationUtils::IsAssetsNeedDownload(emptyAssets));

    // Test ABNORMAL asset - should return true
    Asset asset1;
    asset1.status = static_cast<uint32_t>(AssetStatus::ABNORMAL);
    Assets assets1 = {asset1};
    EXPECT_TRUE(AssetOperationUtils::IsAssetsNeedDownload(assets1));

    // Test DOWNLOADING asset - should return true
    Asset asset2;
    asset2.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING);
    Assets assets2 = {asset2};
    EXPECT_TRUE(AssetOperationUtils::IsAssetsNeedDownload(assets2));

    // Test TO_DOWNLOAD asset - should return true
    Asset asset3;
    asset3.status = static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD);
    Assets assets3 = {asset3};
    EXPECT_TRUE(AssetOperationUtils::IsAssetsNeedDownload(assets3));

    // Test NORMAL asset - should return false
    Asset asset4;
    asset4.status = static_cast<uint32_t>(AssetStatus::NORMAL);
    Assets assets4 = {asset4};
    EXPECT_FALSE(AssetOperationUtils::IsAssetsNeedDownload(assets4));
}

/**
 * @tc.name: IsAssetsNeedDownloadTest002
 * @tc.desc: Test IsAssetsNeedDownload with multiple assets.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, IsAssetsNeedDownloadTest002, TestSize.Level0)
{
    // Test mixed status assets (no need download) - should return false
    Asset asset1;
    asset1.status = static_cast<uint32_t>(AssetStatus::NORMAL);
    Assets assets1 = {asset1};
    EXPECT_FALSE(AssetOperationUtils::IsAssetsNeedDownload(assets1));

    // Test mixed status assets (with need download) - should return true
    Asset asset3;
    asset3.status = static_cast<uint32_t>(AssetStatus::NORMAL);
    Asset asset4;
    asset4.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING);
    Assets assets2 = {asset3, asset4};
    EXPECT_TRUE(AssetOperationUtils::IsAssetsNeedDownload(assets2));
}

/**
 * @tc.name: IsAssetNeedDownloadTest001
 * @tc.desc: Test IsAssetNeedDownload with different asset statuses.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, IsAssetNeedDownloadTest001, TestSize.Level0)
{
    // Test ABNORMAL status - should return true
    Asset asset1;
    asset1.status = static_cast<uint32_t>(AssetStatus::ABNORMAL);
    EXPECT_TRUE(AssetOperationUtils::IsAssetNeedDownload(asset1));

    // Test DOWNLOADING status - should return true
    Asset asset2;
    asset2.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING);
    EXPECT_TRUE(AssetOperationUtils::IsAssetNeedDownload(asset2));

    // Test TO_DOWNLOAD status - should return true
    Asset asset3;
    asset3.status = static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD);
    EXPECT_TRUE(AssetOperationUtils::IsAssetNeedDownload(asset3));

    // Test NORMAL status - should return false
    Asset asset4;
    asset4.status = static_cast<uint32_t>(AssetStatus::NORMAL);
    EXPECT_FALSE(AssetOperationUtils::IsAssetNeedDownload(asset4));

    // Test INSERT status - should return false
    Asset asset5;
    asset5.status = static_cast<uint32_t>(AssetStatus::INSERT);
    EXPECT_FALSE(AssetOperationUtils::IsAssetNeedDownload(asset5));

    // Test DELETE status - should return false
    Asset asset6;
    asset6.status = static_cast<uint32_t>(AssetStatus::DELETE);
    EXPECT_FALSE(AssetOperationUtils::IsAssetNeedDownload(asset6));
}

/**
 * @tc.name: SetToDownloadTest001
 * @tc.desc: Test SetToDownload function with DOWNLOADING status.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, SetToDownloadTest001, TestSize.Level0)
{
    // Create record with DOWNLOADING asset
    Asset asset;
    asset.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING);

    VBucket record;
    record["asset_field"] = asset;

    // Call SetToDownload with skipDownloadAssets = true
    AssetOperationUtils::SetToDownload(true, record);

    // Verify asset status changed to TO_DOWNLOAD
    auto &changedAsset = std::get<Asset>(record["asset_field"]);
    EXPECT_EQ(changedAsset.status, static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD));
}

/**
 * @tc.name: SetToDownloadTest002
 * @tc.desc: Test SetToDownload function with skipDownloadAssets = false.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, SetToDownloadTest002, TestSize.Level0)
{
    // Create record with DOWNLOADING asset
    Asset asset;
    asset.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING);

    VBucket record;
    record["asset_field"] = asset;

    // Call SetToDownload with skipDownloadAssets = false
    AssetOperationUtils::SetToDownload(false, record);

    // Verify asset status not changed
    auto &unchangedAsset = std::get<Asset>(record["asset_field"]);
    EXPECT_EQ(unchangedAsset.status, static_cast<uint32_t>(AssetStatus::DOWNLOADING));
}

/**
 * @tc.name: SetToDownloadTest003
 * @tc.desc: Test SetToDownload function with Assets type.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, SetToDownloadTest003, TestSize.Level0)
{
    // Create record with Assets containing DOWNLOADING asset
    Asset asset1;
    asset1.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING);

    Asset asset2;
    asset2.status = static_cast<uint32_t>(AssetStatus::NORMAL);

    Assets assets = {asset1, asset2};

    VBucket record;
    record["assets_field"] = assets;

    // Call SetToDownload with skipDownloadAssets = true
    AssetOperationUtils::SetToDownload(true, record);

    // Verify assets status changed
    auto &changedAssets = std::get<Assets>(record["assets_field"]);
    EXPECT_EQ(changedAssets[0].status, static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD));
    EXPECT_EQ(changedAssets[1].status, static_cast<uint32_t>(AssetStatus::NORMAL));
}

/**
 * @tc.name: SetToDownloadTest004
 * @tc.desc: Test SetToDownload function with non-asset fields.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, SetToDownloadTest004, TestSize.Level0)
{
    // Create record with mixed field types
    Asset asset;
    asset.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING);

    VBucket record;
    record["int_field"] = int64_t(42);
    record["string_field"] = std::string("test");
    record["asset_field"] = asset;

    // Call SetToDownload with skipDownloadAssets = true
    AssetOperationUtils::SetToDownload(true, record);

    // Verify only asset field changed
    EXPECT_EQ(std::get<int64_t>(record["int_field"]), int64_t(42));
    EXPECT_EQ(std::get<std::string>(record["string_field"]), std::string("test"));
    EXPECT_EQ(std::get<Asset>(record["asset_field"]).status, static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD));
}

void DistributedDBSkipDownloadAssetsTest::PrepareToDownloadEnv()
{
    // Insert data into store1
    EXPECT_EQ(InsertLocalDBData(0, 1, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info2_, {.skipDownloadAssets = true}));
    // Sync from store1 to cloud
    DistributedDB::CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.query = Query::Select().FromTable({SYNC_TABLE_A});
    option.priorityTask = true;
    option.waitTime = DBConstant::MAX_TIMEOUT;
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));
    // Sync from cloud to store2
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, option, OK, OK));
    // Verify data in store2
    int count = CountTableData(info2_, SYNC_TABLE_A);
    EXPECT_EQ(count, 1);
    std::string sql = std::string("SELECT assets FROM ").append(SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info2_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD));
    }));
}

CloudSyncOption DistributedDBSkipDownloadAssetsTest::GetSyncOption()
{
    DistributedDB::CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.query = Query::Select().FromTable({SYNC_TABLE_A});
    option.priorityTask = true;
    option.waitTime = DBConstant::MAX_TIMEOUT;
    return option;
}

/**
 * @tc.name: CloudSyncWithSkipDownloadAssetsTest001
 * @tc.desc: Test data with to_download will be downloaded by asset only.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, CloudSyncWithSkipDownloadAssetsTest001, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareToDownloadEnv());
    std::vector<int64_t> inValue = {0};
    std::map<std::string, std::set<std::string>> assetOnly;
    std::string sql = std::string("SELECT assets FROM ").append(SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(GetAssetsMap(info2_, sql, "assets", assetOnly));
    auto option = GetSyncOption();
    option.query = Query::Select().From(SYNC_TABLE_A).In("id", inValue).And().AssetsOnly(assetOnly);
    option.mode = SyncMode::SYNC_MODE_CLOUD_FORCE_PULL;
    option.priorityLevel = 2;
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, option, OK, OK));
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info2_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
}

/**
 * @tc.name: CloudSyncWithSkipDownloadAssetsTest002
 * @tc.desc: Test data with to_download will be downloaded by download_only.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, CloudSyncWithSkipDownloadAssetsTest002, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareToDownloadEnv());
    auto option = GetSyncOption();
    option.syncFlowType = SyncFlowType::DOWNLOAD_ONLY;
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, option, OK, OK));
    std::string sql = std::string("SELECT assets FROM ").append(SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info2_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
}

/**
 * @tc.name: CloudSyncWithSkipDownloadAssetsTest003
 * @tc.desc: Test data with to_download will not upload to cloud.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSkipDownloadAssetsTest, CloudSyncWithSkipDownloadAssetsTest003, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareToDownloadEnv());
    EXPECT_EQ(ExecuteSQL(std::string("UPDATE ").append(SYNC_TABLE_A).append(" SET stringCol='update'"), info2_),
        E_OK);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, Query::Select().FromTable({SYNC_TABLE_A}), OK, OK));
    std::string sql = std::string("SELECT assets FROM ").append(SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info2_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::TO_DOWNLOAD));
    }));
}
}
#endif // USE_DISTRIBUTEDDB_CLOUD