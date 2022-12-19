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

#include "distributed_kv_data_manager.h"
#include "gtest/gtest.h"
#include "kv_utils.h"
#include "kvstore_datashare_bridge.h"
#include "store_errno.h"
#include "result_set_bridge.h"
#include "kvstore_result_set.h"
#include "types.h"

namespace {
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
class BridgeWriter final : public ResultSetBridge::Writer {
public:
    int AllocRow() override;
    int Write(uint32_t column) override;
    int Write(uint32_t column, int64_t value) override;
    int Write(uint32_t column, double value) override;
    int Write(uint32_t column, const uint8_t *value, size_t size) override;
    int Write(uint32_t column, const char *value, size_t size) override;
    void SetAllocRowStatue(int status);
    Key GetKey() const;
    Key GetValue() const;

private:
    int allocStatus_ = E_OK;
    std::vector<uint8_t> key_;
    std::vector<uint8_t> value_;
};

void BridgeWriter::SetAllocRowStatue(int status)
{
    allocStatus_ = status;
}

Key BridgeWriter::GetKey() const
{
    return key_;
}

Value BridgeWriter::GetValue() const
{
    return value_;
}

int BridgeWriter::AllocRow()
{
    return allocStatus_;
}

int BridgeWriter::Write(uint32_t column)
{
    return E_OK;
}

int BridgeWriter::Write(uint32_t column, int64_t value)
{
    return E_OK;
}

int BridgeWriter::Write(uint32_t column, double value)
{
    return E_OK;
}

int BridgeWriter::Write(uint32_t column, const uint8_t *value, size_t size)
{
    return E_OK;
}

int BridgeWriter::Write(uint32_t column, const char *value, size_t size)
{
    if (column < 0 || column > 1 || value == nullptr) {
        return E_ERROR;
    }
    auto vec = std::vector<uint8_t>(value, value + size - 1);
    if (column == 0) {
        key_.insert(key_.end(), vec.begin(), vec.end());
    } else {
        value_.insert(value_.end(), vec.begin(), vec.end());
    }
    return E_OK;
}

class KvstoreDatashareBridgeTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}

protected:
    static DistributedKvDataManager manager;
    static std::shared_ptr<SingleKvStore> singleKvStore;
};
std::shared_ptr<SingleKvStore> KvstoreDatashareBridgeTest::singleKvStore = nullptr;
DistributedKvDataManager KvstoreDatashareBridgeTest::manager;
static constexpr int32_t INVALID_COUNT = -1;
static constexpr const char *VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
                                                           "\"SCHEMA_MODE\":\"STRICT\","
                                                           "\"SCHEMA_SKIPSIZE\":0,"
                                                           "\"SCHEMA_DEFINE\":{"
                                                           "\"age\":\"INTEGER, NOT NULL\""
                                                           "},"
                                                           "\"SCHEMA_INDEXES\":[\"$.age\"]}";

void KvstoreDatashareBridgeTest::SetUpTestCase(void)
{
    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = false,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema =  VALID_SCHEMA_STRICT_DEFINE };
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/KvstoreDatashareBridgeTest");
    AppId appId = { "KvstoreDatashareBridgeTest" };
    StoreId storeId = { "test_single" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    manager.DeleteKvStore(appId, storeId, options.baseDir);
    manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
    EXPECT_NE(singleKvStore, nullptr);
    singleKvStore->Put("test_key_1", "{\"age\":1}");
    singleKvStore->Put("test_key_2", "{\"age\":2}");
    singleKvStore->Put("test_key_3", "{\"age\":3}");
    singleKvStore->Put("data_share", "{\"age\":4}");
}

void KvstoreDatashareBridgeTest::TearDownTestCase(void)
{
    manager.DeleteKvStore(
            {"KvstoreDatashareBridgeTest"}, {"test_single"},
            "/data/service/el1/public/database/KvstoreDatashareBridgeTest");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeTest/key");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeTest/kvdb");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeTest");
}

/**
* @tc.name:ToDataShareResult
* @tc.desc: get row count, the kvStore resultSet is nullptr
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvstoreDatashareBridgeTest, GetRowCountByInvalidBridge, TestSize.Level0)
{
    auto bridge = std::make_shared<KvStoreDataShareBridge>(nullptr);
    int32_t count;
    auto result = bridge->GetRowCount(count);
    EXPECT_EQ(result, E_ERROR);
    EXPECT_EQ(count, INVALID_COUNT);
    std::vector<std::string> columnNames;
    result = bridge->GetAllColumnNames(columnNames);
    EXPECT_FALSE(columnNames.empty());
    EXPECT_EQ(result, E_OK);
}

/**
* @tc.name:ToDataShareResultSet
* @tc.desc: kvStore resultSet to dataShare resultSet, the former has invalid predicate
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvstoreDatashareBridgeTest, KvStoreResultSetToDataShareResultSetAbnormal, TestSize.Level0)
{
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    DataQuery query;
    query.KeyPrefix("key");
    singleKvStore->GetResultSet(query, resultSet);
    EXPECT_NE(resultSet, nullptr);
    auto bridge = KvUtils::ToResultSetBridge(resultSet);
    int32_t count;
    auto result = bridge->GetRowCount(count);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(count, 0);
}

/**
* @tc.name:ToDataShareResultSet
* @tc.desc: kvStore resultSet to dataShare resultSet, the former has valid predicate
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvstoreDatashareBridgeTest, KvStoreResultSetToDataShareResultSetNormal, TestSize.Level0)
{
    DataQuery query;
    query.KeyPrefix("test");
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    singleKvStore->GetResultSet(query, resultSet);
    EXPECT_NE(resultSet, nullptr);
    auto bridge = KvUtils::ToResultSetBridge(resultSet);
    int32_t count;
    auto result = bridge->GetRowCount(count);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(count, 3);
    count = -1;
    bridge->GetRowCount(count);
    EXPECT_EQ(count, 3);
}

/**
* @tc.name:BridgeOnGo
* @tc.desc: bridge on go, the input parameter is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvstoreDatashareBridgeTest, BridgeOnGoAbnormal, TestSize.Level0)
{
    DataQuery query;
    query.KeyPrefix("test");
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    singleKvStore->GetResultSet(query, resultSet);
    EXPECT_NE(resultSet, nullptr);
    auto bridge = KvUtils::ToResultSetBridge(resultSet);
    int32_t start = -1;
    int32_t target = 0;
    BridgeWriter writer;
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);
    EXPECT_TRUE(writer.GetKey().Empty());
    EXPECT_TRUE(writer.GetValue().Empty());
    start = 0;
    target = -1;
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);
    EXPECT_TRUE(writer.GetKey().Empty());
    EXPECT_TRUE(writer.GetValue().Empty());
    start = 1;
    target = 0;
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);
    EXPECT_TRUE(writer.GetKey().Empty());
    EXPECT_TRUE(writer.GetValue().Empty());
    start = 1;
    target = 3;
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);
    EXPECT_TRUE(writer.GetKey().Empty());
    EXPECT_TRUE(writer.GetValue().Empty());
}

/**
* @tc.name:BridgeOnGo
* @tc.desc: bridge on go, the input parameter is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvstoreDatashareBridgeTest, BridgeOnGoNormal, TestSize.Level0)
{
    DataQuery query;
    query.KeyPrefix("test");
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    singleKvStore->GetResultSet(query, resultSet);
    EXPECT_NE(resultSet, nullptr);
    auto bridge = KvUtils::ToResultSetBridge(resultSet);
    int start = 0;
    int target = 2;
    BridgeWriter writer;
    writer.SetAllocRowStatue(E_ERROR);
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);
    EXPECT_TRUE(writer.GetKey().Empty());
    EXPECT_TRUE(writer.GetValue().Empty());
    writer.SetAllocRowStatue(E_OK);
    EXPECT_EQ(bridge->OnGo(start, target, writer), target);
    size_t  keySize = 0;
    size_t  valueSize = 0;
    for (auto i = start; i <= target; i++) {
        resultSet->MoveToPosition(i);
        Entry entry;
        resultSet->GetEntry(entry);
        keySize += entry.key.Size();
        valueSize += entry.value.Size();
    }
    EXPECT_EQ(writer.GetKey().Size(), keySize);
    EXPECT_EQ(writer.GetValue().Size(), valueSize);
}
} // namespace
 