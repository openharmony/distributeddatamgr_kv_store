/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "switch_observer_bridge.h"

#include <gtest/gtest.h>

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class SwitchObserverBridgeTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

    static AppId invalidAppId_;
    static AppId validAppId_;
};

AppId SwitchObserverBridgeTest::invalidAppId_;
AppId SwitchObserverBridgeTest::validAppId_;

void SwitchObserverBridgeTest::SetUpTestCase(void)
{
    invalidAppId_.appId = "";
    validAppId_.appId = "SwitchObserverBridgeTest";
}

class SwitchObserverTest : public KvStoreObserver {
public:
    void OnSwitchChange(const SwitchNotification &notification) override {}
};

/**
* @tc.name: AddAndDeleteSwitchCallbackTest001
* @tc.desc: test add and delete switch callback
* @tc.type: FUNC
*/
HWTEST_F(SwitchObserverBridgeTest, AddAndDeleteSwitchCallbackTest001, TestSize.Level1)
{
    std::shared_ptr<SwitchObserverBridge> switchBridge = std::make_shared<SwitchObserverBridge>(validAppId_);

    switchBridge->AddSwitchCallback(nullptr);
    ASSERT_TRUE(switchBridge->switchObservers_.Empty());

    std::shared_ptr<SwitchObserverTest> observer = std::make_shared<SwitchObserverTest>();
    switchBridge->AddSwitchCallback(observer);
    ASSERT_FALSE(switchBridge->switchObservers_.Empty());

    switchBridge->DeleteSwitchCallback(nullptr);
    ASSERT_FALSE(switchBridge->switchObservers_.Empty());

    switchBridge->DeleteSwitchCallback(observer);
    ASSERT_TRUE(switchBridge->switchObservers_.Empty());
}

/**
* @tc.name: OnRemoteDiedTest001
* @tc.desc: test on remote died
* @tc.type: FUNC
*/
HWTEST_F(SwitchObserverBridgeTest, OnRemoteDiedTest001, TestSize.Level1)
{
    std::shared_ptr<SwitchObserverBridge> switchBridge = std::make_shared<SwitchObserverBridge>(invalidAppId_);
    switchBridge->OnRemoteDied();
    ASSERT_FALSE(switchBridge->switchAppId_.IsValid());

    switchBridge = std::make_shared<SwitchObserverBridge>(validAppId_);
    switchBridge->OnRemoteDied();
    ASSERT_TRUE(switchBridge->switchAppId_.IsValid());
    ASSERT_TRUE(switchBridge->switchObservers_.Empty());
}
} // namespace OHOS::Test