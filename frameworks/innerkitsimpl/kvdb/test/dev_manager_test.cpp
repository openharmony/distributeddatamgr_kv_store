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

#define LOG_TAG "DevManagerTest"
#include <gtest/gtest.h>

#include "dev_manager.h"
#include "types.h"
#include "log_print.h"
namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class DevManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void DevManagerTest::SetUpTestCase(void)
{}

void DevManagerTest::TearDownTestCase(void)
{}

void DevManagerTest::SetUp(void)
{}

void DevManagerTest::TearDown(void)
{}

/**
* @tc.name: GetLocalDevice
* @tc.desc: Get local device's infomation
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(DevManagerTest, GetLocalDevice, TestSize.Level1)
{
    ZLOGI("GetLocalDevice begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_NE(devInfo.networkId, "");
    EXPECT_NE(devInfo.uuid, "");
}

/**
* @tc.name: ToUUID
* @tc.desc: Get uuid from networkId
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(DevManagerTest, ToUUID, TestSize.Level1)
{
    ZLOGI("ToUUID begin.");
    auto &devMgr = DevManager::GetInstance();
    auto devInfo = devMgr.GetLocalDevice();
    EXPECT_NE(devInfo.networkId, "");
    auto uuid = devMgr.ToUUID(devInfo.networkId);
    EXPECT_NE(uuid, "");
    EXPECT_EQ(uuid, devInfo.uuid);
}

/**
* @tc.name: ToNetworkId
* @tc.desc: Get networkId from uuid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DevManagerTest, ToNetworkId, TestSize.Level1)
{
    auto &devMgr = DevManager::GetInstance();
    auto devInfo = devMgr.GetLocalDevice();
    EXPECT_NE(devInfo.uuid, "");
    auto networkId = devMgr.ToNetworkId(devInfo.uuid);
    EXPECT_NE(networkId, "");
    EXPECT_EQ(networkId, devInfo.networkId);
}

/**
* @tc.name: GetRemoteDevices
* @tc.desc: Get remote devices
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(DevManagerTest, GetRemoteDevices, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices begin.");
    DevManager &devManager = OHOS::DistributedKv::DevManager::GetInstance();
    auto devInfos = devManager.GetRemoteDevices();
    EXPECT_EQ(devInfos.size(), 0);
}
} // namespace OHOS::Test