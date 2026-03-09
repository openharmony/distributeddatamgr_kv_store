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

#define LOG_TAG "DevManagerMockTestNew"
#include "dev_manager.h"
#include "device_manager_mock.h"
#include "log_print.h"
#include "types.h"
#include <gtest/gtest.h>
namespace OHOS::Test {
using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedHardware;
using DmDeviceInfo = OHOS::DistributedHardware::DmDeviceInfo;
class DevManagerMockTestNew : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void DevManagerMockTestNew::SetUpTestCase(void) { }

void DevManagerMockTestNew::TearDownTestCase(void) { }

void DevManagerMockTestNew::SetUp(void) { }

void DevManagerMockTestNew::TearDown(void) { }

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid001, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid001 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid002, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid002 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid003, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid003 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid004, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid004 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid005, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid005 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid006, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid006 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid007, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid007 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid008, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid008 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid009, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid009 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid010, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid010 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid011, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid011 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid012, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid012 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid013, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid013 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid014, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid014 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid015, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid015 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid016, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid016 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid017, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid017 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid018, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid018 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid019, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid019 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid020, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid020 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid021, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid021 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid022, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid022 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid023, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid023 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid024, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid024 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid025, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid025 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid026, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid026 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid027, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid027 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid028, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid028 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid029, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid029 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid030, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid030 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid031, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid031 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid032, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid032 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid033, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid033 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid034, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid034 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid035, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid035 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid036, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid036 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid037, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid037 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid038, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid038 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid039, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid039 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid040, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid040 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid041, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid041 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid042, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid042 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid043, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid043 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid044, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid044 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid045, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid045 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid046, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid046 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid047, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid047 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid048, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid048 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid049, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid049 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid050, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid050 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid051, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid051 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid052, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid052 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid053, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid053 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid054, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid054 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid055, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid055 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid056, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid056 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid057, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid057 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid058, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid058 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid059, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid059 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid060, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid060 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid061, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid061 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid062, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid062 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid063, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid063 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid064, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid064 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid065, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid065 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid066, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid066 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid067, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid067 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid068, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid068 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid069, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid069 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid070, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid070 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid071, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid071 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid072, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid072 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid073, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid073 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid074, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid074 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid075, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid075 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid076, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid076 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid077, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid077 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid078, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid078 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid079, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid079 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid080, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid080 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid081, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid081 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid082, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid082 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid083, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid083 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid084, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid084 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid085, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid085 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid086, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid086 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid087, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid087 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid088, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid088 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid089, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid089 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid090, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid090 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid091, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid091 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid092, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid092 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid093, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid093 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid094, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid094 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid095, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid095 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid096, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid096 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid097, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid097 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid098, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid098 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid099, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid099 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetUnEncryptedUuid100, TestSize.Level1)
{
    ZLOGI("GetUnEncryptedUuid100 begin.");
    auto uuid = DevManager::GetInstance().GetUnEncryptedUuid();
    EXPECT_TRUE(uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice001, TestSize.Level1)
{
    ZLOGI("GetLocalDevice001 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice002, TestSize.Level1)
{
    ZLOGI("GetLocalDevice002 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice003, TestSize.Level1)
{
    ZLOGI("GetLocalDevice003 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice004, TestSize.Level1)
{
    ZLOGI("GetLocalDevice004 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice005, TestSize.Level1)
{
    ZLOGI("GetLocalDevice005 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice006, TestSize.Level1)
{
    ZLOGI("GetLocalDevice006 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice007, TestSize.Level1)
{
    ZLOGI("GetLocalDevice007 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice008, TestSize.Level1)
{
    ZLOGI("GetLocalDevice008 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice009, TestSize.Level1)
{
    ZLOGI("GetLocalDevice009 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice010, TestSize.Level1)
{
    ZLOGI("GetLocalDevice010 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice011, TestSize.Level1)
{
    ZLOGI("GetLocalDevice011 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice012, TestSize.Level1)
{
    ZLOGI("GetLocalDevice012 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice013, TestSize.Level1)
{
    ZLOGI("GetLocalDevice013 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice014, TestSize.Level1)
{
    ZLOGI("GetLocalDevice014 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice015, TestSize.Level1)
{
    ZLOGI("GetLocalDevice015 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice016, TestSize.Level1)
{
    ZLOGI("GetLocalDevice016 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice017, TestSize.Level1)
{
    ZLOGI("GetLocalDevice017 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice018, TestSize.Level1)
{
    ZLOGI("GetLocalDevice018 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice019, TestSize.Level1)
{
    ZLOGI("GetLocalDevice019 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice020, TestSize.Level1)
{
    ZLOGI("GetLocalDevice020 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice021, TestSize.Level1)
{
    ZLOGI("GetLocalDevice021 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice022, TestSize.Level1)
{
    ZLOGI("GetLocalDevice022 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice023, TestSize.Level1)
{
    ZLOGI("GetLocalDevice023 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice024, TestSize.Level1)
{
    ZLOGI("GetLocalDevice024 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice025, TestSize.Level1)
{
    ZLOGI("GetLocalDevice025 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice026, TestSize.Level1)
{
    ZLOGI("GetLocalDevice026 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice027, TestSize.Level1)
{
    ZLOGI("GetLocalDevice027 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice028, TestSize.Level1)
{
    ZLOGI("GetLocalDevice028 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice029, TestSize.Level1)
{
    ZLOGI("GetLocalDevice029 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice030, TestSize.Level1)
{
    ZLOGI("GetLocalDevice030 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice031, TestSize.Level1)
{
    ZLOGI("GetLocalDevice031 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice032, TestSize.Level1)
{
    ZLOGI("GetLocalDevice032 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice033, TestSize.Level1)
{
    ZLOGI("GetLocalDevice033 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice034, TestSize.Level1)
{
    ZLOGI("GetLocalDevice034 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice035, TestSize.Level1)
{
    ZLOGI("GetLocalDevice035 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice036, TestSize.Level1)
{
    ZLOGI("GetLocalDevice036 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice037, TestSize.Level1)
{
    ZLOGI("GetLocalDevice037 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice038, TestSize.Level1)
{
    ZLOGI("GetLocalDevice038 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice039, TestSize.Level1)
{
    ZLOGI("GetLocalDevice039 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice040, TestSize.Level1)
{
    ZLOGI("GetLocalDevice040 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice041, TestSize.Level1)
{
    ZLOGI("GetLocalDevice041 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice042, TestSize.Level1)
{
    ZLOGI("GetLocalDevice042 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice043, TestSize.Level1)
{
    ZLOGI("GetLocalDevice043 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice044, TestSize.Level1)
{
    ZLOGI("GetLocalDevice044 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice045, TestSize.Level1)
{
    ZLOGI("GetLocalDevice045 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice046, TestSize.Level1)
{
    ZLOGI("GetLocalDevice046 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice047, TestSize.Level1)
{
    ZLOGI("GetLocalDevice047 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice048, TestSize.Level1)
{
    ZLOGI("GetLocalDevice048 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice049, TestSize.Level1)
{
    ZLOGI("GetLocalDevice049 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice050, TestSize.Level1)
{
    ZLOGI("GetLocalDevice050 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice051, TestSize.Level1)
{
    ZLOGI("GetLocalDevice051 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice052, TestSize.Level1)
{
    ZLOGI("GetLocalDevice052 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice053, TestSize.Level1)
{
    ZLOGI("GetLocalDevice053 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice054, TestSize.Level1)
{
    ZLOGI("GetLocalDevice054 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice055, TestSize.Level1)
{
    ZLOGI("GetLocalDevice055 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice056, TestSize.Level1)
{
    ZLOGI("GetLocalDevice056 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice057, TestSize.Level1)
{
    ZLOGI("GetLocalDevice057 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice058, TestSize.Level1)
{
    ZLOGI("GetLocalDevice058 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice059, TestSize.Level1)
{
    ZLOGI("GetLocalDevice059 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice060, TestSize.Level1)
{
    ZLOGI("GetLocalDevice060 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice061, TestSize.Level1)
{
    ZLOGI("GetLocalDevice061 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice062, TestSize.Level1)
{
    ZLOGI("GetLocalDevice062 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice063, TestSize.Level1)
{
    ZLOGI("GetLocalDevice063 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice064, TestSize.Level1)
{
    ZLOGI("GetLocalDevice064 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice065, TestSize.Level1)
{
    ZLOGI("GetLocalDevice065 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice066, TestSize.Level1)
{
    ZLOGI("GetLocalDevice066 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice067, TestSize.Level1)
{
    ZLOGI("GetLocalDevice067 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice068, TestSize.Level1)
{
    ZLOGI("GetLocalDevice068 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice069, TestSize.Level1)
{
    ZLOGI("GetLocalDevice069 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice070, TestSize.Level1)
{
    ZLOGI("GetLocalDevice070 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice071, TestSize.Level1)
{
    ZLOGI("GetLocalDevice071 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice072, TestSize.Level1)
{
    ZLOGI("GetLocalDevice072 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice073, TestSize.Level1)
{
    ZLOGI("GetLocalDevice073 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice074, TestSize.Level1)
{
    ZLOGI("GetLocalDevice074 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice075, TestSize.Level1)
{
    ZLOGI("GetLocalDevice075 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice076, TestSize.Level1)
{
    ZLOGI("GetLocalDevice076 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice077, TestSize.Level1)
{
    ZLOGI("GetLocalDevice077 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice078, TestSize.Level1)
{
    ZLOGI("GetLocalDevice078 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice079, TestSize.Level1)
{
    ZLOGI("GetLocalDevice079 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice080, TestSize.Level1)
{
    ZLOGI("GetLocalDevice080 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice081, TestSize.Level1)
{
    ZLOGI("GetLocalDevice081 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice082, TestSize.Level1)
{
    ZLOGI("GetLocalDevice082 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice083, TestSize.Level1)
{
    ZLOGI("GetLocalDevice083 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice084, TestSize.Level1)
{
    ZLOGI("GetLocalDevice084 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice085, TestSize.Level1)
{
    ZLOGI("GetLocalDevice085 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice086, TestSize.Level1)
{
    ZLOGI("GetLocalDevice086 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice087, TestSize.Level1)
{
    ZLOGI("GetLocalDevice087 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice088, TestSize.Level1)
{
    ZLOGI("GetLocalDevice088 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice089, TestSize.Level1)
{
    ZLOGI("GetLocalDevice089 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice090, TestSize.Level1)
{
    ZLOGI("GetLocalDevice090 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice091, TestSize.Level1)
{
    ZLOGI("GetLocalDevice091 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice092, TestSize.Level1)
{
    ZLOGI("GetLocalDevice092 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice093, TestSize.Level1)
{
    ZLOGI("GetLocalDevice093 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice094, TestSize.Level1)
{
    ZLOGI("GetLocalDevice094 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice095, TestSize.Level1)
{
    ZLOGI("GetLocalDevice095 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice096, TestSize.Level1)
{
    ZLOGI("GetLocalDevice096 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice097, TestSize.Level1)
{
    ZLOGI("GetLocalDevice097 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice098, TestSize.Level1)
{
    ZLOGI("GetLocalDevice098 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice099, TestSize.Level1)
{
    ZLOGI("GetLocalDevice099 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetLocalDevice100, TestSize.Level1)
{
    ZLOGI("GetLocalDevice100 begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_TRUE(devInfo.uuid == "");
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices001, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices001 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices002, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices002 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices003, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices003 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices004, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices004 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices005, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices005 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices006, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices006 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices007, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices007 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices008, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices008 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices009, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices009 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices010, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices010 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices011, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices011 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices012, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices012 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices013, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices013 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices014, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices014 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices015, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices015 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices016, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices016 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices017, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices017 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices018, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices018 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices019, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices019 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices020, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices020 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices021, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices021 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices022, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices022 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices023, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices023 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices024, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices024 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices025, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices025 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices026, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices026 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices027, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices027 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices028, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices028 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices029, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices029 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices030, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices030 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices031, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices031 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices032, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices032 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices033, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices033 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices034, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices034 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices035, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices035 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}



HWTEST_F(DevManagerMockTestNew, GetRemoteDevices037, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices037 begin.");
    auto detailInfo = DevManager::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(detailInfo.empty());
}

HWTEST_F(DevManagerMockTestNew, GetRemoteDevices038, TestSize.Level1)
