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

#define LOG_TAG "DevManagerMockUnitTest"
#include "dev_manager.h"
#include "device_manager_mock.h"
#include "log_print.h"
#include "types.h"
#include <gtest/gtest.h>
namespace OHOS::Test {
using namespace std;
using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedHardware;
using DmDeviceInfo = OHOS::DistributedHardware::DmDeviceInfo;
class DevManagerMockUnitTest : public testing::Test {
public:
    static void TearDownTestCase(void);
    static void SetUpTestCase(void);
    void TearDown();
    void SetUp();
};

void DevManagerMockUnitTest::TearDownTestCase(void) {}
void DevManagerMockUnitTest::SetUpTestCase(void) {}
void DevManagerMockUnitTest::TearDown(void) {}
void DevManagerMockUnitTest::SetUp(void) {}

/**
 * @tc.name: GetUnEncryptedUuid003
 * @tc.desc: test GetUnEncryptedUuid get local device info
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(DevManagerMockUnitTest, GetUnEncryptedUuid003, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid003 start.");
    string uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_NE(uuid, "xafakgk");
}

/**
 * @tc.name: GetUnEncryptedUuid004
 * @tc.desc: test GetUnEncryptedUuid networkid isn't empty
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(DevManagerMockUnitTest, GetUnEncryptedUuid004, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid004 start.");
    string uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_NE(uuid, "xqfafgag");
}

/**
 * @tc.name: GetLocalDevice001
 * @tc.desc: test get local device info
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(DevManagerMockUnitTest, GetLocalDevice001, TestSize.Level1)
{
    ZLOGI("GetLocalDevice start.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_EQ(devInfo.uuid, "aoggj");
}

/**
 * @tc.name: GetRemoteDevices003
 * @tc.desc: test get trusted device
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(DevManagerMockUnitTest, GetRemoteDevices003, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices003 start.");
    auto specInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_FALSE(specInfo.empty());
}

/**
 * @tc.name: GetRemoteDevices004
 * @tc.desc: test GetRemoteDevices get trusted device no remote device
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wang dong
 */
HWTEST_F(DevManagerMockUnitTest, GetRemoteDevices004, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices004 begin.");
    auto specInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_FALSE(specInfo.empty());
}
}