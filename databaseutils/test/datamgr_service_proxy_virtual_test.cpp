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
#define LOG_TAG "DataMgrServiceProxyVirtualTest"

#include "datamgr_service_proxy.h"
#include "itypes_util.h"
#include "log_print.h"
#include "message_parcel.h"
#include "mock_remote_object.h"
#include "types.h"
#include <gtest/gtest.h>
#include <ipc_skeleton.h>
#include <vector>

using namespace OHOS::DistributedKv;
using namespace testing;
using namespace testing::ext;
namespace OHOS::Test {
class DataMgrServiceProxyVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DataMgrServiceProxyVirtualTest::SetUpTestCase(void)
{}

void DataMgrServiceProxyVirtualTest::TearDownTestCase(void)
{}

void DataMgrServiceProxyVirtualTest::SetUp(void)
{}

void DataMgrServiceProxyVirtualTest::TearDown(void)
{}

/**
 * @tc.name: GetFeatureInterface_Success
 * @tc.desc:
 * @tc.type: GetFeatureInterface_Success test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_Success, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_Success begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(0)); // Simulation returns successfully

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_Success");
    ASSERT_NE(result, nullptr);
}

/**
 * @tc.name: GetFeatureInterface_WriteDescriptorFailed
 * @tc.desc:
 * @tc.type: GetFeatureInterface_WriteDescriptorFailed test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_WriteDescriptorFailed, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_WriteDescriptorFailed begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_WriteDescriptorFailed");
    ASSERT_EQ(result, nullptr);
}

/**
 * @tc.name: GetFeatureInterface_WriteNameFailed
 * @tc.desc:
 * @tc.type: GetFeatureInterface_WriteNameFailed test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_WriteNameFailed, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_WriteNameFailed begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_WriteNameFailed");
    ASSERT_EQ(result, nullptr);
}

/**
 * @tc.name: GetFeatureInterface_SendRequestError
 * @tc.desc:
 * @tc.type: GetFeatureInterface_SendRequestError test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_SendRequestError, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_SendRequestError begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(-1)); // Simulating an error

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_SendRequestError");
    ASSERT_EQ(result, nullptr);
}

/**
 * @tc.name: RegisterClientDeathObserver_Success
 * @tc.desc:
 * @tc.type: RegisterClientDeathObserver_Success test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_Success, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_Success begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(0)); // Simulation returns successfully

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_Success";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: RegisterClientDeathObserver_WriteDescriptorFailed
 * @tc.desc:
 * @tc.type: RegisterClientDeathObserver_WriteDescriptorFailed test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_WriteDescriptorFailed, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_WriteDescriptorFailed begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_WriteDescriptorFailed";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::IPC_ERROR);
}

/**
 * @tc.name: RegisterClientDeathObserver_WriteStringFailed
 * @tc.desc:
 * @tc.type: RegisterClientDeathObserver_WriteStringFailed test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_WriteStringFailed, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_WriteStringFailed begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_WriteStringFailed";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::IPC_ERROR);
}

/**
 * @tc.name: RegisterClientDeathObserver_NullObserver
 * @tc.desc:
 * @tc.type: RegisterClientDeathObserver_NullObserver test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_NullObserver, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_NullObserver begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_NullObserver";
    auto status = proxy.RegisterClientDeathObserver(appId, nullptr);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
 * @tc.name: RegisterClientDeathObserver_SendRequestError
 * @tc.desc:
 * @tc.type: RegisterClientDeathObserver_SendRequestError test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_SendRequestError, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_SendRequestError begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(-1)); // Simulating an error

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_SendRequestError";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::IPC_ERROR);
}

/**
 * @tc.name: ClearAppStorage_Success
 * @tc.desc:
 * @tc.type: ClearAppStorage_Success test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_Success, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_Success begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(0)); // Simulation returns successfully

    auto result = proxy.ClearAppStorage("ClearAppStorage_Success", 1, 1, 1);
    ASSERT_EQ(result, Status::SUCCESS);
}

/**
 * @tc.name: ClearAppStorage_WriteDescriptorFailed
 * @tc.desc:
 * @tc.type: ClearAppStorage_WriteDescriptorFailed test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_WriteDescriptorFailed, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_WriteDescriptorFailed begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    auto result = proxy.ClearAppStorage("ClearAppStorage_WriteDescriptorFailed", 1, 1, 1);
    ASSERT_EQ(result, Status::IPC_ERROR);
}

/**
 * @tc.name: ClearAppStorage_WriteDataFailed
 * @tc.desc:
 * @tc.type: ClearAppStorage_WriteDataFailed test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_WriteDataFailed, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_WriteDataFailed begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); //  SendRequest is not called

    auto result = proxy.ClearAppStorage("ClearAppStorage_WriteDataFailed", 1, 1, 1);
    ASSERT_EQ(result, Status::IPC_ERROR);
}

/**
 * @tc.name: ClearAppStorage_SendRequestError
 * @tc.desc:
 * @tc.type: ClearAppStorage_SendRequestError test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_SendRequestError, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_SendRequestError begin.");
    std::shared_ptr<IRemoteObject> mockImpl = std::make_shared<MockRemoteObject>();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(-1)); // Simulating an error

    auto result = proxy.ClearAppStorage("ClearAppStorage_SendRequestError", 1, 1, 1);
    ASSERT_EQ(result, Status::IPC_ERROR);
}
} // namespace OHOS::Test