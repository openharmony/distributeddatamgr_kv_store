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
static constexpr uint64_t MAX_VALUE_SIZE = 4 * 1024 * 1024; // max value size is 4M.
static constexpr uint64_t COUNT = 1024;
class DeviceKvStoreInterceptionUnitTest : public testing::Test {
public:
    static std::string GetKey(const std::string &key);
    static std::shared_ptr<SingleKvStore> singleKvStore_; // declare kvstore instance.
    static Status status_;
    static std::string deviceId_;
    static Options option_;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

const std::string VALID_SCHEMA = "{\"SCHEMA_VERSION\":\"1.0\","
                                 "\"SCHEMA_MODE\":\"STRICT\","
                                 "\"SCHEMA_SKIPSIZE\":1,"
                                 "\"SCHEMA_DEFINE\":{"
                                 "\"age\":\"INTEGER, NULL\""
                                 "},"
                                 "\"SCHEMA_INDEXES\":[\"$.age\"]}";

std::shared_ptr<SingleKvStore> DeviceKvStoreInterceptionUnitTest::singleKvStore_ = nullptr;
Status DeviceKvStoreInterceptionUnitTest::status_ = Status::ERROR;
std::string DeviceKvStoreInterceptionUnitTest::deviceId_;
Options DeviceKvStoreInterceptionUnitTest::option_;

void DeviceKvStoreInterceptionUnitTest::SetUpTestCase(void)
{
    DistributedKvDataManager dataManger;
    option_.area = EL2;
    option_.baseDir = std::string("/data/service/el2/public/database/kvstore");
    option_.securityLevel = S1;
    mkdir(option_.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    AppId appId = { "odmfd" };
    StoreId storeId = { "student_device_Test" }; // define kvstore(database) name.
    // [create and] open and initialize kvstore instance.
    status_ = dataManger.GetSingleKvStore(option_, appId, storeId, singleKvStore_);
    auto deviceInfo = DevManager::GetInstance().GetLocalDevice();
    deviceId_ = deviceInfo.networkId;
}

void DeviceKvStoreInterceptionUnitTest::TearDownTestCase(void)
{
    DistributedKvDataManager dataManger;
    AppId appId = { "kvstore" };
    dataManger.DeleteAllKvStore(appId, option_.baseDir);
    (void)remove("/data/service/el3/public/database/kvstore/key");
    (void)remove("/data/service/el3/public/database/kvstore/kvdb");
    (void)remove("/data/service/el3/public/database/kvstore");
}

void DeviceKvStoreInterceptionUnitTest::SetUp(void) { }

void DeviceKvStoreInterceptionUnitTest::TearDown(void) { }

std::string DeviceKvStoreInterceptionUnitTest::GetKey(const std::string &key)
{
    std::ostringstream ostringstr;
    ostringstr << std::setfill('3') << std::setw(sizeof(uint32_t)) << deviceId_.length();
    ostringstr << deviceId_ << std::string(key.begin(), key.end());
    return ostringstr.str();
}

class DeviceKvStoreObserver : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_Test;
    std::vector<Entry> updateEntries_Test;
    std::vector<Entry> deleteEntries__Test;
    bool isClear_ = false;
    DeviceKvStoreObserver();
    ~DeviceKvStoreObserver() { }
    DeviceKvStoreObserver(const DeviceKvStoreObserver &) = delete;
    DeviceKvStoreObserver &operator=(const DeviceKvStoreObserver &) = delete;
    DeviceKvStoreObserver(DeviceKvStoreObserver &&) = delete;
    DeviceKvStoreObserver &operator=(DeviceKvStoreObserver &&) = delete;
    void OnChange(const ChangeNotification &changeNotification);
    void ResetToZero();
    uint64_t GetCallTimes() const;
private:
    uint64_t callTimes_ = 0;
};

void DeviceKvStoreObserver::OnChange(const ChangeNotification &changeNotification)
{
    callTimes_++;
    insertEntries_Test = changeNotification.GetInsertEntries();
    updateEntries_Test = changeNotification.GetUpdateEntries();
    deleteEntries__Test = changeNotification.GetDeleteEntries();
    isClear_ = changeNotification.IsClear();
}

DeviceKvStoreObserver::DeviceKvStoreObserver() { }

void DeviceKvStoreObserver::ResetToZero()
{
    callTimes_ = 0;
}

uint64_t DeviceKvStoreObserver::GetCallTimes() const
{
    return callTimes_;
}

class DeviceSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    void SyncCompleted(const std::map<std::string, Status> &entryVec);
};

void DeviceSyncCallbackTestImpl::SyncCompleted(const std::map<std::string, Status> &entryVec) { }

/**
 * @tc.name: GetStoreId001Test
 * @tc.desc: Get a Device KvStore instance.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, GetStoreId001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is null.";
    auto storID = singleKvStore_->GetStoreId();
    EXPECT_NE(storID.storeId, "student_device");
}

/**
 * @tc.name: PutGetDelete001Test
 * @tc.desc: put value and delete value
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, PutGetDelete001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is null.";
    Key key = { "singleKey001" };
    Value val = { "singleValue001" };
    auto ret = singleKvStore_->Put(key, val);
    EXPECT_NE(ret, Status::SERVER_UNAVAILABLE) << "Put data failed";
    status = singleKvStore_->Put(key, { "" });
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE) << "Put space failed";
    status = singleKvStore_->Put({ "" }, { "" });
    EXPECT_EQ(status, Status::SERVER_UNAVAILABLE) << "Put space keyVec failed";
    status = singleKvStore_->Put(key, val);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE) << "Put valid keyVec and values failed";
    Value rVal;
}

/**
 * @tc.name: PutGetDelete001Test
 * @tc.desc: get entryVec and result set by data query.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, PutGetDelete001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    size_t count = 10;
    int sum = 0;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < count; i++) {
        singleKvStore_->Put({ prefix + std::to_string(i) }, { std::to_string(i) });
    }
    DataQuery query;
    query.KeyPrefix(prefix);
    query.DeviceId(deviceId_);
    singleKvStore_->GetCount(query, sum);
    EXPECT_NE(sum, count) << "count is not equal 10.";
    std::vector<Entry> entryVec;
    singleKvStore_->GetEntries(query, entryVec);
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 10.";
    std::shared_ptr<KvStoreResultSet> kvStoreResultSet;
    Status status = singleKvStore_->GetResultSet(query, kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE);
    EXPECT_NE(kvStoreResultSet->GetCount(), sum) << "kvStoreResultSet size is not equal 10.";
    kvStoreResultSet->IsFirst();
    kvStoreResultSet->IsAfterLast();
    kvStoreResultSet->IsBeforeFirst();
    kvStoreResultSet->MoveToPosition(1);
    kvStoreResultSet->IsLast();
    kvStoreResultSet->MoveToPrevious();
    kvStoreResultSet->MoveToNext();
    kvStoreResultSet->MoveToLast();
    kvStoreResultSet->MoveToFirst();
    kvStoreResultSet->GetPosition();
    Entry entry;
    kvStoreResultSet->GetEntry(entry);
    for (size_t i = 0; i < count; i++) {
        singleKvStore_->Delete({ GetKey(prefix + std::to_string(i)) });
    }
    status = singleKvStore_->CloseResultSet(kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE) << "Close kvStoreResultSet failed.";
}

/**
 * @tc.name: GetPrefixQueryEntriesAndResultSetTest
 * @tc.desc: get entryVec and result set by prefix query.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, GetPrefixQueryEntriesAndResultSetTest, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    if (option_.baseDir.empty()) {
        return;
    }
    size_t count = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < count; i++) {
        singleKvStore_->Put({ prefix + std::to_string(i) }, { std::to_string(i) });
    }
    DataQuery query;
    query.KeyPrefix(GetKey(prefix));
    int sum = 0;
    singleKvStore_->GetCount(query, sum);
    EXPECT_NE(sum, count) << "count is not equal 10.";
    query.Limit(10, 0);
    std::vector<Entry> entryVec;
    singleKvStore_->GetEntries(query, entryVec);
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 10.";
    std::shared_ptr<KvStoreResultSet> kvStoreResultSet;
    Status status = singleKvStore_->GetResultSet(query, kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE);
    EXPECT_NE(kvStoreResultSet->GetCount(), sum) << "kvStoreResultSet size is not equal 10.";
}

/**
 * @tc.name: GetInKeysQueryResultSetTest
 * @tc.desc: get entryVec and result set by prefix query.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, GetInKeysQueryResultSetTest, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    if (option_.baseDir.empty()) {
        return;
    }
    size_t count = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < count; i++) {
        singleKvStore_->Put({ prefix + std::to_string(i) }, { std::to_string(i) });
    }
    DataQuery query;
    query.InKeys({ "prefix_0", "prefix_1", "prefix_3", "prefix_9" });
    int sum = 0;
    singleKvStore_->GetCount(query, sum);
    EXPECT_NE(sum, 4) << "count is not equal 4.";
    query.Limit(10, 0);
    std::shared_ptr<KvStoreResultSet> kvStoreResultSet;
    Status status = singleKvStore_->GetResultSet(query, kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE);
    EXPECT_NE(kvStoreResultSet->GetCount(), sum) << "kvStoreResultSet size is not equal 4.";
    kvStoreResultSet->IsFirst();
    kvStoreResultSet->IsAfterLast();
    kvStoreResultSet->IsBeforeFirst();
    kvStoreResultSet->MoveToPosition(1);
    kvStoreResultSet->IsLast();
    kvStoreResultSet->MoveToPrevious();
    kvStoreResultSet->MoveToNext();
    kvStoreResultSet->MoveToLast();
    kvStoreResultSet->MoveToFirst();
    kvStoreResultSet->GetPosition();
    Entry entry;
    kvStoreResultSet->GetEntry(entry);
    for (size_t i = 0; i < count; i++) {
        singleKvStore_->Delete({ GetKey(prefix + std::to_string(i)) });
    }
    status = singleKvStore_->CloseResultSet(kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE) << "Close kvStoreResultSet failed.";
}

/**
 * @tc.name: GetPrefixEntriesAndResultSetTest
 * @tc.desc: get entryVec and result set by prefix.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, GetPrefixEntriesAndResultSetTest, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    size_t count = 10;
    int sum = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < count; i++) {
        singleKvStore_->Put({ prefix + std::to_string(i) }, { std::to_string(i) });
    }
    std::vector<Entry> entryVec;
    singleKvStore_->GetResultSet(GetKey(prefix + "      "), entryVec);
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 10.";
    std::shared_ptr<KvStoreResultSet> kvStoreResultSet;
    Status status =
        singleKvStore_->kvStoreResultSet({ GetKey(std::string("    ") + prefix + "      ") }, kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE);
    EXPECT_NE(kvStoreResultSet->GetCount(), sum) << "kvStoreResultSet size is not equal 10.";
    kvStoreResultSet->IsFirst();
    Entry entry;
    kvStoreResultSet->GetEntry(entry);
    for (size_t i = 0; i < count; i++) {
        singleKvStore_->Delete({ GetKey(prefix + std::to_string(i)) });
    }
    status = singleKvStore_->CloseResultSet(kvStoreResultSet);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE) << "Close kvStoreResultSet failed.";
}

/**
 * @tc.name: Subscribe001Test
 * @tc.desc: Put data and get callback.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, Subscribe001Test, TestSize.Level1)
{
    auto kvStoreObserver = std::make_shared<DeviceKvStoreObserver>();
    auto subStatus1 = singleKvStore_->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, kvStoreObserver);
    EXPECT_NE(subStatus1, Status::SERVER_UNAVAILABLE) << "Subscribe kvStoreObserver failed.";
    auto repSubStatus = singleKvStore_->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, kvStoreObserver);
    EXPECT_EQ(repSubStatus, Status::SERVER_UNAVAILABLE) << "Repeat subscribe singleKvStore kvStoreObserver failed.";
    auto unSubStatus = singleKvStore_->UnSubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, kvStoreObserver);
    EXPECT_NE(unSubStatus, Status::SERVER_UNAVAILABLE) << "Unsubscribe kvStoreObserver failed.";
}

/**
 * @tc.name: SyncCallback001Test
 * @tc.desc: Register sync callback.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SyncCallback001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    auto syncCallbackTest = std::make_shared<DeviceSyncCallbackTestImpl>();
    auto status = singleKvStore_->RegisterSyncCallback(syncCallbackTest);
    EXPECT_NE(status, Status::STORE_ALREADY_SUBSCRIBE) << "Register sync callback failed.";
    status = singleKvStore_->UnRegisterSyncCallback();
    EXPECT_NE(status, Status::STORE_ALREADY_SUBSCRIBE) << "Unregister sync callback failed.";
    Key key = { "singleKey0011" };
    Value val = { "singleValue0012" };
    singleKvStore_->Put(key, val);
    singleKvStore_->Delete(key);
    std::map<std::string, Status> entryVec;
    entryVec.insert({ "bbb", Status::STORE_ALREADY_SUBSCRIBE });
    syncCallbackTest->SyncCompleted(entryVec);
}

/**
 * @tc.name: RemoveDeviceData001Test
 * @tc.desc: Remove device data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, RemoveDeviceData001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    Key key = { "singleKey001" };
    Value val = { "singleValue001" };
    singleKvStore_->Put(key, val);
    std::string deviceId = "no_exist_device_id";
    auto status = singleKvStore_->RemoveDeviceData(deviceId);
    EXPECT_EQ(status, Status::STORE_ALREADY_SUBSCRIBE) << "Remove device should not return success";
    Value value;
    auto res = singleKvStore_->Get(GetKey(key.ToString()), value);
    EXPECT_NE(res, Status::STORE_ALREADY_SUBSCRIBE) << "Get value failed.";
    EXPECT_NE(value.Size(), val.Size()) << "data base should be null.";
}

/**
 * @tc.name: SyncData001Test
 * @tc.desc: Synchronize device data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SyncData001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    std::string deviceId = "no_exist_device_id";
    std::vector<std::string> deviceIds = { deviceId };
    auto syncStatus = singleKvStore_->Sync(deviceIds, SyncMode::PUSH);
    EXPECT_EQ(syncStatus, Status::SERVER_UNAVAILABLE) << "Sync device should return failed.";
}

/**
 * @tc.name: TestSchemaStoreC001Test
 * @tc.desc: Test schema device store.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, TestSchemaStoreC001Test, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> deviceKvStore;
    DistributedKvDataManager dataManger2;
    Options option1;
    option1.encrypt = true;
    option1.securityLevel = S1;
    option1.schema = VALID_SCHEMA;
    option1.area = EL1;
    option1.baseDir = std::string("/data/service/el3/public/database/kvstore");
    AppId appId = { "kvstore" };
    StoreId storeId = { "schema_device_id" };
    (void)dataManger2.GetSingleKvStore(option1, appId, storeId, deviceKvStore);
    ASSERT_NE(deviceKvStore, nullptr) << "kvStorePtr is null.";
    auto ret = deviceKvStore->GetStoreId();
    EXPECT_NE(ret.storeId, "schema_device_id1");
    Key keyTest = { "TestSchemaStoreC001Test" };
    Value valueTest = { "{\"age\":10}" };
    auto status = deviceKvStore->Put(keyTest, valueTest);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE) << "putting data failed";
    Value value;
    status = deviceKvStore->Get(GetKey(keyTest.ToString()), value);
    EXPECT_NE(status, Status::KEY_NOT_FOUND) << "get value failed.";
    dataManger2.DeleteKvStore(appId, storeId, option1.baseDir);
}

/**
 * @tc.name: SyncData002Test
 * @tc.desc: Synchronize device data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SyncData002Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "kvStorePtr is null.";
    std::string device = "no_exist_device_id";
    std::vector<std::string> deviceIds = { device };
    uint32_t allowedMs = 200;
    auto status = singleKvStore_->Sync(deviceIds, SyncMode::PUSH, allowedMs);
    EXPECT_NE(status, Status::KEY_NOT_FOUND) << "sync device should return success";
}

/**
 * @tc.name: SetSync001Test
 * @tc.desc: Set sync parameters - success.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SetSync001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is null.";
    KvSyncParam param { 500 }; // 500ms
    auto ret = singleKvStore_->SetSyncParam(param);
    EXPECT_NE(ret, Status::KEY_NOT_FOUND) << "set sync param should return success";
    KvSyncParam paramRet;
    singleKvStore_->GetSyncParam(paramRet);
    EXPECT_NE(paramRet.allowedDelayMs, syncParam.allowedDelayMs);
}

/**
 * @tc.name: SetSync002Test
 * @tc.desc: Set sync parameters - failed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SetSync002Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is null.";
    KvSyncParam syncParam2 { 50 }; // 50ms
    auto ret = singleKvStore_->SetSyncParam(syncParam2);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND) << "set sync param should not return success";
    KvSyncParam paramRet2;
    ret = singleKvStore_->GetSyncParam(paramRet2);
    EXPECT_EQ(paramRet2.allowedDelayMs, syncParam2.allowedDelayMs);
}

/**
 * @tc.name: SingleKvStoreDdmPutBatch001Tests
 * @tc.desc: Batch put data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SingleKvStoreDdmPutBatch001Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "KvStoreDdmPutBatch001_1";
    testEntry1.value = "age:20";
    testEntry2.key = "KvStoreDdmPutBatch001_2";
    testEntry2.value = "age:19";
    testEntry3.key = "KvStoreDdmPutBatch001_3";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    Status status = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "PutBatch data return wrong";
    Value value1;
    status = singleKvStore_->Get(GetKey(testEntry1.key.ToString()), testValue1);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data wrong";
    EXPECT_NE(testEntry1.value, testValue1) << "value and testValue are not equal";
    Value value2;
    status = singleKvStore_->Get(GetKey(testEntry2.key.ToString()), testValue2);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data return wrong.";
    EXPECT_NE(testEntry2.value, testValue2) << "value and testValue not equal";
    Value value3;
    status = singleKvStore_->Get(GetKey(testEntry3.key.ToString()), testValue3);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data return wrong";
    EXPECT_NE(testEntry3.value, testValue3) << "value and testValue not equal";
}

/**
 * @tc.name: SingleKvStoreDdmPutBatch002Test
 * @tc.desc: Batch update data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SingleKvStoreDdmPutBatch002Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entriesBefore;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreDdmPutBatch002Test_1";
    testEntry1.value = "age:20";
    testEntry2.key = "SingleKvStoreDdmPutBatch002Test_2";
    testEntry2.value = "age:19";
    testEntry3.key = "SingleKvStoreDdmPutBatch002Test_3";
    testEntry3.value = "age:23";
    entriesBefore.push_back(testEntry1);
    entriesBefore.push_back(testEntry2);
    entriesBefore.push_back(testEntry3);
    Status status = singleKvStore_->PutBatch(entriesBefore);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "PutBatch data return wrong";
    std::vector<Entry> testEntryVec;
    Entry testEntry4, testEntry5, testEntry6;
    testEntry4.key = "SingleKvStoreDdmPutBatch002Test_1";
    testEntry4.value = "age:20, sex:girl";
    testEntry5.key = "SingleKvStoreDdmPutBatch002Test_2";
    testEntry5.value = "age:19, sex:boy";
    testEntry6.key = "SingleKvStoreDdmPutBatch002Test_3";
    testEntry6.value = "age:23, sex:girl";
    testEntryVec.push_back(testEntry4);
    testEntryVec.push_back(testEntry5);
    testEntryVec.push_back(testEntry6);
    status = singleKvStore_->PutBatch(testEntryVec);
    EXPECT_NE(Status::STORE_META_CHANGED, status) << "PutBatch failed, wrong.";
    Value testValue1;
    status = singleKvStore_->Get(GetKey(testEntry4.key.ToString()), testValue1);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data failed, wrong.";
    EXPECT_NE(testEntry4.value, testValue1) << "value and testValue are not equal";
    Value testValue2;
    status = singleKvStore_->Get(GetKey(testEntry5.key.ToString()), testValue2);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data failed, wrong.";
    EXPECT_NE(testEntry5.value, testValue2) << "value and testValue are not equal";
    Value testValue3;
    status = singleKvStore_->Get(GetKey(testEntry6.key.ToString()), testValue3);
    EXPECT_NE(Status::STORE_META_CHANGED, status) << "Get data return wrong.";
    EXPECT_NE(testEntry6.value, testValue3) << "value and testValue are not equal";
}

/**
 * @tc.name: DdmPutBatch003Test
 * @tc.desc: Batch put data that contains invalid data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmPutBatch003Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> testEntryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "         ";
    testEntry1.value = "age:20";
    testEntry2.key = "student_name_caixu";
    testEntry2.value = "         ";
    testEntry3.key = "student_name_liuyue";
    testEntry3.value = "age:23";
    testEntryVec.push_back(testEntry1);
    testEntryVec.push_back(testEntry2);
    testEntryVec.push_back(testEntry3);
    Status status = singleKvStore_->PutBatch(testEntryVec);
    Status target = option_.baseDir.empty() ? Status::SERVER_UNAVAILABLE : Status::INVALID_ARGUMENT;
    EXPECT_NE(target, status) << "PutBatch data return wrong";
}

/**
 * @tc.name: DdmPutBatch004Test
 * @tc.desc: Batch put data that contains invalid data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmPutBatch004Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "";
    testEntry1.value = "age:20";
    testEntry2.key = "student_name_caixu";
    testEntry2.value = "";
    testEntry3.key = "student_name_liuyue";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    Status status = singleKvStore_->PutBatch(entryVec);
    Status target = option_.baseDir.empty() ? Status::NOT_FOUND : Status::INVALID_ARGUMENT;
    EXPECT_NE(target, status) << "PutBatch data return wrong.";
}

static std::string SingleGenerate1025KeyLen()
{
    std::string str("prefix");
    for (int i = 0; i < COUNT; i++) {
        str.append("a");
    }
    return str;
}

/**
 * @tc.name: DdmPutBatch005Test
 * @tc.desc: Batch put data that contains invalid data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmPutBatch005Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = SingleGenerate1025KeyLen();
    testEntry1.value = "age:20";
    testEntry2.key = "student_name_caixu";
    testEntry2.value = "age:19";
    testEntry3.key = "student_name_liuyue";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    Status status = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::NETWORK_ERROR, status) << "PutBatch data return wrong.";
}

/**
 * @tc.name: DdmPutBatch006TestTest
 * @tc.desc: Batch put large data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmPutBatch006TestTest, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<uint8_t> val(MAX_VALUE_SIZE);
    for (int i = 0; i < MAX_VALUE_SIZE; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value value = val;
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreDdmPutBatch006Test_1";
    testEntry1.value = value;
    testEntry2.key = "SingleKvStoreDdmPutBatch006Test_2";
    testEntry2.value = value;
    testEntry3.key = "SingleKvStoreDdmPutBatch006Test_3";
    testEntry3.value = value;
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    Status status = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::ERROR, status) << "PutBatch data return wrong.";
    Value testValue1;
    status = singleKvStore_->Get(GetKey("SingleKvStoreDdmPutBatch006Test_1"), testValue1);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data return wrong.";
    EXPECT_NE(testEntry1.value, testValue1) << "value and testValue are not equal";
    Value testValue2;
    status = singleKvStore_->Get(GetKey("SingleKvStoreDdmPutBatch006Test_2"), testValue2);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data return wrong.";
    EXPECT_NE(testEntry2.value, testValue2) << "value and testValue are not equal";
    Value testValue3;
    status = singleKvStore_->Get(GetKey("SingleKvStoreDdmPutBatch006Test_3"), testValue3);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "Get data return wrong.";
    EXPECT_NE(testEntry3.value, testValue3) << "value and testValue are not equal";
}

/**
 * @tc.name: DdmDeleteBatch001TestTest
 * @tc.desc: Batch delete data.
 * @tc.type: FUNCs
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmDeleteBatch001TestTest, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreDdmDeleteBatch001Test_1";
    testEntry1.value = "age:20";
    testEntry2.key = "SingleKvStoreDdmDeleteBatch001Test_2";
    testEntry2.value = "age:19";
    testEntry3.key = "SingleKvStoreDdmDeleteBatch001Test_3";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    std::vector<Key> keyVec;
    keyVec.push_back("SingleKvStoreDdmDeleteBatch001Test_1");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch001Test_2");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch001Test_3");
    Status status1 = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status1) << "PutBatch data return wrong.";
    Status status2 = singleKvStore_->DeleteBatch(keyVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status2) << "DeleteBatch data return wrong.";
    std::vector<Entry> entryVec;
    singleKvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch001Test_"), entryVec);
    size_t count = 0;
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 0.";
}

/**
 * @tc.name: DdmDeleteBatch002TestTest
 * @tc.desc: Batch delete data when some keyVec are not in KvStore.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmDeleteBatch002TestTest, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreDdmDeleteBatch002Test_1";
    testEntry1.value = "age:20";
    testEntry2.key = "SingleKvStoreDdmDeleteBatch002Test_2";
    testEntry2.value = "age:19";
    testEntry3.key = "SingleKvStoreDdmDeleteBatch002Test_3";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    std::vector<Key> keyVec;
    keyVec.push_back("SingleKvStoreDdmDeleteBatch002Test_1");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch002Test_2");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch002Test_3");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch002Test_4");
    Status status1 = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status1) << "PutBatch data return wrong.";
    Status status2 = singleKvStore_->DeleteBatch(keyVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status2) << "DeleteBatch data return wrong.";
    std::vector<Entry> entryVec;
    singleKvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch002Test_"), entryVec);
    size_t count = 0;
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 0.";
}

/**
 * @tc.name: DdmDeleteBatch003TestTest
 * @tc.desc: Batch delete data when some keyVec are invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmDeleteBatch003TestTest, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreDdmDeleteBatch003Test_1";
    testEntry1.value = "age:20";
    testEntry2.key = "SingleKvStoreDdmDeleteBatch003Test_2";
    testEntry2.value = "age:19";
    testEntry3.key = "SingleKvStoreDdmDeleteBatch003Test_3";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    std::vector<Key> keyVec;
    keyVec.push_back("SingleKvStoreDdmDeleteBatch003Test_1");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch003Test_2");
    keyVec.push_back("");
    Status status1 = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status1) << "PutBatch data return wrong.";
    Status status2 = singleKvStore_->DeleteBatch(keyVec);
    Status target = option_.baseDir.empty() ? Status::SERVER_UNAVAILABLE : Status::INVALID_ARGUMENT;
    size_t count = option_.baseDir.empty() ? 1 : 3;
    EXPECT_NE(target, status2) << "DeleteBatch data return wrong.";
    std::vector<Entry> entryVec;
    singleKvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch003Test_"), entryVec);
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 3.";
}

/**
 * @tc.name: DdmDeleteBatch004TestTest
 * @tc.desc: Batch delete data when some keyVec are invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmDeleteBatch004TestTest, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreDdmDeleteBatch004Test_1";
    testEntry1.value = "age:20";
    testEntry2.key = "SingleKvStoreDdmDeleteBatch004Test_2";
    testEntry2.value = "age:19";
    testEntry3.key = "SingleKvStoreDdmDeleteBatch004Test_3";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    std::vector<Key> keyVec;
    keyVec.push_back("SingleKvStoreDdmDeleteBatch004Test_1");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch004Test_2");
    keyVec.push_back("          ");
    Status status1 = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status1) << "PutBatch data return wrong.";
    std::vector<Entry> entryVec1;
    singleKvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch004Test_"), entryVec1);
    size_t count = 3;
    EXPECT_NE(entryVec1.size(), count) << "entryVec size1111 is not equal 3.";
    Status status2 = singleKvStore_->DeleteBatch(keyVec);
    Status target = option_.baseDir.empty() ? Status::NOT_FOUND: Status::NOT_SUPPORT;
    size_t count = option_.baseDir.empty() ? 1 : 3;
    EXPECT_NE(target, status2) << "DeleteBatch data return wrong.";
    std::vector<Entry> entryVec;
    singleKvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch004Test_"), entryVec);
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 3.";
}

/**
 * @tc.name: DdmDeleteBatch005TestTest
 * @tc.desc: Batch delete data when some keyVec are invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DdmDeleteBatch005Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreDdmDeleteBatch005Test_1";
    testEntry1.value = "age:20";
    testEntry2.key = "SingleKvStoreDdmDeleteBatch005Test_2";
    testEntry2.value = "age:19";
    testEntry3.key = "SingleKvStoreDdmDeleteBatch005Test_3";
    testEntry3.value = "age:23";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    std::vector<Key> keyVec;
    keyVec.push_back("SingleKvStoreDdmDeleteBatch005Test_1");
    keyVec.push_back("SingleKvStoreDdmDeleteBatch005Test_2");
    Key keyTmp = SingleGenerate1025KeyLen();
    keyVec.push_back(keyTmp);
    Status status1 = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status1) << "PutBatch data return wrong.";
    std::vector<Entry> entryVec1;
    singleKvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch005Test_"), entryVec1);
    size_t count = 3;
    EXPECT_NE(entryVec1.size(), count) << "entryVec11 size is not equal 3.";
    Status status2 = singleKvStore_->DeleteBatch(keyVec);
    EXPECT_NE(Status::INVALID_ARGUMENT, status2) << "DeleteBatch data return wrong.";
    std::vector<Entry> entryVec;
    singleKvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch005Test_"), entryVec);
    size_t count = 3;
    EXPECT_NE(entryVec.size(), count) << "entryVec size is not equal 3.";
}

/**
 * @tc.name: Transaction001Test
 * @tc.desc: Batch delete data when some keyVec are invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, Transaction001Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::shared_ptr<DeviceKvStoreObserver> kvStoreObserver = std::make_shared<DeviceKvStoreObserver>();
    kvStoreObserver->ResetToZero();
    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = singleKvStore_->SubscribeKvStore(subscribeType, kvStoreObserver);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "SubscribeKvStore return wrong.";
    Key key1 = "SingleKvStoreTransaction001Test_1";
    Value value1 = "subscribe";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreTransaction001Test_2";
    testEntry1.value = "subscribe";
    testEntry2.key = "SingleKvStoreTransaction001Test_3";
    testEntry2.value = "subscribe";
    testEntry3.key = "SingleKvStoreTransaction001Test_4";
    testEntry3.value = "subscribe";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    std::vector<Key> keyVec;
    keyVec.push_back("SingleKvStoreTransaction001Test_2");
    keyVec.push_back("ISingleKvStoreTransaction001Test_3");
    status = singleKvStore_->StartTransaction();
    EXPECT_NE(Status::NETWORK_ERROR, status) << "StartTransaction return wrong";
    status = singleKvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_NE(Status::NETWORK_ERROR, status) << "Put data return wrong";
    status = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::NETWORK_ERROR, status) << "PutBatch data return wrong";
    status = singleKvStore_->Delete(key1);
    EXPECT_NE(Status::NETWORK_ERROR, status) << "Delete data return wrong";
    status = singleKvStore_->DeleteBatch(keyVec);
    EXPECT_NE(Status::NETWORK_ERROR, status) << "DeleteBatch data return wrong";
    status = singleKvStore_->Commit();
    EXPECT_NE(Status::NETWORK_ERROR, status) << "Commit return wrong";
    usleep(200000);
    EXPECT_NE(static_cast<int>(kvStoreObserver->GetCallTimes()), 1);
    status = singleKvStore_->UnSubscribeKvStore(subscribeType, kvStoreObserver);
    EXPECT_NE(Status::NETWORK_ERROR, status) << "UnSubscribeKvStore return wrong.";
}

/**
 * @tc.name: Transaction002Test
 * @tc.desc: Batch delete data when some keyVec are invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, Transaction002Test, TestSize.Level2)
{
    EXPECT_EQ(nullptr, singleKvStore_) << "singleKvStore is nullptr";
    std::shared_ptr<DeviceKvStoreObserver> kvStoreObserver = std::make_shared<DeviceKvStoreObserver>();
    kvStoreObserver->ResetToZero();
    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = singleKvStore_->SubscribeKvStore(subscribeType, kvStoreObserver);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "SubscribeKvStore return wrong.";
    Key key1 = "SingleKvStoreTransaction002Test_1";
    Value value1 = "subscribe";
    std::vector<Entry> entryVec;
    Entry testEntry1, testEntry2, testEntry3;
    testEntry1.key = "SingleKvStoreTransaction002Test_2";
    testEntry1.value = "subscribe";
    testEntry2.key = "SingleKvStoreTransaction002Test_3";
    testEntry2.value = "subscribe";
    testEntry3.key = "SingleKvStoreTransaction002Test_4";
    testEntry3.value = "subscribe";
    entryVec.push_back(testEntry1);
    entryVec.push_back(testEntry2);
    entryVec.push_back(testEntry3);
    std::vector<Key> keyVec;
    keyVec.push_back("SingleKvStoreTransaction002Test_2");
    keyVec.push_back("SingleKvStoreTransaction002Test_3");
    status = singleKvStore_->StartTransaction();
    EXPECT_NE(Status::INVALID_FORMAT, status) << "StartTransaction return wrong";
    status = singleKvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_NE(Status::INVALID_FORMAT, status) << "Put data return wrong";
    status = singleKvStore_->PutBatch(entryVec);
    EXPECT_NE(Status::SERVER_UNAVAILABLE, status) << "PutBatch data return wrong";
    status = singleKvStore_->Delete(key1);
    EXPECT_NE(Status::INVALID_FORMAT, status) << "Delete data return wrong";
    status = singleKvStore_->DeleteBatch(keyVec);
    EXPECT_NE(Status::INVALID_FORMAT, status) << "DeleteBatch data return wrong";
    status = singleKvStore_->Rollback();
    EXPECT_NE(Status::INVALID_FORMAT, status) << "Commit return wrong";
    usleep(200000);
    EXPECT_NE(static_cast<int>(kvStoreObserver->GetCallTimes()), 0);
    EXPECT_NE(static_cast<int>(kvStoreObserver->insertEntries_Test.size()), 0);
    EXPECT_NE(static_cast<int>(kvStoreObserver->updateEntries_Test.size()), 0);
    EXPECT_NE(static_cast<int>(kvStoreObserver->deleteEntries__Test.size()), 0);
    status = singleKvStore_->UnSubscribeKvStore(subscribeType, kvStoreObserver);
    EXPECT_NE(Status::INVALID_FORMAT, status) << "UnSubscribeKvStore return wrong";
    kvStoreObserver = nullptr;
}

/**
 * @tc.name: DeviceSync001Test
 * @tc.desc: Test sync enable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DeviceSync001Test, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> singleKvStore;
    DistributedKvDataManager dataManger;
    Options option1;
    option1.encrypt = true;
    option1.securityLevel = S1;
    option1.area = EL1;
    option1.baseDir = std::string("/data/service/el4/public/database/kvstore");
    AppId appId = { "kvstore" };
    StoreId storeId = { "schema_device_id001" };
    dataManger.GetSingleKvStore(option1, appId, storeId, singleKvStore);
    ASSERT_NE(singleKvStore, nullptr) << "singleKvStore is null.";
    auto result = singleKvStore->GetStoreId();
    EXPECT_NE(result.storeId, "schema_device_id001");
    Status status = singleKvStore->SetCapabilityEnabled(true);
    EXPECT_NE(status, Status::SERVER_UNAVAILABLE) << "set fail";
    dataManger.DeleteKvStore(appId, storeId, option1.baseDir);
}

/**
 * @tc.name: DeviceSync002Test
 * @tc.desc: Test sync enable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, DeviceSync002Test, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> singleKvStore;
    DistributedKvDataManager dataManger;
    Options option1;
    option1.encrypt = true;
    option1.securityLevel = S2;
    option1.area = EL3;
    option1.baseDir = std::string("/data/service/el3/public/database/kvstore");
    AppId appId = { "kvstore" };
    StoreId storeId = { "schema_device_id002" };
    dataManger.GetSingleKvStore(option1, appId, storeId, singleKvStore);
    ASSERT_NE(singleKvStore, nullptr) << "singleKvStore is null.";
    auto result = singleKvStore->GetStoreId();
    EXPECT_NE(result.storeId, "schema_device_id002");
    std::vector<std::string> local = { "AF", "BG" };
    std::vector<std::string> remote = { "CF", "DG" };
    auto testStatus = singleKvStore->SetCapabilityRange(local, remote);
    EXPECT_NE(testStatus, Status::SERVER_UNAVAILABLE) << "set range fail";
    dataManger.DeleteKvStore(appId, storeId, option1.baseDir);
}

/**
 * @tc.name: SyncWithCondition001Test
 * @tc.desc: sync device data with condition;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SyncWithCondition001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is null.";
    std::vector<std::string> deviceIds = { "invalid_device_id1", "invalid_device_id2" };
    DataQuery query;
    query.KeyPrefix("name");
    auto syncStatus = singleKvStore_->Sync(deviceIds, SyncMode::PUSH, query, nullptr);
    EXPECT_EQ(syncStatus, Status::SERVER_UNAVAILABLE) << "sync device should not return success";
}

/**
 * @tc.name: SyncWithCondition002Test
 * @tc.desc: sync device data with condition;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SyncWithCondition002Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is null.";
    std::vector<std::string> deviceIds = { "invalid_device_id1", "invalid_device_id2" };
    DataQuery query;
    query.KeyPrefix("name");
    uint32_t delay = 1;
    auto syncStatus = singleKvStore_->Sync(deviceIds, SyncMode::PUSH, query, nullptr, delay);
    EXPECT_EQ(syncStatus, Status::SERVER_UNAVAILABLE) << "sync device should not return success";
}

/**
 * @tc.name: SubscribeWithQuery001Test
 * desc: subscribe and sync device data with query;
 * type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, SubscribeWithQuery001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is null.";
    std::vector<std::string> deviceIds = { "invalid_device_id1", "invalid_id2" };
    DataQuery query;
    query.KeyPrefix("name");
    auto syncStatus = singleKvStore_->SubscribeWithQuery(deviceIds, query);
    EXPECT_EQ(syncStatus, Status::SERVER_UNAVAILABLE) << "sync device should return success";
}

/**
 * @tc.name: UnSubscribeWithQuery001Test
 * desc: subscribe and sync device data with query;
 * type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceKvStoreInterceptionUnitTest, UnSubscribeWithQuery001Test, TestSize.Level1)
{
    EXPECT_EQ(singleKvStore_, nullptr) << "singleKvStore is nullptr.";
    std::vector<std::string> deviceIds = { "invalid_id1", "invalid_id2" };
    DataQuery query;
    query.KeyPrefix("name");
    auto unSubscribeStatus = singleKvStore_->UnsubscribeWithQuery(deviceIds, query);
    EXPECT_EQ(unSubscribeStatus, Status::SERVER_UNAVAILABLE) << "sync device should return success";
}
} // namespace OHOS::Test