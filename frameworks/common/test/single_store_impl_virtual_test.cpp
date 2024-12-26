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
#include <condition_variable>
#include <gtest/gtest.h>
#include <vector>

#include "block_data.h"
#include "device_manager.h"
#include "distributed_kv_data_manager.h"
#include "dev_manager.h"
#include "dm_device_info.h"
#include "file_ex.h"
#include "kv_store_nb_delegate.h"
#include "single_store_impl.h"
#include "store_factory.h"
#include "store_manager.h"
#include "sys/stat.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using DevInfo = OHOS::DistributedHardware::DmDeviceInfo;
using DBStatus = DistributedDB::DBStatus;
using DBStore = DistributedDB::KvStoreNbDelegate;
using SyncCallback = KvStoreSyncCallback;
namespace OHOS::Test {

std::vector<uint8_t> Random(int32_t len)
{
    return std::vector<uint8_t>(len, 'aaaa');
}

class SingleStoreImplVirtualTest : public testing::Test {
public:
    class TestObserverVirtual : public KvStoreObserver {
    public:
        TestObserverVirtual()
        {
            // The time interval parameter is 5.
            data = std::make_shared<OHOS::BlockData<bool>>(5, false);
        }
        void OnChange(const ChangeNotification &notification) override
        {
            insertVirtual = notification.GetInsertEntries();
            updateVirtual = notification.GetUpdateEntries();
            deleteVirtual = notification.GetDeleteEntries();
            deviceId = notification.GetDeviceId();
            bool valueTest = true;
            data->SetValue(valueTest);
        }
        std::vector<Entry> insertVirtual;
        std::vector<Entry> updateVirtual;
        std::vector<Entry> deleteVirtual;
        std::string deviceId;
        std::shared_ptr<OHOS::BlockData<bool>> data;
    };

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<SingleKvStore> kvStoreImpl;
    static constexpr int MAX_RESULTSET_SIZE_VIRTUAL = 8;
    std::shared_ptr<SingleKvStore> CreateKVStoreDB(std::string storeIdTest, KvStoreType type, bool encrypt, bool backup);
    std::shared_ptr<SingleStoreImpl> CreateKVStoreDB(bool autosync = false);
};

void SingleStoreImplVirtualTest::SetUpTestCase(void)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    mkdir(baseDirTest.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SingleStoreImplVirtualTest::TearDownTestCase(void)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    StoreManagerTest::GetInstance().Delete({ "SingleStoreImplVirtualTest" }, { "SingleKVStore" }, baseDirTest);
    (void)remove("/data/service/el1/public/database/SingleStoreImplVirtualTest/key");
    (void)remove("/data/service/el1/public/database/SingleStoreImplVirtualTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleStoreImplVirtualTest");
}

void SingleStoreImplVirtualTest::SetUp(void)
{
    kvStoreImpl = CreateKVStoreDB("SingleKVStore", SINGLE_VERSION, false, true);
    if (kvStoreImpl == nullptr) {
        kvStoreImpl = CreateKVStoreDB("SingleKVStore", SINGLE_VERSION, false, true);
    }
    EXPECT_NE(kvStoreImpl, nullptr);
}

void SingleStoreImplVirtualTest::TearDown(void)
{
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStore" };
    kvStoreImpl = nullptr;
    auto statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
    auto baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, SUCCESS);
}

std::shared_ptr<SingleStoreImpl> SingleStoreImplVirtualTest::CreateKVStoreDB(bool autosync)
{
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "DestructorTest" };
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S2;
    optionsTest.area = EL1;
    optionsTest.autoSync = autosync;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    StoreFactory storeFactoryTest;
    auto dbManagerTest = storeFactoryTest.GetDBManager(optionsTest.baseDirTest, appId11);
    auto dbPasswordTest = SecurityManager::GetInstance().GetDBPassword(storeId11.storeId11, optionsTest.baseDirTest, optionsTest.encrypt);
    DBStatus dbStatus = DBStatus::DB_ERROR;
    dbManagerTest->GetKvStore(storeId11, storeFactoryTest.GetDBOption(optionsTest, dbPasswordTest),
        [&dbManagerTest, &singleKvStore, &appId11, &dbStatus, &optionsTest, &storeFactoryTest](auto statusTest, auto *store) {
            dbStatus = statusTest;
            if (store == nullptr) {
                return;
            }
            auto release = [dbManagerTest](auto *store) {
                dbManagerTest->CloseKvStore(store);
            };
            auto dbStore = std::shared_ptr<DBStore>(store, release);
            storeFactoryTest.SetDbConfig(dbStore);
            const Convertor &convertor = *(storeFactoryTest.convertors_[optionsTest.kvStoreType]);
            singleKvStore = std::make_shared<SingleStoreImpl>(dbStore, appId11, optionsTest, convertor);
        });
    return singleKvStore;
}

std::shared_ptr<SingleKvStore> SingleStoreImplVirtualTest::CreateKVStoreDB(
    std::string storeIdTest, KvStoreType type, bool encrypt, bool backup)
{
    Options optionsTest;
    optionsTest.kvStoreType = type;
    optionsTest.securityLevel = S1;
    optionsTest.encrypt = encrypt;
    optionsTest.area = EL1;
    optionsTest.backup = backup;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";

    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { storeIdTest };
    Status statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, optionsTest.baseDirTest);
    return StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
}

/**
 * @tc.name: GetStoreId
 * @tc.desc: get the store id of the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, GetStoreIdTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto storeId11 = kvStoreImpl->GetStoreId();
    EXPECT_EQ(storeId11.storeId11, "SingleKVStore");
}

/**
 * @tc.name: Put
 * @tc.desc: put key-valueTest data to the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, PutTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto statusTest = kvStoreImpl->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->Put({ "   Put Test" }, { "Put2 Value" });
    EXPECT_EQ(statusTest, SUCCESS);
    Value valueTest;
    statusTest = kvStoreImpl->Get({ "Put Test" }, valueTest);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(valueTest.ToString(), "Put2 Value");
}

/**
 * @tc.name: Put_Invalid_Key
 * @tc.desc: put invalid key-valueTest data to the device kv store and single kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, Put_Invalid_KeyTest, TestSize.Level0)
{
    std::shared_ptr<SingleKvStore> singleKvStore;
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "DeviceKVStore" };
    singleKvStore = CreateKVStoreDB(storeId11.storeId11, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(singleKvStore, nullptr);

    size_t maxDevKeyLen = 897;
    std::string str(maxDevKeyLen, 'a');
    Blob key(str);
    Blob valueTest("test_value");
    Status statusTest = singleKvStore->Put(key, valueTest);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);

    Blob key1("");
    Blob value1("test_value1");
    statusTest = singleKvStore->Put(key1, value1);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);

    singleKvStore = nullptr;
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, SUCCESS);

    size_t maxSingleKeyLen = 1025;
    std::string str1(maxSingleKeyLen, 'b');
    Blob key2(str1);
    Blob value2("test_value2");
    statusTest = kvStoreImpl->Put(key2, value2);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
    statusTest = kvStoreImpl->Put(key1, value1);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: PutBatch
 * @tc.desc: put some key-valueTest data to the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, PutBatchTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> entriesTest;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        entriesTest.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(entriesTest);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: IsRebuild
 * @tc.desc: test IsRebuild
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, IsRebuildTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto statusTest = kvStoreImpl->IsRebuild();
    EXPECT_EQ(statusTest, false);
}

/**
 * @tc.name: PutBatch001
 * @tc.desc: entry111.valueTest.Size() > MAX_VALUE_LENGTH
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, PutBatch001Test, TestSize.Level1)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    size_t totalLength = SingleStoreImpl::MAX_VALUE_LENGTH + 1; // create an out-of-limit large number
    char fillChar = 'a';
    std::string longString(totalLength, fillChar);
    std::vector<Entry> entriesTest;
    Entry entry111;
    entry111.key = "PutBatch001_test";
    entry111.valueTest = longString;
    entriesTest.push_back(entry111);
    auto statusTest = kvStoreImpl->PutBatch(entriesTest);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
    entriesTest.clear();
    Entry entrys;
    entrys.key = "";
    entrys.valueTest = "PutBatch_test";
    entriesTest.push_back(entrys);
    statusTest = kvStoreImpl->PutBatch(entriesTest);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: Delete
 * @tc.desc: deleteVirtual the valueTest of the key
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto statusTest = kvStoreImpl->Put({ "Put Test001" }, { "Put Value" });
    EXPECT_EQ(statusTest, SUCCESS);
    Value valueTest;
    statusTest = kvStoreImpl->Get({ "Put Test001" }, valueTest);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(std::string("Put Value"), valueTest.ToString());
    statusTest = kvStoreImpl->Delete({ "Put Test001" });
    EXPECT_EQ(statusTest, SUCCESS);
    valueTest = {};
    statusTest = kvStoreImpl->Get({ "Put Test001" }, valueTest);
    EXPECT_EQ(statusTest, KEY_NOT_FOUND);
    EXPECT_EQ(std::string(""), valueTest.ToString());
}

/**
 * @tc.name: DeleteBatch
 * @tc.desc: deleteVirtual the values of the keys
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteBatchTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> entriesTest;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_vv");
        entriesTest.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(entriesTest);
    EXPECT_EQ(statusTest, SUCCESS);
    std::vector<Key> keys;
    for (int i = 0; i < 10; ++i) {
        Key key = std::to_string(i).append("_kk");
        keys.push_back(key);
    }
    statusTest = kvStoreImpl->DeleteBatch(keys);
    EXPECT_EQ(statusTest, SUCCESS);
    for (int i = 0; i < 10; ++i) {
        Value valueTest;
        statusTest = kvStoreImpl->Get(keys[i], valueTest);
        EXPECT_EQ(statusTest, KEY_NOT_FOUND);
        EXPECT_EQ(valueTest.ToString(), std::string(""));
    }
}

/**
 * @tc.name: Transaction
 * @tc.desc: do transaction
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, TransactionTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto statusTest = kvStoreImpl->StartTransaction();
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->Commit();
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->StartTransaction();
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->Rollback();
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: SubscribeKvStore
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, SubscribeKvStoreTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto observerVirtual = std::make_shared<TestObserverVirtual>();
    auto statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observerVirtual);
    EXPECT_EQ(statusTest, STORE_ALREADY_SUBSCRIBE);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observerVirtual);
    EXPECT_EQ(statusTest, STORE_ALREADY_SUBSCRIBE);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observerVirtual);
    EXPECT_EQ(statusTest, STORE_ALREADY_SUBSCRIBE);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
    EXPECT_EQ(statusTest, STORE_ALREADY_SUBSCRIBE);
    bool invalidValue = false;
    observerVirtual->data->Clear(invalidValue);
    statusTest = kvStoreImpl->Put({ "Put Test002" }, { "Put Value" });
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_TRUE(observerVirtual->data->GetValue());
    EXPECT_EQ(observerVirtual->insertVirtual.size(), 1);
    EXPECT_EQ(observerVirtual->updateVirtual.size(), 0);
    EXPECT_EQ(observerVirtual->deleteVirtual.size(), 0);
    observerVirtual->data->Clear(invalidValue);
    statusTest = kvStoreImpl->Delete({ "Put Test002" });
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_TRUE(observerVirtual->data->GetValue());
    EXPECT_EQ(observerVirtual->insertVirtual.size(), 0);
    EXPECT_EQ(observerVirtual->updateVirtual.size(), 0);
    EXPECT_EQ(observerVirtual->deleteVirtual.size(), 1);
    statusTest = kvStoreImpl->Put({ "Put Test002" }, { "Put Value1" });
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_TRUE(observerVirtual->data->GetValue());
    EXPECT_EQ(observerVirtual->insertVirtual.size(), 0);
    EXPECT_EQ(observerVirtual->updateVirtual.size(), 1);
    EXPECT_EQ(observerVirtual->deleteVirtual.size(), 0);
    observerVirtual->data->Clear(invalidValue);
}

/**
 * @tc.name: SubscribeKvStore002
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, SubscribeKvStore002Test, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::shared_ptr<TestObserverVirtual> subscribedObserverTest;
    std::shared_ptr<TestObserverVirtual> unSubscribedObserverTest;
    for (int i = 0; i < 15; ++i) {
        auto observerVirtual = std::make_shared<TestObserverVirtual>();
        auto status1 = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
        auto status2 = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observerVirtual);
        if (i < 8) {
            EXPECT_EQ(status1, SUCCESS);
            EXPECT_EQ(status2, SUCCESS);
            subscribedObserverTest = observerVirtual;
        } else {
            EXPECT_EQ(status1, OVER_MAX_LIMITS);
            EXPECT_EQ(status2, OVER_MAX_LIMITS);
            unSubscribedObserverTest = observerVirtual;
        }
    }

    auto statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserverTest);
    EXPECT_EQ(statusTest, STORE_ALREADY_SUBSCRIBE);

    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, {});
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);

    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, subscribedObserverTest);
    EXPECT_EQ(statusTest, STORE_ALREADY_SUBSCRIBE);

    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserverTest);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, unSubscribedObserverTest);
    EXPECT_EQ(statusTest, SUCCESS);
    subscribedObserverTest = unSubscribedObserverTest;
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserverTest);
    EXPECT_EQ(statusTest, SUCCESS);

    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserverTest);
    EXPECT_EQ(statusTest, SUCCESS);

    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserverTest);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserverTest);
    EXPECT_EQ(statusTest, SUCCESS);

    auto observerVirtual = std::make_shared<TestObserverVirtual>();
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    observerVirtual = std::make_shared<TestObserverVirtual>();
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observerVirtual);
    EXPECT_EQ(statusTest, OVER_MAX_LIMITS);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: SubscribeKvStore003
 * @tc.desc: isClientSync_
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, SubscribeKvStore003Test, TestSize.Level0)
{
    auto observerVirtual = std::make_shared<TestObserverVirtual>();
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB();
    EXPECT_NE(singleKvStore, nullptr);
    singleKvStore->isClientSync_ = true;
    auto statusTest = singleKvStore->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: UnsubscribeKvStore
 * @tc.desc: unsubscribe
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, UnsubscribeKvStoreTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto observerVirtual = std::make_shared<TestObserverVirtual>();
    auto statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observerVirtual);
    EXPECT_EQ(statusTest, STORE_NOT_SUBSCRIBE);
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observerVirtual);
    EXPECT_EQ(statusTest, STORE_NOT_SUBSCRIBE);
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observerVirtual);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observerVirtual);
    EXPECT_EQ(statusTest, STORE_NOT_SUBSCRIBE);
}

/**
 * @tc.name: GetEntries
 * @tc.desc: get entriesTest by prefix
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_PrefixTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> inputPragram;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::vector<Entry> outputPragram;
    statusTest = kvStoreImpl->GetEntries({ "" }, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::sort(outputPragram.begin(), outputPragram.end(), [](const Entry &entry111, const Entry &sentryTest) {
        return entry111.key.Data() < sentryTest.key.Data();
    });
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(inputPragram[i].key == outputPragram[i].key);
        EXPECT_TRUE(inputPragram[i].valueTest == outputPragram[i].valueTest);
    }
}

/**
 * @tc.name: GetEntries_Less_Prefix
 * @tc.desc: get entriesTest by prefix and the key size less than sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_Less_PrefixTest, TestSize.Level0)
{
    std::shared_ptr<SingleKvStore> singleKvStore;
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "DeviceKVStore" };
    singleKvStore = CreateKVStoreDB(storeId11.storeId11, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(singleKvStore, nullptr);

    std::vector<Entry> inputPragram;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        inputPragram.push_back(entry111);
    }
    auto statusTest = singleKvStore->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::vector<Entry> outputPragram;
    statusTest = singleKvStore->GetEntries({ "1" }, outputPragram);
    EXPECT_NE(outputPragram.empty(), true);
    EXPECT_EQ(statusTest, SUCCESS);

    singleKvStore = nullptr;
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, SUCCESS);

    statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::vector<Entry> outputPragram1;
    statusTest = kvStoreImpl->GetEntries({ "1" }, outputPragram1);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram1.empty(), true);
}

/**
 * @tc.name: GetEntries_Greater_Prefix
 * @tc.desc: get entriesTest by prefix and the key size is greater than  sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_Greater_PrefixTest, TestSize.Level0)
{
    std::shared_ptr<SingleKvStore> singleKvStore;
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "DeviceKVStore" };
    singleKvStore = CreateKVStoreDB(storeId11.storeId11, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(singleKvStore, nullptr);

    size_t keyLen = sizeof(uint32_t);
    std::vector<Entry> inputPragram;
    for (int i = 1; i < 10; ++i) {
        Entry entry111;
        std::string str(keyLen, i + '0');
        entry111.key = str;
        entry111.valueTest = std::to_string(i).append("_value");
        inputPragram.push_back(entry111);
    }
    auto statusTest = singleKvStore->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::vector<Entry> outputPragram;
    std::string str1(keyLen, '1');
    statusTest = singleKvStore->GetEntries(str1, outputPragram);
    EXPECT_NE(outputPragram.empty(), true);
    EXPECT_EQ(statusTest, SUCCESS);

    singleKvStore = nullptr;
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, SUCCESS);

    statusTest = kvStoreImpl->GetEntries(str1, outputPragram1);
    EXPECT_NE(outputPragram1.empty(), true);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::vector<Entry> outputPragram1;
}

/**
 * @tc.name: GetEntries_DataQuery
 * @tc.desc: get entriesTest by dataQuery
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_DataQueryTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> inputPragram;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    DataQuery dataQuery;
    dataQuery.InKeys({ "0_key", "1_key" });
    std::vector<Entry> outputPragram;
    statusTest = kvStoreImpl->GetEntries(dataQuery, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::sort(outputPragram.begin(), outputPragram.end(), [](const Entry &entry111, const Entry &sentryTest) {
        return entry111.key.Data() < sentryTest.key.Data();
    });
    ASSERT_LE(outputPragram.size(), 2);
    for (size_t i = 0; i < outputPragram.size(); ++i) {
        EXPECT_TRUE(inputPragram[i].key == outputPragram[i].key);
        EXPECT_TRUE(inputPragram[i].valueTest == outputPragram[i].valueTest);
    }
}

/**
 * @tc.name: GetResultSet
 * @tc.desc: get result set by prefix
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, GetResultSet_PrefixTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> inputPragram;
    auto cmp = [](const Key &entry111, const Key &sentryTest) {
        return entry111.Data() < sentryTest.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionaryMap(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        dictionaryMap[entry111.key] = entry111.valueTest;
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::shared_ptr<KvStoreResultSet> outputPragram;
    statusTest = kvStoreImpl->GetResultSet({ "" }, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(outputPragram->GetCount(), 10);
    EXPECT_NE(outputPragram, nullptr);
    int countSum = 0;
    while (outputPragram->MoveToNext()) {
        countSum++;
        Entry entry111;
        outputPragram->GetEntry(entry111);
        EXPECT_EQ(entry111.valueTest.Data(), dictionaryMap[entry111.key].Data());
    }
    EXPECT_EQ(countSum, outputPragram->GetCount());
}

/**
 * @tc.name: GetResultSet
 * @tc.desc: get result set by dataQuery
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, GetResultSet_QueryTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> inputPragram;
    auto cmp = [](const Key &entry111, const Key &sentryTest) {
        return entry111.Data() < sentryTest.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionaryMap(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        dictionaryMap[entry111.key] = entry111.valueTest;
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    DataQuery dataQuery;
    dataQuery.InKeys({ "0_key", "1_key" });
    std::shared_ptr<KvStoreResultSet> outputPragram;
    statusTest = kvStoreImpl->GetResultSet(dataQuery, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram, nullptr);
    ASSERT_LE(outputPragram->GetCount(), 2);
    int countSum = 0;
    while (outputPragram->MoveToNext()) {
        countSum++;
        Entry entry111;
        outputPragram->GetEntry(entry111);
        EXPECT_EQ(entry111.valueTest.Data(), dictionaryMap[entry111.key].Data());
    }
    EXPECT_EQ(countSum, outputPragram->GetCount());
}

/**
 * @tc.name: CloseResultSet
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseResultSetTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> inputPragram;
    auto cmp = [](const Key &entry111, const Key &sentryTest) {
        return entry111.Data() < sentryTest.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionaryMap(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        dictionaryMap[entry111.key] = entry111.valueTest;
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    DataQuery dataQuery;
    dataQuery.InKeys({ "0_key", "1_key" });
    std::shared_ptr<KvStoreResultSet> outputPragram;
    statusTest = kvStoreImpl->GetResultSet(dataQuery, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram, nullptr);
    ASSERT_LE(outputPragram->GetCount(), 2);
    auto outputTmp = outputPragram;
    statusTest = kvStoreImpl->CloseResultSet(outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(outputPragram, nullptr);
    EXPECT_EQ(outputTmp->GetPosition(), KvStoreResultSet::INVALID_POSITION);
    EXPECT_EQ(outputTmp->GetCount(), KvStoreResultSet::INVALID_COUNT);
    EXPECT_EQ(outputTmp->MoveToFirst(), false);
    EXPECT_EQ(outputTmp->MoveToNext(), false);
    EXPECT_EQ(outputTmp->MoveToLast(), false);
    EXPECT_EQ(outputTmp->MoveToPrevious(), false);
    EXPECT_EQ(outputTmp->Move(1), false);
    EXPECT_EQ(outputTmp->MoveToPosition(1), false);
    EXPECT_EQ(outputTmp->IsBeforeFirst(), false);
    EXPECT_EQ(outputTmp->IsAfterLast(), false);
    EXPECT_EQ(outputTmp->IsFirst(), false);
    EXPECT_EQ(outputTmp->IsLast(), false);
    Entry entry111;
    EXPECT_EQ(outputTmp->GetEntry(entry111), ALREADY_CLOSED);
}

/**
 * @tc.name: CloseResultSet001
 * @tc.desc: outputPragram = nullptr;
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseResultSet001Test, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::shared_ptr<KvStoreResultSet> outputPragram;
    outputPragram = nullptr;
    auto statusTest = kvStoreImpl->CloseResultSet(outputPragram);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: ResultSetMaxSizeTest
 * @tc.desc: test if kv supports 8 resultSets at the same time
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, ResultSetMaxSizeTest_QueryTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> inputPragram;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = "k_" + std::to_string(i);
        entry111.valueTest = "v_" + std::to_string(i);
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);

    DataQuery dataQuery;
    dataQuery.KeyPrefix("k_");
    std::vector<std::shared_ptr<KvStoreResultSet>> outputs(MAX_RESULTSET_SIZE_VIRTUAL + 1);
    for (int i = 0; i < MAX_RESULTSET_SIZE_VIRTUAL; i++) {
        std::shared_ptr<KvStoreResultSet> outputPragram;
        statusTest = kvStoreImpl->GetResultSet(dataQuery, outputs[i]);
        EXPECT_EQ(statusTest, SUCCESS);
    }

    statusTest = kvStoreImpl->GetResultSet(dataQuery, outputs[MAX_RESULTSET_SIZE_VIRTUAL]);
    EXPECT_EQ(statusTest, OVER_MAX_LIMITS);
    statusTest = kvStoreImpl->CloseResultSet(outputs[0]);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->GetResultSet(dataQuery, outputs[MAX_RESULTSET_SIZE_VIRTUAL]);
    EXPECT_EQ(statusTest, SUCCESS);

    for (int i = 1; i <= MAX_RESULTSET_SIZE_VIRTUAL; i++) {
        statusTest = kvStoreImpl->CloseResultSet(outputs[i]);
        EXPECT_EQ(statusTest, SUCCESS);
    }
}

/**
 * @tc.name: ResultSetMaxSizeTest
 * @tc.desc: test if kv supports 8 resultSets at the same time
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, ResultSetMaxSizeTest_PrefixTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);

    std::vector<Entry> inputPragram;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = "k_" + std::to_string(i);
        entry111.valueTest = "v_" + std::to_string(i);
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);

    std::vector<std::shared_ptr<KvStoreResultSet>> outputs(MAX_RESULTSET_SIZE_VIRTUAL + 1);
    for (int i = 0; i < MAX_RESULTSET_SIZE_VIRTUAL; i++) {
        std::shared_ptr<KvStoreResultSet> outputPragram;
        statusTest = kvStoreImpl->GetResultSet({ "k_i" }, outputs[i]);
        EXPECT_EQ(statusTest, SUCCESS);
    }

    statusTest = kvStoreImpl->GetResultSet({ "" }, outputs[MAX_RESULTSET_SIZE_VIRTUAL]);
    EXPECT_EQ(statusTest, OVER_MAX_LIMITS);

    statusTest = kvStoreImpl->CloseResultSet(outputs[0]);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->GetResultSet({ "" }, outputs[MAX_RESULTSET_SIZE_VIRTUAL]);
    EXPECT_EQ(statusTest, SUCCESS);

    for (int i = 1; i <= MAX_RESULTSET_SIZE_VIRTUAL; i++) {
        statusTest = kvStoreImpl->CloseResultSet(outputs[i]);
        EXPECT_EQ(statusTest, SUCCESS);
    }
}

/**
 * @tc.name: MaxLogSizeTest
 * @tc.desc: test if the default max limit of wal is 200MB
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, MaxLogSizeTestTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);

    std::string key;
    std::vector<uint8_t> valueTest = Random(4 * 1024 * 1024);
    key = "test0";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
    key = "test1";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
    key = "test2";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);

    std::shared_ptr<KvStoreResultSet> outputPragram;
    auto statusTest = kvStoreImpl->GetResultSet({ "" }, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram, nullptr);
    EXPECT_EQ(outputPragram->GetCount(), 3);
    EXPECT_EQ(outputPragram->MoveToFirst(), true);

    for (int i = 0; i < 50; i++) {
        key = "test_" + std::to_string(i);
        EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
    }
    key = "test3";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreImpl->Delete(key), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreImpl->StartTransaction(), WAL_OVER_LIMITS);

    statusTest = kvStoreImpl->CloseResultSet(outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
}

/**
 * @tc.name: MaxTest002
 * @tc.desc: test if the default max limit of wal is 200MB
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, MaxLogSizeTest002Test, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);

    std::string key;
    std::vector<uint8_t> valueTest = Random(4 * 1024 * 1024);
    key = "test0";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
    key = "test1";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
    key = "test2";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);

    std::shared_ptr<KvStoreResultSet> outputPragram;
    auto statusTest = kvStoreImpl->GetResultSet({ "" }, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram, nullptr);
    EXPECT_EQ(outputPragram->GetCount(), 3);
    EXPECT_EQ(outputPragram->MoveToFirst(), true);

    for (int i = 0; i < 50; i++) {
        key = "test_" + std::to_string(i);
        EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
    }

    key = "test3";
    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreImpl->Delete(key), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreImpl->StartTransaction(), WAL_OVER_LIMITS);
    statusTest = kvStoreImpl->CloseResultSet(outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);

    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStore" };
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.encrypt = false;
    optionsTest.area = EL1;
    optionsTest.backup = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";

    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
    kvStoreImpl = nullptr;
    kvStoreImpl = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_EQ(statusTest, SUCCESS);

    statusTest = kvStoreImpl->GetResultSet({ "" }, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram, nullptr);
    EXPECT_EQ(outputPragram->MoveToFirst(), true);

    EXPECT_EQ(kvStoreImpl->Put(key, valueTest), SUCCESS);
    statusTest = kvStoreImpl->CloseResultSet(outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: Move_Offset
 * @tc.desc: Move the ResultSet Relative Distance
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 */
HWTEST_F(SingleStoreImplVirtualTest, Move_OffsetTest, TestSize.Level0)
{
    std::vector<Entry> inputPragram;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);

    Key prefix = "2";
    std::shared_ptr<KvStoreResultSet> outputPragram;
    statusTest = kvStoreImpl->GetResultSet(prefix, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram, nullptr);

    auto outputTmp = outputPragram;
    EXPECT_EQ(outputTmp->Move(1), true);
    statusTest = kvStoreImpl->CloseResultSet(outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(outputPragram, nullptr);

    std::shared_ptr<SingleKvStore> singleKvStore;
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "DeviceKVStore" };
    singleKvStore = CreateKVStoreDB(storeId11.storeId11, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(singleKvStore, nullptr);

    statusTest = singleKvStore->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    std::shared_ptr<KvStoreResultSet> outputPragram1;
    statusTest = singleKvStore->GetResultSet(prefix, outputPragram1);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(outputPragram1, nullptr);
    auto outputTmp1 = outputPragram1;
    EXPECT_EQ(outputTmp1->Move(1), true);
    statusTest = singleKvStore->CloseResultSet(outputPragram1);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(outputPragram1, nullptr);

    singleKvStore = nullptr;
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: GetCount
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplVirtualTest, GetCountTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::vector<Entry> inputPragram;
    auto cmp = [](const Key &entry111, const Key &sentryTest) {
        return entry111.Data() < sentryTest.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionaryMap(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        dictionaryMap[entry111.key] = entry111.valueTest;
        inputPragram.push_back(entry111);
    }
    auto statusTest = kvStoreImpl->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    DataQuery dataQuery;
    dataQuery.InKeys({ "0_key", "1_key" });
    int countSum = 0;
    statusTest = kvStoreImpl->GetCount(dataQuery, countSum);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(countSum, 2);
    dataQuery.Reset();
    statusTest = kvStoreImpl->GetCount(dataQuery, countSum);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(countSum, 10);
}

void ChangeOwnerToService(std::string baseDirTest, std::string hashId)
{
    static constexpr int ddmsId = 3012;
    std::string path = baseDirTest;
    chown(path.c_str(), ddmsId, ddmsId);
    path = path + "/kvdb";
    chown(path.c_str(), ddmsId, ddmsId);
    path = path + "/" + hashId;
    chown(path.c_str(), ddmsId, ddmsId);
    path = path + "/single_ver";
    chown(path.c_str(), ddmsId, ddmsId);
    chown((path + "/meta").c_str(), ddmsId, ddmsId);
    chown((path + "/cache").c_str(), ddmsId, ddmsId);
    path = path + "/main";
    chown(path.c_str(), ddmsId, ddmsId);
    chown((path + "/gen_natural_store.db").c_str(), ddmsId, ddmsId);
    chown((path + "/gen_natural_store.db-shm").c_str(), ddmsId, ddmsId);
    chown((path + "/gen_natural_store.db-wal").c_str(), ddmsId, ddmsId);
}

/**
 * @tc.name: RemoveDeviceData
 * @tc.desc: remove local device data
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplVirtualTest, RemoveDeviceDataTest, TestSize.Level0)
{
    auto store = CreateKVStoreDB("DeviceKVStore", DEVICE_COLLABORATION, false, true);
    EXPECT_NE(store, nullptr);
    std::vector<Entry> inputPragram;
    auto cmp = [](const Key &entry111, const Key &sentryTest) {
        return entry111.Data() < sentryTest.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionaryMap(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        dictionaryMap[entry111.key] = entry111.valueTest;
        inputPragram.push_back(entry111);
    }
    auto statusTest = store->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    int countSum = 0;
    statusTest = store->GetCount({}, countSum);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(countSum, 10);
    ChangeOwnerToService("/data/service/el1/public/database/SingleStoreImplVirtualTest",
        "703c6ec99aa7226bb9f6194cdd60e1873ea9ee52faebd55657ade9f5a5cc3cbd");
    statusTest = store->RemoveDeviceData(DevManager::GetInstance().GetLocalDevice().networkId);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = store->GetCount({}, countSum);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(countSum, 10);
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusTest = StoreManagerTest::GetInstance().Delete({ "SingleStoreImplVirtualTest" }, { "DeviceKVStore" }, baseDirTest);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: GetSecurityLevel
 * @tc.desc: get security level
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplVirtualTest, GetSecurityLevelTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto statusTest = kvStoreImpl->GetSecurityLevel(securityLevel);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(securityLevel, S1);
}

/**
 * @tc.name: RegisterSyncCallback
 * @tc.desc: register the data sync callback
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplVirtualTest, RegisterSyncCallbackTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override { }
        void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override { }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto statusTest = kvStoreImpl->RegisterSyncCallback(callback);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: UnRegisterSyncCallback
 * @tc.desc: unregister the data sync callback
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplVirtualTest, UnRegisterSyncCallbackTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override { }
        void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override { }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto statusTest = kvStoreImpl->RegisterSyncCallback(callback);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->UnRegisterSyncCallback();
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: disableBackup
 * @tc.desc: Disable backup
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, disableBackupTest, TestSize.Level0)
{
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStoreNoBackup" };
    std::shared_ptr<SingleKvStore> kvStoreNoBackup;
    kvStoreNoBackup = CreateKVStoreDB(storeId11, SINGLE_VERSION, true, false);
    EXPECT_NE(kvStoreNoBackup, nullptr);
    auto baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    auto statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: PutOverMaxValue
 * @tc.desc: put key-valueTest data to the kv store and the valueTest size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, PutOverMaxValueTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::string valueTest;
    int maxsize = 1024 * 1024;
    for (int i = 0; i <= maxsize; i++) {
        valueTest += "test";
    }
    Value valuePut(valueTest);
    auto statusTest = kvStoreImpl->Put({ "Put Test" }, valuePut);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}
/**
 * @tc.name: DeleteOverMaxKey
 * @tc.desc: deleteVirtual the values of the keys and the key size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteOverMaxKeyTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    Key key(str);
    auto statusTest = kvStoreImpl->Put(key, "Put Test");
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
    Value valueTest;
    statusTest = kvStoreImpl->Get(key, valueTest);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
    statusTest = kvStoreImpl->Delete(key);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetEntriesOverMaxKey
 * @tc.desc: get entriesTest the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntriesOverMaxPrefixTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::vector<Entry> outputPragram;
    auto statusTest = kvStoreImpl->GetEntries(prefix, outputPragram);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetResultSetOverMaxPrefix
 * @tc.desc: get result set the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetResultSetOverMaxPrefixTest, TestSize.Level0)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::shared_ptr<KvStoreResultSet> outputPragram;
    auto statusTest = kvStoreImpl->GetResultSet(prefix, outputPragram);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: RemoveNullDeviceData
 * @tc.desc: remove local device data and the device is null
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, RemoveNullDeviceDataTest, TestSize.Level0)
{
    auto store = CreateKVStoreDB("DeviceKVStore", DEVICE_COLLABORATION, false, true);
    EXPECT_NE(store, nullptr);
    std::vector<Entry> inputPragram;
    auto cmp = [](const Key &entry111, const Key &sentryTest) {
        return entry111.Data() < sentryTest.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionaryMap(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = std::to_string(i).append("_kk");
        entry111.valueTest = std::to_string(i).append("_value");
        dictionaryMap[entry111.key] = entry111.valueTest;
        inputPragram.push_back(entry111);
    }
    auto statusTest = store->PutBatch(inputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    int countSum = 0;
    statusTest = store->GetCount({}, countSum);
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(countSum, 10);
    const string device = { "" };
    ChangeOwnerToService("/data/service/el1/public/database/SingleStoreImplVirtualTest",
        "703c6ec99aa7226bb9f6194cdd60e1873ea9ee52faebd55657ade9f5a5cc3cbd");
    statusTest = store->RemoveDeviceData(device);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: CloseKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseKVStoreWithInvalidAppIdTest, TestSize.Level0)
{
    AppId appId11 = { "" };
    StoreId storeId11 = { "SingleKVStore" };
    Status statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseKVStoreWithInvalidStoreId
 * @tc.desc: close the kv store with invalid store id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseKVStoreWithInvalidStoreIdTest, TestSize.Level0)
{
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "" };
    Status statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseAllKVStore
 * @tc.desc: close all kv store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseAllKVStoreTest, TestSize.Level0)
{
    AppId appId11 = { "SingleStoreImplTestCloseAll" };
    std::vector<std::shared_ptr<SingleKvStore>> kvStores;
    for (int i = 0; i < 5; i++) {
        std::shared_ptr<SingleKvStore> singleKvStore;
        Options optionsTest;
        optionsTest.kvStoreType = SINGLE_VERSION;
        optionsTest.securityLevel = S1;
        optionsTest.area = EL1;
        optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
        std::string sId = "SingleStoreImplTestCloseAll" + std::to_string(i);
        StoreId storeId11 = { sId };
        Status statusTest;
        singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
        EXPECT_NE(singleKvStore, nullptr);
        kvStores.push_back(singleKvStore);
        EXPECT_EQ(statusTest, SUCCESS);
        singleKvStore = nullptr;
    }
    Status statusTest = StoreManagerTest::GetInstance().CloseAllKVStore(appId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: CloseAllKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseAllKVStoreWithInvalidAppIdTest, TestSize.Level0)
{
    AppId appId11 = { "" };
    Status statusTest = StoreManagerTest::GetInstance().CloseAllKVStore(appId11);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidAppId
 * @tc.desc: deleteVirtual the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteWithInvalidAppIdTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "" };
    StoreId storeId11 = { "SingleKVStore" };
    Status statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidStoreId
 * @tc.desc: deleteVirtual the kv store with invalid storeid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteWithInvalidStoreIdTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "" };
    Status statusTest = StoreManagerTest::GetInstance().Delete(appId11, storeId11, baseDirTest);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetKVStoreWithPersistentFalse
 * @tc.desc: deleteVirtual the kv store with the persistent is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithPersistentFalseTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStorePersistentFalse" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.persistent = false;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_EQ(singleKvStore, nullptr);
}

/**
 * @tc.name: GetKVStoreWithInvalidType
 * @tc.desc: deleteVirtual the kv store with the KvStoreType is InvalidType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithInvalidTypeTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImpStore";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStoreInvalidType" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = INVALID_TYPE;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_EQ(singleKvStore, nullptr);
}

/**
 * @tc.name: GetKVStoreWithCreateIfMissingFalse
 * @tc.desc: deleteVirtual the kv store with the createIfMissing is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithCreateIfMissingFalseTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStoreCreateIfMissingFalse" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.createIfMissing = false;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_EQ(singleKvStore, nullptr);
}

/**
 * @tc.name: GetKVStoreWithAutoSync
 * @tc.desc: deleteVirtual the kv store with the autoSync is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithAutoSyncTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStoreAutoSync" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.autoSync = false;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithAreaEL2
 * @tc.desc: deleteVirtual the kv store with the area is EL2
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithAreaEL2Test, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el2/100/SingleStoreImplVirtualTest";
    mkdir(baseDirTest.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStoreAreaEL2" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S2;
    optionsTest.area = EL2;
    optionsTest.baseDirTest = "/data/service/el2/100/SingleStoreImplVirtualTest";
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithRebuildTrue
 * @tc.desc: deleteVirtual the kv store with the rebuild is true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithRebuildTrueTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SingleKVStoreRebuildFalse" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.rebuild = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: GetStaticStore
 * @tc.desc: get static store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangijang
 */
HWTEST_F(SingleStoreImplVirtualTest, GetStaticStoreTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "StaticStoreTest" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.rebuild = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsTest.dataType = DataType::TYPE_STATICS;
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: StaticStoreAsyncGet
 * @tc.desc: static store async get
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangijang
 */
HWTEST_F(SingleStoreImplVirtualTest, StaticStoreAsyncGetTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "StaticStoreAsyncGetTest" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.rebuild = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsTest.dataType = DataType::TYPE_STATICS;
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, Value &&)> result = [&blockData](Status statusTest, Value &&valueTest) {
        EXPECT_EQ(statusTest, Status::NOT_FOUND);
        blockData.SetValue(true);
    };
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    singleKvStore->Get({ "key" }, networkId, result);
    blockData.GetValue();
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: StaticStoreAsyncGetEntries
 * @tc.desc: static store async get entriesTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangijang
 */
HWTEST_F(SingleStoreImplVirtualTest, StaticStoreAsyncGetEntriesTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "StaticStoreAsyncGetEntriesTest" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.rebuild = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsTest.dataType = DataType::TYPE_STATICS;
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, std::vector<Entry> &&)> result = [&blockData](
                                                                    Status statusTest, std::vector<Entry> &&valueTest) {
        EXPECT_EQ(statusTest, Status::SUCCESS);
        blockData.SetValue(true);
    };
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    singleKvStore->GetEntries({ "key" }, networkId, result);
    blockData.GetValue();
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: DynamicStoreAsyncGet
 * @tc.desc: dynamic store async get
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangijang
 */
HWTEST_F(SingleStoreImplVirtualTest, DynamicStoreAsyncGetTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "DynamicStoreAsyncGetTest" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.rebuild = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsTest.dataType = DataType::TYPE_DYNAMICAL;
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    statusTest = singleKvStore->Put({ "Put Test" }, { "Put Value" });
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, Value &&)> result = [&blockData](Status statusTest, Value &&valueTest) {
        EXPECT_EQ(statusTest, Status::SUCCESS);
        EXPECT_EQ(valueTest.ToString(), "Put Value");
        blockData.SetValue(true);
    };
    singleKvStore->Get({ "Put Test" }, networkId, result);
    blockData.GetValue();
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: DynamicStoreAsyncGetEntries
 * @tc.desc: dynamic store async get entriesTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangijang
 */
HWTEST_F(SingleStoreImplVirtualTest, DynamicStoreAsyncGetEntriesTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "DynamicStoreAsyncGetEntriesTest" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.rebuild = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsTest.dataType = DataType::TYPE_DYNAMICAL;
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    std::vector<Entry> entriesTest;
    for (int i = 0; i < 10; ++i) {
        Entry entry111;
        entry111.key = "key_" + std::to_string(i);
        entry111.valueTest = std::to_string(i);
        entriesTest.push_back(entry111);
    }
    statusTest = singleKvStore->PutBatch(entriesTest);
    EXPECT_EQ(statusTest, SUCCESS);
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, std::vector<Entry> &&)> result = [entriesTest, &blockData](
                                                                    Status statusTest, std::vector<Entry> &&valueTest) {
        EXPECT_EQ(statusTest, Status::SUCCESS);
        EXPECT_EQ(valueTest.size(), entriesTest.size());
        blockData.SetValue(true);
    };
    singleKvStore->GetEntries({ "key_" }, networkId, result);
    blockData.GetValue();
    statusTest = StoreManagerTest::GetInstance().CloseKVStore(appId11, storeId11);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: SetConfig
 * @tc.desc: SetConfig
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: ht
 */
HWTEST_F(SingleStoreImplVirtualTest, SetConfigTest, TestSize.Level0)
{
    std::string baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appId11 = { "SingleStoreImplVirtualTest" };
    StoreId storeId11 = { "SetConfigTest" };
    std::shared_ptr<SingleKvStore> singleKvStore;
    Options optionsTest;
    optionsTest.kvStoreType = SINGLE_VERSION;
    optionsTest.securityLevel = S1;
    optionsTest.area = EL1;
    optionsTest.rebuild = true;
    optionsTest.baseDirTest = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsTest.dataType = DataType::TYPE_DYNAMICAL;
    optionsTest.cloudConfig.enableCloud = false;
    Status statusTest;
    singleKvStore = StoreManagerTest::GetInstance().GetKVStore(appId11, storeId11, optionsTest, statusTest);
    EXPECT_NE(singleKvStore, nullptr);
    StoreConfig storeConfig;
    storeConfig.cloudConfig.enableCloud = true;
    EXPECT_EQ(singleKvStore->SetConfig(storeConfig), Status::SUCCESS);
}

/**
 * @tc.name: GetDeviceEntries001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, GetDeviceEntries001Test, TestSize.Level1)
{
    std::string PKG_NAME_EX = "_distributed_data";
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB();
    EXPECT_NE(singleKvStore, nullptr);
    std::vector<Entry> outputPragram;
    std::string device = DevManager::GetInstance().GetUnEncryptedUuid();
    std::string devices = "GetDeviceEntriestest";
    auto statusTest = singleKvStore->GetDeviceEntries("", outputPragram);
    EXPECT_EQ(statusTest, INVALID_ARGUMENT);
    statusTest = singleKvStore->GetDeviceEntries(device, outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
    DevInfo devinfo;
    std::string PKG_NAME = std::to_string(getpid()) + PKG_NAME_EX;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, devinfo);
    EXPECT_NE(std::string(devinfo.deviceId), "");
    statusTest = singleKvStore->GetDeviceEntries(std::string(devinfo.deviceId), outputPragram);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: DoSync001
 * @tc.desc: observerVirtual = nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoSync001Test, TestSize.Level1)
{
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB();
    EXPECT_NE(singleKvStore, nullptr) << "kvStorePtr is null.";
    std::string deviceId = "no_exist_device_id";
    std::vector<std::string> deviceIds = { deviceId };
    uint32_t allowedDelayMs = 200;
    singleKvStore->isClientSync_ = false;
    auto syncStatus = singleKvStore->Sync(deviceIds, SyncMode::PUSH, allowedDelayMs);
    EXPECT_EQ(syncStatus, Status::SUCCESS) << "sync device should return success";
    singleKvStore->isClientSync_ = true;
    singleKvStore->syncObserver_ = nullptr;
    syncStatus = singleKvStore->Sync(deviceIds, SyncMode::PUSH, allowedDelayMs);
    EXPECT_EQ(syncStatus, Status::SUCCESS) << "sync device should return success";
}

/**
 * @tc.name: SetCapabilityEnabled001
 * @tc.desc: enabled
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, SetCapabilityEnabled001Test, TestSize.Level1)
{
    EXPECT_NE(kvStoreImpl, nullptr);
    auto statusTest = kvStoreImpl->SetCapabilityEnabled(true);
    EXPECT_EQ(statusTest, SUCCESS);
    statusTest = kvStoreImpl->SetCapabilityEnabled(false);
    EXPECT_EQ(statusTest, SUCCESS);
}

/**
 * @tc.name: DoClientSync001
 * @tc.desc: observerVirtual = nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoClientSync001Test, TestSize.Level1)
{
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB();
    EXPECT_NE(singleKvStore, nullptr);
    KVDBService::SyncInfo syncInfo;
    syncInfo.mode = SyncMode::PULL;
    syncInfo.seqId = 10; // syncInfo seqId
    syncInfo.devices = { "networkId" };
    std::shared_ptr<SyncCallback> observerVirtual;
    observerVirtual = nullptr;
    auto statusTest = singleKvStore->DoClientSync(syncInfo, observerVirtual);
    EXPECT_EQ(statusTest, DB_ERROR);
}

/**
 * @tc.name: DoNotifyChange001
 * @tc.desc: called within timeout
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoNotifyChange001Test, TestSize.Level1)
{
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB();
    EXPECT_NE(singleKvStore, nullptr) << "kvStorePtr is null.";
    auto notifyExpiredTime = singleKvStore->notifyExpiredTime_;
    EXPECT_NE(notifyExpiredTime, 0);
    statusTest = singleKvStore->Put({ "Put Test1" }, { "Put Value1" });
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(notifyExpiredTime, singleKvStore->notifyExpiredTime_);
    auto statusTest = singleKvStore->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(singleKvStore->notifyExpiredTime_, 0);
    singleKvStore->cloudAutoSync_ = true;
    statusTest = singleKvStore->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusTest, SUCCESS);
    sleep(1);
    statusTest = singleKvStore->Put({ "Put Test2" }, { "Put Value2" });
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_NE(notifyExpiredTime, singleKvStore->notifyExpiredTime_);
}

/**
 * @tc.name: DoAutoSync001
 * @tc.desc: observerVirtual = nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoAutoSync001Test, TestSize.Level1)
{
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB(true);
    EXPECT_NE(singleKvStore, nullptr);
    singleKvStore->isApplication_ = true;
    auto statusTest = singleKvStore->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusTest, SUCCESS);
    EXPECT_EQ(!singleKvStore->autoSync_ || !singleKvStore->isApplication_, false);
}

/**
 * @tc.name: IsRemoteChanged
 * @tc.desc: is remote changed
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, IsRemoteChangedTest, TestSize.Level0)
{
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB();
    EXPECT_NE(singleKvStore, nullptr);
    bool ret = singleKvStore->IsRemoteChanged("");
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: ReportDBCorruptedFault
 * @tc.desc: report DB corrupted fault
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, ReportDBCorruptedFaultTest, TestSize.Level0)
{
    std::shared_ptr<SingleStoreImpl> singleKvStore;
    singleKvStore = CreateKVStoreDB();
    EXPECT_NE(singleKvStore, nullptr);
    Status statusTest = DATA_CORRUPTED;
    singleKvStore->ReportDBCorruptedFault(statusTest);
    EXPECT_TRUE(statusTest == DATA_CORRUPTED);
}
} // namespace OHOS::Test