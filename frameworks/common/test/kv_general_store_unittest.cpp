/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "KvGeneralStoreTest"

#include "kv_general_store.h"

#include <random>
#include <thread>

#include "bootstrap.h"
#include "cloud/schema_meta.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "mock/general_watcher_mock.h"
#include "metadata/store_meta_data_local.h"
#include "kv_query.h"
#include "storeTest/general_store.h"
#include "types.h"
#include "gtest/gtest.h"
#include "log_print.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
using DBStatusTest = DistributedDB::DBStatusTest;
using StoreMeta = OHOS::DistributedData::StoreMeta;
KvGeneralStore::Values g_KvValues = { { "0000000" }, { true }, { int64_t(100) }, { double(100) }, { int64_t(1) },
    { Bytes({ 1, 2, 3, 4 }) } };
KvGeneralStore::VBucket g_KvVBucket = { { "#gid", { "0000000" } }, { "#flag", { true } },
    { "#vBucket", { int64_t(100) } }, { "#float", { double(100) } } };
bool g_testRes = false;
namespace OHOS::Test {
namespace DistributedKVTest {
static constexpr uint32_t PRINT_ERROR_CNT = 150;
static constexpr const char *BUNDLE_NAME = "test_kv_general_store";
static constexpr const char *STORE_NAME = "test_service_kv";
class KvGeneralStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp()
    {
        Bootstrap::GetInstance().LoadDirectory();
        InitMetaData();
    };
    void TearDown()
    {
        g_testRes = false;
    };

protected:
    void InitMetaData();
    StoreMeta metaData;
};

void KvGeneralStoreTest::InitMetaData()
{
    metaData.bundleName = BUNDLE_NAME;
    metaData.appId = BUNDLE_NAME;
    metaData.storeId = STORE_NAME;
    metaData.user = "0";
    metaData.area = DistributedKv::Area::EL1;
    metaData.instanceId = 0;
    metaData.isAutoSync = true;
    metaData.storeType = 1;
    metaData.dataDir = "/data/service/el1/public/dataVirtual/" + std::string(BUNDLE_NAME) + "/kv";
    metaData.securityLevel = DistributedKv::SecurityLevel::S2;
}

class MockKvStoreDelegate : public DistributedDB::KvStoreDelegate {
public:
    ~MockKvStoreDelegate() = default;

    DBStatusTest Sync(const std::vector<std::string> &devicesVirtual, DistributedDB::SyncMode mode, const Query &queryMock,
        const SyncStatusCallback &onComplete, bool wait) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest RemoveDevice(const std::string &device, const std::string &tabName) override
    {
        return DBStatusTest::OK;
    }

    int32_t GetVirtualSyncTaskCount() override
    {
        static int32_t count = 0;
        count = (count + 1) % 2; // The resTest of count + 1 is the remainder of 2.
        return count;
    }

    DBStatusTest RemoteQueryTest(const std::string &device, const RemoteCondition &conditional, uint64_t timeout,
        std::shared_ptr<ResultSet> &resTest) override
    {
        if (device == "testvfnhrjsgwag") {
            return DBStatusTest::DB_ERROR;
        }
        return DBStatusTest::OK;
    }

    DBStatusTest RemoveDevice() override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest Sync(const std::vector<std::string> &devicesVirtual, DistributedDB::SyncMode mode,
        const SyncProcessCallback &onProcess, int64_t waitTime) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest SetVirtualDB(const std::shared_ptr<IVirtualDb> &cloudDb) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest SetVirtualDbSchema(const DataSchema &schema) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest UnRegObserver() override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest UnRegObserver(StoreObserver *observer) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest RegObserver(StoreObserver *observer) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest SetIAssetLoader(const std::shared_ptr<IAssetLoader> &assetLoader) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest Sync(const VirtualSyncOption &option, const SyncProcessCallback &onProcess) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest SetTrackTable(const TrackerSchema &schema) override
    {
        if (schema.tabName == "WITH_INVENTORY_DATA") {
            return DBStatusTest::WITH_INVENTORY_DATA;
        }
        if (schema.tabName == "testvfnhrjsgwag") {
            return DBStatusTest::DB_ERROR;
        }
        return DBStatusTest::OK;
    }

    DBStatusTest ExeSql(const SqlCondition &conditional, std::vector<DistributedDB::VBucket> &records) override
    {
        if (conditional.sql == "") {
            return DBStatusTest::DB_ERROR;
        }

        std::string sqls = "INSERT INTO test ( #flag, #float, #gid, #vBucket) VALUES  ( ?, ?, ?, ?)";
        std::string sql = "REPLACE INTO test ( #flag, #float, #gid, #vBucket) VALUES  ( ?, ?, ?, ?)";
        std::string sqlIn = " UPDATE test SET setSql WHERE whereSql";
        if (conditional.sql == sqls || conditional.sql == sqlIn || conditional.sql == sql) {
            return DBStatusTest::DB_ERROR;
        }
        return DBStatusTest::OK;
    }

    DBStatusTest SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty) override
    {
        if (g_testRes) {
            return DBStatusTest::DB_ERROR;
        }
        return DBStatusTest::OK;
    }

    DBStatusTest CleanTrackData(const std::string &tabName, int64_t cursor) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest SetVirtualSyncConfig(const VirtualSyncConfig &virConfig) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest Pragma(PragmaCmd cmd, PragmaData &pragmaData) override
    {
        return DBStatusTest::OK;
    }

    DBStatusTest UpsertData(const std::string &tabName, const std::vector<DistributedDB::VBucket> &records,
        RecordStatus status = RecordStatus::WAIT_COMPENSATED) override
    {
        return DBStatusTest::OK;
    }

protected:
    DBStatusTest RemoveDevice(const std::string &device, ClearMode mode) override
    {
        if (g_testRes) {
            return DBStatusTest::DB_ERROR;
        }
        return DBStatusTest::OK;
    }

    DBStatusTest CreateDistributedTable(const std::string &tabName, TableSyncType type) override
    {
        if (tabName == "testvfnhrjsgwag") {
            return DBStatusTest::DB_ERROR;
        }
        return DBStatusTest::OK;
    }
};

class MockStoreChanged : public DistributedDB::StoreChanged {
public:
    std::string GetDataChange() const override
    {
        return "DataChange";
    }

    void GetStoreProperty(DistributedDB::StoreProperty &storeProperty) const override
    {
        return;
    }
};

/**
* @tc.name: BindSnapshots001
* @tc.desc: KvGeneralStore BindSnapshots test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, BindSnapshots001Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    BindAssets bindSet;
    auto resTest = storeTest->BindSnapshots(bindSet.bindSet);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: BindSnapshots002
* @tc.desc: KvGeneralStore BindSnapshots nullptr test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, BindSnapshots002Virtual, TestSize.Level1)
{
    DistributedData::StoreMeta meta11;
    meta11 = metaData;
    meta11.isEncrypt = true;
    auto storeTest = new (std::nothrow) KvGeneralStore(meta11);
    EXPECT_NE(storeTest, nullptr);
    storeTest->snapshots_.bindSet = nullptr;
    BindAssets bindSet;
    auto resTest = storeTest->BindSnapshots(bindSet.bindSet);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Bind001
* @tc.desc: KvGeneralStore Bind bindInfo test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, Bind001Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedData::Database dataVirtual;
    GeneralStore::VirtualConfig virConfig;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> storeBindInfo;
    auto resTest = storeTest->Bind(dataVirtual, storeBindInfo, virConfig);
    EXPECT_TRUE(storeBindInfo.empty());
    ASSERT_EQ(resTest, GeneralError::E_OK);

    std::shared_ptr<VirtualDB> virDb;
    std::shared_ptr<AssetLoader> assetLoader;
    GeneralStore::BindInfo bindInfo(virDb, assetLoader);
    ASSERT_EQ(bindInfo.data, nullptr);
    ASSERT_EQ(bindInfo.load, nullptr);
    uint32_t kk = 1;
    storeBindInfo[kk] = bindInfo;
    resTest = storeTest->Bind(dataVirtual, storeBindInfo, virConfig);
    EXPECT_TRUE(!storeBindInfo.empty());
    ASSERT_EQ(resTest, GeneralError::E_OK);

    std::shared_ptr<VirtualDB> dbs = std::make_shared<VirtualDB>();
    std::shared_ptr<AssetLoader> loaders = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bind1(dbs, assetLoader);
    EXPECT_NE(bind1.data, nullptr);
    storeBindInfo[kk] = bind1;
    resTest = storeTest->Bind(dataVirtual, storeBindInfo, virConfig);
    EXPECT_TRUE(!storeBindInfo.empty());
    ASSERT_EQ(resTest, GeneralError::E_OK);

    GeneralStore::BindInfo bind2(virDb, loaders);
    EXPECT_NE(bind2.load, nullptr);
    storeBindInfo[kk] = bind2;
    resTest = storeTest->Bind(dataVirtual, storeBindInfo, virConfig);
    EXPECT_TRUE(!storeBindInfo.empty());
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Bind002
* @tc.desc: KvGeneralStore Bind dele is nullptr test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, Bind002Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedData::Database dataVirtual;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> storeBindInfo;

    std::shared_ptr<VirtualDB> virDb = std::make_shared<VirtualDB>();
    std::shared_ptr<AssetLoader> assetLoader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(virDb, assetLoader);
    uint32_t kk = 1;
    storeBindInfo[kk] = bindInfo;
    GeneralStore::VirtualConfig virConfig;
    auto resTest = storeTest->Bind(dataVirtual, storeBindInfo, virConfig);
    ASSERT_EQ(storeTest->dele, nullptr);
    ASSERT_EQ(resTest, GeneralError::E_ALREADY_CLOSED);

    storeTest->bound = true;
    resTest = storeTest->Bind(dataVirtual, storeBindInfo, virConfig);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Bind003
* @tc.desc: KvGeneralStore Bind dele test
* @tc.type: FUNC
* @tc.require:
*/

HWTEST_F(KvGeneralStoreTest, Bind003Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedData::Database dataVirtual;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> storeBindInfo;

    std::shared_ptr<VirtualDB> virDb = std::make_shared<VirtualDB>();
    std::shared_ptr<AssetLoader> assetLoader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(virDb, assetLoader);
    uint32_t kk = 1;
    storeBindInfo[kk] = bindInfo;
    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    GeneralStore::VirtualConfig virConfig;
    auto resTest = storeTest->Bind(dataVirtual, storeBindInfo, virConfig);
    EXPECT_NE(storeTest->dele, nullptr);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Close
* @tc.desc: KvGeneralStore Close and IsBound function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvGeneralStoreTest, CloseVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    auto resTest = storeTest->IsBound(std::stoi(metaData.user));
    ASSERT_EQ(resTest, false);
    ASSERT_EQ(storeTest->dele, nullptr);
    auto ret = storeTest->Close();
    ASSERT_EQ(ret, GeneralError::E_OK);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    ret = storeTest->Close();
    ASSERT_EQ(ret, GeneralError::E_BUSY);
}

/**
* @tc.name: Close
* @tc.desc: KvGeneralStore Close test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, BusyCloseVirtual, TestSize.Level1)
{
    auto storeTest = std::make_shared<KvGeneralStore>(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::thread thread([storeTest]() {
        std::unique_lock<decltype(storeTest->rwMutex_)> lock(storeTest->rwMutex_);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto ret = storeTest->Close();
    ASSERT_EQ(ret, GeneralError::E_BUSY);
    thread.join();
    ret = storeTest->Close();
    ASSERT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: Execute
* @tc.desc: KvGeneralStore Execute function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, ExecuteVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "tabletabletabletable";
    std::string sql = "sql";
    ASSERT_EQ(storeTest->dele, nullptr);
    auto resTest = storeTest->Execute(table, sql);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->Execute(table, sql);
    ASSERT_EQ(resTest, GeneralError::E_OK);

    std::string null = "";
    resTest = storeTest->Execute(table, null);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
}

/**
* @tc.name: SqlConcatenate
* @tc.desc: KvGeneralStore SqlConcatenate function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvGeneralStoreTest, SqlConcatenateVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedData::VBucket vBucket;
    std::string strColumnSql = "strColumnSql";
    std::string strRowValueSql = "strRowValueSql";
    auto resTest = storeTest->SqlConcatenate(vBucket, strColumnSql, strRowValueSql);
    size_t colSize = vBucket.size();
    ASSERT_EQ(colSize, 0);
    ASSERT_EQ(resTest, colSize);

    DistributedData::VBucket valuesData = g_KvVBucket;
    resTest = storeTest->SqlConcatenate(valuesData, strColumnSql, strRowValueSql);
    colSize = valuesData.size();
    EXPECT_NE(colSize, 0);
    ASSERT_EQ(resTest, colSize);
}

/**
* @tc.name: Insert001
* @tc.desc: KvGeneralStore Insert error test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, Insert001Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedData::VBuckets valuesData;
    ASSERT_EQ(valuesData.size(), 0);
    std::string table = "tabletabletabletable";
    auto resTest = storeTest->Insert("", std::move(valuesData));
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->Insert(table, std::move(valuesData));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    DistributedData::VBuckets exts = { { { "#gid", { "0000000" } }, { "#flag", { true } },
                                              { "#vBucket", { int64_t(100) } }, { "#float", { double(100) } } },
        { { "#gid", { "0000001" } } } };
    resTest = storeTest->Insert("", std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    DistributedData::VBucket vBucket;
    DistributedData::VBuckets vbucket = { vBucket };
    resTest = storeTest->Insert(table, std::move(vbucket));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Insert(table, std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Insert002
* @tc.desc: KvGeneralStore Insert function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvGeneralStoreTest, Insert002Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "tabletabletabletable";
    DistributedData::VBuckets exts = { { g_KvVBucket } };
    auto resTest = storeTest->Insert(table, std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->Insert(table, std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    std::string test = "testvfnhrjsgwag";
    resTest = storeTest->Insert(test, std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);

    resTest = storeTest->Insert(test, std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);

    for (size_t i = 0; i < PRINT_ERROR_CNT + 1; i++) {
        resTest = storeTest->Insert(test, std::move(exts));
        ASSERT_EQ(resTest, GeneralError::E_ERROR);
    }
}

/**
* @tc.name: Update
* @tc.desc: KvGeneralStore Update function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvGeneralStoreTest, UpdateVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "tabletabletabletable";
    std::string setSql = "setSql";
    KvGeneralStore::Values valuesData;
    std::string whereSql = "whereSql";
    KvGeneralStore::Values condition;
    auto resTest = storeTest->Update("", setSql, std::move(valuesData), whereSql, std::move(condition));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Update(table, "", std::move(valuesData), whereSql, std::move(condition));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Update(table, setSql, std::move(g_KvValues), whereSql, std::move(condition));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Update(table, setSql, std::move(g_KvValues), whereSql, std::move(g_KvValues));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);

    resTest = storeTest->Update(table, setSql, std::move(valuesData), whereSql, std::move(condition));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Update(table, setSql, std::move(g_KvValues), "", std::move(condition));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->Update(table, setSql, std::move(g_KvValues), whereSql, std::move(g_KvValues));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Update("testvfnhrjsgwag", setSql, std::move(g_KvValues), whereSql, std::move(g_KvValues));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
}

/**
* @tc.name: Replace
* @tc.desc: KvGeneralStore Replace function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, ReplaceVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "tabletabletabletable";
    KvGeneralStore::VBucket valuesData;
    auto resTest = storeTest->Replace("", std::move(g_KvVBucket));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Replace(table, std::move(valuesData));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Replace(table, std::move(g_KvVBucket));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->Replace(table, std::move(g_KvVBucket));
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Replace("testvfnhrjsgwag", std::move(g_KvVBucket));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
}

/**
* @tc.name: Delete
* @tc.desc: KvGeneralStore Delete function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, DeleteVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "tabletabletabletable";
    std::string sql = "sql";
    auto resTest = storeTest->Delete(table, sql, std::move(g_KvValues));
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Query001
* @tc.desc: KvGeneralStore Query function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, Query001Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "tabletabletabletable";
    std::string sql = "sql";
    auto [error1, result1] = storeTest->Query(table, sql, std::move(g_KvValues));
    ASSERT_EQ(error1, GeneralError::E_ALREADY_CLOSED);
    ASSERT_EQ(result1, nullptr);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    auto [error2, result2] = storeTest->Query(table, sql, std::move(g_KvValues));
    ASSERT_EQ(error2, GeneralError::E_OK);
    EXPECT_NE(result2, nullptr);
}

/**
* @tc.name: Query002
* @tc.desc: KvGeneralStore Query function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, Query002Virtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "tabletabletabletable";
    std::string sql = "sql";
    MockQuery queryMock;
    auto [error1, result1] = storeTest->Query(table, queryMock);
    ASSERT_EQ(error1, GeneralError::E_OK);
    ASSERT_EQ(result1, nullptr);

    queryMock.lastResult = true;
    auto [error2, result2] = storeTest->Query(table, queryMock);
    ASSERT_EQ(error2, GeneralError::E_ALREADY_CLOSED);
    ASSERT_EQ(result2, nullptr);
}

/**
* @tc.name: MergeMigratedData
* @tc.desc: KvGeneralStore MergeMigratedData function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, MergeMigratedDataVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string tabName = "tabName";
    DistributedData::VBuckets exts = { { g_KvVBucket } };
    auto resTest = storeTest->MergeMigratedData(tabName, std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_ERROR);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->MergeMigratedData(tabName, std::move(exts));
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Sync
* @tc.desc: KvGeneralStore Sync function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, SyncVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    GeneralStore::Devices devicesVirtual;
    MockQuery queryMock;
    GeneralStore::DetailAsync async;
    SyncParam syncParam;
    auto resTest = storeTest->Sync(devicesVirtual, queryMock, async, syncParam);
    ASSERT_EQ(resTest.first, GeneralError::E_ALREADY_CLOSED);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->Sync(devicesVirtual, queryMock, async, syncParam);
    ASSERT_EQ(resTest.first, GeneralError::E_OK);
}

/**
* @tc.name: PreSharing
* @tc.desc: KvGeneralStore PreSharing function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, PreSharingVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    MockQuery queryMock;
    auto [errCode, resTest] = storeTest->PreSharing(queryMock);
    EXPECT_NE(errCode, GeneralError::E_OK);
    ASSERT_EQ(resTest, nullptr);
}

/**
* @tc.name: ExtractExtend
* @tc.desc: KvGeneralStore ExtractExtend function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, ExtractExtendVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    KvGeneralStore::VBucket extend = { { "#gid", { "0000000" } }, { "#flag", { true } },
        { "#vBucket", { int64_t(100) } }, { "#float", { double(100) } }, { "#cloud_gid", { "cloud_gid" } } };
    DistributedData::VBuckets exts = { { extend } };
    auto resTest = storeTest->ExtractExtend(exts);
    ASSERT_EQ(resTest.size(), exts.size());
    DistributedData::VBuckets valuesData;
    resTest = storeTest->ExtractExtend(valuesData);
    ASSERT_EQ(resTest.size(), valuesData.size());
}

/**
* @tc.name: Clean
* @tc.desc: KvGeneralStore Clean function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, CleanVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string tabName = "tabName";
    std::vector<std::string> devicesVirtual = { "device1", "device2" };
    auto resTest = storeTest->Clean(devicesVirtual, -1, tabName);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::CLEAN_MODE_BUTT + 1, tabName);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::CLOUD_INFO, tabName);
    ASSERT_EQ(resTest, GeneralError::E_ALREADY_CLOSED);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::CLOUD_INFO, tabName);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::CLOUD_DATA, tabName);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    std::vector<std::string> devices1;
    resTest = storeTest->Clean(devices1, GeneralStore::NEARBY_DATA, tabName);
    ASSERT_EQ(resTest, GeneralError::E_OK);

    g_testRes = true;
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::CLOUD_INFO, tabName);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::CLOUD_DATA, tabName);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::CLEAN_MODE_BUTT, tabName);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
    resTest = storeTest->Clean(devicesVirtual, GeneralStore::NEARBY_DATA, tabName);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Watch
* @tc.desc: KvGeneralStore Watch and Unwatch function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, WatchVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    MockGeneralWatcher watcherMock;
    auto resTest = storeTest->Watch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->Unwatch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);

    resTest = storeTest->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: OnChange
* @tc.desc: KvGeneralStore OnChange function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, OnChangeVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    MockGeneralWatcher watcherMock;
    MockStoreChanged data;
    DistributedDB::ChangedData changedData;
    storeTest->observe.OnChange(data);
    storeTest->observe.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    auto resTest = storeTest->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    storeTest->observe.OnChange(data);
    storeTest->observe.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    resTest = storeTest->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcherMock);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: Release
* @tc.desc: KvGeneralStore Release and AddRef function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, ReleaseVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    auto resTest = storeTest->Release();
    ASSERT_EQ(resTest, 0);
    storeTest = new (std::nothrow) KvGeneralStore(metaData);
    storeTest->ref_ = 0;
    resTest = storeTest->Release();
    ASSERT_EQ(resTest, 0);
    storeTest->ref_ = 2;
    resTest = storeTest->Release();
    ASSERT_EQ(resTest, 1);

    resTest = storeTest->AddRef();
    ASSERT_EQ(resTest, 2);
    storeTest->ref_ = 0;
    resTest = storeTest->AddRef();
    ASSERT_EQ(resTest, 0);
}

/**
* @tc.name: SetDistributedTables
* @tc.desc: KvGeneralStore SetDistributedTables function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, SetDistributedTablesVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::vector<std::string> tab = { "table1", "table2" };
    int32_t type = 0;
    std::vector<DistributedData::Reference> references;
    auto resTest = storeTest->SetDistributedTables(tab, type, references);
    ASSERT_EQ(resTest, GeneralError::E_ALREADY_CLOSED);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->SetDistributedTables(tab, type, references);
    ASSERT_EQ(resTest, GeneralError::E_OK);

    std::vector<std::string> test = { "testvfnhrjsgwag" };
    resTest = storeTest->SetDistributedTables(test, type, references);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
    g_testRes = true;
    resTest = storeTest->SetDistributedTables(tab, type, references);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
}

/**
* @tc.name: SetTrackTable
* @tc.desc: KvGeneralStore SetTrackTable function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, SetTrackTableVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string tabName = "tabName";
    std::set<std::string> trackerColNames = { "col1", "col2" };
    std::set<std::string> extendColNames = { "extendColName1", "extendColName2" };
    auto resTest = storeTest->SetTrackTable(tabName, trackerColNames, extendColNames);
    ASSERT_EQ(resTest, GeneralError::E_ALREADY_CLOSED);

    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    resTest = storeTest->SetTrackTable(tabName, trackerColNames, extendColNames);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->SetTrackTable("WITH_INVENTORY_DATA", trackerColNames, extendColNames);
    ASSERT_EQ(resTest, GeneralError::E_WITH_INVENTORY_DATA);
    resTest = storeTest->SetTrackTable("testvfnhrjsgwag", trackerColNames, extendColNames);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
}

/**
* @tc.name: RemoteQueryTest
* @tc.desc: KvGeneralStore RemoteQueryTest function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, RemoteQueryTestVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string device = "device";
    DistributedDB::RemoteCondition remoteCondition;
    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    auto resTest = storeTest->RemoteQueryTest("testvfnhrjsgwag", remoteCondition);
    ASSERT_EQ(resTest, nullptr);
    resTest = storeTest->RemoteQueryTest(device, remoteCondition);
    EXPECT_NE(resTest, nullptr);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: KvGeneralStore ConvertStatus function test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, ConvertStatusVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    auto resTest = storeTest->ConvertStatus(DBStatusTest::OK);
    ASSERT_EQ(resTest, GeneralError::E_OK);
    resTest = storeTest->ConvertStatus(DBStatusTest::CLOUD_NETWORK_ERROR);
    ASSERT_EQ(resTest, GeneralError::E_NETWORK_ERROR);
    resTest = storeTest->ConvertStatus(DBStatusTest::CLOUD_LOCK_ERROR);
    ASSERT_EQ(resTest, GeneralError::E_LOCKED_BY_OTHERS);
    resTest = storeTest->ConvertStatus(DBStatusTest::CLOUD_FULL_RECORDS);
    ASSERT_EQ(resTest, GeneralError::E_RECODE_LIMIT_EXCEEDED);
    resTest = storeTest->ConvertStatus(DBStatusTest::CLOUD_ASSET_SPACE_INSUFFICIENT);
    ASSERT_EQ(resTest, GeneralError::E_NO_SPACE_FOR_ASSET);
    resTest = storeTest->ConvertStatus(DBStatusTest::BUSY);
    ASSERT_EQ(resTest, GeneralError::E_BUSY);
    resTest = storeTest->ConvertStatus(DBStatusTest::DB_ERROR);
    ASSERT_EQ(resTest, GeneralError::E_ERROR);
    resTest = storeTest->ConvertStatus(DBStatusTest::CLOUD_DISABLED);
    ASSERT_EQ(resTest, GeneralError::E_CLOUD_DISABLED);
}

/**
* @tc.name: QuerySql
* @tc.desc: KvGeneralStore QuerySql function test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, QuerySqlVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    MockKvStoreDelegate kvDelegate;
    storeTest->dele = &kvDelegate;
    auto [error1, result1] = storeTest->QuerySql("", std::move(g_KvValues));
    ASSERT_EQ(error1, GeneralError::E_ERROR);
    EXPECT_TRUE(result1.empty());

    auto [error2, result2] = storeTest->QuerySql("sql", std::move(g_KvValues));
    ASSERT_EQ(error1, GeneralError::E_ERROR);
    EXPECT_TRUE(result2.empty());
}

/**
* @tc.name: BuildSqlWhenCloumnEmpty
* @tc.desc: test buildsql method when cloumn empty
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvGeneralStoreTest, BuildSqlWhenCloumnEmptyVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "mock_table";
    std::string state = "mock_statement";
    std::vector<std::string> col;
    std::string expSql = "select cloud_gid from naturalbase_kv_aux_mock_table_log, (select rowid from "
                            "mock_tablemock_statement) where data_key = rowid";
    std::string resultSql = storeTest->BuildSql(table, state, col);
    ASSERT_EQ(resultSql, expSql);
}

/**
* @tc.name: BuildSqlWhenParamValid
* @tc.desc: test buildsql method when param valid
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, BuildSqlWhenParamValidVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    std::string table = "mock_table";
    std::string state = "mock_statement";
    std::vector<std::string> col;
    col.push_back("mock_column_1");
    col.push_back("mock_column_2");
    std::string expSql = "select cloud_gid, mock_column_1, mock_column_2 from naturalbase_kv_aux_mock_table_log, "
                            "(select rowid, mock_column_1, mock_column_2 from mock_tablemock_statement) where "
                            "data_key = rowid";
    std::string resultSql = storeTest->BuildSql(table, state, col);
    ASSERT_EQ(resultSql, expSql);
}

/**
* @tc.name: LockAndUnLockVirtualDBTest
* @tc.desc: lock and unlock cloudDB test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, LockAndUnLockVirtualDBTestVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    auto resTest = storeTest->LockVirtualDB();
    ASSERT_EQ(resTest.first, 1);
    ASSERT_EQ(resTest.second, 0);
    auto unlockRes = storeTest->UnLockVirtualDB();
    ASSERT_EQ(unlockRes, 1);
}

/**
* @tc.name: InFinishedTest
* @tc.desc: isFinished test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, InFinishedTestVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedKv::KvGeneralStore::SyncId syncId = 1;
    bool isFinished = storeTest->IsFinished(syncId);
    EXPECT_TRUE(isFinished);
}

/**
* @tc.name: GetKvVirtualTest
* @tc.desc: getKvVirtual test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, GetKvVirtualTestVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    auto kvVirtual = storeTest->GetKvVirtual();
    ASSERT_EQ(kvVirtual, nullptr);
}

/**
* @tc.name: RegisterDetailProgressObserverTest
* @tc.desc: RegisterDetailProgressObserver test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, RegisterDetailProgressObserverTestVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedData::GeneralStore::DetailAsync async;
    auto resTest = storeTest->RegisterDetailProgressObserver(async);
    ASSERT_EQ(resTest, GeneralError::E_OK);
}

/**
* @tc.name: GetFinishTaskTest
* @tc.desc: GetFinishTask test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, GetFinishTaskTestVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedKv::KvGeneralStore::SyncId syncId = 1;
    auto resTest = storeTest->GetFinishTask(syncId);
    EXPECT_NE(resTest, nullptr);
}

/**
* @tc.name: GetCBTest
* @tc.desc: GetCB test
* @tc.type: FUNC
*/
HWTEST_F(KvGeneralStoreTest, GetCBTestVirtual, TestSize.Level1)
{
    auto storeTest = new (std::nothrow) KvGeneralStore(metaData);
    EXPECT_NE(storeTest, nullptr);
    DistributedKv::KvGeneralStore::SyncId syncId = 1;
    auto resTest = storeTest->GetCB(syncId);
    EXPECT_NE(resTest, nullptr);
}
} // namespace DistributedKVTest
} // namespace OHOS::Test