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

#include "kvdelegatemanager_fuzzer.h"
#include <codecvt>
#include <cstdio>
#include <list>
#include <securec.h>
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "kv_store_delegate_manager.h"
#include "log_print.h"
#include "runtime_config.h"

class KvDelegateManagerFuzzer {
    /* Keep C++ file names the same as the class name. */
};

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
static constexpr const int MOD = 1024; // 1024 is mod
KvStoreDelegateManager g_mgr("APP_ID", "USER_ID");
const std::string DUMP_DISTRIBUTED_DB = "--database";

void GetAutoLaunchOption(FuzzedDataProvider &fdp, AutoLaunchOption &option)
{
    std::string randomStr = fdp.ConsumeRandomLengthString();
    option.schema = randomStr;
    option.observer.reset();
    option.notifier = nullptr;
    option.storeObserver = nullptr;
}

void RuntimeConfigFuzz()
{
    int handle = -1;
    std::vector<std::u16string> params;
    const std::u16string u16DumpParam = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.
        from_bytes(DUMP_DISTRIBUTED_DB);
    params.push_back(u16DumpParam);
    RuntimeConfig::Dump(handle, params);
}

void CallbackFuzz(FuzzedDataProvider &fdp, std::string storeId)
{
    uint64_t diskSize = fdp.ConsumeIntegral<uint64_t>();
    g_mgr.GetKvStoreDiskSize(storeId, diskSize);
    bool isPermissionCheck = fdp.ConsumeBool();
    auto permissionCheckCallback = [isPermissionCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId, uint8_t flag) -> bool { return isPermissionCheck; };
    (void) KvStoreDelegateManager::SetPermissionCheckCallback(permissionCheckCallback);
    auto permissionCheckCallbackV2 = [isPermissionCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId, const std::string &deviceId, uint8_t flag) -> bool {
            return isPermissionCheck;
    };
    (void) KvStoreDelegateManager::SetPermissionCheckCallback(permissionCheckCallbackV2);
    bool isSyncActivationCheck = fdp.ConsumeBool();
    auto syncActivationCheck = [isSyncActivationCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId) -> bool { return isSyncActivationCheck; };
    KvStoreDelegateManager::SetSyncActivationCheckCallback(syncActivationCheck);
    KvStoreDelegateManager::NotifyUserChanged();
}

void CombineTest(FuzzedDataProvider &fdp)
{
    LOGD("Begin KvDelegateManagerFuzzer");
    std::string path;
    DistributedDBToolsTest::TestDirInit(path);
    const int paramCount = 3;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(paramCount, MOD);
    for (size_t len = 1; len < (size / paramCount); len++) {
        std::string appId = fdp.ConsumeRandomLengthString();
        std::string userId = fdp.ConsumeRandomLengthString();
        std::string storeId = fdp.ConsumeRandomLengthString();
        std::string dir;
        (void) KvStoreDelegateManager::GetDatabaseDir(storeId, appId, userId, dir);
        (void) KvStoreDelegateManager::GetDatabaseDir(storeId, dir);
        (void) KvStoreDelegateManager::SetProcessLabel(appId, userId);
        bool syncDualTupleMode = fdp.ConsumeBool();
        (void) KvStoreDelegateManager::GetKvStoreIdentifier(userId, appId, storeId, syncDualTupleMode);
        AutoLaunchOption option;
        GetAutoLaunchOption(fdp, option);
        option.dataDir = path;
        (void) KvStoreDelegateManager::EnableKvStoreAutoLaunch(userId, appId, storeId, option, nullptr);
        (void) KvStoreDelegateManager::DisableKvStoreAutoLaunch(userId, appId, storeId);
        CallbackFuzz(fdp, storeId);
        std::string targetDev = fdp.ConsumeRandomLengthString();
        bool isCheckOk = fdp.ConsumeBool();
        auto databaseStatusNotifyCallback = [userId, appId, storeId, targetDev, &isCheckOk] (
            const std::string &notifyUserId, const std::string &notifyAppId, const std::string &notifyStoreId,
            const std::string &deviceId, bool onlineStatus) -> void {
                if (notifyUserId == userId && notifyAppId == appId && notifyStoreId == storeId &&
                    deviceId == targetDev && onlineStatus == true) {
                    isCheckOk = true;
                }
            };
        g_mgr.SetStoreStatusNotifier(databaseStatusNotifyCallback);
        RuntimeConfigFuzz();
    }
    KvStoreDelegateManager::SetKvStoreCorruptionHandler(nullptr);
    DistributedDBToolsTest::RemoveTestDbFiles(path);
    LOGD("End KvDelegateManagerFuzzer");
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::CombineTest(fdp);
    return 0;
}
