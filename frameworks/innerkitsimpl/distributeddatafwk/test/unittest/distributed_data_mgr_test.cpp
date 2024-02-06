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

#include "distributed_data_mgr.h"
#include <gtest/gtest.h>
#include <vector>
#include "types.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::Security::AccessToken;
namespace OHOS::Test {
class DistributedDataMgrTest : public testing::Test {
public:
    static DistributedDataMgr manager;
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp();
    void TearDown();
    static constexpr int32_t TEST_USERID = 100;
    static constexpr int32_t APP_INDEX = 0;
    NativeTokenInfoParams infoInstance {0};
};
DistributedDataMgr DistributedDataMgrTest::manager;

void DistributedDataMgrTest::SetUp(void)
{
    infoInstance.dcapsNum = 0;
    infoInstance.permsNum = 0;
    infoInstance.aclsNum = 0;
    infoInstance.dcaps = nullptr;
    infoInstance.perms = nullptr;
    infoInstance.acls = nullptr;
    infoInstance.processName = "KvStoreDataServiceClearTest";
    infoInstance.aplStr = "system_core";

    HapInfoParams info = {
        .userID = TEST_USERID,
        .bundleName = "ohos.test.demo",
        .instIndex = 0,
        .appIDDesc = "ohos.test.demo"
    };
    PermissionDef infoManagerTestPermDef = {
        .permissionName = "ohos.permission.test",
        .bundleName = "ohos.test.demo",
        .grantMode = 1,
        .availableLevel = APL_NORMAL,
        .label = "label",
        .labelId = 1,
        .description = "open the door",
        .descriptionId = 1
    };
    PermissionStateFull infoManagerTestState = {
        .permissionName = "ohos.permission.test",
        .isGeneral = true,
        .resDeviceID = {"local"},
        .grantStatus = {PermissionState::PERMISSION_GRANTED},
        .grantFlags = {1}
    };
    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {infoManagerTestPermDef},
        .permStateList = {infoManagerTestState}
    };
    AccessTokenKit::AllocHapToken(info, policy);
}

void DistributedDataMgrTest::TearDown(void)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(TEST_USERID, "ohos.test.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: ClearAppStorage
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DistributedDataMgrTest, ClearAppStorage001, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(TEST_USERID, "ohos.test.demo", 0);
    auto ret = manager.ClearAppStorage("ohos.test.demo", TEST_USERID, APP_INDEX, tokenId);
    EXPECT_EQ(ret, Status::SUCCESS);
}

/**
* @tc.name: ClearAppStorage
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DistributedDataMgrTest, ClearAppStorage002, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(TEST_USERID, "ohos.test.fail", 0);
    auto ret = manager.ClearAppStorage("ohos.test.demo", TEST_USERID, APP_INDEX, tokenId);
    EXPECT_EQ(ret, Status::ERROR);
}
} // namespace OHOS::Test