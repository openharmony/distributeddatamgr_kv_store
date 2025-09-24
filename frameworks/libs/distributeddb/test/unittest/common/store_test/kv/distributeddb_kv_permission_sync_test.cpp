/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "distributeddb_tools_unit_test.h"
#include "kv_general_ut.h"
#include "kv_store_nb_delegate_impl.h"
#include "runtime_config.h"
#include "sqlite_single_ver_natural_store_connection.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBKvPermissionSyncTest : public KVGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    void PrepareData();
    void CheckData(const StoreInfo &info, const Entry &expectEntry, DBStatus status);
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
};

void DistributedDBKvPermissionSyncTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
    ASSERT_NO_FATAL_FAILURE(PrepareData());
    RuntimeConfig::SetDataFlowCheckCallback([storeInfo1, storeInfo2](const PermissionCheckParam &param,
        const Property &property) {
        if (param.storeId == storeInfo2.storeId) {
            EXPECT_EQ(property.size(), 1);
            return DataFlowCheckRet::DEFAULT;
        }
        EXPECT_EQ(property.size(), 0);
        return DataFlowCheckRet::DENIED_SEND;
    });
}

void DistributedDBKvPermissionSyncTest::TearDown()
{
    RuntimeConfig::SetDataFlowCheckCallback(nullptr);
    PermissionCheckCallbackV4 callbackV4 = nullptr;
    RuntimeConfig::SetPermissionCheckCallback(callbackV4);
    KVGeneralUt::TearDown();
}

void DistributedDBKvPermissionSyncTest::PrepareData()
{
    /**
     * @tc.steps: step1. dev1 put (k1,v1)
     * @tc.expected: step1. return OK
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto k1v1 = DistributedDBToolsUnitTest::GetK1V1();
    EXPECT_EQ(store1->Put(k1v1.key, k1v1.value), OK);
    Value actualValue;
    EXPECT_EQ(store1->Get(k1v1.key, actualValue), OK);
    EXPECT_EQ(k1v1.value, actualValue);
    Property property;
    store1->SetProperty(property);
    /**
     * @tc.steps: step2. dev2 put (k2,v2)
     * @tc.expected: step2. return OK
     */
    auto storeInfo2 = GetStoreInfo2();
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    auto k2v2 = DistributedDBToolsUnitTest::GetK2V2();
    EXPECT_EQ(store2->Put(k2v2.key, k2v2.value), OK);
    EXPECT_EQ(store2->Get(k2v2.key, actualValue), OK);
    EXPECT_EQ(k2v2.value, actualValue);
    property["field"] = std::string("field");
    store2->SetProperty(property);
}

void DistributedDBKvPermissionSyncTest::CheckData(const StoreInfo &info, const Entry &expectEntry, DBStatus status)
{
    auto store = GetDelegate(info);
    ASSERT_NE(store, nullptr);
    Value actualValue;
    ASSERT_EQ(store->Get(expectEntry.key, actualValue), status);
    if (status != OK) {
        return;
    }
    EXPECT_EQ(expectEntry.value, actualValue);
}

/**
 * @tc.name: KVPermissionSync001
 * @tc.desc: Test push OK by permission callback denied.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 sync to dev2
     * @tc.expected: step1. sync ok
     */
    BlockDeviceSync(GetStoreInfo1(), GetStoreInfo2(), SyncMode::SYNC_MODE_PUSH_ONLY, DBStatus::OK);
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK1V1(), OK));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo2(), DistributedDBToolsUnitTest::GetK1V1(), NOT_FOUND));
}

/**
 * @tc.name: KVPermissionSync002
 * @tc.desc: Test pull ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 sync to dev2
     * @tc.expected: step1. sync ok and dev1 exist (K1,V1) (K2,V2)
     */
    BlockDeviceSync(GetStoreInfo1(), GetStoreInfo2(), SyncMode::SYNC_MODE_PULL_ONLY, OK);
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK1V1(), OK));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK2V2(), OK));
}

/**
 * @tc.name: KVPermissionSync003
 * @tc.desc: Test push pull ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 sync to dev2
     * @tc.expected: step1. sync ok and dev1 exist (K1,V1) (K2,V2)
     */
    BlockDeviceSync(GetStoreInfo1(), GetStoreInfo2(), SyncMode::SYNC_MODE_PUSH_PULL, OK);
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK1V1(), OK));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK2V2(), OK));
}

/**
 * @tc.name: KVPermissionSync004
 * @tc.desc: Test push ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev2 sync to dev1
     * @tc.expected: step1. sync ok and dev1 exist (K1,V1) (K2,V2)
     */
    BlockDeviceSync(GetStoreInfo2(), GetStoreInfo1(), SyncMode::SYNC_MODE_PUSH_ONLY, OK);
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK1V1(), OK));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK2V2(), OK));
}

/**
 * @tc.name: KVPermissionSync005
 * @tc.desc: Test push OK by permission callback denied.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev2 sync to dev1
     * @tc.expected: step1. sync OK
     */
    BlockDeviceSync(GetStoreInfo2(), GetStoreInfo1(), SyncMode::SYNC_MODE_PULL_ONLY,
        DBStatus::OK);
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK1V1(), OK));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo2(), DistributedDBToolsUnitTest::GetK1V1(), NOT_FOUND));
}

/**
 * @tc.name: KVPermissionSync006
 * @tc.desc: Test push pull ok with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev2 sync to dev1
     * @tc.expected: step1. sync ok and dev1 exist (K1,V1) (K2,V2)
     */
    BlockDeviceSync(GetStoreInfo2(), GetStoreInfo1(), SyncMode::SYNC_MODE_PUSH_PULL, OK);
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK1V1(), OK));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK2V2(), OK));
}

/**
 * @tc.name: KVPermissionSync007
 * @tc.desc: Test push pull ok with process callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev2 sync to dev1
     * @tc.expected: step1. sync ok and dev2 only exist (K2,V2)
     */
    auto fromStore = GetDelegate(GetStoreInfo2());
    ASSERT_NE(fromStore, nullptr);
    std::map<std::string, DeviceSyncProcess> syncProcessMap;
    DeviceSyncOption option;
    option.devices = {DEVICE_A};
    option.mode = SYNC_MODE_PULL_ONLY;
    option.query = Query::Select().PrefixKey({'k'});
    option.isQuery = true;
    option.isWait = false;
    DistributedDBToolsUnitTest tool;
    DBStatus status = tool.SyncTest(fromStore, option, syncProcessMap);
    EXPECT_EQ(status, DBStatus::OK);
    for (const auto &item : syncProcessMap) {
        EXPECT_EQ(item.second.pullInfo.total, 0);
        EXPECT_EQ(item.second.pullInfo.finishedCount, 0);
    }
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo2(), DistributedDBToolsUnitTest::GetK1V1(), NOT_FOUND));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo2(), DistributedDBToolsUnitTest::GetK2V2(), OK));
}

/**
 * @tc.name: KVPermissionSync008
 * @tc.desc: Test push pull failed with permission callback.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, KVPermissionSync008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev2 sync to dev1
     * @tc.expected: step1. sync failed by permission denied
     */
    auto storeInfo1 = GetStoreInfo1();
    RuntimeConfig::SetPermissionCheckCallback([storeInfo1](const PermissionCheckParamV4 &param, uint8_t) {
        if (storeInfo1.storeId == param.storeId) {
            return false;
        }
        return true;
    });
    BlockDeviceSync(GetStoreInfo2(), storeInfo1, SyncMode::SYNC_MODE_PUSH_PULL, PERMISSION_CHECK_FORBID_SYNC);
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK1V1(), OK));
    EXPECT_NO_FATAL_FAILURE(CheckData(GetStoreInfo1(), DistributedDBToolsUnitTest::GetK2V2(), NOT_FOUND));
}

/**
 * @tc.name: MockAPITest001
 * @tc.desc: Test kvStoreNbDelegate api.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, MockAPITest001, TestSize.Level0)
{
    class VirtualKvStoreNbDelegate : public KvStoreNbDelegateImpl {
    public:
        VirtualKvStoreNbDelegate() : KvStoreNbDelegateImpl(nullptr, "store")
        {
        }

        DBStatus SetProperty(const Property &property) override
        {
            return KvStoreNbDelegate::SetProperty(property);
        }
    };
    VirtualKvStoreNbDelegate store;
    EXPECT_EQ(store.SetProperty({}), OK);
}

/**
 * @tc.name: InvalidArgs001
 * @tc.desc: Test invalid use api.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, InvalidArgs001, TestSize.Level0)
{
    KvStoreNbDelegateImpl store(nullptr, "store");
    EXPECT_EQ(store.SetProperty({}), DB_ERROR);
}

/**
 * @tc.name: InvalidArgs002
 * @tc.desc: Test invalid use api.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvPermissionSyncTest, InvalidArgs002, TestSize.Level0)
{
    class VirtualConnection : public SQLiteSingleVerNaturalStoreConnection {
    public:
        VirtualConnection() : SQLiteSingleVerNaturalStoreConnection(nullptr)
        {
        }

        int SetProperty(const Property &property) override
        {
            return GenericKvDBConnection::SetProperty(property);
        }
    };
    VirtualConnection connection;
    EXPECT_EQ(connection.SetProperty({}), -E_INVALID_CONNECTION);
}
}