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
#include <gtest/gtest.h>

#include "db_constant.h"
#include "distributeddb_tools_unit_test.h"
#include "meta_data.h"
#include "virtual_single_ver_sync_db_Interface.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
constexpr const char *DEVICE_A = "deviceA";
constexpr const char *DEVICE_B = "deviceB";
class DistributedDBMetaDataTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
protected:
    std::shared_ptr<Metadata> metadata_ = nullptr;
    VirtualSingleVerSyncDBInterface *storage_ = nullptr;
};

void DistributedDBMetaDataTest::SetUpTestCase()
{
}

void DistributedDBMetaDataTest::TearDownTestCase()
{
}

void DistributedDBMetaDataTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    metadata_ = std::make_shared<Metadata>();
    ASSERT_NE(metadata_, nullptr);
    storage_ = new(std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_NE(storage_, nullptr);
    metadata_->Initialize(storage_);
}

void DistributedDBMetaDataTest::TearDown()
{
    metadata_ = nullptr;
    if (storage_ != nullptr) {
        delete storage_;
        storage_ = nullptr;
    }
}

/**
 * @tc.name: MetadataTest001
 * @tc.desc: Test metadata set and get ability sync mark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check ability sync finish before set mark.
     * @tc.expected: step1. Default all ability sync finish is false.
     */
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step2. Set A ability sync finish.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_A, true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step3. Set B ability sync finish.
     * @tc.expected: step3. A and B is finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_B, true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step4. Set A ability sync not finish.
     * @tc.expected: step4. A is not finish B is finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_A, false), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step5. Clear all ability sync finish.
     * @tc.expected: step5. A and B is not finish.
     */
    EXPECT_EQ(metadata_->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_B));
}

/**
 * @tc.name: MetadataTest002
 * @tc.desc: Test metadata set and get time sync mark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check time sync finish before set mark.
     * @tc.expected: step1. Default all time sync finish is false.
     */
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step2. Set A time sync finish.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_A, true), E_OK);
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step3. Set B time sync finish.
     * @tc.expected: step3. A and B is finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_B, true), E_OK);
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A));
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step4. Set A time sync not finish.
     * @tc.expected: step4. A is not finish B is finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_A, false), E_OK);
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A));
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step5. Clear all time sync finish.
     * @tc.expected: step5. A and B is not finish.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_B));
}

/**
 * @tc.name: MetadataTest003
 * @tc.desc: Test metadata set remote schema version.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check remote schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A), 0u);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_B), 0u);
    /**
     * @tc.steps: step2. Set A schema version.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(metadata_->SetRemoteSchemaVersion(DEVICE_A, SOFTWARE_VERSION_CURRENT), E_OK);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_B), 0u);
    /**
     * @tc.steps: step3. Clear all ability sync finish.
     * @tc.expected: step3. A and B version is zero.
     */
    EXPECT_EQ(metadata_->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A), 0u);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_B), 0u);
}

/**
 * @tc.name: MetadataTest004
 * @tc.desc: Test metadata set remote system time off set.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check remote schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A), 0u);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_B), 0u);
    /**
     * @tc.steps: step2. Set A schema version.
     * @tc.expected: step2. A is finish B is not finish.
     */
    const int64_t offset = 100u;
    EXPECT_EQ(metadata_->SetSystemTimeOffset(DEVICE_A, offset), E_OK);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A), offset);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_B), 0u);
    /**
     * @tc.steps: step3. Clear all time sync finish.
     * @tc.expected: step3. A and B system time offset is zero.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A), 0u);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_B), 0u);
}

/**
 * @tc.name: MetadataTest005
 * @tc.desc: Test metadata set local schema version.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check local schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    auto res = metadata_->GetLocalSchemaVersion();
    EXPECT_EQ(res.first, E_OK);
    EXPECT_EQ(res.second, 0u);
    /**
     * @tc.steps: step2. Set local schema version.
     * @tc.expected: step2. set success.
     */
    EXPECT_EQ(metadata_->SetLocalSchemaVersion(SOFTWARE_VERSION_CURRENT), E_OK);
    res = metadata_->GetLocalSchemaVersion();
    EXPECT_EQ(res.first, E_OK);
    EXPECT_EQ(res.second, SOFTWARE_VERSION_CURRENT);
}
}