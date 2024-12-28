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
#include "meta_data.h"
#include "virtual_single_ver_sync_db_Interface.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
constexpr const char *DEVICE_A = "deviceA";
constexpr const char *DEVICE_B = "deviceB";
class KvDBMetaDataTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    void GetMetaDataValue(const std::string &deviceId, MetaDataValue &metaDataValue);
    void PutMetaDataValue(const std::string &deviceId, MetaDataValue &metaDataValue);
protected:
    std::shared_ptr<Metadata> meta = nullptr;
    VirtualSingleVerSyncDBInterface *storage = nullptr;
};

void KvDBMetaDataTest::SetUpTestCase()
{
}

void KvDBMetaDataTest::TearDownTestCase()
{
}

void KvDBMetaDataTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    meta = std::make_shared<Metadata>();
    ASSERT_NE(meta, nullptr);
    storage = new(std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_NE(storage, nullptr);
    meta->Initialize(storage);
}

void KvDBMetaDataTest::TearDown()
{
    meta = nullptr;
    if (storage != nullptr) {
        delete storage;
        storage = nullptr;
    }
}

void KvDBMetaDataTest::GetMetaDataValue(const std::string &deviceId, MetaDataValue &metaDataValue)
{
    Key key;
    DBCommon::StringToVector(deviceId, key);
    Value value;
    int errCode = storage->GetMetaData(key, value);
    if (errCode == -E_NOT_FOUND) {
        return;
    }
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(memcpy_s(&metaDataValue, sizeof(MetaDataValue), value.data(), value.size()), EOK);
}

void KvDBMetaDataTest::PutMetaDataValue(const std::string &deviceId, MetaDataValue &metaDataValue)
{
    Key key;
    DBCommon::StringToVector(deviceId, key);
    Value value;
    value.resize(sizeof(MetaDataValue));
    EXPECT_EQ(memcpy_s(value.data(), value.size(), &metaDataValue, sizeof(MetaDataValue)), EOK);
    EXPECT_EQ(storage->PutMetaData(key, value, false), E_OK);
}

/**
 * @tc.name: MetadataTest001
 * @tc.desc: Test meta set and get ability sync mark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check ability sync finish before set mark.
     * @tc.expected: step1. Default all ability sync finish is false.
     */
    EXPECT_FALSE(meta->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_FALSE(meta->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step2. Set A ability sync finish.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(meta->SetAbilitySyncFinishMark(DEVICE_A, true), E_OK);
    EXPECT_TRUE(meta->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_FALSE(meta->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step3. Set B ability sync finish.
     * @tc.expected: step3. A and B is finish.
     */
    EXPECT_EQ(meta->SetAbilitySyncFinishMark(DEVICE_B, true), E_OK);
    EXPECT_TRUE(meta->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_TRUE(meta->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step4. Set A ability sync not finish.
     * @tc.expected: step4. A is not finish B is finish.
     */
    EXPECT_EQ(meta->SetAbilitySyncFinishMark(DEVICE_A, false), E_OK);
    EXPECT_FALSE(meta->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_TRUE(meta->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step5. Clear all time sync finish.
     * @tc.expected: step5. A and B is finish.
     */
    EXPECT_EQ(meta->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_FALSE(meta->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_TRUE(meta->IsAbilitySyncFinish(DEVICE_B));
    /**
     * @tc.steps: step6. Clear all ability sync finish.
     * @tc.expected: step6. A and B is not finish.
     */
    EXPECT_EQ(meta->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_FALSE(meta->IsAbilitySyncFinish(DEVICE_A));
    EXPECT_FALSE(meta->IsAbilitySyncFinish(DEVICE_B));
}

/**
 * @tc.name: MetadataTest002
 * @tc.desc: Test meta set and get time sync mark.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check time sync finish before set mark.
     * @tc.expected: step1. Default all time sync finish is false.
     */
    EXPECT_FALSE(meta->IsTimeSyncFinish(DEVICE_A));
    EXPECT_FALSE(meta->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step2. Set A time sync finish.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(meta->SetTimeSyncFinishMark(DEVICE_A, true), E_OK);
    EXPECT_TRUE(meta->IsTimeSyncFinish(DEVICE_A));
    EXPECT_FALSE(meta->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step3. Set B time sync finish.
     * @tc.expected: step3. A and B is finish.
     */
    EXPECT_EQ(meta->SetTimeSyncFinishMark(DEVICE_B, true), E_OK);
    EXPECT_TRUE(meta->IsTimeSyncFinish(DEVICE_A));
    EXPECT_TRUE(meta->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step4. Set A time sync not finish.
     * @tc.expected: step4. A is not finish B is finish.
     */
    EXPECT_EQ(meta->SetTimeSyncFinishMark(DEVICE_A, false), E_OK);
    EXPECT_FALSE(meta->IsTimeSyncFinish(DEVICE_A));
    EXPECT_TRUE(meta->IsTimeSyncFinish(DEVICE_B));
    /**
     * @tc.steps: step5. Clear all time sync finish.
     * @tc.expected: step5. A and B is not finish.
     */
    EXPECT_EQ(meta->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_FALSE(meta->IsTimeSyncFinish(DEVICE_A));
    EXPECT_FALSE(meta->IsTimeSyncFinish(DEVICE_B));
}

/**
 * @tc.name: MetadataTest003
 * @tc.desc: Test meta set remote schema version.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check remote schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    EXPECT_EQ(meta->GetRemoteSchemaVersion(DEVICE_A), 0u);
    EXPECT_EQ(meta->GetRemoteSchemaVersion(DEVICE_B), 0u);
    /**
     * @tc.steps: step2. Set A schema version.
     * @tc.expected: step2. A is finish B is not finish.
     */
    EXPECT_EQ(meta->SetRemoteSchemaVersion(DEVICE_A, SOFTWARE_VERSION_CURRENT), E_OK);
    EXPECT_EQ(meta->GetRemoteSchemaVersion(DEVICE_A), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(meta->GetRemoteSchemaVersion(DEVICE_B), 0u);
    /**
     * @tc.steps: step3. Clear all ability sync finish.
     * @tc.expected: step3. A and B version is zero.
     */
    EXPECT_EQ(meta->ClearAllAbilitySyncFinishMark(), E_OK);
    EXPECT_EQ(meta->GetRemoteSchemaVersion(DEVICE_A), 0u);
    EXPECT_EQ(meta->GetRemoteSchemaVersion(DEVICE_B), 0u);
}

/**
 * @tc.name: MetadataTest004
 * @tc.desc: Test meta set remote system time off set.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check remote schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    EXPECT_EQ(meta->GetSystemTimeOffset(DEVICE_A), 0u);
    EXPECT_EQ(meta->GetSystemTimeOffset(DEVICE_B), 0u);
    /**
     * @tc.steps: step2. Set A schema version.
     * @tc.expected: step2. A is finish B is not finish.
     */
    const int64_t offset = 100u;
    EXPECT_EQ(meta->SetSystemTimeOffset(DEVICE_A, offset), E_OK);
    EXPECT_EQ(meta->GetSystemTimeOffset(DEVICE_A), offset);
    EXPECT_EQ(meta->GetSystemTimeOffset(DEVICE_B), 0u);
    /**
     * @tc.steps: step3. Clear all time sync finish.
     * @tc.expected: step3. A and B system time offset is zero.
     */
    EXPECT_EQ(meta->ClearAllTimeSyncFinishMark(), E_OK);
    EXPECT_EQ(meta->GetSystemTimeOffset(DEVICE_A), 0u);
    EXPECT_EQ(meta->GetSystemTimeOffset(DEVICE_B), 0u);
}

/**
 * @tc.name: MetadataTest005
 * @tc.desc: Test meta set local schema version.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check local schema version before set version.
     * @tc.expected: step1. Default all version is zero.
     */
    auto res = meta->GetLocalSchemaVersion();
    EXPECT_EQ(res.first, E_OK);
    EXPECT_NE(res.second, 0u);
    /**
     * @tc.steps: step2. Set local schema version.
     * @tc.expected: step2. set success.
     */
    EXPECT_EQ(meta->SetLocalSchemaVersion(SOFTWARE_VERSION_CURRENT), E_OK);
    res = meta->GetLocalSchemaVersion();
    EXPECT_EQ(res.first, E_OK);
    EXPECT_EQ(res.second, SOFTWARE_VERSION_CURRENT);
}

/**
 * @tc.name: MetadataTest006
 * @tc.desc: Test meta remove device data with reload.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set storage ability sync finish.
     * @tc.expected: step1. A is finish.
     */
    std::string deviceId = DBConstant::DEVICEID_PREFIX_KEY + DBCommon::TransferHashString(DEVICE_A);
    MetaDataValue metaDataValue;
    GetMetaDataValue(deviceId, metaDataValue);
    EXPECT_EQ(meta->EraseDeviceWaterMark(DEVICE_A, true), E_OK);
    EXPECT_TRUE(meta->IsAbilitySyncFinish(DEVICE_A));
}

/**
 * @tc.name: MetadataTest007
 * @tc.desc: Test meta init with time change if need.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Check time sync finish by meta.
     * @tc.expected: step1. B is change because of time change.
     */
    RuntimeContext::GetInstance()->SetTimeChanged(true);
    EXPECT_TRUE(meta->IsTimeChange(DEVICE_B));
    RuntimeContext::GetInstance()->SetTimeChanged(false);
    RuntimeContext::GetInstance()->StopTimeTickMonitorIfNeed();
}

/**
 * @tc.name: MetadataTest008
 * @tc.desc: Test meta deserialize v1 local meta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMetaDataTest, MetadataTest008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert v1 local meta data.
     * @tc.expected: step1.Insert OK.
     */
    std::string keyStr = "localMetaData";
    Key key(keyStr.begin(), keyStr.end());
    Value value;
    value.resize(Parcel::GetUInt32Len() + Parcel::GetUInt64Len());
    Parcel parce(value.data(), value.size());
    LocalMetaData expectLocalMetaData;
    (void)parce.WriteUInt32(expectLocalMetaData.version);
    (void)parce.WriteUInt64(expectLocalMetaData.localSchemaVersion);
    ASSERT_FALSE(parce.IsError());
    ASSERT_EQ(storage->PutMetaData(key, value, false), E_OK);
    /**
     * @tc.steps: step2. Read v1 local meta data.
     * @tc.expected: step2.Read OK.
     */
    auto res = meta->GetLocalSchemaVersion();
    EXPECT_EQ(res.first, E_OK);
    EXPECT_EQ(res.second, expectLocalMetaData.localSchemaVersion);
}
}