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

#include <gtest/gtest.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "dev_manager.h"
#include "distributed_kv_data_manager.h"
#include "file_ex.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class EndPointTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static std::string GetKey(const std::string &key);
    static std::shared_ptr<SingleKvStore> kvStore_; // declare kvstore instance.
    static Status status_;
    static std::string deviceId_;
    static Options options_;
};

const std::string VALID_SCHEMA = "{\"SCHEMA_VERSION\":\"1.0\","
                                 "\"SCHEMA_MODE\":\"STRICT\","
                                 "\"SCHEMA_SKIPSIZE\":0,"
                                 "\"SCHEMA_DEFINE\":{"
                                 "\"age\":\"INTEGER, NOT NULL\""
                                 "},"
                                 "\"SCHEMA_INDEXES\":[\"$.age\"]}";

std::shared_ptr<SingleKvStore> EndPointTest::kvStore_ = nullptr;
Status EndPointTest::status_ = Status::ERROR;
std::string EndPointTest::deviceId_;
Options EndPointTest::options_;

void EndPointTest::SetUpTestCase(void)
{
    DistributedKvDataManager manager;
    options_.area = EL1;
    options_.baseDir = std::string("/data/service/el1/public/database/odmf");
    options_.securityLevel = S1;
    mkdir(options_.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    AppId appId = { "odmf" };
    StoreId storeId = { "student_device" }; // define kvstore(database) name.
    // [create and] open and initialize kvstore instance.
    status_ = manager.GetSingleKvStore(options_, appId, storeId, kvStore_);
    auto deviceInfo = DevManager::GetInstance().GetLocalDevice();
    deviceId_ = deviceInfo.networkId;
}

void EndPointTest::TearDownTestCase(void)
{
    DistributedKvDataManager manager;
    AppId appId = { "odmf" };
    manager.DeleteAllKvStore(appId, options_.baseDir);
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void EndPointTest::SetUp(void)
{}

void EndPointTest::TearDown(void)
{}

std::string EndPointTest::GetKey(const std::string& key)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(sizeof(uint32_t)) << deviceId_.length();
    oss << deviceId_ << std::string(key.begin(), key.end());
    return oss.str();
}

class EndpointMock : public Endpoint {
public:
    EndpointMock() {}
    virtual ~EndpointMock() {}

    Status Start() override
    {
        return Status::SUCCESS;
    }

    Status Stop() override
    {
        return Status::SUCCESS;
    }

    Status RegOnDataReceive(const RecvHandler &callback) override
    {
        return Status::SUCCESS;
    }

    Status SendData(const std::string &dtsIdentifier, const uint8_t *data, uint32_t length) override
    {
        return Status::SUCCESS;
    }

    uint32_t GetMtuSize(const std::string &identifier) override
    {
        return 1 * 1024  * 1024; // 1 * 1024 * 1024 Byte.
    }

    std::string GetLocalDeviceInfos() override
    {
        return "Mock Device";
    }

    bool IsSaferThanDevice(int securityLevel, const std::string &devId) override
    {
        return true;
    }

    bool HasDataSyncPermission(const StoreBriefInfo &param, uint8_t flag) override
    {
        return true;
    }
};

/**
* @tc.name: SetEndpoint001
* @tc.desc: test the SetEndpoint(std::shared_ptr<Endpoint> endpoint)
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(EndPointTest, SetEndpoint001, TestSize.Level1)
{
    DistributedKvDataManager manager;
    std::shared_ptr<EndpointMock> endpoint = nullptr;
    Status status = manager.SetEndpoint(endpoint);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: SetEndpoint002
* @tc.desc: test the SetEndpoint(std::shared_ptr<Endpoint> endpoint)
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(EndPointTest, SetEndpoint002, TestSize.Level1)
{
    DistributedKvDataManager manager;
    std::shared_ptr<EndpointMock> endpoint = std::make_shared<EndpointMock>();
    Status status = manager.SetEndpoint(endpoint);
    EXPECT_EQ(status, Status::SUCCESS);
    status = manager.SetEndpoint(endpoint);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: SetIdentifier001
 * desc: test the Status set identifier function.
 * type: FUNC
 * require:
 * author:SQL
 */
HWTEST_F(EndPointTest, SetIdentifier001, TestSize.Level1)
{
    DistributedKvDataManager manager;
    EXPECT_NE(kvStore_, nullptr) << "kvStorePtr is null.";
    AppId appId = { "odmf" };
    StoreId storeId = { "test_storeid" };
    std::vector<std::string> targetDev = {"devicid1", "devicid2"};
    std::string accountId = "testAccount";
    std::shared_ptr<EndpointMock> endpoint = std::make_shared<EndpointMock>();
    Status status = manager.SetEndpoint(endpoint);
    EXPECT_EQ(status, Status::SUCCESS);
    auto testStatus = kvStore_->SetIdentifier(accountId, appId, storeId, targetDev);
    EXPECT_EQ(testStatus, Status::SUCCESS);
}

/**
 * @tc.name: SetIdentifier002
 * desc: test the Status set identifier function.
 * type: FUNC
 * require:
 * author:SQL
 */
HWTEST_F(EndPointTest, SetIdentifier002, TestSize.Level1)
{
    DistributedKvDataManager manager;
    EXPECT_NE(kvStore_, nullptr) << "kvStorePtr is null.";
    AppId appId = { "" };
    StoreId storeId = { "" };
    std::vector<std::string> targetDev = {"devicid1", "devicid2"};
    std::string accountId = "testAccount";
    std::shared_ptr<EndpointMock> endpoint = std::make_shared<EndpointMock>();
    Status status = manager.SetEndpoint(endpoint);
    EXPECT_EQ(status, Status::SUCCESS);
    auto testStatus = kvStore_->SetIdentifier(accountId, appId, storeId, targetDev);
    EXPECT_NE(testStatus, Status::SUCCESS);
}
} // namespace OHOS::Test