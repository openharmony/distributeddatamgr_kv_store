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
    int FreeLastRow() override;
    int Write(uint32_t column) override;
    int Write(uint32_t column, int64_t value) override;
    int Write(uint32_t column, double value) override;
    int Write(uint32_t column, const uint8_t *value, size_t size) override;
    int Write(uint32_t column, const char *value, size_t size) override;
    void SetAllocRowStatus(int status);
    void SetWriteStatus(int status);

private:
    int allocStatus_ = E_OK;
    int writeStatus_ = E_OK;
};

void BridgeWriter::SetAllocRowStatus(int status)
{
    allocStatus_ = status;
}

void BridgeWriter::SetWriteStatus(int status)
{
    writeStatus_ = status;
}

int BridgeWriter::AllocRow()
{
    return allocStatus_;
}

int BridgeWriter::FreeLastRow()
{
    return writeStatus_;
}

int BridgeWriter::Write(uint32_t column)
{
    return writeStatus_;
}

int BridgeWriter::Write(uint32_t column, int64_t value)
{
    return writeStatus_;
}

int BridgeWriter::Write(uint32_t column, double value)
{
    return writeStatus_;
}

int BridgeWriter::Write(uint32_t column, const uint8_t *value, size_t size)
{
    return writeStatus_;
}

int BridgeWriter::Write(uint32_t column, const char *value, size_t size)
{
    return writeStatus_;
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
    static std::shared_ptr<SingleKvStore> schemaSingleKvStore;
};

std::shared_ptr<SingleKvStore> KvstoreDatashareBridgeTest::singleKvStore = nullptr;
std::shared_ptr<SingleKvStore> KvstoreDatashareBridgeTest::schemaSingleKvStore = nullptr;
DistributedKvDataManager KvstoreDatashareBridgeTest::manager;
static constexpr int32_t INVALID_COUNT = -1;

void KvstoreDatashareBridgeTest::SetUpTestCase(void)
{
    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = false,
        .kvStoreType = KvStoreType::SINGLE_VERSION };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/KvstoreDatashareBridgeTest");
    AppId appId = { "KvstoreDatashareBridgeTest" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    StoreId storeId = { "test_single" };
    manager.DeleteKvStore(appId, storeId, options.baseDir);
    manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
    EXPECT_NE(singleKvStore, nullptr);
    std::vector<uint8_t> value = { 0x00, 0x74, 0x65, 0x73, 0x74 };
    singleKvStore->Put("test_string", value);
    value = { 0x03, 0x01, 0x02 };
    singleKvStore->Put("test_blob", value);
    value = { 0x04, 0x01 };
    singleKvStore->Put("test_boolean", value);
    value = { 0x05, 0x40, 0x50, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 };
    singleKvStore->Put("test_double", value);
    value = { 0x06, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    singleKvStore->Put("test_int64", value);

    options.schema = "{\"SCHEMA_VERSION\":\"1.0\", \"SCHEMA_MODE\":\"STRICT\", \"SCHEMA_SKIPSIZE\":0,"
        "\"SCHEMA_DEFINE\":{ \"age\":\"INTEGER, NOT NULL\"}, \"SCHEMA_INDEXES\":[\"$.age\"] }";
    storeId = { "test_single_schema" };
    manager.DeleteKvStore(appId, storeId, options.baseDir);
    manager.GetSingleKvStore(options, appId, storeId, schemaSingleKvStore);
    EXPECT_NE(schemaSingleKvStore, nullptr);
    schemaSingleKvStore->Put("test_key_1", "{\"age\":1}");
    schemaSingleKvStore->Put("test_key_2", "{\"age\":2}");
    schemaSingleKvStore->Put("test_key_3", "{\"age\":3}");
}

void KvstoreDatashareBridgeTest::TearDownTestCase(void)
{
    manager.DeleteKvStore({"KvstoreDatashareBridgeTest"}, {"test_single"},
        "/data/service/el1/public/database/KvstoreDatashareBridgeTest");
    manager.DeleteKvStore({"KvstoreDatashareBridgeTest"}, {"test_single_schema"},
        "/data/service/el1/public/database/KvstoreDatashareBridgeTest");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeTest/key");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeTest/kvdb");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeTest");
}

/**
* @tc.name: GetRowCountByInvalidBridge
* @tc.desc: get row count, the kvStore resultSet is nullptr
* @tc.type: FUNC
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
* @tc.name: KvStoreResultSetToDataShareResultSetAbnormal
* @tc.desc: kvStore resultSet to dataShare resultSet, the former has invalid predicate
* @tc.type: FUNC
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
* @tc.name: KvStoreResultSetToDataShareResultSetNormal
* @tc.desc: kvStore resultSet to dataShare resultSet, the former has valid predicate
* @tc.type: FUNC
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
    EXPECT_EQ(count, 5);
    count = -1;
    bridge->GetRowCount(count);
    EXPECT_EQ(count, 5);
}

/**
* @tc.name: BridgeOnGoAbnormal
* @tc.desc: bridge on go, the input parameter is invalid
* @tc.type: FUNC
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
    start = 0;
    target = -1;
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);
    start = 1;
    target = 0;
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);
}

/**
* @tc.name: BridgeOnGoNormal
* @tc.desc: bridge on go, the input parameter is valid
* @tc.type: FUNC
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
    int target = 4;
    BridgeWriter writer;

    writer.SetAllocRowStatus(E_ERROR);
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);

    writer.SetAllocRowStatus(E_OK);
    writer.SetWriteStatus(E_ERROR);
    EXPECT_EQ(bridge->OnGo(start, target, writer), -1);

    writer.SetWriteStatus(E_OK);
    EXPECT_EQ(bridge->OnGo(start, target, writer), target);

    schemaSingleKvStore->GetResultSet(query, resultSet);
    ASSERT_NE(resultSet, nullptr);
    bridge = KvUtils::ToResultSetBridge(resultSet);
    EXPECT_EQ(bridge->OnGo(0, 2, writer), 2);
}

/**
* @tc.name: BridgeOnGoWithInvalidValue
* @tc.desc: bridge on go, the input parameter is valid
* @tc.type: FUNC
*/
HWTEST_F(KvstoreDatashareBridgeTest, BridgeOnGoWithInvalidValue, TestSize.Level0)
{
    DataQuery query;
    query.KeyPrefix("test");
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    BridgeWriter writer;
    writer.SetAllocRowStatus(E_OK);
    writer.SetWriteStatus(E_OK);

    std::vector<uint8_t> value = { 0x05, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    singleKvStore->Put("test_invalid_value", value);
    singleKvStore->GetResultSet(query, resultSet);
    EXPECT_NE(resultSet, nullptr);
    auto bridge = KvUtils::ToResultSetBridge(resultSet);
    EXPECT_NE(bridge->OnGo(0, 5, writer), 5);

    value = { 0x06, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    singleKvStore->Put("test_invalid_value", value);
    singleKvStore->GetResultSet(query, resultSet);
    EXPECT_NE(resultSet, nullptr);
    bridge = KvUtils::ToResultSetBridge(resultSet);
    EXPECT_NE(bridge->OnGo(0, 5, writer), 5);

    value = { 0x07, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    singleKvStore->Put("test_invalid_value", value);
    singleKvStore->GetResultSet(query, resultSet);
    EXPECT_NE(resultSet, nullptr);
    bridge = KvUtils::ToResultSetBridge(resultSet);
    EXPECT_NE(bridge->OnGo(0, 5, writer), 5);
}
} // namespace