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

#include "include/accesstoken_kit_mock.h"
#include "include/convertor_mock.h"
#include "include/dev_manager_mock.h"
#include "include/kvdb_notifier_client_mock.h"
#include "include/kvdb_service_client_mock.h"
#include "include/observer_bridge_mock.h"
#include "include/task_executor_mock.h"
#include "kvstore_observer.h"
#include "single_store_impl.h"
#include "store_factory.h"
#include "store_manager.h"

namespace OHOS::DistributedKv {
using namespace std;
using namespace testing;
using namespace DistributedDB;
using namespace Security::AccessToken;

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
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock = nullptr;
    static inline shared_ptr<ConvertorMock> convertorMock = nullptr;
    std::shared_ptr<SingleStoreImpl> CreateKVStore(bool autosync = false, bool backup = true);
};

void SingleStoreImplMockTest::SetUp() { }

void SingleStoreImplMockTest::TearDown() { }

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
    accessTokenKitMock = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenKit = accessTokenKitMock;
    convertorMock = make_shared<ConvertorMock>();
    BConvertor::convertor = convertorMock;
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
    BAccessTokenKit::accessTokenKit = nullptr;
    accessTokenKitMock = nullptr;
    BConvertor::convertor = nullptr;
    convertorMock = nullptr;
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    (void)remove("/data/service/el1/public/database/SingleStoreImplTest");
}

std::shared_ptr<SingleStoreImpl> SingleStoreImplMockTest::CreateKVStore(bool autosync, bool backup)
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
    options.backup = backup;
    StoreFactory storeFactory;
    auto dbManager = storeFactory.GetDBManager(options.baseDir, appId);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(storeId.storeId, options.baseDir, options.encrypt);
    DBStatus dbStatus = DBStatus::DB_ERROR;
    dbManager->GetKvStore(storeId, storeFactory.GetDBOption(options, dbPassword),
        [&dbManager, &kvStore, &appId, &dbStatus, &options, &storeFactory](auto status, auto *store) {
            dbStatus = status;
            if (store == nullptr) {
                return;
            }
            auto release = [dbManager](auto *store) {
                dbManager->CloseKvStore(store);
            };
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
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_INVALID));
        std::shared_ptr<SingleStoreImpl> kvStore;
        kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_FALSE(kvStore->isApplication_);

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
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_HAP));
        std::shared_ptr<SingleStoreImpl> kvStore;
        kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_TRUE(kvStore->isApplication_);

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

/**
* @tc.name: Put_001
* @tc.desc: Put.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, Put_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin Put_001";
    try {
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).Times(1);
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).Times(AnyNumber());
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::vector<uint8_t> vect;
        EXPECT_CALL(*convertorMock, ToLocalDBKey(_)).WillOnce(Return(vect));
        size_t maxTestKeyLen = 10;
        std::string str(maxTestKeyLen, 'a');
        Blob key(str);
        Blob value("test_value");
        Status status = kvStore->Put(key, value);
        EXPECT_TRUE(status == INVALID_ARGUMENT);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by Put_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end Put_001";
}


/**
* @tc.name: Put_002
* @tc.desc: Put.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, Put_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin Put_002";
    try {
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).Times(AnyNumber());
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::vector<uint8_t> vect{3, 8};
        EXPECT_CALL(*convertorMock, ToLocalDBKey(_)).WillOnce(Return(vect));
        size_t overlongTestKeyLen = 4 * 1024 * 1024 + 1;
        std::string str(overlongTestKeyLen, 'b');
        Blob key1("key1");
        Blob value1(str);
        Status status = kvStore->Put(key1, value1);
        EXPECT_TRUE(status == INVALID_ARGUMENT);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by Put_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end Put_002";
}

/**
* @tc.name: PutBatch_001
* @tc.desc: PutBatch.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, PutBatch_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin PutBatch_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_ = nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        std::vector<Entry> in;
        for (int i = 0; i < 2; ++i) {
            Entry entry;
            entry.key = std::to_string(i).append("_k");
            entry.value = std::to_string(i).append("_v");
            in.emplace_back(entry);
        }
        Status status = kvStore->PutBatch(in);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by PutBatch_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end PutBatch_001";
}

/**
* @tc.name: PutBatch_002
* @tc.desc: PutBatch.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, PutBatch_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin PutBatch_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::vector<Entry> in;
        for (int i = 0; i < 2; ++i) {
            Entry entry;
            entry.key = std::to_string(i).append("_key");
            entry.value = std::to_string(i).append("_val");
            in.emplace_back(entry);
        }
        std::vector<uint8_t> vect;
        EXPECT_CALL(*convertorMock, ToLocalDBKey(_)).WillOnce(Return(vect));
        Status status = kvStore->PutBatch(in);
        EXPECT_TRUE(status == INVALID_ARGUMENT);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by PutBatch_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end PutBatch_002";
}

/**
* @tc.name: PutBatch_003
* @tc.desc: PutBatch.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, PutBatch_003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin PutBatch_003";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::vector<Entry> in;
        for (int i = 0; i < 2; ++i) {
            Entry entry;
            entry.key = std::to_string(i).append("_key");
            entry.value = std::to_string(i).append("_val");
            in.emplace_back(entry);
        }
        std::vector<uint8_t> vect;
        EXPECT_CALL(*convertorMock, ToLocalDBKey(_)).WillOnce(Return(vect));
        Status status = kvStore->PutBatch(in);
        EXPECT_TRUE(status == INVALID_ARGUMENT);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by PutBatch_003.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end PutBatch_003";
}

/**
* @tc.name: Delete
* @tc.desc: Delete Key.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, Delete, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin Delete";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Blob key1("key1");
        Status status = kvStore->Delete(key1);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by Delete.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end Delete";
}

/**
* @tc.name: DeleteBatch
* @tc.desc: DeleteBatch Keys.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, DeleteBatch, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin DeleteBatch";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        std::vector<Key> keys;
        for (int i = 0; i < 2; ++i) {
            Key key = std::to_string(i).append("_k");
            keys.emplace_back(key);
        }
        Status status = kvStore->DeleteBatch(keys);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by DeleteBatch.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end DeleteBatch";
}

/**
* @tc.name: StartTransaction
* @tc.desc: Start Transaction.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, StartTransaction, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin StartTransaction";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->StartTransaction();
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by StartTransaction.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end StartTransaction";
}

/**
* @tc.name: Commit
* @tc.desc: Commit.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, Commit, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin Commit";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->Commit();
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by Commit.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end Commit";
}

/**
* @tc.name: Rollback
* @tc.desc: Rollback kvstore.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, Rollback, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin Rollback";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->Rollback();
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by Rollback.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end Rollback";
}

/**
* @tc.name: Get
* @tc.desc: Get.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, Get, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin Get";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        size_t testKeyLen = 10;
        std::string str(testKeyLen, 'a');
        Blob key(str);
        Blob value("test_value");
        Status status = kvStore->Get(key, value);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by Get.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end Get";
}

/**
* @tc.name: GetEntries_001
* @tc.desc: Get Entries.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, GetEntries_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin GetEntries_001";
    try {
        std::vector<uint8_t> vct;
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*convertorMock, GetPrefix(An<const Key&>())).WillOnce(Return(vct));
        Blob key("test");
        std::vector<Entry> vecs;
        for (int i = 0; i < 2; ++i) {
            Entry entry;
            entry.key = std::to_string(i).append("_key");
            entry.value = std::to_string(i).append("_val");
            vecs.emplace_back(entry);
        }
        Status status = kvStore->GetEntries(key, vecs);
        EXPECT_TRUE(status == INVALID_ARGUMENT);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by GetEntries_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end GetEntries_001";
}

/**
* @tc.name: GetDeviceEntries
* @tc.desc: Get device entries.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, GetDeviceEntries, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin GetDeviceEntries";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        std::vector<Entry> vcs;
        for (int i = 0; i < 2; ++i) {
            Entry entry;
            entry.key = std::to_string(i).append("_key");
            entry.value = std::to_string(i).append("_val");
            vcs.emplace_back(entry);
        }
        std::string device = "test device";
        Status status = kvStore->GetDeviceEntries(device, vcs);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by GetDeviceEntries.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end GetDeviceEntries";
}

/**
* @tc.name: GetCount
* @tc.desc: Get count.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, GetCount, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin GetCount";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        DataQuery query;
        int cnt = 0;
        Status status = kvStore->GetCount(query, cnt);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by GetCount.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end GetCount";
}

/**
* @tc.name: GetCount
* @tc.desc: Get count.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, GetSecurityLevel, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin GetSecurityLevel";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        SecurityLevel securityLevel = NO_LABEL;
        Status status = kvStore->GetSecurityLevel(securityLevel);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by GetSecurityLevel.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end GetSecurityLevel";
}

/**
* @tc.name: RemoveDeviceData_001
* @tc.desc: Remove device data.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, RemoveDeviceData_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin RemoveDeviceData_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->RemoveDeviceData("testdevice");
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by RemoveDeviceData_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end RemoveDeviceData_001";
}

/**
* @tc.name: RemoveDeviceData_002
* @tc.desc: Remove device data.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, RemoveDeviceData_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin RemoveDeviceData_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        Status status = kvStore->RemoveDeviceData("testdevice");
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by RemoveDeviceData_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end RemoveDeviceData_002";
}

/**
* @tc.name: CloudSync_001
* @tc.desc: Cloud sync.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, CloudSync_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin CloudSync_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        AsyncDetail asyncDetail;
        Status status = kvStore->CloudSync(asyncDetail);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by CloudSync_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end CloudSync_001";
}

/**
* @tc.name: CloudSync_002
* @tc.desc: Cloud sync.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, CloudSync_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin CloudSync_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::shared_ptr<KVDBServiceClient> ser = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_NE(ser, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(ser));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        AsyncDetail asyncDetail;
        Status status = kvStore->CloudSync(asyncDetail);
        EXPECT_TRUE(status == ILLEGAL_STATE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by CloudSync_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end CloudSync_002";
}

/**
* @tc.name: SetSyncParam
* @tc.desc: Set sync param.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, SetSyncParam, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin SetSyncParam";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        KvSyncParam syncParam{ 500 };
        Status status = kvStore->SetSyncParam(syncParam);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by SetSyncParam.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end SetSyncParam";
}

/**
* @tc.name: GetSyncParam
* @tc.desc: Get sync param.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, GetSyncParam, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin GetSyncParam";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        KvSyncParam syncParam;
        Status status = kvStore->GetSyncParam(syncParam);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by GetSyncParam.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end GetSyncParam";
}

/**
* @tc.name: SetCapabilityEnabled_001
* @tc.desc: Set capability enabled.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, SetCapabilityEnabled_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin SetCapabilityEnabled_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        Status status = kvStore->SetCapabilityEnabled(false);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by SetCapabilityEnabled_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end SetCapabilityEnabled_001";
}

/**
* @tc.name: SetCapabilityEnabled_002
* @tc.desc: Set capability enabled.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, SetCapabilityEnabled_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin SetCapabilityEnabled_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::shared_ptr<KVDBServiceClient> service = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_NE(service, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(service));
        Status status = kvStore->SetCapabilityEnabled(true);
        EXPECT_TRUE(status == ERROR);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by SetCapabilityEnabled_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end SetCapabilityEnabled_002";
}

/**
* @tc.name: SetCapabilityRange
* @tc.desc: Set capability range.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, SetCapabilityRange, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin SetCapabilityRange";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        std::vector<std::string> localLabels{"local", "near"};
        std::vector<std::string> remoteLabels{"remote", "far"};
        Status status = kvStore->SetCapabilityRange(localLabels, remoteLabels);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by SetCapabilityRange.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end SetCapabilityRange";
}

/**
* @tc.name: SubscribeWithQuery_001
* @tc.desc: Subscribe with query.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, SubscribeWithQuery_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin SubscribeWithQuery_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        std::vector<std::string> devices{"dev1", "dev2"};
        DataQuery query;
        Status status = kvStore->SubscribeWithQuery(devices, query);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by SubscribeWithQuery_001";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end SubscribeWithQuery_001";
}

/**
* @tc.name: SubscribeWithQuery_002
* @tc.desc: Subscribe with query.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(SingleStoreImplMockTest, SubscribeWithQuery_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-begin SubscribeWithQuery_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_NE(kvStore, nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::shared_ptr<KVDBServiceClient> serv = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_NE(serv, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(serv));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        std::vector<std::string> devices{"dev0", "dev1"};
        DataQuery query;
        Status status = kvStore->SubscribeWithQuery(devices, query);
        EXPECT_TRUE(status == ILLEGAL_STATE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockTest-an exception occurred by SubscribeWithQuery_002";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockTest-end SubscribeWithQuery_002";
}
} // namespace OHOS::DistributedKv