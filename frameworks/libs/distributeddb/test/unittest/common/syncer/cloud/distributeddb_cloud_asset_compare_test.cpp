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
#include "cloud_syncer.h"

#include <gtest/gtest.h>

#include "cloud/asset_operation_utils.h"
#include "cloud_syncer_test.h"
#include "cloud_store_types.h"
#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"
#include "distributeddb_data_generate_unit_test.h"
#include "relational_sync_able_storage.h"
#include "relational_store_instance.h"
#include "sqlite_relational_store.h"
#include "log_table_manager_factory.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr auto FIELD_ID = "id";
    constexpr auto FIELD_NAME = "name";
    constexpr auto FIELD_HOUSE = "house";
    constexpr auto FIELD_CARS = "cars";
    const string STORE_ID = "Relational_Store_ID";
    const string TABLE_NAME = "cloudData";
    string g_storePath;
    string g_dbDir;
    string TEST_DIR;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;
    IRelationalStore *g_store = nullptr;
    std::shared_ptr<TestStorageProxy> g_storageProxy = nullptr;
    std::shared_ptr<TestCloudSyncer> g_cloudSyncer = nullptr;
    Asset a1;
    Asset a1Changed;
    Asset a2;
    Asset a2Changed;
    Asset a3;
    Asset a3Changed;
    Asset a4;
    Asset a4Changed;
    Asset a5;
    Asset a5Changed;
    const std::vector<Field> ASSET_FIELDS = {
        {FIELD_HOUSE, TYPE_INDEX<Asset>, false}, {FIELD_CARS, TYPE_INDEX<Assets>, false}
    };
    VBucket DATA_BASELINE;
    VBucket DATA_EMPTY_ASSET;
    VBucket DATA_ASSET_SAME_NAME_BUT_CHANGE;
    VBucket DATA_ASSETS_SAME_NAME_PARTIALLY_CHANGED;
    VBucket DATA_ALL_SAME;
    VBucket DATA_ASSETS_MORE_FIELD;
    VBucket DATA_EMPTY;
    VBucket DATA_ASSETS_DIFFERENT_FIELD;
    VBucket DATA_ASSETS_DIFFERENT_CHANGED_FIELD;
    VBucket DATA_ASSETS_SAME_NAME_ALL_CHANGED;
    VBucket DATA_ASSETS_ASSET_SAME_NAME;
    VBucket DATA_NULL_ASSET;
    VBucket DATA_ASSET_IN_ASSETS;
    VBucket DATA_NULL_ASSETS;
    VBucket DATA_ALL_NULL_ASSETS;
    VBucket DATA_EMPTY_ASSETS;
    VBucket DATA_UPDATE_DELTE_NOCHANGE_INSERT;
    VBucket DATA_SAME_NAME_ASSETS;

    Asset GenAsset(std::string name, std::string hash)
    {
        Asset asset;
        asset.name = name;
        asset.hash = hash;
        return asset;
    }

    VBucket GenDatum(int64_t id, std::string name, Type asset, Type assets)
    {
        VBucket datum;
        datum[FIELD_ID] = id;
        datum[FIELD_NAME] = name;
        datum[FIELD_HOUSE] = asset;
        datum[FIELD_CARS] = assets;
        return datum;
    }

    void GenData()
    {
        a1 = GenAsset("mansion", "mansion1");
        a1Changed = GenAsset("mansion", "mansion1Changed");
        a2 = GenAsset("suv", "suv1");
        a2Changed = GenAsset("suv", "suv1Changed");
        a3 = GenAsset("truck", "truck1");
        a3Changed = GenAsset("truck", "truck1Changed");
        a4 = GenAsset("sedan", "sedan1");
        a4Changed = GenAsset("sedan", "sedan1Changed");
        a5 = GenAsset("trucker", "truck1");
        a5Changed = GenAsset("trucker", "truck1Changed");
        DATA_EMPTY.clear();
        DATA_BASELINE = GenDatum(1, "Jack", a1, Assets({a2, a3, a4})); // id is 1
        DATA_EMPTY_ASSET = GenDatum(2, "PoorGuy", a1, Assets({})); // id is 2
        DATA_EMPTY_ASSET.erase(FIELD_HOUSE);
        DATA_ASSET_SAME_NAME_BUT_CHANGE = GenDatum(3, "Alice", a1Changed, Assets({a2, a3, a4})); // id is 3
        DATA_ASSETS_SAME_NAME_PARTIALLY_CHANGED = GenDatum(4, "David", a1, Assets({a2, a3Changed, a4})); // id is 4
        DATA_ALL_SAME = GenDatum(5, "Marry", a1, Assets({a2, a3, a4})); // id is 5
        DATA_ASSETS_MORE_FIELD = GenDatum(6, "Carl", a1, Assets({a2, a3, a4, a5})); // id is 6
        DATA_ASSETS_DIFFERENT_FIELD = GenDatum(7, "Carllol", a1, Assets({a2, a3, a5})); // id is 7
        DATA_ASSETS_DIFFERENT_CHANGED_FIELD = GenDatum(8, "Carllol", a1, Assets({a2, a3Changed, a5})); // id is 8
        DATA_ASSETS_SAME_NAME_ALL_CHANGED = GenDatum(
            9, "Lob", a1Changed, Assets({a2Changed, a3Changed, a4Changed})); // id is 9
        DATA_ASSETS_ASSET_SAME_NAME = GenDatum(10, "Lob2", a1, Assets({a1, a2, a3})); // id is 10
        std::monostate nil;
        DATA_NULL_ASSET = GenDatum(11, "Lob3", nil, Assets({a1, a2, a3})); // id is 11
        DATA_ASSET_IN_ASSETS = GenDatum(12, "Lob4", Assets({a1}), Assets({a2, a3, a4})); // id is 12
        DATA_NULL_ASSETS = GenDatum(13, "Lob5", Assets({a1}), nil); // id is 13
        DATA_ALL_NULL_ASSETS = GenDatum(14, "Nico", nil, nil); // id is 14
        DATA_EMPTY_ASSETS = GenDatum(15, "Lob6", a1, Assets({})); // id is 15
        DATA_EMPTY_ASSETS.erase(FIELD_CARS);
        DATA_UPDATE_DELTE_NOCHANGE_INSERT = GenDatum(16, "Nico321", nil, Assets({a2Changed, a4, a5})); // id is 16
        DATA_SAME_NAME_ASSETS = GenDatum(16, "Nico156", nil, Assets({a1, a1Changed})); // id is 16
    }

    void CreateDB()
    {
        sqlite3 *db = nullptr;
        int errCode = sqlite3_open(g_storePath.c_str(), &db);
        if (errCode != SQLITE_OK) {
            LOGE("open db failed:%d", errCode);
            sqlite3_close(db);
            return;
        }
        const string sql =
            "PRAGMA journal_mode=WAL;";
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql.c_str()), E_OK);
        sqlite3_close(db);
    }

    void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
        RelationalDBProperties &properties)
    {
        properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
        properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
        properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
        properties.SetStringProp(RelationalDBProperties::STORE_ID, STORE_ID);
        std::string identifier = userId + "-" + appId + "-" + STORE_ID;
        std::string hashIdentifier = DBCommon::TransferHashString(identifier);
        properties.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
    }

    const RelationalSyncAbleStorage *GetRelationalStore()
    {
        RelationalDBProperties properties;
        InitStoreProp(g_storePath, APP_ID, USER_ID, properties);
        int errCode = E_OK;
        g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
        if (g_store == nullptr) {
            LOGE("Get db failed:%d", errCode);
            return nullptr;
        }
        return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
    }

    class DistributedDBCloudAssetCompareTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    };

    void DistributedDBCloudAssetCompareTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(TEST_DIR);
        LOGD("test dir is %s", TEST_DIR.c_str());
        g_dbDir = TEST_DIR + "/";
        g_storePath =  g_dbDir + STORE_ID + ".db";
        DistributedDBToolsUnitTest::RemoveTestDbFiles(TEST_DIR);
    }

    void DistributedDBCloudAssetCompareTest::TearDownTestCase(void)
    {
    }

    void DistributedDBCloudAssetCompareTest::SetUp(void)
    {
        DistributedDBToolsUnitTest::PrintTestCaseInfo();
        LOGD("Test dir is %s", TEST_DIR.c_str());
        CreateDB();
        ASSERT_EQ(g_mgr.OpenStore(g_storePath, STORE_ID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
        ASSERT_NE(g_delegate, nullptr);
        g_storageProxy = std::make_shared<TestStorageProxy>((ICloudSyncStorageInterface *) GetRelationalStore());
        ASSERT_NE(g_storageProxy, nullptr);
        g_cloudSyncer = std::make_shared<TestCloudSyncer>(g_storageProxy);
        ASSERT_NE(g_cloudSyncer, nullptr);
        g_cloudSyncer->SetAssetFields(TABLE_NAME, ASSET_FIELDS);
        GenData();
    }

    void DistributedDBCloudAssetCompareTest::TearDown(void)
    {
        RefObject::DecObjRef(g_store);
        if (g_delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
            g_delegate = nullptr;
            g_storageProxy = nullptr;
            g_cloudSyncer = nullptr;
        }
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(TEST_DIR) != 0) {
            LOGE("rm test db files error.");
        }
    }

    static bool IsAssetEq(Asset &target, Asset &expected)
    {
        if (target.name != expected.name ||
            target.flag != expected.flag ||
            target.status != expected.status) {
            return false;
        }
        return true;
    }

    static bool CheckAssetDownloadList(std::string fieldName, std::map<std::string, Assets> &target,
        std::map<std::string, Assets> &expected)
    {
        if (target[fieldName].size() != expected[fieldName].size()) {
            LOGE("[CheckAssetDownloadList] size is not equal actual %zu expect %zu", target[fieldName].size(),
                expected[fieldName].size());
            return false;
        }
        for (size_t i = 0; i < target[fieldName].size(); i++) {
            if (!IsAssetEq(target[fieldName][i], expected[fieldName][i])) {
                LOGE("[CheckAssetDownloadList] asset not equal fieldName %s index %zu", fieldName.c_str(), i);
                return false;
            }
        }
        return true;
    }

    static void TagAsset(AssetOpType flag, AssetStatus status, Asset &asset)
    {
        asset.flag = static_cast<uint32_t>(flag);
        asset.status = static_cast<uint32_t>(status);
    }

    static AssetStatus GetDownloadWithNullStatus()
    {
        return static_cast<AssetStatus>(static_cast<uint32_t>(DOWNLOADING) | static_cast<uint32_t>(DOWNLOAD_WITH_NULL));
    }

    /**
     * @tc.name: AssetCmpTest001
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest001, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(DATA_BASELINE, DATA_EMPTY);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a1);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a2);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a3);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a4);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest002
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest002, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(DATA_BASELINE, DATA_EMPTY_ASSET);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a1);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a2);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a3);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a4);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest003
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest003, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(DATA_BASELINE, DATA_ASSET_SAME_NAME_BUT_CHANGE);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a1);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest004
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest004, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ASSETS_SAME_NAME_PARTIALLY_CHANGED);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a3);
        expectedList[FIELD_HOUSE] = {};
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest005
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest005, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ALL_SAME);
        std::map<std::string, Assets> expectedList;
        expectedList[FIELD_HOUSE] = {};
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest006
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest006, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ASSETS_MORE_FIELD);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a5);
        expectedList[FIELD_HOUSE] = {};
        expectedList[FIELD_CARS] = { a2, a3, a4, a5 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest007
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest007, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ASSETS_DIFFERENT_FIELD);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a5);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a4);
        expectedList[FIELD_HOUSE] = {};
        expectedList[FIELD_CARS] = { a2, a3, a5, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest008
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest008, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ASSETS_DIFFERENT_CHANGED_FIELD);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a3);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a5);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a4);
        expectedList[FIELD_HOUSE] = {};
        expectedList[FIELD_CARS] = { a2, a3, a5, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest009
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest009, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ASSETS_SAME_NAME_ALL_CHANGED);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a1);
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a2);
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a3);
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a4);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest010
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest010, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_EMPTY_ASSET, DATA_BASELINE);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a1);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a2);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a3);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a4);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest011
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest011, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_EMPTY_ASSET, DATA_ASSETS_ASSET_SAME_NAME);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a1);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a2);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a3);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a1, a2, a3 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest012
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest012, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_EMPTY_ASSET, DATA_ASSETS_ASSET_SAME_NAME);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a1);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a2);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a3);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a1, a2, a3 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest013
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest013, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ASSET_IN_ASSETS);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::NO_CHANGE, AssetStatus::DOWNLOADING, a1);
        TagAsset(AssetOpType::NO_CHANGE, AssetStatus::DOWNLOADING, a2);
        TagAsset(AssetOpType::NO_CHANGE, AssetStatus::DOWNLOADING, a3);
        TagAsset(AssetOpType::NO_CHANGE, AssetStatus::DOWNLOADING, a4);
        a2.status = static_cast<uint32_t>(AssetStatus::NORMAL);
        a3.status = static_cast<uint32_t>(AssetStatus::NORMAL);
        a4.status = static_cast<uint32_t>(AssetStatus::NORMAL);
        expectedList[FIELD_CARS] = {a2, a3, a4};
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest014
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest014, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_NULL_ASSETS);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::NO_CHANGE, AssetStatus::DOWNLOADING, a1);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a2);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a3);
        TagAsset(AssetOpType::INSERT, GetDownloadWithNullStatus(), a4);
        expectedList[FIELD_HOUSE] = {};
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }


    /**
     * @tc.name: AssetCmpTest015
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest015, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(DATA_ASSET_SAME_NAME_BUT_CHANGE, DATA_BASELINE);
        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, a1Changed);
        expectedList[FIELD_HOUSE] = { a1Changed };
        expectedList[FIELD_CARS] = {a2, a3, a4};
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest016
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest016, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(DATA_BASELINE, DATA_ASSET_SAME_NAME_BUT_CHANGE);
        EXPECT_EQ(std::get<Asset>(DATA_BASELINE[FIELD_HOUSE]).flag, static_cast<uint32_t>(AssetOpType::UPDATE));
    }

    /**
     * @tc.name: AssetCmpTest017
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest017, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ASSETS_SAME_NAME_PARTIALLY_CHANGED, true);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[0].status, AssetStatus::NORMAL);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[1].status, AssetStatus::UPDATE);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[2].status, AssetStatus::NORMAL);
    }

    /**
     * @tc.name: AssetCmpTest018
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest018, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_EMPTY, true);
        EXPECT_EQ(std::get<Asset>(DATA_BASELINE[FIELD_HOUSE]).status, AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[0].status, AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[1].status, AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[2].status, AssetStatus::INSERT);
    }

    /**
     * @tc.name: AssetCmpTest019
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest019, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_EMPTY, DATA_BASELINE, true);
        EXPECT_EQ(std::get<Assets>(DATA_EMPTY[FIELD_CARS])[0].status, AssetStatus::DELETE | AssetStatus::HIDDEN);
        EXPECT_EQ(std::get<Assets>(DATA_EMPTY[FIELD_CARS])[1].status, AssetStatus::DELETE | AssetStatus::HIDDEN);
        EXPECT_EQ(std::get<Assets>(DATA_EMPTY[FIELD_CARS])[2].status, AssetStatus::DELETE | AssetStatus::HIDDEN);
    }

    /**
     * @tc.name: AssetCmpTest020
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest020, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_ALL_NULL_ASSETS, DATA_BASELINE, false);
        EXPECT_EQ(std::get<Asset>(DATA_BASELINE[FIELD_HOUSE]).flag, static_cast<uint32_t>(AssetOpType::DELETE));
        EXPECT_EQ(std::get<Assets>(DATA_ALL_NULL_ASSETS[FIELD_CARS])[0].flag,
            static_cast<uint32_t>(AssetOpType::DELETE));
        EXPECT_EQ(std::get<Assets>(DATA_ALL_NULL_ASSETS[FIELD_CARS])[1].flag,
            static_cast<uint32_t>(AssetOpType::DELETE));
        EXPECT_EQ(std::get<Assets>(DATA_ALL_NULL_ASSETS[FIELD_CARS])[2].flag,
            static_cast<uint32_t>(AssetOpType::DELETE));

        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a1);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a2);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a3);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, a4);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest021
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest021, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_ALL_NULL_ASSETS, DATA_ALL_NULL_ASSETS, true);
        ASSERT_TRUE(DATA_ALL_NULL_ASSETS[FIELD_HOUSE].index() == TYPE_INDEX<Nil>);
        ASSERT_TRUE(DATA_ALL_NULL_ASSETS[FIELD_CARS].index() == TYPE_INDEX<Nil>);

        std::map<std::string, Assets> expectedList;
        expectedList[FIELD_HOUSE] = {};
        expectedList[FIELD_CARS] = {};
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest022
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest022, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_BASELINE, DATA_ALL_NULL_ASSETS, false);
        EXPECT_EQ(std::get<Asset>(DATA_BASELINE[FIELD_HOUSE]).flag, static_cast<uint32_t>(AssetOpType::INSERT));
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[0].flag, static_cast<uint32_t>(AssetOpType::INSERT));
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[1].flag, static_cast<uint32_t>(AssetOpType::INSERT));
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[2].flag, static_cast<uint32_t>(AssetOpType::INSERT));

        std::map<std::string, Assets> expectedList;
        TagAsset(AssetOpType::INSERT,
            static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL), a1);
        TagAsset(AssetOpType::INSERT,
            static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL), a2);
        TagAsset(AssetOpType::INSERT,
            static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL), a3);
        TagAsset(AssetOpType::INSERT,
            static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL), a4);
        expectedList[FIELD_HOUSE] = { a1 };
        expectedList[FIELD_CARS] = { a2, a3, a4 };
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_HOUSE, assetList, expectedList));
        ASSERT_TRUE(CheckAssetDownloadList(FIELD_CARS, assetList, expectedList));
    }

    /**
     * @tc.name: AssetCmpTest023
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest023, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_ASSET_SAME_NAME_BUT_CHANGE, DATA_BASELINE, true);
        EXPECT_EQ(std::get<Asset>(DATA_ASSET_SAME_NAME_BUT_CHANGE[FIELD_HOUSE]).status,
            AssetStatus::UPDATE);
        EXPECT_EQ(std::get<Assets>(DATA_ASSET_SAME_NAME_BUT_CHANGE[FIELD_CARS])[0].status,
            AssetStatus::NORMAL);
        EXPECT_EQ(std::get<Assets>(DATA_ASSET_SAME_NAME_BUT_CHANGE[FIELD_CARS])[1].status,
            AssetStatus::NORMAL);
        EXPECT_EQ(std::get<Assets>(DATA_ASSET_SAME_NAME_BUT_CHANGE[FIELD_CARS])[2].status,
            AssetStatus::NORMAL);
    }

    /**
     * @tc.name: AssetCmpTest024
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest024, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_ASSETS_DIFFERENT_CHANGED_FIELD, DATA_BASELINE, true);
        EXPECT_EQ(std::get<Asset>(DATA_ASSETS_DIFFERENT_CHANGED_FIELD[FIELD_HOUSE]).status,
            AssetStatus::NORMAL);
        EXPECT_EQ(std::get<Assets>(DATA_ASSETS_DIFFERENT_CHANGED_FIELD[FIELD_CARS])[0].status,
            AssetStatus::NORMAL);
        EXPECT_EQ(std::get<Assets>(DATA_ASSETS_DIFFERENT_CHANGED_FIELD[FIELD_CARS])[1].status,
            AssetStatus::UPDATE);
        EXPECT_EQ(std::get<Assets>(DATA_ASSETS_DIFFERENT_CHANGED_FIELD[FIELD_CARS])[2].status,
            AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_ASSETS_DIFFERENT_CHANGED_FIELD[FIELD_CARS])[3].status,
            AssetStatus::DELETE | AssetStatus::HIDDEN);
    }

    /**
     * @tc.name: AssetCmpTest025
     * @tc.desc: Cloud device contain a record without assets, local device insert an assets and begin to sync.
     * CloudAsset will be a record without assets. Local data will be a record with assets.
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest025, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(DATA_BASELINE, DATA_NULL_ASSETS, true);
        EXPECT_EQ(std::get<Asset>(DATA_BASELINE[FIELD_HOUSE]).status, AssetStatus::NORMAL);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[0].status, AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[1].status, AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[2].status, AssetStatus::INSERT);
    }

    /**
     * @tc.name: AssetCmpTest026
     * @tc.desc: Cloud device contain a record without assets, local device insert an assets and begin to sync.
     * CloudAsset will be a record without assets. Local data will be a record with assets.
     * In this case, record do not contain certain asset column
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest026, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(DATA_BASELINE, DATA_EMPTY_ASSETS, true);
        EXPECT_EQ(std::get<Asset>(DATA_BASELINE[FIELD_HOUSE]).status, AssetStatus::NORMAL);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[0].status, AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[1].status, AssetStatus::INSERT);
        EXPECT_EQ(std::get<Assets>(DATA_BASELINE[FIELD_CARS])[2].status, AssetStatus::INSERT);
    }

    /**
     * @tc.name: AssetCmpTest027
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest027, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_UPDATE_DELTE_NOCHANGE_INSERT, DATA_BASELINE, false);
        EXPECT_EQ(std::get<Assets>(DATA_UPDATE_DELTE_NOCHANGE_INSERT[FIELD_CARS])[0].flag,
            static_cast<uint32_t>(AssetOpType::UPDATE));
        EXPECT_EQ(std::get<Assets>(DATA_UPDATE_DELTE_NOCHANGE_INSERT[FIELD_CARS])[1].flag,
            static_cast<uint32_t>(AssetOpType::NO_CHANGE));
        EXPECT_EQ(std::get<Assets>(DATA_UPDATE_DELTE_NOCHANGE_INSERT[FIELD_CARS])[2].flag,
            static_cast<uint32_t>(AssetOpType::INSERT));
        EXPECT_EQ(std::get<Assets>(DATA_UPDATE_DELTE_NOCHANGE_INSERT[FIELD_CARS])[3].flag,
            static_cast<uint32_t>(AssetOpType::DELETE));
    }

    /**
     * @tc.name: AssetCmpTest028
     * @tc.desc: Two same name asset appears in the assets field, this situation is not allowed
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest028, TestSize.Level0)
    {
        auto assetList = g_cloudSyncer->TestTagAssetsInSingleRecord(
            DATA_ALL_NULL_ASSETS, DATA_BASELINE, true);
        EXPECT_EQ(std::get<Assets>(DATA_ALL_NULL_ASSETS[FIELD_CARS])[0].status,
            AssetStatus::DELETE | AssetStatus::HIDDEN);
        EXPECT_EQ(std::get<Assets>(DATA_ALL_NULL_ASSETS[FIELD_CARS])[1].status,
            AssetStatus::DELETE | AssetStatus::HIDDEN);
        EXPECT_EQ(std::get<Assets>(DATA_ALL_NULL_ASSETS[FIELD_CARS])[2].status,
            AssetStatus::DELETE | AssetStatus::HIDDEN);
    }

    /**
     * @tc.name: AssetCmpTest029
     * @tc.desc: Two same name asset appears in the assets field, this situation is not allowed
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest029, TestSize.Level0)
    {
        Field field1 = { FIELD_HOUSE, TYPE_INDEX<Asset> };
        Field field2 = { FIELD_CARS, TYPE_INDEX<Assets> };
        std::vector<Field> assetFields = { field1, field2 };
        ASSERT_TRUE(g_cloudSyncer->TestIsDataContainDuplicateAsset(assetFields, DATA_SAME_NAME_ASSETS));
        ASSERT_TRUE(g_cloudSyncer->TestIsDataContainDuplicateAsset(assetFields, DATA_BASELINE) == false);
    }

    /**
     * @tc.name: AssetCmpTest030
     * @tc.desc: Check operator == in the asset structure
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetCmpTest030, TestSize.Level0)
    {
        Asset asset = GenAsset("mansion", "mansion1");
        EXPECT_TRUE(std::get<Asset>(DATA_BASELINE[FIELD_HOUSE]) == asset);
    }

    /**
     * @tc.name: AssetOperation001
     * @tc.desc: Different opType with end download action and assets
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangqiquan
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetOperation001, TestSize.Level0)
    {
        VBucket cacheAssets;
        VBucket dbAssets;
        Asset asset;
        asset.status = AssetStatus::DOWNLOADING;
        asset.name = "name";
        cacheAssets["field"] = asset;
        dbAssets["field"] = asset;
        // check both downloading after download
        auto res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::END_DOWNLOAD);
        EXPECT_EQ(res["field"].size(), 1u);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::HANDLE);
        // status mask download with null
        asset.status = (static_cast<uint32_t>(AssetStatus::DOWNLOADING) |
            static_cast<uint32_t>(AssetStatus::DOWNLOAD_WITH_NULL));
        cacheAssets["field"] = asset;
        dbAssets["field"] = asset;
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::END_DOWNLOAD);
        EXPECT_EQ(res["field"].size(), 1u);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::HANDLE);
        // if status not equal, not handle
        asset.status = AssetStatus::UPDATE;
        dbAssets["field"] = asset;
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::END_DOWNLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
        // if asset not exist, not handle
        dbAssets.erase("field");
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::END_DOWNLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
    }

    /**
     * @tc.name: AssetOperation002
     * @tc.desc: Different opType with start download action and assets
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangqiquan
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetOperation002, TestSize.Level0)
    {
        VBucket cacheAssets;
        VBucket dbAssets;
        Asset asset;
        asset.name = "name";
        asset.status = AssetStatus::DOWNLOADING;
        cacheAssets["field"] = asset;
        dbAssets["field"] = asset;
        // check both downloading before download
        auto res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::START_DOWNLOAD);
        EXPECT_EQ(res["field"].size(), 1u);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::HANDLE);
        // if status not equal, not handle
        asset.status = AssetStatus::UPDATE;
        dbAssets["field"] = asset;
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::START_DOWNLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
        // if db asset not exist, not handle
        dbAssets.erase("field");
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::START_DOWNLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
        // if db asset not exist but cache is delete, handle
        asset.flag = static_cast<uint32_t>(AssetOpType::DELETE);
        cacheAssets["field"] = asset;
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::START_DOWNLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::HANDLE);
    }

    /**
     * @tc.name: AssetOperation003
     * @tc.desc: Different opType with start upload action and assets
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangqiquan
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetOperation003, TestSize.Level0)
    {
        Asset asset;
        VBucket cacheAssets;
        VBucket dbAssets;
        asset.name = "name";
        asset.status = AssetStatus::UPDATE;
        cacheAssets["field"] = asset;
        dbAssets["field"] = asset;
        // check both update before upload
        auto res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::START_UPLOAD);
        EXPECT_EQ(res["field"].size(), 1u);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::HANDLE);
        // if status not equal, not handle
        asset.status = AssetStatus::DELETE;
        dbAssets["field"] = asset;
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::START_UPLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
        // if asset not exist, not handle
        dbAssets.erase("field");
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::START_UPLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
    }

    /**
     * @tc.name: AssetOperation004
     * @tc.desc: Different opType with start upload action and assets
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangqiquan
     */
    HWTEST_F(DistributedDBCloudAssetCompareTest, AssetOperation004, TestSize.Level0)
    {
        Asset asset;
        VBucket cacheAssets;
        VBucket dbAssets;
        asset.name = "name";
        asset.status = (static_cast<uint32_t>(AssetStatus::UPDATE) | static_cast<uint32_t>(AssetStatus::UPLOADING));
        cacheAssets["field"] = asset;
        dbAssets["field"] = asset;
        // check both UPLOADING after upload
        auto res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::END_UPLOAD);
        EXPECT_EQ(res["field"].size(), 1u);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::HANDLE);
        // if status not equal, not handle
        asset.status = AssetStatus::DELETE;
        dbAssets["field"] = asset;
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::END_UPLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
        // if asset not exist, not handle
        dbAssets.erase("field");
        res = AssetOperationUtils::CalAssetOperation(cacheAssets, dbAssets,
            AssetOperationUtils::CloudSyncAction::END_UPLOAD);
        EXPECT_EQ(res["field"][asset.name], AssetOperationUtils::AssetOpType::NOT_HANDLE);
    }
}
