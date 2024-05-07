/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "block_data.h"
#include "dev_manager.h"
#include "distributed_kv_data_manager.h"
#include "file_ex.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class SingleKvStoreAsyncGetTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();

    static std::shared_ptr<SingleKvStore> singleKvStore;
    static Status status_;
};

std::shared_ptr<SingleKvStore> SingleKvStoreAsyncGetTest::singleKvStore = nullptr;
Status SingleKvStoreAsyncGetTest::status_ = Status::ERROR;

void SingleKvStoreAsyncGetTest::SetUpTestCase(void)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .dataType = DataType::TYPE_DYNAMICAL };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appId = { "asyncgettest" };
    StoreId storeId = { "asyncgettest_store_0" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    status_ = manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
}

void SingleKvStoreAsyncGetTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/asyncgettest/key");
    (void)remove("/data/service/el1/public/database/asyncgettest/kvdb");
    (void)remove("/data/service/el1/public/database/asyncgettest");
}

void SingleKvStoreAsyncGetTest::SetUp(void)
{}

void SingleKvStoreAsyncGetTest::TearDown(void)
{}

/**
* @tc.name: CreateDefaultKvStore
* @tc.desc: get a single KvStore instance, default is dynamic.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, CreateDefaultKvStore, TestSize.Level1)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .kvStoreType = KvStoreType::SINGLE_VERSION};
    EXPECT_EQ(options.autoSync, false);
    EXPECT_EQ(options.dataType, DataType::TYPE_DYNAMICAL);
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appId = { "asyncgettest" };
    StoreId storeId = { "asyncgettest_store_1" };

    std::shared_ptr<SingleKvStore> store = nullptr;
    auto status = manager.GetSingleKvStore(options, appId, storeId, store);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_NE(store, nullptr);
    status = manager.CloseKvStore(appId, storeId);
    EXPECT_EQ(status, Status::SUCCESS);
    status = manager.DeleteKvStore(appId, storeId, options.baseDir);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: CreateStaticKvStore
* @tc.desc: get a single KvStore instance, data type is STATICS.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, CreateStaticKvStore, TestSize.Level1)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .autoSync = true, .dataType = DataType::TYPE_STATICS };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appId = { "asyncgettest" };
    StoreId storeId = { "asyncgettest_store" };

    std::shared_ptr<SingleKvStore> store = nullptr;
    auto status = manager.GetSingleKvStore(options, appId, storeId, store);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: GetKvStoreWithDiffDataType
* @tc.desc: get a single KvStore instance 2 times, data type is different.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, GetKvStoreWithDiffDataType, TestSize.Level1)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .dataType = DataType::TYPE_STATICS };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appId = { "asyncgettest" };
    StoreId storeId = { "asyncgettest_store_2" };

    std::shared_ptr<SingleKvStore> store = nullptr;
    auto status = manager.GetSingleKvStore(options, appId, storeId, store);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_NE(store, nullptr);
    status = manager.CloseKvStore(appId, storeId);
    EXPECT_EQ(status, Status::SUCCESS);
    options.dataType = DataType::TYPE_DYNAMICAL;
    status = manager.GetSingleKvStore(options, appId, storeId, store);
    EXPECT_EQ(status, Status::STORE_META_CHANGED);
    status = manager.DeleteKvStore(appId, storeId, options.baseDir);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: AsyncGetValue
* @tc.desc: async get value, data type is TYPE_STATICS.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, AsyncGetValue, TestSize.Level1)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .dataType = DataType::TYPE_STATICS };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appId = { "asyncgettest" };
    StoreId storeId = { "asyncgettest_store_3" };

    std::shared_ptr<SingleKvStore> store = nullptr;
    auto status = manager.GetSingleKvStore(options, appId, storeId, store);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_NE(store, nullptr);
    status = store->Put({ "test_key" }, { "test_value" });
    EXPECT_EQ(status, Status::SUCCESS);
    Value value;
    status = store->Get({ "test_key" }, value);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(value.ToString(), "test_value");
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, Value&&)> call = [blockData](Status status, Value &&value) {
        EXPECT_EQ(status, Status::NOT_SUPPORT);
        blockData->SetValue(true);
    };
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    store->Get({ "test_key" }, devInfo.networkId, call);
    EXPECT_EQ(blockData->GetValue(), true);
    status = manager.CloseKvStore(appId, storeId);
    EXPECT_EQ(status, Status::SUCCESS);
    status = manager.DeleteKvStore(appId, storeId, options.baseDir);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: AsyncGetValueWithInvalidNetworkId
* @tc.desc: async get value, networkId is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, AsyncGetValueWithInvalidNetworkId, TestSize.Level1)
{
    EXPECT_NE(singleKvStore, nullptr);
    auto status = singleKvStore->Put({ "test_key_0" }, { "test_value_0" });
    EXPECT_EQ(status, Status::SUCCESS);
    Value value;
    status = singleKvStore->Get({ "test_key_0" }, value);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(value.ToString(), "test_value_0");
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, Value&&)> call = [blockData](Status status, Value &&value) {
        EXPECT_EQ(status, Status::INVALID_ARGUMENT);
        blockData->SetValue(true);
    };
    singleKvStore->Get({ "test_key_0" }, "", call);
    EXPECT_EQ(blockData->GetValue(), true);
    blockData->Clear(false);
    singleKvStore->Get({ "test_key_0" }, "networkId_test", call);
    EXPECT_EQ(blockData->GetValue(), true);
}

/**
* @tc.name: AsyncGetEntriesWithInvalidNetworkId
* @tc.desc: async get entries, networkId is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, AsyncGetEntriesWithInvalidNetworkId, TestSize.Level1)
{
    EXPECT_NE(singleKvStore, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "prefix_key_" + std::to_string(i);
        entry.value = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto status = singleKvStore->PutBatch(entries);
    EXPECT_EQ(status, Status::SUCCESS);
    std::vector<Entry> results;
    singleKvStore->GetEntries({ "prefix_key_" }, results);
    EXPECT_EQ(results.size(), 10);
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, std::vector<Entry>&&)> call = [blockData](Status status, std::vector<Entry> &&value) {
        EXPECT_EQ(status, Status::INVALID_ARGUMENT);
        blockData->SetValue(true);
    };
    singleKvStore->GetEntries({ "prefix_key_" }, "", call);
    EXPECT_EQ(blockData->GetValue(), true);
    blockData->Clear(false);
    singleKvStore->GetEntries({ "prefix_key_" }, "networkId_test", call);
    EXPECT_EQ(blockData->GetValue(), true);
}

/**
* @tc.name: AsyncGetValueWithLocalNetworkId
* @tc.desc: async get value, networkId is local.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, AsyncGetValueWithLocalNetworkId, TestSize.Level1)
{
    EXPECT_NE(singleKvStore, nullptr);
    auto status = singleKvStore->Put({ "test_key_1" }, { "test_value_1" });
    EXPECT_EQ(status, Status::SUCCESS);
    Value result;
    status = singleKvStore->Get({ "test_key_1" }, result);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(result.ToString(), "test_value_1");
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, Value&&)> call = [blockData, result](Status status, Value &&value) {
        EXPECT_EQ(status, Status::SUCCESS);
        EXPECT_EQ(result.ToString(), value.ToString());
        blockData->SetValue(true);
    };
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    singleKvStore->Get("test_key_1", devInfo.networkId, call);
    EXPECT_EQ(blockData->GetValue(), true);
}

/**
* @tc.name: AsyncGetEntriesWithLocalNetworkId
* @tc.desc: async get entries, networkId is local.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreAsyncGetTest, AsyncGetEntriesWithLocalNetworkId, TestSize.Level1)
{
    EXPECT_NE(singleKvStore, nullptr);
    int num = 5;
    for (int i = 0; i < num; i++) {
        singleKvStore->Put({ "prefix_of_" + std::to_string(i) }, { "test_value_2" });
    }
    std::vector<Entry> results;
    singleKvStore->GetEntries({ "prefix_of_" }, results);
    EXPECT_EQ(results.size(), num);
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, std::vector<Entry>&&)> call =
        [blockData, results](Status status, std::vector<Entry>&& values) {
            EXPECT_EQ(status, Status::SUCCESS);
            EXPECT_EQ(results.size(), values.size());
            EXPECT_EQ(values[0].value.ToString(), "test_value_2");
            blockData->SetValue(true);
    };
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    singleKvStore->GetEntries("prefix_of_", devInfo.networkId, call);
    EXPECT_EQ(blockData->GetValue(), true);
}
} // namespace OHOS::Test