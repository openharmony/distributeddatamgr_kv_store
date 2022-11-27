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

#include <endian.h>
#include <vector>
#include <map>
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "distributed_kv_data_manager.h"
#include "gtest/gtest.h"
#include "kv_utils.h"
#include "kvstore_datashare_bridge.h"
#include "kvstore_result_set.h"
#include "result_set_bridge.h"
#include "store_errno.h"
#include "types.h"

namespace {
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
using var_t = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>>;
class KvUtilTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}

protected:
    static DistributedKvDataManager manager;
    static std::shared_ptr<SingleKvStore> singleKvStore;
    static constexpr const char *KEY = "key";
    static constexpr const char *VALUE = "value";
    static constexpr const char *VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
                                                              "\"SCHEMA_MODE\":\"STRICT\","
                                                              "\"SCHEMA_SKIPSIZE\":0,"
                                                              "\"SCHEMA_DEFINE\":"
                                                              "{"
                                                              "\"age\":\"INTEGER, NOT NULL\""
                                                              "},"
                                                              "\"SCHEMA_INDEXES\":[\"$.age\"]}";
    static std::string Entry2Str(const Entry &entry);
    static void ClearEntry(Entry &entry);
    static Blob VariantValue2Blob(const var_t &value);
    static Blob VariantKey2Blob(const var_t &value);
};
std::shared_ptr<SingleKvStore> KvUtilTest::singleKvStore = nullptr;
DistributedKvDataManager KvUtilTest::manager;

std::string KvUtilTest::Entry2Str(const Entry &entry)
{
    return entry.key.ToString() + entry.value.ToString();
}

void KvUtilTest::ClearEntry(Entry &entry)
{
    entry.key.Clear();
    entry.value.Clear();
}

Blob KvUtilTest::VariantKey2Blob(const var_t &value)
{
    std::vector<uint8_t> uData;
    if (auto *val = std::get_if<std::string>(&value)) {
        std::string data = *val;
        uData.insert(uData.end(), data.begin(), data.end());
    }
    return Blob(uData);
}

Blob KvUtilTest::VariantValue2Blob(const var_t &value)
{
    std::vector<uint8_t> data;
    auto strValue = std::get_if<std::string>(&value);
    if (strValue != nullptr) {
        data.push_back(KvUtils::STRING);
        data.insert(data.end(), (*strValue).begin(), (*strValue).end());
    }
    auto boolValue = std::get_if<bool>(&value);
    if (boolValue != nullptr) {
        data.push_back(KvUtils::BOOLEAN);
        data.push_back(static_cast<uint8_t>(*boolValue));
    }
    uint8_t *tmp = nullptr;
    auto dblValue = std::get_if<double>(&value);
    if (dblValue != nullptr) {
        double tmp4dbl = *dblValue;
        uint64_t tmp64 = htobe64(*reinterpret_cast<uint64_t*>(&tmp4dbl));
        tmp = reinterpret_cast<uint8_t*>(&tmp64);
        data.push_back(KvUtils::DOUBLE);
        data.insert(data.end(), tmp, tmp + sizeof(double) / sizeof(uint8_t));
    }
    auto intValue = std::get_if<int64_t>(&value);
    if (intValue != nullptr) {
        int64_t tmp4int = *intValue;
        uint64_t tmp64 = htobe64(*reinterpret_cast<uint64_t*>(&tmp4int));
        tmp = reinterpret_cast<uint8_t*>(&tmp64);
        data.push_back(KvUtils::INTEGER);
        data.insert(data.end(), tmp, tmp + sizeof(int64_t) / sizeof(uint8_t));
    }
    auto u8ArrayValue = std::get_if<std::vector<uint8_t>>(&value);
    if (u8ArrayValue != nullptr) {
        data.push_back(KvUtils::BYTE_ARRAY);
        data.insert(data.end(), (*u8ArrayValue).begin(), (*u8ArrayValue).end());
    }
    return Blob(data);
}

void KvUtilTest::SetUpTestCase(void)
{
    Options options = {.createIfMissing = true, .encrypt = false, .autoSync = false,
            .kvStoreType = KvStoreType::SINGLE_VERSION, .schema =  VALID_SCHEMA_STRICT_DEFINE};
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/kvUtilTest");
    AppId appId = { "kvUtilTest" };
    StoreId storeId = { "test_single" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    manager.DeleteKvStore(appId, storeId, options.baseDir);
    manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
    EXPECT_NE(singleKvStore, nullptr);
    singleKvStore->Put("test_key_1", "{\"age\":1}");
    singleKvStore->Put("test_key_2", "{\"age\":2}");
    singleKvStore->Put("test_key_3", "{\"age\":3}");
    singleKvStore->Put("kv_utils", "{\"age\":4}");
}

void KvUtilTest::TearDownTestCase(void)
{
    manager.DeleteKvStore(
            {"kvUtilTest"}, {"test_single"}, "/data/service/el1/public/database/kvUtilTest");
    (void) remove("/data/service/el1/public/database/kvUtilTest/key");
    (void) remove("/data/service/el1/public/database/kvUtilTest/kvdb");
    (void) remove("/data/service/el1/public/database/kvUtilTest");
}

/**
* @tc.name: KvStoreResultSetToResultSetBridge
* @tc.desc: kvStore resultSet to resultSet bridge, the former is nullptr
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, KvStoreResultSetToResultSetBridgeAbnormal, TestSize.Level0)
{
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    auto bridge = KvUtils::ToResultSetBridge(resultSet);
    EXPECT_EQ(bridge, nullptr);
}

/**
* @tc.name: KvStoreResultSetToResultSetBridge
* @tc.desc: kvStore resultSet to resultSet bridge
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, KvStoreResultSetToResultSetBridge, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.KeyPrefix("test");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    status = singleKvStore->GetResultSet(query, resultSet);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GetCount(), 3);
    auto bridge = KvUtils::ToResultSetBridge(resultSet);
    EXPECT_NE(bridge, nullptr);
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryEqualTo, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.EqualTo("$.age", 1);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.EqualTo("$.age", 1);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query not equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryNotEqualTo, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.NotEqualTo("$.age", 1);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.NotEqualTo("$.age", 1);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query greater than
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryGreaterThan, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.GreaterThan("$.age", 1);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.GreaterThan("$.age", 1);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query less than
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryLessThan, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.LessThan("$.age", 3);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.LessThan("$.age", 3);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query greater than or equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryGreaterThanOrEqualTo, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.GreaterThanOrEqualTo("$.age", 1);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.GreaterThanOrEqualTo("$.age", 1);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query less than or equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryLessThanOrEqualTo, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.LessThanOrEqualTo("$.age", 3);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.LessThanOrEqualTo("$.age", 3);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query in
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryIn, TestSize.Level0)
{
    std::vector<int> vectInt{ 1, 2 };
    DataSharePredicates predicates;
    predicates.In("$.age", vectInt);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.In("$.age", vectInt);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query not in
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryNotIn, TestSize.Level0)
{
    std::vector<int> vectInt{ 1, 2 };
    DataSharePredicates predicates;
    predicates.NotIn("$.age", vectInt);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.NotIn("$.age", vectInt);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query or, like
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryLike, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.Like("$.age", "1");
    predicates.Or();
    predicates.Like("$.age", "3");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.Like("$.age", "1");
    trgQuery.Or();
    trgQuery.Like("$.age", "3");
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query and, unlike
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryUnlike, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.Unlike("$.age", "1");
    predicates.And();
    predicates.Unlike("$.age", "3");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.Unlike("$.age", "1");
    trgQuery.And();
    trgQuery.Unlike("$.age", "3");
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query is null
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryIsNull, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.IsNull("$.age");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.IsNull("$.age");
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query is not null
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryIsNotNull, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.IsNotNull("$.age");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.IsNotNull("$.age");
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query is order by asc
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryOrderByAsc, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.OrderByAsc("$.age");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.OrderByAsc("$.age");
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query is order by desc
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryOrderByDesc, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.OrderByDesc("$.age");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.OrderByDesc("$.age");
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query is limit
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryLimit, TestSize.Level0)
{
    DataSharePredicates predicates;
    predicates.Limit(0, 9);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.Limit(0, 9);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: PredicatesToQuery
* @tc.desc: to query is in keys
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, PredicatesToQueryInKeys, TestSize.Level0)
{
    std::vector<std::string> keys { "test_field", "", "^test_field", "^", "test_field_name" };
    DataSharePredicates predicates;
    predicates.InKeys(keys);
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    EXPECT_EQ(status, Status::SUCCESS);
    DataQuery trgQuery;
    trgQuery.InKeys(keys);
    EXPECT_EQ(query.ToString(), trgQuery.ToString());
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryAbnormal, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    Entry trgEntry {};
    auto entry = KvUtils::ToEntry(bucket);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
    bucket.Put("invalid key", "value");
    EXPECT_FALSE(bucket.IsEmpty());
    entry = KvUtils::ToEntry(bucket);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
    bucket.Put(KEY, "value");
    entry = KvUtils::ToEntry(bucket);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket value is null
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryNull, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    bucket.Put(KEY, {});
    bucket.Put(VALUE, {});
    auto entry = KvUtils::ToEntry(bucket);
    Entry trgEntry;
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket value type is int
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryInt64, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    int valueInt = 314;
    std::string key = "314";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, valueInt);
    auto entry = KvUtils::ToEntry(bucket);
    var_t varValue;
    varValue.emplace<1>(314);
    var_t varKey;
    varKey.emplace<3>("314");
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket value type is int64_t
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryInt64_t, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    int64_t valueInt = 314;
    std::string key = "314";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, valueInt);
    auto entry = KvUtils::ToEntry(bucket);
    var_t varValue;
    varValue.emplace<1>(314);
    var_t varKey;
    varKey.emplace<3>("314");
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket value type is double
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryDouble, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    double valueDouble = 3.14;
    std::string key = "3.14";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, valueDouble);
    auto entry = KvUtils::ToEntry(bucket);
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<2>(3.14);
    var_t varKey;
    varKey.emplace<3>("3.14");
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket value type is string
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryString, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    std::string valueDouble = "3.14";
    std::string key = "3.14";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, valueDouble);
    auto entry = KvUtils::ToEntry(bucket);
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<3>("3.14");
    var_t varKey;
    varKey.emplace<3>("3.14");
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket value type is bool
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryBool, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    bool valueBool = true;
    std::string key = "314";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, valueBool);
    auto entry = KvUtils::ToEntry(bucket);
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<4>(true);
    var_t varKey;
    varKey.emplace<3>("314");
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket value type is uint8array
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryUint8Array, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    std::vector<uint8_t> vecUint8 { 3, 14 };
    std::string key = "314";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, vecUint8);
    auto entry = KvUtils::ToEntry(bucket);
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<5>(vecUint8);
    var_t varKey;
    varKey.emplace<3>("314");
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entry, the bucket key type is not string
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntryInvalidKey, TestSize.Level0)
{
    DataShareValuesBucket bucket {};
    std::vector<uint8_t> vecUint8 { 3, 14 };
    bucket.Put(KEY, vecUint8);
    bucket.Put(VALUE, vecUint8);
    auto entry = KvUtils::ToEntry(bucket);
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<5>(vecUint8);
    var_t varKey;
    varKey.emplace<1>(314);
    trgEntry.key = VariantKey2Blob(varKey);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entries, the buckets is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntriesAbnormal, TestSize.Level0)
{
    std::vector<DataShareValuesBucket> buckets {};
    auto entries = KvUtils::ToEntries(buckets);
    EXPECT_TRUE(entries.empty());
}

/**
* @tc.name: ToEntry
* @tc.desc: dataShare values bucket to entries, the buckets has valid value
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, DataShareValuesBucketToEntriesNormal, TestSize.Level0)
{
    std::vector<DataShareValuesBucket> buckets {};
    DataShareValuesBucket bucket;
    int64_t valueInt64 = 314;
    std::string key = "314";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, valueInt64);
    buckets.emplace_back(bucket);
    Entry trgEntryFirst;
    Entry trgEntrySecond;
    var_t varValue;
    varValue.emplace<1>(314);
    var_t varKey;
    varKey.emplace<3>("314");
    trgEntryFirst.key = VariantKey2Blob(varKey);
    trgEntryFirst.value = VariantValue2Blob(varValue);
    bucket.Clear();
    double valueDouble = 3.14;
    key = "3.14";
    bucket.Put(KEY, key);
    bucket.Put(VALUE, valueDouble);
    buckets.emplace_back(bucket);
    varValue.emplace<2>(3.14);
    varKey.emplace<3>("3.14");
    trgEntrySecond.key = VariantKey2Blob(varKey);
    trgEntrySecond.value = VariantValue2Blob(varValue);
    auto entries = KvUtils::ToEntries(buckets);
    EXPECT_EQ(entries.size(), 2);
    EXPECT_EQ(Entry2Str(entries[0]), Entry2Str(trgEntryFirst));
    EXPECT_EQ(Entry2Str(entries[1]), Entry2Str(trgEntrySecond));
}

/**
* @tc.name: GetKeys
* @tc.desc: get keys from data share predicates, the predicates is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, GetKeysFromDataSharePredicatesAbnormal, TestSize.Level0)
{
    DataSharePredicates predicates;
    std::vector<Key> kvKeys;
    auto status = KvUtils::GetKeys(predicates, kvKeys);
    EXPECT_EQ(status, Status::ERROR);
    predicates.EqualTo("$.age", 1);
    status = KvUtils::GetKeys(predicates, kvKeys);
    EXPECT_EQ(status, Status::NOT_SUPPORT);
}

/**
* @tc.name: GetKeys
* @tc.desc: get keys from data share predicates, the predicates has valid value
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(KvUtilTest, GetKeysFromDataSharePredicatesNormal, TestSize.Level0)
{
    std::vector<std::string> keys { "test_field", "", "^test_field", "^", "test_field_name" };
    DataSharePredicates predicates;
    predicates.InKeys(keys);
    std::vector<Key> kvKeys;
    auto status = KvUtils::GetKeys(predicates, kvKeys);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(keys.size(), kvKeys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        EXPECT_EQ(keys[i], kvKeys[i].ToString());
    }
}
} // namespace