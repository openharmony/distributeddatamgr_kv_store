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
static constexpr const int MAX_LEN = 100;
static constexpr const int PASSWDLEN = 20; // 20 is passwdlen
static constexpr const int MIN_COUNT = 1;
static constexpr const int MAX_COMPRESSION_RATE = 100;
static constexpr const int MIN_CIPHER_TYPE = static_cast<int>(CipherType::DEFAULT);
static constexpr const int MAX_CIPHER_TYPE = static_cast<int>(CipherType::AES_256_GCM) + 1;
static constexpr const int MIN_POLICY = ConflictResolvePolicy::LAST_WIN;
static constexpr const int MAX_POLICY = ConflictResolvePolicy::DEVICE_COLLABORATION + 1;
static constexpr const int MIN_MODE = static_cast<int>(DistributedTableMode::COLLABORATION);
static constexpr const int MAX_MODE = static_cast<int>(DistributedTableMode::SPLIT_BY_DEVICE) + 1;
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
    params.emplace_back(u16DumpParam);
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
    auto permissionCheckCallbackV4 = [isPermissionCheck](const PermissionCheckParamV4 &param, uint8_t flag) -> bool {
        return isPermissionCheck;
    };
    (void)KvStoreDelegateManager::SetPermissionCheckCallback(permissionCheckCallbackV4);
    bool isSyncActivationCheck = fdp.ConsumeBool();
    auto syncActivationCheck = [isSyncActivationCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId) -> bool { return isSyncActivationCheck; };
    KvStoreDelegateManager::SetSyncActivationCheckCallback(syncActivationCheck);
    KvStoreDelegateManager::NotifyUserChanged();
}

void SetStoreStatusNotifierV2Fuzz(FuzzedDataProvider &fdp)
{
    StoreStatusNotifierParam param;
    param.userId = fdp.ConsumeRandomLengthString();
    param.appId = fdp.ConsumeRandomLengthString();
    param.storeId = fdp.ConsumeRandomLengthString();
    param.subUserId = fdp.ConsumeRandomLengthString();
    param.deviceId = fdp.ConsumeRandomLengthString();
    bool isCheckOk = false;
    auto databaseStatusNotifyCallbackV2 = [&](const StoreStatusNotifierParam &notifierParam,
                                              bool onLineStatus) -> void {
        if (param.userId == notifierParam.userId && param.appId == notifierParam.appId &&
            param.storeId == notifierParam.storeId && param.deviceId == notifierParam.deviceId &&
            onLineStatus == true) {
            isCheckOk = true;
        }
    };
    g_mgr.SetStoreStatusNotifier(databaseStatusNotifyCallbackV2);
}

CipherPassword GetPassWord(FuzzedDataProvider &fdp)
{
    CipherPassword passwd;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t *val = static_cast<uint8_t*>(new uint8_t[size]);
    fdp.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    return passwd;
}

void SetAutoLaunchFuzz(FuzzedDataProvider &fdp)
{
    int securityLabel = fdp.ConsumeIntegralInRange<int>(SecurityLabel::INVALID_SEC_LABEL, SecurityLabel::S4);
    int securityFlag = fdp.ConsumeIntegralInRange<int>(SecurityFlag::INVALID_SEC_FLAG, SecurityFlag::SECE);
    AutoLaunchOption option = {
        .createIfNecessary = fdp.ConsumeBool(),
        .isEncryptedDb = fdp.ConsumeBool(),
        .cipher = static_cast<CipherType>(fdp.ConsumeIntegralInRange<int>(MIN_CIPHER_TYPE, MAX_CIPHER_TYPE)),
        .passwd = GetPassWord(fdp),
        .schema = fdp.ConsumeRandomLengthString(),
        .createDirByStoreIdOnly = fdp.ConsumeBool(),
        .dataDir = fdp.ConsumeRandomLengthString(),
        .conflictType = fdp.ConsumeIntegral<int>(),
        .notifier = nullptr,
        .secOption = {static_cast<SecurityLabel>(securityLabel), static_cast<SecurityFlag>(securityFlag)},
        .isNeedIntegrityCheck = fdp.ConsumeBool(),
        .isNeedRmCorruptedDb = fdp.ConsumeBool(),
        .isNeedCompressOnSync = fdp.ConsumeBool(),
        .compressionRate = fdp.ConsumeIntegralInRange<uint8_t>(MIN_COUNT, MAX_COMPRESSION_RATE),
        .isAutoSync = fdp.ConsumeBool(),
        .storeObserver = nullptr,
        .syncDualTupleMode = fdp.ConsumeBool(),
        .iterateTimes = fdp.ConsumeIntegral<uint32_t>(),
        .conflictResolvePolicy = fdp.ConsumeIntegralInRange<int>(MIN_POLICY, MAX_POLICY),
        .tableMode = static_cast<DistributedTableMode>(fdp.ConsumeIntegralInRange<int>(MIN_MODE, MAX_MODE)),
    };
    std::string appId = fdp.ConsumeRandomLengthString();
    std::string userId = fdp.ConsumeRandomLengthString();
    std::string storeId = fdp.ConsumeRandomLengthString();
    const AutoLaunchParam encryptedParam = {
        .userId = fdp.ConsumeRandomLengthString(),
        .appId = fdp.ConsumeRandomLengthString(),
        .storeId = fdp.ConsumeRandomLengthString(),
        .option = option,
        .notifier = nullptr,
        .path = fdp.ConsumeRandomLengthString(),
        .subUser = fdp.ConsumeRandomLengthString(),
    };
    std::string id = g_mgr.GetKvStoreIdentifier(userId, appId, storeId);
    const AutoLaunchRequestCallback callback = [&](const std::string &identifier, AutoLaunchParam &param) {
        if (id != identifier) {
            return false;
        }
        param = encryptedParam;
        return true;
    };
    g_mgr.SetAutoLaunchRequestCallback(callback);
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
        SetAutoLaunchFuzz(fdp);
        CallbackFuzz(fdp, storeId);
        std::string targetDev = fdp.ConsumeRandomLengthString();
        bool isCheckOk = fdp.ConsumeBool();
        auto databaseStatusNotifyCallback = [userId, appId, storeId, targetDev, &isCheckOk] (
            const std::string &notifyUserId, const std::string &notifyAppId, const std::string &notifyStoreId,
            const std::string &deviceId, bool onLineStatus) -> void {
                if (notifyUserId == userId && notifyAppId == appId && notifyStoreId == storeId &&
                    deviceId == targetDev && onLineStatus == true) {
                    isCheckOk = true;
                }
            };
        g_mgr.SetStoreStatusNotifier(databaseStatusNotifyCallback);
        SetStoreStatusNotifierV2Fuzz(fdp);
        RuntimeConfigFuzz();
    }
    KvStoreDelegateManager::SetKvStoreCorruptionHandler(nullptr);
    DistributedDBToolsTest::RemoveTestDbFiles(path);
    LOGD("End KvDelegateManagerFuzzer");
}

void SetTest(FuzzedDataProvider &fdp)
{
    std::string appId = fdp.ConsumeRandomLengthString();
    std::string userId = fdp.ConsumeRandomLengthString();
    g_mgr.SetProcessLabel(appId, userId);
    int securityLabel = fdp.ConsumeIntegralInRange<int>(SecurityLabel::INVALID_SEC_LABEL, SecurityLabel::S4);
    int securityFlag = fdp.ConsumeIntegralInRange<int>(SecurityFlag::INVALID_SEC_FLAG, SecurityFlag::SECE);
    AutoLaunchOption option = {
        .createIfNecessary = fdp.ConsumeBool(),
        .isEncryptedDb = fdp.ConsumeBool(),
        .cipher = static_cast<CipherType>(fdp.ConsumeIntegralInRange<int>(MIN_CIPHER_TYPE, MAX_CIPHER_TYPE)),
        .passwd = GetPassWord(fdp),
        .schema = fdp.ConsumeRandomLengthString(),
        .createDirByStoreIdOnly = fdp.ConsumeBool(),
        .dataDir = fdp.ConsumeRandomLengthString(),
        .conflictType = fdp.ConsumeIntegral<int>(),
        .notifier = nullptr,
        .secOption = {static_cast<SecurityLabel>(securityLabel), static_cast<SecurityFlag>(securityFlag)},
        .isNeedIntegrityCheck = fdp.ConsumeBool(),
        .isNeedRmCorruptedDb = fdp.ConsumeBool(),
        .isNeedCompressOnSync = fdp.ConsumeBool(),
        .compressionRate = fdp.ConsumeIntegralInRange<uint8_t>(1, MAX_LEN), // Valid in [1, 100].
        .isAutoSync = fdp.ConsumeBool(),
        .storeObserver = nullptr,
        .syncDualTupleMode = fdp.ConsumeBool(),
        .iterateTimes = fdp.ConsumeIntegral<uint32_t>(),
        .conflictResolvePolicy = fdp.ConsumeIntegralInRange<int>(MIN_POLICY, MAX_POLICY),
        .tableMode = static_cast<DistributedTableMode>(fdp.ConsumeIntegralInRange<int>(MIN_MODE, MAX_MODE)),
    };
    const AutoLaunchParam param = {
        .userId = fdp.ConsumeRandomLengthString(),
        .appId = fdp.ConsumeRandomLengthString(),
        .storeId = fdp.ConsumeRandomLengthString(),
        .option = option,
        .notifier = nullptr,
        .path = fdp.ConsumeRandomLengthString(),
        .subUser = fdp.ConsumeRandomLengthString(),
    };
    g_mgr.EnableKvStoreAutoLaunch(param);
    g_mgr.IsProcessSystemApiAdapterValid();
    std::string subUser = fdp.ConsumeRandomLengthString();
    std::string storeId = fdp.ConsumeRandomLengthString();
    g_mgr.DisableKvStoreAutoLaunch(userId, subUser, appId, storeId);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::CombineTest(fdp);
    OHOS::SetTest(fdp);
    return 0;
}
