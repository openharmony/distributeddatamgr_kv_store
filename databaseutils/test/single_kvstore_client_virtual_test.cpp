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
#define LOG_TAG "SingleKvStoreClientVirtualTest"

#include "change_notification.h"
#include "distributed_kv_data_manager.h"
#include "file_ex.h"
#include "iremote_broker.h"
#include "iremote_object.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "itypes_util.h"
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <unistd.h>
#include <variant>
#include <vector>

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
static constexpr uint64_t MAX_VALUE_SIZE = 4 * 1024 * 1024; // max valVirtualue size is 4M.
class SingleKvStoreClientVirtualTest : public testing::Test {
public:
    static std::shared_ptr<SingleKvStore> singleKvStoreVirtual; // declare kvstore instance.
    static Status status_;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

const std::string VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
                                               "\"SCHEMA_MODE\":\"STRICT\","
                                               "\"SCHEMA_SKIPSIZE\":0,"
                                               "\"SCHEMA_DEFINE\":{"
                                               "\"age\":\"INTEGER, NOT NULL\""
                                               "},"
                                               "\"SCHEMA_INDEXES\":[\"$.age\"]}";

std::shared_ptr<SingleKvStore> SingleKvStoreClientVirtualTest::singleKvStoreVirtual = nullptr;
Status SingleKvStoreClientVirtualTest::status_ = Status::ERROR;

void SingleKvStoreClientVirtualTest::SetUpTestCase(void)
{
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual = {
        .createIfMissing = true,
        .encrypt = false,
        .autoSync = true,
        .kvStoreType = KvStoreType::SINGLE_VERSION
    };
    optionsVirtual.area = EL1;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "student_single" }; // define kvstore(database) name.
    mkdir(optionsVirtual.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    // [create and] open and initialize kvstore instance.
    status_ =
        managerVirtual.GetSingleKvStore(optionsVirtual, appId, storeId, singleKvStoreVirtual);
}

void SingleKvStoreClientVirtualTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/odmf/keyVirtual");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void SingleKvStoreClientVirtualTest::SetUp(void) { }

void SingleKvStoreClientVirtualTest::TearDown(void) { }

class KvStoreObserverTestImplVirtual : public KvStoreObserver {
public:
    std::vector<Entry> insertEntriesVirtual_;
    std::vector<Entry> updateEntriesVirtual_;
    std::vector<Entry> deleteEntriesVirtual_;
    bool isClear_ = false;
    KvStoreObserverTestImplVirtual();
    ~KvStoreObserverTestImplVirtual() { }

    KvStoreObserverTestImplVirtual(const KvStoreObserverTestImplVirtual &) = delete;
    KvStoreObserverTestImplVirtual &operator=(const KvStoreObserverTestImplVirtual &) = delete;
    KvStoreObserverTestImplVirtual(KvStoreObserverTestImplVirtual &&) = delete;
    KvStoreObserverTestImplVirtual &operator=(KvStoreObserverTestImplVirtual &&) = delete;

    void OnChange(const ChangeNotification &changeNotification);

    // reset the callCount_ to zero.
    void ResetToZero();

    uint64_t GetCallCount() const;

private:
    uint64_t callCount_ = 0;
};

void KvStoreObserverTestImplVirtual::OnChange(const ChangeNotification &changeNotification)
{
    callCount_++;
    insertEntriesVirtual_ = changeNotification.GetInsertEntries();
    updateEntriesVirtual_ = changeNotification.GetUpdateEntries();
    deleteEntriesVirtual_ = changeNotification.GetDeleteEntries();
    isClear_ = changeNotification.IsClear();
}

KvStoreObserverTestImplVirtual::KvStoreObserverTestImplVirtual() { }

void KvStoreObserverTestImplVirtual::ResetToZero()
{
    callCount_ = 0;
}

uint64_t KvStoreObserverTestImplVirtual::GetCallCount() const
{
    return callCount_;
}

class KvStoreSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    void SyncCompleted(const std::map<std::string, Status> &results);
};

void KvStoreSyncCallbackTestImpl::SyncCompleted(const std::map<std::string, Status> &results) { }

using var_t = std::variant<std::monostate, uint32_t, std::string, int32_t, uint64_t>;

class TypesUtilVirtualTest : public testing::Test {
public:
    class ITestRemoteObject : public IRemoteBroker {
    public:
        DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.ITestRemoteObject");
    };
    class TestRemoteObjectStub : public IRemoteStub<ITestRemoteObject> {
    public:
    };
    class TestRemoteObjectProxy : public IRemoteProxy<ITestRemoteObject> {
    public:
        explicit TestRemoteObjectProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<ITestRemoteObject>(impl) { }
        ~TestRemoteObjectProxy() = default;

    private:
        static inline BrokerDelegator<TestRemoteObjectProxy> delegator_;
    };
    class TestRemoteObjectClient : public TestRemoteObjectStub {
    public:
        TestRemoteObjectClient() { }
        virtual ~TestRemoteObjectClient() { }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void TypesUtilVirtualTest::SetUpTestCase(void) { }

void TypesUtilVirtualTest::TearDownTestCase(void) { }

void TypesUtilVirtualTest::SetUp(void) { }

void TypesUtilVirtualTest::TearDown(void) { }

/**
 * @tc.name: DeviceInfo
 * @tc.desc: DeviceInfo function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, DeviceInfo, TestSize.Level0)
{
    ZLOGI("DeviceInfo begin.");
    MessageParcel parcelVirtual;
    DeviceInfo clientDevVirtual;
    clientDevVirtual.deviceId = "123";
    clientDevVirtual.deviceName = "rk3568";
    clientDevVirtual.deviceType = "phone";
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtual, clientDevVirtual));
    DeviceInfo serverDevVirtual;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtual, serverDevVirtual));
    EXPECT_EQ(clientDevVirtual.deviceId, serverDevVirtual.deviceId);
    EXPECT_EQ(clientDevVirtual.deviceName, serverDevVirtual.deviceName);
    EXPECT_EQ(clientDevVirtual.deviceType, serverDevVirtual.deviceType);
}

/**
 * @tc.name: Entry
 * @tc.desc: Entry function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, Entry, TestSize.Level0)
{
    ZLOGI("Entry begin.");
    MessageParcel parcelVirtual;
    Entry entryVirtualIn;
    entryVirtualIn.key = "student_name_mali";
    entryVirtualIn.value = "age:20";
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtual, entryVirtualIn));
    Entry entryVirtualOut;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtual, entryVirtualOut));
    EXPECT_EQ(entryVirtualOut.key.ToString(), std::string("student_name_mali"));
    EXPECT_EQ(entryVirtualOut.value.ToString(), std::string("age:20"));
}

/**
 * @tc.name: ChangeNotification
 * @tc.desc: ChangeNotification function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, ChangeNotification, TestSize.Level0)
{
    ZLOGI("ChangeNotification begin.");
    Entry insert, update, del;
    insert.key = "insert";
    update.key = "update";
    del.key = "delete";
    insert.value = "insert_value";
    update.value = "update_value";
    del.value = "delete_value";
    std::vector<Entry> inserts, updates, deleteds;
    inserts.push_back(insert);
    updates.push_back(update);
    deleteds.push_back(del);

    ChangeNotification changeIn(std::move(inserts), std::move(updates), std::move(deleteds), std::string(), false);
    MessageParcel parcelVirtual;
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtual, changeIn));
    ChangeNotification changeOut({}, {}, {}, "", false);
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtual, changeOut));
    EXPECT_EQ(changeOut.GetInsertEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetInsertEntries().front().key.ToString(), std::string("insert"));
    EXPECT_EQ(changeOut.GetInsertEntries().front().value.ToString(), std::string("insert_value"));
    EXPECT_EQ(changeOut.GetUpdateEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetUpdateEntries().front().key.ToString(), std::string("update"));
    EXPECT_EQ(changeOut.GetUpdateEntries().front().value.ToString(), std::string("update_value"));
    EXPECT_EQ(changeOut.GetDeleteEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetDeleteEntries().front().key.ToString(), std::string("delete"));
    EXPECT_EQ(changeOut.GetDeleteEntries().front().value.ToString(), std::string("delete_value"));
    EXPECT_EQ(changeOut.IsClear(), false);
}

/**
 * @tc.name: Multiple
 * @tc.desc: Multiple function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, Multiple, TestSize.Level0)
{
    ZLOGI("Multiple begin.");
    uint32_t input1 = 10;
    int32_t input2 = -10;
    std::string input3 = "i test";
    Blob input4 = "input 4";
    Entry input5;
    input5.key = "my test";
    input5.value = "test value";
    DeviceInfo input6 = { .deviceId = "mock deviceId", .deviceName = "mock phone", .deviceType = "0" };
    sptr<ITestRemoteObject> input7 = new TestRemoteObjectClient();
    MessageParcel parcelVirtual;
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtual, input1, input2, input3, input4, input5, input6, input7->AsObject()));
    uint32_t outputVirtual1 = 0;
    int32_t outputVirtual2 = 0;
    std::string outputVirtual3 = "";
    Blob outputVirtual4;
    Entry outputVirtual5;
    DeviceInfo outputVirtual6;
    sptr<IRemoteObject> outputVirtual7;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtual, outputVirtual1, outputVirtual2, outputVirtual3, outputVirtual4,
        outputVirtual5, outputVirtual6, outputVirtual7));
    EXPECT_EQ(outputVirtual1, input1);
    EXPECT_EQ(outputVirtual2, input2);
    EXPECT_EQ(outputVirtual3, input3);
    EXPECT_EQ(outputVirtual4, input4);
    EXPECT_EQ(outputVirtual5.key, input5.key);
    EXPECT_EQ(outputVirtual5.value, input5.value);
    EXPECT_EQ(outputVirtual6.deviceId, input6.deviceId);
    EXPECT_EQ(outputVirtual6.deviceName, input6.deviceName);
    EXPECT_EQ(outputVirtual6.deviceType, input6.deviceType);
    EXPECT_EQ(outputVirtual7, input7->AsObject());
}

/**
 * @tc.name: Variant
 * @tc.desc: Variant function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, Variant, TestSize.Level0)
{
    ZLOGI("Variant begin.");
    MessageParcel parcelVirtualNull;
    var_t valueNullIn;
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtualNull, valueNullIn));
    var_t valueNullOut;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtualNull, valueNullOut));
    EXPECT_EQ(valueNullOut.index(), 0);

    MessageParcel parcelVirtualUint;
    var_t valueUintIn;
    valueUintIn.emplace<1>(100);
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtualUint, valueUintIn));
    var_t valueUintOut;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtualUint, valueUintOut));
    EXPECT_EQ(valueUintOut.index(), 1);
    EXPECT_EQ(std::get<uint32_t>(valueUintOut), 100);

    MessageParcel parcelVirtualString;
    var_t valueStringIn;
    valueStringIn.emplace<2>("valueString");
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtualString, valueStringIn));
    var_t valueStringOut;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtualString, valueStringOut));
    EXPECT_EQ(valueStringOut.index(), 2);
    EXPECT_EQ(std::get<std::string>(valueStringOut), "valueString");

    MessageParcel parcelVirtualInt;
    var_t valueIntIn;
    valueIntIn.emplace<3>(101);
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtualInt, valueIntIn));
    var_t valueIntOut;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtualInt, valueIntOut));
    EXPECT_EQ(valueIntOut.index(), 3);
    EXPECT_EQ(std::get<int32_t>(valueIntOut), 101);

    MessageParcel parcelVirtualUint64;
    var_t valueUint64In;
    valueUint64In.emplace<4>(110);
    EXPECT_TRUE(ITypesUtil::Marshal(parcelVirtualUint64, valueUint64In));
    var_t valueUint64Out;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcelVirtualUint64, valueUint64Out));
    EXPECT_EQ(valueUint64Out.index(), 4);
    EXPECT_EQ(std::get<uint64_t>(valueUint64Out), 110);
}

/**
 * @tc.name: MarshalToBufferLimitTest001
 * @tc.desc: MarshalToBufferLimitTest001 function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, MarshalToBufferLimitTest001, TestSize.Level0)
{
    ZLOGI("MarshalToBufferLimitTest001 begin.");
    MessageParcel parcelVirtual;
    std::vector<Entry> exceedMaxCountInput(ITypesUtil::MAX_COUNT + 1);
    EXPECT_FALSE(
        ITypesUtil::MarshalToBuffer(exceedMaxCountInput, sizeof(int) * exceedMaxCountInput.size(), parcelVirtual));
}

/**
 * @tc.name: MarshalToBufferLimitTest002
 * @tc.desc: construct a invalid vector and check MarshalToBuffer function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, MarshalToBufferLimitTest002, TestSize.Level0)
{
    ZLOGI("MarshalToBufferLimitTest002 begin.");
    MessageParcel parcelVirtual;
    std::vector<Entry> inputNormal(10);
    EXPECT_FALSE(ITypesUtil::MarshalToBuffer(inputNormal, ITypesUtil::MAX_SIZE + 1, parcelVirtual));
    EXPECT_FALSE(ITypesUtil::MarshalToBuffer(inputNormal, -1, parcelVirtual));
}

/**
 * @tc.name: UnmarshalFromBufferLimitTest001
 * @tc.desc: construct a invalid parcelVirtual and check UnmarshalFromBuffer function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, UnmarshalFromBufferLimitTest001, TestSize.Level0)
{
    ZLOGI("UnmarshalFromBufferLimitTest001 begin.");
    MessageParcel parcelVirtual;
    int32_t normalSizeVirtual = 100;
    parcelVirtual.WriteInt32(normalSizeVirtual);         // normal size
    parcelVirtual.WriteInt32(ITypesUtil::MAX_COUNT + 1); // exceed MAX_COUNT
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(normalSizeVirtual);
    parcelVirtual.WriteRawData(buffer.get(), normalSizeVirtual);

    std::vector<Entry> outputVirtual;
    EXPECT_FALSE(ITypesUtil::UnmarshalFromBuffer(parcelVirtual, outputVirtual));
    EXPECT_TRUE(outputVirtual.empty());
}

/**
 * @tc.name: UnmarshalFromBufferLimitTest002
 * @tc.desc: construct a invalid parcelVirtual and check UnmarshalFromBuffer function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(TypesUtilVirtualTest, UnmarshalFromBufferLimitTest002, TestSize.Level0)
{
    ZLOGI("UnmarshalFromBufferLimitTest002 begin.");
    MessageParcel parcelVirtual;
    parcelVirtual.WriteInt32(ITypesUtil::MAX_SIZE + 1); // exceedMaxSize size
    std::vector<Entry> outputVirtual;
    EXPECT_FALSE(ITypesUtil::UnmarshalFromBuffer(parcelVirtual, outputVirtual));
    EXPECT_TRUE(outputVirtual.empty());
}

/**
 * @tc.name: GetStoreId001
 * @tc.desc: Get a single KvStore instance.
 * @tc.type: FUNC
 * @tc.require: SR000DORPS AR000DPRQ7 AR000DDPRPL
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, GetStoreId001, TestSize.Level0)
{
    ZLOGI("GetStoreId001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";

    auto storID = singleKvStoreVirtual->GetStoreId();
    ASSERT_EQ(storID.storeId, "student_single");
}

/**
 * @tc.name: PutGetDelete001
 * @tc.desc: put valVirtualue and delete valVirtualue
 * @tc.type: FUNC
 * @tc.require: SR000DORPS AR000DPRQ7 AR000DDPRPL
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, PutGetDelete001, TestSize.Level0)
{
    ZLOGI("PutGetDelete001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";

    Key skeyVirtual = { "single_001" };
    Value svalVirtual = { "valVirtualue_001" };
    auto statusVirtual = singleKvStoreVirtual->Put(skeyVirtual, svalVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS) << "putting data failed";

    auto delStatus = singleKvStoreVirtual->Delete(skeyVirtual);
    ASSERT_EQ(delStatus, Status::SUCCESS) << "deleting data failed";

    auto notExistStatus = singleKvStoreVirtual->Delete(skeyVirtual);
    ASSERT_EQ(notExistStatus, Status::SUCCESS) << "deleting non-existing data failed";

    auto spaceStatus = singleKvStoreVirtual->Put(skeyVirtual, { "" });
    ASSERT_EQ(spaceStatus, Status::SUCCESS) << "putting space failed";

    auto spaceKeyStatus = singleKvStoreVirtual->Put({ "" }, { "" });
    ASSERT_NE(spaceKeyStatus, Status::SUCCESS) << "putting space keyVirtuals failed";

    Status valVirtualidStatus = singleKvStoreVirtual->Put(skeyVirtual, svalVirtual);
    ASSERT_EQ(valVirtualidStatus,
        Status::SUCCESS) << "putting valVirtualid keyVirtuals and valVirtualues failed";

    Value rVal;
    auto valVirtualidPutStatus = singleKvStoreVirtual->Get(skeyVirtual, rVal);
    ASSERT_EQ(valVirtualidPutStatus, Status::SUCCESS) << "Getting valVirtualue failed";
    ASSERT_EQ(svalVirtual, rVal) << "Got and put valVirtualues not equal";
}

/**
 * @tc.name: GetEntriesAndResultSet001
 * @tc.desc: Batch put valVirtualues and get valVirtualues.
 * @tc.type: FUNC
 * @tc.require: SR000DORPS AR000DPRQ7 AR000DDPRPL
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, GetEntriesAndResultSet001, TestSize.Level0)
{
    ZLOGI("GetEntriesAndResultSet001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";

    // prepare 10
    size_t sum = 10;
    int sum1 = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < sum; i++) {
        singleKvStoreVirtual->Put({ prefix + std::to_string(i) }, { std::to_string(i) });
    }

    std::vector<Entry> results;
    singleKvStoreVirtual->GetEntries({ prefix }, results);
    ASSERT_EQ(results.size(), sum) << "entries size is not equal 10.";

    std::shared_ptr<KvStoreResultSet> resultSetVirtual;
    Status statusVirtual =
        singleKvStoreVirtual->GetResultSet({ prefix }, resultSetVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_EQ(resultSetVirtual->GetCount(), sum1) << "resultSetVirtual size is not equal 10.";
    resultSetVirtual->IsFirst();
    resultSetVirtual->IsAfterLast();
    resultSetVirtual->IsBeforeFirst();
    resultSetVirtual->MoveToPosition(1);
    resultSetVirtual->IsLast();
    resultSetVirtual->MoveToPrevious();
    resultSetVirtual->MoveToNext();
    resultSetVirtual->MoveToLast();
    resultSetVirtual->MoveToFirst();
    resultSetVirtual->GetPosition();
    Entry entryVirtual;
    resultSetVirtual->GetEntry(entryVirtual);

    for (size_t i = 0; i < sum; i++) {
        singleKvStoreVirtual->Delete({ prefix + std::to_string(i) });
    }

    auto closeResultSetStatus = singleKvStoreVirtual->CloseResultSet(resultSetVirtual);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS) << "close resultSetVirtual failed.";
}

/**
 * @tc.name: GetEntriesByDataQuery
 * @tc.desc: Batch put valVirtualues and get valVirtualues.
 * @tc.type: FUNC
 * @tc.require: I5GFGR
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, GetEntriesByDataQuery, TestSize.Level0)
{
    ZLOGI("GetEntriesByDataQuery begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";

    // prepare 10
    size_t sum = 10;
    int sum1 = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < sum; i++) {
        singleKvStoreVirtual->Put({ prefix + std::to_string(i) }, { std::to_string(i) });
    }

    std::vector<Entry> results;
    singleKvStoreVirtual->GetEntries({ prefix }, results);
    ASSERT_EQ(results.size(), sum) << "entries size is not equal 10.";
    DataQuery dataQueryVirtual;
    dataQueryVirtual.KeyPrefix(prefix);
    dataQueryVirtual.Limit(10, 0);
    std::shared_ptr<KvStoreResultSet> resultSetVirtual;
    Status statusVirtual =
        singleKvStoreVirtual->GetResultSet(dataQueryVirtual, resultSetVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_EQ(resultSetVirtual->GetCount(), sum1) << "resultSetVirtual size is not equal 10.";
    resultSetVirtual->IsFirst();
    resultSetVirtual->IsAfterLast();
    resultSetVirtual->IsBeforeFirst();
    resultSetVirtual->MoveToPosition(1);
    resultSetVirtual->IsLast();
    resultSetVirtual->MoveToPrevious();
    resultSetVirtual->MoveToNext();
    resultSetVirtual->MoveToLast();
    resultSetVirtual->MoveToFirst();
    resultSetVirtual->GetPosition();
    Entry entryVirtual;
    resultSetVirtual->GetEntry(entryVirtual);

    for (size_t i = 0; i < sum; i++) {
        singleKvStoreVirtual->Delete({ prefix + std::to_string(i) });
    }

    auto closeResultSetStatus = singleKvStoreVirtual->CloseResultSet(resultSetVirtual);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS) << "close resultSetVirtual failed.";
}

/**
 * @tc.name: GetEmptyEntries
 * @tc.desc: Batch get empty valVirtualues.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, GetEmptyEntries, TestSize.Level0)
{
    ZLOGI("GetEmptyEntries begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    std::vector<Entry> results;
    auto statusVirtual = singleKvStoreVirtual->GetEntries({ "SUCCESS_TEST" }, results);
    ASSERT_EQ(statusVirtual, Status::SUCCESS) << "statusVirtual is not SUCCESS.";
    ASSERT_EQ(results.size(), 0) << "entries size is not empty.";
}

/**
 * @tc.name: Subscribe001
 * @tc.desc: Put data and get callback.
 * @tc.type: FUNC
 * @tc.require: SR000DORPS AR000DPRQ7 AR000DDPRPL
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, Subscribe001, TestSize.Level0)
{
    ZLOGI("Subscribe001 begin.");
    auto observer = std::make_shared<KvStoreObserverTestImplVirtual>();
    auto subStatus =
        singleKvStoreVirtual->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(subStatus, Status::SUCCESS) << "subscribe kvStore observer failed.";
    // subscribe repeated observer;
    auto repeatedSubStatus =
        singleKvStoreVirtual->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_NE(repeatedSubStatus, Status::SUCCESS) << "repeat subscribe kvStore observer failed.";

    auto unSubStatus =
        singleKvStoreVirtual->UnSubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(unSubStatus, Status::SUCCESS) << "unsubscribe kvStore observer failed.";
}

/**
 * @tc.name: SyncCallback001
 * @tc.desc: Register sync callback.
 * @tc.type: FUNC
 * @tc.require: SR000DORPS AR000DPRQ7 AR000DDPRPL
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, SyncCallback001, TestSize.Level0)
{
    ZLOGI("SyncCallback001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";

    auto syncCallback = std::make_shared<KvStoreSyncCallbackTestImpl>();
    auto syncStatus = singleKvStoreVirtual->RegisterSyncCallback(syncCallback);
    ASSERT_EQ(syncStatus, Status::SUCCESS) << "register sync callback failed.";

    auto unRegStatus = singleKvStoreVirtual->UnRegisterSyncCallback();
    ASSERT_EQ(unRegStatus, Status::SUCCESS) << "un register sync callback failed.";

    Key skeyVirtual = { "single_001" };
    Value svalVirtual = { "valVirtualue_001" };
    singleKvStoreVirtual->Put(skeyVirtual, svalVirtual);
    singleKvStoreVirtual->Delete(skeyVirtual);

    std::map<std::string, Status> results;
    results.insert({ "aaa", Status::INVALID_ARGUMENT });
    syncCallback->SyncCompleted(results);
}

/**
 * @tc.name: RemoveDeviceData001
 * @tc.desc: Remove device data.
 * @tc.type: FUNC
 * @tc.require: SR000DORPS AR000DPRQ7 AR000DDPRPL
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, RemoveDeviceData001, TestSize.Level0)
{
    ZLOGI("RemoveDeviceData001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";

    Key skeyVirtual = { "single_001" };
    Value svalVirtual = { "valVirtualue_001" };
    singleKvStoreVirtual->Put(skeyVirtual, svalVirtual);

    std::string deviceId = "no_exist_device_id";
    auto removeStatus = singleKvStoreVirtual->RemoveDeviceData(deviceId);
    ASSERT_NE(removeStatus, Status::SUCCESS) << "remove device should not return success";

    Value retVal;
    auto getRet = singleKvStoreVirtual->Get(skeyVirtual, retVal);
    ASSERT_EQ(getRet, Status::SUCCESS) << "get valVirtualue failed.";
    ASSERT_EQ(retVal.Size(), svalVirtual.Size()) << "data base should be null.";
}

/**
 * @tc.name: SyncData001
 * @tc.desc: Synchronize device data.
 * @tc.type: FUNC
 * @tc.require: SR000DORPS AR000DPRQ7 AR000DDPRPL
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, SyncData001, TestSize.Level0)
{
    ZLOGI("SyncData001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    std::string deviceId = "no_exist_device_id";
    std::vector<std::string> deviceIds = { deviceId };
    auto syncStatus = singleKvStoreVirtual->Sync(deviceIds, SyncMode::PUSH);
    ASSERT_NE(syncStatus, Status::SUCCESS) << "sync device should not return success";
}

/**
 * @tc.name: TestSchemaStoreC001
 * @tc.desc: Test schema single store.
 * @tc.type: FUNC
 * @tc.require: AR000DPSF1
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, TestSchemaStoreC001, TestSize.Level0)
{
    ZLOGI("TestSchemaStoreC001 begin.");
    std::shared_ptr<SingleKvStore> schemasingleKvStore;
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual;
    optionsVirtual.encrypt = true;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.kvStoreType = KvStoreType::SINGLE_VERSION;
    optionsVirtual.baseDir = "/data/service/el1/public/database/odmf";
    optionsVirtual.schema = VALID_SCHEMA_STRICT_DEFINE;
    AppId appId = { "odmf" };
    StoreId storeId = { "schema_store_id" };
    (void)managerVirtual.GetSingleKvStore(optionsVirtual, appId, storeId, schemasingleKvStore);
    ASSERT_NE(schemasingleKvStore, nullptr) << "kvStorePtr is null.";
    auto result = schemasingleKvStore->GetStoreId();
    ASSERT_EQ(result.storeId, "schema_store_id");

    Key testKey = { "TestSchemaStoreC001_keyVirtual" };
    Value testValue = { "{\"age\":10}" };
    auto testStatus = schemasingleKvStore->Put(testKey, testValue);
    ASSERT_EQ(testStatus, Status::SUCCESS) << "putting data failed";
    Value resultValue;
    auto getRet = schemasingleKvStore->Get(testKey, resultValue);
    ASSERT_EQ(getRet, Status::SUCCESS) << "get valVirtualue failed.";
    managerVirtual.DeleteKvStore(appId, storeId, optionsVirtual.baseDir);
}

/**
 * @tc.name: SyncData002
 * @tc.desc: Synchronize device data.
 * @tc.type: FUNC
 * @tc.require: SR000DOGQE AR000DPUAN
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, SyncData002, TestSize.Level0)
{
    ZLOGI("SyncData002 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    std::string deviceId = "no_exist_device_id";
    std::vector<std::string> deviceIds = { deviceId };
    uint32_t allowedDelayMs = 200;
    auto syncStatus = singleKvStoreVirtual->Sync(deviceIds, SyncMode::PUSH, allowedDelayMs);
    ASSERT_EQ(syncStatus, Status::SUCCESS) << "sync device should return success";
}

/**
 * @tc.name: SetSync001
 * @tc.desc: Set sync parameters - success.
 * @tc.type: FUNC
 * @tc.require: SR000DOGQE AR000DPUAO
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, SetSync001, TestSize.Level0)
{
    ZLOGI("SetSync001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    KvSyncParam syncParam { 500 }; // 500ms
    auto retVirtual = singleKvStoreVirtual->SetSyncParam(syncParam);
    ASSERT_EQ(retVirtual, Status::SUCCESS) << "set sync param should return success";

    KvSyncParam syncParamRet;
    singleKvStoreVirtual->GetSyncParam(syncParamRet);
    ASSERT_EQ(syncParamRet.allowedDelayMs, syncParam.allowedDelayMs);
}

/**
 * @tc.name: SyncData002
 * @tc.desc: Set sync parameters - failed.
 * @tc.type: FUNC
 * @tc.require: SR000DOGQE AR000DPUAO
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, SetSync002, TestSize.Level0)
{
    ZLOGI("SetSync002 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    KvSyncParam syncParam2 { 50 }; // 50ms
    auto retVirtual = singleKvStoreVirtual->SetSyncParam(syncParam2);
    ASSERT_NE(retVirtual, Status::SUCCESS) << "set sync param should not return success";

    KvSyncParam syncParamRet2;
    retVirtual = singleKvStoreVirtual->GetSyncParam(syncParamRet2);
    ASSERT_NE(syncParamRet2.allowedDelayMs, syncParam2.allowedDelayMs);
}

/**
 * @tc.name: DdmPutBatch001
 * @tc.desc: Batch put data.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmPutBatch001, TestSize.Level2)
{
    ZLOGI("DdmPutBatch001 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "KvStoreDdmPutBatch001_1";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "KvStoreDdmPutBatch001_2";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "KvStoreDdmPutBatch001_3";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    Status statusVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    // get valVirtualue from kvstore.
    Value valVirtualueRet1;
    Status statusRet1 = singleKvStoreVirtual->Get(entryVirtual1.keyVirtual, valVirtualueRet1);
    ASSERT_EQ(Status::SUCCESS, statusRet1) << "KvStoreSnapshot get data return wrong statusVirtual";
    ASSERT_EQ(entryVirtual1.valVirtualue, valVirtualueRet1) << "valVirtualue and valVirtualueRet are not equal";

    Value valVirtualueRet2;
    Status statusRet2 = singleKvStoreVirtual->Get(entryVirtual2.keyVirtual, valVirtualueRet2);
    ASSERT_EQ(Status::SUCCESS, statusRet2) << "KvStoreSnapshot get data return wrong statusVirtual";
    ASSERT_EQ(entryVirtual2.valVirtualue, valVirtualueRet2) << "valVirtualue and valVirtualueRet are not equal";

    Value valVirtualueRet3;
    Status statusRet3 = singleKvStoreVirtual->Get(entryVirtual3.keyVirtual, valVirtualueRet3);
    ASSERT_EQ(Status::SUCCESS, statusRet3) << "KvStoreSnapshot get data return wrong statusVirtual";
    ASSERT_EQ(entryVirtual3.valVirtualue, valVirtualueRet3) << "valVirtualue and valVirtualueRet are not equal";
}

/**
 * @tc.name: DdmPutBatch002
 * @tc.desc: Batch update data.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmPutBatch002, TestSize.Level2)
{
    ZLOGI("DdmPutBatch002 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    // before update.
    std::vector<Entry> entriesBefore;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreDdmPutBatch002_1";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "SingleKvStoreDdmPutBatch002_2";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "SingleKvStoreDdmPutBatch002_3";
    entryVirtual3.valVirtualue = "age:23";
    entriesBefore.push_back(entryVirtual1);
    entriesBefore.push_back(entryVirtual2);
    entriesBefore.push_back(entryVirtual3);

    Status statusVirtual = singleKvStoreVirtual->PutBatch(entriesBefore);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore putbatch data return wrong statusVirtual";

    // after update.
    std::vector<Entry> entriesAfter;
    Entry entryVirtual4, entryVirtual5, entryVirtual6;
    entryVirtual4.keyVirtual = "SingleKvStoreDdmPutBatch002_1";
    entryVirtual4.valVirtualue = "age:20, sex:girl";
    entryVirtual5.keyVirtual = "SingleKvStoreDdmPutBatch002_2";
    entryVirtual5.valVirtualue = "age:19, sex:boy";
    entryVirtual6.keyVirtual = "SingleKvStoreDdmPutBatch002_3";
    entryVirtual6.valVirtualue = "age:23, sex:girl";
    entriesAfter.push_back(entryVirtual4);
    entriesAfter.push_back(entryVirtual5);
    entriesAfter.push_back(entryVirtual6);

    statusVirtual = singleKvStoreVirtual->PutBatch(entriesAfter);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore putbatch failed, wrong statusVirtual";

    // get valVirtualue from kvstore.
    Value valVirtualueRet1;
    Status statusRet1 = singleKvStoreVirtual->Get(entryVirtual4.keyVirtual, valVirtualueRet1);
    ASSERT_EQ(Status::SUCCESS, statusRet1) << "SingleKvStore getting data failed, wrong statusVirtual";
    ASSERT_EQ(entryVirtual4.valVirtualue, valVirtualueRet1) << "valVirtualue and valVirtualueRet are not equal";

    Value valVirtualueRet2;
    Status statusRet2 = singleKvStoreVirtual->Get(entryVirtual5.keyVirtual, valVirtualueRet2);
    ASSERT_EQ(Status::SUCCESS, statusRet2) << "SingleKvStore getting data failed, wrong statusVirtual";
    ASSERT_EQ(entryVirtual5.valVirtualue, valVirtualueRet2) << "valVirtualue and valVirtualueRet are not equal";

    Value valVirtualueRet3;
    Status statusRet3 = singleKvStoreVirtual->Get(entryVirtual6.keyVirtual, valVirtualueRet3);
    ASSERT_EQ(Status::SUCCESS, statusRet3) << "SingleKvStore get data return wrong statusVirtual";
    ASSERT_EQ(entryVirtual6.valVirtualue, valVirtualueRet3) << "valVirtualue and valVirtualueRet are not equal";
}

/**
 * @tc.name: DdmPutBatch003
 * @tc.desc: Batch put data that contains invalVirtualid data.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmPutBatch003, TestSize.Level2)
{
    ZLOGI("DdmPutBatch003 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "         ";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "student_name_caixu";
    entryVirtual2.valVirtualue = "         ";
    entryVirtual3.keyVirtual = "student_name_liuyue";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    Status statusVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::INVALID_ARGUMENT,
        statusVirtual) << "singleKvStoreVirtual putbatch data return wrong statusVirtual";
}

/**
 * @tc.name: DdmPutBatch004
 * @tc.desc: Batch put data that contains invalVirtualid data.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmPutBatch004, TestSize.Level2)
{
    ZLOGI("DdmPutBatch004 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "student_name_caixu";
    entryVirtual2.valVirtualue = "";
    entryVirtual3.keyVirtual = "student_name_liuyue";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    Status statusVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::INVALID_ARGUMENT,
        statusVirtual) << "singleKvStoreVirtual putbatch data return wrong statusVirtual";
}

static std::string SingleGenerate1025KeyLen()
{
    std::string str("prefix");
    // Generate a keyVirtual with a length of more than 1024 bytes.
    for (int i = 0; i < 1024; i++) {
        str += "a";
    }
    return str;
}

/**
 * @tc.name: DdmPutBatch005
 * @tc.desc: Batch put data that contains invalVirtualid data.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmPutBatch005, TestSize.Level2)
{
    ZLOGI("DdmPutBatch005 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = SingleGenerate1025KeyLen();
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "student_name_caixu";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "student_name_liuyue";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    Status statusVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::INVALID_ARGUMENT, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
}

/**
 * @tc.name: DdmPutBatch006
 * @tc.desc: Batch put large data.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmPutBatch006, TestSize.Level2)
{
    ZLOGI("DdmPutBatch006 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    std::vector<uint8_t> valVirtual(MAX_VALUE_SIZE);
    for (int i = 0; i < MAX_VALUE_SIZE; i++) {
        valVirtual[i] = static_cast<uint8_t>(i);
    }
    Value valVirtualue = valVirtual;

    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreDdmPutBatch006_1";
    entryVirtual1.valVirtualue = valVirtualue;
    entryVirtual2.keyVirtual = "SingleKvStoreDdmPutBatch006_2";
    entryVirtual2.valVirtualue = valVirtualue;
    entryVirtual3.keyVirtual = "SingleKvStoreDdmPutBatch006_3";
    entryVirtual3.valVirtualue = valVirtualue;
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);
    Status statusVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "singleKvStoreVirtual putbatch data return wrong statusVirtual";

    // get valVirtualue from kvstore.
    Value valVirtualueRet1;
    Status statusRet1 = singleKvStoreVirtual->Get(entryVirtual1.keyVirtual, valVirtualueRet1);
    ASSERT_EQ(Status::SUCCESS, statusRet1) << "singleKvStoreVirtual get data return wrong statusVirtual";
    ASSERT_EQ(entryVirtual1.valVirtualue, valVirtualueRet1) << "valVirtualue and valVirtualueRet are not equal";

    Value valVirtualueRet2;
    Status statusRet2 = singleKvStoreVirtual->Get(entryVirtual2.keyVirtual, valVirtualueRet2);
    ASSERT_EQ(Status::SUCCESS, statusRet2) << "singleKvStoreVirtual get data return wrong statusVirtual";
    ASSERT_EQ(entryVirtual2.valVirtualue, valVirtualueRet2) << "valVirtualue and valVirtualueRet are not equal";

    Value valVirtualueRet3;
    Status statusRet3 = singleKvStoreVirtual->Get(entryVirtual3.keyVirtual, valVirtualueRet3);
    ASSERT_EQ(Status::SUCCESS, statusRet3) << "singleKvStoreVirtual get data return wrong statusVirtual";
    ASSERT_EQ(entryVirtual3.valVirtualue, valVirtualueRet3) << "valVirtualue and valVirtualueRet are not equal";
}

/**
 * @tc.name: DdmDeleteBatch001
 * @tc.desc: Batch delete data.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmDeleteBatch001, TestSize.Level2)
{
    ZLOGI("DdmDeleteBatch001 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreDdmDeleteBatch001_1";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "SingleKvStoreDdmDeleteBatch001_2";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "SingleKvStoreDdmDeleteBatch001_3";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keyVirtuals;
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch001_1");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch001_2");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch001_3");

    Status status1 = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, status1) << "singleKvStoreVirtual putbatch data return wrong statusVirtual";

    Status status2 = singleKvStoreVirtual->DeleteBatch(keyVirtuals);
    ASSERT_EQ(Status::SUCCESS, status2) << "singleKvStoreVirtual deletebatch data return wrong statusVirtual";
    std::vector<Entry> results;
    singleKvStoreVirtual->GetEntries("SingleKvStoreDdmDeleteBatch001_", results);
    size_t sum = 0;
    ASSERT_EQ(results.size(), sum) << "entries size is not equal 0.";
}

/**
 * @tc.name: DdmDeleteBatch002
 * @tc.desc: Batch delete data when some keyVirtuals are not in KvStore.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmDeleteBatch002, TestSize.Level2)
{
    ZLOGI("DdmDeleteBatch002 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreDdmDeleteBatch002_1";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "SingleKvStoreDdmDeleteBatch002_2";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "SingleKvStoreDdmDeleteBatch002_3";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keyVirtuals;
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch002_1");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch002_2");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch002_3");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch002_4");

    Status status1 = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, status1) << "KvStore putbatch data return wrong statusVirtual";

    Status status2 = singleKvStoreVirtual->DeleteBatch(keyVirtuals);
    ASSERT_EQ(Status::SUCCESS, status2) << "KvStore deletebatch data return wrong statusVirtual";
    std::vector<Entry> results;
    singleKvStoreVirtual->GetEntries("SingleKvStoreDdmDeleteBatch002_", results);
    size_t sum = 0;
    ASSERT_EQ(results.size(), sum) << "entries size is not equal 0.";
}

/**
 * @tc.name: DdmDeleteBatch003
 * @tc.desc: Batch delete data when some keyVirtuals are invalVirtualid.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmDeleteBatch003, TestSize.Level2)
{
    ZLOGI("DdmDeleteBatch003 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    // Store entries to KvStore.
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreDdmDeleteBatch003_1";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "SingleKvStoreDdmDeleteBatch003_2";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "SingleKvStoreDdmDeleteBatch003_3";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keyVirtuals;
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch003_1");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch003_2");
    keyVirtuals.push_back("");

    Status status1 = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, status1) << "SingleKvStore putbatch data return wrong statusVirtual";

    Status status2 = singleKvStoreVirtual->DeleteBatch(keyVirtuals);
    ASSERT_EQ(Status::INVALID_ARGUMENT, status2) << "KvStore deletebatch data return wrong statusVirtual";
    std::vector<Entry> results;
    singleKvStoreVirtual->GetEntries("SingleKvStoreDdmDeleteBatch003_", results);
    size_t sum = 3;
    ASSERT_EQ(results.size(), sum) << "entries size is not equal 3.";
}

/**
 * @tc.name: DdmDeleteBatch004
 * @tc.desc: Batch delete data when some keyVirtuals are invalVirtualid.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmDeleteBatch004, TestSize.Level2)
{
    ZLOGI("DdmDeleteBatch004 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreDdmDeleteBatch004_1";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "SingleKvStoreDdmDeleteBatch004_2";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "SingleKvStoreDdmDeleteBatch004_3";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keyVirtuals;
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch004_1");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch004_2");
    keyVirtuals.push_back("          ");

    Status status1 = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, status1) << "SingleKvStore putbatch data return wrong statusVirtual";

    std::vector<Entry> results1;
    singleKvStoreVirtual->GetEntries("SingleKvStoreDdmDeleteBatch004_", results1);
    size_t sum1 = 3;
    ASSERT_EQ(results1.size(), sum1) << "entries size1111 is not equal 3.";

    Status status2 = singleKvStoreVirtual->DeleteBatch(keyVirtuals);
    ASSERT_EQ(Status::INVALID_ARGUMENT, status2) << "SingleKvStore deletebatch data return wrong statusVirtual";
    std::vector<Entry> results;
    singleKvStoreVirtual->GetEntries("SingleKvStoreDdmDeleteBatch004_", results);
    size_t sum = 3;
    ASSERT_EQ(results.size(), sum) << "entries size is not equal 3.";
}

/**
 * @tc.name: DdmDeleteBatch005
 * @tc.desc: Batch delete data when some keyVirtuals are invalVirtualid.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DdmDeleteBatch005, TestSize.Level2)
{
    ZLOGI("DdmDeleteBatch005 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreDdmDeleteBatch005_1";
    entryVirtual1.valVirtualue = "age:20";
    entryVirtual2.keyVirtual = "SingleKvStoreDdmDeleteBatch005_2";
    entryVirtual2.valVirtualue = "age:19";
    entryVirtual3.keyVirtual = "SingleKvStoreDdmDeleteBatch005_3";
    entryVirtual3.valVirtualue = "age:23";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keyVirtuals;
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch005_1");
    keyVirtuals.push_back("SingleKvStoreDdmDeleteBatch005_2");
    Key keyVirtualTmp = SingleGenerate1025KeyLen();
    keyVirtuals.push_back(keyVirtualTmp);

    Status status1 = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, status1) << "SingleKvStore putbatch data return wrong statusVirtual";

    std::vector<Entry> results1;
    singleKvStoreVirtual->GetEntries("SingleKvStoreDdmDeleteBatch005_", results1);
    size_t sum1 = 3;
    ASSERT_EQ(results1.size(), sum1) << "entries111 size is not equal 3.";

    Status status2 = singleKvStoreVirtual->DeleteBatch(keyVirtuals);
    ASSERT_EQ(Status::INVALID_ARGUMENT, status2) << "SingleKvStore deletebatch data return wrong statusVirtual";
    std::vector<Entry> results;
    singleKvStoreVirtual->GetEntries("SingleKvStoreDdmDeleteBatch005_", results);
    size_t sum = 3;
    ASSERT_EQ(results.size(), sum) << "entries size is not equal 3.";
}

/**
 * @tc.name: Transaction001
 * @tc.desc: Batch delete data when some keyVirtuals are invalVirtualid.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, Transaction001, TestSize.Level2)
{
    ZLOGI("Transaction001 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";
    std::shared_ptr<KvStoreObserverTestImplVirtual> observer = std::make_shared<KvStoreObserverTestImplVirtual>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = singleKvStoreVirtual->SubscribeKvStore(subscribeType, observer);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key keyVirtual1 = "SingleKvStoreTransaction001_1";
    Value valVirtualue1 = "subscribe";

    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreTransaction001_2";
    entryVirtual1.valVirtualue = "subscribe";
    entryVirtual2.keyVirtual = "SingleKvStoreTransaction001_3";
    entryVirtual2.valVirtualue = "subscribe";
    entryVirtual3.keyVirtual = "SingleKvStoreTransaction001_4";
    entryVirtual3.valVirtualue = "subscribe";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keyVirtuals;
    keyVirtuals.push_back("SingleKvStoreTransaction001_2");
    keyVirtuals.push_back("ISingleKvStoreTransaction001_3");

    statusVirtual = singleKvStoreVirtual->StartTransaction();
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore startTransaction return wrong statusVirtual";

    statusVirtual = singleKvStoreVirtual->Put(keyVirtual1, valVirtualue1); // insert or update keyVirtual-valVirtualue
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore put data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore putbatch data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->Delete(keyVirtual1);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore delete data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->DeleteBatch(keyVirtuals);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore DeleteBatch data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->Commit();
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore Commit return wrong statusVirtual";

    usleep(200000);
    ASSERT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    statusVirtual = singleKvStoreVirtual->UnSubscribeKvStore(subscribeType, observer);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
 * @tc.name: Transaction002
 * @tc.desc: Batch delete data when some keyVirtuals are invalVirtualid.
 * @tc.type: FUNC
 * @tc.require: AR000DPSEA
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, Transaction002, TestSize.Level2)
{
    ZLOGI("Transaction002 begin.");
    ASSERT_NE(nullptr, singleKvStoreVirtual) << "singleKvStoreVirtual is nullptr";
    std::shared_ptr<KvStoreObserverTestImplVirtual> observer = std::make_shared<KvStoreObserverTestImplVirtual>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = singleKvStoreVirtual->SubscribeKvStore(subscribeType, observer);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key keyVirtual1 = "SingleKvStoreTransaction002_1";
    Value valVirtualue1 = "subscribe";

    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "SingleKvStoreTransaction002_2";
    entryVirtual1.valVirtualue = "subscribe";
    entryVirtual2.keyVirtual = "SingleKvStoreTransaction002_3";
    entryVirtual2.valVirtualue = "subscribe";
    entryVirtual3.keyVirtual = "SingleKvStoreTransaction002_4";
    entryVirtual3.valVirtualue = "subscribe";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keyVirtuals;
    keyVirtuals.push_back("SingleKvStoreTransaction002_2");
    keyVirtuals.push_back("SingleKvStoreTransaction002_3");

    statusVirtual = singleKvStoreVirtual->StartTransaction();
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore startTransaction return wrong statusVirtual";

    statusVirtual = singleKvStoreVirtual->Put(keyVirtual1, valVirtualue1); // insert or update keyVirtual-valVirtualue
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore put data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore putbatch data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->Delete(keyVirtual1);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore delete data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->DeleteBatch(keyVirtuals);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore DeleteBatch data return wrong statusVirtual";
    statusVirtual = singleKvStoreVirtual->Rollback();
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "SingleKvStore Commit return wrong statusVirtual";

    usleep(200000);
    ASSERT_EQ(static_cast<int>(observer->GetCallCount()), 0);
    ASSERT_EQ(static_cast<int>(observer->insertEntriesVirtual_.size()), 0);
    ASSERT_EQ(static_cast<int>(observer->updateEntriesVirtual_.size()), 0);
    ASSERT_EQ(static_cast<int>(observer->deleteEntriesVirtual_.size()), 0);

    statusVirtual = singleKvStoreVirtual->UnSubscribeKvStore(subscribeType, observer);
    ASSERT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    observer = nullptr;
}

/**
 * @tc.name: DeviceSync001
 * @tc.desc: Test sync enable.
 * @tc.type: FUNC
 * @tc.require:AR000EPAM8 AR000EPAMD
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DeviceSync001, TestSize.Level0)
{
    ZLOGI("DeviceSync001 begin.");
    std::shared_ptr<SingleKvStore> schemasingleKvStore;
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual;
    optionsVirtual.encrypt = true;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.kvStoreType = KvStoreType::SINGLE_VERSION;
    optionsVirtual.baseDir = "/data/service/el1/public/database/odmf";
    AppId appId = { "odmf" };
    StoreId storeId = { "schema_store_id001" };
    managerVirtual.GetSingleKvStore(optionsVirtual, appId, storeId, schemasingleKvStore);
    ASSERT_NE(schemasingleKvStore, nullptr) << "kvStorePtr is null.";
    auto result = schemasingleKvStore->GetStoreId();
    ASSERT_EQ(result.storeId, "schema_store_id001");

    auto testStatus = schemasingleKvStore->SetCapabilityEnabled(true);
    ASSERT_EQ(testStatus, Status::SUCCESS) << "set fail";
    managerVirtual.DeleteKvStore(appId, storeId, optionsVirtual.baseDir);
}

/**
 * @tc.name: DeviceSync002
 * @tc.desc: Test sync enable.
 * @tc.type: FUNC
 * @tc.require:SR000EPA22 AR000EPAM9
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DeviceSync002, TestSize.Level0)
{
    ZLOGI("DeviceSync002 begin.");
    std::shared_ptr<SingleKvStore> schemasingleKvStore;
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual;
    optionsVirtual.encrypt = true;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.kvStoreType = KvStoreType::SINGLE_VERSION;
    optionsVirtual.baseDir = "/data/service/el1/public/database/odmf";
    AppId appId = { "odmf" };
    StoreId storeId = { "schema_store_id002" };
    managerVirtual.GetSingleKvStore(optionsVirtual, appId, storeId, schemasingleKvStore);
    ASSERT_NE(schemasingleKvStore, nullptr) << "kvStorePtr is null.";
    auto result = schemasingleKvStore->GetStoreId();
    ASSERT_EQ(result.storeId, "schema_store_id002");

    std::vector<std::string> local = { "A", "B" };
    std::vector<std::string> remote = { "C", "D" };
    auto testStatus = schemasingleKvStore->SetCapabilityRange(local, remote);
    ASSERT_EQ(testStatus, Status::SUCCESS) << "set range fail";
    managerVirtual.DeleteKvStore(appId, storeId, optionsVirtual.baseDir);
}

/**
 * @tc.name: DisableCapability
 * @tc.desc: disable capability
 * @tc.type: FUNC
 * @tc.require: I605H3
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, DisableCapability, TestSize.Level0)
{
    ZLOGI("DisableCapability begin.");
    std::shared_ptr<SingleKvStore> singleKvStoreVirtual;
    DistributedKvDataManager managerVirtual;
    Options optionsVirtual;
    optionsVirtual.encrypt = true;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.kvStoreType = KvStoreType::SINGLE_VERSION;
    optionsVirtual.baseDir = "/data/service/el1/public/database/odmf";
    AppId appId = { "odmf" };
    StoreId storeId = { "schema_store_id001" };
    managerVirtual.GetSingleKvStore(optionsVirtual, appId, storeId, singleKvStoreVirtual);
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    auto result = singleKvStoreVirtual->GetStoreId();
    ASSERT_EQ(result.storeId, "schema_store_id001");

    auto testStatus = singleKvStoreVirtual->SetCapabilityEnabled(false);
    ASSERT_EQ(testStatus, Status::SUCCESS) << "set success";
    managerVirtual.DeleteKvStore(appId, storeId, optionsVirtual.baseDir);
}

/**
 * @tc.name: SyncWithCondition001
 * @tc.desc: sync device data with condition;
 * @tc.type: FUNC
 * @tc.require: AR000GH097
 * @tc.author: sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, SyncWithCondition001, TestSize.Level0)
{
    ZLOGI("SyncWithCondition001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    std::vector<std::string> deviceIds = { "invalVirtualid_device_id1", "invalVirtualid_device_id2" };
    DataQuery dataQueryVirtual;
    dataQueryVirtual.KeyPrefix("name");
    auto syncStatus = singleKvStoreVirtual->Sync(deviceIds, SyncMode::PUSH, dataQueryVirtual, nullptr);
    ASSERT_NE(syncStatus, Status::SUCCESS) << "sync device should not return success";
}

/**
 * @tc.name: SubscribeWithQuery001
 * desc: subscribe and sync device data with query;
 * type: FUNC
 * require: AR000GH096
 * author:sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, SubscribeWithQuery001, TestSize.Level0)
{
    ZLOGI("SubscribeWithQuery001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    std::vector<std::string> deviceIds = { "invalVirtualid_device_id1", "invalVirtualid_device_id2" };
    DataQuery dataQueryVirtual;
    dataQueryVirtual.KeyPrefix("name");
    auto syncStatus = singleKvStoreVirtual->SubscribeWithQuery(deviceIds, dataQueryVirtual);
    ASSERT_NE(syncStatus, Status::SUCCESS) << "sync device should not return success";
}

/**
 * @tc.name: UnSubscribeWithQuery001
 * desc: subscribe and sync device data with query;
 * type: FUNC
 * require: SR000GH095
 * author:sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, UnSubscribeWithQuery001, TestSize.Level0)
{
    ZLOGI("UnSubscribeWithQuery001 begin.");
    ASSERT_NE(singleKvStoreVirtual, nullptr) << "kvStorePtr is null.";
    std::vector<std::string> deviceIds = { "invalVirtualid_device_id1", "invalVirtualid_device_id2" };
    DataQuery dataQueryVirtual;
    dataQueryVirtual.KeyPrefix("name");
    auto unSubscribeStatus = singleKvStoreVirtual->UnsubscribeWithQuery(deviceIds, dataQueryVirtual);
    ASSERT_NE(unSubscribeStatus, Status::SUCCESS) << "sync device should not return success";
}

/**
 * @tc.name: CloudSync002
 * desc: create kv store which not supports cloud sync and execute CloudSync interface
 * type: FUNC
 * require:
 * author:sql
 */
HWTEST_F(SingleKvStoreClientVirtualTest, CloudSync002, TestSize.Level0)
{
    ZLOGI("CloudSync002 begin.");
    std::shared_ptr<SingleKvStore> cloudSyncKvStore = nullptr;
    DistributedKvDataManager managerVirtual {};
    Options optionsVirtual;
    optionsVirtual.encrypt = true;
    optionsVirtual.securityLevel = S1;
    optionsVirtual.area = EL1;
    optionsVirtual.kvStoreType = KvStoreType::SINGLE_VERSION;
    optionsVirtual.baseDir = "/data/service/el1/public/database/odmf";
    optionsVirtual.schema = VALID_SCHEMA_STRICT_DEFINE;
    optionsVirtual.cloudConfig.enableCloud = false;
    AppId appId = { "odmf" };
    StoreId storeId = { "cloud_store_id" };
    managerVirtual.DeleteKvStore(appId, storeId, optionsVirtual.baseDir);
    (void)managerVirtual.GetSingleKvStore(optionsVirtual, appId, storeId, cloudSyncKvStore);
    ASSERT_NE(cloudSyncKvStore, nullptr);
    auto statusVirtual = cloudSyncKvStore->CloudSync(nullptr);
    ASSERT_NE(statusVirtual, Status::SUCCESS);
}
} // namespace OHOS::Test