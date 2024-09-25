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

#define LOG_TAG "DevManagerMockTest"
#include <gtest/gtest.h>
#include "dev_manager.h"
#include "device_manager.h"
#include "device_manager_mock.h"
#include "dm_device_info.h"
#include "types.h"
#include "log_print.h"
namespace OHOS::Test {
using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedHardware;
using DmDeviceInfo = OHOS::DistributedHardware::DmDeviceInfo;
class DevManagerMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void DevManagerMockTest::SetUpTestCase(void)
{}

void DevManagerMockTest::TearDownTestCase(void)
{}

void DevManagerMockTest::SetUp(void)
{}

void DevManagerMockTest::TearDown(void)
{}

/**
* @tc.name: GetLocalDevice
* @tc.desc: test GetLocalDevice get local device info fail
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DevManagerMockTest, GetLocalDevice, TestSize.Level1)
{
    ZLOGI("GetLocalDevice begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_EQ(devInfo.uuid, "");
}

/**
* @tc.name: GetUnEncryptedUuid
* @tc.desc: test GetUnEncryptedUuid get local device info fail
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DevManagerMockTest, GetUnEncryptedUuid001, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid001 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_EQ(uuid, "");
}

/**
* @tc.name: GetUnEncryptedUuid
* @tc.desc: test GetUnEncryptedUuid networkid empty
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DevManagerMockTest, GetUnEncryptedUuid002, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid002 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_EQ(uuid, "");
}

/**
* @tc.name: GetUnEncryptedUuid
* @tc.desc: test GetUnEncryptedUuid get uuid by networkid fail
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DevManagerMockTest, GetUnEncryptedUuid003, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid003 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_EQ(uuid, "");
}

/**
* @tc.name: GetRemoteDevices
* @tc.desc: test GetRemoteDevices get trusted device
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DevManagerMockTest, GetRemoteDevices, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}
} // namespace OHOS::Test