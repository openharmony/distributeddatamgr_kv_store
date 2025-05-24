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
#include "db_common.h"
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
constexpr const char *USER_A = "userA";
constexpr const char *USER_B = "userB";
class DistributedDBMetaDataTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    void GetMetaDataValue(const std::string &hashDeviceId, MetaDataValue &metaDataValue);
    void PutMetaDataValue(const std::string &hashDeviceId, MetaDataValue &metaDataValue);
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

void DistributedDBMetaDataTest::GetMetaDataValue(const std::string &hashDeviceId, MetaDataValue &metaDataValue)
{
    Key key;
    DBCommon::StringToVector(hashDeviceId, key);
    Value value;
    int errCode = storage_->GetMetaData(key, value);
    if (errCode == -E_NOT_FOUND) {
        return;
    }
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(memcpy_s(&metaDataValue, sizeof(MetaDataValue), value.data(), value.size()), EOK);
}

void DistributedDBMetaDataTest::PutMetaDataValue(const std::string &hashDeviceId, MetaDataValue &metaDataValue)
{
    Key key;
    DBCommon::StringToVector(hashDeviceId, key);
    Value value;
    value.resize(sizeof(MetaDataValue));
    EXPECT_EQ(memcpy_s(value.data(), value.size(), &metaDataValue, sizeof(MetaDataValue)), EOK);
    EXPECT_EQ(storage_->PutMetaData(key, value, false), E_OK);
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
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step2. Set A ability sync finish.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_A, "", true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step3. Set B ability sync finish.
     * @tc.expected: step3. A and B is finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_B, "", true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step4. Set A ability sync not finish.
     * @tc.expected: step4. A is not finish B is finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_A, "", false), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step5. Clear all time sync finish.
     * @tc.expected: step5. A and B is finish.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step6. Clear all ability sync finish.
     * @tc.expected: step6. A and B is not finish.
     */
    EXPECT_EQ(metadata_->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_B, ""));
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
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, ""));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step2. Set A time sync finish.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_A, "", true), E_OK);
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A, ""));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step3. Set B time sync finish.
     * @tc.expected: step3. A and B is finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_B, "", true), E_OK);
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A, ""));
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step4. Set A time sync not finish.
     * @tc.expected: step4. A is not finish B is finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_A, "", false), E_OK);
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, ""));
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_B, ""));
    /**
     * @tc.steps: step5. Clear all time sync finish.
     * @tc.expected: step5. A and B is not finish.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, ""));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_B, ""));
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
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, ""), 0u);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_B, ""), 0u);
    /**
     * @tc.steps: step2. Set A schema version.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(metadata_->SetRemoteSchemaVersion(DEVICE_A, "", SOFTWARE_VERSION_CURRENT), E_OK);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, ""), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_B, ""), 0u);
    /**
     * @tc.steps: step3. Clear all ability sync finish.
     * @tc.expected: step3. A and B version is zero.
     */
    EXPECT_EQ(metadata_->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, ""), 0u);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_B, ""), 0u);
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
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, ""), 0u);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_B, ""), 0u);
    /**
     * @tc.steps: step2. Set A schema version.
     * @tc.expected: step2. A is finish B is not finish.
     */
    const int64_t offset = 100u;
    EXPECT_EQ(metadata_->SetSystemTimeOffset(DEVICE_A, "", offset), E_OK);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, ""), offset);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_B, ""), 0u);
    /**
     * @tc.steps: step3. Clear all time sync finish.
     * @tc.expected: step3. A and B system time offset is zero.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, ""), 0u);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_B, ""), 0u);
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
    EXPECT_NE(res.second, 0u);
    /**
     * @tc.steps: step2. Set local schema version.
     * @tc.expected: step2. set success.
     */
    EXPECT_EQ(metadata_->SetLocalSchemaVersion(SOFTWARE_VERSION_CURRENT), E_OK);
    res = metadata_->GetLocalSchemaVersion();
    EXPECT_EQ(res.first, E_OK);
    EXPECT_EQ(res.second, SOFTWARE_VERSION_CURRENT);
}

/**
 * @tc.name: MetadataTest006
 * @tc.desc: Test metadata remove device data with reload.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set storage ability sync finish.
     * @tc.expected: step1. A is finish.
     */
    std::string hashDeviceId = DBConstant::DEVICEID_PREFIX_KEY + DBCommon::TransferHashString(DEVICE_A) +
        DBConstant::USERID_PREFIX_KEY + "";
    MetaDataValue metaDataValue;
    GetMetaDataValue(hashDeviceId, metaDataValue);
    EXPECT_EQ(metaDataValue.syncMark & static_cast<uint64_t>(SyncMark::SYNC_MARK_ABILITY_SYNC), 0u);
    metaDataValue.syncMark = static_cast<uint64_t>(SyncMark::SYNC_MARK_ABILITY_SYNC);
    PutMetaDataValue(hashDeviceId, metaDataValue);
    /**
     * @tc.steps: step2. Check ability sync finish by meta.
     * @tc.expected: step2. A is finish because meta data is loaded from db.
     */
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
    /**
     * @tc.steps: step3. Erase water mark and check again.
     * @tc.expected: step3. A is finish because meta data is loaded from db.
     */
    EXPECT_EQ(metadata_->EraseDeviceWaterMark(DEVICE_A, true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, ""));
}

/**
 * @tc.name: MetadataTest007
 * @tc.desc: Test metadata init with time change if need.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check time sync finish by meta.
     * @tc.expected: step1. B is change because of time change.
     */
    RuntimeContext::GetInstance()->SetTimeChanged(true);
    EXPECT_TRUE(metadata_->IsTimeChange(DEVICE_B, ""));
    RuntimeContext::GetInstance()->SetTimeChanged(false);
    RuntimeContext::GetInstance()->StopTimeTickMonitorIfNeed();
}

/**
 * @tc.name: MetadataTest008
 * @tc.desc: Test metadata deserialize v1 local meta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert v1 local meta data.
     * @tc.expected: step1.Insert OK.
     */
    std::string keyStr = "localMetaData";
    Key key(keyStr.begin(), keyStr.end());
    Value value;
    value.resize(Parcel::GetUInt32Len() + Parcel::GetUInt64Len());
    Parcel parcel(value.data(), value.size());
    LocalMetaData expectLocalMetaData;
    expectLocalMetaData.version = LOCAL_META_DATA_VERSION_V1;
    expectLocalMetaData.localSchemaVersion = SOFTWARE_VERSION_RELEASE_9_0;
    (void)parcel.WriteUInt32(expectLocalMetaData.version);
    (void)parcel.WriteUInt64(expectLocalMetaData.localSchemaVersion);
    ASSERT_FALSE(parcel.IsError());
    ASSERT_EQ(storage_->PutMetaData(key, value, false), E_OK);
    /**
     * @tc.steps: step2. Read v1 local meta data.
     * @tc.expected: step2.Read OK.
     */
    auto res = metadata_->GetLocalSchemaVersion();
    EXPECT_EQ(res.first, E_OK);
    EXPECT_EQ(res.second, expectLocalMetaData.localSchemaVersion);
}

/**
 * @tc.name: MetadataTest009
 * @tc.desc: Test initially saved metadata after time changed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenghuitao
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest009, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check time changed after metadata is saved initially by SetDbCreateTime.
     * @tc.expected: step1. B is change because of time change.
     */
    RuntimeContext::GetInstance()->SetTimeChanged(true);
    EXPECT_EQ(metadata_->SetDbCreateTime(DEVICE_B, "", 10u, true), E_OK);
    EXPECT_TRUE(metadata_->IsTimeChange(DEVICE_B, ""));
    RuntimeContext::GetInstance()->SetTimeChanged(false);
    RuntimeContext::GetInstance()->StopTimeTickMonitorIfNeed();
}

/**
 * @tc.name: MetadataTest010
 * @tc.desc: Test metadata set and get ability sync mark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest010, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check ability sync finish before set mark.
     * @tc.expected: step1. Default all ability sync finish is false.
     */
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step2. Set user A ability sync finish.
     * @tc.expected: step2. user A is finish user B is not finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_A, USER_A, true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step3. Set user B ability sync finish.
     * @tc.expected: step3. user A and user B is finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_A, USER_B, true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step4. Set user A ability sync not finish.
     * @tc.expected: step4. user A is not finish user B is finish.
     */
    EXPECT_EQ(metadata_->SetAbilitySyncFinishMark(DEVICE_A, USER_A, false), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step5. Clear all time sync finish.
     * @tc.expected: step5. user A and user B unchanged.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step6. Clear all ability sync finish.
     * @tc.expected: step6. user A and user B is not finish.
     */
    EXPECT_EQ(metadata_->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));
}

/**
 * @tc.name: MetadataTest011
 * @tc.desc: Test metadata set and get time sync mark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest011, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check time sync finish before set mark.
     * @tc.expected: step1. Default all time sync finish is false.
     */
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_A));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step2. Set user A time sync finish.
     * @tc.expected: step2. user A is finish user B is not finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_A, USER_A, true), E_OK);
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_A));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step3. Set user B time sync finish.
     * @tc.expected: step3. user A and user B is finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_A, USER_B, true), E_OK);
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_A));
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step4. Set user A time sync not finish.
     * @tc.expected: step4. user A is not finish user B is finish.
     */
    EXPECT_EQ(metadata_->SetTimeSyncFinishMark(DEVICE_A, USER_A, false), E_OK);
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_A));
    EXPECT_TRUE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_B));
    /**
     * @tc.steps: step5. Clear all time sync finish.
     * @tc.expected: step5. user A and user B is not finish.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_A));
    EXPECT_FALSE(metadata_->IsTimeSyncFinish(DEVICE_A, USER_B));
}

/**
 * @tc.name: MetadataTest012
 * @tc.desc: Test metadata set remote schema version.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest012, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check remote schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, USER_A), 0u);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, USER_B), 0u);
    /**
     * @tc.steps: step2. Set user A schema version.
     * @tc.expected: step2. user A is finish user B is not finish.
     */
    EXPECT_EQ(metadata_->SetRemoteSchemaVersion(DEVICE_A, USER_A, SOFTWARE_VERSION_CURRENT), E_OK);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, USER_A), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, USER_B), 0u);
    /**
     * @tc.steps: step3. Clear all ability sync finish.
     * @tc.expected: step3. user A and user B version is zero.
     */
    EXPECT_EQ(metadata_->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, USER_A), 0u);
    EXPECT_EQ(metadata_->GetRemoteSchemaVersion(DEVICE_A, USER_B), 0u);
}

/**
 * @tc.name: MetadataTest013
 * @tc.desc: Test metadata set remote system time off set.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest013, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check remote schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, USER_A), 0u);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, USER_B), 0u);
    /**
     * @tc.steps: step2. Set user A schema version.
     * @tc.expected: step2. user A is finish user B is not finish.
     */
    const int64_t offset = 100u;
    EXPECT_EQ(metadata_->SetSystemTimeOffset(DEVICE_A, USER_A, offset), E_OK);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, USER_A), offset);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, USER_B), 0u);
    /**
     * @tc.steps: step3. Clear all time sync finish.
     * @tc.expected: step3. user A and user B system time offset is zero.
     */
    EXPECT_EQ(metadata_->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, USER_A), 0u);
    EXPECT_EQ(metadata_->GetSystemTimeOffset(DEVICE_A, USER_B), 0u);
}

/**
 * @tc.name: MetadataTest014
 * @tc.desc: Test metadata remove device data with reload.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest014, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set storage ability sync finish.
     * @tc.expected: step1. user A and B finish.
     */
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_FALSE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));

    /**
     * @tc.steps: step2. Check ability sync finish by meta.
     * @tc.expected: step2. user A and B is finish because meta data is loaded from db.
     */
    std::string hashDeviceIdA = DBConstant::DEVICEID_PREFIX_KEY + DBCommon::TransferHashString(DEVICE_A) +
        DBConstant::USERID_PREFIX_KEY + USER_A;
    MetaDataValue metaDataValueA;
    GetMetaDataValue(hashDeviceIdA, metaDataValueA);
    EXPECT_EQ(metaDataValueA.syncMark & static_cast<uint64_t>(SyncMark::SYNC_MARK_ABILITY_SYNC), 0u);
    metaDataValueA.syncMark = static_cast<uint64_t>(SyncMark::SYNC_MARK_ABILITY_SYNC);
    PutMetaDataValue(hashDeviceIdA, metaDataValueA);

    std::string hashDeviceIdB = DBConstant::DEVICEID_PREFIX_KEY + DBCommon::TransferHashString(DEVICE_A) +
        DBConstant::USERID_PREFIX_KEY + USER_B;
    MetaDataValue metaDataValueB;
    GetMetaDataValue(hashDeviceIdB, metaDataValueB);
    EXPECT_EQ(metaDataValueB.syncMark & static_cast<uint64_t>(SyncMark::SYNC_MARK_ABILITY_SYNC), 0u);
    metaDataValueB.syncMark = static_cast<uint64_t>(SyncMark::SYNC_MARK_ABILITY_SYNC);
    PutMetaDataValue(hashDeviceIdB, metaDataValueB);

    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));

    /**
     * @tc.steps: step3. Erase water mark and check again.
     * @tc.expected: step3. user A and B is finish because meta data is loaded from db.
     */
    EXPECT_EQ(metadata_->EraseDeviceWaterMark(DEVICE_A, true), E_OK);
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_A));
    EXPECT_TRUE(metadata_->IsAbilitySyncFinish(DEVICE_A, USER_B));
}

/**
 * @tc.name: MetadataTest015
 * @tc.desc: Test metadata init with time change if need.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest015, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check time sync finish by meta.
     * @tc.expected: step1. user A and B is change because of time change.
     */
    RuntimeContext::GetInstance()->SetTimeChanged(true);
    EXPECT_TRUE(metadata_->IsTimeChange(DEVICE_B, USER_A));
    EXPECT_TRUE(metadata_->IsTimeChange(DEVICE_B, USER_B));
    RuntimeContext::GetInstance()->SetTimeChanged(false);
    RuntimeContext::GetInstance()->StopTimeTickMonitorIfNeed();
}

/**
 * @tc.name: MetadataTest016
 * @tc.desc: Test metaData init correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest016, TestSize.Level1)
{
    /**
    * @tc.steps: step1. init meta data
    * @tc.expected: step1. E_OK
    */
    Metadata meta;
    VirtualSingleVerSyncDBInterface storage;
    WaterMark setWaterMark = 1;
    ASSERT_EQ(meta.Initialize(&storage), E_OK);

    /**
    * @tc.steps: step2. meta save and get waterMark
    * @tc.expected: step2. expect get the same waterMark
    */
    EXPECT_EQ(meta.SaveLocalWaterMark(DEVICE_A, USER_A, setWaterMark), E_OK);
    EXPECT_EQ(meta.SaveLocalWaterMark(DEVICE_A, USER_B, setWaterMark), E_OK);
    WaterMark getWaterMark = 0;
    meta.GetLocalWaterMark(DEVICE_A, USER_A, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);
    meta.GetLocalWaterMark(DEVICE_A, USER_B, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);

    /**
    * @tc.steps: step3. init again
    * @tc.expected: step3. E_OK
    */
    Metadata anotherMeta;
    ASSERT_EQ(anotherMeta.Initialize(&storage), E_OK);

    /**
    * @tc.steps: step4. get waterMark again
    * @tc.expected: step4. expect get the same waterMark
    */
    anotherMeta.GetLocalWaterMark(DEVICE_A, USER_A, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);
    anotherMeta.GetLocalWaterMark(DEVICE_A, USER_B, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);
}

/**
 * @tc.name: MetadataTest017
 * @tc.desc: Test metaData save and get queryWaterMark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest017, TestSize.Level1)
{
    /**
     * @tc.steps: step1. save receive and send watermark
     * @tc.expected: step1. E_OK
     */
    WaterMark w1 = 1;
    EXPECT_EQ(metadata_->SetRecvQueryWaterMark("Q1", DEVICE_A, USER_A, w1), E_OK);
    EXPECT_EQ(metadata_->SetSendQueryWaterMark("Q1", DEVICE_A, USER_A, w1), E_OK);
    WaterMark w2 = 2;
    EXPECT_EQ(metadata_->SetRecvQueryWaterMark("Q1", DEVICE_A, USER_B, w2), E_OK);
    EXPECT_EQ(metadata_->SetSendQueryWaterMark("Q1", DEVICE_A, USER_B, w2), E_OK);
    /**
     * @tc.steps: step2. get receive and send watermark
     * @tc.expected: step2. E_OK and get the latest value
     */
    WaterMark w = 0;
    EXPECT_EQ(metadata_->GetRecvQueryWaterMark("Q1", DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(metadata_->GetSendQueryWaterMark("Q1", DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(metadata_->GetRecvQueryWaterMark("Q1", DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w2, w);
    EXPECT_EQ(metadata_->GetSendQueryWaterMark("Q1", DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w2, w);
    /**
     * @tc.steps: step3. set peer and local watermark
     * @tc.expected: step3. E_OK
     */
    WaterMark w3 = 3;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_A, USER_A, w3), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_A, USER_A, w3, true), E_OK);
    WaterMark w4 = 4;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_A, USER_B, w4), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_A, USER_B, w4, true), E_OK);
    /**
     * @tc.steps: step4. get receive and send watermark
     * @tc.expected: step4. E_OK and get the w1
     */
    EXPECT_EQ(metadata_->GetRecvQueryWaterMark("Q1", DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w3, w);
    EXPECT_EQ(metadata_->GetSendQueryWaterMark("Q1", DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w3, w);
    EXPECT_EQ(metadata_->GetRecvQueryWaterMark("Q1", DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w4, w);
    EXPECT_EQ(metadata_->GetSendQueryWaterMark("Q1", DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w4, w);
    /**
     * @tc.steps: step5. set peer and local watermark
     * @tc.expected: step5. E_OK
     */
    WaterMark w5 = 5;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_B, USER_A, w5), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_B, USER_A, w5, true), E_OK);
    WaterMark w6 = 6;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_B, USER_B, w6), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_B, USER_B, w6, true), E_OK);
    /**
     * @tc.steps: step6. get receive and send watermark
     * @tc.expected: step6. E_OK and get the w3
     */
    EXPECT_EQ(metadata_->GetRecvQueryWaterMark("Q2", DEVICE_B, USER_A, w), E_OK);
    EXPECT_EQ(w5, w);
    EXPECT_EQ(metadata_->GetSendQueryWaterMark("Q2", DEVICE_B, USER_A, w), E_OK);
    EXPECT_EQ(w5, w);
    EXPECT_EQ(metadata_->GetRecvQueryWaterMark("Q2", DEVICE_B, USER_B, w), E_OK);
    EXPECT_EQ(w6, w);
    EXPECT_EQ(metadata_->GetSendQueryWaterMark("Q2", DEVICE_B, USER_B, w), E_OK);
    EXPECT_EQ(w6, w);
    /**
     * @tc.steps: step7. get not exit receive and send watermark
     * @tc.expected: step7. E_OK and get the 0
     */
    EXPECT_EQ(metadata_->GetRecvQueryWaterMark("Q3", "D3", "U3", w), E_OK);
    EXPECT_EQ(w, 0u);
    EXPECT_EQ(metadata_->GetSendQueryWaterMark("Q3", "D3", "U3", w), E_OK);
    EXPECT_EQ(w, 0u);
}

/**
 * @tc.name: MetadataTest018
 * @tc.desc: Test metaData get and set dbCreateTime.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest018, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set dbCreateTime
     * @tc.expected: step1. E_OK
     */
    EXPECT_EQ(metadata_->SetDbCreateTime(DEVICE_A, USER_A, 10u, true), E_OK);
    EXPECT_EQ(metadata_->SetDbCreateTime(DEVICE_A, USER_B, 20u, true), E_OK);
    /**
     * @tc.steps: step2. check dbCreateTime
     * @tc.expected: step2. E_OK
     */
    uint64_t curDbCreatTime = 0;
    metadata_->GetDbCreateTime(DEVICE_A, USER_A, curDbCreatTime);
    EXPECT_EQ(curDbCreatTime, 10u);
    metadata_->GetDbCreateTime(DEVICE_A, USER_B, curDbCreatTime);
    EXPECT_EQ(curDbCreatTime, 20u);
    /**
     * @tc.steps: step3. user A change dbCreateTime and check
     * @tc.expected: step3. E_OK
     */
    EXPECT_EQ(metadata_->SetDbCreateTime(DEVICE_A, USER_A, 30u, true), E_OK);
    uint64_t clearDeviceDataMark = INT_MAX;
    metadata_->GetRemoveDataMark(DEVICE_A, USER_A, clearDeviceDataMark);
    EXPECT_EQ(clearDeviceDataMark, 1u);
    EXPECT_EQ(metadata_->ResetMetaDataAfterRemoveData(DEVICE_A, USER_A), E_OK);
    metadata_->GetRemoveDataMark(DEVICE_A, USER_A, clearDeviceDataMark);
    EXPECT_EQ(clearDeviceDataMark, 0u);
    metadata_->GetDbCreateTime(DEVICE_A, USER_A, curDbCreatTime);
    EXPECT_EQ(curDbCreatTime, 30u);

    /**
     * @tc.steps: step4. user B unchanged dbCreateTime and check
     * @tc.expected: step4. E_OK
     */
    metadata_->GetRemoveDataMark(DEVICE_A, USER_B, clearDeviceDataMark);
    EXPECT_EQ(clearDeviceDataMark, 0u);
    metadata_->GetDbCreateTime(DEVICE_A, USER_B, curDbCreatTime);
    EXPECT_EQ(curDbCreatTime, 20u);
}

/**
 * @tc.name: MetadataTest019
 * @tc.desc: Test metaData save and get deleteWaterMark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBMetaDataTest, MetadataTest019, TestSize.Level1)
{
    /**
     * @tc.steps: step1. save receive and send watermark
     * @tc.expected: step1. E_OK
     */
    WaterMark w1 = 1;
    EXPECT_EQ(metadata_->SetRecvDeleteSyncWaterMark(DEVICE_A, USER_A, w1), E_OK);
    EXPECT_EQ(metadata_->SetSendDeleteSyncWaterMark(DEVICE_A, USER_A, w1), E_OK);
    WaterMark w2 = 2;
    EXPECT_EQ(metadata_->SetRecvDeleteSyncWaterMark(DEVICE_A, USER_B, w2), E_OK);
    EXPECT_EQ(metadata_->SetSendDeleteSyncWaterMark(DEVICE_A, USER_B, w2), E_OK);

    /**
     * @tc.steps: step2. get receive and send watermark
     * @tc.expected: step2. E_OK and get the latest value
     */
    WaterMark w = 0;
    EXPECT_EQ(metadata_->GetRecvDeleteSyncWaterMark(DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(metadata_->GetSendDeleteSyncWaterMark(DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(metadata_->GetRecvDeleteSyncWaterMark(DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w2, w);
    EXPECT_EQ(metadata_->GetSendDeleteSyncWaterMark(DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w2, w);

    /**
     * @tc.steps: step3. set peer and local watermark
     * @tc.expected: step3. E_OK
     */
    WaterMark w3 = 3;
    WaterMark w4 = 4;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_A, USER_A, w3), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_A, USER_A, w4, true), E_OK);
    WaterMark w5 = 5;
    WaterMark w6 = 6;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_A, USER_B, w5), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_A, USER_B, w6, true), E_OK);

    /**
     * @tc.steps: step4. get receive and send watermark
     * @tc.expected: step4. E_OK and get the w1
     */
    EXPECT_EQ(metadata_->GetSendDeleteSyncWaterMark(DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w3, w);
    EXPECT_EQ(metadata_->GetRecvDeleteSyncWaterMark(DEVICE_A, USER_A, w), E_OK);
    EXPECT_EQ(w4, w);
    EXPECT_EQ(metadata_->GetSendDeleteSyncWaterMark(DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w5, w);
    EXPECT_EQ(metadata_->GetRecvDeleteSyncWaterMark(DEVICE_A, USER_B, w), E_OK);
    EXPECT_EQ(w6, w);

    /**
     * @tc.steps: step5. set peer and local watermark
     * @tc.expected: step5. E_OK
     */
    WaterMark w7 = 7;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_B, USER_A, w7), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_B, USER_A, w7, true), E_OK);
    WaterMark w8 = 8;
    EXPECT_EQ(metadata_->SaveLocalWaterMark(DEVICE_B, USER_B, w8), E_OK);
    EXPECT_EQ(metadata_->SavePeerWaterMark(DEVICE_B, USER_B, w8, true), E_OK);

    /**
     * @tc.steps: step6. get receive and send watermark
     * @tc.expected: step6. E_OK and get the w3
     */
    EXPECT_EQ(metadata_->GetRecvDeleteSyncWaterMark(DEVICE_B, USER_A, w), E_OK);
    EXPECT_EQ(w7, w);
    EXPECT_EQ(metadata_->GetSendDeleteSyncWaterMark(DEVICE_B, USER_A, w), E_OK);
    EXPECT_EQ(w7, w);
    EXPECT_EQ(metadata_->GetRecvDeleteSyncWaterMark(DEVICE_B, USER_B, w), E_OK);
    EXPECT_EQ(w8, w);
    EXPECT_EQ(metadata_->GetSendDeleteSyncWaterMark(DEVICE_B, USER_B, w), E_OK);
    EXPECT_EQ(w8, w);

    /**
     * @tc.steps: step7. get not exit receive and send watermark
     * @tc.expected: step7. E_OK and get the 0
     */
    EXPECT_EQ(metadata_->GetRecvDeleteSyncWaterMark("D3", USER_A, w), E_OK);
    EXPECT_EQ(w, 0u);
    EXPECT_EQ(metadata_->GetSendDeleteSyncWaterMark("D3", USER_A, w), E_OK);
    EXPECT_EQ(w, 0u);
}
}
