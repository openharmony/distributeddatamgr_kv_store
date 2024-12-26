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
#define LOG_TAG "DelegateMgrCallbackVirtualTest"

#include "kv_types_util.h"
#include "types.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "distributeddb_kvstore_delegate_manager.h"
#include "delegate_mgr_callback.h"
#include <vector>

using namespace OHOS::DistributedKv;
using namespace testing;
using namespace testing::ext;
namespace OHOS::Test {
class DelegateMgrCallbackVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DelegateMgrCallbackVirtualTest::SetUpTestCase(void)
{
}

void DelegateMgrCallbackVirtualTest::TearDownTestCase(void)
{
}

void DelegateMgrCallbackVirtualTest::SetUp(void)
{
}

void DelegateMgrCallbackVirtualTest::TearDown(void)
{
}

class MockKvStoreDelegateManager : public KvStoreDelegateManager {
public:
    MOCK_METHOD2(GetKvStoreDiskSize, DBStatus(const std::string &, uint64_t &));
};

/**
 * @tc.name: GetKvStoreDiskSize_DelegateIsNull_ReturnsFalse
 * @tc.desc:
 * @tc.type: GetKvStoreDiskSize_DelegateIsNull_ReturnsFalse test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GetKvStoreDiskSize_DelegateIsNull_ReturnsFalse, TestSize.Level0)
{
    ZLOGI("GetKvStoreDiskSize_DelegateIsNull_ReturnsFalse begin.");
    MockKvStoreDelegateManager *mockDelegate1;
    DelegateMgrCallback *callback1;
    mockDelegate1 = new MockKvStoreDelegateManager();
    callback1 = new DelegateMgrCallback(nullptr);

    uint64_t size = 0;
    EXPECT_FALSE(callback1->GetKvStoreDiskSize("storeId", size));

    delete callback1;
    delete mockDelegate1;
}

/**
 * @tc.name: GetKvStoreDiskSize_DelegateReturnsOK_ReturnsTrue
 * @tc.desc:
 * @tc.type: GetKvStoreDiskSize_DelegateReturnsOK_ReturnsTrue test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GetKvStoreDiskSize_DelegateReturnsOK_ReturnsTrue, TestSize.Level0)
{
    ZLOGI("GetKvStoreDiskSize_DelegateReturnsOK_ReturnsTrue begin.");
    MockKvStoreDelegateManager *mockDelegate2;
    DelegateMgrCallback *callback2;
    mockDelegate2 = new MockKvStoreDelegateManager();
    callback2 = new DelegateMgrCallback(mockDelegate2);
    uint64_t size = 0;
    EXPECT_CALL(*mockDelegate2, GetKvStoreDiskSize("storeId", _)).WillOnce(Return(DBStatus::OK));
    EXPECT_TRUE(callback2->GetKvStoreDiskSize("storeId", size));

    delete callback2;
    delete mockDelegate2;
}

/**
 * @tc.name: GetKvStoreDiskSize_DelegateReturnsNonOK_ReturnsFalse
 * @tc.desc:
 * @tc.type: GetKvStoreDiskSize_DelegateReturnsNonOK_ReturnsFalse test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GetKvStoreDiskSize_DelegateReturnsNonOK_ReturnsFalse, TestSize.Level0)
{
    ZLOGI("GetKvStoreDiskSize_DelegateReturnsNonOK_ReturnsFalse begin.");
    MockKvStoreDelegateManager *mockDelegate3;
    DelegateMgrCallback *callback3;
    mockDelegate3 = new MockKvStoreDelegateManager();
    callback3 = new DelegateMgrCallback(mockDelegate3);

    uint64_t size = 0;
    EXPECT_CALL(*mockDelegate3, GetKvStoreDiskSize("storeId", _)).WillOnce(Return(DBStatus::NOT_FOUND));
    EXPECT_FALSE(callback->GetKvStoreDiskSize("storeId", size));

    delete callback3;
    delete mockDelegate3;
}
} // namespace OHOS::Test