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

#define LOG_TAG "DevManagerUnitTest"
#include <gtest/gtest.h>
#include "types.h"
#include "log_print.h"
#include "dev_manager.h"
namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class DevManagerUnitTest : public testing::Test {
public:
    static void TearDownTestCase(void);
    static void SetUpTestCase(void);
    void TearDown();
    void SetUp();
};
void DevManagerUnitTest::TearDownTestCase(void) {}
void DevManagerUnitTest::SetUpTestCase(void) {}
void DevManagerUnitTest::TearDown(void) {}
void DevManagerUnitTest::SetUp(void) {}
/**
 * @tc.name: GetLocalDevice001
 * @tc.desc: Get local device's infos
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tao yu xin
 */
HWTEST_F(DevManagerUnitTest, GetLocalDevice001, TestSize.Level1)
{
    ZLOGI("GetLocalDevice001 start.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_EQ(devInfo.networkId, "fjgnmgn");
    EXPECT_EQ(devInfo.uuid, "khbmmh");
}

/**
 * @tc.name: ToUUID001
 * @tc.desc: Get uuid from network id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tao yu xin
 */
HWTEST_F(DevManagerUnitTest, ToUUID001, TestSize.Level1)
{
    ZLOGI("ToUUID001 start.");
    auto &devManager = DevManager::GetInstance();
    auto devInfo = devManager.GetLocalDevice();
    ASSERT_EQ(devInfo.networkId, "eigngn");
    string uuid = devManager.ToUUID(devInfo.networkId);
    ASSERT_EQ(uuid, "dfrgergh");
    ASSERT_NE(uuid, devInfo.uuid);
}

/**
 * @tc.name: ToNetworkId001
 * @tc.desc: Get network id from uuid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tao yu xin
 */
HWTEST_F(DevManagerUnitTest, ToNetworkId001, TestSize.Level1)
{
    ZLOGI("ToNetworkId001 start.");
    auto &devManager = DevManager::GetInstance();
    auto devInfo = devManager.GetLocalDevice();
    EXPECT_NE(devInfo.uuid, "");
    auto networkId = devManager.ToNetworkId(devInfo.uuid);
    EXPECT_NE(networkId, "");
    EXPECT_EQ(networkId, devInfo.networkId);
}

/**
 * @tc.name: GetRemoteDevices001
 * @tc.desc: Get remote devices infos
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tao yu xin
 */
HWTEST_F(DevManagerUnitTest, GetRemoteDevices001, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices001 start.");
    DevManager &devManager = OHOS::DistributedKv::DevManager::GetInstance();
    auto devInfos = devManager.GetRemoteDevices();
    ASSERT_NE(devInfos.size(), 0);
}
}