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
#include <gtest/gtest.h>

#include <utility>
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_data_utils.h"
#include "cloud/cloud_db_types.h"
#include "cloud/cloud_db_proxy.h"
#include "cloud/cloud_sync_utils.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_errno.h"
#include "mock_icloud_sync_storage_interface.h"
#include "virtual_cloud_db.h"
#include "virtual_cloud_syncer.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;

namespace {
constexpr const char *TABLE_NAME = "Table";
std::vector<Field> GetFields()
{
    return {
        {
            .colName = "col1",
            .type = TYPE_INDEX<int64_t>,
            .primary = true,
            .nullable = false
        },
        {
            .colName = "col2",
            .type = TYPE_INDEX<std::string>,
            .primary = false
        },
        {
            .colName = "col3",
            .type = TYPE_INDEX<Bytes>,
            .primary = false
        }
    };
}

void ModifyRecords(std::vector<VBucket> &expectRecord)
{
    std::vector<VBucket> tempRecord;
    for (const auto &record: expectRecord) {
        VBucket bucket;
        for (auto &[field, val] : record) {
            LOGD("modify field %s", field.c_str());
            if (val.index() == TYPE_INDEX<int64_t>) {
                int64_t v = std::get<int64_t>(val);
                bucket.insert({ field, static_cast<int64_t>(v + 1) });
            } else {
                bucket.insert({ field, val });
            }
        }
        tempRecord.push_back(bucket);
    }
    expectRecord = tempRecord;
}

DBStatus Sync(CloudSyncer *cloudSyncer, int &callCount)
{
    std::mutex processMutex;
    std::condition_variable cv;
    SyncProcess syncProcess;
    const auto callback = [&callCount, &syncProcess, &processMutex, &cv](
        const std::map<std::string, SyncProcess> &process) {
        {
            std::lock_guard<std::mutex> autoLock(processMutex);
            syncProcess = process.begin()->second;
            if (!process.empty()) {
                syncProcess = process.begin()->second;
            } else {
                SyncProcess tmpProcess;
                syncProcess = tmpProcess;
            }
            callCount++;
        }
        cv.notify_all();
    };
    EXPECT_EQ(cloudSyncer->Sync({ "cloud" }, SyncMode::SYNC_MODE_CLOUD_MERGE, { TABLE_NAME }, callback, 0), E_OK);
    {
        LOGI("begin to wait sync");
        std::unique_lock<std::mutex> uniqueLock(processMutex);
        cv.wait(uniqueLock, [&syncProcess]() {
            return syncProcess.process == ProcessStatus::FINISHED;
        });
        LOGI("end to wait sync");
    }
    return syncProcess.errCode;
}

class DistributedDBCloudDBProxyTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

protected:
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
};

void DistributedDBCloudDBProxyTest::SetUpTestCase()
{
}

void DistributedDBCloudDBProxyTest::TearDownTestCase()
{
}

void DistributedDBCloudDBProxyTest::SetUp()
{
    DistributedDBUnitTest::DistributedDBToolsUnitTest::PrintTestCaseInfo();
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
}

void DistributedDBCloudDBProxyTest::TearDown()
{
    virtualCloudDb_ = nullptr;
}

/**
 * @tc.name: CloudDBProxyTest001
 * @tc.desc: Verify cloud db init and close function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudDBProxyTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloud db to proxy
     * @tc.expected: step1. E_OK
     */
    CloudDBProxy proxy;
    proxy.SetCloudDB(virtualCloudDb_);
    /**
     * @tc.steps: step2. proxy close cloud db with cloud error
     * @tc.expected: step2. -E_CLOUD_ERROR
     */
    virtualCloudDb_->SetCloudError(true);
    EXPECT_EQ(proxy.Close(), -E_CLOUD_ERROR);
    /**
     * @tc.steps: step3. proxy close cloud db again
     * @tc.expected: step3. E_OK because cloud db has been set nullptr
     */
    EXPECT_EQ(proxy.Close(), E_OK);
    virtualCloudDb_->SetCloudError(false);
    EXPECT_EQ(proxy.Close(), E_OK);
}

/**
 * @tc.name: CloudDBProxyTest002
 * @tc.desc: Verify cloud db insert function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudDBProxyTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloud db to proxy
     * @tc.expected: step1. E_OK
     */
    CloudDBProxy proxy;
    proxy.SetCloudDB(virtualCloudDb_);
    /**
     * @tc.steps: step2. insert data to cloud db
     * @tc.expected: step2. OK
     */
    TableSchema schema = {
        .name = TABLE_NAME,
        .sharedTableName = "",
        .fields = GetFields()
    };
    std::vector<VBucket> expectRecords = CloudDBDataUtils::GenerateRecords(10, schema); // generate 10 records
    std::vector<VBucket> expectExtends = CloudDBDataUtils::GenerateExtends(10); // generate 10 extends
    Info uploadInfo;
    std::vector<VBucket> insert = expectRecords;
    EXPECT_EQ(proxy.BatchInsert(TABLE_NAME, insert, expectExtends, uploadInfo), OK);

    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::string("");
    std::vector<VBucket> actualRecords;
    EXPECT_EQ(proxy.Query(TABLE_NAME, extend, actualRecords), -E_QUERY_END);
    /**
     * @tc.steps: step3. proxy query data
     * @tc.expected: step3. data is equal to expect
     */
    ASSERT_EQ(actualRecords.size(), expectRecords.size());
    for (size_t i = 0; i < actualRecords.size(); ++i) {
        for (const auto &field: schema.fields) {
            Type expect = expectRecords[i][field.colName];
            Type actual = actualRecords[i][field.colName];
            EXPECT_EQ(expect.index(), actual.index());
        }
    }
    /**
     * @tc.steps: step4. proxy close cloud db
     * @tc.expected: step4. E_OK
     */
    EXPECT_EQ(proxy.Close(), E_OK);
}

/**
 * @tc.name: CloudDBProxyTest003
 * @tc.desc: Verify cloud db update function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudDBProxyTest003, TestSize.Level0)
{
    TableSchema schema = {
        .name = TABLE_NAME,
        .sharedTableName = "",
        .fields = GetFields()
    };
    /**
     * @tc.steps: step1. set cloud db to proxy
     * @tc.expected: step1. E_OK
     */
    CloudDBProxy proxy;
    proxy.SetCloudDB(virtualCloudDb_);
    /**
     * @tc.steps: step2. insert data to cloud db
     * @tc.expected: step2. OK
     */
    std::vector<VBucket> expectRecords = CloudDBDataUtils::GenerateRecords(10, schema); // generate 10 records
    std::vector<VBucket> expectExtends = CloudDBDataUtils::GenerateExtends(10); // generate 10 extends
    Info uploadInfo;
    std::vector<VBucket> insert = expectRecords;
    EXPECT_EQ(proxy.BatchInsert(TABLE_NAME, insert, expectExtends, uploadInfo), OK);
    /**
     * @tc.steps: step3. update data to cloud db
     * @tc.expected: step3. E_OK
     */
    ModifyRecords(expectRecords);
    std::vector<VBucket> update = expectRecords;
    EXPECT_EQ(proxy.BatchUpdate(TABLE_NAME, update, expectExtends, uploadInfo), OK);
    /**
     * @tc.steps: step3. proxy close cloud db
     * @tc.expected: step3. E_OK
     */
    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::string("");
    std::vector<VBucket> actualRecords;
    EXPECT_EQ(proxy.Query(TABLE_NAME, extend, actualRecords), -E_QUERY_END);
    ASSERT_EQ(actualRecords.size(), expectRecords.size());
    for (size_t i = 0; i < actualRecords.size(); ++i) {
        for (const auto &field: schema.fields) {
            Type expect = expectRecords[i][field.colName];
            Type actual = actualRecords[i][field.colName];
            EXPECT_EQ(expect.index(), actual.index());
        }
    }
    /**
     * @tc.steps: step4. proxy close cloud db
     * @tc.expected: step4. E_OK
     */
    EXPECT_EQ(proxy.Close(), E_OK);
}

/**
 * @tc.name: CloudDBProxyTest005
 * @tc.desc: Verify sync failed after cloud error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudDBProxyTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloud db to proxy and sleep 5s when download
     * @tc.expected: step1. E_OK
     */
    auto iCloud = std::make_shared<MockICloudSyncStorageInterface>();
    auto cloudSyncer = new(std::nothrow) VirtualCloudSyncer(StorageProxy::GetCloudDb(iCloud.get()));
    EXPECT_CALL(*iCloud, StartTransaction).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*iCloud, Commit).WillRepeatedly(testing::Return(E_OK));
    ASSERT_NE(cloudSyncer, nullptr);
    cloudSyncer->SetCloudDB(virtualCloudDb_);
    cloudSyncer->SetSyncAction(true, false);
    virtualCloudDb_->SetCloudError(true);
    /**
     * @tc.steps: step2. call sync and wait sync finish
     * @tc.expected: step2. CLOUD_ERROR by lock error
     */
    int callCount = 0;
    EXPECT_EQ(Sync(cloudSyncer, callCount), CLOUD_ERROR);
    /**
     * @tc.steps: step3. get cloud lock status and heartbeat count
     * @tc.expected: step3. cloud is unlock and no heartbeat
     */
    EXPECT_FALSE(virtualCloudDb_->GetLockStatus());
    EXPECT_GE(virtualCloudDb_->GetHeartbeatCount(), 0);
    virtualCloudDb_->ClearHeartbeatCount();
    cloudSyncer->Close();
    RefObject::KillAndDecObjRef(cloudSyncer);
}

/**
 * @tc.name: CloudDBProxyTest008
 * @tc.desc: Verify cloud db heartbeat with diff status.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudDBProxyTest008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloud db to proxy
     * @tc.expected: step1. E_OK
     */
    CloudDBProxy proxy;
    proxy.SetCloudDB(virtualCloudDb_);
    /**
     * @tc.steps: step2. proxy heartbeat with diff status
     */
    virtualCloudDb_->SetActionStatus(CLOUD_NETWORK_ERROR);
    int errCode = proxy.HeartBeat();
    EXPECT_EQ(errCode, -E_CLOUD_NETWORK_ERROR);
    EXPECT_EQ(TransferDBErrno(errCode), CLOUD_NETWORK_ERROR);

    virtualCloudDb_->SetActionStatus(CLOUD_SYNC_UNSET);
    errCode = proxy.HeartBeat();
    EXPECT_EQ(errCode, -E_CLOUD_SYNC_UNSET);
    EXPECT_EQ(TransferDBErrno(errCode), CLOUD_SYNC_UNSET);

    virtualCloudDb_->SetActionStatus(CLOUD_FULL_RECORDS);
    errCode = proxy.HeartBeat();
    EXPECT_EQ(errCode, -E_CLOUD_FULL_RECORDS);
    EXPECT_EQ(TransferDBErrno(errCode), CLOUD_FULL_RECORDS);

    virtualCloudDb_->SetActionStatus(CLOUD_LOCK_ERROR);
    errCode = proxy.HeartBeat();
    EXPECT_EQ(errCode, -E_CLOUD_LOCK_ERROR);
    EXPECT_EQ(TransferDBErrno(errCode), CLOUD_LOCK_ERROR);

    virtualCloudDb_->SetActionStatus(DB_ERROR);
    errCode = proxy.HeartBeat();
    EXPECT_EQ(errCode, -E_CLOUD_ERROR);
    EXPECT_EQ(TransferDBErrno(errCode), CLOUD_ERROR);

    /**
     * @tc.steps: step3. proxy close cloud db
     * @tc.expected: step3. E_OK
     */
    EXPECT_EQ(proxy.Close(), E_OK);
}

/**
 * @tc.name: CloudDBProxyTest009
 * @tc.desc: Verify cloud db closed and current task exit .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudDBProxyTest009, TestSize.Level3)
{
    /**
     * @tc.steps: step1. set cloud db to proxy and sleep 5s when download
     * @tc.expected: step1. E_OK
     */
    auto iCloud = std::make_shared<MockICloudSyncStorageInterface>();
    ASSERT_NE(iCloud, nullptr);
    EXPECT_CALL(*iCloud, Commit).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*iCloud, StartTransaction).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*iCloud, Rollback).WillRepeatedly(testing::Return(E_OK));
    auto cloudSyncer = new(std::nothrow) VirtualCloudSyncer(StorageProxy::GetCloudDb(iCloud.get()));
    ASSERT_NE(cloudSyncer, nullptr);
    cloudSyncer->SetCloudDB(virtualCloudDb_);
    cloudSyncer->SetSyncAction(true, false);
    cloudSyncer->SetDownloadFunc([]() {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5s
        return -E_CLOUD_ERROR;
    });
    /**
     * @tc.steps: step2. call sync and wait sync finish
     * @tc.expected: step2. E_OK
     */
    std::mutex processMutex;
    bool finished = false;
    std::condition_variable cv;
    LOGI("[CloudDBProxyTest009] Call cloud sync");
    const auto callback = [&finished, &processMutex, &cv](const std::map<std::string, SyncProcess> &process) {
        {
            std::lock_guard<std::mutex> autoLock(processMutex);
            for (const auto &item: process) {
                if (item.second.process == DistributedDB::FINISHED) {
                    finished = true;
                    EXPECT_EQ(item.second.errCode, DB_CLOSED);
                }
            }
        }
        cv.notify_all();
    };
    EXPECT_EQ(cloudSyncer->Sync({ "cloud" }, SyncMode::SYNC_MODE_CLOUD_MERGE, { TABLE_NAME }, callback, 0), E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cloudSyncer->Close();
    {
        LOGI("[CloudDBProxyTest009] begin to wait sync");
        std::unique_lock<std::mutex> uniqueLock(processMutex);
        cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT), [&finished]() {
            return finished;
        });
        LOGI("[CloudDBProxyTest009] end to wait sync");
    }
    RefObject::KillAndDecObjRef(cloudSyncer);
}

/**
 * @tc.name: CloudDBProxyTest010
 * @tc.desc: Verify cloud db lock with diff status.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudDBProxyTest010, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloud db to proxy
     * @tc.expected: step1. E_OK
     */
    CloudDBProxy proxy;
    proxy.SetCloudDB(virtualCloudDb_);
    /**
     * @tc.steps: step2. proxy lock with diff status
     */
    virtualCloudDb_->SetActionStatus(CLOUD_NETWORK_ERROR);
    auto ret = proxy.Lock();
    EXPECT_EQ(ret.first, -E_CLOUD_NETWORK_ERROR);
    EXPECT_EQ(TransferDBErrno(ret.first), CLOUD_NETWORK_ERROR);

    virtualCloudDb_->SetActionStatus(CLOUD_LOCK_ERROR);
    ret = proxy.Lock();
    EXPECT_EQ(ret.first, -E_CLOUD_LOCK_ERROR);
    EXPECT_EQ(TransferDBErrno(ret.first), CLOUD_LOCK_ERROR);
    /**
     * @tc.steps: step3. proxy close cloud db
     * @tc.expected: step3. E_OK
     */
    EXPECT_EQ(proxy.Close(), E_OK);
}

/**
 * @tc.name: CloudSyncQueue001
 * @tc.desc: Verify sync task count decrease after sync finished.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudSyncQueue001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. set cloud db to proxy and sleep 5s when download
     * @tc.expected: step1. E_OK
     */
    auto iCloud = std::make_shared<MockICloudSyncStorageInterface>();
    ASSERT_NE(iCloud, nullptr);
    auto cloudSyncer = new(std::nothrow) VirtualCloudSyncer(StorageProxy::GetCloudDb(iCloud.get()));
    ASSERT_NE(cloudSyncer, nullptr);
    EXPECT_CALL(*iCloud, Rollback).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*iCloud, Commit).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*iCloud, StartTransaction).WillRepeatedly(testing::Return(E_OK));
    cloudSyncer->SetCloudDB(virtualCloudDb_);
    cloudSyncer->SetSyncAction(true, false);
    cloudSyncer->SetDownloadFunc([cloudSyncer]() {
        EXPECT_EQ(cloudSyncer->GetQueueCount(), 1u);
        std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep 2s
        return E_OK;
    });
    /**
     * @tc.steps: step2. call sync and wait sync finish
     */
    int callCount = 0;
    EXPECT_EQ(Sync(cloudSyncer, callCount), OK);
    RuntimeContext::GetInstance()->StopTaskPool();
    EXPECT_EQ(callCount, 1);
    RefObject::KillAndDecObjRef(cloudSyncer);
}

/**
 * @tc.name: CloudSyncerTest001
 * @tc.desc: Verify syncer notify by queue schedule.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, CloudSyncerTest001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. set cloud db to proxy
     * @tc.expected: step1. E_OK
     */
    auto iCloud = std::make_shared<MockICloudSyncStorageInterface>();
    EXPECT_CALL(*iCloud, StartTransaction).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*iCloud, Commit).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*iCloud, GetIdentify).WillRepeatedly(testing::Return("CloudSyncerTest001"));
    auto cloudSyncer = new(std::nothrow) VirtualCloudSyncer(StorageProxy::GetCloudDb(iCloud.get()));
    std::atomic<int> callCount = 0;
    std::condition_variable cv;
    cloudSyncer->SetCurrentTaskInfo([&callCount, &cv](const std::map<std::string, SyncProcess> &) {
        callCount++;
        int before = callCount;
        LOGD("on callback %d", before);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        EXPECT_EQ(before, callCount);
        cv.notify_all();
    }, 1u);
    const int notifyCount = 2;
    for (int i = 0; i < notifyCount; ++i) {
        cloudSyncer->Notify();
    }
    cloudSyncer->SetCurrentTaskInfo(nullptr, 0); // 0 is invalid task id
    std::mutex processMutex;
    std::unique_lock<std::mutex> uniqueLock(processMutex);
    cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT), [&callCount]() {
        return callCount == notifyCount;
    });
    cloudSyncer->Close();
    RefObject::KillAndDecObjRef(cloudSyncer);
}

/**
 * @tc.name: SameBatchTest001
 * @tc.desc: Verify update cache in same batch.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, SameBatchTest001, TestSize.Level0)
{
    std::map<std::string, LogInfo> localLogInfoCache;
    LogInfo cloudInfo;
    LogInfo localInfo;
    localInfo.hashKey = {'k'};
    cloudInfo.cloudGid = "gid";
    /**
     * @tc.steps: step1. insert cloud into local
     * @tc.expected: step1. local cache has gid
     */
    CloudSyncUtils::UpdateLocalCache(OpType::INSERT, cloudInfo, localInfo, localLogInfoCache);
    std::string hashKey(localInfo.hashKey.begin(), localInfo.hashKey.end());
    EXPECT_EQ(localLogInfoCache[hashKey].cloudGid, cloudInfo.cloudGid);
    /**
     * @tc.steps: step2. delete local
     * @tc.expected: step2. local flag is delete
     */
    CloudSyncUtils::UpdateLocalCache(OpType::DELETE, cloudInfo, localInfo, localLogInfoCache);
    EXPECT_EQ(localLogInfoCache[hashKey].flag, static_cast<uint64_t>(LogInfoFlag::FLAG_DELETE));
}

/**
 * @tc.name: SameBatchTest002
 * @tc.desc: Verify cal opType in same batch.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudDBProxyTest, SameBatchTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare two data with same pk
     */
    ICloudSyncer::SyncParam param;
    param.downloadData.opType.push_back(OpType::INSERT);
    param.downloadData.opType.push_back(OpType::UPDATE);
    const std::string pkField = "pk";
    param.changedData.field.push_back(pkField);
    VBucket oneRow;
    oneRow[pkField] = static_cast<int64_t>(1); // 1 is pk
    param.downloadData.data.push_back(oneRow);
    param.downloadData.data.push_back(oneRow);
    /**
     * @tc.steps: step2. cal opType by utils
     * @tc.expected: step2. all type should be INSERT
     */
    for (size_t i = 0; i < param.downloadData.data.size(); ++i) {
        EXPECT_EQ(CloudSyncUtils::CalOpType(param, i), OpType::INSERT);
    }
    /**
     * @tc.steps: step3. cal opType by utils
     * @tc.expected: step3. should be UPDATE because diff pk
     */
    oneRow[pkField] = static_cast<int64_t>(2); // 2 is pk
    param.downloadData.data.push_back(oneRow);
    param.downloadData.opType.push_back(OpType::UPDATE);
    // index start with zero
    EXPECT_EQ(CloudSyncUtils::CalOpType(param, param.downloadData.data.size() - 1), OpType::UPDATE);
}
}