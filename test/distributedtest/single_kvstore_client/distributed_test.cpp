/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "DistributedTest"

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "directory_ex.h"
#include "distributed_kv_data_manager.h"
#include "distributed_major.h"
#include "log_print.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributeSystemTest;
using namespace OHOS::Security::AccessToken;
namespace OHOS::Test {
class DistributedTest : public DistributeTest {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

    Status GetRemote(const Key &key, Value &value);

    static DistributedKvDataManager manager_;
    static std::shared_ptr<SingleKvStore> singleKvStore_;
    static constexpr int FILE_PERMISSION = 0777;
    static constexpr int WAIT_TIME = 5;
};

DistributedKvDataManager DistributedTest::manager_;
std::shared_ptr<SingleKvStore> DistributedTest::singleKvStore_ = nullptr;

class KvStoreSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    static bool IsSyncComplete()
    {
        bool flag = completed_;
        completed_ = false;
        return flag;
    }

    void SyncCompleted(const std::map<std::string, Status> &results) override
    {
        ZLOGI("SyncCallback Called!");
        for (const auto &result : results) {
            if (result.second == SUCCESS) {
                completed_ = true;
            }
            ZLOGI("device: %{public}s, status: 0x%{public}x", result.first.c_str(), result.second);
        }
    }

private:
    static bool completed_;
};

bool KvStoreSyncCallbackTestImpl::completed_ = false;

void DistributedTest::SetUpTestCase()
{
    const char **perms = new const char *[1];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    TokenInfoParams info = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_test",
        .aplStr = "system_basic",
    };
    auto tokenId = GetAccessTokenId(&info);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;

    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = false,
                        .kvStoreType = KvStoreType::SINGLE_VERSION };
    options.area = EL1;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "student" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    auto status = manager_.GetSingleKvStore(options, appId, storeId, singleKvStore_);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_NE(singleKvStore_, nullptr);
    OHOS::ChangeModeDirectory(options.baseDir, FILE_PERMISSION);

    std::shared_ptr<KvStoreSyncCallback> syncCallback = std::make_shared<KvStoreSyncCallbackTestImpl>();
    status = singleKvStore_->RegisterSyncCallback(syncCallback);
    ASSERT_EQ(status, SUCCESS);
}

void DistributedTest::TearDownTestCase()
{
    (void)manager_.CloseAllKvStore({ "odmf" });
    (void)manager_.DeleteKvStore({ "odmf" }, { "student" }, "/data/service/el1/public/database/odmf");
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void DistributedTest::SetUp()
{}

void DistributedTest::TearDown()
{}

Status DistributedTest::GetRemote(const Key &key, Value &value)
{
    Status status = ERROR;
    std::string message = "get";
    message += ",";
    message += key.ToString();
    (void)SendMessage(AGENT_NO::ONE, message, message.size(),
        [&](const std::string &returnBuf, int length) -> bool {
            auto index = returnBuf.find(",");
            status = Status(std::stoi(returnBuf.substr(0, index)));
            if (length > static_cast<int>(index + 1)) {
                value = Value(returnBuf.substr(index + 1));
            }
            return true;
        });
    return status;
}

/**
* @tc.name: SendMessage001
* @tc.desc: Verify distributed test framework SendMessage
* @tc.type: FUNC
*/
HWTEST_F(DistributedTest, SendMessage001, TestSize.Level1)
{
    ZLOGI("SendMessage001 Start");

    std::string message = "SendMessage";
    std::string result = "";
    auto flag = SendMessage(AGENT_NO::ONE, message, message.size(),
        [&](const std::string &returnBuf, int length) -> bool {
            result = returnBuf;
            return true;
        });
    ASSERT_TRUE(flag);
    ASSERT_EQ(result, "Send Message OK!");

    ZLOGI("SendMessage001 end");
}

/**
* @tc.name: RunCommand001
* @tc.desc: Verify distributed test framework RunCommand
* @tc.type: FUNC
*/
HWTEST_F(DistributedTest, RunCommand001, TestSize.Level1)
{
    ZLOGI("RunCommand001 Start");

    std::string command = "CommandTest";
    std::string arguments = "";
    std::string result = "0";
    auto flag = RunCmdOnAgent(AGENT_NO::ONE, command, arguments, result);
    ASSERT_TRUE(flag);
    auto status = GetReturnVal();
    ASSERT_EQ(status, SUCCESS);

    ZLOGI("RunCommand001 end");
}

/**
* @tc.name: SyncData001
* @tc.desc: Sync data with push mode and get data from other device
* @tc.type: FUNC
*/
HWTEST_F(DistributedTest, SyncData001, TestSize.Level1)
{
    ZLOGI("SyncData001 begin");

    Key key = { "key1" };
    Value valueLocal = { "value1" };
    auto status = singleKvStore_->Put(key, valueLocal);
    ASSERT_EQ(status, SUCCESS);

    std::vector<std::string> devices;
    status = singleKvStore_->Sync(devices, SyncMode::PUSH);
    ASSERT_EQ(status, SUCCESS);

    sleep(WAIT_TIME);

    auto flag = KvStoreSyncCallbackTestImpl::IsSyncComplete();
    ASSERT_TRUE(flag);

    Value valueRemote;
    status = GetRemote(key, valueRemote);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(valueLocal.ToString(), valueRemote.ToString());

    ZLOGI("SyncData001 end");
}

/**
* @tc.name: SyncData002
* @tc.desc: Sync data with pull mode by other device and get data from other device
* @tc.type: FUNC
*/
HWTEST_F(DistributedTest, SyncData002, TestSize.Level1)
{
    ZLOGI("SyncData002 begin");

    Key key = { "key2" };
    Value valueLocal = { "value2" };
    auto status = singleKvStore_->Put(key, valueLocal);
    ASSERT_EQ(status, SUCCESS);

    SyncMode mode = SyncMode::PULL;
    std::string arguments = std::to_string(mode);
    auto flag = RunCmdOnAgent(AGENT_NO::ONE, "sync", arguments, "0");
    ASSERT_TRUE(flag);
    status = static_cast<Status>(GetReturnVal());
    ASSERT_EQ(status, SUCCESS);

    Value valueRemote;
    status = GetRemote(key, valueRemote);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(valueLocal.ToString(), valueRemote.ToString());

    ZLOGI("SyncData002 end");
}
} // namespace OHOS::Test

int main(int argc, char *argv[])
{
    g_pDistributetestEnv = new DistributeTestEnvironment("major.desc");
    testing::AddGlobalTestEnvironment(g_pDistributetestEnv);
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}