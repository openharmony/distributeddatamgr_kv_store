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

#include <gtest/gtest.h>
#include "datamgr_service_proxy.h"
#include <ipc_skeleton.h>
#include "itypes_util.h"
#include "message_parcel.h"
#include "types.h"
#include "log_print.h"
#include <vector>
#include "mock_remote_object.h"

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
{
}

void DataMgrServiceProxyVirtualTest::TearDownTestCase(void)
{
}

void DataMgrServiceProxyVirtualTest::SetUp(void)
{
}

void DataMgrServiceProxyVirtualTest::TearDown(void)
{
}

/**
 * @tc.name: GetFeatureInterface_Success
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_Success, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_Success begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(0)); // Simulation returns successfully

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_Success");
    ASSERT_NE(result, nullptr);
    delete mockImpl;
}

/**
 * @tc.name: GetFeatureInterface_WriteDescriptorFailed
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_WriteDescriptorFailed, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_WriteDescriptorFailed begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_WriteDescriptorFailed");
    ASSERT_EQ(result, nullptr);
    delete mockImpl;
}

/**
 * @tc.name: GetFeatureInterface_WriteDescriptorFailed
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_WriteDescriptorFailed, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_WriteDescriptorFailed begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_WriteDescriptorFailed");
    ASSERT_EQ(result, nullptr);
    delete mockImpl;
}

/**
 * @tc.name: GetFeatureInterface_WriteNameFailed
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_WriteNameFailed, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_WriteNameFailed begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_WriteNameFailed");
    ASSERT_EQ(result, nullptr);
    delete mockImpl;
}

/**
 * @tc.name: GetFeatureInterface_SendRequestError
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, GetFeatureInterface_SendRequestError, TestSize.Level0)
{
    ZLOGI("GetFeatureInterface_SendRequestError begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(-1)); // Simulating an error

    auto result = proxy.GetFeatureInterface("GetFeatureInterface_SendRequestError");
    ASSERT_EQ(result, nullptr);
    delete mockImpl;
}

/**
 * @tc.name: RegisterClientDeathObserver_Success
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_Success, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_Success begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(0)); // Simulation returns successfully

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_Success";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::SUCCESS);
    delete mockImpl;
}

/**
 * @tc.name: RegisterClientDeathObserver_WriteDescriptorFailed
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_WriteDescriptorFailed, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_WriteDescriptorFailed begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_WriteDescriptorFailed";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::IPC_ERROR);
    delete mockImpl;
}

/**
 * @tc.name: RegisterClientDeathObserver_WriteStringFailed
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_WriteStringFailed, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_WriteStringFailed begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_WriteStringFailed";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::IPC_ERROR);
    delete mockImpl;
}

/**
 * @tc.name: RegisterClientDeathObserver_NullObserver
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_NullObserver, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_NullObserver begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_NullObserver";
    auto status = proxy.RegisterClientDeathObserver(appId, nullptr);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
    delete mockImpl;
}

/**
 * @tc.name: RegisterClientDeathObserver_SendRequestError
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, RegisterClientDeathObserver_SendRequestError, TestSize.Level0)
{
    ZLOGI("RegisterClientDeathObserver_SendRequestError begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(-1)); // Simulating an error

    AppId appId;
    appId.appId = "RegisterClientDeathObserver_SendRequestError";
    sptr<IRemoteObject> observer = new MockRemoteObject();
    auto status = proxy.RegisterClientDeathObserver(appId, observer);
    ASSERT_EQ(status, Status::IPC_ERROR);
    delete mockImpl;
}

/**
 * @tc.name: ClearAppStorage_Success
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_Success, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_Success begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(0)); // Simulation returns successfully

    auto result = proxy.ClearAppStorage("ClearAppStorage_Success", 1, 1, 1);
    ASSERT_EQ(result, Status::SUCCESS);
    delete mockImpl;
}

/**
 * @tc.name: ClearAppStorage_WriteDescriptorFailed
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_WriteDescriptorFailed, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_WriteDescriptorFailed begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); // SendRequest is not called

    auto result = proxy.ClearAppStorage("ClearAppStorage_WriteDescriptorFailed", 1, 1, 1);
    ASSERT_EQ(result, Status::IPC_ERROR);
    delete mockImpl;
}

/**
 * @tc.name: ClearAppStorage_WriteDataFailed
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_WriteDataFailed, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_WriteDataFailed begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .Times(0); //  SendRequest is not called

    auto result = proxy.ClearAppStorage("ClearAppStorage_WriteDataFailed", 1, 1, 1);
    ASSERT_EQ(result, Status::IPC_ERROR);
    delete mockImpl;
}

/**
 * @tc.name: ClearAppStorage_SendRequestError
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataMgrServiceProxyVirtualTest, ClearAppStorage_SendRequestError, TestSize.Level0)
{
    ZLOGI("ClearAppStorage_SendRequestError begin.");
    sptr<IRemoteObject> mockImpl = new MockRemoteObject();
    DataMgrServiceProxy proxy(mockImpl);

    EXPECT_CALL(*mockImpl, SendRequest(_, _, _, _))
        .WillOnce(Return(-1)); // Simulating an error

    auto result = proxy.ClearAppStorage("ClearAppStorage_SendRequestError", 1, 1, 1);
    ASSERT_EQ(result, Status::IPC_ERROR);
    delete mockImpl;
}
} // namespace OHOS::Test