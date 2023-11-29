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
#include "distributeddb_tools_unit_test.h"
#include "strategy_factory.h"
#include "virtual_cloud_syncer.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;

namespace {
class DistributedDBCloudStrategyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBCloudStrategyTest::SetUpTestCase(void)
{
}

void DistributedDBCloudStrategyTest::TearDownTestCase(void)
{
}

void DistributedDBCloudStrategyTest::SetUp(void)
{
    DistributedDBUnitTest::DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBCloudStrategyTest::TearDown(void)
{
}

/**
 * @tc.name: StrategyFactoryTest001
 * @tc.desc: Verify cloud strategy build function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudStrategyTest, StrategyFactoryTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. PUSH PULL PUSH_PULL mode will get base strategy
     * @tc.expected: step1. both judge update cursor and upload got false
     */
    auto strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_PUSH_ONLY);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->JudgeUpdateCursor(), false);
    EXPECT_EQ(strategy->JudgeUpload(), false);

    strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_PULL_ONLY);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->JudgeUpdateCursor(), false);
    EXPECT_EQ(strategy->JudgeUpload(), false);

    strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_PUSH_PULL);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->JudgeUpdateCursor(), false);
    EXPECT_EQ(strategy->JudgeUpload(), false);
    /**
     * @tc.steps: step2. CLOUD_MERGE mode will get cloud merge strategy
     * @tc.expected: step2. both judge update cursor and upload got true
     */
    strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_CLOUD_MERGE);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->JudgeUpdateCursor(), true);
    EXPECT_EQ(strategy->JudgeUpload(), true);
}

/**
 * @tc.name: TagOpTyeTest001
 * @tc.desc: Verify cloud merge strategy tag operation type function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudStrategyTest, TagOpTyeTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. build cloud merge strategy
     */
    auto strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_CLOUD_MERGE);
    ASSERT_NE(strategy, nullptr);
    LogInfo localInfo;
    LogInfo cloudInfo;
    /**
     * @tc.steps: step2. local not exist cloud record
     * @tc.expected: step2. insert cloud record to local
     */
    EXPECT_EQ(strategy->TagSyncDataStatus(false, localInfo, cloudInfo), OpType::INSERT);
    /**
     * @tc.steps: step3. local record is newer and local not exist gid
     * @tc.expected: step3. only update gid
     */
    localInfo.timestamp = 1u;
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::ONLY_UPDATE_GID);
    /**
     * @tc.steps: step4. local record is newer and local exist gid
     * @tc.expected: step4. no need handle this record
     */
    localInfo.cloudGid = "gid";
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::NOT_HANDLE);
    /**
     * @tc.steps: step5. cloud record is newer
     * @tc.expected: step5. update cloud record
     */
    cloudInfo.timestamp = 2u; // mark 2 means cloud is new
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::UPDATE);
    /**
     * @tc.steps: step6. cloud record is newer and it is delete
     * @tc.expected: step6. delete cloud record
     */
    cloudInfo.flag = 0x01; // it means delete
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::DELETE);
    /**
     * @tc.steps: step7. cloud is new and local is delete
     * @tc.expected: step7 insert cloud record
     */
    cloudInfo.flag = 0; // it means no delete
    localInfo.flag = 0x01; // it means delete
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::INSERT);
    /**
     * @tc.steps: step8. cloud is new and local both delete
     * @tc.expected: step8 not handle cloud record
     */
    cloudInfo.flag = 0x01; // it means delete
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::UPDATE_TIMESTAMP);
    /**
     * @tc.steps: step9. cloud is delete and local not exist
     * @tc.expected: step9 not handle cloud record
     */
    EXPECT_EQ(strategy->TagSyncDataStatus(false, localInfo, cloudInfo), OpType::NOT_HANDLE);
    /**
     * @tc.steps: step10. cloud is old and delete, local has gid
     * @tc.expected: step10 clear gid
     */
    localInfo.timestamp = 3u; // mark 3 means local is new
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::CLEAR_GID);
    /**
     * @tc.steps: step10. cloud is old and delete, local has not gid
     * @tc.expected: step10 not handle cloud record
     */
    localInfo.cloudGid = "";
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::NOT_HANDLE);
    /**
     * @tc.steps: step11. cloud has same timestamp with local, and local has empty gid
     * @tc.expected: step11 only update gid
     */
    cloudInfo.timestamp = localInfo.timestamp;
    cloudInfo.flag = 0; // it means no delete
    localInfo.flag = 0; // it means no delete
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::ONLY_UPDATE_GID);
}

/**
 * @tc.name: TagOpTyeTest002
 * @tc.desc: Verify local cover cloud strategy tag operation type function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudStrategyTest, TagOpTyeTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. build local cover cloud strategy
     */
    auto strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH);
    ASSERT_NE(strategy, nullptr);
    LogInfo localInfo;
    LogInfo cloudInfo;

    /**
     * @tc.steps: step2. local not exist cloud record
     * @tc.expected: step2. not handle
     */
    EXPECT_EQ(strategy->TagSyncDataStatus(false, localInfo, cloudInfo), OpType::NOT_HANDLE);

    /**
     * @tc.steps: step3. local has cloud record but don't have gid
     * @tc.expected: step3. only update gid
     */
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::ONLY_UPDATE_GID);

    /**
     * @tc.steps: step4. local has cloud record and have gid
     * @tc.expected: step4. not handle
     */
    localInfo.cloudGid = "gid";
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::NOT_HANDLE);
    localInfo.cloudGid = "";
    /**
     * @tc.steps: step5. local has cloud record(without gid) but cloud flag is delete
     * @tc.expected: step5. ONLY UPDATE GID
     */
    cloudInfo.flag = 0x01; // it means delete
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::NOT_HANDLE);
    /**
     * @tc.steps: step6. local has cloud record(with gid) but cloud flag is delete
     * @tc.expected: step6. CLEAR_GID
     */
    localInfo.cloudGid = "gid";
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::CLEAR_GID);
}

/**
 * @tc.name: TagOpTyeTest003
 * @tc.desc: Verify cloud cover local strategy tag operation type function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudStrategyTest, TagOpTyeTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. cloud cover local strategy
     */
    auto strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_CLOUD_FORCE_PULL);
    ASSERT_NE(strategy, nullptr);
    LogInfo localInfo;
    LogInfo cloudInfo;

    /**
     * @tc.steps: step2. local not exist cloud record(without gid) and its not deleted in cloud
     * @tc.expected: step2. insert
     */
    EXPECT_EQ(strategy->TagSyncDataStatus(false, localInfo, cloudInfo), OpType::INSERT);

    /**
     * @tc.steps: step3. local not exist cloud record and it's deleted in cloud (without gid)
     * @tc.expected: step3. not handle
     */
    localInfo.cloudGid = "";
    cloudInfo.flag = 0x01; // it means delete
    EXPECT_EQ(strategy->TagSyncDataStatus(false, localInfo, cloudInfo), OpType::NOT_HANDLE);

    /**
     * @tc.steps: step4. local not exist cloud record and it's deleted in cloud (with gid)
     * @tc.expected: step4. delete
     */
    localInfo.cloudGid = "gid";
    EXPECT_EQ(strategy->TagSyncDataStatus(false, localInfo, cloudInfo), OpType::DELETE);

    /**
     * @tc.steps: step5. local exist cloud record and its deleted in cloud
     * @tc.expected: step5. delete
     */
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::DELETE);
    localInfo.cloudGid = "";
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::DELETE);

    /**
     * @tc.steps: step6. local exist cloud record and its not deleted in cloud(WITH OR WITHOUT gid)
     * @tc.expected: step6. UPDATE
     */
    cloudInfo.flag = 0x00;
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::UPDATE);
    localInfo.cloudGid = "gid";
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::UPDATE);
}

/**
 * @tc.name: TagOpTyeTest004
 * @tc.desc: Verify cloud cover local strategy tag operation type function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudStrategyTest, TagOpTyeTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. cloud cover local strategy
     */
    auto strategy = StrategyFactory::BuildSyncStrategy(SyncMode::SYNC_MODE_CLOUD_FORCE_PULL);
    ASSERT_NE(strategy, nullptr);
    LogInfo localInfo;
    LogInfo cloudInfo;

    cloudInfo.flag = 0x01;
    localInfo.flag = 0x01;
    localInfo.cloudGid = "";
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::UPDATE_TIMESTAMP);

    cloudInfo.flag = 0x00;
    localInfo.flag = 0x01;
    EXPECT_EQ(strategy->TagSyncDataStatus(true, localInfo, cloudInfo), OpType::INSERT);
}

/**
 * @tc.name: TagOpTyeTest005
 * @tc.desc: Verify same data conflict in local.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudStrategyTest, TagOpTyeTest005, TestSize.Level0)
{
    auto syncer = new(std::nothrow) VirtualCloudSyncer(nullptr);
    ASSERT_NE(syncer, nullptr);
    DataInfoWithLog localInfo;
    LogInfo cloudInfo;
    OpType result = OpType::INSERT;
    EXPECT_EQ(syncer->CallTagStatusByStrategy(true, localInfo, cloudInfo, result), E_OK);
    EXPECT_EQ(result, OpType::NOT_HANDLE);
    localInfo.logInfo.device = "dev";
    EXPECT_EQ(syncer->CallTagStatusByStrategy(true, localInfo, cloudInfo, result), -E_INTERNAL_ERROR);
    RefObject::KillAndDecObjRef(syncer);
}
}