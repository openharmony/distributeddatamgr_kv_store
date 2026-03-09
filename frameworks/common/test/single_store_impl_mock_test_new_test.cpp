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

class SingleStoreImplMockNew : public testing::Test {
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
    std::shared_ptr<SingleStoreImpl> CreateKVStore(bool autosync = false, bool backup = true, bool isSyncable = false);
};

void SingleStoreImplMockNew::SetUp() { }

void SingleStoreImplMockNew::TearDown() { }

void SingleStoreImplMockNew::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase enter";
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplNew";
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

void SingleStoreImplMockNew::TearDownTestCase()
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
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplNew";
    (void)remove("/data/service/el1/public/database/SingleStoreImplNew");
}

std::shared_ptr<SingleStoreImpl> SingleStoreImplMockNew::CreateKVStore(bool autosync, bool backup, bool isSyncable)
{
    AppId appId = { "SingleStoreImplNew" };
    StoreId storeId = { "DestructorTestNew" };
    std::shared_ptr<SingleStoreImpl> kvStore;
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S2;
    options.area = EL1;
    options.autoSync = autosync;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplNew";
    options.backup = backup;
    options.syncable = isSyncable;
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
 * @tc.name: IsRemoteChangedNew
 * @tc.desc: is remote changed.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, IsRemoteChangedNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin IsRemoteChangedNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore();
        ASSERT_TRUE(kvStore != nullptr);
        std::shared_ptr<KVDBServiceClient> client = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(client != nullptr);
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
        ASSERT_TRUE(testAgent != nullptr);
        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return("123456789"));
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(client));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(testAgent));
        EXPECT_CALL(*kVDBNotifierClientMock, IsChanged(_, _)).WillOnce(Return(true));
        ret = kvStore->IsRemoteChanged("123456789");
        EXPECT_TRUE(ret);
        kvStore = CreateKVStore(false, true, true);
        ASSERT_TRUE(kvStore != nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by IsRemoteChangedNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end IsRemoteChangedNew";
}

/**
 * @tc.name: OnRemoteDiedNew
 * @tc.desc: remote died.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, OnRemoteDiedNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin OnRemoteDiedNew";
    try {
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_INVALID));
        std::shared_ptr<SingleStoreImpl> kvStore;
        kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_FALSE(kvStore->isApplication_);

        kvStore->taskId_ = 1;
        kvStore->OnRemoteDied();

        kvStore->taskId_ = 0;
        shared_ptr<Observer> observer = make_shared<Observer>();
        shared_ptr<Observer> observer1 = make_shared<Observer>();
        Convertor cvt;
        Convertor cvt1;
        shared_ptr<ObserverBridge> obsBridge = make_shared<ObserverBridge>(appId, storeId, 0, observer, cvt);
        shared_ptr<ObserverBridge> obsBridge1 = make_shared<ObserverBridge>(appId, storeId, 0, observer1, cvt1);

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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by OnRemoteDiedNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end OnRemoteDiedNew";
}

/**
 * @tc.name: RegisterNew
 * @tc.desc: register.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, RegisterNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin RegisterNew";
    try {
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_HAP));
        std::shared_ptr<SingleStoreImpl> kvStore;
        kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_TRUE(kvStore->isApplication_);

        shared_ptr<Observer> observer = make_shared<Observer>();
        shared_ptr<Observer> observer1 = make_shared<Observer>();
        Convertor cvt;
        Convertor cvt1;
        shared_ptr<ObserverBridge> obsBridge = make_shared<ObserverBridge>(appId, storeId, 0, observer, cvt);
        shared_ptr<ObserverBridge> obsBridge1 = make_shared<ObserverBridge>(appId, storeId, 0, observer1, cvt1);

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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by RegisterNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end RegisterNew";
}

/**
 * @tc.name: PutNew_001
 * @tc.desc: Put.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, PutNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin PutNew_001";
    try {
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).Times(AnyNumber());
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false, true);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by PutNew_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end PutNew_001";
}

/**
 * @tc.name: PutNew_002
 * @tc.desc: Put.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, PutNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin PutNew_002";
    try {
        EXPECT_CALL(*taskExecutorMock, Schedule(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).Times(AnyNumber());
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by PutNew_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end PutNew_002";
}

/**
 * @tc.name: PutBatchNew_001
 * @tc.desc: PutBatch.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, PutBatchNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin PutBatchNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by PutBatchNew_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end PutBatchNew_001";
}

/**
 * @tc.name: PutBatchNew_002
 * @tc.desc: PutBatch.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, PutBatchNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin PutBatchNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by PutBatchNew_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end PutBatchNew_002";
}

/**
 * @tc.name: PutBatchNew_003
 * @tc.desc: PutBatch.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, PutBatchNew_003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin PutBatchNew_003";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by PutBatchNew_003.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end PutBatchNew_003";
}

/**
 * @tc.name: DeleteNew
 * @tc.desc: Delete Key.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, DeleteNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin DeleteNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Blob key1("key1");
        Status status = kvStore->Delete(key1);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by DeleteNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end DeleteNew";
}

/**
 * @tc.name: DeleteBatchNew
 * @tc.desc: DeleteBatch Keys.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, DeleteBatchNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin DeleteBatchNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by DeleteBatchNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end DeleteBatchNew";
}

/**
 * @tc.name: StartTransactionNew
 * @tc.desc: Start Transaction.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, StartTransactionNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin StartTransactionNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->StartTransaction();
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by StartTransactionNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end StartTransactionNew";
}

/**
 * @tc.name: CommitNew
 * @tc.desc: Commit.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, CommitNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin CommitNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->Commit();
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by CommitNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end CommitNew";
}

/**
 * @tc.name: RollbackNew
 * @tc.desc: Rollback kvstore.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, RollbackNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin RollbackNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->Rollback();
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by RollbackNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end RollbackNew";
}

/**
 * @tc.name: GetNew
 * @tc.desc: Get.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetNew";
}

/**
 * @tc.name: GetEntriesNew_001
 * @tc.desc: Get Entries.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetEntriesNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetEntriesNew_001";
    try {
        std::vector<uint8_t> vct;
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetEntriesNew_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetEntriesNew_001";
}

/**
 * @tc.name: GetDeviceEntriesNew
 * @tc.desc: Get device entries.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetDeviceEntriesNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetDeviceEntriesNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
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
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetDeviceEntriesNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetDeviceEntriesNew";
}

/**
 * @tc.name: GetCountNew
 * @tc.desc: Get count.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetCountNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetCountNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        DataQuery query;
        int cnt = 0;
        Status status = kvStore->GetCount(query, cnt);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetCountNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetCountNew";
}

/**
 * @tc.name: GetSecurityLevelNew
 * @tc.desc: Get security level.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetSecurityLevelNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetSecurityLevelNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        SecurityLevel securityLevel = NO_LABEL;
        Status status = kvStore->GetSecurityLevel(securityLevel);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetSecurityLevelNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetSecurityLevelNew";
}

/**
 * @tc.name: RemoveDeviceDataNew_001
 * @tc.desc: Remove device data.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, RemoveDeviceDataNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin RemoveDeviceDataNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        kvStore->dbStore_= nullptr;
        EXPECT_TRUE(kvStore->dbStore_ == nullptr);
        Status status = kvStore->RemoveDeviceData("testdevice");
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by RemoveDeviceDataNew_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end RemoveDeviceDataNew_001";
}

/**
 * @tc.name: RemoveDeviceDataNew_002
 * @tc.desc: Remove device data.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, RemoveDeviceDataNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin RemoveDeviceDataNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        Status status = kvStore->RemoveDeviceData("testdevice");
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by RemoveDeviceDataNew_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end RemoveDeviceDataNew_002";
}

/**
 * @tc.name: CloudSyncNew_001
 * @tc.desc: Cloud sync.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, CloudSyncNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin CloudSyncNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        AsyncDetail asyncDetail;
        Status status = kvStore->CloudSync(asyncDetail);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by CloudSyncNew_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end CloudSyncNew_001";
}

/**
 * @tc.name: CloudSyncNew_002
 * @tc.desc: Cloud sync.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, CloudSyncNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin CloudSyncNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::shared_ptr<KVDBServiceClient> ser = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(ser != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(ser));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        AsyncDetail asyncDetail;
        Status status = kvStore->CloudSync(asyncDetail);
        EXPECT_TRUE(status == ILLEGAL_STATE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by CloudSyncNew_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end CloudSyncNew_002";
}

/**
 * @tc.name: SetSyncParamNew
 * @tc.desc: Set sync param.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, SetSyncParamNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin SetSyncParamNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        KvSyncParam syncParam{ 500 };
        Status status = kvStore->SetSyncParam(syncParam);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by SetSyncParamNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end SetSyncParamNew";
}

/**
 * @tc.name: GetSyncParamNew
 * @tc.desc: Get sync param.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetSyncParamNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetSyncParamNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        KvSyncParam syncParam;
        Status status = kvStore->GetSyncParam(syncParam);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetSyncParamNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetSyncParamNew";
}

/**
 * @tc.name: SetCapabilityEnabledNew_001
 * @tc.desc: Set capability enabled.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, SetCapabilityEnabledNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin SetCapabilityEnabledNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        Status status = kvStore->SetCapabilityEnabled(false);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by SetCapabilityEnabledNew_001.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end SetCapabilityEnabledNew_001";
}

/**
 * @tc.name: SetCapabilityEnabledNew_002
 * @tc.desc: Set capability enabled.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, SetCapabilityEnabledNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin SetCapabilityEnabledNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::shared_ptr<KVDBServiceClient> service = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(service != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(service));
        Status status = kvStore->SetCapabilityEnabled(true);
        EXPECT_TRUE(status == ERROR);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by SetCapabilityEnabledNew_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end SetCapabilityEnabledNew_002";
}

/**
 * @tc.name: SetCapabilityRangeNew
 * @tc.desc: Set capability range.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, SetCapabilityRangeNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin SetCapabilityRangeNew";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        std::vector<std::string> localLabels{"local", "near"};
        std::vector<std::string> remoteLabels{"remote", "far"};
        Status status = kvStore->SetCapabilityRange(localLabels, remoteLabels);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by SetCapabilityRangeNew.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end SetCapabilityRangeNew";
}

/**
 * @tc.name: SubscribeWithQueryNew_001
 * @tc.desc: Subscribe with query.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, SubscribeWithQueryNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin SubscribeWithQueryNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        std::vector<std::string> devices{"dev1", "dev2"};
        DataQuery query;
        Status status = kvStore->SubscribeWithQuery(devices, query);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by SubscribeWithQueryNew_001";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end SubscribeWithQueryNew_001";
}

/**
 * @tc.name: SubscribeWithQueryNew_002
 * @tc.desc: Subscribe with query.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, SubscribeWithQueryNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin SubscribeWithQueryNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        std::shared_ptr<KVDBServiceClient> serv = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(serv != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(serv));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        std::vector<std::string> devices{"dev0", "dev1"};
        DataQuery query;
        Status status = kvStore->SubscribeWithQuery(devices, query);
        EXPECT_TRUE(status == ILLEGAL_STATE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by SubscribeWithQueryNew_002";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end SubscribeWithQueryNew_002";
}

/**
 * @tc.name: UnsubscribeWithQueryNew_001
 * @tc.desc: Unsubscribe with query.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, UnsubscribeWithQueryNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin UnsubscribeWithQueryNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        std::vector<std::string> devs{"dev0", "dev1"};
        DataQuery quer;
        Status status = kvStore->UnsubscribeWithQuery(devs, quer);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by UnsubscribeWithQueryNew_001";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end UnsubscribeWithQueryNew_001";
}

/**
 * @tc.name: UnsubscribeWithQueryNew_002
 * @tc.desc: Unsubscribe with query.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, UnsubscribeWithQueryNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin UnsubscribeWithQueryNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        EXPECT_NE(kv->dbStore_, nullptr);
        std::shared_ptr<KVDBServiceClient> serv = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(serv != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(serv));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        std::vector<std::string> devs{"dev3", "dev4"};
        DataQuery quer;
        Status status = kv->UnsubscribeWithQuery(devs, quer);
        EXPECT_TRUE(status == ILLEGAL_STATE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by UnsubscribeWithQueryNew_002";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end UnsubscribeWithQueryNew_002";
}

/**
 * @tc.name: RestoreNew_001
 * @tc.desc: restore kv.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, RestoreNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin RestoreNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        std::string baseDirect = "/data/service/el1/public/database/SingleStoreImplNew";
        std::string file = "test.txt";
        kv->isApplication_ = false;
        Status status = kv->Restore(file, baseDirect);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by RestoreNew_001";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end RestoreNew_001";
}

/**
 * @tc.name: RestoreNew_002
 * @tc.desc: restore kv.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, RestoreNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin RestoreNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        std::shared_ptr<KVDBServiceClient> serv = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(serv != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(serv));
        std::string baseDirect = "/data/service/el1/public/database/SingleStoreImplNew";
        std::string file = "test1.txt";
        kv->isApplication_ = true;
        kv->apiVersion_ = 15;
        Status status = kv->Restore(file, baseDirect);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by RestoreNew_002";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end RestoreNew_002";
}

/**
 * @tc.name: GetResultSetNew
 * @tc.desc: Get result Set.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetResultSetNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetResultSetNew";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        kv->dbStore_= nullptr;
        EXPECT_TRUE(kv->dbStore_ == nullptr);
        SingleStoreImpl::DBQuery dbQuer;
        std::shared_ptr<KvStoreResultSet> output;
        Status status = kv->GetResultSet(dbQuer, output);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetResultSetNew";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetResultSetNew";
}

/**
 * @tc.name: GetEntriesNew
 * @tc.desc: Get entries.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetEntriesNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetEntriesNew";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        kv->dbStore_= nullptr;
        EXPECT_TRUE(kv->dbStore_ == nullptr);
        std::vector<Entry> vects;
        SingleStoreImpl::DBQuery dbQuer;
        Status status = kv->GetEntries(dbQuer, vects);
        EXPECT_TRUE(status == ALREADY_CLOSED);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetEntriesNew";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetEntriesNew";
}

/**
 * @tc.name: DoSyncNew_001
 * @tc.desc: do sync.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, DoSyncNew_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin DoSyncNew_001";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        kv->isClientSync_ = false;
        ASSERT_FALSE(kv->isClientSync_);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        SingleStoreImpl::SyncInfo syInfo;
        std::shared_ptr<SingleStoreImpl::SyncCallback> obser;
        auto res = kv->DoSync(syInfo, obser);
        EXPECT_TRUE(res == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by DoSyncNew_001";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end DoSyncNew_001";
}

/**
 * @tc.name: DoSyncNew_002
 * @tc.desc: do sync.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, DoSyncNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin DoSyncNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        kv->isClientSync_ = false;
        std::shared_ptr<KVDBServiceClient> servic = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(servic != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(servic));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        SingleStoreImpl::SyncInfo syInfo;
        std::shared_ptr<SingleStoreImpl::SyncCallback> observer;
        auto res = kv->DoSync(syInfo, observer);
        EXPECT_TRUE(res == ILLEGAL_STATE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by DoSyncNew_002";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end DoSyncNew_002";
}

/**
 * @tc.name: SetConfigNew
 * @tc.desc: set config.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, SetConfigNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin SetConfigNew";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        StoreConfig storeConfig;
        auto res = kv->SetConfig(storeConfig);
        EXPECT_TRUE(res == SERVER_UNAVAILABLE);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by SetConfigNew";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end SetConfigNew";
}

/**
 * @tc.name: DoNotifyChangeNew
 * @tc.desc: Do notify change.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, DoNotifyChangeNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin DoNotifyChangeNew";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        kv->cloudAutoSync_ = false;
        kv->DoNotifyChange();
        EXPECT_TRUE(!kv->cloudAutoSync_);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by DoNotifyChangeNew";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end DoNotifyChangeNew";
}

/**
 * @tc.name: IsRebuildNew
 * @tc.desc: is rebuild.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, IsRebuildNew, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin IsRebuildNew";
    try {
        std::shared_ptr<SingleStoreImpl> kv = CreateKVStore(false, false);
        ASSERT_TRUE(kv != nullptr);
        kv->dbStore_= nullptr;
        EXPECT_TRUE(kv->dbStore_ == nullptr);
        auto res = kv->IsRebuild();
        EXPECT_FALSE(res);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by IsRebuildNew";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end IsRebuildNew";
}































/**
 * @tc.name: GetDeviceEntriesNew_002
 * @tc.desc: Get device entries with valid store.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplMockNew, GetDeviceEntriesNew_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-begin GetDeviceEntriesNew_002";
    try {
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_TRUE(kvStore != nullptr);
        EXPECT_NE(kvStore->dbStore_, nullptr);
        
        // Test with valid dbStore_ and mock device manager
        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return(""));
        std::string device = "test device";
        std::vector<Entry> vcs;
        Status status = kvStore->GetDeviceEntries(device, vcs);
        EXPECT_TRUE(status == ERROR); // Expecting ERROR when device UUID is empty
        
        // Test with valid device UUID but null service agent
        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return("valid_device_uuid"));
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(nullptr));
        status = kvStore->GetDeviceEntries(device, vcs);
        EXPECT_TRUE(status == SERVER_UNAVAILABLE); // Expecting SERVER_UNAVAILABLE when service is unavailable
        
        // Test with valid device and service client
        std::shared_ptr<KVDBServiceClient> client = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_TRUE(client != nullptr);
        EXPECT_CALL(*devManagerMock, ToUUID(_)).WillOnce(Return("valid_device_uuid"));
        EXPECT_CALL(*kVDBServiceClientMock, GetInstance()).WillOnce(Return(client));
        EXPECT_CALL(*kVDBServiceClientMock, GetServiceAgent(_)).WillOnce(Return(nullptr));
        status = kvStore->GetDeviceEntries(device, vcs);
        EXPECT_TRUE(status == ILLEGAL_STATE); // Expecting ILLEGAL_STATE when service agent is null
        
        for (int i = 0; i < 2; ++i) {
            Entry entry;
            entry.key = std::to_string(i).append("_key");
            entry.value = std::to_string(i).append("_val");
            vcs.emplace_back(entry);
        }
        // Verify the vector has entries
        EXPECT_EQ(vcs.size(), 2);
        
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "SingleStoreImplMockNew-an exception occurred by GetDeviceEntriesNew_002.";
    }
    GTEST_LOG_(INFO) << "SingleStoreImplMockNew-end GetDeviceEntriesNew_002";
}

} // namespace OHOS::DistributedKv
