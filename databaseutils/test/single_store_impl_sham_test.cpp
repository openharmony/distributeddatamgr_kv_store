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
#define LOG_TAG "SingleStoreImplShamTest"

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
std::vector<uint8_t> RandomSham(int32_t len)
{
    return std::vector<uint8_t>(len, 'a');
}

class SingleStoreImplShamTest : public testing::Test {
public:
    class TestObserverSham : public KvStoreObserver {
    public:
        TestObserverSham()
        {
            // The time interval parameter is 5.
            data_ = std::make_shared<OHOS::BlockData<bool>>(5, false);
        }
        void OnChange(const ChangeNotification &notificationSham) override
        {
            insert_ = notificationSham.GetInsertEntries();
            update_ = notificationSham.GetUpdateEntries();
            delete_ = notificationSham.GetDeleteEntries();
            deviceId_ = notificationSham.GetDeviceId();
            bool valueSham = true;
            data_->SetValue(valueSham);
        }
        std::vector<Entry> insert_;
        std::vector<Entry> update_;
        std::vector<Entry> delete_;
        std::string deviceId_;

        std::shared_ptr<OHOS::BlockData<bool>> data_;
    };

    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, KvStoreType type, bool encrypt, bool backup);
    std::shared_ptr<SingleStoreImpl> CreateKVStore(bool autosync = false);
    std::shared_ptr<SingleKvStore> kvStoreSham_;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SingleStoreImplShamTest::SetUpTestCase(void)
{
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    mkdir(baseDirSham.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SingleStoreImplShamTest::TearDownTestCase(void)
{
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    StoreManager::GetInstance().Delete({ "SingleStoreImplShamTest" }, { "SingleKVStore" }, baseDirSham);

    (void)remove("/data/service/el1/public/database/SingleStoreImplShamTest/key");
    (void)remove("/data/service/el1/public/database/SingleStoreImplShamTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleStoreImplShamTest");
}

void SingleStoreImplShamTest::SetUp(void)
{
    kvStoreSham_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false, true);
    if (kvStoreSham_ == nullptr) {
        kvStoreSham_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false, true);
    }
    ASSERT_NE(kvStoreSham_, nullptr);
}

void SingleStoreImplShamTest::TearDown(void)
{
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStore" };
    kvStoreSham_ = nullptr;
    auto statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
    auto baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

std::shared_ptr<SingleKvStore> SingleStoreImplShamTest::CreateKVStore(
    std::string storeIdTest, KvStoreType type, bool encrypt, bool backup)
{
    Options optionsSham;
    optionsSham.kvStoreType = type;
    optionsSham.securityLevel = S1;
    optionsSham.encrypt = encrypt;
    optionsSham.area = EL1;
    optionsSham.backup = backup;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";

    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { storeIdTest };
    Status statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, optionsSham.baseDirSham);
    return StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
}

std::shared_ptr<SingleStoreImpl> SingleStoreImplShamTest::CreateKVStore(bool autosync)
{
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "DestructorTest" };
    std::shared_ptr<SingleStoreImpl> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S2;
    optionsSham.area = EL1;
    optionsSham.autoSync = autosync;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    StoreFactory storeFactory;
    auto dbManager = storeFactory.GetDBManager(optionsSham.baseDirSham, appIdSham);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(
        storeIdSham.storeIdSham, optionsSham.baseDirSham, optionsSham.encrypt);
    DBStatus dbStatus = DBStatus::DB_ERROR;
    dbManager->GetKvStore(storeIdSham, storeFactory.GetDBOption(optionsSham, dbPassword),
        [&dbManager, &kvStoreSham, &appIdSham, &dbStatus, &optionsSham, &storeFactory](auto statusSham, auto *store) {
            dbStatus = statusSham;
            if (store == nullptr) {
                return;
            }
            auto release = [dbManager](auto *store) {
                dbManager->CloseKvStore(store);
            };
            auto dbStore = std::shared_ptr<DBStore>(store, release);
            storeFactory.SetDbConfig(dbStore);
            const Convertor &convertor = *(storeFactory.convertors_[optionsSham.kvStoreType]);
            kvStoreSham = std::make_shared<SingleStoreImpl>(dbStore, appIdSham, optionsSham, convertor);
        });
    return kvStoreSham;
}

/**
 * @tc.name: GetStoreId
 * @tc.desc: get the store id of the kv store
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetStoreId, TestSize.Level0)
{
    ZLOGI("GetStoreId start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    auto storeIdSham = kvStoreSham_->GetStoreId();
    ASSERT_EQ(storeIdSham.storeIdSham, "SingleKVStore");
}

/**
 * @tc.name: Put
 * @tc.desc: put key-valueSham data to the kv store
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, Put, TestSize.Level0)
{
    ZLOGI("Put start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    auto statusSham = kvStoreSham_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->Put({ "   Put Test" }, { "Put2 Value" });
    ASSERT_EQ(statusSham, SUCCESS);
    Value valueSham;
    statusSham = kvStoreSham_->Get({ "Put Test" }, valueSham);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(valueSham.ToString(), "Put2 Value");
}

/**
 * @tc.name: Put_Invalid_Key
 * @tc.desc: put invalid key-valueSham data to the device kv store and single kv store
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author:SQL
 */
HWTEST_F(SingleStoreImplShamTest, Put_Invalid_Key, TestSize.Level0)
{
    ZLOGI("Put_Invalid_Key start.");
    std::shared_ptr<SingleKvStore> kvStoreSham;
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "DeviceKVStore" };
    kvStoreSham = CreateKVStore(storeIdSham.storeIdSham, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStoreSham, nullptr);

    size_t maxDevKeyLen = 897;
    std::string str(maxDevKeyLen, 'a');
    Blob key(str);
    Blob valueSham("test_value");
    Status statusSham = kvStoreSham->Put(key, valueSham);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);

    Blob key1("");
    Blob value1("test_value1");
    statusSham = kvStoreSham->Put(key1, value1);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);

    kvStoreSham = nullptr;
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, SUCCESS);

    size_t maxSingleKeyLen = 1025;
    std::string str1(maxSingleKeyLen, 'b');
    Blob key2(str1);
    Blob value2("test_value2");
    statusSham = kvStoreSham_->Put(key2, value2);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);

    statusSham = kvStoreSham_->Put(key1, value1);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: PutBatch
 * @tc.desc: put some key-valueSham data to the kv store
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, PutBatch, TestSize.Level0)
{
    ZLOGI("PutBatch start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(entries);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: IsRebuild
 * @tc.desc: test IsRebuild
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplShamTest, IsRebuild, TestSize.Level0)
{
    ZLOGI("IsRebuild start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    auto statusSham = kvStoreSham_->IsRebuild();
    ASSERT_EQ(statusSham, false);
}

/**
 * @tc.name: PutBatch001
 * @tc.desc: entry.valueSham.Size() > MAX_VALUE_LENGTH
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplShamTest, PutBatch001, TestSize.Level0)
{
    ZLOGI("PutBatch001 start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    size_t totalLength = SingleStoreImpl::MAX_VALUE_LENGTH + 1; // create an out-of-limit large number
    char fillChar = 'a';
    std::string longString(totalLength, fillChar);
    std::vector<Entry> entries;
    Entry entry;
    entry.key = "PutBatch001_test";
    entry.valueSham = longString;
    entries.push_back(entry);
    auto statusSham = kvStoreSham_->PutBatch(entries);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
    entries.clear();
    Entry entrys;
    entrys.key = "";
    entrys.valueSham = "PutBatch001_test_value";
    entries.push_back(entrys);
    statusSham = kvStoreSham_->PutBatch(entries);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: Delete
 * @tc.desc: delete the valueSham of the key
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, Delete, TestSize.Level0)
{
    ZLOGI("Delete start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    auto statusSham = kvStoreSham_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(statusSham, SUCCESS);
    Value valueSham;
    statusSham = kvStoreSham_->Get({ "Put Test" }, valueSham);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(std::string("Put Value"), valueSham.ToString());
    statusSham = kvStoreSham_->Delete({ "Put Test" });
    ASSERT_EQ(statusSham, SUCCESS);
    valueSham = {};
    statusSham = kvStoreSham_->Get({ "Put Test" }, valueSham);
    ASSERT_EQ(statusSham, KEY_NOT_FOUND);
    ASSERT_EQ(std::string(""), valueSham.ToString());
}

/**
 * @tc.name: DeleteBatch
 * @tc.desc: delete the values of the keys
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, DeleteBatch, TestSize.Level0)
{
    ZLOGI("DeleteBatch start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(entries);
    ASSERT_EQ(statusSham, SUCCESS);
    std::vector<Key> keys;
    for (int i = 0; i < 10; ++i) {
        Key key = std::to_string(i).append("_k");
        keys.push_back(key);
    }
    statusSham = kvStoreSham_->DeleteBatch(keys);
    ASSERT_EQ(statusSham, SUCCESS);
    for (int i = 0; i < 10; ++i) {
        Value valueSham;
        statusSham = kvStoreSham_->Get(keys[i], valueSham);
        ASSERT_EQ(statusSham, KEY_NOT_FOUND);
        ASSERT_EQ(valueSham.ToString(), std::string(""));
    }
}

/**
 * @tc.name: Transaction
 * @tc.desc: do transaction
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, Transaction, TestSize.Level0)
{
    ZLOGI("Transaction start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    auto statusSham = kvStoreSham_->StartTransaction();
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->Commit();
    ASSERT_EQ(statusSham, SUCCESS);

    statusSham = kvStoreSham_->StartTransaction();
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->Rollback();
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: SubscribeKvStore
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, SubscribeKvStore, TestSize.Level0)
{
    ZLOGI("SubscribeKvStore start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    auto observer = std::make_shared<TestObserverSham>();
    auto statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(statusSham, STORE_ALREADY_SUBSCRIBE);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(statusSham, STORE_ALREADY_SUBSCRIBE);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observer);
    ASSERT_EQ(statusSham, STORE_ALREADY_SUBSCRIBE);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(statusSham, STORE_ALREADY_SUBSCRIBE);
    bool invalidValue = false;
    observer->data_->Clear(invalidValue);
    statusSham = kvStoreSham_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_TRUE(observer->data_->GetValue());
    ASSERT_EQ(observer->insert_.size(), 1);
    ASSERT_EQ(observer->update_.size(), 0);
    ASSERT_EQ(observer->delete_.size(), 0);
    observer->data_->Clear(invalidValue);
    statusSham = kvStoreSham_->Put({ "Put Test" }, { "Put Value1" });
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_TRUE(observer->data_->GetValue());
    ASSERT_EQ(observer->insert_.size(), 0);
    ASSERT_EQ(observer->update_.size(), 1);
    ASSERT_EQ(observer->delete_.size(), 0);
    observer->data_->Clear(invalidValue);
    statusSham = kvStoreSham_->Delete({ "Put Test" });
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_TRUE(observer->data_->GetValue());
    ASSERT_EQ(observer->insert_.size(), 0);
    ASSERT_EQ(observer->update_.size(), 0);
    ASSERT_EQ(observer->delete_.size(), 1);
}

/**
 * @tc.name: SubscribeKvStore002
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: Hollokin
 */
HWTEST_F(SingleStoreImplShamTest, SubscribeKvStore002, TestSize.Level0)
{
    ZLOGI("SubscribeKvStore002 start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::shared_ptr<TestObserverSham> subscribedObserver;
    std::shared_ptr<TestObserverSham> unSubscribedObserver;
    for (int i = 0; i < 15; ++i) {
        auto observer = std::make_shared<TestObserverSham>();
        auto status1 = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
        auto status2 = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
        if (i < 8) {
            ASSERT_EQ(status1, SUCCESS);
            ASSERT_EQ(status2, SUCCESS);
            subscribedObserver = observer;
        } else {
            ASSERT_EQ(status1, OVER_MAX_LIMITS);
            ASSERT_EQ(status2, OVER_MAX_LIMITS);
            unSubscribedObserver = observer;
        }
    }

    auto statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(statusSham, STORE_ALREADY_SUBSCRIBE);

    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, {});
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);

    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, subscribedObserver);
    ASSERT_EQ(statusSham, STORE_ALREADY_SUBSCRIBE);

    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserver);
    ASSERT_EQ(statusSham, SUCCESS);

    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(statusSham, SUCCESS);

    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserver);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, unSubscribedObserver);
    ASSERT_EQ(statusSham, SUCCESS);
    subscribedObserver = unSubscribedObserver;
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(statusSham, SUCCESS);
    auto observer = std::make_shared<TestObserverSham>();
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    observer = std::make_shared<TestObserverSham>();
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(statusSham, OVER_MAX_LIMITS);
}

/**
 * @tc.name: SubscribeKvStore003
 * @tc.desc: isClientSync_
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplShamTest, SubscribeKvStore003, TestSize.Level0)
{
    ZLOGI("SubscribeKvStore003 start.");
    auto observer = std::make_shared<TestObserverSham>();
    std::shared_ptr<SingleStoreImpl> kvStoreSham;
    kvStoreSham = CreateKVStore();
    ASSERT_NE(kvStoreSham, nullptr);
    kvStoreSham->isClientSync_ = true;
    auto statusSham = kvStoreSham->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: UnsubscribeKvStore
 * @tc.desc: unsubscribe
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, UnsubscribeKvStore, TestSize.Level0)
{
    ZLOGI("UnsubscribeKvStore start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    auto observer = std::make_shared<TestObserverSham>();
    auto statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_CLOUD, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(statusSham, STORE_NOT_SUBSCRIBE);
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(statusSham, STORE_NOT_SUBSCRIBE);
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(statusSham, STORE_NOT_SUBSCRIBE);
    statusSham = kvStoreSham_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetEntries_Prefix
 * @tc.desc: get entries by prefix
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetEntries_Prefix, TestSize.Level0)
{
    ZLOGI("GetEntries_Prefix start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    std::vector<Entry> output;
    statusSham = kvStoreSham_->GetEntries({ "" }, output);
    ASSERT_EQ(statusSham, SUCCESS);
    std::sort(output.start(), output.end(), [](const Entry &entry, const Entry &sentry) {
        return entry.key.Data() < sentry.key.Data();
    });
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(input[i].key == output[i].key);
        ASSERT_TRUE(input[i].valueSham == output[i].valueSham);
    }
}

/**
 * @tc.name: GetEntries_Less_Prefix
 * @tc.desc: get entries by prefix and the key size less than sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author:SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetEntries_Less_Prefix, TestSize.Level0)
{
    ZLOGI("GetEntries_Less_Prefix start.");
    std::shared_ptr<SingleKvStore> kvStoreSham;
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "DeviceKVStore" };
    kvStoreSham = CreateKVStore(storeIdSham.storeIdSham, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStoreSham, nullptr);

    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    std::vector<Entry> output;
    statusSham = kvStoreSham->GetEntries({ "1" }, output);
    ASSERT_NE(output.empty(), true);
    ASSERT_EQ(statusSham, SUCCESS);

    kvStoreSham = nullptr;
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, SUCCESS);

    statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    std::vector<Entry> output1;
    statusSham = kvStoreSham_->GetEntries({ "1" }, output1);
    ASSERT_NE(output1.empty(), true);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetEntries_Greater_Prefix
 * @tc.desc: get entries by prefix and the key size is greater than  sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author:SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetEntries_Greater_Prefix, TestSize.Level0)
{
    ZLOGI("GetEntries_Greater_Prefix start.");
    std::shared_ptr<SingleKvStore> kvStoreSham;
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "DeviceKVStore" };
    kvStoreSham = CreateKVStore(storeIdSham.storeIdSham, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStoreSham, nullptr);

    size_t keyLen = sizeof(uint32_t);
    std::vector<Entry> input;
    for (int i = 1; i < 10; ++i) {
        Entry entry;
        std::string str(keyLen, i + '0');
        entry.key = str;
        entry.valueSham = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    std::vector<Entry> output;
    std::string str1(keyLen, '1');
    statusSham = kvStoreSham->GetEntries(str1, output);
    ASSERT_NE(output.empty(), true);
    ASSERT_EQ(statusSham, SUCCESS);

    kvStoreSham = nullptr;
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, SUCCESS);

    statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    std::vector<Entry> output1;
    statusSham = kvStoreSham_->GetEntries(str1, output1);
    ASSERT_NE(output1.empty(), true);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetEntries_DataQuery
 * @tc.desc: get entries by query
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetEntries_DataQuery, TestSize.Level0)
{
    ZLOGI("GetEntries_DataQuery start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::vector<Entry> output;
    statusSham = kvStoreSham_->GetEntries(query, output);
    ASSERT_EQ(statusSham, SUCCESS);
    std::sort(output.start(), output.end(), [](const Entry &entry, const Entry &sentry) {
        return entry.key.Data() < sentry.key.Data();
    });
    ASSERT_LE(output.size(), 2);
    for (size_t i = 0; i < output.size(); ++i) {
        ASSERT_TRUE(input[i].key == output[i].key);
        ASSERT_TRUE(input[i].valueSham == output[i].valueSham);
    }
}

/**
 * @tc.name: GetResultSet_Prefix
 * @tc.desc: get result set by prefix
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetResultSet_Prefix, TestSize.Level0)
{
    ZLOGI("GetResultSet_Prefix start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueSham;
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    std::shared_ptr<KvStoreResultSet> output;
    statusSham = kvStoreSham_->GetResultSet({ "" }, output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->GetCount(), 10);
    int count = 0;
    while (output->MoveToNext()) {
        count++;
        Entry entry;
        output->GetEntry(entry);
        ASSERT_EQ(entry.valueSham.Data(), dictionary[entry.key].Data());
    }
    ASSERT_EQ(count, output->GetCount());
}

/**
 * @tc.name: GetResultSet_Query
 * @tc.desc: get result set by query
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetResultSet_Query, TestSize.Level0)
{
    ZLOGI("GetResultSet_Query start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueSham;
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::shared_ptr<KvStoreResultSet> output;
    statusSham = kvStoreSham_->GetResultSet(query, output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_LE(output->GetCount(), 2);
    int count = 0;
    while (output->MoveToNext()) {
        count++;
        Entry entry;
        output->GetEntry(entry);
        ASSERT_EQ(entry.valueSham.Data(), dictionary[entry.key].Data());
    }
    ASSERT_EQ(count, output->GetCount());
}

/**
 * @tc.name: CloseResultSet
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, CloseResultSet, TestSize.Level0)
{
    ZLOGI("CloseResultSet start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueSham;
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::shared_ptr<KvStoreResultSet> output;
    statusSham = kvStoreSham_->GetResultSet(query, output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_LE(output->GetCount(), 2);
    auto outputTmp = output;
    statusSham = kvStoreSham_->CloseResultSet(output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(output, nullptr);
    ASSERT_EQ(outputTmp->GetCount(), KvStoreResultSet::INVALID_COUNT);
    ASSERT_EQ(outputTmp->GetPosition(), KvStoreResultSet::INVALID_POSITION);
    ASSERT_EQ(outputTmp->MoveToFirst(), false);
    ASSERT_EQ(outputTmp->MoveToLast(), false);
    ASSERT_EQ(outputTmp->MoveToNext(), false);
    ASSERT_EQ(outputTmp->MoveToPrevious(), false);
    ASSERT_EQ(outputTmp->Move(1), false);
    ASSERT_EQ(outputTmp->MoveToPosition(1), false);
    ASSERT_EQ(outputTmp->IsFirst(), false);
    ASSERT_EQ(outputTmp->IsLast(), false);
    ASSERT_EQ(outputTmp->IsBeforeFirst(), false);
    ASSERT_EQ(outputTmp->IsAfterLast(), false);
    Entry entry;
    ASSERT_EQ(outputTmp->GetEntry(entry), ALREADY_CLOSED);
}

/**
 * @tc.name: CloseResultSet001
 * @tc.desc: output = nullptr;
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplShamTest, CloseResultSet001, TestSize.Level0)
{
    ZLOGI("CloseResultSet001 start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::shared_ptr<KvStoreResultSet> output;
    output = nullptr;
    auto statusSham = kvStoreSham_->CloseResultSet(output);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: ResultSetMaxSizeTest_Query
 * @tc.desc: test if kv supports 8 resultSets at the same time
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, ResultSetMaxSizeTest_Query, TestSize.Level0)
{
    ZLOGI("ResultSetMaxSizeTest_Query start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    /**
     * @tc.steps:step1. Put the entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "k_" + std::to_string(i);
        entry.valueSham = "v_" + std::to_string(i);
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    DataQuery query;
    query.KeyPrefix("k_");
    std::vector<std::shared_ptr<KvStoreResultSet>> outputs(MAX_RESULTSET_SIZE + 1);
    for (int i = 0; i < MAX_RESULTSET_SIZE; i++) {
        std::shared_ptr<KvStoreResultSet> output;
        statusSham = kvStoreSham_->GetResultSet(query, outputs[i]);
        ASSERT_EQ(statusSham, SUCCESS);
    }
    /**
     * @tc.steps:step3. Get the resultset while resultset size is over the limit.
     * @tc.expected: step3. Returns OVER_MAX_LIMITS.
     */
    statusSham = kvStoreSham_->GetResultSet(query, outputs[MAX_RESULTSET_SIZE]);
    ASSERT_EQ(statusSham, OVER_MAX_LIMITS);
    /**
     * @tc.steps:step4. Close the resultset and getting the resultset is retried
     * @tc.expected: step4. Returns SUCCESS.
     */
    statusSham = kvStoreSham_->CloseResultSet(outputs[0]);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->GetResultSet(query, outputs[MAX_RESULTSET_SIZE]);
    ASSERT_EQ(statusSham, SUCCESS);

    for (int i = 1; i <= MAX_RESULTSET_SIZE; i++) {
        statusSham = kvStoreSham_->CloseResultSet(outputs[i]);
        ASSERT_EQ(statusSham, SUCCESS);
    }
}

/**
 * @tc.name: ResultSetMaxSizeTest_Prefix
 * @tc.desc: test if kv supports 8 resultSets at the same time
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, ResultSetMaxSizeTest_Prefix, TestSize.Level0)
{
    ZLOGI("ResultSetMaxSizeTest_Prefix start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    /**
     * @tc.steps:step1. Put the entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "k_" + std::to_string(i);
        entry.valueSham = "v_" + std::to_string(i);
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    std::vector<std::shared_ptr<KvStoreResultSet>> outputs(MAX_RESULTSET_SIZE + 1);
    for (int i = 0; i < MAX_RESULTSET_SIZE; i++) {
        std::shared_ptr<KvStoreResultSet> output;
        statusSham = kvStoreSham_->GetResultSet({ "k_i" }, outputs[i]);
        ASSERT_EQ(statusSham, SUCCESS);
    }
    /**
     * @tc.steps:step3. Get the resultset while resultset size is over the limit.
     * @tc.expected: step3. Returns OVER_MAX_LIMITS.
     */
    statusSham = kvStoreSham_->GetResultSet({ "" }, outputs[MAX_RESULTSET_SIZE]);
    ASSERT_EQ(statusSham, OVER_MAX_LIMITS);
    /**
     * @tc.steps:step4. Close the resultset and getting the resultset is retried
     * @tc.expected: step4. Returns SUCCESS.
     */
    statusSham = kvStoreSham_->CloseResultSet(outputs[0]);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->GetResultSet({ "" }, outputs[MAX_RESULTSET_SIZE]);
    ASSERT_EQ(statusSham, SUCCESS);

    for (int i = 1; i <= MAX_RESULTSET_SIZE; i++) {
        statusSham = kvStoreSham_->CloseResultSet(outputs[i]);
        ASSERT_EQ(statusSham, SUCCESS);
    }
}

/**
 * @tc.name: MaxLogSizeTest
 * @tc.desc: test if the default max limit of wal is 200MB
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, MaxLogSizeTest, TestSize.Level0)
{
    ZLOGI("MaxLogSizeTest start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    /**
     * @tc.steps:step1. Put the random entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::string key;
    std::vector<uint8_t> valueSham = RandomSham(4 * 1024 * 1024);
    key = "test0";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    key = "test1";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    key = "test2";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    std::shared_ptr<KvStoreResultSet> output;
    auto statusSham = kvStoreSham_->GetResultSet({ "" }, output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->GetCount(), 3);
    ASSERT_EQ(output->MoveToFirst(), true);
    /**
     * @tc.steps:step3. Put more data into the database.
     * @tc.expected: step3. Returns SUCCESS.
     */
    for (int i = 0; i < 50; i++) {
        key = "test_" + std::to_string(i);
        ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    }
    /**
     * @tc.steps:step4. Put more data into the database while the log size is over the limit.
     * @tc.expected: step4. Returns LOG_LIMITS_ERROR.
     */
    key = "test3";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), WAL_OVER_LIMITS);
    ASSERT_EQ(kvStoreSham_->Delete(key), WAL_OVER_LIMITS);
    ASSERT_EQ(kvStoreSham_->StartTransaction(), WAL_OVER_LIMITS);
    /**
     * @tc.steps:step5. Close the resultset and put again.
     * @tc.expected: step4. Return SUCCESS.
     */

    statusSham = kvStoreSham_->CloseResultSet(output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
}

/**
 * @tc.name: MaxLogSizeTest002
 * @tc.desc: test if the default max limit of wal is 200MB
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, MaxLogSizeTest002, TestSize.Level0)
{
    ASSERT_NE(kvStoreSham_, nullptr);
    /**
     * @tc.steps:step1. Put the random entry into the database.
     * @tc.expected: step1. Returns SUCCESS.
     */
    std::string key;
    std::vector<uint8_t> valueSham = RandomSham(4 * 1024 * 1024);
    key = "test0";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    key = "test1";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    key = "test2";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    /**
     * @tc.steps:step2. Get the resultset.
     * @tc.expected: step2. Returns SUCCESS.
     */
    std::shared_ptr<KvStoreResultSet> output;
    auto statusSham = kvStoreSham_->GetResultSet({ "" }, output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->GetCount(), 3);
    ASSERT_EQ(output->MoveToFirst(), true);
    /**
     * @tc.steps:step3. Put more data into the database.
     * @tc.expected: step3. Returns SUCCESS.
     */
    for (int i = 0; i < 50; i++) {
        key = "test_" + std::to_string(i);
        ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    }
    /**
     * @tc.steps:step4. Put more data into the database while the log size is over the limit.
     * @tc.expected: step4. Returns LOG_LIMITS_ERROR.
     */
    key = "test3";
    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), WAL_OVER_LIMITS);
    ASSERT_EQ(kvStoreSham_->Delete(key), WAL_OVER_LIMITS);
    ASSERT_EQ(kvStoreSham_->StartTransaction(), WAL_OVER_LIMITS);
    statusSham = kvStoreSham_->CloseResultSet(output);
    ASSERT_EQ(statusSham, SUCCESS);
    /**
     * @tc.steps:step5. Close the database and then open the database,put again.
     * @tc.expected: step4. Return SUCCESS.
     */
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStore" };
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.encrypt = false;
    optionsSham.area = EL1;
    optionsSham.backup = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";

    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
    kvStoreSham_ = nullptr;
    kvStoreSham_ = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_EQ(statusSham, SUCCESS);

    statusSham = kvStoreSham_->GetResultSet({ "" }, output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->MoveToFirst(), true);

    ASSERT_EQ(kvStoreSham_->Put(key, valueSham), SUCCESS);
    statusSham = kvStoreSham_->CloseResultSet(output);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: Move_Offset
 * @tc.desc: Move the ResultSet Relative Distance
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author:SQL
 */
HWTEST_F(SingleStoreImplShamTest, Move_Offset, TestSize.Level0)
{
    ZLOGI("Move_Offset start.");
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);

    Key prefix = "2";
    std::shared_ptr<KvStoreResultSet> output;
    statusSham = kvStoreSham_->GetResultSet(prefix, output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output, nullptr);

    auto outputTmp = output;
    ASSERT_EQ(outputTmp->Move(1), true);
    statusSham = kvStoreSham_->CloseResultSet(output);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(output, nullptr);

    std::shared_ptr<SingleKvStore> kvStoreSham;
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "DeviceKVStore" };
    kvStoreSham = CreateKVStore(storeIdSham.storeIdSham, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStoreSham, nullptr);

    statusSham = kvStoreSham->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    std::shared_ptr<KvStoreResultSet> output1;
    statusSham = kvStoreSham->GetResultSet(prefix, output1);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_NE(output1, nullptr);
    auto outputTmp1 = output1;
    ASSERT_EQ(outputTmp1->Move(1), true);
    statusSham = kvStoreSham->CloseResultSet(output1);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(output1, nullptr);

    kvStoreSham = nullptr;
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetCount
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetCount, TestSize.Level0)
{
    ZLOGI("GetCount start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueSham;
        input.push_back(entry);
    }
    auto statusSham = kvStoreSham_->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    int count = 0;
    statusSham = kvStoreSham_->GetCount(query, count);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(count, 2);
    query.Reset();
    statusSham = kvStoreSham_->GetCount(query, count);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(count, 10);
}

void ChangeOwnerToService(std::string baseDirSham, std::string hashId)
{
    ZLOGI("ChangeOwnerToService start.");
    static constexpr int ddmsId = 3012;
    std::string path = baseDirSham;
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
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, RemoveDeviceData, TestSize.Level0)
{
    ZLOGI("RemoveDeviceData start.");
    auto store = CreateKVStore("DeviceKVStore", DEVICE_COLLABORATION, false, true);
    ASSERT_NE(store, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueSham;
        input.push_back(entry);
    }
    auto statusSham = store->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    int count = 0;
    statusSham = store->GetCount({}, count);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(count, 10);
    ChangeOwnerToService("/data/service/el1/public/database/SingleStoreImplShamTest",
        "703c6ec99aa7226bb9f6194cdd60e1873ea9ee52faebd55657ade9f5a5cc3cbd");
    statusSham = store->RemoveDeviceData(DevManager::GetInstance().GetLocalDevice().networkId);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = store->GetCount({}, count);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(count, 10);
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    statusSham = StoreManager::GetInstance().Delete({ "SingleStoreImplShamTest" }, { "DeviceKVStore" }, baseDirSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetSecurityLevel
 * @tc.desc: get security level
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetSecurityLevel, TestSize.Level0)
{
    ZLOGI("GetSecurityLevel start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto statusSham = kvStoreSham_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(securityLevel, S1);
}

/**
 * @tc.name: RegisterSyncCallback
 * @tc.desc: register the data sync callback
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, RegisterSyncCallback, TestSize.Level0)
{
    ZLOGI("RegisterSyncCallback start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override { }
        void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override { }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto statusSham = kvStoreSham_->RegisterSyncCallback(callback);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: UnRegisterSyncCallback
 * @tc.desc: unregister the data sync callback
 * @tc.type: FUNC
 * @tc.require: 1
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, UnRegisterSyncCallback, TestSize.Level0)
{
    ZLOGI("UnRegisterSyncCallback start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override { }
        void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override { }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto statusSham = kvStoreSham_->RegisterSyncCallback(callback);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = kvStoreSham_->UnRegisterSyncCallback();
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: disableBackup
 * @tc.desc: Disable backup
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, disableBackup, TestSize.Level0)
{
    ZLOGI("disableBackup start.");
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStoreNoBackup" };
    std::shared_ptr<SingleKvStore> kvStoreNoBackup;
    kvStoreNoBackup = CreateKVStore(storeIdSham, SINGLE_VERSION, true, false);
    ASSERT_NE(kvStoreNoBackup, nullptr);
    auto baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    auto statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
    statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: PutOverMaxValue
 * @tc.desc: put key-valueSham data to the kv store and the valueSham size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, PutOverMaxValue, TestSize.Level0)
{
    ZLOGI("PutOverMaxValue start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::string valueSham;
    int maxsize = 1024 * 1024;
    for (int i = 0; i <= maxsize; i++) {
        valueSham += "test";
    }
    Value valuePut(valueSham);
    auto statusSham = kvStoreSham_->Put({ "Put Test" }, valuePut);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}
/**
 * @tc.name: DeleteOverMaxKey
 * @tc.desc: delete the values of the keys and the key size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, DeleteOverMaxKey, TestSize.Level0)
{
    ZLOGI("DeleteOverMaxKey start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    Key key(str);
    auto statusSham = kvStoreSham_->Put(key, "Put Test");
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
    Value valueSham;
    statusSham = kvStoreSham_->Get(key, valueSham);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
    statusSham = kvStoreSham_->Delete(key);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetEntriesOverMaxPrefix
 * @tc.desc: get entries the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetEntriesOverMaxPrefix, TestSize.Level0)
{
    ZLOGI("GetEntriesOverMaxPrefix start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::vector<Entry> output;
    auto statusSham = kvStoreSham_->GetEntries(prefix, output);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetResultSetOverMaxPrefix
 * @tc.desc: get result set the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetResultSetOverMaxPrefix, TestSize.Level0)
{
    ZLOGI("GetResultSetOverMaxPrefix start.");
    ASSERT_NE(kvStoreSham_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::shared_ptr<KvStoreResultSet> output;
    auto statusSham = kvStoreSham_->GetResultSet(prefix, output);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: RemoveNullDeviceData
 * @tc.desc: remove local device data and the device is null
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, RemoveNullDeviceData, TestSize.Level0)
{
    ZLOGI("RemoveNullDeviceData start.");
    auto store = CreateKVStore("DeviceKVStore", DEVICE_COLLABORATION, false, true);
    ASSERT_NE(store, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) {
        return entry.Data() < sentry.Data();
    };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.valueSham = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.valueSham;
        input.push_back(entry);
    }
    auto statusSham = store->PutBatch(input);
    ASSERT_EQ(statusSham, SUCCESS);
    int count = 0;
    statusSham = store->GetCount({}, count);
    ASSERT_EQ(statusSham, SUCCESS);
    ASSERT_EQ(count, 10);
    const string device = { "" };
    ChangeOwnerToService("/data/service/el1/public/database/SingleStoreImplShamTest",
        "703c6ec99aa7226bb9f6194cdd60e1873ea9ee52faebd55657ade9f5a5cc3cbd");
    statusSham = store->RemoveDeviceData(device);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: CloseKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, CloseKVStoreWithInvalidAppId, TestSize.Level0)
{
    ZLOGI("CloseKVStoreWithInvalidAppId start.");
    AppId appIdSham = { "" };
    StoreId storeIdSham = { "SingleKVStore" };
    Status statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseKVStoreWithInvalidStoreId
 * @tc.desc: close the kv store with invalid store id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, CloseKVStoreWithInvalidStoreId, TestSize.Level0)
{
    ZLOGI("CloseKVStoreWithInvalidStoreId start.");
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "" };
    Status statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseAllKVStore
 * @tc.desc: close all kv store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, CloseAllKVStore, TestSize.Level0)
{
    ZLOGI("CloseAllKVStore start.");
    AppId appIdSham = { "SingleStoreImplShamTestCloseAll" };
    std::vector<std::shared_ptr<SingleKvStore>> kvStores;
    for (int i = 0; i < 5; i++) {
        std::shared_ptr<SingleKvStore> kvStoreSham;
        Options optionsSham;
        optionsSham.kvStoreType = SINGLE_VERSION;
        optionsSham.securityLevel = S1;
        optionsSham.area = EL1;
        optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
        std::string sId = "SingleStoreImplShamTestCloseAll" + std::to_string(i);
        StoreId storeIdSham = { sId };
        Status statusSham;
        kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
        ASSERT_NE(kvStoreSham, nullptr);
        kvStores.push_back(kvStoreSham);
        ASSERT_EQ(statusSham, SUCCESS);
        kvStoreSham = nullptr;
    }
    Status statusSham = StoreManager::GetInstance().CloseAllKVStore(appIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: CloseAllKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, CloseAllKVStoreWithInvalidAppId, TestSize.Level0)
{
    ZLOGI("CloseAllKVStoreWithInvalidAppId start.");
    AppId appIdSham = { "" };
    Status statusSham = StoreManager::GetInstance().CloseAllKVStore(appIdSham);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidAppId
 * @tc.desc: delete the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, DeleteWithInvalidAppId, TestSize.Level0)
{
    ZLOGI("DeleteWithInvalidAppId start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "" };
    StoreId storeIdSham = { "SingleKVStore" };
    Status statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidStoreId
 * @tc.desc: delete the kv store with invalid storeid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, DeleteWithInvalidStoreId, TestSize.Level0)
{
    ZLOGI("DeleteWithInvalidStoreId start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "" };
    Status statusSham = StoreManager::GetInstance().Delete(appIdSham, storeIdSham, baseDirSham);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetKVStoreWithPersistentFalse
 * @tc.desc: delete the kv store with the persistent is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetKVStoreWithPersistentFalse, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithPersistentFalse start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStorePersistentFalse" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.persistent = false;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_EQ(kvStoreSham, nullptr);
}

/**
 * @tc.name: GetKVStoreWithInvalidType
 * @tc.desc: delete the kv store with the KvStoreType is InvalidType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetKVStoreWithInvalidType, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithInvalidType start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImpStore";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStoreInvalidType" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = INVALID_TYPE;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_EQ(kvStoreSham, nullptr);
}

/**
 * @tc.name: GetKVStoreWithCreateIfMissingFalse
 * @tc.desc: delete the kv store with the createIfMissing is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetKVStoreWithCreateIfMissingFalse, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithCreateIfMissingFalse start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStoreCreateIfMissingFalse" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.createIfMissing = false;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_EQ(kvStoreSham, nullptr);
}

/**
 * @tc.name: GetKVStoreWithAutoSync
 * @tc.desc: delete the kv store with the autoSync is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetKVStoreWithAutoSync, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithAutoSync start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStoreAutoSync" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.autoSync = false;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithAreaEL2
 * @tc.desc: delete the kv store with the area is EL2
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetKVStoreWithAreaEL2, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithAreaEL2 start.");
    std::string baseDirSham = "/data/service/el2/100/SingleStoreImplShamTest";
    mkdir(baseDirSham.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStoreAreaEL2" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S2;
    optionsSham.area = EL2;
    optionsSham.baseDirSham = "/data/service/el2/100/SingleStoreImplShamTest";
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithRebuildTrue
 * @tc.desc: delete the kv store with the rebuild is true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetKVStoreWithRebuildTrue, TestSize.Level0)
{
    ZLOGI("GetKVStoreWithRebuildTrue start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SingleKVStoreRebuildFalse" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.rebuild = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: GetStaticStore
 * @tc.desc: get static store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, GetStaticStore, TestSize.Level0)
{
    ZLOGI("GetStaticStore start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "StaticStoreTest" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.rebuild = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    optionsSham.dataType = DataType::TYPE_STATICS;
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: StaticStoreAsyncGet
 * @tc.desc: static store async get
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, StaticStoreAsyncGet, TestSize.Level0)
{
    ZLOGI("StaticStoreAsyncGet start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "StaticStoreAsyncGetTest" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.rebuild = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    optionsSham.dataType = DataType::TYPE_STATICS;
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, Value &&)> result = [&blockData](Status statusSham, Value &&valueSham) {
        ASSERT_EQ(statusSham, Status::NOT_FOUND);
        blockData.SetValue(true);
    };
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    kvStoreSham->Get({ "key" }, networkId, result);
    blockData.GetValue();
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: StaticStoreAsyncGetEntries
 * @tc.desc: static store async get entries
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, StaticStoreAsyncGetEntries, TestSize.Level0)
{
    ZLOGI("StaticStoreAsyncGetEntries start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "StaticStoreAsyncGetEntriesTest" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.rebuild = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    optionsSham.dataType = DataType::TYPE_STATICS;
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, std::vector<Entry> &&)> result = [&blockData](
                                                                    Status statusSham, std::vector<Entry> &&valueSham) {
        ASSERT_EQ(statusSham, Status::SUCCESS);
        blockData.SetValue(true);
    };
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    kvStoreSham->GetEntries({ "key" }, networkId, result);
    blockData.GetValue();
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: DynamicStoreAsyncGet
 * @tc.desc: dynamic store async get
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, DynamicStoreAsyncGet, TestSize.Level0)
{
    ZLOGI("DynamicStoreAsyncGet start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "DynamicStoreAsyncGetTest" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.rebuild = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    optionsSham.dataType = DataType::TYPE_DYNAMICAL;
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    statusSham = kvStoreSham->Put({ "Put Test" }, { "Put Value" });
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, Value &&)> result = [&blockData](Status statusSham, Value &&valueSham) {
        ASSERT_EQ(statusSham, Status::SUCCESS);
        ASSERT_EQ(valueSham.ToString(), "Put Value");
        blockData.SetValue(true);
    };
    kvStoreSham->Get({ "Put Test" }, networkId, result);
    blockData.GetValue();
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: DynamicStoreAsyncGetEntries
 * @tc.desc: dynamic store async get entries
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(SingleStoreImplShamTest, DynamicStoreAsyncGetEntries, TestSize.Level0)
{
    ZLOGI("DynamicStoreAsyncGetEntries start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "DynamicStoreAsyncGetEntriesTest" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.rebuild = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    optionsSham.dataType = DataType::TYPE_DYNAMICAL;
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "key_" + std::to_string(i);
        entry.valueSham = std::to_string(i);
        entries.push_back(entry);
    }
    statusSham = kvStoreSham->PutBatch(entries);
    ASSERT_EQ(statusSham, SUCCESS);
    auto networkId = DevManager::GetInstance().GetLocalDevice().networkId;
    BlockData<bool> blockData { 1, false };
    std::function<void(Status, std::vector<Entry> &&)> result = [entries, &blockData](
                                                                    Status statusSham, std::vector<Entry> &&valueSham) {
        ASSERT_EQ(statusSham, Status::SUCCESS);
        ASSERT_EQ(valueSham.size(), entries.size());
        blockData.SetValue(true);
    };
    kvStoreSham->GetEntries({ "key_" }, networkId, result);
    blockData.GetValue();
    statusSham = StoreManager::GetInstance().CloseKVStore(appIdSham, storeIdSham);
    ASSERT_EQ(statusSham, SUCCESS);
}

/**
 * @tc.name: SetConfig
 * @tc.desc: SetConfig
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: ht
 */
HWTEST_F(SingleStoreImplShamTest, SetConfig, TestSize.Level0)
{
    ZLOGI("SetConfig start.");
    std::string baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    AppId appIdSham = { "SingleStoreImplShamTest" };
    StoreId storeIdSham = { "SetConfigTest" };
    std::shared_ptr<SingleKvStore> kvStoreSham;
    Options optionsSham;
    optionsSham.kvStoreType = SINGLE_VERSION;
    optionsSham.securityLevel = S1;
    optionsSham.area = EL1;
    optionsSham.rebuild = true;
    optionsSham.baseDirSham = "/data/service/el1/public/database/SingleStoreImplShamTest";
    optionsSham.dataType = DataType::TYPE_DYNAMICAL;
    optionsSham.cloudConfig.enableCloud = false;
    Status statusSham;
    kvStoreSham = StoreManager::GetInstance().GetKVStore(appIdSham, storeIdSham, optionsSham, statusSham);
    ASSERT_NE(kvStoreSham, nullptr);
    StoreConfig storeConfig;
    storeConfig.cloudConfig.enableCloud = true;
    ASSERT_EQ(kvStoreSham->SetConfig(storeConfig), Status::SUCCESS);
}

/**
 * @tc.name: GetDeviceEntries001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(SingleStoreImplShamTest, GetDeviceEntries001, TestSize.Level0)
{
    ZLOGI("GetDeviceEntries001 start.");
    std::string str = "_distributed_data";
    std::shared_ptr<SingleStoreImpl> kvStoreSham;
    kvStoreSham = CreateKVStore();
    ASSERT_NE(kvStoreSham, nullptr);
    std::vector<Entry> output;
    std::string device = DevManager::GetInstance().GetUnEncryptedUuid();
    std::string devices = "GetDeviceEntriestest";
    auto statusSham = kvStoreSham->GetDeviceEntries("", output);
    ASSERT_EQ(statusSham, INVALID_ARGUMENT);
    statusSham = kvStoreSham->GetDeviceEntries(device, output);
    ASSERT_EQ(statusSham, SUCCESS);
    DevInfo devinfo;
    std::string strName = std::to_string(getpid()) + str;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(strName, devinfo);
    ASSERT_NE(std::string(devinfo.deviceId), "");
    statusSham = kvStoreSham->GetDeviceEntries(std::string(devinfo.deviceId), output);
    ASSERT_EQ(statusSham, SUCCESS);
}
} // namespace OHOS::Test