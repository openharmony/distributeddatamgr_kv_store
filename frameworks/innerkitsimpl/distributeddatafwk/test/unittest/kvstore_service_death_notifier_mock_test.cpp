/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>

#include "kvstore_service_death_notifier.h"
#include "log_print.h"
#include "system_ability_definition.h"
#include "system_ability_manager_mock.h"
#include "ipc_object_stub.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
using namespace std;
using namespace testing;

namespace {
class KvStoreServiceDeathNotifierMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
public:
    static inline std::shared_ptr<SystemAbilityMock> sa = nullptr;
    static inline sptr<SystemAbilityManagerMock> sam = nullptr;
};

void KvStoreServiceDeathNotifierMockTest::SetUpTestCase(void)
{
}

void KvStoreServiceDeathNotifierMockTest::TearDownTestCase(void)
{
}

void KvStoreServiceDeathNotifierMockTest::SetUp(void)
{
    sa = std::make_shared<SystemAbilityMock>();
    ISystemAbilityBase::sab = sa;
    sam = sptr(new SystemAbilityManagerMock());
}

void KvStoreServiceDeathNotifierMockTest::TearDown(void)
{
    sam = nullptr;
    ISystemAbilityBase::sab = nullptr;
    sa = nullptr;
}

HWTEST_F(KvStoreServiceDeathNotifierMockTest, GetService_SamgrNull_001, TestSize.Level1)
{
    EXPECT_CALL(*sa, GetSystemAbilityManager()).WillOnce(Return(nullptr));
    auto result = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(KvStoreServiceDeathNotifierMockTest, GetService_CheckSASuccess_ProxyFail_004, TestSize.Level1)
{
    sptr<IRemoteObject> remoteObject = new IPCObjectStub();
    EXPECT_CALL(*sa, GetSystemAbilityManager()).WillOnce(Return(sam));
    EXPECT_CALL(*sam, CheckSystemAbility(_)).WillOnce(Return(remoteObject));
    auto result = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    EXPECT_EQ(result, nullptr);
}

}
