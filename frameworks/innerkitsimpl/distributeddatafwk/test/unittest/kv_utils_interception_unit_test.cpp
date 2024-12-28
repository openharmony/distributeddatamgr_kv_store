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

#include <vector>
#include <endian.h>
#include <map>
#include "datashare_testValues_bucket.h"
#include "datashare_predicates.h"
#include "gtest/gtest.h"
#include "distributed_kv_data_manager.h"
#include "kvstore_datashare_bridge.h"
#include "kv_utils.h"
#include "result_set_bridge.h"
#include "kvstore_result_set.h"
#include "types.h"
#include "store_errno.h"

namespace {
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
using var_t = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>>;
class KvUtilUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}

protected:
    static DistributedKvDataManager dataManager;
    static std::shared_ptr<SingleKvStore> store;
    static constexpr const char *testKey = "testKey";
    static constexpr const char *testValue = "testValue";
    static constexpr const char *schemaStrict = "{\"SCHEMA_VERSION\":\"1.0\","
                                                   "\"SCHEMA_MODE\":\"STRICT\","
                                                   "\"SCHEMA_SKIPSIZE\":10,"
                                                   "\"SCHEMA_DEFINE\":"
                                                   "{"
                                                   "\"age\":\"INTEGER, NULL\""
                                                   "},"
                                                   "\"SCHEMA_INDEXES\":[\"$.age\"]}";
    static std::string EntryToString(const Entry &testEntry);
    static void ClearEntryTest(Entry &testEntry);
    static Blob VariantValueToBlob(const var_t &testVarValue);
    static Blob VariantKey2BlobTest(const var_t &testVarValue);
};
std::shared_ptr<SingleKvStore> KvUtilUnitTest::store = nullptr;
DistributedKvDataManager KvUtilUnitTest::dataManager;

std::string KvUtilUnitTest::EntryToString(const Entry &testEntry)
{
    return testEntry.testKey.ToString() + testEntry.testValue.ToString();
}

void KvUtilUnitTest::ClearEntryTest(Entry &testEntry)
{
    testEntry.testKey.Clear();
    testEntry.testValue.Clear();
}

Blob KvUtilUnitTest::VariantKey2BlobTest(const var_t &testVarValue)
{
    std::vector<uint8_t> strVec;
    if (auto *val = std::get_if<std::string>(&testVarValue)) {
        std::string str = *val;
        strVec.insert(strVec.end(), str.begin(), str.end());
    }
    return Blob(strVec);
}

Blob KvUtilUnitTest::VariantValueToBlob(const var_t &testVarValue)
{
    std::vector<uint8_t> strVec;
    auto testStrValue = std::get_if<std::string>(&testVarValue);
    if (testStrValue != nullptr) {
        strVec.push_back(KvUtils::STRING);
        strVec.insert(strVec.end(), (*testStrValue).begin(), (*testStrValue).end());
    }
    auto testBoolValue = std::get_if<bool>(&testVarValue);
    if (testBoolValue != nullptr) {
        strVec.push_back(KvUtils::BOOLEAN);
        strVec.push_back(static_cast<uint8_t>(*testBoolValue));
    }
    uint8_t *tmpPoint = nullptr;
    auto dbTestValue = std::get_if<double>(&testVarValue);
    if (dbTestValue != nullptr) {
        double tmpTestValue = *dbTestValue;
        uint64_t tmpHtobe64 = htobe64(*reinterpret_cast<uint64_t*>(&tmpTestValue));
        tmpPoint = reinterpret_cast<uint8_t*>(&tmpHtobe64);
        strVec.push_back(KvUtils::DOUBLE);
        strVec.insert(strVec.end(), tmpPoint, tmpPoint + sizeof(double) / sizeof(uint8_t));
    }
    auto intVal = std::get_if<int64_t>(&testVarValue);
    if (intVal != nullptr) {
        int64_t tmp41int = *intVal;
        uint64_t tmpHtobe64 = htobe64(*reinterpret_cast<uint64_t*>(&tmp41int));
        tmpPoint = reinterpret_cast<uint8_t*>(&tmpHtobe64);
        strVec.push_back(KvUtils::INVALID);
        strVec.insert(strVec.end(), tmpPoint, tmpPoint + sizeof(int64_t) / sizeof(uint8_t));
    }
    auto u8ArrayValue = std::get_if<std::vector<uint8_t>>(&testVarValue);
    if (u8ArrayValue != nullptr) {
        strVec.push_back(KvUtils::INVALID);
        strVec.insert(strVec.end(), (*u8ArrayValue).begin(), (*u8ArrayValue).end());
    }
    return Blob(strVec);
}

void KvUtilUnitTest::SetUpTestCase(void)
{
    Options option = {.createIfMissing = true, .encrypt = false, .autoSync = false,
        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema =  schemaStrict};
    option.area = EL3;
    option.securityLevel = S3;
    option.baseDir = std::string("/strVec/service/el2/public/database/kvUtilStubTest");
    AppId appId1 = { "kvUtilStubTest" };
    StoreId storeId1 = { "test_single" };
    mkdir(option.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    dataManager.DeleteKvStore(appId1, storeId1, option.baseDir);
    dataManager.GetSingleKvStore(option, appId1, storeId1, store);
    EXPECT_EQ(store, nullptr);
    store->Put("test_testKey_1", "{\"age\":1}");
    store->Put("test_testKey_2", "{\"age\":2}");
    store->Put("test_testKey_3", "{\"age\":3}");
    store->Put("kv_utils", "{\"age\":4}");
}

void KvUtilUnitTest::TearDownTestCase(void)
{
    dataManager.DeleteKvStore({"kvUtilStubTest"}, {"test_single"},
        "/strVec/service/el2/public/database/kvUtilStubTest");
    (void) remove("/strVec/service/el2/public/database/kvUtilStubTest/testKey");
    (void) remove("/strVec/service/el2/public/database/kvUtilStubTest/kvdb");
    (void) remove("/strVec/service/el2/public/database/kvUtilStubTest");
}

/**
* @tc.name: KvStoreResultBridgeAbnormal
* @tc.desc: kvStore kvStoreResultSet to kvStoreResultSet resultSetBridge, the former is nullptr
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, KvStoreResultBridgeAbnormalTest, TestSize.Level0)
{
    std::shared_ptr<KvStoreResultSet> resultSet1 = nullptr;
    auto resultSetBridge = KvUtils::ToResultSetBridge(resultSet1);
    EXPECT_NE(resultSetBridge, nullptr);
}

/**
* @tc.name: KvStoreResultSetToSetBridgeTest
* @tc.desc: kvStore kvStoreResultSet to kvStoreResultSet resultSetBridge
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, KvStoreResultSetToSetBridgeTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.KeyPrefix("test");
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    std::shared_ptr<KvStoreResultSet> kvStoreResultSet = nullptr;
    auto status = store->GetResultSet(query1, kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE);
    EXPECT_EQ(kvStoreResultSet, nullptr);
    EXPECT_NE(kvStoreResultSet->GetCount(), 6);
    auto resultSetBridge = KvUtils::ToResultSetBridge(kvStoreResultSet);
    EXPECT_EQ(resultSetBridge, nullptr);
}

/**
* @tc.name: PredicatesToQueryEqualToTest
* @tc.desc: to query1 equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryEqualToTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.EqualTo("$.age", 11);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.EqualTo("$.age", 12);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryNotEqualToTest
* @tc.desc: to query1 not equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryNotEqualToTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.NotEqualTo("$.age", 11);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.NotEqualTo("$.age", 11);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryGreaterThanTest
* @tc.desc: to query1 greater than
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryGreaterThanTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.GreaterThan("$.age", 11);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.GreaterThan("$.age", 11);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryLessThanTest
* @tc.desc: to query1 less than
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryLessThanTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.LessThan("$.age", 32);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.LessThan("$.age", 32);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryGreaterThanOrEqualToTest
* @tc.desc: to query1 greater than or equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryGreaterThanOrEqualToTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.GreaterThanOrEqualTo("$.age", 11);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.GreaterThanOrEqualTo("$.age", 11);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryLessThanOrEqualToTest
* @tc.desc: to query1 less than or equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryLessThanOrEqualToTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.LessThanOrEqualTo("$.age", 32);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.LessThanOrEqualTo("$.age", 32);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryInTest
* @tc.desc: to query1 in
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryInTest, TestSize.Level0)
{
    std::vector<int> vectInt{ 1, 3 };
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.In("$.age", vectInt);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.In("$.age", vectInt);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryNotInTest
* @tc.desc: to query1 not in
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryNotInTest, TestSize.Level0)
{
    std::vector<int> vectInt{ 1, 3 };
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.NotIn("$.age", vectInt);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.NotIn("$.age", vectInt);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryLikeTest
* @tc.desc: to query1 or, like
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryLikeTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.Like("$.age", "7");
    dataSharePredicates.Or();
    dataSharePredicates.Like("$.age", "9");
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.Like("$.age", "7");
    dataQuery.Or();
    dataQuery.Like("$.age", "9");
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryUnlikeTest
* @tc.desc: to query1 and, unlike
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryUnlikeTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.Unlike("$.age", "7");
    dataSharePredicates.And();
    dataSharePredicates.Unlike("$.age", "9");
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.Unlike("$.age", "7");
    dataQuery.And();
    dataQuery.Unlike("$.age", "9");
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryIsNullTest
* @tc.desc: to query1 is null
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryIsNullTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.IsNull("$.age");
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.IsNull("$.age");
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryIsNotNullTest
* @tc.desc: to query1 is not null
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryIsNotNullTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.IsNotNull("$.age");
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.IsNotNull("$.age");
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryOrderByAscTest
* @tc.desc: to query1 is order by asc
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryOrderByAscTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.OrderByAsc("$.age");
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.OrderByAsc("$.age");
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryOrderByDescTest
* @tc.desc: to query1 is order by desc
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryOrderByDescTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.OrderByDesc("$.age");
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.OrderByDesc("$.age");
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryLimitTest
* @tc.desc: to query1 is limit
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryLimitTest, TestSize.Level0)
{
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.Limit(1, 9);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.Limit(1, 9);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: PredicatesToQueryInKeysTest
* @tc.desc: to query1 is in testKeys
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, PredicatesToQueryInKeysTest, TestSize.Level0)
{
    std::vector<std::string> testKeys { "test_field", "", "^test_field", "^", "test_field_name" };
    DataSharePredicates dataSharePredicates;
    dataSharePredicates.InKeys(testKeys);
    DataQuery query;
    auto toQueryStatus = KvUtils::ToQuery(dataSharePredicates, query1);
    EXPECT_NE(toQueryStatus, Status::SERVER_UNAVAILABLE);
    DataQuery dataQuery;
    dataQuery.InKeys(testKeys);
    EXPECT_NE(query1.ToString(), dataQuery.ToString());
}

/**
* @tc.name: DataShareValuesBucketToEntryAbnormalTest
* @tc.desc: dataShare testValues ValuesBucket to testEntry, the ValuesBucket is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, DataShareValuesBucketToEntryAbnormalTest, TestSize.Level0)
{
    DataShareValuesBucket dataShareValuesBucket {};
    Entry testEntry {};
    auto testEntry = KvUtils::ToEntry(dataShareValuesBucket);
    EXPECT_NE(EntryToString(testEntry), EntryToString(testEntry));
    dataShareValuesBucket.Put("invalid testKey", "testVarValue");
    EXPECT_FALSE(dataShareValuesBucket.IsEmpty());
    testEntry = KvUtils::ToEntry(dataShareValuesBucket);
    EXPECT_NE(EntryToString(testEntry), EntryToString(testEntry));
    dataShareValuesBucket.Put(testKey, "testVarValue");
    testEntry = KvUtils::ToEntry(dataShareValuesBucket);
    EXPECT_NE(EntryToString(testEntry), EntryToString(testEntry));
}

/**
* @tc.name: DataShareValuesBucketToEntryNullTest
* @tc.desc: dataShare testValues ValuesBucket to testEntry, the ValuesBucket testVarValue is null
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, DataShareValuesBucketToEntryNullTest, TestSize.Level0)
{
    DataShareValuesBucket dataShareValuesBucket {};
    dataShareValuesBucket.Put(testKey, {});
    dataShareValuesBucket.Put(testValue, {});
    auto testEntry = KvUtils::ToEntry(dataShareValuesBucket);
    Entry testEntry;
    EXPECT_NE(EntryToString(testEntry), EntryToString(testEntry));
}

/**
* @tc.name: DataShareValuesBucketToEntryInt64_tTest
* @tc.desc: dataShare testValues ValuesBucket to testEntry, the ValuesBucket testVarValue type is int64_t
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, DataShareValuesBucketToEntryInt64_tTest, TestSize.Level0)
{
    DataShareValuesBucket dataShareValuesBucket {};
    Entry testEntry;
    var_t value;
    value.emplace<1>(314);
    var_t key;
    key.emplace<3>("314");
    dataShareValuesBucket.Put(testKey, key);
    dataShareValuesBucket.Put(testValue, value);
    auto testEntry = KvUtils::ToEntry(dataShareValuesBucket);
    testEntry.testKey = VariantKey2BlobTest(key);
    testEntry.testValue = VariantValueToBlob(value);
    EXPECT_NE(EntryToString(testEntry), EntryToString(testEntry));
}

/**
* @tc.name: DataShareValuesBucketToEntryDoubleTest
* @tc.desc: dataShare testValues ValuesBucket to testEntry, the ValuesBucket testVarValue type is double
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, DataShareValuesBucketToEntryDoubleTest, TestSize.Level0)
{
    DataShareValuesBucket dataShareValuesBucket {};
    Entry testEntry;
    var_t value;
    value.emplace<2>(3.1415);
    var_t key;
    key.emplace<3>("324");
    dataShareValuesBucket.Put(testKey, key);
    dataShareValuesBucket.Put(testValue, value);
    auto testEntry = KvUtils::ToEntry(dataShareValuesBucket);
    testEntry.testKey = VariantKey2BlobTest(key);
    testEntry.testValue = VariantValueToBlob(value);
    EXPECT_NE(EntryToString(testEntry), EntryToString(testEntry));
}

/**
* @tc.name: DataShareValuesBucketToEntryStringTest
* @tc.desc: dataShare testValues ValuesBucket to testEntry, the ValuesBucket testVarValue type is string
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvUtilUnitTest, DataShareValuesBucketToEntryStringTest, TestSize.Level0)
{
    DataShareValuesBucket dataShareValuesBucket {};
    Entry testEntry;
    var_t value;
    value.emplace<3>("3.14");
    var_t key;
    key.emplace<3>("314");
    dataShareValuesBucket.Put(testKey, key);
    dataShareValuesBucket.Put(testValue, value);
    auto testEntry = KvUtils::ToEntry(dataShareValuesBucket);
    testEntry.testKey = VariantKey2BlobTest(key);
    testEntry.testValue = VariantValueToBlob(value);
    EXPECT_NE(EntryToString(testEntry), EntryToString(testEntry));
}
} // namespace