/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with License.
 * You may obtain a copy of License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cloud/asset_operation_utils.h"
#include "db_common.h"
#include "rdb_general_ut.h"

#ifdef USE_DISTRIBUTEDDB_CLOUD
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBRDBAssetConflictTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    static constexpr const char *CLOUD_SYNC_TABLE_A = "CLOUD_SYNC_TABLE_A";
    void InitTables(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitSchema(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitDistributedTable(const StoreInfo &info1, const StoreInfo &info2,
        const std::vector<std::string> &tables = {CLOUD_SYNC_TABLE_A});
    Asset CreateAsset(int64_t id, int64_t assetTime);
    void InsertCloudData(int64_t begin, int64_t count, const std::string &tableName,
        int64_t recordTime = 0, int64_t assetTime = 0);

    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBAssetConflictTest::SetUp()
{
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(true);
    RDBGeneralUt::SetUp();
    // create db first
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1_, "dev1"), E_OK);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2_, "dev2"), E_OK);
    EXPECT_NO_FATAL_FAILURE(InitTables());
    InitSchema(info1_);
    InitSchema(info2_);
    InitDistributedTable(info1_, info2_);
}

void DistributedDBRDBAssetConflictTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBAssetConflictTest::InitTables(const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT,"
        "assetsCol ASSETS)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
}

void DistributedDBRDBAssetConflictTest::InitSchema(const StoreInfo &info, const std::string &table)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, true}, false},
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
        {{"stringCol2", TYPE_INDEX<std::string>, false, true}, false},
        {{"assetsCol", TYPE_INDEX<Assets>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBRDBAssetConflictTest::InitDistributedTable(const StoreInfo &info1,
    const StoreInfo &info2, const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1);
    RDBGeneralUt::SetCloudDbConfig(info2);
    ASSERT_EQ(SetDistributedTables(info1, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
}

Asset DistributedDBRDBAssetConflictTest::CreateAsset(int64_t id, int64_t assetTime)
{
    Field field = {.colName = "assetsCol"};
    Asset asset = {
        .name = RDBDataGenerator::GenerateAsset(id, field).name,
        .assetId = std::to_string(id),
        .modifyTime = std::to_string(assetTime),
        .createTime = std::to_string(assetTime),
        .hash = std::to_string(id),
        .status = static_cast<uint32_t>(AssetStatus::INSERT),
    };
    return asset;
}

void DistributedDBRDBAssetConflictTest::InsertCloudData(int64_t begin, int64_t count, const std::string &tableName,
    int64_t recordTime, int64_t assetTime)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::vector<VBucket> records;
    std::vector<VBucket> extends;
    for (int64_t i = begin; i < begin + count; ++i) {
        int num = 10;
        VBucket record;
        record["id"] = i;
        record["intCol"] = i * num;
        record["stringCol1"] = "cloud_insert_" + std::to_string(i);
        record["stringCol2"] = "cloud_insert_str2_" + std::to_string(i);
        record["uuidCol"] = "uuid_" + std::to_string(i);
        record["assetsCol"] = Assets { CreateAsset(i, assetTime) };
        records.push_back(std::move(record));

        VBucket extend;
        extend[CloudDbConstant::GID_FIELD] = std::to_string(i);
        extend[CloudDbConstant::CREATE_FIELD] = recordTime;
        extend[CloudDbConstant::MODIFY_FIELD] = recordTime;
        extend[CloudDbConstant::DELETE_FIELD] = false;
        extends.push_back(std::move(extend));
    }
    EXPECT_EQ(cloudDB->BatchInsertWithGid(tableName, std::move(records), extends), DBStatus::OK);
    LOGI("[DistributedDBRDBAssetConflictTest] InsertCloudData success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

/**
 * @tc.name: AssetConflictPolicy001
 * @tc.desc: Test no trigger download with default asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy001, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_DEFAULT
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to 0 and make sure record conflict result is local win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, 0));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 0);
}

/**
 * @tc.name: AssetConflictPolicy002
 * @tc.desc: Test trigger download with default asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy002, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_DEFAULT
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to INT64_MAX and make sure record conflict result is cloud win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, INT64_MAX));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 1);
}

/**
 * @tc.name: AssetConflictPolicy003
 * @tc.desc: Test no trigger download with time first asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy003, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_TIME_FIRST
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to 0 and make sure record conflict result is local win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, 0, INT64_MAX));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 1);
}

/**
 * @tc.name: AssetConflictPolicy004
 * @tc.desc: Test no trigger download with time first asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy004, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_TIME_FIRST
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to 0 and make sure record conflict result is local win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, 0, 0));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 0);
}

/**
 * @tc.name: AssetConflictPolicy005
 * @tc.desc: Test no trigger download with time first asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy005, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_TIME_FIRST
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to INT64_MAX and make sure record conflict result is local win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, INT64_MAX, INT64_MAX));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 1);
}

/**
 * @tc.name: AssetConflictPolicy006
 * @tc.desc: Test no trigger download with time first asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy006, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_TIME_FIRST
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to INT64_MAX and make sure record conflict result is cloud win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, INT64_MAX, 0));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::INSERT));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 0);
}

/**
 * @tc.name: AssetConflictPolicy007
 * @tc.desc: Test trigger download with temp path asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy007, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_TEMP_PATH
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to INT64_MAX and make sure record conflict result is cloud win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, INT64_MAX));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 1);
}

/**
 * @tc.name: AssetConflictPolicy008
 * @tc.desc: Test trigger download with temp path asset conflict policy.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBAssetConflictTest, AssetConflictPolicy008, TestSize.Level0)
{
    CloudSyncConfig config = {
        .assetPolicy = AssetConflictPolicy::CONFLICT_POLICY_TEMP_PATH
    };
    ASSERT_NO_FATAL_FAILURE(SetCloudSyncConfig(info1_, config));
    // set cloud data modifyTime to 0 and make sure record conflict result is local win
    const int begin = 0;
    const int count = 1;
    ASSERT_EQ(InsertLocalDBData(begin, count, info1_), E_OK);
    ASSERT_NO_FATAL_FAILURE(InsertCloudData(begin, count, CLOUD_SYNC_TABLE_A, 0));
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    ASSERT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query));
    std::string sql = std::string("SELECT assetsCol FROM ").append(CLOUD_SYNC_TABLE_A);
    EXPECT_NO_FATAL_FAILURE(CheckAssets(info1_, sql, false, [](const Asset &asset) {
        EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::NORMAL));
    }));
    auto loader = GetVirtualAssetLoader();
    ASSERT_NE(loader, nullptr);
    EXPECT_EQ(loader->GetBatchDownloadCount(), 1);
    auto cloud = GetVirtualCloudDb();
    EXPECT_EQ(cloud->GetUpdateCount(), 0);
}
} // namespace
#endif // USE_DISTRIBUTEDDB_CLOUD