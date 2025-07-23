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
#include <gtest/gtest.h>
#include "kvdb_service_client.h"
#include "distributeddata_kvdb_ipc_interface_code.h"
#include "kvstore_observer_client.h"
#include "kvstore_service_death_notifier.h"
#include "mock_remote_object.h"
#include "mock_kvstore_data_service.h"
#include "mock_kvdb_notifier.h"
#include "mock_kvstore_observer.h"

using namespace testing;
using namespace OHOS::DistributedKv;

namespace {
class KVDBServiceClientTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        mockRemote_ = new MockRemoteObject();
        mockService_ = new MockKvStoreDataService();
        mockNotifier_ = new MockKVDBNotifier();
        mockObserver_ = new MockKvStoreObserver();
    }

    static void TearDownTestCase()
    {
        mockRemote_ = nullptr;
        mockService_ = nullptr;
        mockNotifier_ = nullptr;
        mockObserver_ = nullptr;
    }

    void SetUp() override
    {
        KVDBServiceClient::GetInstance().reset();
        KVDBServiceClient::isWatched_.store(false);
    }

    void TearDown() override
    {
        Mock::VerifyAndClearExpectations(mockRemote_.GetRefPtr());
        Mock::VerifyAndClearExpectations(mockService_.GetRefPtr());
        Mock::VerifyAndClearExpectations(mockNotifier_.GetRefPtr());
        Mock::VerifyAndClearExpectations(mockObserver_.GetRefPtr());
    }

    static sptr<MockRemoteObject> mockRemote_;
    static sptr<MockKvStoreDataService> mockService_;
    static sptr<MockKVDBNotifier> mockNotifier_;
    static sptr<MockKvStoreObserver> mockObserver_;
};

sptr<MockRemoteObject> KVDBServiceClientTest::mockRemote_ = nullptr;
sptr<MockKvStoreDataService> KVDBServiceClientTest::mockService_ = nullptr;
sptr<MockKVDBNotifier> KVDBServiceClientTest::mockNotifier_ = nullptr;
sptr<MockKvStoreObserver> KVDBServiceClientTest::mockObserver_ = nullptr;

// Helper constants
const AppId TEST_APP_ID = {"test_app"};
const StoreId TEST_STORE_ID = {"test_store"};
const Options TEST_OPTIONS = {true, false, true};
const std::vector<uint8_t> TEST_PASSWORD = {1, 2, 3, 4};
const SyncInfo TEST_SYNC_INFO = {12345, SyncMode::PUSH, {"device1", "device2"}, 1000, "test_query"};
const KvSyncParam TEST_SYNC_PARAM = {5000};
const std::vector<std::string> TEST_CAPABILITIES = {"cap1", "cap2"};
const SwitchData TEST_SWITCH_DATA = {true, false, true};
const StoreConfig TEST_STORE_CONFIG = {"config1", "value1"};
const std::string TEST_DEVICE = "test_device";
const int32_t TEST_SUB_USER = 0;
const int32_t TEST_PASSWORD_TYPE = 1;
} // namespace

// GetInstance tests
TEST_F(KVDBServiceClientTest, GetInstanceReturnsNullWhenServiceNotAvailable)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(nullptr));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto instance = KVDBServiceClient::GetInstance();
    EXPECT_EQ(instance, nullptr);
}

TEST_F(KVDBServiceClientTest, GetInstanceReturnsValidInstanceWhenServiceAvailable)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto instance = KVDBServiceClient::GetInstance();
    EXPECT_NE(instance, nullptr);
}

TEST_F(KVDBServiceClientTest, GetInstanceReturnsSameInstanceOnSubsequentCalls)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto instance1 = KVDBServiceClient::GetInstance();
    auto instance2 = KVDBServiceClient::GetInstance();
    EXPECT_EQ(instance1, instance2);
}

TEST_F(KVDBServiceClientTest, GetInstanceResetsInstanceAfterServiceDeath)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto instance1 = KVDBServiceClient::GetInstance();
    KVDBServiceClient::ServiceDeath death;
    death.OnRemoteDied();
    
    auto instance2 = KVDBServiceClient::GetInstance();
    EXPECT_NE(instance1, instance2);
}

// GetStoreIds tests
TEST_F(KVDBServiceClientTest, GetStoreIdsReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    std::vector<StoreId> storeIds;
    auto status = client->GetStoreIds(TEST_APP_ID, TEST_SUB_USER, storeIds);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, GetStoreIdsReturnsErrorForInvalidAppId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    std::vector<StoreId> storeIds;
    auto status = client->GetStoreIds({"invalid_app"}, TEST_SUB_USER, storeIds);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, GetStoreIdsReturnsEmptyListWhenNoStoresExist)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    std::vector<StoreId> storeIds;
    auto status = client->GetStoreIds(TEST_APP_ID, TEST_SUB_USER, storeIds);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_TRUE(storeIds.empty());
}

// BeforeCreate tests
TEST_F(KVDBServiceClientTest, BeforeCreateReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->BeforeCreate(TEST_APP_ID, TEST_STORE_ID, TEST_OPTIONS);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, BeforeCreateReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->BeforeCreate(TEST_APP_ID, {""}, TEST_OPTIONS);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, BeforeCreateReturnsErrorForInvalidOptions)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    Options invalidOptions = {false, false, false};
    auto status = client->BeforeCreate(TEST_APP_ID, TEST_STORE_ID, invalidOptions);
    EXPECT_NE(status, SUCCESS);
}

// AfterCreate tests
TEST_F(KVDBServiceClientTest, AfterCreateReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->AfterCreate(TEST_APP_ID, TEST_STORE_ID, TEST_OPTIONS, TEST_PASSWORD);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, AfterCreateReturnsErrorForEmptyPassword)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->AfterCreate(TEST_APP_ID, TEST_STORE_ID, TEST_OPTIONS, {});
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, AfterCreateReturnsErrorForInvalidOptions)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    Options invalidOptions = {false, false, false};
    auto status = client->AfterCreate(TEST_APP_ID, TEST_STORE_ID, invalidOptions, TEST_PASSWORD);
    EXPECT_NE(status, SUCCESS);
}

// Delete tests
TEST_F(KVDBServiceClientTest, DeleteReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Delete(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, DeleteReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Delete(TEST_APP_ID, {""}, TEST_SUB_USER);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, DeleteReturnsErrorForInvalidSubUser)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Delete(TEST_APP_ID, TEST_STORE_ID, -1);
    EXPECT_NE(status, SUCCESS);
}

// Close tests
TEST_F(KVDBServiceClientTest, CloseReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Close(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, CloseReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Close(TEST_APP_ID, {""}, TEST_SUB_USER);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, CloseReturnsErrorForInvalidSubUser)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Close(TEST_APP_ID, TEST_STORE_ID, -1);
    EXPECT_NE(status, SUCCESS);
}

// Sync tests
TEST_F(KVDBServiceClientTest, SyncReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    auto status = client->Sync(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SyncReturnsErrorForEmptyDevices)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    syncInfo.devices.clear();
    auto status = client->Sync(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SyncReturnsErrorForInvalidMode)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    syncInfo.mode = static_cast<SyncMode>(999);
    auto status = client->Sync(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

// CloudSync tests
TEST_F(KVDBServiceClientTest, CloudSyncReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    auto status = client->CloudSync(TEST_APP_ID, TEST_STORE_ID, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, CloudSyncReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    auto status = client->CloudSync(TEST_APP_ID, {""}, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, CloudSyncReturnsErrorForZeroSeqId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    syncInfo.seqId = 0;
    auto status = client->CloudSync(TEST_APP_ID, TEST_STORE_ID, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

// NotifyDataChange tests
TEST_F(KVDBServiceClientTest, NotifyDataChangeReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->NotifyDataChange(TEST_APP_ID, TEST_STORE_ID, 1000);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, NotifyDataChangeReturnsErrorForZeroDelay)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->NotifyDataChange(TEST_APP_ID, TEST_STORE_ID, 0);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, NotifyDataChangeReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->NotifyDataChange(TEST_APP_ID, {""}, 1000);
    EXPECT_NE(status, SUCCESS);
}

// RegServiceNotifier tests
TEST_F(KVDBServiceClientTest, RegServiceNotifierReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->RegServiceNotifier(TEST_APP_ID, mockNotifier_);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, RegServiceNotifierReturnsErrorForNullNotifier)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->RegServiceNotifier(TEST_APP_ID, nullptr);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, RegServiceNotifierReturnsErrorForInvalidAppId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->RegServiceNotifier({""}, mockNotifier_);
    EXPECT_NE(status, SUCCESS);
}

// UnregServiceNotifier tests
TEST_F(KVDBServiceClientTest, UnregServiceNotifierReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->UnregServiceNotifier(TEST_APP_ID);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, UnregServiceNotifierReturnsErrorForInvalidAppId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->UnregServiceNotifier({""});
    EXPECT_NE(status, SUCCESS);
}

// SetSyncParam tests
TEST_F(KVDBServiceClientTest, SetSyncParamReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->SetSyncParam(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, TEST_SYNC_PARAM);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SetSyncParamReturnsErrorForInvalidDelay)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    KvSyncParam invalidParam = {0};
    auto status = client->SetSyncParam(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, invalidParam);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SetSyncParamReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->SetSyncParam(TEST_APP_ID, {""}, TEST_SUB_USER, TEST_SYNC_PARAM);
    EXPECT_NE(status, SUCCESS);
}

// GetSyncParam tests
TEST_F(KVDBServiceClientTest, GetSyncParamReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    KvSyncParam syncParam;
    auto status = client->GetSyncParam(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncParam);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, GetSyncParamReturnsDefaultValuesWhenNotSet)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    KvSyncParam syncParam;
    auto status = client->GetSyncParam(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncParam);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(syncParam.allowedDelayMs, 0);
}

TEST_F(KVDBServiceClientTest, GetSyncParamReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    KvSyncParam syncParam;
    auto status = client->GetSyncParam(TEST_APP_ID, {""}, TEST_SUB_USER, syncParam);
    EXPECT_NE(status, SUCCESS);
}

// EnableCapability tests
TEST_F(KVDBServiceClientTest, EnableCapabilityReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->EnableCapability(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, EnableCapabilityReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->EnableCapability(TEST_APP_ID, {""}, TEST_SUB_USER);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, EnableCapabilityReturnsErrorForInvalidSubUser)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->EnableCapability(TEST_APP_ID, TEST_STORE_ID, -1);
    EXPECT_NE(status, SUCCESS);
}

// DisableCapability tests
TEST_F(KVDBServiceClientTest, DisableCapabilityReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->DisableCapability(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, DisableCapabilityReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->DisableCapability(TEST_APP_ID, {""}, TEST_SUB_USER);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, DisableCapabilityReturnsErrorForInvalidSubUser)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->DisableCapability(TEST_APP_ID, TEST_STORE_ID, -1);
    EXPECT_NE(status, SUCCESS);
}

// SetCapability tests
TEST_F(KVDBServiceClientTest, SetCapabilityReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->SetCapability(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, TEST_CAPABILITIES, TEST_CAPABILITIES);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SetCapabilityReturnsErrorForEmptyLocalCapabilities)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->SetCapability(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, {}, TEST_CAPABILITIES);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SetCapabilityReturnsErrorForEmptyRemoteCapabilities)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->SetCapability(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, TEST_CAPABILITIES, {});
    EXPECT_NE(status, SUCCESS);
}

// AddSubscribeInfo tests
TEST_F(KVDBServiceClientTest, AddSubscribeInfoReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->AddSubscribeInfo(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, TEST_SYNC_INFO);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, AddSubscribeInfoReturnsErrorForEmptyDevices)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    syncInfo.devices.clear();
    auto status = client->AddSubscribeInfo(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, AddSubscribeInfoReturnsErrorForEmptyQuery)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    syncInfo.query = "";
    auto status = client->AddSubscribeInfo(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

// RmvSubscribeInfo tests
TEST_F(KVDBServiceClientTest, RmvSubscribeInfoReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->RmvSubscribeInfo(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, TEST_SYNC_INFO);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, RmvSubscribeInfoReturnsErrorForZeroSeqId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    syncInfo.seqId = 0;
    auto status = client->RmvSubscribeInfo(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, RmvSubscribeInfoReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->RmvSubscribeInfo(TEST_APP_ID, {""}, TEST_SUB_USER, TEST_SYNC_INFO);
    EXPECT_NE(status, SUCCESS);
}

// Subscribe tests
TEST_F(KVDBServiceClientTest, SubscribeReturnsSuccessForValidInput)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Subscribe(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, mockObserver_);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SubscribeReturnsErrorForNullObserver)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
    
    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Subscribe(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, nullptr);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SubscribeReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);

    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Subscribe(TEST_APP_ID, {""}, TEST_SUB_USER, mockObserver_);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, SubscribeReturnsErrorForInvalidAppId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);

    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Subscribe({""}, TEST_STORE_ID, TEST_SUB_USER, mockObserver_);
    EXPECT_NE(status, SUCCESS);
}
// AddSubscribeInfo tests
TEST_F(KVDBServiceClientTest, AddSubscribeInfoReturnsErrorForNullSyncInfo)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
}

TEST_F(KVDBServiceClientTest, AddSubscribeInfoReturnsErrorForZeroSeqId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);

    auto client = KVDBServiceClient::GetInstance();
    SyncInfo syncInfo = TEST_SYNC_INFO;
    syncInfo.seqId = 0;
    auto status = client->AddSubscribeInfo(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, AddSubscribeInfoReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);

    auto client = KVDBServiceClient::GetInstance();
    auto status = client->AddSubscribeInfo(TEST_APP_ID, {""}, TEST_SUB_USER, TEST_SYNC_INFO);
    EXPECT_NE(status, SUCCESS);
}

// RmvSubscribeInfo tests
TEST_F(KVDBServiceClientTest, RmvSubscribeInfoReturnsErrorForNullSyncInfo)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
}

TEST_F(KVDBServiceClientTest, RmvSubscribeInfoReturnsErrorForZeroSeqId) {
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
}

TEST_F(KVDBServiceClientTest, RmvSubscribeInfoReturnsErrorForInvalidStoreId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
}

TEST_F(KVDBServiceClientTest, RmvSubscribeInfoReturnsErrorForInvalidAppId)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);
}

// Subscribe tests 
TEST_F(KVDBServiceClientTest, SubscribeReturnsErrorForNullObserver)
{
    EXPECT_CALL(*mockService_, GetFeatureInterface(_)).WillOnce(Return(mockRemote_));
    EXPECT_CALL(*mockRemote_, IsProxyObject()).WillOnce(Return(false));
    KvStoreServiceDeathNotifier::SetDistributedKvDataService(mockService_);

    auto client = KVDBServiceClient::GetInstance();
    auto status = client->Subscribe(TEST_APP_ID, TEST_STORE_ID, TEST_SUB_USER, nullptr);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(KVDBServiceClientTest, GetInstance) {
    auto instance = KVDBServiceClient::GetInstance();
    EXPECT_NE(instance, nullptr);
}

// 测试GetStoreIds方法
TEST_F(KVDBServiceClientTest, GetStoreIds) {
    std::vector<StoreId> storeIds;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetStoreIds(appId, subUser, storeIds);
    EXPECT_EQ(status, SUCCESS);
}

// 测试BeforeCreate方法
TEST_F(KVDBServiceClientTest, BeforeCreate) {
    Options options;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->BeforeCreate(appId, storeId, options);
    EXPECT_EQ(status, SUCCESS);
}

// 测试AfterCreate方法
TEST_F(KVDBServiceClientTest, AfterCreate) {
    Options options;
    std::vector<uint8_t> password = {1, 2, 3, 4};
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->AfterCreate(appId, storeId, options, password);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Delete方法
TEST_F(KVDBServiceClientTest, Delete) {
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Delete(appId, storeId, subUser);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Close方法
TEST_F(KVDBServiceClientTest, Close) {
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Close(appId, storeId, subUser);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Sync方法
TEST_F(KVDBServiceClientTest, Sync) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.mode = 0;
    syncInfo.devices = {"device1", "device2"};
    syncInfo.delay = 100;
    syncInfo.query = "query";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Sync(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试CloudSync方法
TEST_F(KVDBServiceClientTest, CloudSync) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->CloudSync(appId, storeId, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试NotifyDataChange方法
TEST_F(KVDBServiceClientTest, NotifyDataChange) {
    uint64_t delay = 100;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->NotifyDataChange(appId, storeId, delay);
    EXPECT_EQ(status, SUCCESS);
}

// 测试RegServiceNotifier方法
TEST_F(KVDBServiceClientTest, RegServiceNotifier) {
    sptr<MockKVDBNotifier> notifier = new MockKVDBNotifier();
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->RegServiceNotifier(appId, notifier);
    EXPECT_EQ(status, SUCCESS);
}

// 测试UnregServiceNotifier方法
TEST_F(KVDBServiceClientTest, UnregServiceNotifier) {
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->UnregServiceNotifier(appId);
    EXPECT_EQ(status, SUCCESS);
}

// 测试SetSyncParam方法
TEST_F(KVDBServiceClientTest, SetSyncParam) {
    KvSyncParam syncParam;
    syncParam.allowedDelayMs = 1000;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->SetSyncParam(appId, storeId, subUser, syncParam);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetSyncParam方法
TEST_F(KVDBServiceClientTest, GetSyncParam) {
    KvSyncParam syncParam;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetSyncParam(appId, storeId, subUser, syncParam);
    EXPECT_EQ(status, SUCCESS);
}

// 测试EnableCapability方法
TEST_F(KVDBServiceClientTest, EnableCapability) {
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->EnableCapability(appId, storeId, subUser);
    EXPECT_EQ(status, SUCCESS);
}

// 测试DisableCapability方法
TEST_F(KVDBServiceClientTest, DisableCapability) {
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->DisableCapability(appId, storeId, subUser);
    EXPECT_EQ(status, SUCCESS);
}

// 测试SetCapability方法
TEST_F(KVDBServiceClientTest, SetCapability) {
    std::vector<std::string> local = {"local1"};
    std::vector<std::string> remote = {"remote1"};
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->SetCapability(appId, storeId, subUser, local, remote);
    EXPECT_EQ(status, SUCCESS);
}

// 测试AddSubscribeInfo方法
TEST_F(KVDBServiceClientTest, AddSubscribeInfo) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.devices = {"device1"};
    syncInfo.query = "query";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->AddSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试RmvSubscribeInfo方法
TEST_F(KVDBServiceClientTest, RmvSubscribeInfo) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.devices = {"device1"};
    syncInfo.query = "query";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->RmvSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Subscribe方法
TEST_F(KVDBServiceClientTest, Subscribe) {
    sptr<MockKvStoreObserver> observer = new MockKvStoreObserver();
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Subscribe(appId, storeId, subUser, observer);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Unsubscribe方法
TEST_F(KVDBServiceClientTest, Unsubscribe) {
    sptr<MockKvStoreObserver> observer = new MockKvStoreObserver();
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Unsubscribe(appId, storeId, subUser, observer);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetBackupPassword方法
TEST_F(KVDBServiceClientTest, GetBackupPassword) {
    std::vector<std::vector<uint8_t>> passwords;
    int32_t subUser = 0;
    int32_t passwordType = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetBackupPassword(appId, storeId, subUser, passwords, passwordType);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetServiceAgent方法
TEST_F(KVDBServiceClientTest, GetServiceAgent) {
    auto agent = client->GetServiceAgent(appId);
    EXPECT_NE(agent, nullptr);
}

// 测试PutSwitch方法
TEST_F(KVDBServiceClientTest, PutSwitch) {
    SwitchData data;
    data.switchType = 1;
    data.switchValue = "on";
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->PutSwitch(appId, data);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetSwitch方法
TEST_F(KVDBServiceClientTest, GetSwitch) {
    SwitchData data;
    std::string networkId = "network1";
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetSwitch(appId, networkId, data);
    EXPECT_EQ(status, SUCCESS);
}

// 测试SubscribeSwitchData方法
TEST_F(KVDBServiceClientTest, SubscribeSwitchData) {
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->SubscribeSwitchData(appId);
    EXPECT_EQ(status, SUCCESS);
}

// 测试UnsubscribeSwitchData方法
TEST_F(KVDBServiceClientTest, UnsubscribeSwitchData) {
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->UnsubscribeSwitchData(appId);
    EXPECT_EQ(status, SUCCESS);
}

// 测试SetConfig方法
TEST_F(KVDBServiceClientTest, SetConfig) {
    StoreConfig config;
    config.maxSize = 1024;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->SetConfig(appId, storeId, config);
    EXPECT_EQ(status, SUCCESS);
}

// 测试RemoveDeviceData方法
TEST_F(KVDBServiceClientTest, RemoveDeviceData) {
    std::string device = "device1";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->RemoveDeviceData(appId, storeId, subUser, device);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetStoreIds方法（失败场景）
TEST_F(KVDBServiceClientTest, GetStoreIds_Fail) {
    std::vector<StoreId> storeIds;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->GetStoreIds(appId, subUser, storeIds);
    EXPECT_NE(status, SUCCESS);
}

// 测试BeforeCreate方法（失败场景）
TEST_F(KVDBServiceClientTest, BeforeCreate_Fail) {
    Options options;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->BeforeCreate(appId, storeId, options);
    EXPECT_NE(status, SUCCESS);
}

// 测试AfterCreate方法（失败场景）
TEST_F(KVDBServiceClientTest, AfterCreate_Fail) {
    Options options;
    std::vector<uint8_t> password = {1, 2, 3, 4};
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->AfterCreate(appId, storeId, options, password);
    EXPECT_NE(status, SUCCESS);
}

// 测试Delete方法（失败场景）
TEST_F(KVDBServiceClientTest, Delete_Fail) {
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->Delete(appId, storeId, subUser);
    EXPECT_NE(status, SUCCESS);
}

// 测试Sync方法（失败场景）
TEST_F(KVDBServiceClientTest, Sync_Fail) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->Sync(appId, storeId, subUser, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

// 测试CloudSync方法（失败场景）
TEST_F(KVDBServiceClientTest, CloudSync_Fail) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->CloudSync(appId, storeId, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

// 测试NotifyDataChange方法（失败场景）
TEST_F(KVDBServiceClientTest, NotifyDataChange_Fail) {
    uint64_t delay = 100;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->NotifyDataChange(appId, storeId, delay);
    EXPECT_NE(status, SUCCESS);
}

// 测试RegServiceNotifier方法（失败场景）
TEST_F(KVDBServiceClientTest, RegServiceNotifier_Fail) {
    sptr<MockKVDBNotifier> notifier = new MockKVDBNotifier();
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->RegServiceNotifier(appId, notifier);
    EXPECT_NE(status, SUCCESS);
}

// 测试UnregServiceNotifier方法（失败场景）
TEST_F(KVDBServiceClientTest, UnregServiceNotifier_Fail) {
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->UnregServiceNotifier(appId);
    EXPECT_NE(status, SUCCESS);
}

// 测试SetSyncParam方法（失败场景）
TEST_F(KVDBServiceClientTest, SetSyncParam_Fail) {
    KvSyncParam syncParam;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->SetSyncParam(appId, storeId, subUser, syncParam);
    EXPECT_NE(status, SUCCESS);
}

// 测试GetSyncParam方法（失败场景）
TEST_F(KVDBServiceClientTest, GetSyncParam_Fail) {
    KvSyncParam syncParam;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->GetSyncParam(appId, storeId, subUser, syncParam);
    EXPECT_NE(status, SUCCESS);
}


// 测试UnsubscribeSwitchData方法（失败场景）
TEST_F(KVDBServiceClientTest, UnsubscribeSwitchData_Fail) {
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->UnsubscribeSwitchData(appId);
    EXPECT_NE(status, SUCCESS);
}

// 测试SetConfig方法（失败场景）
TEST_F(KVDBServiceClientTest, SetConfig_Fail) {
    Config config;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->SetConfig(appId, storeId, config);
    EXPECT_NE(status, SUCCESS);
}

// 测试RemoveDeviceData方法（失败场景）
TEST_F(KVDBServiceClientTest, RemoveDeviceData_Fail) {
    std::string device = "device";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->RemoveDeviceData(appId, storeId, subUser, device);
    EXPECT_NE(status, SUCCESS);
}

// 测试GetStoreIds方法（失败场景）
TEST_F(KVDBServiceClientTest, GetStoreIds_Fail) {
    std::vector<std::string> storeIds;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
}

// 测试BeforeCreate方法（失败场景）
TEST_F(KVDBServiceClientTest, BeforeCreate_Fail) {
    std::string appId = "appId";
    std::string storeId = "storeId";
    Options options;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
}

DBServiceClientTest, EnableCapability_Fail) {
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->EnableCapability(appId, storeId, subUser);
    EXPECT_NE(status, SUCCESS);
}

// 测试DisableCapability方法（失败场景）
TEST_F(KVDBServiceClientTest, DisableCapability_Fail) {
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->DisableCapability(appId, storeId, subUser);
    EXPECT_NE(status, SUCCESS);
}

// 测试SetCapability方法（失败场景）
TEST_F(KVDBServiceClientTest, SetCapability_Fail) {
    std::vector<std::string> local = {"local1"};
    std::vector<std::string> remote = {"remote1"};
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->SetCapability(appId, storeId, subUser, local, remote);
    EXPECT_NE(status, SUCCESS);
}

// 测试AddSubscribeInfo方法（失败场景）
TEST_F(KVDBServiceClientTest, AddSubscribeInfo_Fail) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->AddSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

// 测试RmvSubscribeInfo方法（失败场景）
TEST_F(KVDBServiceClientTest, RmvSubscribeInfo_Fail) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->RmvSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_NE(status, SUCCESS);
}

// 测试Subscribe方法（失败场景）
TEST_F(KVDBServiceClientTest, Subscribe_Fail) {
    sptr<MockKvStoreObserver> observer = new MockKvStoreObserver();
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->Subscribe(appId, storeId, subUser, observer);
    EXPECT_NE(status, SUCCESS);
}

// 测试Unsubscribe方法（失败场景）
TEST_F(KVDBServiceClientTest, Unsubscribe_Fail) {
    sptr<MockKvStoreObserver> observer = new MockKvStoreObserver();
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->Unsubscribe(appId, storeId, subUser, observer);
    EXPECT_NE(status, SUCCESS);
}

// 测试GetBackupPassword方法（失败场景）
TEST_F(KVDBServiceClientTest, GetBackupPassword_Fail) {
    std::vector<std::vector<uint8_t>> passwords;
    int32_t subUser = 0;
    int32_t passwordType = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->GetBackupPassword(appId, storeId, subUser, passwords, passwordType);
    EXPECT_NE(status, SUCCESS);
}

// 测试PutSwitch方法（失败场景）
TEST_F(KVDBServiceClientTest, PutSwitch_Fail) {
    SwitchData data;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->PutSwitch(appId, data);
    EXPECT_NE(status, SUCCESS);
}

// 测试GetSwitch方法（失败场景）
TEST_F(KVDBServiceClientTest, GetSwitch_Fail) {
    SwitchData data;
    std::string networkId = "network1";
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->GetSwitch(appId, networkId, data);
    EXPECT_NE(status, SUCCESS);
}

// 测试SubscribeSwitchData方法（失败场景）
TEST_F(KVDBServiceClientTest, SubscribeSwitchData_Fail) {
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->SubscribeSwitchData(appId);
    EXPECT_NE(status, SUCCESS);
}

// 测试UnsubscribeSwitchData方法（失败场景）
TEST_F(KVDBServiceClientTest, UnsubscribeSwitchData_Fail) {
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->UnsubscribeSwitchData(appId);
    EXPECT_NE(status, SUCCESS);
}

// 测试SetConfig方法（失败场景）
TEST_F(KVDBServiceClientTest, SetConfig_Fail) {
    StoreConfig config;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->SetConfig(appId, storeId, config);
    EXPECT_NE(status, SUCCESS);
}

// 测试RemoveDeviceData方法（失败场景）
TEST_F(KVDBServiceClientTest, RemoveDeviceData_Fail) {
    std::string device = "device1";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(-1));
    Status status = client->RemoveDeviceData(appId, storeId, subUser, device);
    EXPECT_NE(status, SUCCESS);
}

// 测试GetStoreIds方法（多用户场景）
TEST_F(KVDBServiceClientTest, GetStoreIds_MultiUser) {
    std::vector<StoreId> storeIds;
    int32_t subUser = 1;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetStoreIds(appId, subUser, storeIds);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Delete方法（多用户场景）
TEST_F(KVDBServiceClientTest, Delete_MultiUser) {
    int32_t subUser = 1;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Delete(appId, storeId, subUser);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Close方法（多用户场景）
TEST_F(KVDBServiceClientTest, Close_MultiUser) {
    int32_t subUser = 1;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Close(appId, storeId, subUser);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Sync方法（多设备场景）
TEST_F(KVDBServiceClientTest, Sync_MultiDevices) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.devices = {"device1", "device2", "device3"};
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Sync(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Sync方法（延迟场景）
TEST_F(KVDBServiceClientTest, Sync_Delay) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.delay = 500;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Sync(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试AddSubscribeInfo方法（多设备场景）
TEST_F(KVDBServiceClientTest, AddSubscribeInfo_MultiDevices) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.devices = {"device1", "device2"};
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->AddSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试RmvSubscribeInfo方法（多设备场景）
TEST_F(KVDBServiceClientTest, RmvSubscribeInfo_MultiDevices) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.devices = {"device1", "device2"};
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->RmvSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetSwitch方法（不同网络ID场景）
TEST_F(KVDBServiceClientTest, GetSwitch_DifferentNetworkId) {
    SwitchData data;
    std::string networkId = "network2";
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetSwitch(appId, networkId, data);
    EXPECT_EQ(status, SUCCESS);
}

// 测试SetSyncParam方法（不同延迟场景）
TEST_F(KVDBServiceClientTest, SetSyncParam_DifferentDelay) {
    KvSyncParam syncParam;
    syncParam.allowedDelayMs = 2000;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->SetSyncParam(appId, storeId, subUser, syncParam);
    EXPECT_EQ(status, SUCCESS);
}

// 测试SetCapability方法（多标签场景）
TEST_F(KVDBServiceClientTest, SetCapability_MultiLabels) {
    std::vector<std::string> local = {"local1", "local2"};
    std::vector<std::string> remote = {"remote1", "remote2"};
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->SetCapability(appId, storeId, subUser, local, remote);
    EXPECT_EQ(status, SUCCESS);
}

// 测试CloudSync方法（不同序列号场景）
TEST_F(KVDBServiceClientTest, CloudSync_DifferentSeqId) {
    SyncInfo syncInfo;
    syncInfo.seqId = 456;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->CloudSync(appId, storeId, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试NotifyDataChange方法（不同延迟场景）
TEST_F(KVDBServiceClientTest, NotifyDataChange_DifferentDelay) {
    uint64_t delay = 1000;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->NotifyDataChange(appId, storeId, delay);
    EXPECT_EQ(status, SUCCESS);
}

// 测试AfterCreate方法（加密场景）
TEST_F(KVDBServiceClientTest, AfterCreate_Encrypt) {
    Options options;
    options.encrypt = true;
    std::vector<uint8_t> password = {1, 2, 3, 4};
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->AfterCreate(appId, storeId, options, password);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetServiceAgent方法（重复调用场景）
TEST_F(KVDBServiceClientTest, GetServiceAgent_RepeatCall) {
    auto agent1 = client->GetServiceAgent(appId);
    auto agent2 = client->GetServiceAgent(appId);
    EXPECT_EQ(agent1, agent2);
}

// 测试Subscribe方法（重复订阅场景）
TEST_F(KVDBServiceClientTest, Subscribe_Repeat) {
    sptr<MockKvStoreObserver> observer = new MockKvStoreObserver();
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillRepeatedly(Return(0));
    Status status1 = client->Subscribe(appId, storeId, subUser, observer);
    Status status2 = client->Subscribe(appId, storeId, subUser, observer);
    EXPECT_EQ(status1, SUCCESS);
    EXPECT_EQ(status2, SUCCESS);
}

// 测试Unsubscribe方法（未订阅场景）
TEST_F(KVDBServiceClientTest, Unsubscribe_NotSubscribed) {
    sptr<MockKvStoreObserver> observer = new MockKvStoreObserver();
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->Unsubscribe(appId, storeId, subUser, observer);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetStoreIds方法（空应用ID场景）
TEST_F(KVDBServiceClientTest, GetStoreIds_EmptyAppId) {
    AppId emptyAppId;
    emptyAppId.appId = "";
    std::vector<StoreId> storeIds;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetStoreIds(emptyAppId, subUser, storeIds);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetStoreIds方法（长应用ID场景）
TEST_F(KVDBServiceClientTest, GetStoreIds_LongAppId) {
    AppId longAppId;
    longAppId.appId = std::string(1024, 'a');
    std::vector<StoreId> storeIds;
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetStoreIds(longAppId, subUser, storeIds);
    EXPECT_EQ(status, SUCCESS);
}

// 测试AddSubscribeInfo方法（空查询场景）
TEST_F(KVDBServiceClientTest, AddSubscribeInfo_EmptyQuery) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.query = "";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->AddSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试AddSubscribeInfo方法（长查询场景）
TEST_F(KVDBServiceClientTest, AddSubscribeInfo_LongQuery) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.query = std::string(2048, 'q');
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->AddSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试RmvSubscribeInfo方法（空查询场景）
TEST_F(KVDBServiceClientTest, RmvSubscribeInfo_EmptyQuery) {
    SyncInfo syncInfo;
    syncInfo.seqId = 123;
    syncInfo.query = "";
    int32_t subUser = 0;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->RmvSubscribeInfo(appId, storeId, subUser, syncInfo);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetBackupPassword方法（不同密码类型场景）
TEST_F(KVDBServiceClientTest, GetBackupPassword_DifferentType) {
    std::vector<std::vector<uint8_t>> passwords;
    int32_t subUser = 0;
    int32_t passwordType = 1;
    EXPECT_CALL(*mockRemote, SendRequest(_, _, _, _)).WillOnce(Return(0));
    Status status = client->GetBackupPassword(appId, storeId, subUser, passwords, passwordType);
    EXPECT_EQ(status, SUCCESS);
}

}; // namespace DistributedDB