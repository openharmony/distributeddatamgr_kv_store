/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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


#include "cloud_syncer_test.h"
#include "distributeddb_tools_unit_test.h"
#include "mock_iclouddb.h"
#include "mock_icloud_sync_storage_interface.h"


using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBCloudSyncerProgressManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBCloudSyncerProgressManagerTest::SetUpTestCase(void)
{
}

void DistributedDBCloudSyncerProgressManagerTest::TearDownTestCase(void)
{
}

void DistributedDBCloudSyncerProgressManagerTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBCloudSyncerProgressManagerTest::TearDown(void)
{
}

/**
 * @tc.name: SyncerMgrCheck001
 * @tc.desc: Test case1 about Synchronization parameter
 * @tc.type: FUNC
 * @tc.require: SR000HPUOS
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudSyncerProgressManagerTest, SyncerMgrCheck001, TestSize.Level1)
{
    // Synchronization parameter checks
    MockICloudSyncStorageInterface *iCloud = new MockICloudSyncStorageInterface();
    std::shared_ptr<TestStorageProxy> storageProxy = std::make_shared<TestStorageProxy>(iCloud);
    TestCloudSyncer cloudSyncer(storageProxy);
    std::shared_ptr<MockICloudDB> idb = std::make_shared<MockICloudDB>();
    cloudSyncer.SetMockICloudDB(idb);
    std::vector<DeviceID> devices = {"cloud"};
    std::vector<std::string> tables = {"TestTableA", "TestTableB" };
    
    // check different sync mode
    cloudSyncer.InitCloudSyncerForSync();

    EXPECT_CALL(*idb, Query(_, _, _)).WillRepeatedly(Return(QUERY_END));
    EXPECT_CALL(*idb, BatchInsert(_, _, _)).WillRepeatedly(Return(OK));
    EXPECT_CALL(*idb, HeartBeat()).WillRepeatedly(Return(OK));
    EXPECT_CALL(*idb, Lock()).WillRepeatedly(Return(std::pair<DBStatus, uint32_t>(OK, 10)));
    EXPECT_CALL(*idb, UnLock()).WillRepeatedly(Return(OK));
    EXPECT_CALL(*iCloud, StartTransaction(_)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, PutCloudSyncData(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetMetaData(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, ChkSchema(_)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, Commit()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetUploadCount(_, _, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetCloudTableSchema(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetCloudData(_, _, _, _)).WillRepeatedly(Return(E_OK));

    SyncProcess res;
    int errCode = cloudSyncer.Sync(devices, SYNC_MODE_CLOUD_FORCE_PUSH, tables, [&res](
        const std::map<std::string, SyncProcess> &process) {
        res = process.begin()->second;
    }, 5000);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(res.process, FINISHED);

    TestCloudSyncer cloudSyncer2(storageProxy);
    cloudSyncer2.InitCloudSyncerForSync();
    cloudSyncer2.SetMockICloudDB(idb);
    errCode = cloudSyncer2.Sync(devices, SYNC_MODE_CLOUD_FORCE_PULL, tables, [&res](
        const std::map<std::string, SyncProcess> &process) {
        res = process.begin()->second;
    }, 5000);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(res.process, FINISHED);

    RuntimeContext::GetInstance()->StopTaskPool();
    storageProxy.reset();
    delete iCloud;
}

/**
 * @tc.name: SyncerMgrCheck002
 * @tc.desc: Test case2 about Synchronization parameter
 * @tc.type: FUNC
 * @tc.require: SR000HPUOS
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudSyncerProgressManagerTest, SyncerMgrCheck002, TestSize.Level1)
{
    MockICloudSyncStorageInterface *iCloud = new MockICloudSyncStorageInterface();
    std::shared_ptr<TestStorageProxy> storageProxy = std::make_shared<TestStorageProxy>(iCloud);
    std::shared_ptr<MockICloudDB> idb = std::make_shared<MockICloudDB>();
    TestCloudSyncer cloudSyncer3(storageProxy);
    cloudSyncer3.SetMockICloudDB(idb);
    cloudSyncer3.InitCloudSyncerForSync();
    
    std::vector<DeviceID> devices = {"cloud"};
    std::vector<std::string> tables = {"TestTableA", "TestTableB" };
    EXPECT_CALL(*idb, Query(_, _, _)).WillRepeatedly(Return(QUERY_END));
    EXPECT_CALL(*idb, BatchInsert(_, _, _)).WillRepeatedly(Return(OK));
    EXPECT_CALL(*idb, HeartBeat()).WillRepeatedly(Return(OK));
    EXPECT_CALL(*idb, Lock()).WillRepeatedly(Return(std::pair<DBStatus, uint32_t>(OK, 10)));
    EXPECT_CALL(*idb, UnLock()).WillRepeatedly(Return(OK));
    EXPECT_CALL(*iCloud, StartTransaction(_)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, PutCloudSyncData(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetMetaData(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, ChkSchema(_)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, Commit()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetUploadCount(_, _, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetCloudTableSchema(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetCloudData(_, _, _, _)).WillRepeatedly(Return(E_OK));

    SyncProcess res;
    int errCode = cloudSyncer3.Sync(devices, SYNC_MODE_CLOUD_MERGE, tables, [&res](
        const std::map<std::string, SyncProcess> &process) {
        res = process.begin()->second;
    }, 5000);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(res.process, FINISHED);

    TestCloudSyncer cloudSyncer4(storageProxy);
    cloudSyncer4.InitCloudSyncerForSync();
    cloudSyncer4.SetMockICloudDB(idb);
    errCode = cloudSyncer4.Sync(devices, SYNC_MODE_PULL_ONLY, tables, [&res](
        const std::map<std::string, SyncProcess> &process) {
        res = process.begin()->second;
    }, 5000);
    EXPECT_EQ(errCode, -E_NOT_SUPPORT);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(res.process, FINISHED);

    RuntimeContext::GetInstance()->StopTaskPool();
    storageProxy.reset();
    delete iCloud;
    idb = nullptr;
}

/**
 * @tc.name: SyncerMgrCheck003
 * @tc.desc: Test case2 about Synchronization parameter
 * @tc.type: FUNC
 * @tc.require: SR000HPUOS
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudSyncerProgressManagerTest, SyncerMgrCheck003, TestSize.Level1)
{
    MockICloudSyncStorageInterface *iCloud = new MockICloudSyncStorageInterface();
    std::shared_ptr<TestStorageProxy> storageProxy = std::make_shared<TestStorageProxy>(iCloud);
    TestCloudSyncer cloudSyncer5(storageProxy);
    std::shared_ptr<MockICloudDB> idb = std::make_shared<MockICloudDB>();
    cloudSyncer5.SetMockICloudDB(idb);
    cloudSyncer5.InitCloudSyncerForSync();
    
    std::vector<std::string> tables = {"TestTableA", "TestTableB" };
    EXPECT_CALL(*idb, Query(_, _, _)).WillRepeatedly(Return(QUERY_END));
    EXPECT_CALL(*idb, BatchInsert(_, _, _)).WillRepeatedly(Return(OK));
    EXPECT_CALL(*idb, HeartBeat()).WillRepeatedly(Return(OK));
    EXPECT_CALL(*idb, Lock()).WillRepeatedly(Return(std::pair<DBStatus, uint32_t>(OK, 10)));
    EXPECT_CALL(*idb, UnLock()).WillRepeatedly(Return(OK));
    EXPECT_CALL(*iCloud, StartTransaction(_)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, PutCloudSyncData(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetMetaData(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, ChkSchema(_)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, Commit()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetUploadCount(_, _, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetCloudTableSchema(_, _)).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*iCloud, GetCloudData(_, _, _, _)).WillRepeatedly(Return(E_OK));

    SyncProcess res;
    // check if device is empty
    std::vector<DeviceID> devices = {};
    int errCode = cloudSyncer5.Sync(devices, SYNC_MODE_CLOUD_MERGE, tables, [&res](
        const std::map<std::string, SyncProcess> &process) {
        res = process.begin()->second;
    }, 5000);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RuntimeContext::GetInstance()->StopTaskPool();
    storageProxy.reset();
    delete iCloud;
    idb = nullptr;
}
/**
 * @tc.name: SyncerMgrCheck004
 * @tc.desc: Test the number of queues

 * @tc.type: FUNC
 * @tc.require: SR000HPUOS
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudSyncerProgressManagerTest, SyncerMgrCheck004, TestSize.Level1)
{
    // Check the number of queues
    MockICloudSyncStorageInterface *iCloud = new MockICloudSyncStorageInterface();
    std::shared_ptr<TestStorageProxy> storageProxy = std::make_shared<TestStorageProxy>(iCloud);
    TestCloudSyncer cloudSyncer(storageProxy);
    std::shared_ptr<MockICloudDB> idb = std::make_shared<MockICloudDB>();
    cloudSyncer.SetMockICloudDB(idb);
    std::vector<std::string> tables = {"TestTableA", "TestTableB" };
    SyncProcessCallback onProcess;
    // current limit is 32;
    for (int i = 1; i <= 32; i++) {
        cloudSyncer.taskInfo_ = cloudSyncer.SetAndGetCloudTaskInfo(SYNC_MODE_CLOUD_FORCE_PUSH, tables, onProcess, 5000);
        int errCode = cloudSyncer.CallTryToAddSyncTask(std::move(cloudSyncer.taskInfo_));
        EXPECT_EQ(errCode, E_OK);
    }
    cloudSyncer.taskInfo_ = cloudSyncer.SetAndGetCloudTaskInfo(SYNC_MODE_CLOUD_FORCE_PUSH, tables, onProcess, 5000);
    int errCode = cloudSyncer.CallTryToAddSyncTask(std::move(cloudSyncer.taskInfo_));
    EXPECT_EQ(errCode, -E_BUSY);
    
    cloudSyncer.PopTaskQueue();
    cloudSyncer.PopTaskQueue();

    // After pop task from taskQueue, it should be ok to call TryToAddSyncTask
    cloudSyncer.taskInfo_ = cloudSyncer.SetAndGetCloudTaskInfo(SYNC_MODE_CLOUD_FORCE_PUSH, tables, onProcess, 5000);
    errCode = cloudSyncer.CallTryToAddSyncTask(std::move(cloudSyncer.taskInfo_));
    EXPECT_EQ(errCode, E_OK);

    RuntimeContext::GetInstance()->StopTaskPool();
    storageProxy.reset();
    delete iCloud;
    idb = nullptr;
}

/**
 * @tc.name: SyncerMgrCheck005
 * @tc.desc: Test Single-threaded execution of tasks
 * @tc.type: FUNC
 * @tc.require: SR000HPUOS
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudSyncerProgressManagerTest, SyncerMgrCheck005, TestSize.Level1)
{
    // Single-threaded execution of tasks
    MockICloudSyncStorageInterface *iCloud = new MockICloudSyncStorageInterface();
    std::shared_ptr<TestStorageProxy> storageProxy = std::make_shared<TestStorageProxy>(iCloud);
    TestCloudSyncer cloudSyncer(storageProxy);
    std::shared_ptr<MockICloudDB> idb = std::make_shared<MockICloudDB>();
    cloudSyncer.SetMockICloudDB(idb);

    std::vector<string> devices = {"cloud"};
    std::vector<std::string> tables = {"TestTableA", "TestTableB" };
    
    cloudSyncer.InitCloudSyncer(0u, SYNC_MODE_CLOUD_FORCE_PUSH);
    int errCode = cloudSyncer.CreateCloudTaskInfoAndCallTryToAddSync(SYNC_MODE_CLOUD_FORCE_PUSH, tables, {}, 5000);
    errCode = cloudSyncer.CallPrepareSync(1u);
    EXPECT_EQ(errCode, E_OK);

    cloudSyncer.InitCloudSyncer(2u, SYNC_MODE_CLOUD_FORCE_PUSH);
    errCode = cloudSyncer.CallPrepareSync(2u);
    EXPECT_EQ(errCode, -E_DB_CLOSED);

    RuntimeContext::GetInstance()->StopTaskPool();
    storageProxy.reset();
    delete iCloud;
    idb = nullptr;
}
}