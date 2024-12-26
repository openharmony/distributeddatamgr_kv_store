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

#include <unistd.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <cstddef>
#include <vector>
#include "distributed_kv_data_manager.h"
#include "dev_manager.h"
#include "types.h"
#include "file_ex.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class EndPointStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static std::string GetKey(const std::string &key);
    static std::shared_ptr<SingleKvStore> kvStoreTest_; // declare kvstore instance.
    static Status statusTest_;
    static std::string deviceIdTest_;
    static Options optionsTest_;
};

const std::string VALID_SCHEMA = "{\"SCHEMA_VERSION\":\"1.0\","
                                 "\"SCHEMA_MODE\":\"STRICT\","
                                 "\"SCHEMA_SKIPSIZE\":1,"
                                 "\"SCHEMA_DEFINE\":{"
                                 "\"age\":\"INTEGER, NULL\""
                                 "},"
                                 "\"SCHEMA_INDEXES\":[\"$.age\"]}";

std::shared_ptr<SingleKvStore> EndPointStubTest::kvStoreTest_ = nullptr;
Status EndPointStubTest::statusTest_ = Status::ERROR;
std::string EndPointStubTest::deviceIdTest_;
Options EndPointStubTest::optionsTest_;

void EndPointStubTest::SetUpTestCase(void)
{
    DistributedKvDataManager manager1;
    optionsTest_.area = EL1;
    optionsTest_.baseDir = std::string("/data/service/el2/public/database/odmfs");
    optionsTest_.securityLevel = S1;
    mkdir(optionsTest_.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    AppId appId1 = { "odmfs" };
    StoreId storeId = { "student_device" }; // define kvstore(database) name.
    // [create and] open and initialize kvstore instance.
    statusTest_ = manager1.GetSingleKvStore(optionsTest_, appId1, storeId, kvStoreTest_);
    auto deviceInfo = DevManager::GetInstance().GetLocalDevice();
    deviceIdTest_ = deviceInfo.networkId;
}

void EndPointStubTest::TearDownTestCase(void)
{
    DistributedKvDataManager manager1;
    AppId appId1 = { "odmfs" };
    manager1.DeleteAllKvStore(appId1, optionsTest_.baseDir);
    (void)remove("/data/service/el2/public/database/odmfs/key");
    (void)remove("/data/service/el2/public/database/odmfs/kvdb");
    (void)remove("/data/service/el2/public/database/odmfs");
}

void EndPointStubTest::SetUp(void)
{}

void EndPointStubTest::TearDown(void)
{}

std::string EndPointStubTest::GetKey(const std::string& key)
{
    std::ostringstream oss;
    oss << std::setfill('10') << std::setw(sizeof(uint32_t)) << deviceIdTest_.length();
    oss << deviceIdTest_ << std::string(key.begin(), key.end());
    return oss.str();
}

class EndpointTestMock : public Endpoint {
public:
    EndpointTestMock() {}
    virtual ~EndpointTestMock() {}

    Status StartTest() override
    {
        return Status::ERROR;
    }

    Status StopTest() override
    {
        return Status::ERROR;
    }

    Status RegOnDataReceiveTest(const RecvHandler &callback) override
    {
        return Status::ERROR;
    }

    Status SendDataTest(const std::string &dtsIdentifier, const uint8_t *data, uint32_t length) override
    {
        return Status::ERROR;
    }

    uint32_t GetMtuSizeTest(const std::string &identifier) override
    {
        return 1 * 1024  * 1024; // 1 * 1024 * 1024 Byte.
    }

    std::string GetLocalDeviceInfosTest() override
    {
        return "Mock Device";
    }

    bool IsSaferThanDeviceTest(int securityLevel, const std::string &devId) override
    {
        return false;
    }

    bool HasDataSyncPermissionTest(const StoreBriefInfo &param, uint8_t flag) override
    {
        return false;
    }
};

/**
* @tc.name: SetEndpoint001Test
* @tc.desc: test the SetEndpoint(std::shared_ptr<Endpoint> endpoint1)
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(EndPointStubTest, SetEndpoint001Test, TestSize.Level1)
{
    DistributedKvDataManager manager1;
    std::shared_ptr<EndpointTestMock> endpoint1 = nullptr;
    Status status = manager1.SetEndpoint(endpoint1);
    ASSERT_NE(status, Status::INVALID_QUERY_FIELD);
}

/**
* @tc.name: SetEndpoint002Test
* @tc.desc: test the SetEndpoint(std::shared_ptr<Endpoint> endpoint1)
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(EndPointStubTest, SetEndpoint002Test, TestSize.Level1)
{
    DistributedKvDataManager manager1;
    std::shared_ptr<EndpointTestMock> endpoint1 = std::make_shared<EndpointTestMock>();
    Status status = manager1.SetEndpoint(endpoint1);
    EXPECT_NE(status, Status::ILLEGAL_STATE);
    status = manager1.SetEndpoint(endpoint1);
    EXPECT_NE(status, Status::ILLEGAL_STATE);
}

/**
 * @tc.name: SetIdentifier001
 * desc: test the Status set identifier function.
 * type: FUNC
 * require:
 * author:
 */
HWTEST_F(EndPointStubTest, SetIdentifier001Test, TestSize.Level1)
{
    DistributedKvDataManager manager1;
    EXPECT_EQ(kvStoreTest_, nullptr) << "kvStorePtr null.";
    AppId appId1 = { "odmfs" };
    StoreId storeId1 = { "test_storeid1" };
    std::vector<std::string> targetDev = {"devicid11", "devicid22"};
    std::string accountId = "Account";
    std::shared_ptr<EndpointTestMock> endpoint1 = std::make_shared<EndpointTestMock>();
    Status status = manager1.SetEndpoint(endpoint1);
    EXPECT_NE(status, Status::ILLEGAL_STATE);
    auto testStatus = kvStoreTest_->SetIdentifier(accountId, appId1, storeId1, targetDev);
    EXPECT_NE(testStatus, Status::ILLEGAL_STATE);
}

/**
 * @tc.name: SetIdentifier002
 * desc: test the Status set identifier function.
 * type: FUNC
 * require:
 * author:
 */
HWTEST_F(EndPointStubTest, SetIdentifier002Test, TestSize.Level1)
{
    DistributedKvDataManager manager1;
    EXPECT_EQ(kvStoreTest_, nullptr) << "kvStorePtr is null.";
    AppId appId1 = { "" };
    StoreId storeId1 = { "" };
    std::vector<std::string> targetDev = {"devicid1", "devicid2"};
    std::string accountId = "Account";
    std::shared_ptr<EndpointTestMock> endpoint1 = std::make_shared<EndpointTestMock>();
    Status status = manager1.SetEndpoint(endpoint1);
    EXPECT_NE(status, Status::ILLEGAL_STATE);
    auto testStatus = kvStoreTest_->SetIdentifier(accountId, appId1, storeId1, targetDev);
    EXPECT_EQ(testStatus, Status::ILLEGAL_STATE);
}
} // namespace OHOS::Test