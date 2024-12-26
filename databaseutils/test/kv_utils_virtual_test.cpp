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
#define LOG_TAG "KvUtilVirtualTest"

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include "kv_utils.h"
#include "data_query.h"
#include "data_share_abs_predicates.h"
#include "data_share_values_bucket.h"
#include "data_share_value_object.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS::Test {
using var_t = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>>;
class KvUtilVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static DistributedKvDataManager managerVirtual;
    static std::shared_ptr<SingleKvStore> singleKvStoreVirtual;
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
std::shared_ptr<SingleKvStore> KvUtilVirtualTest::singleKvStoreVirtual = nullptr;
DistributedKvDataManager KvUtilVirtualTest::managerVirtual;

void KvUtilVirtualTest::SetUp(void)
{
    ZLOGI("SetUp begin.");}

void KvUtilVirtualTest::TearDown(void)
{
    ZLOGI("TearDown begin.");}

std::string KvUtilVirtualTest::Entry2Str(const Entry &entry)
{
    ZLOGI("Entry2Str begin.");
    return entry.key.ToString() + entry.value.ToString();
}

void KvUtilVirtualTest::ClearEntry(Entry &entry)
{
    ZLOGI("ClearEntry begin.");
    entry.key.Clear();
    entry.value.Clear();
}

Blob KvUtilVirtualTest::VariantKey2Blob(const var_t &value)
{
    ZLOGI("VariantKey2Blob begin.");
    std::vector<uint8_t> uData;
    if (auto *val = std::get_if<std::string>(&value)) {
        std::string data = *val;
        uData.insert(uData.end(), data.begin(), data.end());
    }
    return Blob(uData);
}

Blob KvUtilVirtualTest::VariantValue2Blob(const var_t &value)
{
    ZLOGI("VariantValue2Blob begin.");
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
        uint64_t tmp64 = htobe64(*reinterpret_cast<uint64_t *>(&tmp4dbl));
        tmp = reinterpret_cast<uint8_t *>(&tmp64);
        data.push_back(KvUtils::DOUBLE);
        data.insert(data.end(), tmp, tmp + sizeof(double) / sizeof(uint8_t));
    }
    auto intValue = std::get_if<int64_t>(&value);
    if (intValue != nullptr) {
        int64_t tmp4int = *intValue;
        uint64_t tmp64 = htobe64(*reinterpret_cast<uint64_t *>(&tmp4int));
        tmp = reinterpret_cast<uint8_t *>(&tmp64);
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

void KvUtilVirtualTest::SetUpTestCase(void)
{
    ZLOGI("SetUpTestCase begin.");
    Options options = {
        .createIfMissing = true,
        .encrypt = false,
        .autoSync = false,
        .kvStoreType = KvStoreType::SINGLE_VERSION,
        .schema = VALID_SCHEMA_STRICT_DEFINE };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/KvUtilVirtualTest");
    AppId appId = { "KvUtilVirtualTest" };
    StoreId storeId = { "test_single" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    managerVirtual.DeleteKvStore(appId, storeId, options.baseDir);
    managerVirtual.GetSingleKvStore(options, appId, storeId, singleKvStoreVirtual);
    EXPECT_NE(singleKvStoreVirtual, nullptr);
    singleKvStoreVirtual->Put("test_key_1", "{\"age\":1}");
    singleKvStoreVirtual->Put("test_key_2", "{\"age\":2}");
    singleKvStoreVirtual->Put("test_key_3", "{\"age\":3}");
    singleKvStoreVirtual->Put("kv_utils", "{\"age\":4}");
}

void KvUtilVirtualTest::TearDownTestCase(void)
{
    ZLOGI("TearDownTestCase begin.");
    managerVirtual.DeleteKvStore({"KvUtilVirtualTest" },
        { "test_single" }, "/data/service/el1/public/database/KvUtilVirtualTest");
    (void)remove("/data/service/el1/public/database/KvUtilVirtualTest/key");
    (void)remove("/data/service/el1/public/database/KvUtilVirtualTest/kvdb");
    (void)remove("/data/service/el1/public/database/KvUtilVirtualTest");
}

/**
 * @tc.name: KvStoreResultSetToResultSetBridgeAbnormal
 * @tc.desc: kvStore resultSetVirtual to resultSetVirtual bridgeVirtual, the former is nullptr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, KvStoreResultSetToResultSetBridgeAbnormal, TestSize.Level0)
{
    ZLOGI("KvStoreResultSetToResultSetBridgeAbnormal begin.");
    std::shared_ptr<KvStoreResultSet> resultSetVirtual = nullptr;
    auto bridgeVirtual = KvUtils::ToResultSetBridge(resultSetVirtual);
    EXPECT_EQ(bridgeVirtual, nullptr);
}

/**
 * @tc.name: KvStoreResultSetToResultSetBridge
 * @tc.desc: kvStore resultSetVirtual to resultSetVirtual bridgeVirtual
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, KvStoreResultSetToResultSetBridge, TestSize.Level0)
{
    ZLOGI("KvStoreResultSetToResultSetBridge begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.KeyPrefix("test");
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    std::shared_ptr<KvStoreResultSet> resultSetVirtual = nullptr;
    statusVirtual = singleKvStoreVirtual->GetResultSet(queryVirtual, resultSetVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(resultSetVirtual, nullptr);
    EXPECT_EQ(resultSetVirtual->GetCount(), 3);
    auto bridgeVirtual = KvUtils::ToResultSetBridge(resultSetVirtual);
    EXPECT_NE(bridgeVirtual, nullptr);
}

/**
 * @tc.name: PredicatesToQueryEqualTo
 * @tc.desc: to queryVirtual equalTo
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryEqualTo, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryEqualTo begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.EqualTo("$.age", 1);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.EqualTo("$.age", 1);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryNotEqualTo
 * @tc.desc: to queryVirtual not equalTo
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryNotEqualTo, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryNotEqualTo begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.NotEqualTo("$.age", 1);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.NotEqualTo("$.age", 1);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryGreaterThan
 * @tc.desc: to queryVirtual greater than
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryGreaterThan, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryGreaterThan begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.GreaterThan("$.age", 1);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.GreaterThan("$.age", 1);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryLessThan
 * @tc.desc: to queryVirtual less than
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryLessThan, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryLessThan begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.LessThan("$.age", 3);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.LessThan("$.age", 3);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryGreaterThanOrEqualTo
 * @tc.desc: to queryVirtual greater than or equalTo
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryGreaterThanOrEqualTo, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryGreaterThanOrEqualTo begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.GreaterThanOrEqualTo("$.age", 1);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.GreaterThanOrEqualTo("$.age", 1);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryLessThanOrEqualTo
 * @tc.desc: to queryVirtual less than or equalTo
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryLessThanOrEqualTo, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryLessThanOrEqualTo begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.LessThanOrEqualTo("$.age", 3);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.LessThanOrEqualTo("$.age", 3);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryIn
 * @tc.desc: to queryVirtual in
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryIn, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryIn begin.");
    std::vector<int> vectInt { 1, 2 };
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.In("$.age", vectInt);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.In("$.age", vectInt);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryNotIn
 * @tc.desc: to queryVirtual not in
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryNotIn, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryNotIn begin.");
    std::vector<int> vectInt { 1, 2 };
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.NotIn("$.age", vectInt);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.NotIn("$.age", vectInt);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryLike
 * @tc.desc: to queryVirtual or, like
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryLike, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryLike begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.Like("$.age", "1");
    predicatesVirtual.Or();
    predicatesVirtual.Like("$.age", "3");
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.Like("$.age", "1");
    trgQueryVirtual.Or();
    trgQueryVirtual.Like("$.age", "3");
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryUnlike
 * @tc.desc: to queryVirtual and, unlike
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryUnlike, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryUnlike begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.Unlike("$.age", "1");
    predicatesVirtual.And();
    predicatesVirtual.Unlike("$.age", "3");
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.Unlike("$.age", "1");
    trgQueryVirtual.And();
    trgQueryVirtual.Unlike("$.age", "3");
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryIsNull
 * @tc.desc: to queryVirtual is null
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryIsNull, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryIsNull begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.IsNull("$.age");
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.IsNull("$.age");
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryIsNotNull
 * @tc.desc: to queryVirtual is not null
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryIsNotNull, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryIsNotNull begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.IsNotNull("$.age");
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.IsNotNull("$.age");
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryOrderByAsc
 * @tc.desc: to queryVirtual is order by asc
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryOrderByAsc, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryOrderByAsc begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.OrderByAsc("$.age");
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.OrderByAsc("$.age");
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryOrderByDesc
 * @tc.desc: to queryVirtual is order by desc
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryOrderByDesc, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryOrderByDesc begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.OrderByDesc("$.age");
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.OrderByDesc("$.age");
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryLimit
 * @tc.desc: to queryVirtual is limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryLimit, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryLimit begin.");
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.Limit(0, 9);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.Limit(0, 9);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: PredicatesToQueryInKeys
 * @tc.desc: to queryVirtual is in keys
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, PredicatesToQueryInKeys, TestSize.Level0)
{
    ZLOGI("PredicatesToQueryInKeys begin.");
    std::vector<std::string> keys { "test_field", "", "^test_field", "^", "test_field_name" };
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.InKeys(keys);
    DataQuery queryVirtual;
    auto statusVirtual = KvUtils::ToQuery(predicatesVirtual, queryVirtual);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    DataQuery trgQueryVirtual;
    trgQueryVirtual.InKeys(keys);
    EXPECT_EQ(queryVirtual.ToString(), trgQueryVirtual.ToString());
}

/**
 * @tc.name: DataShareValuesBucketToEntryAbnormal
 * @tc.desc: dataShare values bucket to entry, the bucket is invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryAbnormal, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryAbnormal begin.");
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
 * @tc.name: DataShareValuesBucketToEntryNull
 * @tc.desc: dataShare values bucket to entry, the bucket value is null
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryNull, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryNull begin.");
    DataShareValuesBucket bucket {};
    bucket.Put(KEY, {});
    bucket.Put(VALUE, {});
    auto entry = KvUtils::ToEntry(bucket);
    Entry trgEntry;
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
 * @tc.name: DataShareValuesBucketToEntryInt64_t
 * @tc.desc: dataShare values bucket to entry, the bucket value type is int64_t
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryInt64_t, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryInt64_t begin.");
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<1>(314);
    var_t varKey;
    varKey.emplace<3>("314");
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    auto entry = KvUtils::ToEntry(bucket);
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
 * @tc.name: DataShareValuesBucketToEntryDouble
 * @tc.desc: dataShare values bucket to entry, the bucket value type is double
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryDouble, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryDouble begin.");
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<2>(3.14);
    var_t varKey;
    varKey.emplace<3>("314");
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    auto entry = KvUtils::ToEntry(bucket);
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
 * @tc.name: DataShareValuesBucketToEntryString
 * @tc.desc: dataShare values bucket to entry, the bucket value type is string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryString, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryString begin.");
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<3>("3.14");
    var_t varKey;
    varKey.emplace<3>("314");
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    auto entry = KvUtils::ToEntry(bucket);
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
 * @tc.name: DataShareValuesBucketToEntryBool
 * @tc.desc: dataShare values bucket to entry, the bucket value type is bool
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryBool, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryBool begin.");
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    var_t varValue;
    varValue.emplace<4>(true);
    var_t varKey;
    varKey.emplace<3>("314");
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    auto entry = KvUtils::ToEntry(bucket);
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
 * @tc.name: DataShareValuesBucketToEntryUint8Array
 * @tc.desc: dataShare values bucket to entry, the bucket value type is uint8array
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryUint8Array, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryUint8Array begin.");
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    var_t varValue;
    std::vector<uint8_t> vecUint8 { 3, 14 };
    varValue.emplace<5>(vecUint8);
    var_t varKey;
    varKey.emplace<3>("314");
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    auto entry = KvUtils::ToEntry(bucket);
    trgEntry.key = VariantKey2Blob(varKey);
    trgEntry.value = VariantValue2Blob(varValue);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
 * @tc.name: DataShareValuesBucketToEntryInvalidKey
 * @tc.desc: dataShare values bucket to entry, the bucket key type is not string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntryInvalidKey, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntryInvalidKey begin.");
    DataShareValuesBucket bucket {};
    Entry trgEntry;
    var_t varValue;
    std::vector<uint8_t> vecUint8 { 3, 14 };
    varValue.emplace<5>(vecUint8);
    var_t varKey;
    varKey.emplace<1>(314);
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    auto entry = KvUtils::ToEntry(bucket);
    trgEntry.key = VariantKey2Blob(varKey);
    EXPECT_EQ(Entry2Str(entry), Entry2Str(trgEntry));
}

/**
 * @tc.name: DataShareValuesBucketToEntriesAbnormal
 * @tc.desc: dataShare values bucket to entries, the buckets is invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntriesAbnormal, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntriesAbnormal begin.");
    std::vector<DataShareValuesBucket> buckets {};
    auto entries = KvUtils::ToEntries(buckets);
    EXPECT_TRUE(entries.empty());
}

/**
 * @tc.name: DataShareValuesBucketToEntriesNormal
 * @tc.desc: dataShare values bucket to entries, the buckets has valid value
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, DataShareValuesBucketToEntriesNormal, TestSize.Level0)
{
    ZLOGI("DataShareValuesBucketToEntriesNormal begin.");
    std::vector<DataShareValuesBucket> buckets {};
    DataShareValuesBucket bucket;
    Entry trgEntryFirst;
    Entry trgEntrySecond;
    var_t varValue;
    varValue.emplace<1>(314);
    var_t varKey;
    varKey.emplace<3>("314");
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    buckets.emplace_back(bucket);
    trgEntryFirst.key = VariantKey2Blob(varKey);
    trgEntryFirst.value = VariantValue2Blob(varValue);
    bucket.Clear();
    varValue.emplace<2>(3.14);
    varKey.emplace<3>("3.14");
    bucket.Put(KEY, varKey);
    bucket.Put(VALUE, varValue);
    buckets.emplace_back(bucket);
    trgEntrySecond.key = VariantKey2Blob(varKey);
    trgEntrySecond.value = VariantValue2Blob(varValue);
    auto entries = KvUtils::ToEntries(buckets);
    EXPECT_EQ(entries.size(), 2);
    EXPECT_EQ(Entry2Str(entries[0]), Entry2Str(trgEntryFirst));
    EXPECT_EQ(Entry2Str(entries[1]), Entry2Str(trgEntrySecond));
}

/**
 * @tc.name: GetKeysFromDataSharePredicatesAbnormal
 * @tc.desc: get keys from data share predicatesVirtual, the predicatesVirtual is invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, GetKeysFromDataSharePredicatesAbnormal, TestSize.Level0)
{
    ZLOGI("GetKeysFromDataSharePredicatesAbnormal begin.");
    DataSharePredicates predicatesVirtual;
    std::vector<Key> kvKeys;
    auto statusVirtual = KvUtils::GetKeys(predicatesVirtual, kvKeys);
    EXPECT_EQ(statusVirtual, Status::ERROR);
    predicatesVirtual.EqualTo("$.age", 1);
    statusVirtual = KvUtils::GetKeys(predicatesVirtual, kvKeys);
    EXPECT_EQ(statusVirtual, Status::NOT_SUPPORT);
}

/**
 * @tc.name: GetKeysFromDataSharePredicatesNormal
 * @tc.desc: get keys from data share predicatesVirtual, the predicatesVirtual has valid value
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlzuojiangjiang
 */
HWTEST_F(KvUtilVirtualTest, GetKeysFromDataSharePredicatesNormal, TestSize.Level0)
{
    ZLOGI("GetKeysFromDataSharePredicatesNormal begin.");
    std::vector<std::string> keys { "test_field", "", "^test_field", "^", "test_field_name" };
    DataSharePredicates predicatesVirtual;
    predicatesVirtual.InKeys(keys);
    std::vector<Key> kvKeys;
    auto statusVirtual = KvUtils::GetKeys(predicatesVirtual, kvKeys);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_EQ(keys.size(), kvKeys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        EXPECT_EQ(keys[i], kvKeys[i].ToString());
    }
}

/**
 * @tc.name: ToResultSetBridge_NullResultSet_ReturnsNull
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToResultSetBridge_NullResultSet_ReturnsNull, TestSize.Level0)
{
    ZLOGI("ToResultSetBridge_NullResultSet_ReturnsNull begin.");
    std::shared_ptr<KvStoreResultSet> nullResultSet = nullptr;
    auto result = KvUtils::ToResultSetBridge(nullResultSet);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: ToResultSetBridge_ValidResultSet_ReturnsBridge
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToResultSetBridge_ValidResultSet_ReturnsBridge, TestSize.Level0)
{
    ZLOGI("ToResultSetBridge_ValidResultSet_ReturnsBridge begin.");
    auto result = KvUtils::ToResultSetBridge(resultSet);
    EXPECT_NE(result, nullptr);
}

/**
 * @tc.name: ToQuery_ValidOperations_Success
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToQuery_ValidOperations_Success, TestSize.Level0)
{
    ZLOGI("ToQuery_ValidOperations_Success begin.");
    predicates->AddOperation({IN_KEY, {"key1", "key2"}});
    Status status = KvUtils::ToQuery(*predicates, *query);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: ToQuery_InvalidOperation_NotSupport
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToQuery_InvalidOperation_NotSupport, TestSize.Level0)
{
    ZLOGI("ToQuery_InvalidOperation_NotSupport begin.");
    predicates->AddOperation({-1, {}});
    Status status = KvUtils::ToQuery(*predicates, *query);
    EXPECT_EQ(status, Status::NOT_SUPPORT);
}

/**
 * @tc.name: ToEntries_EmptyBuckets_ReturnsEmpty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntries_EmptyBuckets_ReturnsEmpty, TestSize.Level0)
{
    ZLOGI("ToEntries_EmptyBuckets_ReturnsEmpty begin.");
    std::vector<DataShareValuesBucket> emptyBuckets;
    auto entries = KvUtils::ToEntries(emptyBuckets);
    EXPECT_TRUE(entries.empty());
}

/**
 * @tc.name: ToEntries_NonEmptyBuckets_ReturnsEntries
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntries_NonEmptyBuckets_ReturnsEntries, TestSize.Level0)
{
    ZLOGI("ToEntries_NonEmptyBuckets_ReturnsEntries begin.");
    DataShareValuesBucket bucket;
    bucket.valuesMap = {{"key", "value"}};
    std::vector<DataShareValuesBucket> buckets = {bucket};
    auto entries = KvUtils::ToEntries(buckets);
    EXPECT_EQ(entries.size(), 1);
}

/**
 * @tc.name: ToEntry_EmptyValuesMap_ReturnsEmptyEntry
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntries_NonEmptyBuckets_ReturnsEntries, TestSize.Level0)
{
    ZLOGI("ToEntries_NonEmptyBuckets_ReturnsEntries begin.");
    DataShareValuesBucket bucket;
    Entry entry = KvUtils::ToEntry(bucket);
    EXPECT_TRUE(entry.key.empty());
    EXPECT_TRUE(entry.value.empty());
}

/**
 * @tc.name: ToEntry_NonEmptyValuesMap_ReturnsEntry
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntry_NonEmptyValuesMap_ReturnsEntry, TestSize.Level0)
{
    ZLOGI("ToEntry_NonEmptyValuesMap_ReturnsEntry begin.");
    DataShareValuesBucket bucket;
    bucket.valuesMap = {{"key", "value"}};
    Entry entry = KvUtils::ToEntry(bucket);
    EXPECT_FALSE(entry.key.empty());
    EXPECT_FALSE(entry.value.empty());
}

/**
 * @tc.name: GetKeys_EmptyOperations_ReturnsError
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, GetKeys_EmptyOperations_ReturnsError, TestSize.Level0)
{
    ZLOGI("GetKeys_EmptyOperations_ReturnsError begin.");
    std::vector<Key> keys;
    Status status = KvUtils::GetKeys(*predicates, keys);
    EXPECT_EQ(status, Status::ERROR);
}

/**
 * @tc.name: GetKeys_ValidInKeyOperation_Success
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, GetKeys_ValidInKeyOperation_Success, TestSize.Level0)
{
    ZLOGI("GetKeys_ValidInKeyOperation_Success begin.");
    predicates->AddOperation({IN_KEY, {"key1", "key2"}});
    std::vector<Key> keys;
    Status status = KvUtils::GetKeys(*predicates, keys);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(keys.size(), 2);
}

/**
 * @tc.name: GetKeys_InvalidOperation_NotSupport
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, GetKeys_InvalidOperation_NotSupport, TestSize.Level0)
{
    ZLOGI("GetKeys_InvalidOperation_NotSupport begin.");
    predicates->AddOperation({-1, {}});
    std::vector<Key> keys;
    Status status = KvUtils::GetKeys(*predicates, keys);
    EXPECT_EQ(status, Status::NOT_SUPPORT);
}

/**
 * @tc.name: ToEntryKey_ValidKey_Success
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntryKey_ValidKey_Success, TestSize.Level0)
{
    ZLOGI("ToEntryKey_ValidKey_Success begin.");
    std::map<std::string, DataShareValueObject::Type> values = {{"key", "value"}};
    Blob blob;
    Status status = KvUtils::ToEntryKey(values, blob);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: ToEntryKey_MissingKey_Error
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntryKey_MissingKey_Error, TestSize.Level0)
{
    ZLOGI("ToEntryKey_MissingKey_Error begin.");
    std::map<std::string, DataShareValueObject::Type> values = {{"otherKey", "value"}};
    Blob blob;
    Status status = KvUtils::ToEntryKey(values, blob);
    EXPECT_EQ(status, Status::ERROR);
}

/**
 * @tc.name: ToEntryValue_ValidValue_Success
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntryValue_ValidValue_Success, TestSize.Level0)
{
    ZLOGI("ToEntryValue_ValidValue_Success begin.");
    std::map<std::string, DataShareValueObject::Type> values = {{"value", "value"}};
    Blob blob;
    Status status = KvUtils::ToEntryValue(values, blob);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: ToEntryValue_MissingValue_Error
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KvUtilsTest, ToEntryValue_MissingValue_Error, TestSize.Level0)
{
    ZLOGI("ToEntryValue_MissingValue_Error begin.");
    std::map<std::string, DataShareValueObject::Type> values = {{"otherValue", "value"}};
    Blob blob;
    Status status = KvUtils::ToEntryValue(values, blob);
    EXPECT_EQ(status, Status::ERROR);
}
} // namespace OHOS::Test