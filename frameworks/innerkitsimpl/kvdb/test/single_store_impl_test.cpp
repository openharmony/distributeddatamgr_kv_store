/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "dev_manager.h"
#include "distributed_kv_data_manager.h"
#include "store_manager.h"
#include "sys/stat.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class SingleStoreImplTest : public testing::Test {
public:
    class TestObserver : public KvStoreObserver {
    public:
        TestObserver()
        {
            data_ = std::make_shared<OHOS::BlockData<bool>>(5, false);
        }
        void OnChange(const ChangeNotification &notification) override
        {
            insert_ = notification.GetInsertEntries();
            update_ = notification.GetUpdateEntries();
            delete_ = notification.GetDeleteEntries();
            deviceId_ = notification.GetDeviceId();
            bool value = true;
            data_->SetValue(value);
        }
        std::vector<Entry> insert_;
        std::vector<Entry> update_;
        std::vector<Entry> delete_;
        std::string deviceId_;

        std::shared_ptr<OHOS::BlockData<bool>> data_;
    };

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, KvStoreType type, bool encrypt, bool backup);
    std::shared_ptr<SingleKvStore> kvStore_;
};

void SingleStoreImplTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SingleStoreImplTest::TearDownTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    StoreManager::GetInstance().Delete({ "SingleStoreImplTest" }, { "SingleKVStore" }, baseDir);

    (void)remove("/data/service/el1/public/database/SingleStoreImplTest/key");
    (void)remove("/data/service/el1/public/database/SingleStoreImplTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleStoreImplTest");
}

void SingleStoreImplTest::SetUp(void)
{
    kvStore_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false, true);
    if (kvStore_ == nullptr) {
        kvStore_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false, true);
    }
    ASSERT_NE(kvStore_, nullptr);
}

void SingleStoreImplTest::TearDown(void)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStore" };
    kvStore_ = nullptr;
    auto status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    auto baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

std::shared_ptr<SingleKvStore> SingleStoreImplTest::CreateKVStore(std::string storeIdTest, KvStoreType type,
    bool encrypt, bool backup)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.encrypt = encrypt;
    options.area = EL1;
    options.backup = backup;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";

    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { storeIdTest };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
}

/**
 * @tc.name: GetStoreId
 * @tc.desc: get the store id of the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, GetStoreId, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto storeId = kvStore_->GetStoreId();
    ASSERT_EQ(storeId.storeId, "SingleKVStore");
}

/**
 * @tc.name: Put
 * @tc.desc: put key-value data to the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, Put, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto status = kvStore_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Put({ "   Put Test" }, { "Put2 Value" });
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore_->Get({ "Put Test" }, value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(value.ToString(), "Put2 Value");
}

/**
 * @tc.name: Put_Invalid_Key
 * @tc.desc: put invalid key-value data to the device kv store and single kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: wu fengshan
 */
HWTEST_F(SingleStoreImplTest, Put_Invalid_Key, TestSize.Level0)
{
    std::shared_ptr<SingleKvStore> kvStore;
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "DeviceKVStore" };
    kvStore = CreateKVStore(storeId.storeId, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStore, nullptr);

    size_t MAX_DEV_KEY_LEN = 897;
    std::string str(MAX_DEV_KEY_LEN, 'a');
    Blob key(str);
    Blob value("test_value");
    Status status = kvStore->Put(key, value);
    EXPECT_EQ(status, INVALID_ARGUMENT);

    Blob key1("");
    Blob value1("test_value1");
    status = kvStore->Put(key1, value1);
    EXPECT_EQ(status, INVALID_ARGUMENT);

    kvStore = nullptr;
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);

    size_t MAX_SINGLE_KEY_LEN = 1025;
    std::string str1(MAX_SINGLE_KEY_LEN, 'b');
    Blob key2(str1);
    Blob value2("test_value2");
    status = kvStore_->Put(key2, value2);
    EXPECT_EQ(status, INVALID_ARGUMENT);

    status = kvStore_->Put(key1, value1);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: PutBatch
 * @tc.desc: put some key-value data to the kv store
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, PutBatch, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto status = kvStore_->PutBatch(entries);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: Delete
 * @tc.desc: delete the value of the key
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, Delete, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto status = kvStore_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore_->Get({ "Put Test" }, value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(std::string("Put Value"), value.ToString());
    status = kvStore_->Delete({ "Put Test" });
    ASSERT_EQ(status, SUCCESS);
    value = {};
    status = kvStore_->Get({ "Put Test" }, value);
    ASSERT_EQ(status, KEY_NOT_FOUND);
    ASSERT_EQ(std::string(""), value.ToString());
}

/**
 * @tc.name: DeleteBatch
 * @tc.desc: delete the values of the keys
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, DeleteBatch, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto status = kvStore_->PutBatch(entries);
    ASSERT_EQ(status, SUCCESS);
    std::vector<Key> keys;
    for (int i = 0; i < 10; ++i) {
        Key key = std::to_string(i).append("_k");
        keys.push_back(key);
    }
    status = kvStore_->DeleteBatch(keys);
    ASSERT_EQ(status, SUCCESS);
    for (int i = 0; i < 10; ++i) {
        Value value;
        status = kvStore_->Get(keys[i], value);
        ASSERT_EQ(status, KEY_NOT_FOUND);
        ASSERT_EQ(value.ToString(), std::string(""));
    }
}

/**
 * @tc.name: Transaction
 * @tc.desc: do transaction
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, Transaction, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto status = kvStore_->StartTransaction();
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Commit();
    ASSERT_EQ(status, SUCCESS);

    status = kvStore_->StartTransaction();
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Rollback();
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: SubscribeKvStore
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, SubscribeKvStore, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto observer = std::make_shared<TestObserver>();
    auto status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    bool invalidValue = false;
    observer->data_->Clear(invalidValue);
    status = kvStore_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    ASSERT_TRUE(observer->data_->GetValue());
    ASSERT_EQ(observer->insert_.size(), 1);
    ASSERT_EQ(observer->update_.size(), 0);
    ASSERT_EQ(observer->delete_.size(), 0);
    observer->data_->Clear(invalidValue);
    status = kvStore_->Put({ "Put Test" }, { "Put Value1" });
    ASSERT_EQ(status, SUCCESS);
    ASSERT_TRUE(observer->data_->GetValue());
    ASSERT_EQ(observer->insert_.size(), 0);
    ASSERT_EQ(observer->update_.size(), 1);
    ASSERT_EQ(observer->delete_.size(), 0);
    observer->data_->Clear(invalidValue);
    status = kvStore_->Delete({ "Put Test" });
    ASSERT_EQ(status, SUCCESS);
    ASSERT_TRUE(observer->data_->GetValue());
    ASSERT_EQ(observer->insert_.size(), 0);
    ASSERT_EQ(observer->update_.size(), 0);
    ASSERT_EQ(observer->delete_.size(), 1);
}

/**
 * @tc.name: SubscribeKvStore002
 * @tc.desc: subscribe local
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Hollokin
 */
HWTEST_F(SingleStoreImplTest, SubscribeKvStore002, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::shared_ptr<TestObserver> subscribedObserver;
    std::shared_ptr<TestObserver> unSubscribedObserver;
    for (int i = 0; i < 15; ++i) {
        auto observer = std::make_shared<TestObserver>();
        auto status1 = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
        auto status2 = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
        if (i < 8) {
            ASSERT_EQ(status1, SUCCESS);
            ASSERT_EQ(status2, SUCCESS);
            subscribedObserver = observer;
        } else {
            ASSERT_EQ(status1, OVER_MAX_SUBSCRIBE_LIMITS);
            ASSERT_EQ(status2, OVER_MAX_SUBSCRIBE_LIMITS);
            unSubscribedObserver = observer;
        }
    }

    auto status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);

    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, {});
    ASSERT_EQ(status, INVALID_ARGUMENT);

    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, subscribedObserver);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);

    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserver);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);

    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(status, SUCCESS);

    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, subscribedObserver);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, unSubscribedObserver);
    ASSERT_EQ(status, SUCCESS);
    subscribedObserver = unSubscribedObserver;
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, subscribedObserver);
    ASSERT_EQ(status, SUCCESS);
    auto observer = std::make_shared<TestObserver>();
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, SUCCESS);
    observer = std::make_shared<TestObserver>();
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, OVER_MAX_SUBSCRIBE_LIMITS);
}

/**
 * @tc.name: UnsubscribeKvStore
 * @tc.desc: unsubscribe
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, UnsubscribeKvStore, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto observer = std::make_shared<TestObserver>();
    auto status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, STORE_NOT_SUBSCRIBE);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, STORE_NOT_SUBSCRIBE);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, STORE_NOT_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: GetEntries
 * @tc.desc: get entries by prefix
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, GetEntries_Prefix, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    std::vector<Entry> output;
    status = kvStore_->GetEntries({ "" }, output);
    ASSERT_EQ(status, SUCCESS);
    std::sort(output.begin(), output.end(),
        [](const Entry &entry, const Entry &sentry) { return entry.key.Data() < sentry.key.Data(); });
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(input[i].key == output[i].key);
        ASSERT_TRUE(input[i].value == output[i].value);
    }
}

/**
 * @tc.name: GetEntries_Less_Prefix
 * @tc.desc: get entries by prefix and the key size less than sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: wu fengshan
 */
HWTEST_F(SingleStoreImplTest, GetEntries_Less_Prefix, TestSize.Level0)
{
    std::shared_ptr<SingleKvStore> kvStore;
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "DeviceKVStore" };
    kvStore = CreateKVStore(storeId.storeId, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStore, nullptr);

    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto status = kvStore->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    std::vector<Entry> output;
    status = kvStore->GetEntries({"1"}, output);
    ASSERT_NE(output.empty(), true);
    ASSERT_EQ(status, SUCCESS);

    kvStore = nullptr;
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);

    status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    std::vector<Entry> output1;
    status = kvStore_->GetEntries({"1"}, output1);
    ASSERT_NE(output1.empty(), true);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: GetEntries_Greater_Prefix
 * @tc.desc: get entries by prefix and the key size is greater than  sizeof(uint32_t)
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: wu fengshan
 */
HWTEST_F(SingleStoreImplTest, GetEntries_Greater_Prefix, TestSize.Level0)
{
    std::shared_ptr<SingleKvStore> kvStore;
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "DeviceKVStore" };
    kvStore = CreateKVStore(storeId.storeId, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStore, nullptr);

    size_t KEY_LEN = sizeof(uint32_t);
    std::vector<Entry> input;
    for (int i = 1; i < 10; ++i) {
        Entry entry;
        std::string str(KEY_LEN, i + '0');
        entry.key = str;
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto status = kvStore->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    std::vector<Entry> output;
    std::string str1(KEY_LEN, '1');
    status = kvStore->GetEntries(str1, output);
    ASSERT_NE(output.empty(), true);
    ASSERT_EQ(status, SUCCESS);

    kvStore = nullptr;
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);

    status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    std::vector<Entry> output1;
    status = kvStore_->GetEntries(str1, output1);
    ASSERT_NE(output1.empty(), true);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: GetEntries
 * @tc.desc: get entries by query
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, GetEntries_DataQuery, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::vector<Entry> output;
    status = kvStore_->GetEntries(query, output);
    ASSERT_EQ(status, SUCCESS);
    std::sort(output.begin(), output.end(),
        [](const Entry &entry, const Entry &sentry) { return entry.key.Data() < sentry.key.Data(); });
    ASSERT_LE(output.size(), 2);
    for (size_t i = 0; i < output.size(); ++i) {
        ASSERT_TRUE(input[i].key == output[i].key);
        ASSERT_TRUE(input[i].value == output[i].value);
    }
}

/**
 * @tc.name: GetResultSet
 * @tc.desc: get result set by prefix
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, GetResultSet_Prefix, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.value;
        input.push_back(entry);
    }
    auto status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    std::shared_ptr<KvStoreResultSet> output;
    status = kvStore_->GetResultSet({ "" }, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->GetCount(), 10);
    int count = 0;
    while (output->MoveToNext()) {
        count++;
        Entry entry;
        output->GetEntry(entry);
        ASSERT_EQ(entry.value.Data(), dictionary[entry.key].Data());
    }
    ASSERT_EQ(count, output->GetCount());
}

/**
 * @tc.name: GetResultSet
 * @tc.desc: get result set by query
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, GetResultSet_Query, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.value;
        input.push_back(entry);
    }
    auto status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::shared_ptr<KvStoreResultSet> output;
    status = kvStore_->GetResultSet(query, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_LE(output->GetCount(), 2);
    int count = 0;
    while (output->MoveToNext()) {
        count++;
        Entry entry;
        output->GetEntry(entry);
        ASSERT_EQ(entry.value.Data(), dictionary[entry.key].Data());
    }
    ASSERT_EQ(count, output->GetCount());
}

/**
 * @tc.name: CloseResultSet
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, CloseResultSet, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.value;
        input.push_back(entry);
    }
    auto status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    std::shared_ptr<KvStoreResultSet> output;
    status = kvStore_->GetResultSet(query, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_LE(output->GetCount(), 2);
    auto outputTmp = output;
    status = kvStore_->CloseResultSet(output);
    ASSERT_EQ(status, SUCCESS);
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
 * @tc.name: Move_Offset
 * @tc.desc: Move the ResultSet Relative Distance
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: wu fengshan
 */
HWTEST_F(SingleStoreImplTest, Move_Offset, TestSize.Level0)
{
    std::vector<Entry> input;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
    }
    auto status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);

    Key prefix = "2";
    std::shared_ptr<KvStoreResultSet> output;
    status = kvStore_->GetResultSet(prefix, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(output, nullptr);

    auto outputTmp = output;
    ASSERT_EQ(outputTmp->Move(1), true);
    status = kvStore_->CloseResultSet(output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(output, nullptr);

    std::shared_ptr<SingleKvStore> kvStore;
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "DeviceKVStore" };
    kvStore = CreateKVStore(storeId.storeId, DEVICE_COLLABORATION, false, true);
    ASSERT_NE(kvStore, nullptr);

    status = kvStore->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    std::shared_ptr<KvStoreResultSet> output1;
    status = kvStore->GetResultSet(prefix, output1);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(output1, nullptr);
    auto outputTmp1 = output1;
    ASSERT_EQ(outputTmp1->Move(1), true);
    status = kvStore->CloseResultSet(output1);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(output1, nullptr);

    kvStore = nullptr;
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: GetCount
 * @tc.desc: close the result set
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, GetCount, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.value;
        input.push_back(entry);
    }
    auto status = kvStore_->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    DataQuery query;
    query.InKeys({ "0_k", "1_k" });
    int count = 0;
    status = kvStore_->GetCount(query, count);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(count, 2);
    query.Reset();
    status = kvStore_->GetCount(query, count);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(count, 10);
}

/**
 * @tc.name: RemoveDeviceData
 * @tc.desc: remove local device data
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, RemoveDeviceData, TestSize.Level0)
{
    auto store = CreateKVStore("DeviceKVStore", DEVICE_COLLABORATION, false, true);
    ASSERT_NE(store, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.value;
        input.push_back(entry);
    }
    auto status = store->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    int count = 0;
    status = store->GetCount({}, count);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(count, 10);
    status = store->RemoveDeviceData(DevManager::GetInstance().GetLocalDevice().networkId);
    ASSERT_EQ(status, SUCCESS);
    status = store->GetCount({}, count);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(count, 10);
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    status = StoreManager::GetInstance().Delete({ "SingleStoreImplTest" }, { "DeviceKVStore" }, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: GetSecurityLevel
 * @tc.desc: get security level
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, GetSecurityLevel, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(securityLevel, S1);
}

/**
 * @tc.name: RegisterSyncCallback
 * @tc.desc: register the data sync callback
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, RegisterSyncCallback, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override
        {
        }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto status = kvStore_->RegisterSyncCallback(callback);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: UnRegisterSyncCallback
 * @tc.desc: unregister the data sync callback
 * @tc.type: FUNC
 * @tc.require: I4XVQQ
 * @tc.author: Sven Wang
 */
HWTEST_F(SingleStoreImplTest, UnRegisterSyncCallback, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const map<std::string, Status> &results) override
        {
        }
    };
    auto callback = std::make_shared<TestSyncCallback>();
    auto status = kvStore_->RegisterSyncCallback(callback);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnRegisterSyncCallback();
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: disableBackup
* @tc.desc: Disable backup
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(SingleStoreImplTest, disableBackup, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStoreNoBackup" };
    std::shared_ptr<SingleKvStore> kvStoreNoBackup;
    kvStoreNoBackup = CreateKVStore(storeId, SINGLE_VERSION, true, false);
    ASSERT_NE(kvStoreNoBackup, nullptr);
    auto baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    auto status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: PutOverMaxValue
 * @tc.desc: put key-value data to the kv store and the value size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, PutOverMaxValue, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::string value;
    int maxsize = 1024 * 1024;
    for (int i = 0; i <= maxsize; i++) {
        value += "test";
    }
    Value valuePut(value);
    auto status = kvStore_->Put({ "Put Test" }, valuePut);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}
/**
 * @tc.name: DeleteOverMaxKey
 * @tc.desc: delete the values of the keys and the key size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, DeleteOverMaxKey, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    Key key(str);
    auto status = kvStore_->Put(key, "Put Test");
    ASSERT_EQ(status, INVALID_ARGUMENT);
    Value value;
    status = kvStore_->Get(key, value);
    ASSERT_EQ(status, INVALID_ARGUMENT);
    status = kvStore_->Delete(key);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetEntriesOverMaxKey
 * @tc.desc: get entries the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetEntriesOverMaxPrefix, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::vector<Entry> output;
    auto status = kvStore_->GetEntries(prefix, output);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetResultSetOverMaxPrefix
 * @tc.desc: get result set the by prefix and the prefix size  over the limits
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetResultSetOverMaxPrefix, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::string str;
    int maxsize = 1024;
    for (int i = 0; i <= maxsize; i++) {
        str += "key";
    }
    const Key prefix(str);
    std::shared_ptr<KvStoreResultSet> output;
    auto status = kvStore_->GetResultSet(prefix, output);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: RemoveNullDeviceData
 * @tc.desc: remove local device data and the device is null
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, RemoveNullDeviceData, TestSize.Level0)
{
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
        entry.value = std::to_string(i).append("_v");
        dictionary[entry.key] = entry.value;
        input.push_back(entry);
    }
    auto status = store->PutBatch(input);
    ASSERT_EQ(status, SUCCESS);
    int count = 0;
    status = store->GetCount({}, count);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(count, 10);
    const string device = { "" };
    status = store->RemoveDeviceData(device);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: CloseKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplTest, CloseKVStoreWithInvalidAppId, TestSize.Level0)
{
    AppId appId = { "" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseKVStoreWithInvalidStoreId
 * @tc.desc: close the kv store with invalid store id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplTest, CloseKVStoreWithInvalidStoreId, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "" };
    Status status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: CloseAllKVStore
 * @tc.desc: close all kv store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplTest, CloseAllKVStore, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTestCloseAll" };
    std::vector<std::shared_ptr<SingleKvStore>> kvStores;
    for (int i = 0; i < 5; i++) {
        std::shared_ptr<SingleKvStore> kvStore;
        Options options;
        options.kvStoreType = SINGLE_VERSION;
        options.securityLevel = S1;
        options.area = EL1;
        options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
        std::string sId = "SingleStoreImplTestCloseAll" + std::to_string(i);
        StoreId storeId = { sId };
        Status status;
        kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
        ASSERT_NE(kvStore, nullptr);
        kvStores.push_back(kvStore);
        ASSERT_EQ(status, SUCCESS);
        kvStore = nullptr;
    }
    Status status = StoreManager::GetInstance().CloseAllKVStore(appId);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: CloseAllKVStoreWithInvalidAppId
 * @tc.desc: close the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplTest, CloseAllKVStoreWithInvalidAppId, TestSize.Level0)
{
    AppId appId = { "" };
    Status status = StoreManager::GetInstance().CloseAllKVStore(appId);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidAppId
 * @tc.desc: delete the kv store with invalid appid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplTest, DeleteWithInvalidAppId, TestSize.Level0)
{
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    AppId appId = { "" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: DeleteWithInvalidStoreId
 * @tc.desc: delete the kv store with invalid storeid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Yang Qing
 */
HWTEST_F(SingleStoreImplTest, DeleteWithInvalidStoreId, TestSize.Level0)
{
    std::string baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}

/**
 * @tc.name: GetKVStoreWithPersistentFalse
 * @tc.desc: delete the kv store with the persistent is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetKVStoreWithPersistentFalse, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStorePersistentFalse" };
    std::shared_ptr<SingleKvStore> kvStore;
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.persistent = false;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    Status status;
    kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(kvStore, nullptr);
}

/**
 * @tc.name: GetKVStoreWithInvalidType
 * @tc.desc: delete the kv store with the KvStoreType is InvalidType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetKVStoreWithInvalidType, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStoreInvalidType" };
    std::shared_ptr<SingleKvStore> kvStore;
    Options options;
    options.kvStoreType = INVALID_TYPE;
    options.securityLevel = S1;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    Status status;
    kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(kvStore, nullptr);
}

/**
 * @tc.name: GetKVStoreWithCreateIfMissingFalse
 * @tc.desc: delete the kv store with the createIfMissing is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetKVStoreWithCreateIfMissingFalse, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStoreCreateIfMissingFalse" };
    std::shared_ptr<SingleKvStore> kvStore;
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.createIfMissing = false;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    Status status;
    kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(kvStore, nullptr);
}

/**
 * @tc.name: GetKVStoreWithAutoSync
 * @tc.desc: delete the kv store with the autoSync is false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetKVStoreWithAutoSync, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStoreAutoSync" };
    std::shared_ptr<SingleKvStore> kvStore;
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.autoSync = false;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    Status status;
    kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore, nullptr);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithAreaEL2
 * @tc.desc: delete the kv store with the area is EL2
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetKVStoreWithAreaEL2, TestSize.Level0)
{
    std::string baseDir = "/data/service/el2/100/SingleStoreImplTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStoreAreaEL2" };
    std::shared_ptr<SingleKvStore> kvStore;
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S2;
    options.area = EL2;
    options.baseDir = "/data/service/el2/100/SingleStoreImplTest";
    Status status;
    kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore, nullptr);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
}

/**
 * @tc.name: GetKVStoreWithRebuildTrue
 * @tc.desc: delete the kv store with the rebuild is true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(SingleStoreImplTest, GetKVStoreWithRebuildTrue, TestSize.Level0)
{
    AppId appId = { "SingleStoreImplTest" };
    StoreId storeId = { "SingleKVStoreRebuildFalse" };
    std::shared_ptr<SingleKvStore> kvStore;
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.rebuild = true;
    options.baseDir = "/data/service/el1/public/database/SingleStoreImplTest";
    Status status;
    kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore, nullptr);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
}