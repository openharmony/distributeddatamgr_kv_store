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
#include "ipc_skeleton.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::Security::AccessToken;
std::string BUNDLE_NAME = "ohos.distributeddatamgr.demo";
namespace OHOS::Test {
class DistributedDataMgrInterceptionTest : public testing::Test {
public:
    static DistributedDataMgr manager;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
    static constexpr int32_t userId = 101;
    static constexpr int32_t index = 30;
};
DistributedDataMgr DistributedDataMgrInterceptionTest::manager;

/**
* @tc.name: ClearAppStorage001Test
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedDataMgrInterceptionTest, ClearAppStorage001Test, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetNativeTokenId("foundation");
    SetSelfTokenID(tokenId);
    auto ret = manager.ClearAppStorage(BUNDLE_NAME, userId, index, tokenId);
    EXPECT_EQ(ret, Status::SUCCESS);
}
} // namespace OHOS::Test