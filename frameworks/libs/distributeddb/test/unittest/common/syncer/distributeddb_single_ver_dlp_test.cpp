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

#include <condition_variable>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "mock_sync_task_context.h"
#include "platform_specific.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "single_ver_data_sync_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    std::shared_ptr<std::string> g_testDir = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    const std::string DEVICE_A = "real_device";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const std::string KEY_INSTANCE_ID = "INSTANCE_ID";
    KvVirtualDevice *g_deviceB = nullptr;
    KvVirtualDevice *g_deviceC = nullptr;
    DistributedDBToolsUnitTest g_tool;

    DBStatus OpenDelegate(const std::string &dlpPath, KvStoreNbDelegate *&delegatePtr,
        KvStoreDelegateManager &mgr, bool syncDualTupleMode = false)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbPath = *g_testDir + dlpPath;
        KvStoreConfig storeConfig;
        storeConfig.dataDir = dbPath;
        OS::MakeDBDirectory(dbPath);
        mgr.SetKvStoreConfig(storeConfig);

        string dir = dbPath + "/single_ver";
        DIR* dirTmp = opendir(dir.c_str());
        if (dirTmp == nullptr) {
            OS::MakeDBDirectory(dir);
        } else {
            closedir(dirTmp);
        }

        KvStoreNbDelegate::Option option;
        option.syncDualTupleMode = syncDualTupleMode;
        DBStatus res = OK;
        mgr.GetKvStore(STORE_ID_1, option, [&delegatePtr, &res](DBStatus status, KvStoreNbDelegate *delegate) {
            delegatePtr = delegate;
            res = status;
        });
        return res;
    }

    DBStatus OpenDelegate(const std::string &dlpPath, RelationalStoreDelegate *&rdbDelegatePtr,
        RelationalStoreManager &mgr)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbDir = *g_testDir + dlpPath;
        OS::MakeDBDirectory(dbDir);
        std::string dbPath = dbDir + "/test.db";
        auto db = RelationalTestUtils::CreateDataBase(dbPath);
        EXPECT_EQ(sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        db = nullptr;
        RelationalStoreDelegate::Option option;
        return mgr.OpenStore(dbPath, STORE_ID_1, option, rdbDelegatePtr);
    }

    void CloseDelegate(KvStoreNbDelegate *&delegatePtr, KvStoreDelegateManager &mgr, std::string storeID)
    {
        if (delegatePtr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseKvStore(delegatePtr), OK);
        delegatePtr = nullptr;
        EXPECT_EQ(mgr.DeleteKvStore(storeID), OK);
    }

    void CloseDelegate(RelationalStoreDelegate *&delegatePtr, RelationalStoreManager &mgr)
    {
        if (delegatePtr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseStore(delegatePtr), OK);
        delegatePtr = nullptr;
    }
}

class MockVirtualSingleVerSyncDBInterface : public VirtualSingleVerSyncDBInterface {
public:
    MOCK_CONST_METHOD1(GetSecurityOption, int(SecurityOption &));
};

class DistributedDBSingleVerDLPTest : public testing::Test {
public:
    MockVirtualSingleVerSyncDBInterface *syncInterfaceC = nullptr;
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerDLPTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    if (g_testDir == nullptr) {
        g_testDir = std::make_shared<std::string>(testDir);
    }

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBSingleVerDLPTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (g_testDir != nullptr && DistributedDBToolsUnitTest::RemoveTestDbFiles(*g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerDLPTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    g_deviceC = new (std::nothrow) KvVirtualDevice(DEVICE_C);
    ASSERT_TRUE(g_deviceC != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);
    syncInterfaceC = new (std::nothrow) MockVirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceC != nullptr);
    ASSERT_EQ(g_deviceC->Initialize(g_communicatorAggregator, syncInterfaceC), E_OK);
}

void DistributedDBSingleVerDLPTest::TearDown(void)
{
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    if (g_deviceC != nullptr) {
        delete g_deviceC;
        g_deviceC = nullptr;
    }
    PermissionCheckCallbackV3 nullCallback = nullptr;
    RuntimeConfig::SetPermissionCheckCallback(nullCallback);
    SyncActivationCheckCallbackV2 activeCallBack = nullptr;
    RuntimeConfig::SetSyncActivationCheckCallback(activeCallBack);
    RuntimeConfig::SetPermissionConditionCallback(nullptr);
}

/**
 * @tc.name: SameDelegateTest001
 * @tc.desc: Test kv delegate open with diff instanceID.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerDLPTest, SameDelegateTest001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", delegatePtr1, mgr1), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    KvStoreDelegateManager mgr2(APP_ID, USER_ID, INSTANCE_ID_2);
    KvStoreNbDelegate *delegatePtr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp2", delegatePtr2, mgr2), OK);
    ASSERT_NE(delegatePtr2, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    delegatePtr1->Put(key1, value1);
    Key key2 = {'k', '2'};
    Value value2 = {'v', '2'};
    delegatePtr2->Put(key2, value2);

    Value value;
    EXPECT_EQ(delegatePtr1->Get(key1, value), OK);
    EXPECT_EQ(value1, value);
    EXPECT_EQ(delegatePtr2->Get(key2, value), OK);
    EXPECT_EQ(value2, value);

    EXPECT_EQ(delegatePtr1->Get(key2, value), NOT_FOUND);
    EXPECT_EQ(delegatePtr2->Get(key1, value), NOT_FOUND);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
    CloseDelegate(delegatePtr2, mgr2, STORE_ID_1);
}

/**
 * @tc.name: SameDelegateTest002
 * @tc.desc: Test rdb delegate open with diff instanceID.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerDLPTest, SameDelegateTest002, TestSize.Level1)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", rdbDelegatePtr1, mgr1), OK);
    ASSERT_NE(rdbDelegatePtr1, nullptr);

    RelationalStoreManager mgr2(APP_ID, USER_ID, INSTANCE_ID_2);
    RelationalStoreDelegate *rdbDelegatePtr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp2", rdbDelegatePtr2, mgr2), OK);
    ASSERT_NE(rdbDelegatePtr2, nullptr);

    CloseDelegate(rdbDelegatePtr1, mgr1);
    CloseDelegate(rdbDelegatePtr2, mgr2);
}

/**
 * @tc.name: DlpDelegateCRUDTest001
 * @tc.desc: Test dlp delegate crud function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerDLPTest, DlpDelegateCRUDTest001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", delegatePtr1, mgr1), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    delegatePtr1->Put(key1, value1);

    Value value;
    EXPECT_EQ(delegatePtr1->Get(key1, value), OK);
    EXPECT_EQ(value1, value);

    Value value2 = {'v', '2'};
    delegatePtr1->Put(key1, value2);
    EXPECT_EQ(delegatePtr1->Get(key1, value), OK);
    EXPECT_EQ(value2, value);

    EXPECT_EQ(delegatePtr1->Delete(key1), OK);
    EXPECT_EQ(delegatePtr1->Get(key1, value), NOT_FOUND);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: SandboxDelegateSync001
 * @tc.desc: Test dlp delegate sync function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerDLPTest, SandboxDelegateSync001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    RuntimeConfig::SetPermissionCheckCallback([](const PermissionCheckParam &param, uint8_t flag) {
        if ((flag & PermissionCheckFlag::CHECK_FLAG_RECEIVE) != 0) {
            bool res = false;
            if (param.extraConditions.find(KEY_INSTANCE_ID) != param.extraConditions.end()) {
                res = param.extraConditions.at(KEY_INSTANCE_ID) == std::to_string(INSTANCE_ID_1);
            }
            return res;
        }
        if (param.userId != USER_ID || param.appId != APP_ID || param.storeId != STORE_ID_1 ||
            (param.instanceId != INSTANCE_ID_1 && param.instanceId != 0)) {
            return false;
        }
        return true;
    });
    RuntimeConfig::SetPermissionConditionCallback([](const PermissionConditionParam &param) {
        std::map<std::string, std::string> res;
        res.emplace(KEY_INSTANCE_ID, std::to_string(INSTANCE_ID_1));
        return res;
    });

    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", delegatePtr1, mgr1), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    delegatePtr1->Put(key1, value1);

    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(delegatePtr1, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, result);
    EXPECT_TRUE(status == OK);
    EXPECT_EQ(result[DEVICE_B], OK);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: SingleVerUtilsTest001
 * @tc.desc: Test single verification utils function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lg
 */
HWTEST_F(DistributedDBSingleVerDLPTest, SingleVerUtilsTest001, TestSize.Level0)
{
    bool isCheckStatus;
    EXPECT_EQ(SingleVerDataSyncUtils::QuerySyncCheck(nullptr, isCheckStatus), -E_INVALID_ARGS);
    MockSyncTaskContext context;
    context.SetQuerySync(true);
    context.SetRemoteSoftwareVersion(SOFTWARE_VERSION_RELEASE_1_0);
    EXPECT_EQ(SingleVerDataSyncUtils::QuerySyncCheck(&context, isCheckStatus), OK);
    context.SetRemoteSoftwareVersion(SOFTWARE_VERSION_RELEASE_3_0);
    EXPECT_EQ(SingleVerDataSyncUtils::QuerySyncCheck(&context, isCheckStatus), OK);
    EXPECT_EQ(SingleVerDataSyncUtils::RequestQueryCheck(nullptr, nullptr), -E_INVALID_ARGS);

    std::string deviceId;
    SecurityOption option;
    EXPECT_EQ(SingleVerDataSyncUtils::IsPermitRemoteDeviceRecvData(deviceId, option, nullptr), false);
    EXPECT_CALL(*syncInterfaceC, GetSecurityOption(testing::_)).WillOnce(testing::Return(-E_INVALID_DB));
    EXPECT_EQ(SingleVerDataSyncUtils::IsPermitRemoteDeviceRecvData(deviceId, option, syncInterfaceC), false);
    EXPECT_CALL(*syncInterfaceC, GetSecurityOption(testing::_)).WillRepeatedly(testing::Return(E_OK));
    EXPECT_EQ(SingleVerDataSyncUtils::IsPermitRemoteDeviceRecvData(deviceId, option, syncInterfaceC), false);
    std::vector<SendDataItem> outData = {nullptr};
    SingleVerDataSyncUtils::TransDbDataItemToSendDataItem(deviceId, outData);
    SingleVerDataSyncUtils::TransSendDataItemToLocal(&context, deviceId, outData);
    int errNo = E_OK;
    auto communicator = g_communicatorAggregator->AllocCommunicator(12345, errNo);
    EXPECT_EQ(SingleVerDataSyncUtils::CheckPermitReceiveData(&context, communicator, syncInterfaceC), false);
    EXPECT_CALL(*syncInterfaceC, GetSecurityOption(testing::_)).WillRepeatedly(testing::Return(-E_NOT_SUPPORT));
    EXPECT_EQ(SingleVerDataSyncUtils::CheckPermitReceiveData(&context, communicator, syncInterfaceC), true);
    option.securityLabel = SecurityLabel::S0;
    context.SetRemoteSeccurityOption(option);
    EXPECT_CALL(*syncInterfaceC, GetSecurityOption(testing::_))
        .WillRepeatedly(testing::Invoke([](SecurityOption &option) {
            option.securityLabel = SecurityLabel::S1;
            return E_OK;
        }));
    EXPECT_EQ(SingleVerDataSyncUtils::CheckPermitReceiveData(&context, communicator, syncInterfaceC), true);
    EXPECT_CALL(*syncInterfaceC, GetSecurityOption(testing::_))
        .WillRepeatedly(testing::Invoke([](SecurityOption &option) {
            option.securityLabel = SecurityLabel::S1;
            return E_OK;
        }));
    option.securityLabel = FAILED_GET_SEC_CLASSIFICATION;
    context.SetRemoteSeccurityOption(option);
    EXPECT_EQ(SingleVerDataSyncUtils::CheckPermitReceiveData(&context, communicator, syncInterfaceC), false);
    EXPECT_CALL(*syncInterfaceC, GetSecurityOption(testing::_)).Times(testing::AtLeast(0));
    g_communicatorAggregator->ReleaseCommunicator(communicator);
}

/**
 * @tc.name: SingleVerUtilsTest002
 * @tc.desc: Test single verification utils function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lg
 */
HWTEST_F(DistributedDBSingleVerDLPTest, SingleVerUtilsTest002, TestSize.Level0)
{
    MockSyncTaskContext context;
    context.SetRemoteSoftwareVersion(SOFTWARE_VERSION_RELEASE_3_0);
    context.SetTaskErrCode(-E_EKEYREVOKED);
    context.SetMode(SyncModeType::PUSH_AND_PULL);
    SingleVerDataSyncUtils::PushAndPullKeyRevokHandle(&context);
    EXPECT_EQ(SingleVerDataSyncUtils::GetReSendMode(SyncModeType::RESPONSE_PULL, 1, SyncType::QUERY_SYNC_TYPE),
        SyncModeType::QUERY_PUSH);
    EXPECT_EQ(SingleVerDataSyncUtils::GetReSendMode(SyncModeType::QUERY_PUSH_PULL, 1, SyncType::QUERY_SYNC_TYPE),
        SyncModeType::QUERY_PUSH_PULL);
    EXPECT_EQ(SingleVerDataSyncUtils::GetReSendMode(SyncModeType::QUERY_PUSH_PULL, 0, SyncType::QUERY_SYNC_TYPE),
        SyncModeType::QUERY_PUSH);
    EXPECT_EQ(
        SingleVerDataSyncUtils::GetControlCmdType(SyncModeType::QUERY_PUSH_PULL), ControlCmdType::INVALID_CONTROL_CMD);
    EXPECT_EQ(SingleVerDataSyncUtils::GetModeByControlCmdType(ControlCmdType::INVALID_CONTROL_CMD),
        SyncModeType::INVALID_MODE);
    EXPECT_EQ(SingleVerDataSyncUtils::GetModeByControlCmdType(ControlCmdType::UNSUBSCRIBE_QUERY_CMD),
        SyncModeType::UNSUBSCRIBE_QUERY);
    QuerySyncObject query;
    EXPECT_EQ(SingleVerDataSyncUtils::IsNeedTriggerQueryAutoSync(nullptr, query), false);
    Message msg;
    msg.SetMessageId(CONTROL_SYNC_MESSAGE);
    msg.SetMessageType(TYPE_REQUEST);
    EXPECT_EQ(SingleVerDataSyncUtils::IsNeedTriggerQueryAutoSync(&msg, query), false);
    std::vector<SendDataItem> inData = {nullptr};
    SingleVerDataSyncUtils::GetMaxSendDataTime(inData);
    UpdateWaterMark isUpdate;
    auto kvEnv = new (std::nothrow) GenericSingleVerKvEntry();
    inData.push_back(kvEnv);
    SingleVerDataSyncUtils::GetFullSyncDataTimeRange(inData, 100, isUpdate);
    SingleVerDataSyncUtils::GetQuerySyncDataTimeRange(inData, 100, 100, isUpdate);
    DataItem dataItem;
    dataItem.flag = DataItem::DELETE_FLAG;
    kvEnv->SetEntryData(std::move(dataItem));
    SingleVerDataSyncUtils::GetQuerySyncDataTimeRange(inData, 100, 100, isUpdate);
    delete kvEnv;
}

/**
 * @tc.name: SandboxDelegateSync002
 * @tc.desc: Test dlp delegate sync active if callback return true.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerDLPTest, SandboxDelegateSync002, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    RuntimeConfig::SetSyncActivationCheckCallback([](const ActivationCheckParam &param) {
        if (param.userId == USER_ID && param.appId == APP_ID && param.storeId == STORE_ID_1 &&
            param.instanceId == INSTANCE_ID_1) {
            return true;
        }
        return false;
    });

    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", delegatePtr1, mgr1, true), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(delegatePtr1, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, result);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result[DEVICE_B], OK);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
}