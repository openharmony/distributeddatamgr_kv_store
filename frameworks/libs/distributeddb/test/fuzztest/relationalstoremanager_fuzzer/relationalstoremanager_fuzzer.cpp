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

#include "relationalstoremanager_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "store_changed_data.h"
#include "store_types.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_communicator_aggregator.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;

constexpr const char *DB_SUFFIX = ".db";
constexpr const char *STORE_ID = "Relational_Store_ID";
std::string g_testDir;
std::string g_dbDir;
sqlite3 *g_db = nullptr;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
VirtualCommunicatorAggregator *g_communicatorAggregator = nullptr;
static constexpr const int MIN_COUNT = 1;
static constexpr const int MAX_COMPRESSION_RATE = 100;
static constexpr const int MAX_DOWNLOAD_TASK_COUNT = 12;
static constexpr const int MAX_DOWNLOAD_ASSETS_COUNT = 2000;
static constexpr const int PASSWDLEN = 20; // 20 is passwdlen
static constexpr const int MIN_CIPHER_TYPE = static_cast<int>(CipherType::DEFAULT);
static constexpr const int MAX_CIPHER_TYPE = static_cast<int>(CipherType::AES_256_GCM) + 1;
static constexpr const int MIN_POLICY = ConflictResolvePolicy::LAST_WIN;
static constexpr const int MAX_POLICY = ConflictResolvePolicy::DEVICE_COLLABORATION + 1;
static constexpr const int MIN_MODE = static_cast<int>(DistributedTableMode::COLLABORATION);
static constexpr const int MAX_MODE = static_cast<int>(DistributedTableMode::SPLIT_BY_DEVICE) + 1;
static constexpr const uint32_t MIN_COLLATE_TYPE = static_cast<uint32_t>(CollateType::COLLATE_NONE);
static constexpr const uint32_t MAX_COLLATE_TYPE = static_cast<uint32_t>(CollateType::COLLATE_BUTT) + 1;

void Setup()
{
    DistributedDBToolsTest::TestDirInit(g_testDir);
    g_dbDir = g_testDir + "/";
    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    if (g_communicatorAggregator == nullptr) {
        return;
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);

    g_db = RdbTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    if (g_db == nullptr) {
        return;
    }
}

void TearDown()
{
    RuntimeContext::GetInstance()->StopTaskPool();
    g_mgr.CloseStore(g_delegate);
    g_delegate = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    g_communicatorAggregator = nullptr;
    if (sqlite3_close_v2(g_db) != SQLITE_OK) {
        LOGE("sqlite3_close_v2 faile");
    }
    g_db = nullptr;
    DistributedDBToolsTest::RemoveTestDbFiles(g_testDir);
}

class ThreadPoolImpl : public DistributedDB::IThreadPool {
public:
    const TaskId INVALID_ID = 0u;
    ThreadPoolImpl() {}

    ~ThreadPoolImpl() {}

    TaskId Execute(const Task &task) override
    {
        return INVALID_ID;
    }

    TaskId Execute(const Task &task, Duration delay) override
    {
        return INVALID_ID;
    }

    TaskId Schedule(const Task &task, Duration interval) override
    {
        return INVALID_ID;
    }

    TaskId Schedule(const Task &task, Duration delay, Duration interval) override
    {
        return INVALID_ID;
    }

    TaskId Schedule(const Task &task, Duration delay, Duration interval, uint64_t times) override
    {
        return INVALID_ID;
    }

    bool Remove(const TaskId &taskId, bool wait) override
    {
        return true;
    }

    TaskId Reset(const TaskId &taskId, Duration interval) override
    {
        return INVALID_ID;
    }
};

void RuntimeConfigTest(FuzzedDataProvider &fdp)
{
    bool isPermissionCheck = fdp.ConsumeBool();
    auto permissionCheckCallbackV2 = [isPermissionCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId, const std::string &deviceId, uint8_t flag) -> bool { return isPermissionCheck; };
    RuntimeConfig::SetPermissionCheckCallback(permissionCheckCallbackV2);
    auto permissionCheckCallbackV3 = [isPermissionCheck](const PermissionCheckParam &param, uint8_t flag) -> bool {
        return isPermissionCheck;
    };
    RuntimeConfig::SetPermissionCheckCallback(permissionCheckCallbackV3);
    bool isSyncActivationCheck = fdp.ConsumeBool();
    auto syncActivationCheck = [isSyncActivationCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId) -> bool { return isSyncActivationCheck; };
    RuntimeConfig::SetSyncActivationCheckCallback(syncActivationCheck);
    auto syncActivationCheckV2 = [isSyncActivationCheck](const ActivationCheckParam &param) -> bool {
        return isSyncActivationCheck;
    };
    RuntimeConfig::SetSyncActivationCheckCallback(syncActivationCheckV2);
    std::string userId = fdp.ConsumeRandomLengthString();
    std::string subUserId = fdp.ConsumeRandomLengthString();
    RuntimeConfig::SetPermissionConditionCallback([userId, subUserId](const PermissionConditionParam &param) {
        std::map<std::string, std::string> res;
        res.emplace(userId, subUserId);
        return res;
    });
    RuntimeConfig::IsProcessSystemApiAdapterValid();
    bool isAutoLaunch = fdp.ConsumeBool();
    auto autoLaunchRequestCallback = [isAutoLaunch](const std::string &identifier, AutoLaunchParam &param) -> bool {
        return isAutoLaunch;
    };
    size_t dbTypeLen = sizeof(DBType);
    auto dbType = static_cast<DBType>(fdp.ConsumeIntegral<uint32_t>() % dbTypeLen);
    RuntimeConfig::SetAutoLaunchRequestCallback(autoLaunchRequestCallback, dbType);
    std::string appId = fdp.ConsumeRandomLengthString();
    std::string storeId = fdp.ConsumeRandomLengthString();
    RuntimeConfig::ReleaseAutoLaunch(userId, appId, storeId, dbType);
    RuntimeConfig::ReleaseAutoLaunch(userId, subUserId, appId, storeId, dbType);
    std::vector<DBInfo> dbInfos;
    DBInfo dbInfo = {
        userId,
        appId,
        storeId,
        true,
        true
    };
    dbInfos.emplace_back(dbInfo);
    std::string device = fdp.ConsumeRandomLengthString();
    RuntimeConfig::NotifyDBInfos({ device }, dbInfos);
    RuntimeConfig::NotifyUserChanged();
}

void RuntimeConfigMoreFuncTest(FuzzedDataProvider &fdp)
{
    bool isPermissionCheck = fdp.ConsumeBool();
    std::string userId = fdp.ConsumeRandomLengthString();
    std::string appId = fdp.ConsumeRandomLengthString();
    std::string storeId = fdp.ConsumeRandomLengthString();
    std::string deviceId = fdp.ConsumeRandomLengthString();
    std::string subUserId = fdp.ConsumeRandomLengthString();
    PermissionCheckParamV4 paramV4 = {
        .userId = userId,
        .appId = appId,
        .storeId = storeId,
        .deviceId = deviceId,
        .subUserId = subUserId,
    };
    auto permissionCheckCallbackV4 = [isPermissionCheck, paramV4](
                                         const PermissionCheckParamV4 &param, uint8_t flag) -> bool {
        if (param.userId != paramV4.userId || param.appId != paramV4.appId || param.storeId != paramV4.storeId ||
            param.deviceId != paramV4.deviceId || param.subUserId != paramV4.subUserId) {
            return false;
        }
        return isPermissionCheck;
    };
    RuntimeConfig::SetPermissionCheckCallback(permissionCheckCallbackV4);
    const std::shared_ptr<IThreadPool> threadPool = std::make_shared<ThreadPoolImpl>();
    RuntimeConfig::SetThreadPool(threadPool);
    const std::shared_ptr<ICloudDataTranslate> dataTranslate = std::make_shared<VirtualCloudDataTranslate>();
    RuntimeConfig::SetCloudTranslate(dataTranslate);
    AsyncDownloadAssetsConfig config = {
        .maxDownloadTask = fdp.ConsumeIntegralInRange<uint32_t>(MIN_COUNT, MAX_DOWNLOAD_TASK_COUNT),
        .maxDownloadAssetsCount = fdp.ConsumeIntegralInRange<uint32_t>(MIN_COUNT, MAX_DOWNLOAD_ASSETS_COUNT),
    };
    RuntimeConfig::SetAsyncDownloadAssetsConfig(config);
    bool syncDualTupleMode = fdp.ConsumeBool();
    RuntimeConfig::GetStoreIdentifier(userId, appId, storeId, syncDualTupleMode);
    RuntimeConfig::GetStoreIdentifier(userId, subUserId, appId, storeId, syncDualTupleMode);
    RuntimeConfig::SetDataFlowCheckCallback([](const PermissionCheckParam &, const Property &) {
        return DataFlowCheckRet::DEFAULT;
    });
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

void SetAutoLaunchTest(FuzzedDataProvider &fdp)
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
    std::string id = g_mgr.GetRelationalStoreIdentifier(userId, appId, storeId);
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
    uint32_t instanceId = fdp.ConsumeIntegral<uint32_t>();
    std::string appId = fdp.ConsumeRandomLengthString();
    std::string userId = fdp.ConsumeRandomLengthString();
    std::string storeId = fdp.ConsumeRandomLengthString();
    RelationalStoreManager::GetDistributedTableName(appId, userId);
    RelationalStoreManager mgr(appId, userId, instanceId);
    std::string subUser = fdp.ConsumeRandomLengthString();
    RelationalStoreManager mgrTwo(appId, userId, subUser, instanceId);
    g_mgr.GetDistributedTableName(appId, userId);
    g_mgr.GetDistributedLogTableName(userId);
    g_mgr.OpenStore(g_dbDir + appId + DB_SUFFIX, storeId, {}, g_delegate);
    g_mgr.GetRelationalStoreIdentifier(userId, appId, storeId, instanceId % 2); // 2 is mod num for last parameter
    std::string subUserId = fdp.ConsumeRandomLengthString();
    bool syncDualTupleMode = fdp.ConsumeBool();
    g_mgr.GetRelationalStoreIdentifier(userId, subUserId, appId, storeId, syncDualTupleMode);
    int type = fdp.ConsumeIntegral<int>();
    Bytes bytes = { type, type, type };
    size_t statusLen = sizeof(DBStatus);
    auto status = static_cast<DBStatus>(fdp.ConsumeIntegral<uint32_t>() % statusLen);
    g_mgr.ParserQueryNodes(bytes, status);
    std::string key = fdp.ConsumeRandomLengthString();
    std::map<std::string, Type> primaryKey = {{ key, key }};
    auto collateType =
        static_cast<CollateType>(fdp.ConsumeIntegralInRange<uint32_t>(MIN_COLLATE_TYPE, MAX_COLLATE_TYPE));
    std::map<std::string, CollateType> collateTypeMap = {{ key, collateType }};
    g_mgr.CalcPrimaryKeyHash(primaryKey, collateTypeMap);
    RuntimeConfig::SetProcessLabel(appId, userId);
    RuntimeConfig::SetTranslateToDeviceIdCallback([](const std::string &oriDevId, const StoreInfo &info) {
        return oriDevId + "_" + info.appId;
    });
    RuntimeConfigTest(fdp);
    RuntimeConfigMoreFuncTest(fdp);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Setup();
    FuzzedDataProvider fdp(data, size);
    OHOS::CombineTest(fdp);
    OHOS::SetAutoLaunchTest(fdp);
    OHOS::TearDown();
    return 0;
}
