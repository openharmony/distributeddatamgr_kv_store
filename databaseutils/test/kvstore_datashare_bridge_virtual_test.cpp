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
#define LOG_TAG "KvstoreDatashareBridgeVirtualTest"

#include "distributed_kv_data_manager.h"
#include "gtest/gtest.h"
#include "kv_utils.h"
#include <memory>
#include <vector>
#include "kvstore_datashare_bridge.h"
#include "kvstore_result_set.h"
#include "result_set_bridge.h"
#include "store_errno.h"
#include "types.h"
#include "kvstore_observer_client.h"
#include "kvstore_observer.h"
#include "log_print.h"
#include <mutex>
#include <set>
#include <string>
#include "kvstore_service_death_notifier.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "kvstore_client_death_observer.h"
#include "datamgr_service_proxy.h"
#include "refbase.h"
#include "system_ability_definition.h"
#include "task_executor.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS::Test {
class BridgeWriterVirtual final : public ResultSetBridge::Writer {
public:
    int AllocRow() override;
    int WriteVirtual(uint32_t column) override;
    int WriteVirtual(uint32_t column, int64_t value) override;
    int WriteVirtual(uint32_t column, double value) override;
    int WriteVirtual(uint32_t column, const uint8_t *value, size_t size) override;
    int WriteVirtual(uint32_t column, const char *value, size_t size) override;
    void SetAllocRowStatue(int status);
    Key GetKey() const;
    Key GetValue() const;

private:
    int allocStatus_Virtual = E_OK;
    std::vector<uint8_t> key_Virtual;
    std::vector<uint8_t> value_Virtual;
};

void BridgeWriterVirtual::SetAllocRowStatue(int status)
{
    allocStatus_Virtual = status;
}

Key BridgeWriterVirtual::GetKey() const
{
    return key_Virtual;
}

Value BridgeWriterVirtual::GetValue() const
{
    return value_Virtual;
}

int BridgeWriterVirtual::AllocRow()
{
    return allocStatus_Virtual;
}

int BridgeWriterVirtual::WriteVirtual(uint32_t column)
{
    return E_OK;
}

int BridgeWriterVirtual::WriteVirtual(uint32_t column, int64_t value)
{
    return E_OK;
}

int BridgeWriterVirtual::WriteVirtual(uint32_t column, double value)
{
    return E_OK;
}

int BridgeWriterVirtual::WriteVirtual(uint32_t column, const uint8_t *value, size_t size)
{
    return E_OK;
}

int BridgeWriterVirtual::WriteVirtual(uint32_t column, const char *value, size_t size)
{
    if (column < 0 || column > 1 || value == nullptr) {
        return E_ERROR;
    }
    auto vec = std::vector<uint8_t>(value, value + size - 1);
    if (column == 0) {
        key_Virtual.insert(key_Virtual.end(), vec.begin(), vec.end());
    } else {
        value_Virtual.insert(value_Virtual.end(), vec.begin(), vec.end());
    }
    return E_OK;
}

class KvstoreDatashareBridgeVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}

protected:
    static DistributedKvDataManager managerVirtual;
    static std::shared_ptr<SingleKvStore> singleKvStoreVirtual;
};
std::shared_ptr<SingleKvStore> KvstoreDatashareBridgeVirtualTest::singleKvStoreVirtual = nullptr;
DistributedKvDataManager KvstoreDatashareBridgeVirtualTest::managerVirtual;
static constexpr int32_t INVALID_COUNT = -1;
static constexpr const char *VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
                                                           "\"SCHEMA_MODE\":\"STRICT\","
                                                           "\"SCHEMA_SKIPSIZE\":0,"
                                                           "\"SCHEMA_DEFINE\":{"
                                                           "\"age\":\"INTEGER, NOT NULL\""
                                                           "},"
                                                           "\"SCHEMA_INDEXES\":[\"$.age\"]}";

void KvstoreDatashareBridgeVirtualTest::SetUpTestCase(void)
{
    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = false,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema =  VALID_SCHEMA_STRICT_DEFINE };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/KvstoreDatashareBridgeVirtualTest");
    AppId appId = { "KvstoreDatashareBridgeVirtualTest" };
    StoreId storeId = { "test_single" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    managerVirtual.DeleteKvStore(appId, storeId, options.baseDir);
    managerVirtual.GetSingleKvStore(options, appId, storeId, singleKvStoreVirtual);
    EXPECT_NE(singleKvStoreVirtual, nullptr);
    singleKvStoreVirtual->Put("test_key_1", "{\"age\":1}");
    singleKvStoreVirtual->Put("test_key_2", "{\"age\":2}");
    singleKvStoreVirtual->Put("test_key_3", "{\"age\":3}");
    singleKvStoreVirtual->Put("data_share", "{\"age\":4}");
}

void KvstoreDatashareBridgeVirtualTest::TearDownTestCase(void)
{
    managerVirtual.DeleteKvStore({"KvstoreDatashareBridgeVirtualTest"}, {"test_single"},
        "/data/service/el1/public/database/KvstoreDatashareBridgeVirtualTest");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeVirtualTest/key");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeVirtualTest/kvdb");
    (void) remove("/data/service/el1/public/database/KvstoreDatashareBridgeVirtualTest");
}

class MockKvStoreResultSet : public KvStoreResultSet {
public:
    explicit MockKvStoreResultSet(int count) : count_(count)
    {
    }

    int GetCount() override
    {
        return count_;
    }

    bool MoveToPosition(int pos) override
    {
        return pos >= 0 && pos < count_;
    }

    Status GetEntry(Entry &entry) override
    {
        if (pos_ >= 0 && pos_ < count_) {
            entry.key = Key("key" + std::to_string(pos_));
            entry.value = Value("value" + std::to_string(pos_));
            return Status::SUCCESS;
        }
        return Status::ERROR;
    }

private:
    int count_;
    int pos_ = -1;
};

class MockWriter : public ResultSetBridge::Writer {
public:
    int AllocRow() override
    {
        return E_OK;
    }

    int Write(int index, const char *data, int size) override
    {
        return E_OK;
    }
};

class KvStoreObserverClientVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvStoreObserverClientVirtualTest::SetUpTestCase(void)
{}

void KvStoreObserverClientVirtualTest::TearDownTestCase(void)
{}

void KvStoreObserverClientVirtualTest::SetUp(void)
{}

void KvStoreObserverClientVirtualTest::TearDown(void)
{}

class MockKvStoreObserver : public KvStoreObserver {
public:
    MOCK_METHOD1(OnChange, void(const ChangeNotification &changeNotification));
    MOCK_METHOD2(OnChange, void(const DataOrigin &origin, IKvStoreObserver::Keys &&keys));
};

class KvStoreServiceDeathNotifierVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvStoreServiceDeathNotifierVirtualTest::SetUpTestCase(void)
{}

void KvStoreServiceDeathNotifierVirtualTest::TearDownTestCase(void)
{}

void KvStoreServiceDeathNotifierVirtualTest::SetUp(void)
{}

void KvStoreServiceDeathNotifierVirtualTest::TearDown(void)
{}

class MockSystemAbilityManager : public SystemAbilityManager {
public:
    MOCK_METHOD(sptr<IRemoteObject>, CheckSystemAbility, (int32_t), (override));
};

class MockTaskExecutor : public TaskExecutor {
public:
    MOCK_METHOD(void, Execute, (std::function<void()>), (override));
};

class MockKvStoreDeathRecipient : public KvStoreDeathRecipient {
public:
    MOCK_METHOD(void, OnRemoteDied, (), (override));
};

class SyncObserverVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SyncObserverVirtualTest::SetUpTestCase(void)
{}

void SyncObserverVirtualTest::TearDownTestCase(void)
{}

void SyncObserverVirtualTest::SetUp(void)
{}

void SyncObserverVirtualTest::TearDown(void)
{}

class MockKvStoreSyncCallback : public KvStoreSyncCallback {
public:
    MOCK_METHOD1(SyncCompleted, void(const std::map<std::string, Status> &results));
};

/**
* @tc.name: SyncCompleted_EmptyCallbacks_NoException
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(MockKvStoreSyncCallback, SyncCompleted_EmptyCallbacks_NoException, TestSize.Level0)
{
    ZLOGI("SyncCompleted_EmptyCallbacks_NoException begin.");
    SyncObserver observer;
    std::map<std::string, Status> results;
    EXPECT_NO_THROW(observer.SyncCompleted(results));
}

/**
* @tc.name: SyncCompleted_NonEmptyCallbacks_CallbacksInvoked
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(MockKvStoreSyncCallback, SyncCompleted_NonEmptyCallbacks_CallbacksInvoked, TestSize.Level0)
{
    ZLOGI("SyncCompleted_NonEmptyCallbacks_CallbacksInvoked begin.");
    SyncObserver observer;
    auto mockCallback = std::make_shared<MockKvStoreSyncCallback>();
    observer.Add(mockCallback);

    std::map<std::string, Status> results = {{"key1", Status::SUCCESS}};
    EXPECT_CALL(*mockCallback, SyncCompleted(results)).Times(1);

    observer.SyncCompleted(results);
}

/**
* @tc.name: GetDistributedKvDataService_ProxyExists_ReturnsProxy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreServiceDeathNotifierVirtualTest, GetDistributedKvDataService_ProxyExists_ReturnsProxy, TestSize.Level0)
{
    ZLOGI("GetDistributedKvDataService_ProxyExists_ReturnsProxy begin.");
    auto &instance = KvStoreServiceDeathNotifier::GetInstance();
    instance.kvDataServiceProxy_ = new DataMgrServiceProxy(nullptr);

    auto proxy = instance.GetDistributedKvDataService();

    EXPECT_EQ(proxy, instance.kvDataServiceProxy_);
}

/**
* @tc.name: GetDistributedKvDataService_ProxyDoesNotExist
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreServiceDeathNotifierVirtualTest, GetDistributedKvDataService_ProxyDoesNotExist, TestSize.Level0)
{
    ZLOGI("GetDistributedKvDataService_ProxyDoesNotExist begin.");
    auto &instance = KvStoreServiceDeathNotifier::GetInstance();
    instance.kvDataServiceProxy_ = nullptr;

    auto samgr = std::dynamic_pointer_cast<MockSystemAbilityManager>(
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager());
    auto remote = sptr<IRemoteObject>(new RemoteObject("test"));
    EXPECT_CALL(*samgr, CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID)).WillOnce(testing::Return(remote));

    auto proxy = instance.GetDistributedKvDataService();

    EXPECT_NE(proxy, nullptr);
    EXPECT_NE(instance.kvDataServiceProxy_, nullptr);
}

/**
* @tc.name: RegisterClientDeathObserver_ProxyExists
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreServiceDeathNotifierVirtualTest, RegisterClientDeathObserver_ProxyExists, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_ProxyExists begin.");
    auto &instance = KvStoreServiceDeathNotifier::GetInstance();
    instance.kvDataServiceProxy_ = new DataMgrServiceProxy(nullptr);

    instance.RegisterClientDeathObserver();

    EXPECT_NE(instance.clientDeathObserverPtr_, nullptr);
}

/**
* @tc.name: AddServiceDeathWatcher_Success_AddsWatcher
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreServiceDeathNotifierVirtualTest, AddServiceDeathWatcher_Success_AddsWatcher, TestSize.Level0)
{
    ZLOGI("AddServiceDeathWatcher_Success_AddsWatcher begin.");
    auto &instance = KvStoreServiceDeathNotifier::GetInstance();
    auto watcher = std::make_shared<MockKvStoreDeathRecipient>();

    instance.AddServiceDeathWatcher(watcher);

    EXPECT_EQ(instance.serviceDeathWatchers_.size(), 1);
}

/**
* @tc.name: RemoveServiceDeathWatcher_Found_RemovesWatcher
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreServiceDeathNotifierVirtualTest, RemoveServiceDeathWatcher_Found_RemovesWatcher, TestSize.Level0)
{
    ZLOGI("RemoveServiceDeathWatcher_Found_RemovesWatcher begin.");
    auto &instance = KvStoreServiceDeathNotifier::GetInstance();
    auto watcher = std::make_shared<MockKvStoreDeathRecipient>();
    instance.serviceDeathWatchers_.insert(watcher);

    instance.RemoveServiceDeathWatcher(watcher);

    EXPECT_EQ(instance.serviceDeathWatchers_.size(), 0);
}

/**
* @tc.name: ServiceDeathRecipient_OnRemoteDied_NotifiesWatchers
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreServiceDeathNotifierVirtualTest, ServiceDeathRecipient_OnRemoteDied_NotifiesWatchers, TestSize.Level0)
{
    ZLOGI("ServiceDeathRecipient_OnRemoteDied_NotifiesWatchers begin.");
    auto &instance = KvStoreServiceDeathNotifier::GetInstance();
    auto watcher = std::make_shared<MockKvStoreDeathRecipient>();
    instance.serviceDeathWatchers_.insert(watcher);

    auto executor = std::dynamic_pointer_cast<MockTaskExecutor>(TaskExecutor::GetInstance().GetExecutor());
    EXPECT_CALL(*executor, Execute(testing::_)).WillOnce(testing::Invoke([](std::function<void()> task) {
        task();
    }));

    instance.ServiceDeathRecipient().OnRemoteDied(nullptr);

    EXPECT_CALL(*watcher, OnRemoteDied()).Times(1);
}

/**
* @tc.name: OnChange_ChangeNotification_KvStoreObserverNotNull
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreObserverClientVirtualTest, OnChange_ChangeNotification_KvStoreObserverNotNull, TestSize.Level0)
{
    ZLOGI("OnChange_ChangeNotification_KvStoreObserverNotNull begin.");
    ChangeNotification changeNotification;
    EXPECT_CALL(*kvStoreObserver, OnChange(Ref(changeNotification)));
    kvStoreObserverClient->OnChange(changeNotification);
}

/**
* @tc.name: OnChange_ChangeNotification_KvStoreObserverNull
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreObserverClientVirtualTest, OnChange_ChangeNotification_KvStoreObserverNull, TestSize.Level0)
{
    ZLOGI("OnChange_ChangeNotification_KvStoreObserverNull begin.");
    kvStoreObserverClient = std::make_shared<KvStoreObserverClient>(nullptr);
    ChangeNotification changeNotification;
    EXPECT_CALL(*kvStoreObserver, OnChange(Ref(changeNotification))).Times(0);
    kvStoreObserverClient->OnChange(changeNotification);
}

/**
* @tc.name: OnChange_DataOriginAndKeys_KvStoreObserverNotNull
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreObserverClientVirtualTest, OnChange_DataOriginAndKeys_KvStoreObserverNotNull, TestSize.Level0)
{
    ZLOGI("OnChange_DataOriginAndKeys_KvStoreObserverNotNull begin.");
    DataOrigin origin;
    IKvStoreObserver::Keys keys;
    EXPECT_CALL(*kvStoreObserver, OnChange(Ref(origin), Ref(keys)));
    kvStoreObserverClient->OnChange(origin, std::move(keys));
}

/**
* @tc.name: OnChange_DataOriginAndKeys_KvStoreObserverNull
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvStoreObserverClientVirtualTest, OnChange_DataOriginAndKeys_KvStoreObserverNull, TestSize.Level0)
{
    ZLOGI("OnChange_DataOriginAndKeys_KvStoreObserverNull begin.");
    kvStoreObserverClient = std::make_shared<KvStoreObserverClient>(nullptr);
    DataOrigin origin;
    IKvStoreObserver::Keys keys;
    EXPECT_CALL(*kvStoreObserver, OnChange(Ref(origin), Ref(keys))).Times(0);
    kvStoreObserverClient->OnChange(origin, std::move(keys));
}

/**
* @tc.name: GetRowCountByInvalidBridge
* @tc.desc: get row countVirtual, the kvStore resultSetVirtual is nullptr
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, GetRowCountByInvalidBridge, TestSize.Level0)
{
    ZLOGI("GetRowCountByInvalidBridge begin.");
    auto bridgeVirtual = std::make_shared<KvStoreDataShareBridge>(nullptr);
    int32_t countVirtual;
    auto resultVirtual = bridgeVirtual->GetRowCount(countVirtual);
    EXPECT_EQ(resultVirtual, E_ERROR);
    EXPECT_EQ(countVirtual, INVALID_COUNT);
    std::vector<std::string> columnNames;
    resultVirtual = bridgeVirtual->GetAllColumnNames(columnNames);
    EXPECT_FALSE(columnNames.empty());
    EXPECT_EQ(resultVirtual, E_OK);
}

/**
* @tc.name: KvStoreResultSetToDataShareResultSetAbnormal
* @tc.desc: kvStore resultSetVirtual to dataShare resultSetVirtual, the former has invalid predicate
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, KvStoreResultSetToDataShareResultSetAbnormal, TestSize.Level0)
{
    ZLOGI("KvStoreResultSetToDataShareResultSetAbnormal begin.");
    std::shared_ptr<KvStoreResultSet> resultSetVirtual = nullptr;
    DataQuery queryVirtual;
    queryVirtual.KeyPrefix("key");
    singleKvStoreVirtual->GetResultSet(queryVirtual, resultSetVirtual);
    EXPECT_NE(resultSetVirtual, nullptr);
    auto bridgeVirtual = KvUtils::ToResultSetBridge(resultSetVirtual);
    int32_t countVirtual;
    auto resultVirtual = bridgeVirtual->GetRowCount(countVirtual);
    EXPECT_EQ(resultVirtual, E_OK);
    EXPECT_EQ(countVirtual, 0);
}

/**
* @tc.name: KvStoreResultSetToDataShareResultSetNormal
* @tc.desc: kvStore resultSetVirtual to dataShare resultSetVirtual, the former has valid predicate
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, KvStoreResultSetToDataShareResultSetNormal, TestSize.Level0)
{
    ZLOGI("KvStoreResultSetToDataShareResultSetNormal begin.");
    DataQuery queryVirtual;
    queryVirtual.KeyPrefix("test");
    std::shared_ptr<KvStoreResultSet> resultSetVirtual = nullptr;
    singleKvStoreVirtual->GetResultSet(queryVirtual, resultSetVirtual);
    EXPECT_NE(resultSetVirtual, nullptr);
    auto bridgeVirtual = KvUtils::ToResultSetBridge(resultSetVirtual);
    int32_t countVirtual;
    auto resultVirtual = bridgeVirtual->GetRowCount(countVirtual);
    EXPECT_EQ(resultVirtual, E_OK);
    EXPECT_EQ(countVirtual, 3);
    countVirtual = -1;
    bridgeVirtual->GetRowCount(countVirtual);
    EXPECT_EQ(countVirtual, 3);
}

/**
* @tc.name: BridgeOnGoAbnormal
* @tc.desc: bridgeVirtual on go, the input parameter is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, BridgeOnGoAbnormal, TestSize.Level0)
{
    ZLOGI("BridgeOnGoAbnormal begin.");
    DataQuery queryVirtual;
    queryVirtual.KeyPrefix("test");
    std::shared_ptr<KvStoreResultSet> resultSetVirtual = nullptr;
    singleKvStoreVirtual->GetResultSet(queryVirtual, resultSetVirtual);
    EXPECT_NE(resultSetVirtual, nullptr);
    auto bridgeVirtual = KvUtils::ToResultSetBridge(resultSetVirtual);
    int32_t startVirtual = -1;
    int32_t targetVirtual = 0;
    BridgeWriterVirtual writerVirtual;
    EXPECT_EQ(bridgeVirtual->OnGo(startVirtual, targetVirtual, writerVirtual), -1);
    EXPECT_TRUE(writerVirtual.GetKey().Empty());
    EXPECT_TRUE(writerVirtual.GetValue().Empty());
    startVirtual = 0;
    targetVirtual = -1;
    EXPECT_EQ(bridgeVirtual->OnGo(startVirtual, targetVirtual, writerVirtual), -1);
    EXPECT_TRUE(writerVirtual.GetKey().Empty());
    EXPECT_TRUE(writerVirtual.GetValue().Empty());
    startVirtual = 1;
    targetVirtual = 0;
    EXPECT_EQ(bridgeVirtual->OnGo(startVirtual, targetVirtual, writerVirtual), -1);
    EXPECT_TRUE(writerVirtual.GetKey().Empty());
    EXPECT_TRUE(writerVirtual.GetValue().Empty());
    startVirtual = 1;
    targetVirtual = 3;
    EXPECT_EQ(bridgeVirtual->OnGo(startVirtual, targetVirtual, writerVirtual), -1);
    EXPECT_TRUE(writerVirtual.GetKey().Empty());
    EXPECT_TRUE(writerVirtual.GetValue().Empty());
}

/**
* @tc.name: BridgeOnGoNormal
* @tc.desc: bridgeVirtual on go, the input parameter is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, BridgeOnGoNormal, TestSize.Level0)
{
    ZLOGI("BridgeOnGoNormal begin.");
    DataQuery queryVirtual;
    queryVirtual.KeyPrefix("test");
    std::shared_ptr<KvStoreResultSet> resultSetVirtual = nullptr;
    singleKvStoreVirtual->GetResultSet(queryVirtual, resultSetVirtual);
    EXPECT_NE(resultSetVirtual, nullptr);
    auto bridgeVirtual = KvUtils::ToResultSetBridge(resultSetVirtual);
    int startVirtual = 0;
    int targetVirtual = 2;
    BridgeWriterVirtual writerVirtual;
    writerVirtual.SetAllocRowStatue(E_ERROR);
    EXPECT_EQ(bridgeVirtual->OnGo(startVirtual, targetVirtual, writerVirtual), -1);
    EXPECT_TRUE(writerVirtual.GetKey().Empty());
    EXPECT_TRUE(writerVirtual.GetValue().Empty());
    writerVirtual.SetAllocRowStatue(E_OK);
    EXPECT_EQ(bridgeVirtual->OnGo(startVirtual, targetVirtual, writerVirtual), targetVirtual);
    size_t  keySize = 0;
    size_t  valueSize = 0;
    for (auto i = startVirtual; i <= targetVirtual; i++) {
        resultSetVirtual->MoveToPosition(i);
        Entry entry;
        resultSetVirtual->GetEntry(entry);
        keySize += entry.key.Size();
        valueSize += entry.value.Size();
    }
    EXPECT_EQ(writerVirtual.GetKey().Size(), keySize);
    EXPECT_EQ(writerVirtual.GetValue().Size(), valueSize);
}

/**
* @tc.name: OnGo_ValidIndices_FillAllRows
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, OnGo_ValidIndices_FillAllRows, TestSize.Level0)
{
    ZLOGI("OnGo_ValidIndices_FillAllRows begin.");
    MockWriter writer;
    int result = bridge->OnGo(0, 5, writer);
    EXPECT_EQ(result, 5);
}

/**
* @tc.name: OnGo_InvalidStartIndex_ReturnsError
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, OnGo_InvalidStartIndex_ReturnsError, TestSize.Level0)
{
    ZLOGI("OnGo_InvalidStartIndex_ReturnsError begin.");
    MockWriter writer;
    int result = bridge->OnGo(-1, 5, writer);
    EXPECT_EQ(result, -1);
}

/**
* @tc.name: OnGo_InvalidTargetIndex_ReturnsError
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, OnGo_InvalidTargetIndex_ReturnsError, TestSize.Level0)
{
    ZLOGI("OnGo_InvalidTargetIndex_ReturnsError begin.");
    MockWriter writer;
    int result = bridge->OnGo(0, 15, writer);
    EXPECT_EQ(result, -1);
}

/**
* @tc.name: OnGo_StartGreaterThanTarget_ReturnsError
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, OnGo_StartGreaterThanTarget_ReturnsError, TestSize.Level0)
{
    ZLOGI("OnGo_StartGreaterThanTarget_ReturnsError begin.");
    MockWriter writer;
    int result = bridge->OnGo(5, 0, writer);
    EXPECT_EQ(result, -1);
}

/**
* @tc.name: OnGo_FillBlockFails_ReturnsPartialFill
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: sql
*/
HWTEST_F(KvstoreDatashareBridgeVirtualTest, OnGo_FillBlockFails_ReturnsPartialFill, TestSize.Level0)
{
    ZLOGI("OnGo_FillBlockFails_ReturnsPartialFill begin.");
    kvResultSet = std::make_shared<MockKvStoreResultSet>(1);
    bridge = std::make_shared<KvStoreDataShareBridge>(kvResultSet);

    MockWriter writer;
    int result = bridge->OnGo(0, 5, writer);
    EXPECT_EQ(result, 0);
}
} // namespace OHOS::Test