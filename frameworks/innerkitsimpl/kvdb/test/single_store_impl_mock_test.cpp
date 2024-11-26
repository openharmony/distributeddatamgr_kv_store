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
#include <memory>
#include <sys/types.h>
#include <unistd.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "convertor.h"
#include "single_store_impl.h"
#include "store_manager.h"
#include "store_factory.h"
#include "include/dev_manager_mock.h"
#include "kvstore_observer.h"
#include "include/kvdb_notifier_client_mock.h"
#include "include/kvdb_service_client_mock.h"
#include "include/observer_bridge_mock.h"
#include "include/task_executor_mock.h"

namespace OHOS::DistributedKv {
using namespace std;
using namespace testing;
using namespace DistributedDB;

static StoreId storeId = { "single_test" };
static AppId appId = { "rekey" };

class SingleStoreImplMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
public:
    using DBStatus = DistributedDB::DBStatus;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using Observer = DistributedKv::KvStoreObserver;
    static inline shared_ptr<DevManagerMock> devManagerMock = nullptr;
    static inline shared_ptr<KVDBServiceClientMock> kVDBServiceClientMock = nullptr;
    static inline shared_ptr<KVDBNotifierClientMock> kVDBNotifierClientMock = nullptr;
    static inline shared_ptr<ObserverBridgeMock> observerBridgeMock = nullptr;
    static inline shared_ptr<TaskExecutorMock> taskExecutorMock = nullptr;
    std::shared_ptr<SingleStoreImpl> CreateKVStore(bool autosync = false);
};

void SingleStoreImplMockTest::SetUp()
{
}

void SingleStoreImplMockTest::TearDown()
{
}

void SingleStoreImplMockTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase enter";
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    devManagerMock = make_shared<DevManagerMock>();
    BDevManager::devManager = devManagerMock;
    kVDBServiceClientMock = make_shared<KVDBServiceClientMock>();
    BKVDBServiceClient::kVDBServiceClient = kVDBServiceClientMock;
    kVDBNotifierClientMock = make_shared<KVDBNotifierClientMock>();
    BKVDBNotifierClient::kVDBNotifierClient = kVDBNotifierClientMock;
    observerBridgeMock = make_shared<ObserverBridgeMock>();
    BObserverBridge::observerBridge = observerBridgeMock;
    taskExecutorMock = make_shared<TaskExecutorMock>();
    BTaskExecutor::taskExecutor = taskExecutorMock;
}

void SingleStoreImplMockTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase enter";
    BDevManager::devManager = nullptr;
    devManagerMock = nullptr;
    BKVDBServiceClient::kVDBServiceClient = nullptr;
    kVDBServiceClientMock = nullptr;
    BKVDBNotifierClient::kVDBNotifierClient = nullptr;
    kVDBNotifierClientMock = nullptr;
    BObserverBridge::observerBridge = nullptr;
    observerBridgeMock = nullptr;
    BTaskExecutor::taskExecutor = nullptr;
    taskExecutorMock = nullptr;
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    (void)remove("/data/service/el1/public/database/SingleStoreImplTest");
}

std::shared_ptr<SingleStoreImpl> SingleStoreImplMockTest::CreateKVStore(bool autosync)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "DestructorTest" };
    std::shared_ptr<SingleStoreImpl> kvStore;
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S2;
    options.area = EL1;
    options.autoSync = autosync;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    StoreFactory storeFactory;
    auto dbManager = storeFactory.GetDBManager(options.baseDir, appId);
    auto dbPassword =
        SecurityManager::GetInstance().GetDBPassword(storeId.storeId, options.baseDir, options.encrypt);
    DBStatus dbStatus = DBStatus::DB_ERROR;
    dbManager->GetKvStore(storeId, storeFactory.GetDBOption(options, dbPassword),
        [&dbManager, &kvStore, &appId, &dbStatus, &options, &storeFactory](auto status, auto *store) {
            dbStatus = status;
            if (store == nullptr) {
                return;
            }
            auto release = [dbManager](auto *store) { dbManager->CloseKvStore(store); };
            auto dbStore = std::shared_ptr<DBStore>(store, release);
            storeFactory.SetDbConfig(dbStore);
            const Convertor &convertor = *(storeFactory.convertors_[options.kvStoreType]);
            kvStore = std::make_shared<SingleStoreImpl>(dbStore, appId, options, convertor);
        });
    return kvStore;
}

/**
* @tc.name: IsRemoteChanged
* @tc.desc: is remote changed.
* @tc.type: FUNC
* @tc.require:
* @tc.author: cao zhijun
*/
HWTEST_F(SingleStoreImplMockTest, IsRemoteChanged, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin IsRemoteChanged";
    try {
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).Times(1);
        std::shared_ptr<SingleStoreImpl> kvStore;
        kvStore = CreateKVStore();
        ASSERT_NE(kvStore, nullptr);
        std::shared_ptr<KVDBServiceClient> client = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_NE(client, nullptr);
        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return(""));
        bool ret = kvStore->IsRemoteChanged("123456789");
        EXPECT_TRUE(ret);

        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return("123456789"));
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        ret = kvStore->IsRemoteChanged("123456789");
        EXPECT_TRUE(ret);

        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return("123456789"));
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(client));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        ret = kvStore->IsRemoteChanged("123456789");
        EXPECT_TRUE(ret);

        sptr<KVDBNotifierClient> testAgent = new (std::nothrow) KVDBNotifierClient();
        ASSERT_NE(testAgent, nullptr);
        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return("123456789"));
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(client));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(testAgent));
        EXPECT_CALL(*kVDBNotifierClientMock, IsChanged(_, _)).WillOnce(Return(true));
        ret = kvStore->IsRemoteChanged("123456789");
        EXPECT_TRUE(ret);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by IsRemoteChanged.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end IsRemoteChanged";
}

/**
* @tc.name: OnRemoteDied
* @tc.desc: remote died.
* @tc.type: FUNC
* @tc.require:
* @tc.author: cao zhijun
*/
HWTEST_F(SingleStoreImplMockTest, OnRemoteDied, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin OnRemoteDied";
    try {
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).Times(1);
        std::shared_ptr<SingleStoreImpl> kvStore;
        kvStore = CreateKVStore();
        ASSERT_NE(kvStore, nullptr);
        kvStore->taskId_ = 1;
        kvStore->OnRemoteDied();

        kvStore->taskId_ = 0;
        shared_ptr<Observer> observer = make_shared<Observer>();
        shared_ptr<Observer> observer1 = make_shared<Observer>();
        Convertor cvt;
        Convertor cvt1;
        shared_ptr<ObserverBridge> obsBridge = make_shared<ObserverBridge>(appId, storeId, observer, cvt);
        shared_ptr<ObserverBridge> obsBridge1 = make_shared<ObserverBridge>(appId, storeId, observer1, cvt1);

        uint32_t firs = 0;
        firs |= SUBSCRIBE_TYPE_REMOTE;
        pair<uint32_t, std::shared_ptr<ObserverBridge>> one(0, obsBridge);
        pair<uint32_t, std::shared_ptr<ObserverBridge>> two(firs, obsBridge1);

        kvStore->observers_.Insert(uintptr_t(observer.get()), one);
        kvStore->observers_.Insert(uintptr_t(observer1.get()), two);
        EXPECT_CALL(*observerBridgeMock, OnServiceDeath()).Times(1);
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).WillOnce(Return(1));
        kvStore->OnRemoteDied();
        kvStore->observers_.Erase(uintptr_t(observer.get()));
        kvStore->observers_.Erase(uintptr_t(observer1.get()));
        EXPECT_TRUE(kvStore->taskId_ == 1);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by OnRemoteDied.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end OnRemoteDied";
}

/**
* @tc.name: Register
* @tc.desc: register.
* @tc.type: FUNC
* @tc.require:
* @tc.author: cao zhijun
*/
HWTEST_F(SingleStoreImplMockTest, Register, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin Register";
    try {
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).Times(1);
        std::shared_ptr<SingleStoreImpl> kvStore;
        kvStore = CreateKVStore();
        ASSERT_NE(kvStore, nullptr);

        shared_ptr<Observer> observer = make_shared<Observer>();
        shared_ptr<Observer> observer1 = make_shared<Observer>();
        Convertor cvt;
        Convertor cvt1;
        shared_ptr<ObserverBridge> obsBridge = make_shared<ObserverBridge>(appId, storeId, observer, cvt);
        shared_ptr<ObserverBridge> obsBridge1 = make_shared<ObserverBridge>(appId, storeId, observer1, cvt1);

        uint32_t firs = 0;
        firs |= SUBSCRIBE_TYPE_CLOUD;
        pair<uint32_t, std::shared_ptr<ObserverBridge>> one(0, obsBridge);
        pair<uint32_t, std::shared_ptr<ObserverBridge>> two(firs, obsBridge1);

        kvStore->observers_.Insert(uintptr_t(observer.get()), one);
        kvStore->observers_.Insert(uintptr_t(observer1.get()), two);
        EXPECT_CALL(*observerBridgeMock, RegisterRemoteObserver(_)).WillOnce(Return(ERROR));
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).WillOnce(Return(1));
        kvStore->Register();
        EXPECT_TRUE(kvStore->taskId_ == 1);

        EXPECT_CALL(*observerBridgeMock, RegisterRemoteObserver(_)).WillOnce(Return(SUCCESS));
        kvStore->Register();
        kvStore->observers_.Erase(uintptr_t(observer.get()));
        kvStore->observers_.Erase(uintptr_t(observer1.get()));
        EXPECT_TRUE(kvStore->taskId_ == 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by Register.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end Register";
}
} // namespace OHOS::DistributedKv