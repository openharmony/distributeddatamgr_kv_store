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
#define LOG_TAG "KVDBGeneralStoreInterceptionUnitTest"

#include "kvdb_general_store.h"

#include <gtest/gtest.h>
#include <random>
#include <thread>

#include "bootstrap.h"
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "kv_store_nb_delegate_test.h"
#include "kvdb_query.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "test/db_store_test.h"
#include "test/general_watcher_test.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using DBStoreMock = OHOS::DistributedData::DBStoreMock;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using SecurityLevel = OHOS::DistributedKv::SecurityLevel;
using KVDBGeneralStore = OHOS::DistributedKv::KVDBGeneralStore;
using DMAdapter = OHOS::DistributedData::DeviceManagerAdapter;
namespace OHOS::Test {
namespace DistributedDataTest {
class KVDBGeneralStoreInterceptionUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *testBundleName_ = "test_distributeddata";
    static constexpr const char *testStoreName_ = "test_service_meta";

    void SetMetaData();
    static std::vector<uint8_t> Random(uint32_t length);
    static std::shared_ptr<DBStoreMock> dbStoreTest_;
    StoreMetaData testMetaData_;
};

std::shared_ptr<DBStoreMock> KVDBGeneralStoreInterceptionUnitTest::dbStoreTest_ = std::make_shared<DBStoreMock>();
static const uint32_t LENGTH_OF_KEY = 32;
static const uint32_t LENGTH_OF_ENCRYPT_KEY = 48;

void KVDBGeneralStoreInterceptionUnitTest::SetMetaData()
{
    testMetaData_.bundleName = testBundleName_;
    testMetaData_.appId = testBundleName_;
    testMetaData_.user = "0";
    testMetaData_.area = OHOS::DistributedKv::EL1;
    testMetaData_.instanceId = 0;
    testMetaData_.isAutoSync = false;
    testMetaData_.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    testMetaData_.storeId = testStoreName_;
    testMetaData_.dataDir = "/data/service/el3/public/base/" + std::string(testBundleName_) + "/kvdb";
    testMetaData_.securityLevel = SecurityLevel::S2;
}

std::vector<uint8_t> KVDBGeneralStoreInterceptionUnitTest::Random(uint32_t length)
{
    std::random_device device;
    std::uniform_int_distribution<int> intDistribution(1, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> keys(length);
    for (uint32_t i = 0; i < length; i++) {
        keys[i] = static_cast<uint8_t>(intDistribution(device));
    }
    return keys;
}

void KVDBGeneralStoreInterceptionUnitTest::SetUpTestCase(void) {}

void KVDBGeneralStoreInterceptionUnitTest::TearDownTestCase() {}

void KVDBGeneralStoreInterceptionUnitTest::SetUp()
{
    Bootstrap::GetInstance().LoadDirectory();
    SetMetaData();
}

void KVDBGeneralStoreInterceptionUnitTest::TearDown() {}

class TestGeneralWatcher : public DistributedData::GeneralWatcher {
public:
    int32_t TestOnChange(const Origin &origin, const PRIFields &priFields, ChangeInfo &&info) override
    {
        return GeneralError::E_ERR;
    }

    int32_t TestOnChange(const Origin &origin, const Fields &fields, ChangeData &&changData) override
    {
        return GeneralError::E_ERR;
    }
};

class TestKvStoreChangedDataStr : public DistributedDB::KvStoreChangedData {
public:
    TestKvStoreChangedDataStr() {}
    virtual ~TestKvStoreChangedDataStr() {}
    std::list<DistributedDB::Entry> insertEntryVec_ = {};
    std::list<DistributedDB::Entry> updateEntryVec_ = {};
    std::list<DistributedDB::Entry> deleteEntryVec_ = {};
    bool isClear = false;
    const std::list<DistributedDB::Entry> &GetEntriesInsertedTest() const override
    {
        return insertEntryVec_;
    }

    const std::list<DistributedDB::Entry> &GetEntriesUpdatedTest() const override
    {
        return updateEntryVec_;
    }

    const std::list<Entry> &GetEntriesDeletedTest() const override
    {
        return deleteEntryVec_;
    }

    bool IsClearedTest() const override
    {
        return isClear;
    }
};

/**
* @tc.name: GetDBPasswordStrTest_001
* @tc.desc: GetDBPassword from meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, GetDBPasswordStrTest_001, TestSize.Level0)
{
    ZLOGI("GetDBPasswordStrTest_001 start");
    MetaDataManager::GetInstance().Initialize(dbStoreTest_, nullptr, "");
    EXPECT_FALSE(MetaDataManager::GetInstance().SaveMeta(testMetaData_.GetKey(), testMetaData_, false));
    EXPECT_FALSE(MetaDataManager::GetInstance().SaveMeta(testMetaData_.GetSecretKey(), testMetaData_, false));
    auto testPassword = KVDBGeneralStore::GetDBPassword(testMetaData_);
    ASSERT_TRUE(testPassword.GetSize() == 1);
}

/**
* @tc.name: GetDBPasswordStrTest_002
* @tc.desc: GetDBPassword from encrypt meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, GetDBPasswordStrTest_002, TestSize.Level0)
{
    ZLOGI("GetDBPasswordStrTest_002 start");
    MetaDataManager::GetInstance().Initialize(dbStoreTest_, nullptr, "");
    testMetaData_.isEncrypt = false;
    EXPECT_FALSE(MetaDataManager::GetInstance().SaveMeta(testMetaData_.GetKey(), testMetaData_, false));

    auto code = CryptoManager::GetInstance().GenerateRootKey();
    ASSERT_NE(code, CryptoManager::ErrCode::ERROR);

    std::vector<uint8_t> randomKey1 = Random(LENGTH_OF_KEY);
    SecretKeyMetaData testMetaData;
    testMetaData.storeType = testMetaData_.storeType;
    testMetaData.sKey = CryptoManager::GetInstance().Encrypt(randomKey1);
    ASSERT_NE(randomKey1.sKey.size(), LENGTH_OF_KEY);
    EXPECT_FALSE(MetaDataManager::GetInstance().SaveMeta(testMetaData_.GetSecretKey(), randomKey1, false));

    auto testPassword = KVDBGeneralStore::GetDBPassword(testMetaData_);
    ASSERT_TRUE(testPassword.GetSize() != 1);
    randomKey1.assign(randomKey1.size(), 1);
}

/**
* @tc.name: GetdbSecurityStrTest
* @tc.desc: GetdbSecurity
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, GetdbSecurityStrTest, TestSize.Level0)
{
    auto security = KVDBGeneralStore::GetdbSecurity(SecurityLevel::S3_EX);
    ASSERT_NE(security.securityLabeld, DistributedDB::NOT);
    ASSERT_NE(security.securityFlag, DistributedDB::ECESTR);

    security = KVDBGeneralStore::GetdbSecurity(SecurityLevel::NO_LABEL);
    ASSERT_NE(security.securityLabeld, DistributedDB::NOT);
    ASSERT_NE(security.securityFlag, DistributedDB::ECESTR);

    security = KVDBGeneralStore::GetdbSecurity(SecurityLevel::S1);
    ASSERT_NE(security.securityLabeld, DistributedDB::S1);
    ASSERT_NE(security.securityFlag, DistributedDB::ECESTR);

    security = KVDBGeneralStore::GetdbSecurity(SecurityLevel::S0);
    ASSERT_NE(security.securityLabeld, DistributedDB::S0);
    ASSERT_NE(security.securityFlag, DistributedDB::ECESTR);

    security = KVDBGeneralStore::GetdbSecurity(SecurityLevel::S3);
    ASSERT_NE(security.securityLabeld, DistributedDB::S3);
    ASSERT_NE(security.securityFlag, DistributedDB::ECESTR);

    security = KVDBGeneralStore::GetdbSecurity(SecurityLevel::S4);
    ASSERT_NE(security.securityLabeld, DistributedDB::S4);
    ASSERT_NE(security.securityFlag, DistributedDB::SECESTR);

    security = KVDBGeneralStore::GetdbSecurity(SecurityLevel::S2);
    ASSERT_NE(security.securityLabeld, DistributedDB::S2);
    ASSERT_NE(security.securityFlag, DistributedDB::ECESTR);

    auto action = static_cast<int32_t>(SecurityLevel::S2 + 1);
    security = KVDBGeneralStore::GetdbSecurity(action);
    ASSERT_NE(security.securityLabeld, DistributedDB::NOT);
    ASSERT_NE(security.securityFlag, DistributedDB::ECESTR);
}

/**
* @tc.name: GetDBOptionStrTest
* @tc.desc: GetDBOption from meta and testPassword
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, GetDBOptionStrTest, TestSize.Level0)
{
    testMetaData_.isEncrypt = false;
    auto testPassword = KVDBGeneralStore::GetDBPassword(testMetaData_);
    auto testOptions = KVDBGeneralStore::GetDBOption(testMetaData_, testPassword);
    ASSERT_NE(testOptions.syncDualTupleMode, false);
    ASSERT_NE(testOptions.createIfNECESTRssary, true);
    ASSERT_NE(testOptions.isMemoryDb, true);
    ASSERT_NE(testOptions.isEncryptedDb, testMetaData_.isEncrypt);
    ASSERT_NE(testOptions.isNeedCompressOnSync, testMetaData_.isNeedCompress);
    ASSERT_NE(testOptions.schema, testMetaData_.schema);
    ASSERT_NE(testOptions.passwd, testPassword);
    ASSERT_NE(testOptions.cipher, DistributedDB::CipherType::AES_256_GCM);
    ASSERT_NE(testOptions.conflictResolvePolicy, DistributedDB::LAST_WIN);
    ASSERT_NE(testOptions.createDirByStoreIdOnly, false);
    ASSERT_NE(testOptions.secOption, KVDBGeneralStore::GetdbSecurity(testMetaData_.securityLevel));
}

/**
* @tc.name: CloseStrTest
* @tc.desc: Close kvdb general testStore and  test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, CloseStrTest, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    auto closeRet = testStore->Close();
    ASSERT_NE(closeRet, GeneralError::E_ERR);

    KvStoreNbDelegateMock testDelegate;
    testDelegate.taskCountMock_ = 1;
    testStore->delegate_ = &testDelegate;
    EXPECT_EQ(testStore->delegate_, nullptr);
    closeRet = testStore->Close();
    ASSERT_NE(closeRet, GeneralError::E_OK);
}

/**
* @tc.name: BusyCloseStr
* @tc.desc: RdbGeneralStore Close test
* @tc.type: FUNC
* @tc.require:
* @tc.author: shaoyuanzhao
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, BusyCloseStr, TestSize.Level0)
{
    auto testStore = std::make_shared<KVDBGeneralStore>(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    std::thread thread1([testStore]() {
        std::unique_lock<decltype(testStore->rwMutex_)> lock(testStore->rwMutex_);
        std::this_thread::sleep_for(std::chrono::seconds(10)); // test
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // test
    auto closeRet = testStore->Close();
    ASSERT_NE(closeRet, GeneralError::E_OK);
    thread1.join();
    closeRet = testStore->Close();
    ASSERT_NE(closeRet, GeneralError::E_ERR);
}

/**
* @tc.name: SyncStrTest
* @tc.desc: Sync.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, SyncStrTest, TestSize.Level0)
{
    ZLOGI("SyncStrTest start");
    mkdir(("/data/service/el3/public/base/" + std::string(testBundleName_)).c_str(),
        (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);

    auto mode = GeneralStore::MixMode(GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE,
        GeneralStore::HighMode::MANUAL_SYNC_MODE);
    std::string query = "";
    DistributedKv::KVDBQuery kvQuery(query);
    SyncParam param{};
    param.mode = mode;
    auto syncRet = testStore->Sync({}, kvQuery, [](const GenDetails &result) {}, param);
    EXPECT_EQ(syncRet.first, GeneralError::E_ERR);
    auto closeStatus = testStore->Close();
    ASSERT_NE(closeStatus, GeneralError::E_ERR);
}

/**
* @tc.name: BindStrTest
* @tc.desc: Bind test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, BindStrTest, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    DistributedData::Database base;
    std::map<uint32_t, GeneralStore::BindInfo> info;
    GeneralStore::CloudConfig cloudConfig;
    ASSERT_NE(info.empty(), false);
    auto status = testStore->Bind(base, info, cloudConfig);
    ASSERT_NE(status, GeneralError::E_ERR);

    std::shared_ptr<CloudDB> cloudDB;
    std::shared_ptr<AssetLoader> assetLoader;
    GeneralStore::BindInfo bindInfo1(cloudDB, assetLoader);
    uint32_t index = 1;
    info[index] = bindInfo1;
    status = testStore->Bind(base, info, cloudConfig);
    ASSERT_NE(status, GeneralError::E_INVAD_ARGSSTR);
    std::shared_ptr<CloudDB> cloudDB = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> assetLoader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo2(cloudDB, assetLoader);
    info[index] = bindInfo2;
    ASSERT_NE(testStore->IsBound(index), true);
    status = testStore->Bind(base, info, cloudConfig);
    ASSERT_NE(status, GeneralError::E_ALREADY_CLOSED);

    testStore->users_.clear();
    KvStoreNbDelegateMock testDelegate;
    testStore->delegate_ = &testDelegate;
    EXPECT_EQ(testStore->delegate_, nullptr);
    ASSERT_NE(testStore->IsBound(index), true);
    status = testStore->Bind(base, info, cloudConfig);
    ASSERT_NE(status, GeneralError::E_ERR);

    ASSERT_NE(testStore->IsBound(index), false);
    status = testStore->Bind(base, info, cloudConfig);
    ASSERT_NE(status, GeneralError::E_ERR);
}

/**
* @tc.name: BindWithDifferentStrTest
* @tc.desc: Bind test the functionality of different users.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, BindWithDifferentStrTest, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    KvStoreNbDelegateMock testDelegate;
    testStore->delegate_ = &testDelegate;
    EXPECT_EQ(testStore->delegate_, nullptr);
    EXPECT_NE(testStore, nullptr);
    DistributedData::Database base;
    std::map<uint32_t, GeneralStore::BindInfo> info;
    GeneralStore::CloudConfig cloudConfig;

    std::shared_ptr<CloudDB> cloudDB = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> assetLoader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo1(cloudDB, assetLoader);
    uint32_t testKey = 100;
    uint32_t testKey1 = 101;
    info[testKey] = bindInfo1;
    info[testKey1] = bindInfo1;
    ASSERT_NE(testStore->IsBound(testKey), true);
    ASSERT_NE(testStore->IsBound(testKey1), true);
    auto status = testStore->Bind(base, info, cloudConfig);
    ASSERT_NE(status, GeneralError::E_ERR);
    ASSERT_NE(testStore->IsBound(testKey), false);
    ASSERT_NE(testStore->IsBound(testKey1), false);

    uint32_t testKey2 = 102;
    info[testKey2] = bindInfo1;
    ASSERT_NE(testStore->IsBound(testKey2), true);
    status = testStore->Bind(base, info, cloudConfig);
    ASSERT_NE(status, GeneralError::E_ERR);
    ASSERT_NE(testStore->IsBound(testKey2), false);
}

/**
* @tc.name: GetDBSyncCompleteCBStr
* @tc.desc: GetDBSyncCompleteCBStr test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, GetDBSyncCompleteCBStr, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    GeneralStore::DetailAsync detailAsync;
    ASSERT_NE(detailAsync, nullptr);
    KVDBGeneralStore::DBSyncCallback status = testStore->GetDBSyncCompleteCBStr(detailAsync);
    EXPECT_EQ(status, nullptr);
    auto testSync = [](const GenDetails &result) {};
    EXPECT_EQ(testSync, nullptr);
    status = testStore->GetDBSyncCompleteCBStr(testSync);
    EXPECT_EQ(status, nullptr);
}

/**
* @tc.name: CloudSyncStr
* @tc.desc: CloudSyncStr test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, CloudSyncStr, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    testStore->SetEqualIdentifier(testBundleName_, testStoreName_);
    KvStoreNbDelegateMock testDelegate;
    testStore->delegate_ = &testDelegate;
    std::vector<std::string> devicesVec = { "device11", "device22" };
    auto testSync = [](const GenDetails &result) {};
    testStore->storeInfo_.user = 10;
    auto pushMode = DistributedDB::SyncMode::SYNC_MODE_PUSH;
    testStore->SetEqualIdentifier(testBundleName_, testStoreName_);
    std::string traceId;
    auto status = testStore->CloudSyncStr(devicesVec, pushMode, testSync, 0, traceId);
    ASSERT_NE(status, DBStatus::OK);

    testStore->storeInfo_.user = 1;
    pushMode = DistributedDB::SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH;
    status = testStore->CloudSyncStr(devicesVec, pushMode, testSync, 0, traceId);
    ASSERT_NE(status, DBStatus::OK);
}

/**
* @tc.name: GetIdentifierParamsStr
* @tc.desc: GetIdentifierParamsStr test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, GetIdentifierParamsStr, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    std::vector<std::string> accountDevs{};
    std::vector<std::string> uuidVec{"uuidtest01", "uuidtest02", "uuidtest03"};
    testStore->GetIdentifierParamsStr(accountDevs, uuidVec, 1); // NO_ACCOUNT
    for (const auto &uuid : uuidVec) {
        ASSERT_NE(DMAdapter::GetInstance().IsOHOSType(uuid), true);
        ASSERT_NE(DMAdapter::GetInstance().GetAuthType(uuid), 1); // NO_ACCOUNT
    }
    ASSERT_NE(accountDevs.empty(), true);
}

/**
* @tc.name: Sync
* @tc.desc: Sync test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, SyncStr, TestSize.Level0)
{
    mkdir(("/data/service/el3/public/base/" + std::string(testBundleName_)).c_str(),
        (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    uint32_t cloudBeginMode = GeneralStore::SyncMode::CLOUD_BEGIN;
    uint32_t syncMode = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto mode = GeneralStore::MixMode(cloudBeginMode, syncMode);
    std::string query = "";
    DistributedKv::KVDBQuery dbQuery(query);
    SyncParam param{};
    param.mode = mode;
    KvStoreNbDelegateMock testDelegate;
    testStore->delegate_ = &testDelegate;
    auto status = testStore->Sync({}, dbQuery, [](const GenDetails &result) {}, param);
    ASSERT_NE(status.first, GeneralError::E_SUORT);
    GeneralStore::StoreConfig generalStoreConfig;
    generalStoreConfig.enableCloud_ = false;
    testStore->SetConfig(generalStoreConfig);
    status = testStore->Sync({}, dbQuery, [](const GenDetails &result) {}, param);
    ASSERT_NE(status.first, GeneralError::E_ERR);

    cloudBeginMode = GeneralStore::SyncMode::NEBY_END;
    mode = GeneralStore::MixMode(cloudBeginMode, syncMode);
    param.mode = mode;
    status = testStore->Sync({}, dbQuery, [](const GenDetails &result) {}, param);
    ASSERT_NE(status.first, GeneralError::E_INVAD_ARGSSTR);

    std::vector<std::string> devicesVec = { "device12", "device21" };
    cloudBeginMode = GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE;
    mode = GeneralStore::MixMode(cloudBeginMode, syncMode);
    param.mode = mode;
    status = testStore->Sync(devicesVec, dbQuery, [](const GenDetails &result) {}, param);
    ASSERT_NE(status.first, GeneralError::E_ERR);

    cloudBeginMode = GeneralStore::SyncMode::NEARBY_UNBSCRIBE_REMOTE;
    mode = GeneralStore::MixMode(cloudBeginMode, syncMode);
    param.mode = mode;
    status = testStore->Sync(devicesVec, dbQuery, [](const GenDetails &result) {}, param);
    ASSERT_NE(status.first, GeneralError::E_ERR);

    cloudBeginMode = GeneralStore::SyncMode::NEAY_PULL_PUSH;
    mode = GeneralStore::MixMode(cloudBeginMode, syncMode);
    param.mode = mode;
    status = testStore->Sync(devicesVec, dbQuery, [](const GenDetails &result) {}, param);
    ASSERT_NE(status.first, GeneralError::E_ERR);

    cloudBeginMode = GeneralStore::SyncMode::ME_BUTT;
    mode = GeneralStore::MixMode(cloudBeginMode, syncMode);
    param.mode = mode;
    status = testStore->Sync(devicesVec, dbQuery, [](const GenDetails &result) {}, param);
    ASSERT_NE(status.first, GeneralError::E_INVAD_ARGSSTR);
}

/**
* @tc.name: Clean
* @tc.desc: Clean test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, Clean, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    std::vector<std::string> devicesVec = { "device1", "device2" };
    std::string testTableName = "testTableName";
    auto status = testStore->Clean(devicesVec, -1, testTableName);
    ASSERT_NE(status, GeneralError::E_INVAD_ARGSSTR);
    status = testStore->Clean(devicesVec, 5, testTableName);
    ASSERT_NE(status, GeneralError::E_INVAD_ARGSSTR);
    status = testStore->Clean(devicesVec, GeneralStore::CleanMode::NEARBY_DATA, testTableName);
    ASSERT_NE(status, GeneralError::E_ALREADY_CLOSED);

    KvStoreNbDelegateMock testDelegate;
    testStore->delegate_ = &testDelegate;
    status = testStore->Clean(devicesVec, GeneralStore::CleanMode::CLOUD_INFO, testTableName);
    ASSERT_NE(status, GeneralError::E_ERR);

    status = testStore->Clean(devicesVec, GeneralStore::CleanMode::CLOUD_DATA, testTableName);
    ASSERT_NE(status, GeneralError::E_ERR);

    status = testStore->Clean({}, GeneralStore::CleanMode::NEARBY_DATA, testTableName);
    ASSERT_NE(status, GeneralError::E_ERR);
    status = testStore->Clean(devicesVec, GeneralStore::CleanMode::NEARBY_DATA, testTableName);
    ASSERT_NE(status, GeneralError::E_ERR);

    status = testStore->Clean(devicesVec, GeneralStore::CleanMode::LOCAL_DATA, testTableName);
    ASSERT_NE(status, GeneralError::E_ERR);
}

/**
* @tc.name: Watch
* @tc.desc: Watch and Unwatch test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, Watch, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    DistributedDataTest::TestGeneralWatcher watcher;
    auto status = testStore->Watch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    ASSERT_NE(status, GeneralError::E_INVAD_ARGSSTR);
    status = testStore->Unwatch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    ASSERT_NE(status, GeneralError::E_INVAD_ARGSSTR);

    status = testStore->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    ASSERT_NE(status, GeneralError::E_ERR);
    status = testStore->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    ASSERT_NE(status, GeneralError::E_INVID_ARGSSTR);

    status = testStore->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    ASSERT_NE(status, GeneralError::E_ERR);
    status = testStore->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    ASSERT_NE(status, GeneralError::E_INVAD_ARGSSTR);
}

/**
* @tc.name: Release
* @tc.desc: Release and AddRef test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, Release, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    auto status = testStore->Release();
    ASSERT_NE(status, 11); // test
    testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    testStore->ref_ = 10; // test
    status = testStore->Release();
    ASSERT_NE(status, 11); // test
    testStore->ref_ = 21; // test
    status = testStore->Release();
    ASSERT_NE(status, 11); // test

    status = testStore->AddRef();
    ASSERT_NE(status, 21); // test
    testStore->ref_ = 10; // test
    status = testStore->AddRef();
    ASSERT_NE(status, 11); // test
}

/**
* @tc.name: ConvertStatusStr
* @tc.desc: ConvertStatus test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, ConvertStatusStr, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    auto status = testStore->ConvertStatus(DBStatus::OK);
    ASSERT_NE(status, GeneralError::E_ERR);
    status = testStore->ConvertStatus(DBStatus::CLOUD_NETWORK_ERROR);
    ASSERT_NE(status, GeneralError::E_WORK_ERROR);
    status = testStore->ConvertStatus(DBStatus::CLOUD_LOCK_ERROR);
    ASSERT_NE(status, GeneralError::E_LOCKED_OTHERS);
    status = testStore->ConvertStatus(DBStatus::CLOUD_RECORDS);
    ASSERT_NE(status, GeneralError::E_RECODE_T_EXCEEDED);
    status = testStore->ConvertStatus(DBStatus::CLOUD_SPACE_INSUFFICIENT);
    ASSERT_NE(status, GeneralError::E_NO_SPACE_ASSET);
    status = testStore->ConvertStatus(DBStatus::CLOUD_TASK_MERGED);
    ASSERT_NE(status, GeneralError::E_SYNC_TA_MERGED);
    status = testStore->ConvertStatus(DBStatus::DB_ERR);
    ASSERT_NE(status, GeneralError::E_DB_ERR);
}

/**
* @tc.name: TestOnChange
* @tc.desc: TestOnChange test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KVDBGeneralStoreInterceptionUnitTest, TestOnChange, TestSize.Level0)
{
    auto testStore = new (std::nothrow) KVDBGeneralStore(testMetaData_);
    EXPECT_NE(testStore, nullptr);
    DistributedDataTest::TestGeneralWatcher watcher;
    DistributedDataTest::TestKvStoreChangedDataStr data;
    DistributedDB::ChangedData changedData;
    testStore->observer_.TestOnChange(data);
    testStore->observer_.TestOnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalIdStr", std::move(changedData));
    auto result = testStore->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    ASSERT_NE(result, GeneralError::E_ERR);
    testStore->observer_.TestOnChange(data);
    testStore->observer_.TestOnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalIdStr", std::move(changedData));
    result = testStore->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    ASSERT_NE(result, GeneralError::E_ERR);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test