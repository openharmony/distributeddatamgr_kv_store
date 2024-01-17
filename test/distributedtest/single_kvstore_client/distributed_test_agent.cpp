/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "DistributedTestAgent"

#include <gtest/gtest.h>

#include <map>

#include "accesstoken_kit.h"
#include "distributed_kv_data_manager.h"
#include "types.h"
#include "distributed_agent.h"
#include "directory_ex.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "log_print.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributeSystemTest;
using namespace OHOS::DistributedKv;
using namespace OHOS::Security::AccessToken;
namespace OHOS::Test {
class DistributedTestAgent : public DistributedAgent {
public:
    bool SetUp() override;
    bool TearDown() override;

    int OnProcessMsg(const std::string &msg, int len, std::string &ret, int retLen) override;
    int OnProcessCmd(const std::string &command, int len, const std::string &args,
        int argsLen, const std::string &expectValue, int expectValueLen) override;

    int Get(const std::string &msg, std::string &ret);

    int Put(const std::string &args);
    int Delete(const std::string &args);
    int Sync(const std::string &args);

private:
    static DistributedKvDataManager manager_;
    static std::shared_ptr<SingleKvStore> singleKvStore_;
    static constexpr int FILE_PERMISSION = 0777;
    static constexpr int WAIT_TIME = 5;

    using MessageFunction = int (DistributedTestAgent::*)(const std::string &msg, std::string &ret);
    std::map<std::string, MessageFunction> messageFunctionMap_;
    using CommandFunction = int (DistributedTestAgent::*)(const std::string &args);
    std::map<std::string, CommandFunction> commandFunctionMap_;
};

DistributedKvDataManager DistributedTestAgent::manager_;
std::shared_ptr<SingleKvStore> DistributedTestAgent::singleKvStore_ = nullptr;

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

bool DistributedTestAgent::SetUp()
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
    if (status != SUCCESS || singleKvStore_ == nullptr) {
        return false;
    }
    OHOS::ChangeModeDirectory(options.baseDir, FILE_PERMISSION);

    std::shared_ptr<KvStoreSyncCallback> syncCallback = std::make_shared<KvStoreSyncCallbackTestImpl>();
    status = singleKvStore_->RegisterSyncCallback(syncCallback);
    if (status != SUCCESS) {
        return false;
    }

    messageFunctionMap_["get"] = &DistributedTestAgent::Get;

    commandFunctionMap_["put"] = &DistributedTestAgent::Put;
    commandFunctionMap_["sync"] = &DistributedTestAgent::Sync;
    return true;
}

bool DistributedTestAgent::TearDown()
{
    (void)manager_.CloseAllKvStore({ "odmf" });
    (void)manager_.DeleteKvStore({ "odmf" }, { "student" }, "/data/service/el1/public/database/odmf");
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
    return true;
}

int DistributedTestAgent::OnProcessMsg(const std::string &msg, int len, std::string &ret, int retLen)
{
    if (msg == "SendMessage") {
        ret = "Send Message OK!";
        return ret.size();
    }
    auto index = msg.find(",");
    std::string function = msg.substr(0, index);
    auto iter = messageFunctionMap_.find(function);
    if (iter != messageFunctionMap_.end()) {
        return (this->*messageFunctionMap_[function])(msg.substr(index + 1), ret);
    }
    return DistributedAgent::OnProcessMsg(msg, len, ret, retLen);
}

int DistributedTestAgent::OnProcessCmd(const std::string &command, int len,
    const std::string &args, int argsLen, const std::string &expectValue, int expectValueLen)
{
    if (command == "CommandTest") {
        return SUCCESS;
    }
    auto iter = commandFunctionMap_.find(command);
    if (iter != commandFunctionMap_.end()) {
        return (this->*commandFunctionMap_[command])(args);
    }
    return DistributedAgent::OnProcessCmd(command, len, args, argsLen, expectValue, expectValueLen);
}

int DistributedTestAgent::Get(const std::string &msg, std::string &ret)
{
    Key key(msg);
    Value value;
    auto status = singleKvStore_->Get(key, value);
    ret = std::to_string(status);
    ret += ",";
    if (status == SUCCESS) {
        ret += value.ToString();
    }
    return ret.size();
}

int DistributedTestAgent::Put(const std::string &args)
{
    auto index = args.find(",");
    Key key(args.substr(0, index));
    Value value(args.substr(index + 1));
    auto status = singleKvStore_->Put(key, value);
    return status;
}

int DistributedTestAgent::Sync(const std::string &args)
{
    SyncMode syncMode = static_cast<SyncMode>(std::stoi(args));
    std::vector<std::string> devices;
    auto status = singleKvStore_->Sync(devices, syncMode);

    sleep(WAIT_TIME);

    auto flag = KvStoreSyncCallbackTestImpl::IsSyncComplete();
    if (!flag) {
        status = TIME_OUT;
    }
    return status;
}
} // namespace OHOS::Test

int main()
{
    OHOS::Test::DistributedTestAgent obj;
    if (obj.SetUp()) {
        ZLOGE("Init environment success.");
        obj.Start("agent.desc");
        obj.Join();
    } else {
        ZLOGE("Init environment fail.");
    }
    if (obj.TearDown()) {
        ZLOGE("Clear environment success.");
        return 0;
    } else {
        ZLOGE("Clear environment failed.");
        return -1;
    }
}