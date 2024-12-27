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
#define LOG_TAG "SingleKvStoreVirtualTest"

#include "block_data.h"
#include "dev_manager.h"
#include "distributed_kv_data_manager.h"
#include "file_ex.h"
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <unistd.h>
#include <vector>

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class SingleKvStoreAsyncVirtualTest : public testing::Test {
public:
    static std::shared_ptr<SingleKvStore> singleKvStoreVirtual;
    static Status statusVirtualVirtual_;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

std::shared_ptr<SingleKvStore> SingleKvStoreAsyncVirtualTest::singleKvStoreVirtual = nullptr;
Status SingleKvStoreAsyncVirtualTest::statusVirtualVirtual_ = Status::ERROR;

void SingleKvStoreAsyncVirtualTest::SetUpTestCase(void)
{
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .encrypt = false, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .dataType = DataType::TYPE_DYNAMICAL };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appIdVirtual = { "asyncgettest" };
    StoreId storeIdVirtual = { "asyncgettest_store_0" };
    mkdir(optionsVirtual.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    statusVirtualVirtual_ =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, singleKvStoreVirtual);
}

void SingleKvStoreAsyncVirtualTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/asyncgettest/key");
    (void)remove("/data/service/el1/public/database/asyncgettest/kvdb");
    (void)remove("/data/service/el1/public/database/asyncgettest");
}

void SingleKvStoreAsyncVirtualTest::SetUp(void)
{}

void SingleKvStoreAsyncVirtualTest::TearDown(void)
{}

class SingleKvStoreQueryVirtualTest : public testing::Test {
public:
    static std::shared_ptr<SingleKvStore> singleKvStoreVirtual;
    static Status statusVirtualGetKvStoreVirtual;
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
};


static constexpr const char *VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
                                                          "\"SCHEMA_MODE\":\"STRICT\","
                                                          "\"SCHEMA_SKIPSIZE\":0,"
                                                          "\"SCHEMA_DEFINE\":{"
                                                              "\"name\":\"INTEGER, NOT NULL\""
                                                          "},"
                                                          "\"SCHEMA_INDEXES\":[\"$.name\"]}";
std::shared_ptr<SingleKvStore> SingleKvStoreQueryVirtualTest::singleKvStoreVirtual = nullptr;
Status SingleKvStoreQueryVirtualTest::statusVirtualGetKvStoreVirtual = Status::ERROR;
static constexpr int32_t INVALID_NUMBER = -1;
static constexpr uint32_t MAX_QUERY_LENGTH = 1024;

void SingleKvStoreQueryVirtualTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/SingleKvStoreQueryVirtualTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SingleKvStoreQueryVirtualTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/SingleKvStoreQueryVirtualTest/key");
    (void)remove("/data/service/el1/public/database/SingleKvStoreQueryVirtualTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleKvStoreQueryVirtualTest");
}

void SingleKvStoreQueryVirtualTest::SetUp(void)
{}

void SingleKvStoreQueryVirtualTest::TearDown(void)
{}

/**
* @tc.name: CreateDefaultKvStore
* @tc.desc: get a single KvStore instance, default is dynamic.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, CreateDefaultKvStore, TestSize.Level0)
{
    ZLOGI("CreateDefaultKvStore begin.");
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .kvStoreType = KvStoreType::SINGLE_VERSION};
    ASSERT_EQ(optionsVirtual.autoSync, false);
    ASSERT_EQ(optionsVirtual.dataType, DataType::TYPE_DYNAMICAL);
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appIdVirtual = { "asyncgettest" };
    StoreId storeIdVirtual = { "asyncgettest_store_1" };

    std::shared_ptr<SingleKvStore> storeVirtual = nullptr;
    auto statusVirtualVirtual =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, storeVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    ASSERT_NE(storeVirtual, nullptr);
    statusVirtualVirtual = managerVirtual.CloseKvStore(appIdVirtual, storeIdVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    statusVirtualVirtual =
        managerVirtual.DeleteKvStore(appIdVirtual, storeIdVirtual, optionsVirtual.baseDir);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
}

/**
* @tc.name: CreateStaticKvStore
* @tc.desc: get a single KvStore instance, data type is STATICS.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, CreateStaticKvStore, TestSize.Level0)
{
    ZLOGI("CreateStaticKvStore begin.");
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .autoSync = true, .dataType = DataType::TYPE_STATICS };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appIdVirtual = { "asyncgettest" };
    StoreId storeIdVirtual = { "asyncgettest_store" };

    std::shared_ptr<SingleKvStore> storeVirtual = nullptr;
    auto statusVirtualVirtual =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, storeVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: GetKvStoreWithDiffDataType
* @tc.desc: get a single KvStore instance 2 times, data type is different.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, GetKvStoreWithDiffDataType, TestSize.Level0)
{
    ZLOGI("GetKvStoreWithDiffDataType begin.");
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .dataType = DataType::TYPE_STATICS };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appIdVirtual = { "asyncgettest" };
    StoreId storeIdVirtual = { "asyncgettest_store_2" };

    std::shared_ptr<SingleKvStore> storeVirtual = nullptr;
    auto statusVirtualVirtual =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, storeVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    ASSERT_NE(storeVirtual, nullptr);
    statusVirtualVirtual = managerVirtual.CloseKvStore(appIdVirtual, storeIdVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    optionsVirtual.dataType = DataType::TYPE_DYNAMICAL;
    statusVirtualVirtual =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, storeVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    statusVirtualVirtual =
        managerVirtual.DeleteKvStore(appIdVirtual, storeIdVirtual, optionsVirtual.baseDir);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
}

/**
* @tc.name: AsyncGetValue
* @tc.desc: async get value, data type is TYPE_STATICS.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, AsyncGetValue, TestSize.Level0)
{
    ZLOGI("AsyncGetValue begin.");
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .dataType = DataType::TYPE_STATICS };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = std::string("/data/service/el1/public/database/asyncgettest");
    AppId appIdVirtual = { "asyncgettest" };
    StoreId storeIdVirtual = { "asyncgettest_store_3" };

    std::shared_ptr<SingleKvStore> storeVirtual = nullptr;
    auto statusVirtualVirtual =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, storeVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    ASSERT_NE(storeVirtual, nullptr);
    statusVirtualVirtual = storeVirtual->Put({ "test_key" }, { "test_value" });
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    Value value;
    statusVirtualVirtual = storeVirtual->Get({ "test_key" }, value);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    ASSERT_EQ(value.ToString(), "test_value");
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, Value&&)> call = [blockData, value](Status statusVirtualVirtual, Value &&out) {
        ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
        ASSERT_EQ(out.ToString(), value.ToString());
        blockData->SetValue(true);
    };
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    storeVirtual->Get({ "test_key" }, devInfo.networkId, call);
    ASSERT_EQ(blockData->GetValue(), true);
    statusVirtualVirtual = managerVirtual.CloseKvStore(appIdVirtual, storeIdVirtual);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    statusVirtualVirtual =
        managerVirtual.DeleteKvStore(appIdVirtual, storeIdVirtual, optionsVirtual.baseDir);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
}

/**
* @tc.name: AsyncGetValueWithInvalidNetworkId
* @tc.desc: async get value, networkId is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, AsyncGetValueWithInvalidNetworkId, TestSize.Level0)
{
    ZLOGI("AsyncGetValueWithInvalidNetworkId begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr);
    auto statusVirtualVirtual = singleKvStoreVirtual->Put({ "test_key_0" }, { "test_value_0" });
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    Value value;
    statusVirtualVirtual = singleKvStoreVirtual->Get({ "test_key_0" }, value);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    ASSERT_EQ(value.ToString(), "test_value_0");
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, Value&&)> call = [blockData](Status statusVirtualVirtual, Value &&value) {
        ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
        blockData->SetValue(true);
    };
    singleKvStoreVirtual->Get({ "test_key_0" }, "", call);
    ASSERT_EQ(blockData->GetValue(), true);
    blockData->Clear(false);
    singleKvStoreVirtual->Get({ "test_key_0" }, "networkId_test", call);
    ASSERT_EQ(blockData->GetValue(), true);
}

/**
* @tc.name: AsyncGetEntriesWithInvalidNetworkId
* @tc.desc: async get entries, networkId is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, AsyncGetEntriesWithInvalidNetworkId, TestSize.Level0)
{
    ZLOGI("AsyncGetEntriesWithInvalidNetworkId begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = "prefix_key_" + std::to_string(i);
        entry.value = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto statusVirtualVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    std::vector<Entry> resultsVirtual;
    singleKvStoreVirtual->GetEntries({ "prefix_key_" }, resultsVirtual);
    ASSERT_EQ(resultsVirtual.size(), 10);
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, std::vector<Entry>&&)> call =
        [blockData](Status statusVirtualVirtual, std::vector<Entry> &&value) {
            ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
            blockData->SetValue(true);
        };
    singleKvStoreVirtual->GetEntries({ "prefix_key_" }, "", call);
    ASSERT_EQ(blockData->GetValue(), true);
    blockData->Clear(false);
    singleKvStoreVirtual->GetEntries({ "prefix_key_" }, "networkId_test", call);
    ASSERT_EQ(blockData->GetValue(), true);
}

/**
* @tc.name: AsyncGetValueWithLocalNetworkId
* @tc.desc: async get value, networkId is local.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, AsyncGetValueWithLocalNetworkId, TestSize.Level0)
{
    ZLOGI("AsyncGetValueWithLocalNetworkId begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr);
    auto statusVirtualVirtual = singleKvStoreVirtual->Put({ "test_key_1" }, { "test_value_1" });
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    Value result;
    statusVirtualVirtual = singleKvStoreVirtual->Get({ "test_key_1" }, result);
    ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
    ASSERT_EQ(result.ToString(), "test_value_1");
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, Value&&)> call = [blockData, result](Status statusVirtualVirtual, Value &&value) {
        ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
        ASSERT_EQ(result.ToString(), value.ToString());
        blockData->SetValue(true);
    };
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    singleKvStoreVirtual->Get("test_key_1", devInfo.networkId, call);
    ASSERT_EQ(blockData->GetValue(), true);
}

/**
* @tc.name: AsyncGetEntriesWithLocalNetworkId
* @tc.desc: async get entries, networkId is local.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreAsyncVirtualTest, AsyncGetEntriesWithLocalNetworkId, TestSize.Level0)
{
    ZLOGI("AsyncGetEntriesWithLocalNetworkId begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr);
    int num = 5;
    for (int i = 0; i < num; i++) {
        singleKvStoreVirtual->Put({ "prefix_of_" + std::to_string(i) }, { "test_value_2" });
    }
    std::vector<Entry> resultsVirtual;
    singleKvStoreVirtual->GetEntries({ "prefix_of_" }, resultsVirtual);
    ASSERT_EQ(resultsVirtual.size(), num);
    auto blockData = std::make_shared<BlockData<bool>>(1, false);
    std::function<void(Status, std::vector<Entry>&&)> call =
        [blockData, resultsVirtual](Status statusVirtualVirtual, std::vector<Entry>&& values) {
            ASSERT_EQ(statusVirtualVirtual, Status::SUCCESS);
            ASSERT_EQ(resultsVirtual.size(), values.size());
            ASSERT_EQ(values[0].value.ToString(), "test_value_2");
            blockData->SetValue(true);
    };
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    singleKvStoreVirtual->GetEntries("prefix_of_", devInfo.networkId, call);
    ASSERT_EQ(blockData->GetValue(), true);
    auto ret = singleKvStoreVirtual->GetDeviceEntries("test_device_1", resultsVirtual);
    ASSERT_EQ(ret, Status::SUCCESS);
}

/**
* @tc.name: TestQueryReset
* @tc.desc: the predicate is reset
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, TestQueryReset, TestSize.Level0)
{
    ZLOGI("TestQueryReset begin.");
    DataQuery queryVirtual;
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    std::string strVirtual = "test value";
    queryVirtual.EqualTo("$.test_field_name", strVirtual);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryEqualToInvalidField
* @tc.desc: the predicate is equalTo, the field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryEqualToInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryEqualToInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.EqualTo("", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("$.test_field_name^", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("", (int64_t)100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("^", (int64_t)100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("$.^", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("", false);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("^$.test_field_name", false);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("", std::string("strVirtual"));
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.EqualTo("^^^^^^^", std::string("strVirtual"));
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryEqualToValidField
* @tc.desc: the predicate is equalTo, the field is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryEqualToValidField, TestSize.Level0)
{
    ZLOGI("DataQueryEqualToValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.EqualTo("$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.EqualTo("$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.EqualTo("$.test_field_name", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.EqualTo("$.test_field_name", false);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::string strVirtual = "";
    queryVirtual.EqualTo("$.test_field_name", strVirtual);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryNotEqualToValidField
* @tc.desc: the predicate is notEqualTo, the field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryNotEqualToValidField, TestSize.Level0)
{
    ZLOGI("DataQueryNotEqualToValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.NotEqualTo("", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("$.test_field_name^test", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("", (int64_t)100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("^$.test_field_name", (int64_t)100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("^", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("", false);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("^^", false);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("", std::string("test_value"));
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotEqualTo("$.test_field^_name", std::string("test_value"));
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryNotEqualToInvalidField
* @tc.desc: the predicate is notEqualTo, the field is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryNotEqualToInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryNotEqualToInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.NotEqualTo("$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.NotEqualTo("$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.NotEqualTo("$.test_field_name", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.NotEqualTo("$.test_field_name", false);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::string strVirtual = "test value";
    queryVirtual.NotEqualTo("$.test_field_name", strVirtual);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryGreaterThanInvalidField
* @tc.desc: the predicate is greaterThan, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryGreaterThanInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryGreaterThanInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.GreaterThan("", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThan("$.^^", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThan("", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThan("^$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThan("", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThan("^", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThan("", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThan("$.test_field_name^*%$#", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryGreaterThanValidField
* @tc.desc: the predicate is greaterThan, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryGreaterThanValidField, TestSize.Level0)
{
    ZLOGI("DataQueryGreaterThanValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.GreaterThan("$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.GreaterThan("$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.GreaterThan("$.test_field_name", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.GreaterThan("$.test_field_name$$$", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryLessThanInvalidField
* @tc.desc: the predicate is lessThan, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLessThanInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryLessThanInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.LessThan("", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThan("$.^", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThan("", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThan("^$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThan("", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThan("^^^", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThan("", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThan("$.test_field_name^", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryLessThanValidField
* @tc.desc: the predicate is lessThan, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLessThanValidField, TestSize.Level0)
{
    ZLOGI("DataQueryLessThanValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.LessThan("$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.LessThan("$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.LessThan("$.test_field_name", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.LessThan("$.test_field_name", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryGreaterThanOrEqualToInvalidField
* @tc.desc: the predicate is greaterThanOrEqualTo, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryGreaterThanOrEqualToInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryGreaterThanOrEqualToInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.GreaterThanOrEqualTo("", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThanOrEqualTo("^$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThanOrEqualTo("", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThanOrEqualTo("$.test_field_name^", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThanOrEqualTo("", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThanOrEqualTo("^$.^", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThanOrEqualTo("", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.GreaterThanOrEqualTo("^^=", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryGreaterThanOrEqualToValidField
* @tc.desc: the predicate is greaterThanOrEqualTo, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryGreaterThanOrEqualToValidField, TestSize.Level0)
{
    ZLOGI("DataQueryGreaterThanOrEqualToValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.GreaterThanOrEqualTo("$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.GreaterThanOrEqualTo("$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.GreaterThanOrEqualTo("$.test_field_name", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.GreaterThanOrEqualTo("$.test_field_name", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryLessThanOrEqualToInvalidField
* @tc.desc: the predicate is lessThanOrEqualTo, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLessThanOrEqualToInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryLessThanOrEqualToInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.LessThanOrEqualTo("", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThanOrEqualTo("^$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThanOrEqualTo("", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThanOrEqualTo("$.test_field_name^", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThanOrEqualTo("", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThanOrEqualTo("^", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThanOrEqualTo("", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.LessThanOrEqualTo("678678^", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryLessThanOrEqualToValidField
* @tc.desc: the predicate is lessThanOrEqualTo, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLessThanOrEqualToValidField, TestSize.Level0)
{
    ZLOGI("DataQueryLessThanOrEqualToValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.LessThanOrEqualTo("$.test_field_name", 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.LessThanOrEqualTo("$.test_field_name", (int64_t) 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.LessThanOrEqualTo("$.test_field_name", 1.23);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.LessThanOrEqualTo("$.test_field_name", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryIsNullInvalidField
* @tc.desc: the predicate is isNull, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryIsNullInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryIsNullInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.IsNull("");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.IsNull("$.test^_field_name");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryIsNullValidField
* @tc.desc: the predicate is isNull, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryIsNullValidField, TestSize.Level0)
{
    ZLOGI("DataQueryIsNullValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.IsNull("$.test_field_name");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryInInvalidField
* @tc.desc: the predicate is in, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryInInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryInInvalidField begin.");
    DataQuery queryVirtual;
    std::vector<int> vectInt{ 10, 20, 30 };
    queryVirtual.In("", vectInt);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.In("^", vectInt);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    queryVirtual.In("", vectLong);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.In("$.test_field_name^", vectLong);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    std::vector<double> vectDouble{1.23, 2.23, 3.23};
    queryVirtual.In("", vectDouble);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.In("$.^test_field_name", vectDouble);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    queryVirtual.In("", vectString);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.In("$.test_field_^name^", vectString);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryInValidField
* @tc.desc: the predicate is in, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryInValidField, TestSize.Level0)
{
    ZLOGI("DataQueryInValidField begin.");
    DataQuery queryVirtual;
    std::vector<int> vectInt{ 10, 20, 30 };
    queryVirtual.In("$.test_field_name", vectInt);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    queryVirtual.In("$.test_field_name", vectLong);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::vector<double> vectDouble{1.23, 2.23, 3.23};
    queryVirtual.In("$.test_field_name", vectDouble);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    queryVirtual.In("$.test_field_name", vectString);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryNotInInvalidField
* @tc.desc: the predicate is notIn, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryNotInInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryNotInInvalidField begin.");
    DataQuery queryVirtual;
    std::vector<int> vectInt{ 10, 20, 30 };
    queryVirtual.NotIn("", vectInt);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotIn("$.^", vectInt);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    queryVirtual.NotIn("", vectLong);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotIn("^^", vectLong);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    std::vector<double> vectDouble{ 1.23, 2.23, 3.23 };
    queryVirtual.NotIn("", vectDouble);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotIn("^$.test_field_name", vectDouble);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    queryVirtual.NotIn("", vectString);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.NotIn("$.^", vectString);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryNotInValidField
* @tc.desc: the predicate is notIn, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryNotInValidField, TestSize.Level0)
{
    ZLOGI("DataQueryNotInValidField begin.");
    DataQuery queryVirtual;
    std::vector<int> vectInt{ 10, 20, 30 };
    queryVirtual.NotIn("$.test_field_name", vectInt);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    queryVirtual.NotIn("$.test_field_name", vectLong);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::vector<double> vectDouble{ 1.23, 2.23, 3.23 };
    queryVirtual.NotIn("$.test_field_name", vectDouble);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    queryVirtual.NotIn("$.test_field_name", vectString);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryLikeInvalidField
* @tc.desc: the predicate is like, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLikeInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryLikeInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.Like("", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.Like("$.test_fi^eld_name", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryLikeValidField
* @tc.desc: the predicate is like, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLikeValidField, TestSize.Level0)
{
    ZLOGI("DataQueryLikeValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.Like("$.test_field_name", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryUnlikeInvalidField
* @tc.desc: the predicate is unlike, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryUnlikeInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryUnlikeInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.Unlike("", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.Unlike("$.^", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryUnlikeValidField
* @tc.desc: the predicate is unlike, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryUnlikeValidField, TestSize.Level0)
{
    ZLOGI("DataQueryUnlikeValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.Unlike("$.test_field_name", "test value");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryAnd
* @tc.desc: the predicate is and
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryAnd, TestSize.Level0)
{
    ZLOGI("DataQueryAnd begin.");
    DataQuery queryVirtual;
    queryVirtual.Like("$.test_field_name1", "test value1");
    queryVirtual.And();
    queryVirtual.Like("$.test_field_name2", "test value2");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryOr
* @tc.desc: the predicate is or
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryOr, TestSize.Level0)
{
    ZLOGI("DataQueryOr begin.");
    DataQuery queryVirtual;
    queryVirtual.Like("$.test_field_name1", "test value1");
    queryVirtual.Or();
    queryVirtual.Like("$.test_field_name2", "test value2");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryOrderByAscInvalidField
* @tc.desc: the predicate is orderByAsc, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryOrderByAscInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryOrderByAscInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.OrderByAsc("");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.OrderByAsc("$.^");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryOrderByAscValidField
* @tc.desc: the predicate is orderByAsc, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryOrderByAscValidField, TestSize.Level0)
{
    ZLOGI("DataQueryOrderByAscValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.OrderByAsc("$.test_field_name1");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryOrderByDescInvalidField
* @tc.desc: the predicate is orderByDesc, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryOrderByDescInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryOrderByDescInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.OrderByDesc("");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.OrderByDesc("$.test^_field_name1");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryOrderByDescValidField
* @tc.desc: the predicate is orderByDesc, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryOrderByDescValidField, TestSize.Level0)
{
    ZLOGI("DataQueryOrderByDescValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.OrderByDesc("$.test_field_name1");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryLimitInvalidField
* @tc.desc: the predicate is limit, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLimitInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryLimitInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.Limit(INVALID_NUMBER, 100);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.Limit(10, INVALID_NUMBER);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryLimitValidField
* @tc.desc: the predicate is limit, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryLimitValidField, TestSize.Level0)
{
    ZLOGI("DataQueryLimitValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.Limit(10, 100);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: SingleKvStoreQueryNotEqualTo
* @tc.desc: queryVirtual single kvStore by dataQuery, the predicate is notEqualTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, SingleKvStoreQueryNotEqualTo, TestSize.Level0)
{
    ZLOGI("SingleKvStoreQueryNotEqualTo begin.");
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .encrypt = true, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema =  VALID_SCHEMA_STRICT_DEFINE };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = "/data/service/el1/public/database/SingleKvStoreQueryVirtualTest";
    AppId appIdVirtual = { "SingleKvStoreQueryVirtualTest" };
    StoreId storeIdVirtual = { "SingleKvStoreClientQueryTestStoreId1" };
    statusVirtualGetKvStore =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, singleKvStoreVirtual);
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    singleKvStoreVirtual->Put("test_key_1", "{\"name\":1}");
    singleKvStoreVirtual->Put("test_key_2", "{\"name\":2}");
    singleKvStoreVirtual->Put("test_key_3", "{\"name\":3}");

    DataQuery queryVirtual;
    queryVirtual.NotEqualTo("$.name", 3);
    std::vector<Entry> results;
    Status statusVirtual1 = singleKvStoreVirtual->GetEntries(queryVirtual, results);
    ASSERT_EQ(statusVirtual1, Status::SUCCESS);
    ASSERT_TRUE(results.size() == 2);
    results.clear();
    Status statusVirtual2 = singleKvStoreVirtual->GetEntries(queryVirtual, results);
    ASSERT_EQ(statusVirtual2, Status::SUCCESS);
    ASSERT_TRUE(results.size() == 2);

    std::shared_ptr<KvStoreResultSet> resultSet;
    Status statusVirtual3 = singleKvStoreVirtual->GetResultSet(queryVirtual, resultSet);
    ASSERT_EQ(statusVirtual3, Status::SUCCESS);
    ASSERT_TRUE(resultSet->GetCount() == 2);
    auto closeResultSetStatus = singleKvStoreVirtual->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);
    Status statusVirtual4 = singleKvStoreVirtual->GetResultSet(queryVirtual, resultSet);
    ASSERT_EQ(statusVirtual4, Status::SUCCESS);
    ASSERT_TRUE(resultSet->GetCount() == 2);

    closeResultSetStatus = singleKvStoreVirtual->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);

    int resultSize1;
    Status statusVirtual5 = singleKvStoreVirtual->GetCount(queryVirtual, resultSize1);
    ASSERT_EQ(statusVirtual5, Status::SUCCESS);
    ASSERT_TRUE(resultSize1 == 2);
    int resultSize2;
    Status statusVirtual6 = singleKvStoreVirtual->GetCount(queryVirtual, resultSize2);
    ASSERT_EQ(statusVirtual6, Status::SUCCESS);
    ASSERT_TRUE(resultSize2 == 2);

    singleKvStoreVirtual->Delete("test_key_1");
    singleKvStoreVirtual->Delete("test_key_2");
    singleKvStoreVirtual->Delete("test_key_3");
    Status statusVirtual = managerVirtual.CloseAllKvStore(appIdVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    statusVirtual = managerVirtual.DeleteAllKvStore(appIdVirtual, optionsVirtual.baseDir);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
}

/**
* @tc.name: SingleKvStoreQueryNotEqualToAndEqualTo
* @tc.desc: queryVirtual single kvStore by dataQuery, the predicate is notEqualTo and equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, SingleKvStoreQueryNotEqualToAndEqualTo, TestSize.Level0)
{
    ZLOGI("SingleKvStoreQueryNotEqualToAndEqualTo begin.");
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .encrypt = true, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema = VALID_SCHEMA_STRICT_DEFINE };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = "/data/service/el1/public/database/SingleKvStoreQueryVirtualTest";
    AppId appIdVirtual = { "SingleKvStoreQueryVirtualTest" };
    StoreId storeIdVirtual = { "SingleKvStoreClientQueryTestStoreId2" };
    statusVirtualGetKvStore =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, singleKvStoreVirtual);
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    singleKvStoreVirtual->Put("test_key_1", "{\"name\":1}");
    singleKvStoreVirtual->Put("test_key_2", "{\"name\":2}");
    singleKvStoreVirtual->Put("test_key_3", "{\"name\":3}");

    DataQuery queryVirtual;
    queryVirtual.NotEqualTo("$.name", 3);
    queryVirtual.And();
    queryVirtual.EqualTo("$.name", 1);
    std::vector<Entry> results1;
    Status statusVirtual1 = singleKvStoreVirtual->GetEntries(queryVirtual, results1);
    ASSERT_EQ(statusVirtual1, Status::SUCCESS);
    ASSERT_TRUE(results1.size() == 1);
    std::vector<Entry> results2;
    Status statusVirtual2 = singleKvStoreVirtual->GetEntries(queryVirtual, results2);
    ASSERT_EQ(statusVirtual2, Status::SUCCESS);
    ASSERT_TRUE(results2.size() == 1);

    std::shared_ptr<KvStoreResultSet> resultSet;
    Status statusVirtual3 = singleKvStoreVirtual->GetResultSet(queryVirtual, resultSet);
    ASSERT_EQ(statusVirtual3, Status::SUCCESS);
    ASSERT_TRUE(resultSet->GetCount() == 1);
    auto closeResultSetStatus = singleKvStoreVirtual->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);
    Status statusVirtual4 = singleKvStoreVirtual->GetResultSet(queryVirtual, resultSet);
    ASSERT_EQ(statusVirtual4, Status::SUCCESS);
    ASSERT_TRUE(resultSet->GetCount() == 1);

    closeResultSetStatus = singleKvStoreVirtual->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);

    int resultSize1;
    Status statusVirtual5 = singleKvStoreVirtual->GetCount(queryVirtual, resultSize1);
    ASSERT_EQ(statusVirtual5, Status::SUCCESS);
    ASSERT_TRUE(resultSize1 == 1);
    int resultSize2;
    Status statusVirtual6 = singleKvStoreVirtual->GetCount(queryVirtual, resultSize2);
    ASSERT_EQ(statusVirtual6, Status::SUCCESS);
    ASSERT_TRUE(resultSize2 == 1);

    singleKvStoreVirtual->Delete("test_key_1");
    singleKvStoreVirtual->Delete("test_key_2");
    singleKvStoreVirtual->Delete("test_key_3");
    Status statusVirtual = managerVirtual.CloseAllKvStore(appIdVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    statusVirtual = managerVirtual.DeleteAllKvStore(appIdVirtual, optionsVirtual.baseDir);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
}

/**
* @tc.name: DataQueryGroupAbnormal
* @tc.desc: queryVirtual group, the predicate is prefix, isNotNull, but field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryGroupAbnormal, TestSize.Level0)
{
    ZLOGI("DataQueryGroupAbnormal begin.");
    DataQuery queryVirtual;
    queryVirtual.KeyPrefix("");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.KeyPrefix("prefix^");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.Reset();
    queryVirtual.BeginGroup();
    queryVirtual.IsNotNull("");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.IsNotNull("^$.name");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.EndGroup();
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryByGroupNormal
* @tc.desc: queryVirtual group, the predicate is prefix, isNotNull.
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryByGroupNormal, TestSize.Level0)
{
    ZLOGI("DataQueryByGroupNormal begin.");
    DataQuery queryVirtual;
    queryVirtual.KeyPrefix("prefix");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    queryVirtual.BeginGroup();
    queryVirtual.IsNotNull("$.name");
    queryVirtual.EndGroup();
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQuerySetSuggestIndexInvalidField
* @tc.desc: the predicate is setSuggestIndex, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: liuwenhui
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQuerySetSuggestIndexInvalidField, TestSize.Level0)
{
    ZLOGI("DataQuerySetSuggestIndexInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.SetSuggestIndex("");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.SetSuggestIndex("test_field^_name");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQuerySetSuggestIndexValidField
* @tc.desc: the predicate is setSuggestIndex, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: liuwenhui
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQuerySetSuggestIndexValidField, TestSize.Level0)
{
    ZLOGI("DataQuerySetSuggestIndexValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.SetSuggestIndex("test_field_name");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQuerySetInKeys
* @tc.desc: the predicate is inKeys
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQuerySetInKeys, TestSize.Level0)
{
    ZLOGI("DataQuerySetInKeys begin.");
    DataQuery queryVirtual;
    queryVirtual.InKeys({});
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.InKeys({"test_field_name"});
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.InKeys({"test_field_name_hasKey"});
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::vector<std::string> keys { "test_field", "", "^test_field", "^", "test_field_name" };
    queryVirtual.InKeys(keys);
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
}

/**
* @tc.name: DataQueryDeviceIdInvalidField
* @tc.desc:the predicate is deviceId, the field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryDeviceIdInvalidField, TestSize.Level0)
{
    ZLOGI("DataQueryDeviceIdInvalidField begin.");
    DataQuery queryVirtual;
    queryVirtual.DeviceId("");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.DeviceId("$$^");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
    queryVirtual.DeviceId("device_id^");
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryDeviceIdValidField
* @tc.desc: the predicate is valid deviceId, the field is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryDeviceIdValidField, TestSize.Level0)
{
    ZLOGI("DataQueryDeviceIdValidField begin.");
    DataQuery queryVirtual;
    queryVirtual.DeviceId("device_id");
    ASSERT_TRUE(queryVirtual.ToString().length() > 0);
    queryVirtual.Reset();
    std::string deviceId = "";
    uint32_t i = 0;
    while (i < MAX_QUERY_LENGTH) {
        deviceId += "device";
        i++;
    }
    queryVirtual.DeviceId(deviceId);
    ASSERT_TRUE(queryVirtual.ToString().length() == 0);
}

/**
* @tc.name: DataQueryBetweenInvalid
* @tc.desc: the predicate is between, the value is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(SingleKvStoreQueryVirtualTest, DataQueryBetweenInvalid, TestSize.Level0)
{
    ZLOGI("DataQueryBetweenInvalid begin.");
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = { .createIfMissing = true, .encrypt = true, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema =  VALID_SCHEMA_STRICT_DEFINE };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = "/data/service/el1/public/database/SingleKvStoreQueryVirtualTest";
    AppId appIdVirtual = { "SingleKvStoreQueryVirtualTest" };
    StoreId storeIdVirtual = { "SingleKvStoreClientQueryTestStoreId3" };
    statusVirtualGetKvStore =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, singleKvStoreVirtual);
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    singleKvStoreVirtual->Put("test_key_1", "{\"name\":1}");
    singleKvStoreVirtual->Put("test_key_2", "{\"name\":2}");
    singleKvStoreVirtual->Put("test_key_3", "{\"name\":3}");

    DataQuery queryVirtual;
    queryVirtual.Between({}, {});
    std::vector<Entry> results1;
    Status statusVirtual = singleKvStoreVirtual->GetEntries(queryVirtual, results1);
    ASSERT_EQ(statusVirtual, NOT_SUPPORT);

    singleKvStoreVirtual->Delete("test_key_1");
    singleKvStoreVirtual->Delete("test_key_2");
    singleKvStoreVirtual->Delete("test_key_3");
    statusVirtual = managerVirtual.CloseAllKvStore(appIdVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    statusVirtual = managerVirtual.DeleteAllKvStore(appIdVirtual, optionsVirtual.baseDir);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
}
} // namespace OHOS::Test