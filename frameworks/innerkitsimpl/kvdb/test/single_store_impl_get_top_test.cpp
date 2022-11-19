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
#include <gtest/gtest.h>

#include <condition_variable>
#include <vector>

#include "dev_manager.h"
#include "store_manager.h"
#include "distributed_kv_data_manager.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
class SingleStoreImplGetTopTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();

    static std::shared_ptr<SingleKvStore> singleKvStore; // declare kvstore instance.
    static Status initStatus;
    static int MAX_VALUE_SIZE;
};

const std::string VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
                                               "\"SCHEMA_MODE\":\"STRICT\","
                                               "\"SCHEMA_SKIPSIZE\":0,"
                                               "\"SCHEMA_DEFINE\":{"
                                               "\"age\":\"INTEGER, NOT NULL\""
                                               "},"
                                               "\"SCHEMA_INDEXES\":[\"$.age\"]}";

std::shared_ptr<SingleKvStore> SingleStoreImplGetTopTest::singleKvStore = nullptr;
Status SingleStoreImplGetTopTest::initStatus = Status::ERROR;
int SingleStoreImplGetTopTest::MAX_VALUE_SIZE = 4 * 1024 * 1024; // max value size is 4M.

void SingleStoreImplGetTopTest::SetUpTestCase(void)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = true,
        .kvStoreType = KvStoreType::SINGLE_VERSION };
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "test_single" }; // define kvstore(database) name.
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    // [create and] open and initialize kvstore instance.
    initStatus = manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
}

void SingleStoreImplGetTopTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void SingleStoreImplGetTopTest::SetUp(void)
{}

void SingleStoreImplGetTopTest::TearDown(void)
{}

/**
* @tc.name: GetEntries
* @tc.desc: get entries order by write time Asc
* @tc.type: FUNC
* @tc.require:I5OM83
* @tc.author:ht
*/
HWTEST_F(SingleStoreImplGetTopTest, GetEntriesOrderByWriteTimeAsc, TestSize.Level0)
{
    ASSERT_NE(singleKvStore, nullptr);
    std::vector<Entry> input;
    for (size_t i = 10; i < 30; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
        auto status =singleKvStore->Put(
            entry.key, entry.value);
        ASSERT_EQ(status, SUCCESS);
    }
    DataQuery query;
    query.KeyPrefix("1");
    query.OrderByWriteTime(true);
    std::vector<Entry> output;
    auto status = singleKvStore->GetEntries(query, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(output.size(), 10);
    for (size_t i = 0; i < output.size(); ++i) {
        ASSERT_TRUE(input[i].key == output[i].key);
        ASSERT_TRUE(input[i].value == output[i].value);
    }
}

/**
* @tc.name: GetEntries
* @tc.desc: get entries order by write time Desc
* @tc.type: FUNC
* @tc.require:I5OM83
* @tc.author:ht
*/
HWTEST_F(SingleStoreImplGetTopTest, GetEntriesOrderByWriteTimeDesc, TestSize.Level0)
{
    ASSERT_NE(singleKvStore, nullptr);
    std::vector<Entry> input;
    for (size_t i = 10; i < 30; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
        auto status =singleKvStore->Put(
            entry.key, entry.value);
        ASSERT_EQ(status, SUCCESS);
    }
    DataQuery query;
    query.KeyPrefix("1");
    query.OrderByWriteTime(false);
    std::vector<Entry> output;
    auto status = singleKvStore->GetEntries(query, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(output.size(), 10);
    for (size_t i = 0; i < output.size(); ++i) {
        ASSERT_TRUE(input[9-i].key == output[i].key);
        ASSERT_TRUE(input[9-i].value == output[i].value);
    }
}

/**
* @tc.name: GetEntries
* @tc.desc: get entries order by write time no prefix
* @tc.type: FUNC
* @tc.require:I5OM83
* @tc.author:ht
*/
HWTEST_F(SingleStoreImplGetTopTest, GetEntriesOrderByWriteTimeNoPrefix, TestSize.Level0)
{
    ASSERT_NE(singleKvStore, nullptr);
    std::vector<Entry> input;
    for (size_t i = 10; i < 30; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
        auto status =singleKvStore->Put(
            entry.key, entry.value);
        ASSERT_EQ(status, SUCCESS);
    }
    singleKvStore->Put("test_key_1", "{\"name\":1}");
    DataQuery query;
    query.OrderByWriteTime(true);
    query.EqualTo("$.name",1);
    std::vector<Entry> output;
    auto status = singleKvStore->GetEntries(query, output);
    ASSERT_EQ(status, NOT_SUPPORT);
    ASSERT_EQ(output.size(), 0);
}

/**
* @tc.name: GetResultSet
* @tc.desc: get result set order by write time Asc
* @tc.type: FUNC
* @tc.require:I5OM83
* @tc.author:ht
*/
HWTEST_F(SingleStoreImplGetTopTest, GetResultSetOrderByWriteTimeAsc, TestSize.Level0)
{
    ASSERT_NE(singleKvStore, nullptr);
    std::vector<Entry> input;
    for (size_t i = 10; i < 30; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
        auto status =singleKvStore->Put(
            entry.key, entry.value);
        ASSERT_EQ(status, SUCCESS);
    }
    DataQuery query;
    query.InKeys({"10_k", "11_k"});
    query.OrderByWriteTime(true);
    std::shared_ptr<KvStoreResultSet> output;
    auto status = singleKvStore->GetResultSet(query, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->GetCount(), 2);
    for (size_t i = 0; i < 2; ++i) {
        output->MoveToNext();
        Entry entry;
        status=output->GetEntry(entry);
        ASSERT_EQ(status, SUCCESS);
        ASSERT_TRUE(input[i].key == entry.key);
        ASSERT_TRUE(input[i].value == entry.value);
    }
}

/**
* @tc.name: GetResultSet
* @tc.desc: get result set order by write time Desc
* @tc.type: FUNC
* @tc.require:I5OM83
* @tc.author:ht
*/
HWTEST_F(SingleStoreImplGetTopTest, GetResultSetOrderByWriteTimeDesc, TestSize.Level0)
{
    ASSERT_NE(singleKvStore, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (size_t i = 10; i < 30; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
        dictionary[entry.key] = entry.value;
        auto status =singleKvStore->Put(
            entry.key, entry.value);
        ASSERT_EQ(status, SUCCESS);
    }
    DataQuery query;
    query.KeyPrefix("1");
    query.OrderByWriteTime(false);
    std::shared_ptr<KvStoreResultSet> output;
    auto status = singleKvStore->GetResultSet(query, output);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->GetCount(), 10);
    for (size_t i = 0; i < 10; ++i) {
        output->MoveToNext();
        Entry entry;
        output->GetEntry(entry);
        ASSERT_TRUE(input[9-i].key == entry.key);
        ASSERT_TRUE(input[9-i].value == entry.value);
    }
}

/**
* @tc.name: GetResultSet
* @tc.desc: get result set order by write time no prefix
* @tc.type: FUNC
* @tc.require:I5OM83
* @tc.author:ht
*/
HWTEST_F(SingleStoreImplGetTopTest, GetResultSetOrderByWriteTimeNoPrefix, TestSize.Level0)
{
    ASSERT_NE(singleKvStore, nullptr);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (size_t i = 10; i < 30; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        input.push_back(entry);
        dictionary[entry.key] = entry.value;
        auto status =singleKvStore->Put(
            entry.key, entry.value);
        ASSERT_EQ(status, SUCCESS);
    }
    singleKvStore->Put("test_key_1", "{\"name\":1}");
    DataQuery query;
    query.OrderByWriteTime(true);
    query.EqualTo("$.name", 1);
    std::shared_ptr<KvStoreResultSet> output;
    auto status = singleKvStore->GetResultSet(query, output);
    ASSERT_EQ(status, NOT_SUPPORT);
    ASSERT_EQ(output, nullptr);
}