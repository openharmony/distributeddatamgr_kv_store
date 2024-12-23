/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include <unistd.h>
#include <memory>
#include <sys/types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "kvstore_observer.h"
#include "single_store_impl.h"
#include "include/accesstoken_kit_mock.h"
#include "include/convertor_mock.h"
#include "store_factory.h"
#include "store_manager.h"
#include "include/dev_manager_mock.h"
#include "include/observer_bridge_mock.h"
#include "include/task_executor_mock.h"
#include "include/kvdb_notifier_client_mock.h"
#include "include/kvdb_service_client_mock.h"

namespace OHOS::DistributedKv {
using namespace testing;
using namespace std;
using namespace Security::AccessToken;
using namespace DistributedDB;
static StoreId stId = { "test_single" };
static AppId apId = { "appid" };

class SingleStoreImplMockUnitTest : public testing::Test {
public:
    static void TearDownTestCase();
    static void SetUpTestCase(void);
    void TearDown() override;
    void SetUp() override;
    using DBStatus = DistributedDB::DBStatus;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using Observer = DistributedKv::KvStoreObserver;
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock_ = nullptr;
    static inline shared_ptr<ConvertorMock> convertorMock_ = nullptr;
    static inline shared_ptr<ObserverBridgeMock> observerBridgeMock_ = nullptr;
    static inline shared_ptr<DevManagerMock> devManagerMock_ = nullptr;
    static inline shared_ptr<KVDBServiceClientMock> kVDBServiceClientMock_ = nullptr;
    static inline shared_ptr<KVDBNotifierClientMock> kVDBNotifierClientMock_ = nullptr;
    static inline shared_ptr<TaskExecutorMock> taskExecutorMock_ = nullptr;
    std::shared_ptr<SingleStoreImpl> CreateKVStore(bool autosync = false, bool backup = true);
};

void SingleStoreImplMockUnitTest::SetUp() {}
void SingleStoreImplMockUnitTest::TearDown() {}

void SingleStoreImplMockUnitTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase enter";
    string bDir = "/data/service/el1/public/database/SingleStoreImplMockUnitTest";
    mkdir(bDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    devManagerMock_ = make_shared<DevManagerMock>();
    BDevManager::devManager = devManagerMock_;
    kVDBServiceClientMock_ = make_shared<KVDBServiceClientMock>();
    BKVDBServiceClient::kVDBServiceClient = kVDBServiceClientMock_;
    kVDBNotifierClientMock_ = make_shared<KVDBNotifierClientMock>();
    BKVDBNotifierClient::kVDBNotifierClient = kVDBNotifierClientMock_;
    observerBridgeMock_ = make_shared<ObserverBridgeMock>();
    BObserverBridge::observerBridge = observerBridgeMock_;
    taskExecutorMock_ = make_shared<TaskExecutorMock>();
    BTaskExecutor::taskExecutor = taskExecutorMock_;
    accessTokenKitMock_ = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenKit = accessTokenKitMock_;
    convertorMock_ = make_shared<ConvertorMock>();
    BConvertor::convertor = convertorMock_;
    GTEST_LOG_(INFO) << "SetUpTestCase exit";
}

void SingleStoreImplMockUnitTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase enter";
    BDevManager::devManager = nullptr;
    devManagerMock_ = nullptr;
    BKVDBServiceClient::kVDBServiceClient = nullptr;
    kVDBServiceClientMock_ = nullptr;
    BKVDBNotifierClient::kVDBNotifierClient = nullptr;
    kVDBNotifierClientMock_ = nullptr;
    BObserverBridge::observerBridge = nullptr;
    observerBridgeMock_ = nullptr;
    BTaskExecutor::taskExecutor = nullptr;
    taskExecutorMock_ = nullptr;
    BAccessTokenKit::accessTokenKit = nullptr;
    accessTokenKitMock_ = nullptr;
    BConvertor::convertor = nullptr;
    convertorMock_ = nullptr;
    std::string bDir = "/data/service/el1/public/database/SingleStoreImplMockUnitTest";
    (void)remove("/data/service/el1/public/database/SingleStoreImplMockUnitTest");
    GTEST_LOG_(INFO) << "TearDownTestCase exit";
}

std::shared_ptr<SingleStoreImpl> SingleStoreImplMockUnitTest::CreateKVStore(bool autoSync, bool backup)
{
    AppId apId = { "SingleStoreImplMockTest" };
    StoreId stId = { "DestructorTest" };
    Options option;
    option.kvStoreType = SINGLE_VERSION;
    option.area = EL1;
    option.securityLevel = S1;
    option.autoSync = autoSync;
    option.backup = backup;
    option.baseDir = "/data/service/el1/public/database/SingleStoreImplMockUnitTest";
    std::shared_ptr<SingleStoreImpl> kvStor;
    StoreFactory storFact;
    auto dbMgr = storFact.GetDBManager(option.baseDir, apId);
    auto dbPwd = SecurityManager::GetInstance().GetDBPassword(stId.stId, option.baseDir, option.encrypt);
    DBStatus dbStats = DBStatus::DB_ERROR;
    dbMgr->GetKvStore(stId, storFact.GetDBOption(option, dbPwd),
        [&dbMgr, &kvStor, &apId, &dbStats, &option, &storFact](auto status, auto *stor) {
            dbStats = status;
            if (stor == nullptr) {
                return;
            }
            auto release = [dbMgr](auto *stor) {
                dbMgr->CloseKvStore(stor);
            };
            auto dbStor = std::shared_ptr<DBStore>(stor, release);
            storFact.SetDbConfig(dbStor);
            const Convertor &convert = *(storFact.convertors_[option.kvStoreType]);
            kvStor = std::make_shared<SingleStoreImpl>(dbStor, apId, option, convert);
        });
    return kvStor;
}

/**
 * @tc.name: IsRemoteChanged001
 * @tc.desc: test is not remote changed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(SingleStoreImplMockUnitTest, IsRemoteChanged001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsRemoteChanged001 of SingleStoreImplMockUnitTest-begin ";
    try {
        EXPECT_CALL(*taskExecutorMock_, Schedule(_, _, _, _)).Times(AnyNumber());
        std::shared_ptr<SingleStoreImpl> kvSt;
        kvSt = CreateKVStore();
        ASSERT_EQ(kvSt, nullptr);
        std::shared_ptr<KVDBServiceClient> cli = make_shared<KVDBServiceClient>(nullptr);
        ASSERT_EQ(cli, nullptr);
        EXPECT_CALL(*devManagerMock_, ToUUID(_)).WillOnce(Return("cccc"));
        bool ret = kvSt->IsRemoteChanged("afafgafga");
        EXPECT_TRUE(ret);
        EXPECT_CALL(*devManagerMock_, ToUUID(_)).WillOnce(Return("XXXXX"));
        EXPECT_CALL(*kVDBServiceClientMock_, GetInstance()).WillOnce(Return(nullptr));
        ret = kvSt->IsRemoteChanged("qwdfafaf");
        EXPECT_FALSE(ret);
        sptr<KVDBNotifierClient> testAgent = new (std::nothrow) KVDBNotifierClient();
        ASSERT_NE(testAgent, nullptr);
        EXPECT_CALL(*devManagerMock_, ToUUID(_)).WillOnce(Return("vsjbmsjwsmh"));
        EXPECT_CALL(*kVDBServiceClientMock_, GetInstance()).WillOnce(Return(cli));
        EXPECT_CALL(*kVDBServiceClientMock_, GetServiceAgent(_)).WillOnce(Return(testAgent));
        EXPECT_CALL(*kVDBNotifierClientMock_, IsChanged(_, _)).WillOnce(Return(false));
        ret = kvSt->IsRemoteChanged("cajgkmghkh");
        EXPECT_FALSE(ret);
        EXPECT_CALL(*devManagerMock_, ToUUID(_)).WillOnce(Return("faggsag"));
        EXPECT_CALL(*kVDBServiceClientMock_, GetInstance()).WillOnce(Return(cli));
        EXPECT_CALL(*kVDBServiceClientMock_, GetServiceAgent(_)).WillOnce(Return(nullptr));
        ret = kvSt->IsRemoteChanged("xafafgg");
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(INFO) << "IsRemoteChanged001 of SingleStoreImplMockUnitTest happend an exception occurred.";
    }
    GTEST_LOG_(INFO) << "IsRemoteChanged001 of SingleStoreImplMockUnitTest-end";
}

/**
 * @tc.name: OnRemoteDied001
 * @tc.desc: remote died.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(SingleStoreImplMockUnitTest, OnRemoteDied001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnRemoteDied001 of SingleStoreImplMockUnitTest-begin";
    try {
        EXPECT_CALL(*taskExecutorMock_, Schedule(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*accessTokenKitMock_, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_INVALID));
        std::shared_ptr<SingleStoreImpl> kvSto;
        kvSto = CreateKVStore(false, false);
        ASSERT_EQ(kvSto, nullptr);
        EXPECT_EQ(kvSto->dbStore_, nullptr);
        EXPECT_TRUE(kvSto->isApplication_);
        kvSto->taskId_ = 2; // assign for 2
        kvSto->OnRemoteDied();
        kvSto->taskId_ = 3; // assign for 3
        shared_ptr<Observer> obser = make_shared<Observer>();
        shared_ptr<Observer> obser1 = make_shared<Observer>();
        Convertor cvt0;
        Convertor cvt2;
        shared_ptr<ObserverBridge> obsBridge1 = make_shared<ObserverBridge>(apId, stId, obser, cvt0);
        shared_ptr<ObserverBridge> obsBridge1 = make_shared<ObserverBridge>(apId, stId, obser1, cvt2);

        uint32_t firs = 4; // assign for the first time
        firs |= SUBSCRIBE_TYPE_REMOTE;
        pair<uint32_t, std::shared_ptr<ObserverBridge>> one(0, obsBridge1);
        pair<uint32_t, std::shared_ptr<ObserverBridge>> two(firs, obsBridge1);

        kvSto->observers_.Insert(uintptr_t(obser.get()), one);
        kvSto->observers_.Insert(uintptr_t(obser1.get()), two);
        EXPECT_CALL(*observerBridgeMock_, OnServiceDeath()).Times(AnyNumber());
        EXPECT_CALL(*taskExecutorMock_, Schedule(_, _, _, _)).WillOnce(Return(0));
        kvSto->OnRemoteDied();
        kvSto->observers_.Erase(uintptr_t(obser.get()));
        kvSto->observers_.Erase(uintptr_t(obser1.get()));
        EXPECT_TRUE(kvSto->taskId_ == 0);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "OnRemoteDied001 of SingleStoreImplMockUnitTest happend-an exception.";
    }
    GTEST_LOG_(INFO) << "OnRemoteDied001 of  SingleStoreImplMockUnitTest-end";
}

/**
 * @tc.name: Register001
 * @tc.desc: register.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(SingleStoreImplMockUnitTest, Register001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "Register001 of SingleStoreImplMockUnitTest-begin ";
    try {
        EXPECT_CALL(*taskExecutorMock_, Schedule(_, _, _, _)).Times(1);
        EXPECT_CALL(*accessTokenKitMock_, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_HAP));
        std::shared_ptr<SingleStoreImpl> kvStre;
        kvStre = CreateKVStore(true, true);
        ASSERT_EQ(kvStre, nullptr);
        EXPECT_EQ(kvStre->dbStore_, nullptr);
        EXPECT_FALSE(kvStre->isApplication_);
        shared_ptr<Observer> observer0 = make_shared<Observer>();
        shared_ptr<Observer> observer2 = make_shared<Observer>();
        Convertor cvt0;
        Convertor cvt0;
        shared_ptr<ObserverBridge> obsBrdge = make_shared<ObserverBridge>(apId, stId, observer0, cvt0);
        shared_ptr<ObserverBridge> obsBrdge1 = make_shared<ObserverBridge>(apId, stId, observer2, cvt0);
        uint32_t firs = 0;
        pair<uint32_t, std::shared_ptr<ObserverBridge>> fisrt(0, obsBrdge);
        pair<uint32_t, std::shared_ptr<ObserverBridge>> sec(firs, obsBrdge1);
        kvStre->observers_.Insert(uintptr_t(observer0.get()), fisrt);
        kvStre->observers_.Insert(uintptr_t(observer2.get()), sec);
        EXPECT_CALL(*observerBridgeMock_, RegisterRemoteObserver(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*taskExecutorMock_, Schedule(_, _, _, _)).WillOnce(Return(0));
        kvStre->Register();
        EXPECT_FALSE(kvStre->taskId_ == 0);
        EXPECT_CALL(*observerBridgeMock_, RegisterRemoteObserver(_)).WillOnce(Return(ERROR));
        kvStre->Register();
        kvStre->observers_.Erase(uintptr_t(observer0.get()));
        kvStre->observers_.Erase(uintptr_t(observer2.get()));
        EXPECT_FALSE(kvStre->taskId_ == 0);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "Register001 of SingleStoreImplMockUnitTest happend-an exception.";
    }
    GTEST_LOG_(INFO) << "Register001 of SingleStoreImplMockUnitTest-end";
}

/**
* @tc.name: put001
* @tc.desc: Put.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wang dong
*/
HWTEST_F(SingleStoreImplMockUnitTest, put001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "put001 of SingleStoreImplMockUnitTest-begin";
    try {
        EXPECT_CALL(*taskExecutorMock_, Schedule(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*accessTokenKitMock_, GetTokenTypeFlag(_)).Times(1);
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(true, false);
        ASSERT_EQ(kvStore, nullptr);
        EXPECT_EQ(kvStore->dbStore_, nullptr);
        std::vector<uint8_t> vec;
        EXPECT_CALL(*convertorMock_, ToLocalDBKey(_)).WillOnce(Return(vec));
        size_t maxTestLen = 8;
        std::string str(maxTestLen, 'a');
        Blob key(str);
        Blob val("testvalue");
        Status stats = kvStore->Put(key, val);
        EXPECT_FALSE(stats == INVALID_ARGUMENT);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "put001 of SingleStoreImplMockUnitTest happend-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "put001 of SingleStoreImplMockUnitTest-end";
}

/**
* @tc.name: put002
* @tc.desc: Put.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wang dong
*/
HWTEST_F(SingleStoreImplMockUnitTest, put002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "put002 of SingleStoreImplMockUnitTest-begin";
    try {
        EXPECT_CALL(*taskExecutorMock_, Schedule(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*accessTokenKitMock_, GetTokenTypeFlag(_)).Times(AnyNumber());
        std::shared_ptr<SingleStoreImpl> kvStore = CreateKVStore(false, false);
        ASSERT_EQ(kvStore, nullptr);
        EXPECT_EQ(kvStore->dbStore_, nullptr);
        std::vector<uint8_t> vect0{3, 8};
        EXPECT_CALL(*convertorMock_, ToLocalDBKey(_)).WillOnce(Return(vect0));
        size_t longTestLen = 4 * 1024 * 1024 + 1;
        std::string str(longTestLen, 'b');
        Blob key0("key1");
        Blob key2(str);
        Status statu = kvStore->Put(key0, key2);
        EXPECT_FALSE(statu == INVALID_ARGUMENT);
    } catch (...) {
        EXPECT_FALSE(true);
        GTEST_LOG_(INFO) << "put002 of SingleStoreImplMockUnitTest happend-an exception.";
    }
    GTEST_LOG_(INFO) << "put002 of SingleStoreImplMockUnitTest-end";
}
}