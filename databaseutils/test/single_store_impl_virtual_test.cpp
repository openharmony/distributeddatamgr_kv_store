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
#define LOG_TAG "SingleStoreImplVirtualTest"

#include "block_data.h"
#include "dev_manager.h"
#include "device_manager.h"
#include "distributed_kv_data_manager.h"
#include "dm_device_info.h"
#include "file_ex.h"
#include "kv_store_nb_delegate.h"
#include "single_store_impl.h"
#include "store_factory.h"
#include "store_manager.h"
#include "sys/stat.h"
#include "types.h"
#include <condition_variable>
#include <gtest/gtest.h>
#include <vector>

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using DBStatus = DistributedDB::DBStatus;
using DBStore = DistributedDB::KvStoreNbDelegate;
using SyncCallback = KvStoreSyncCallback;
using DevInfo = OHOS::DistributedHardware::DmDeviceInfo;
namespace OHOS::Test {
static constexpr int MAX_RESULTSET_SIZE = 8;
std::vector<uint8_t> RandomVirtual(int32_t len)
{
    return std::vector<uint8_t>(len, 'a');
}

class SingleStoreImplVirtualTest : public testing::Test {
public:
    class TestObserverVirtual : public KvStoreObserver {
    public:
        TestObserverVirtual()
        {
            // The time interval parameter is 5.
            data_ = std::make_shared<OHOS::BlockData<bool>>(5, false);
        }
        void OnChange(const ChangeNotification &notificationVirtual) override
        {
            insert_ = notificationVirtual.GetInsertEntries();
            update_ = notificationVirtual.GetUpdateEntries();
            delete_ = notificationVirtual.GetDeleteEntries();
            deviceId_ = notificationVirtual.GetDeviceId();
            bool valueVirtual = true;
            data_->SetValue(valueVirtual);
        }
        std::vector<Entry> insert_;
        std::vector<Entry> update_;
        std::vector<Entry> delete_;
        std::string deviceId_;

        std::shared_ptr<OHOS::BlockData<bool>> data_;
    };

    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, KvStoreType type, bool encrypt, bool backup);
    std::shared_ptr<SingleStoreImpl> CreateKVStore(bool autosync = false);
    std::shared_ptr<SingleKvStore> kvStoreVirtual_;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SingleStoreImplVirtualTest::SetUpTestCase(void)
{
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    mkdir(baseDirVirtual.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SingleStoreImplVirtualTest::TearDownTestCase(void)
{
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    StoreManager::GetInstance().Delete({ "SingleStoreImplVirtualTest" }, { "SingleKVStore" }, baseDirVirtual);

    (void)remove("/data/service/el1/public/database/SingleStoreImplVirtualTest/key");
    (void)remove("/data/service/el1/public/database/SingleStoreImplVirtualTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleStoreImplVirtualTest");
}

void SingleStoreImplVirtualTest::SetUp(void)
{
    kvStoreVirtual_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false, true);
    if (kvStoreVirtual_ == nullptr) {
        kvStoreVirtual_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false, true);
    }
    EXPECT_NE(kvStoreVirtual_, nullptr);
}

void SingleStoreImplVirtualTest::TearDown(void)
{
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStore" };
    kvStoreVirtual_ = nullptr;
    auto statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    auto baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

std::shared_ptr<SingleKvStore> SingleStoreImplVirtualTest::CreateKVStore(
    std::string storeIdTest, KvStoreType type, bool encrypt, bool backup)
{
    Options optionsVirtual;
    optionsVirtual.kvStoreType = type;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.encrypt = encrypt;
    optionsVirtual.area = EL1;
    optionsVirtual.backup = backup;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";

    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { storeIdTest };
    Status statusVirtual =
        StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, optionsVirtual.baseDirVirtual);
    return StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
}

std::shared_ptr<SingleStoreImpl> SingleStoreImplVirtualTest::CreateKVStore(bool autosync)
{
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "DestructorTest" };
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S2;
    optionsVirtual.area = EL1;
    optionsVirtual.autoSync = autosync;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    StoreFactory storeFactory;
    auto dbManager = storeFactory.GetDBManager(optionsVirtual.baseDirVirtual, appIdVirtual);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(
        storeIdVirtual.storeIdVirtual, optionsVirtual.baseDirVirtual, optionsVirtual.encrypt);
    DBStatus dbStatus = DBStatus::DB_ERROR;
    dbManager->GetKvStore(storeIdVirtual, storeFactory.GetDBOption(optionsVirtual, dbPassword),
        [&dbManager, &kvStoreVirtual, &appIdVirtual, &dbStatus, &optionsVirtual, &storeFactory](
            auto statusVirtual, auto *store) {
            dbStatus = statusVirtual;
            if (store == nullptr) {
                return;
            }
            auto release = [dbManager](auto *store) {
                dbManager->CloseKvStore(store);
            };
            auto dbStore = std::shared_ptr<DBStore>(store, release);
            storeFactory.SetDbConfig(dbStore);
            const Convertor &convertor = *(storeFactory.convertors_[optionsVirtual.kvStoreType]);
            kvStoreVirtual = std::make_shared<SingleStoreImpl>(dbStore, appIdVirtual, optionsVirtual, convertor);
        });
    return kvStoreVirtual;
}

/**
 * @tc.name: GetStoreId
 * @tc.desc: get the store id of the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetStoreId, TestSize.Level0)
{
    ZLOGI("GetStoreId begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto storeIdVirtual = kvStoreVirtual_->GetStoreId();
    EXPECT_EQ(storeIdVirtual.storeIdVirtual, "SingleKVStore");
}

/**
 * @tc.name: Put
 * @tc.desc: put key-valueVirtual data to the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, Put, TestSize.Level0)
{
    ZLOGI("Put begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto statusVirtual = kvStoreVirtual_->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->Put({ "   Put Test" }, { "Put2 Value" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    Value valueVirtual;
    statusVirtual = kvStoreVirtual_->Get({ "Put Test" }, valueVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(valueVirtual.ToString(), "Put2 Value");
}

/**
 * @tc.name: Put_Invalid_Key
 * @tc.desc: put invalid key-valueVirtual data to the device kv store and single kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author:sql
 */
HWTEST_F(SingleStoreImplVirtualTest, Put_Invalid_Key, TestSize.Level0)
{
    ZLOGI("Put_Invalid_Key begin.");
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "DeviceKVStore" };
    kvStoreVirtual = CreateKVStore(storeIdVirtual.storeIdVirtual, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(kvStoreVirtual, nullptr);

    size_t maxDevKeyLen = 897;
    std::string str(maxDevKeyLen, 'a');
    Blob key(str);
    Blob valueVirtual("test_value");
    Status statusVirtual = kvStoreVirtual->Put(key, valueVirtual);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);

    Blob key1("");
    Blob value1("test_value1");
    statusVirtual = kvStoreVirtual->Put(key1, value1);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);

    kvStoreVirtual = nullptr;
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);

    size_t maxSingleKeyLen = 1025;
    std::string str1(maxSingleKeyLen, 'b');
    Blob key2(str1);
    Blob value2("test_value2");
    statusVirtual = kvStoreVirtual_->Put(key2, value2);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);

    statusVirtual = kvStoreVirtual_->Put(key1, value1);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: PutBatch
 * @tc.desc: put some key-valueVirtual data to the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, PutBatch, TestSize.Level0)
{
    ZLOGI("PutBatch begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(entries);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: IsRebuild
 * @tc.desc: test IsRebuild
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, IsRebuild, TestSize.Level0)
{
    ZLOGI("IsRebuild begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto statusVirtual = kvStoreVirtual_->IsRebuild();
    EXPECT_EQ(statusVirtual, false);
}

/**
 * @tc.name: PutBatch001
 * @tc.desc: entry.valueVirtual.Size() > MAX_VALUE_LENGTH
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, PutBatch001, TestSize.Level1)
{
    ZLOGI("PutBatch001 begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    size_t totalLength = SingleStoreImpl::MAX_VALUE_LENGTH + 1; // create an out-of-limit large number
    char fillChar = 'a';
    std::string longString(totalLength, fillChar);
    std::vector<Entry> entries;
    Entry entry;
    entry.key = "PutBatch001_test";
    entry.valueVirtual = longString;
    entries.push_back(entry);
    auto statusVirtual = kvStoreVirtual_->PutBatch(entries);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
    entries.clear();
    Entry entrys;
    entrys.key = "";
    entrys.valueVirtual = "PutBatch001_test_value";
    entries.push_back(entrys);
    statusVirtual = kvStoreVirtual_->PutBatch(entries);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: Delete
 * @tc.desc: delete the valueVirtual of the key
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, Delete, TestSize.Level0)
{
    ZLOGI("Delete begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto statusVirtual = kvStoreVirtual_->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    Value valueVirtual;
    statusVirtual = kvStoreVirtual_->Get({ "Put Test" }, valueVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(std::string("Put Value"), valueVirtual.ToString());
    statusVirtual = kvStoreVirtual_->Delete({ "Put Test" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    valueVirtual = {};
    statusVirtual = kvStoreVirtual_->Get({ "Put Test" }, valueVirtual);
    EXPECT_EQ(statusVirtual, KEY_NOT_FOUND);
    EXPECT_EQ(std::string(""), valueVirtual.ToString());
}

/**
 * @tc.name: DeleteBatch
 * @tc.desc: delete the values of the keys
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteBatch, TestSize.Level0)
{
    ZLOGI("DeleteBatch begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(entries);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::vector<Key> keys;
    for (int i = 0; i < 10; ++i) {
        Key key = std::to_string(i).append("_k");
        keys.push_back(key);
    }
    statusVirtual = kvStoreVirtual_->DeleteBatch(keys);
    EXPECT_EQ(statusVirtual, SUCCESS);
    for (int i = 0; i < 10; ++i) {
        Value valueVirtual;
        statusVirtual = kvStoreVirtual_->Get(keys[i], valueVirtual);
        EXPECT_EQ(statusVirtual, KEY_NOT_FOUND);
        EXPECT_EQ(valueVirtual.ToString(), std::string(""));
    }
}

/**
 * @tc.name: Transaction
 * @tc.desc: do transaction
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, Transaction, TestSize.Level0)
{
    ZLOGI("Transaction begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto statusVirtual = kvStoreVirtual_->StartTransaction();
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->Commit();
    EXPECT_EQ(statusVirtual, SUCCESS);

    statusVirtual = kvStoreVirtual_->StartTransaction();
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->Rollback();
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: SubscribeKvStore
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, SubscribeKvStore, TestSize.Level0)
{
    ZLOGI("SubscribeKvStore begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto observer = std::make_shared<TestObserverVirtual>();
    auto statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(statusVirtual, STORE_ALREADY_SUBSCRIBE);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    EXPECT_EQ(statusVirtual, STORE_ALREADY_SUBSCRIBE);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observer);
    EXPECT_EQ(statusVirtual, STORE_ALREADY_SUBSCRIBE);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(statusVirtual, STORE_ALREADY_SUBSCRIBE);
    bool invalidValue = false;
    observer->data_->Clear(invalidValue);
    statusVirtual = kvStoreVirtual_->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_TRUE(observer->data_->GetValue());
    EXPECT_EQ(observer->insert_.size(), 1);
    EXPECT_EQ(observer->update_.size(), 0);
    EXPECT_EQ(observer->delete_.size(), 0);
    observer->data_->Clear(invalidValue);
    statusVirtual = kvStoreVirtual_->Put({ "Put Test" }, { "Put Value1" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_TRUE(observer->data_->GetValue());
    EXPECT_EQ(observer->insert_.size(), 0);
    EXPECT_EQ(observer->update_.size(), 1);
    EXPECT_EQ(observer->delete_.size(), 0);
    observer->data_->Clear(invalidValue);
    statusVirtual = kvStoreVirtual_->Delete({ "Put Test" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_TRUE(observer->data_->GetValue());
    EXPECT_EQ(observer->insert_.size(), 0);
    EXPECT_EQ(observer->update_.size(), 0);
    EXPECT_EQ(observer->delete_.size(), 1);
}

/**
 * @tc.name: SubscribeKvStore002
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Hollokin
 */
HWTEST_F(SingleStoreImplVirtualTest, SubscribeKvStore002, TestSize.Level0)
{
    ZLOGI("SubscribeKvStore002 begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::shared_ptr<TestObserverVirtual> subscribedObserver;
    std::shared_ptr<TestObserverVirtual> unSubscribedObserver;
    for (int i = 0; i < 15; ++i) {
        auto observer = std::make_shared<TestObserverVirtual>();
        auto status1 = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
        auto status2 = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
        if (i < 8) {
            EXPECT_EQ(status1, SUCCESS);
            EXPECT_EQ(status2, SUCCESS);
            subscribedObserver = observer;
        } else {
            EXPECT_EQ(status1, OVER_MAX_LIMITS);
            EXPECT_EQ(status2, OVER_MAX_LIMITS);
            unSubscribedObserver = observer;
        }
    }

    auto statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    EXPECT_EQ(statusVirtual, STORE_ALREADY_SUBSCRIBE);

    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, {});
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);

    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, subscribedObserver);
    EXPECT_EQ(statusVirtual, STORE_ALREADY_SUBSCRIBE);

    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserver);
    EXPECT_EQ(statusVirtual, SUCCESS);

    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    EXPECT_EQ(statusVirtual, SUCCESS);

    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserver);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, unSubscribedObserver);
    EXPECT_EQ(statusVirtual, SUCCESS);
    subscribedObserver = unSubscribedObserver;
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    EXPECT_EQ(statusVirtual, SUCCESS);
    auto observer = std::make_shared<TestObserverVirtual>();
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    observer = std::make_shared<TestObserverVirtual>();
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(statusVirtual, OVER_MAX_LIMITS);
}

/**
 * @tc.name: SubscribeKvStore003
 * @tc.desc: isClientSync_
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, SubscribeKvStore003, TestSize.Level0)
{
    ZLOGI("SubscribeKvStore003 begin.");
    auto observer = std::make_shared<TestObserverVirtual>();
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore();
    EXPECT_NE(kvStoreVirtual, nullptr);
    kvStoreVirtual->isClientSync_ = true;
    auto statusVirtual = kvStoreVirtual->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: UnsubscribeKvStore
 * @tc.desc: unsubscribe
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, UnsubscribeKvStore, TestSize.Level0)
{
    ZLOGI("UnsubscribeKvStore begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto observer = std::make_shared<TestObserverVirtual>();
    auto statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    EXPECT_EQ(statusVirtual, STORE_NOT_SUBSCRIBE);
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(statusVirtual, STORE_NOT_SUBSCRIBE);
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(statusVirtual, STORE_NOT_SUBSCRIBE);
    statusVirtual = kvStoreVirtual_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetEntries_Prefix
 * @tc.desc: get entries by prefix
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_Prefix, TestSize.Level0)
{
    ZLOGI("GetEntries_Prefix begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::vector<Entry> output;
    statusVirtual = kvStoreVirtual_->GetEntries({ "" }, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::sort(output.begin(), output.end(), [](const Entry &entry, const Entry &sentry) {
        return entry.key.Data() < sentry.key.Data();
    });
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(input[i].key == output[i].key);
        EXPECT_TRUE(input[i].valueVirtual == output[i].valueVirtual);
    }
}

/**
 * @tc.name: GetEntries_Less_Prefix
 * @tc.desc: get entries by prefix and the key size less than sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author:sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_Less_Prefix, TestSize.Level0)
{
    ZLOGI("GetEntries_Less_Prefix begin.");
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "DeviceKVStore" };
    kvStoreVirtual = CreateKVStore(storeIdVirtual.storeIdVirtual, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(kvStoreVirtual, nullptr);

    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::vector<Entry> output;
    statusVirtual = kvStoreVirtual->GetEntries({ "1" }, output);
    EXPECT_NE(output.empty(), true);
    EXPECT_EQ(statusVirtual, SUCCESS);

    kvStoreVirtual = nullptr;
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);

    statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::vector<Entry> output1;
    statusVirtual = kvStoreVirtual_->GetEntries({ "1" }, output1);
    EXPECT_NE(output1.empty(), true);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetEntries_Greater_Prefix
 * @tc.desc: get entries by prefix and the key size is greater than  sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author:sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_Greater_Prefix, TestSize.Level0)
{
    ZLOGI("GetEntries_Greater_Prefix begin.");
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "DeviceKVStore" };
    kvStoreVirtual = CreateKVStore(storeIdVirtual.storeIdVirtual, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(kvStoreVirtual, nullptr);

    size_t keyLen = sizeof(uint32_t);
    std::vector<Entry> input;
    for (int i = 1; i < 10; ++i) {
        Entry entry;
        std::string str(keyLen, i + '0');
        entry.key = str;
        entry.valueVirtual = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::vector<Entry> output;
    std::string str1(keyLen, '1');
    statusVirtual = kvStoreVirtual->GetEntries(str1, output);
    EXPECT_NE(output.empty(), true);
    EXPECT_EQ(statusVirtual, SUCCESS);

    kvStoreVirtual = nullptr;
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);

    statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::vector<Entry> output1;
    statusVirtual = kvStoreVirtual_->GetEntries(str1, output1);
    EXPECT_NE(output1.empty(), true);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetEntries_DataQuery
 * @tc.desc: get entries by query
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntries_DataQuery, TestSize.Level0)
{
    ZLOGI("GetEntries_DataQuery begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::vector<Entry> output;
    statusVirtual = kvStoreVirtual_->GetEntries(query, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::sort(output.begin(), output.end(), [](const Entry &entry, const Entry &sentry) {
        return entry.key.Data() < sentry.key.Data();
    });
    EXPECT_LE(output.size(), 2);
    for (size_t i = 0; i < output.size(); ++i) {
        EXPECT_TRUE(input[i].key == output[i].key);
        EXPECT_TRUE(input[i].valueVirtual == output[i].valueVirtual);
    }
}

/**
 * @tc.name: GetResultSet_Prefix
 * @tc.desc: get result set by prefix
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetResultSet_Prefix, TestSize.Level0)
{
    ZLOGI("GetResultSet_Prefix begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueVirtual;
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::shared_ptr<KvStoreResultSet> output;
    statusVirtual = kvStoreVirtual_->GetResultSet({ "" }, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output, nullptr);
    EXPECT_EQ(output->GetCount(), 10);
    int count = 0;
    while (output->MoveToNext()) {
        count++;
        Entry entry;
        output->GetEntry(entry);
        EXPECT_EQ(entry.valueVirtual.Data(), dictionary[entry.key].Data());
    }
    EXPECT_EQ(count, output->GetCount());
}

/**
 * @tc.name: GetResultSet_Query
 * @tc.desc: get result set by query
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetResultSet_Query, TestSize.Level0)
{
    ZLOGI("GetResultSet_Query begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueVirtual;
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::shared_ptr<KvStoreResultSet> output;
    statusVirtual = kvStoreVirtual_->GetResultSet(query, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output, nullptr);
    EXPECT_LE(output->GetCount(), 2);
    int count = 0;
    while (output->MoveToNext()) {
        count++;
        Entry entry;
        output->GetEntry(entry);
        EXPECT_EQ(entry.valueVirtual.Data(), dictionary[entry.key].Data());
    }
    EXPECT_EQ(count, output->GetCount());
}

/**
 * @tc.name: CloseResultSet
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseResultSet, TestSize.Level0)
{
    ZLOGI("CloseResultSet begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueVirtual;
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::shared_ptr<KvStoreResultSet> output;
    statusVirtual = kvStoreVirtual_->GetResultSet(query, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output, nullptr);
    EXPECT_LE(output->GetCount(), 2);
    auto outputTmp = output;
    statusVirtual = kvStoreVirtual_->CloseResultSet(output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(output, nullptr);
    EXPECT_EQ(outputTmp->GetCount(), KvStoreResultSet::INVALID_COUNT);
    EXPECT_EQ(outputTmp->GetPosition(), KvStoreResultSet::INVALID_POSITION);
    EXPECT_EQ(outputTmp->MoveToFirst(), false);
    EXPECT_EQ(outputTmp->MoveToLast(), false);
    EXPECT_EQ(outputTmp->MoveToNext(), false);
    EXPECT_EQ(outputTmp->MoveToPrevious(), false);
    EXPECT_EQ(outputTmp->Move(1), false);
    EXPECT_EQ(outputTmp->MoveToPosition(1), false);
    EXPECT_EQ(outputTmp->IsFirst(), false);
    EXPECT_EQ(outputTmp->IsLast(), false);
    EXPECT_EQ(outputTmp->IsBeforeFirst(), false);
    EXPECT_EQ(outputTmp->IsAfterLast(), false);
    Entry entry;
    EXPECT_EQ(outputTmp->GetEntry(entry), ALREADY_CLOSED);
}

/**
 * @tc.name: CloseResultSet001
 * @tc.desc: output = nullptr;
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseResultSet001, TestSize.Level0)
{
    ZLOGI("CloseResultSet001 begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::shared_ptr<KvStoreResultSet> output;
    output = nullptr;
    auto statusVirtual = kvStoreVirtual_->CloseResultSet(output);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: ResultSetMaxSizeTest_Query
 * @tc.desc: test if kv supports 8 resultSets at the same time
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, ResultSetMaxSizeTest_Query, TestSize.Level0)
{
    ZLOGI("ResultSetMaxSizeTest_Query begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    /**
     * @tc.steps:step1. Put the entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "k_" + std::to_string(i);
        entry.valueVirtual = "v_" + std::to_string(i);
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    DataQuery query;
    query.KeyPrefix("k_");
    std::vector<std::shared_ptr<KvStoreResultSet>> outputs(MAX_RESULTSET_SIZE + 1);
    for (int i = 0; i < MAX_RESULTSET_SIZE; i++) {
        std::shared_ptr<KvStoreResultSet> output;
        statusVirtual = kvStoreVirtual_->GetResultSet(query, outputs[i]);
        EXPECT_EQ(statusVirtual, SUCCESS);
    }
    /**
     * @tc.steps:step3. Get the resultset while resultset size is over the limit.
     * @tc.expected: step3. Returns OVER_MAX_LIMITS.
     */
    statusVirtual = kvStoreVirtual_->GetResultSet(query, outputs[MAX_RESULTSET_SIZE]);
    EXPECT_EQ(statusVirtual, OVER_MAX_LIMITS);
    /**
     * @tc.steps:step4. Close the resultset and getting the resultset is retried
     * @tc.expected: step4. Returns SUCCESS.
     */
    statusVirtual = kvStoreVirtual_->CloseResultSet(outputs[0]);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->GetResultSet(query, outputs[MAX_RESULTSET_SIZE]);
    EXPECT_EQ(statusVirtual, SUCCESS);

    for (int i = 1; i <= MAX_RESULTSET_SIZE; i++) {
        statusVirtual = kvStoreVirtual_->CloseResultSet(outputs[i]);
        EXPECT_EQ(statusVirtual, SUCCESS);
    }
}

/**
 * @tc.name: ResultSetMaxSizeTest_Prefix
 * @tc.desc: test if kv supports 8 resultSets at the same time
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, ResultSetMaxSizeTest_Prefix, TestSize.Level0)
{
    ZLOGI("ResultSetMaxSizeTest_Prefix begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    /**
     * @tc.steps:step1. Put the entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "k_" + std::to_string(i);
        entry.valueVirtual = "v_" + std::to_string(i);
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    std::vector<std::shared_ptr<KvStoreResultSet>> outputs(MAX_RESULTSET_SIZE + 1);
    for (int i = 0; i < MAX_RESULTSET_SIZE; i++) {
        std::shared_ptr<KvStoreResultSet> output;
        statusVirtual = kvStoreVirtual_->GetResultSet({ "k_i" }, outputs[i]);
        EXPECT_EQ(statusVirtual, SUCCESS);
    }
    /**
     * @tc.steps:step3. Get the resultset while resultset size is over the limit.
     * @tc.expected: step3. Returns OVER_MAX_LIMITS.
     */
    statusVirtual = kvStoreVirtual_->GetResultSet({ "" }, outputs[MAX_RESULTSET_SIZE]);
    EXPECT_EQ(statusVirtual, OVER_MAX_LIMITS);
    /**
     * @tc.steps:step4. Close the resultset and getting the resultset is retried
     * @tc.expected: step4. Returns SUCCESS.
     */
    statusVirtual = kvStoreVirtual_->CloseResultSet(outputs[0]);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->GetResultSet({ "" }, outputs[MAX_RESULTSET_SIZE]);
    EXPECT_EQ(statusVirtual, SUCCESS);

    for (int i = 1; i <= MAX_RESULTSET_SIZE; i++) {
        statusVirtual = kvStoreVirtual_->CloseResultSet(outputs[i]);
        EXPECT_EQ(statusVirtual, SUCCESS);
    }
}

/**
 * @tc.name: MaxLogSizeTest
 * @tc.desc: test if the default max limit of wal is 200MB
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, MaxLogSizeTest, TestSize.Level0)
{
    ZLOGI("MaxLogSizeTest begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    /**
     * @tc.steps:step1. Put the random entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::string key;
    std::vector<uint8_t> valueVirtual = RandomVirtual(4 * 1024 * 1024);
    key = "test0";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    key = "test1";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    key = "test2";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    std::shared_ptr<KvStoreResultSet> output;
    auto statusVirtual = kvStoreVirtual_->GetResultSet({ "" }, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output, nullptr);
    EXPECT_EQ(output->GetCount(), 3);
    EXPECT_EQ(output->MoveToFirst(), true);
    /**
     * @tc.steps:step3. Put more data into the database.
     * @tc.expected: step3. Returns SUCCESS.
     */
    for (int i = 0; i < 50; i++) {
        key = "test_" + std::to_string(i);
        EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    }
    /**
     * @tc.steps:step4. Put more data into the database while the log size is over the limit.
     * @tc.expected: step4. Returns LOG_LIMITS_ERROR.
     */
    key = "test3";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreVirtual_->Delete(key), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreVirtual_->StartTransaction(), WAL_OVER_LIMITS);
    /**
     * @tc.steps:step5. Close the resultset and put again.
     * @tc.expected: step4. Return SUCCESS.
     */

    statusVirtual = kvStoreVirtual_->CloseResultSet(output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
}

/**
 * @tc.name: MaxLogSizeTest002
 * @tc.desc: test if the default max limit of wal is 200MB
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, MaxLogSizeTest002, TestSize.Level0)
{
    EXPECT_NE(kvStoreVirtual_, nullptr);
    /**
     * @tc.steps:step1. Put the random entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::string key;
    std::vector<uint8_t> valueVirtual = RandomVirtual(4 * 1024 * 1024);
    key = "test0";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    key = "test1";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    key = "test2";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    std::shared_ptr<KvStoreResultSet> output;
    auto statusVirtual = kvStoreVirtual_->GetResultSet({ "" }, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output, nullptr);
    EXPECT_EQ(output->GetCount(), 3);
    EXPECT_EQ(output->MoveToFirst(), true);
    /**
     * @tc.steps:step3. Put more data into the database.
     * @tc.expected: step3. Returns SUCCESS.
     */
    for (int i = 0; i < 50; i++) {
        key = "test_" + std::to_string(i);
        EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    }
    /**
     * @tc.steps:step4. Put more data into the database while the log size is over the limit.
     * @tc.expected: step4. Returns LOG_LIMITS_ERROR.
     */
    key = "test3";
    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreVirtual_->Delete(key), WAL_OVER_LIMITS);
    EXPECT_EQ(kvStoreVirtual_->StartTransaction(), WAL_OVER_LIMITS);
    statusVirtual = kvStoreVirtual_->CloseResultSet(output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    /**
     * @tc.steps:step5. Close the database and then open the database,put again.
     * @tc.expected: step4. Return SUCCESS.
     */
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStore" };
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.encrypt = false;
    optionsVirtual.area = EL1;
    optionsVirtual.backup = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";

    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    kvStoreVirtual_ = nullptr;
    kvStoreVirtual_ =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);

    statusVirtual = kvStoreVirtual_->GetResultSet({ "" }, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output, nullptr);
    EXPECT_EQ(output->MoveToFirst(), true);

    EXPECT_EQ(kvStoreVirtual_->Put(key, valueVirtual), SUCCESS);
    statusVirtual = kvStoreVirtual_->CloseResultSet(output);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: Move_Offset
 * @tc.desc: Move the ResultSet Relative Distance
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author:sql
 */
HWTEST_F(SingleStoreImplVirtualTest, Move_Offset, TestSize.Level0)
{
    ZLOGI("Move_Offset begin.");
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);

    Key prefix = "2";
    std::shared_ptr<KvStoreResultSet> output;
    statusVirtual = kvStoreVirtual_->GetResultSet(prefix, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output, nullptr);

    auto outputTmp = output;
    EXPECT_EQ(outputTmp->Move(1), true);
    statusVirtual = kvStoreVirtual_->CloseResultSet(output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(output, nullptr);

    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "DeviceKVStore" };
    kvStoreVirtual = CreateKVStore(storeIdVirtual.storeIdVirtual, DEVICE_COLLABORATION, false, true);
    EXPECT_NE(kvStoreVirtual, nullptr);

    statusVirtual = kvStoreVirtual->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::shared_ptr<KvStoreResultSet> output1;
    statusVirtual = kvStoreVirtual->GetResultSet(prefix, output1);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(output1, nullptr);
    auto outputTmp1 = output1;
    EXPECT_EQ(outputTmp1->Move(1), true);
    statusVirtual = kvStoreVirtual->CloseResultSet(output1);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(output1, nullptr);

    kvStoreVirtual = nullptr;
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetCount
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetCount, TestSize.Level0)
{
    ZLOGI("GetCount begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueVirtual;
        input.push_back(entry);
    }
    auto statusVirtual = kvStoreVirtual_->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    int count = 0;
    statusVirtual = kvStoreVirtual_->GetCount(query, count);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(count, 2);
    query.Reset();
    statusVirtual = kvStoreVirtual_->GetCount(query, count);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(count, 10);
}

void ChangeOwnerToService(std::string baseDirVirtual, std::string hashId)
{
    ZLOGI("ChangeOwnerToService begin.");
    static constexpr int ddmsId = 3012;
    std::string path = baseDirVirtual;
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
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, RemoveDeviceData, TestSize.Level0)
{
    ZLOGI("RemoveDeviceData begin.");
    auto store = CreateKVStore("DeviceKVStore", DEVICE_COLLABORATION, false, true);
    EXPECT_NE(store, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueVirtual;
        input.push_back(entry);
    }
    auto statusVirtual = store->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    int count = 0;
    statusVirtual = store->GetCount({}, count);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(count, 10);
    ChangeOwnerToService("/data/service/el1/public/database/SingleStoreImplVirtualTest",
        "703c6ec99aa7226bb9f6194cdd60e1873ea9ee52faebd55657ade9f5a5cc3cbd");
    statusVirtual = store->RemoveDeviceData(DevManager::GetInstance().GetLocalDevice().networkId);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = store->GetCount({}, count);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(count, 10);
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    statusVirtual =
        StoreManager::GetInstance().Delete({ "SingleStoreImplVirtualTest" }, { "DeviceKVStore" }, baseDirVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetSecurityLevel
 * @tc.desc: get security level
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetSecurityLevel, TestSize.Level0)
{
    ZLOGI("GetSecurityLevel begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto statusVirtual = kvStoreVirtual_->GetSecurityLevel(securityLevel);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(securityLevel, S1);
}

/**
 * @tc.name: RegisterSyncCallback
 * @tc.desc: register the data sync callback
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, RegisterSyncCallback, TestSize.Level0)
{
    ZLOGI("RegisterSyncCallback begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override { }
        void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override { }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto statusVirtual = kvStoreVirtual_->RegisterSyncCallback(callback);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: UnRegisterSyncCallback
 * @tc.desc: unregister the data sync callback
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, UnRegisterSyncCallback, TestSize.Level0)
{
    ZLOGI("UnRegisterSyncCallback begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override { }
        void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override { }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto statusVirtual = kvStoreVirtual_->RegisterSyncCallback(callback);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->UnRegisterSyncCallback();
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: disableBackup
 * @tc.desc: Disable backup
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, disableBackup, TestSize.Level0)
{
    ZLOGI("disableBackup begin.");
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStoreNoBackup" };
    std::shared_ptr<SingleKvStore> kvStoreNoBackup;
    kvStoreNoBackup = CreateKVStore(storeIdVirtual, SINGLE_VERSION, true, false);
    EXPECT_NE(kvStoreNoBackup, nullptr);
    auto baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    auto statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: PutOverMaxValue
 * @tc.desc: put key-valueVirtual data to the kv store and the valueVirtual size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, PutOverMaxValue, TestSize.Level0)
{
    ZLOGI("PutOverMaxValue begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::string valueVirtual;
    int maxsize = 1024 * 1024;
    for (int i = 0; i <= maxsize; i++) {
        valueVirtual += "test";
    }
    Value valuePut(valueVirtual);
    auto statusVirtual = kvStoreVirtual_->Put({ "Put Test" }, valuePut);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}
/**
 * @tc.name: DeleteOverMaxKey
 * @tc.desc: delete the values of the keys and the key size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteOverMaxKey, TestSize.Level0)
{
    ZLOGI("DeleteOverMaxKey begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    Key key(str);
    auto statusVirtual = kvStoreVirtual_->Put(key, "Put Test");
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
    Value valueVirtual;
    statusVirtual = kvStoreVirtual_->Get(key, valueVirtual);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
    statusVirtual = kvStoreVirtual_->Delete(key);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetEntriesOverMaxPrefix
 * @tc.desc: get entries the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetEntriesOverMaxPrefix, TestSize.Level0)
{
    ZLOGI("GetEntriesOverMaxPrefix begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::vector<Entry> output;
    auto statusVirtual = kvStoreVirtual_->GetEntries(prefix, output);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetResultSetOverMaxPrefix
 * @tc.desc: get result set the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetResultSetOverMaxPrefix, TestSize.Level0)
{
    ZLOGI("GetResultSetOverMaxPrefix begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::shared_ptr<KvStoreResultSet> output;
    auto statusVirtual = kvStoreVirtual_->GetResultSet(prefix, output);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: RemoveNullDeviceData
 * @tc.desc: remove local device data and the device is null
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, RemoveNullDeviceData, TestSize.Level0)
{
    ZLOGI("RemoveNullDeviceData begin.");
    auto store = CreateKVStore("DeviceKVStore", DEVICE_COLLABORATION, false, true);
    EXPECT_NE(store, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueVirtual = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueVirtual;
        input.push_back(entry);
    }
    auto statusVirtual = store->PutBatch(input);
    EXPECT_EQ(statusVirtual, SUCCESS);
    int count = 0;
    statusVirtual = store->GetCount({}, count);
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(count, 10);
    const string device = { "" };
    ChangeOwnerToService("/data/service/el1/public/database/SingleStoreImplVirtualTest",
        "703c6ec99aa7226bb9f6194cdd60e1873ea9ee52faebd55657ade9f5a5cc3cbd");
    statusVirtual = store->RemoveDeviceData(device);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: CloseKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseKVStoreWithInvalidAppId, TestSize.Level0)
{
    ZLOGI("CloseKVStoreWithInvalidAppId begin.");
    AppId appIdVirtual = { "" };
    StoreId storeIdVirtual = { "SingleKVStore" };
    Status statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseKVStoreWithInvalidStoreId
 * @tc.desc: close the kv store with invalid store id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseKVStoreWithInvalidStoreId, TestSize.Level0)
{
    ZLOGI("CloseKVStoreWithInvalidStoreId begin.");
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "" };
    Status statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseAllKVStore
 * @tc.desc: close all kv store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseAllKVStore, TestSize.Level0)
{
    ZLOGI("CloseAllKVStore begin.");
    AppId appIdVirtual = { "SingleStoreImplVirtualTestCloseAll" };
    std::vector<std::shared_ptr<SingleKvStore>> kvStores;
    for (int i = 0; i < 5; i++) {
        std::shared_ptr<SingleKvStore> kvStoreVirtual;
        Options optionsVirtual;
        optionsVirtual.kvStoreType = SINGLE_VERSION;
        optionsVirtual.securityLevel = S1;
        optionsVirtual.area = EL1;
        optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
        std::string sId = "SingleStoreImplVirtualTestCloseAll" + std::to_string(i);
        StoreId storeIdVirtual = { sId };
        Status statusVirtual;
        kvStoreVirtual =
            StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
        EXPECT_NE(kvStoreVirtual, nullptr);
        kvStores.push_back(kvStoreVirtual);
        EXPECT_EQ(statusVirtual, SUCCESS);
        kvStoreVirtual = nullptr;
    }
    Status statusVirtual = StoreManager::GetInstance().CloseAllKVStore(appIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: CloseAllKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, CloseAllKVStoreWithInvalidAppId, TestSize.Level0)
{
    ZLOGI("CloseAllKVStoreWithInvalidAppId begin.");
    AppId appIdVirtual = { "" };
    Status statusVirtual = StoreManager::GetInstance().CloseAllKVStore(appIdVirtual);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidAppId
 * @tc.desc: delete the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteWithInvalidAppId, TestSize.Level0)
{
    ZLOGI("DeleteWithInvalidAppId begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "" };
    StoreId storeIdVirtual = { "SingleKVStore" };
    Status statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidStoreId
 * @tc.desc: delete the kv store with invalid storeid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, DeleteWithInvalidStoreId, TestSize.Level0)
{
    ZLOGI("DeleteWithInvalidStoreId begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "" };
    Status statusVirtual = StoreManager::GetInstance().Delete(appIdVirtual, storeIdVirtual, baseDirVirtual);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetKVStoreWithPersistentFalse
 * @tc.desc: delete the kv store with the persistent is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithPersistentFalse, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithPersistentFalse begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStorePersistentFalse" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.persistent = false;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_EQ(kvStoreVirtual, nullptr);
}

/**
 * @tc.name: GetKVStoreWithInvalidType
 * @tc.desc: delete the kv store with the KvStoreType is InvalidType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithInvalidType, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithInvalidType begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImpStore";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStoreInvalidType" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = INVALID_TYPE;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_EQ(kvStoreVirtual, nullptr);
}

/**
 * @tc.name: GetKVStoreWithCreateIfMissingFalse
 * @tc.desc: delete the kv store with the createIfMissing is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithCreateIfMissingFalse, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithCreateIfMissingFalse begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStoreCreateIfMissingFalse" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.createIfMissing = false;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_EQ(kvStoreVirtual, nullptr);
}

/**
 * @tc.name: GetKVStoreWithAutoSync
 * @tc.desc: delete the kv store with the autoSync is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithAutoSync, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithAutoSync begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStoreAutoSync" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.autoSync = false;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithAreaEL2
 * @tc.desc: delete the kv store with the area is EL2
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithAreaEL2, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithAreaEL2 begin.");
    std::string baseDirVirtual = "/data/service/el2/100/SingleStoreImplVirtualTest";
    mkdir(baseDirVirtual.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStoreAreaEL2" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S2;
    optionsVirtual.area = EL2;
    optionsVirtual.baseDirVirtual = "/data/service/el2/100/SingleStoreImplVirtualTest";
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithRebuildTrue
 * @tc.desc: delete the kv store with the rebuild is true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetKVStoreWithRebuildTrue, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithRebuildTrue begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SingleKVStoreRebuildFalse" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.rebuild = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: GetStaticStore
 * @tc.desc: get static store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, GetStaticStore, TestSize.Level0)
{
    ZLOGI("GetStaticStore begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "StaticStoreTest" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.rebuild = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsVirtual.dataType = DataType::TYPE_STATICS;
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: StaticStoreAsyncGet
 * @tc.desc: static store async get
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, StaticStoreAsyncGet, TestSize.Level0)
{
    ZLOGI("StaticStoreAsyncGet begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "StaticStoreAsyncGetTest" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.rebuild = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsVirtual.dataType = DataType::TYPE_STATICS;
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, Value &&)> result = [&blockData](Status statusVirtual, Value &&valueVirtual) {
        EXPECT_EQ(statusVirtual, Status::NOT_FOUND);
        blockData.SetValue(true);
    };
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    kvStoreVirtual->Get({ "key" }, networkId, result);
    blockData.GetValue();
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: StaticStoreAsyncGetEntries
 * @tc.desc: static store async get entries
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, StaticStoreAsyncGetEntries, TestSize.Level0)
{
    ZLOGI("StaticStoreAsyncGetEntries begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "StaticStoreAsyncGetEntriesTest" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.rebuild = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsVirtual.dataType = DataType::TYPE_STATICS;
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, std::vector<Entry> &&)> result = [&blockData](Status statusVirtual,
                                                                    std::vector<Entry> &&valueVirtual) {
        EXPECT_EQ(statusVirtual, Status::SUCCESS);
        blockData.SetValue(true);
    };
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    kvStoreVirtual->GetEntries({ "key" }, networkId, result);
    blockData.GetValue();
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: DynamicStoreAsyncGet
 * @tc.desc: dynamic store async get
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, DynamicStoreAsyncGet, TestSize.Level0)
{
    ZLOGI("DynamicStoreAsyncGet begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "DynamicStoreAsyncGetTest" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.rebuild = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsVirtual.dataType = DataType::TYPE_DYNAMICAL;
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    statusVirtual = kvStoreVirtual->Put({ "Put Test" }, { "Put Value" });
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, Value &&)> result = [&blockData](Status statusVirtual, Value &&valueVirtual) {
        EXPECT_EQ(statusVirtual, Status::SUCCESS);
        EXPECT_EQ(valueVirtual.ToString(), "Put Value");
        blockData.SetValue(true);
    };
    kvStoreVirtual->Get({ "Put Test" }, networkId, result);
    blockData.GetValue();
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: DynamicStoreAsyncGetEntries
 * @tc.desc: dynamic store async get entries
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleStoreImplVirtualTest, DynamicStoreAsyncGetEntries, TestSize.Level0)
{
    ZLOGI("DynamicStoreAsyncGetEntries begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "DynamicStoreAsyncGetEntriesTest" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.rebuild = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsVirtual.dataType = DataType::TYPE_DYNAMICAL;
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "key_" + std::to_string(i);
        entry.valueVirtual = std::to_string(i);
        entries.push_back(entry);
    }
    statusVirtual = kvStoreVirtual->PutBatch(entries);
    EXPECT_EQ(statusVirtual, SUCCESS);
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, std::vector<Entry> &&)> result = [entries, &blockData](Status statusVirtual,
                                                                    std::vector<Entry> &&valueVirtual) {
        EXPECT_EQ(statusVirtual, Status::SUCCESS);
        EXPECT_EQ(valueVirtual.size(), entries.size());
        blockData.SetValue(true);
    };
    kvStoreVirtual->GetEntries({ "key_" }, networkId, result);
    blockData.GetValue();
    statusVirtual = StoreManager::GetInstance().CloseKVStore(appIdVirtual, storeIdVirtual);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: SetConfig
 * @tc.desc: SetConfig
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: ht
 */
HWTEST_F(SingleStoreImplVirtualTest, SetConfig, TestSize.Level0)
{
    ZLOGI("SetConfig begin.");
    std::string baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    AppId appIdVirtual = { "SingleStoreImplVirtualTest" };
    StoreId storeIdVirtual = { "SetConfigTest" };
    std::shared_ptr<SingleKvStore> kvStoreVirtual;
    Options optionsVirtual;
    optionsVirtual.kvStoreType = SINGLE_VERSION;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.rebuild = true;
    optionsVirtual.baseDirVirtual = "/data/service/el1/public/database/SingleStoreImplVirtualTest";
    optionsVirtual.dataType = DataType::TYPE_DYNAMICAL;
    optionsVirtual.cloudConfig.enableCloud = false;
    Status statusVirtual;
    kvStoreVirtual =
        StoreManager::GetInstance().GetKVStore(appIdVirtual, storeIdVirtual, optionsVirtual, statusVirtual);
    EXPECT_NE(kvStoreVirtual, nullptr);
    StoreConfig storeConfig;
    storeConfig.cloudConfig.enableCloud = true;
    EXPECT_EQ(kvStoreVirtual->SetConfig(storeConfig), Status::SUCCESS);
}

/**
 * @tc.name: GetDeviceEntries001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, GetDeviceEntries001, TestSize.Level1)
{
    ZLOGI("GetDeviceEntries001 begin.");
    std::string str = "_distributed_data";
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore();
    EXPECT_NE(kvStoreVirtual, nullptr);
    std::vector<Entry> output;
    std::string device = DevManager::GetInstance().GetUnEncryptedUuid();
    std::string devices = "GetDeviceEntriestest";
    auto statusVirtual = kvStoreVirtual->GetDeviceEntries("", output);
    EXPECT_EQ(statusVirtual, INVALID_ARGUMENT);
    statusVirtual = kvStoreVirtual->GetDeviceEntries(device, output);
    EXPECT_EQ(statusVirtual, SUCCESS);
    DevInfo devinfo;
    std::string strName = std::to_string(getpid()) + str;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(strName, devinfo);
    EXPECT_NE(std::string(devinfo.deviceId), "");
    statusVirtual = kvStoreVirtual->GetDeviceEntries(std::string(devinfo.deviceId), output);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: DoSync001
 * @tc.desc: observer = nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoSync001, TestSize.Level1)
{
    ZLOGI("DoSync001 begin.");
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore();
    EXPECT_NE(kvStoreVirtual, nullptr) << "kvStorePtr is null.";
    std::string deviceId = "no_exist_device_id";
    std::vector<std::string> deviceIds = { deviceId };
    uint32_t allowedDelayMs = 200;
    kvStoreVirtual->isClientSync_ = false;
    auto syncStatus = kvStoreVirtual->Sync(deviceIds, SyncMode::PUSH, allowedDelayMs);
    EXPECT_EQ(syncStatus, Status::SUCCESS) << "sync device should return success";
    kvStoreVirtual->isClientSync_ = true;
    kvStoreVirtual->syncObserver_ = nullptr;
    syncStatus = kvStoreVirtual->Sync(deviceIds, SyncMode::PUSH, allowedDelayMs);
    EXPECT_EQ(syncStatus, Status::SUCCESS) << "sync device should return success";
}

/**
 * @tc.name: SetCapabilityEnabled001
 * @tc.desc: enabled
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, SetCapabilityEnabled001, TestSize.Level1)
{
    ZLOGI("SetCapabilityEnabled001 begin.");
    EXPECT_NE(kvStoreVirtual_, nullptr);
    auto statusVirtual = kvStoreVirtual_->SetCapabilityEnabled(true);
    EXPECT_EQ(statusVirtual, SUCCESS);
    statusVirtual = kvStoreVirtual_->SetCapabilityEnabled(false);
    EXPECT_EQ(statusVirtual, SUCCESS);
}

/**
 * @tc.name: DoClientSync001
 * @tc.desc: observer = nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoClientSync001, TestSize.Level1)
{
    ZLOGI("DoClientSync001 begin.");
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore();
    EXPECT_NE(kvStoreVirtual, nullptr);
    KVDBService::SyncInfo syncInfo;
    syncInfo.mode = SyncMode::PULL;
    syncInfo.seqId = 10; // syncInfo seqId
    syncInfo.devices = { "networkId" };
    std::shared_ptr<SyncCallback> observer;
    observer = nullptr;
    auto statusVirtual = kvStoreVirtual->DoClientSync(syncInfo, observer);
    EXPECT_EQ(statusVirtual, DB_ERROR);
}

/**
 * @tc.name: DoNotifyChange001
 * @tc.desc: called within timeout
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoNotifyChange001, TestSize.Level1)
{
    ZLOGI("DoNotifyChange001 begin.");
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore();
    EXPECT_NE(kvStoreVirtual, nullptr) << "kvStorePtr is null.";
    auto statusVirtual = kvStoreVirtual->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(kvStoreVirtual->notifyExpiredTime_, 0);
    kvStoreVirtual->cloudAutoSync_ = true;
    statusVirtual = kvStoreVirtual->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    auto notifyExpiredTime = kvStoreVirtual->notifyExpiredTime_;
    EXPECT_NE(notifyExpiredTime, 0);
    statusVirtual = kvStoreVirtual->Put({ "Put Test1" }, { "Put Value1" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(notifyExpiredTime, kvStoreVirtual->notifyExpiredTime_);
    sleep(1);
    statusVirtual = kvStoreVirtual->Put({ "Put Test2" }, { "Put Value2" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_NE(notifyExpiredTime, kvStoreVirtual->notifyExpiredTime_);
}

/**
 * @tc.name: DoAutoSync001
 * @tc.desc: observer = nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, DoAutoSync001, TestSize.Level1)
{
    ZLOGI("DoAutoSync001 begin.");
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore(true);
    EXPECT_NE(kvStoreVirtual, nullptr);
    kvStoreVirtual->isApplication_ = true;
    auto statusVirtual = kvStoreVirtual->Put({ "Put Test" }, { "Put Value" });
    EXPECT_EQ(statusVirtual, SUCCESS);
    EXPECT_EQ(!kvStoreVirtual->autoSync_ || !kvStoreVirtual->isApplication_, false);
}

/**
 * @tc.name: IsRemoteChanged
 * @tc.desc: is remote changed
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, IsRemoteChanged, TestSize.Level0)
{
    ZLOGI("IsRemoteChanged begin.");
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore();
    EXPECT_NE(kvStoreVirtual, nullptr);
    bool ret = kvStoreVirtual->IsRemoteChanged("");
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: ReportDBCorruptedFault
 * @tc.desc: report DB corrupted fault
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplVirtualTest, ReportDBCorruptedFault, TestSize.Level0)
{
    ZLOGI("ReportDBCorruptedFault begin.");
    std::shared_ptr<SingleStoreImpl> kvStoreVirtual;
    kvStoreVirtual = CreateKVStore();
    EXPECT_NE(kvStoreVirtual, nullptr);
    Status statusVirtual = DATA_CORRUPTED;
    kvStoreVirtual->ReportDBCorruptedFault(statusVirtual);
    EXPECT_TRUE(statusVirtual == DATA_CORRUPTED);
}
} // namespace OHOS::Test