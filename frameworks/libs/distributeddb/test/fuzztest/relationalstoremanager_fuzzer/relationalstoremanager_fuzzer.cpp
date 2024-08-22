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
#include "fuzzer_data.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "store_changed_data.h"
#include "store_types.h"
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
    g_mgr.CloseStore(g_delegate);
    g_delegate = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    g_communicatorAggregator = nullptr;
    if (sqlite3_close_v2(g_db) != SQLITE_OK) {
        LOGI("sqlite3_close_v2 faile");
    }
    g_db = nullptr;
    DistributedDBToolsTest::RemoveTestDbFiles(g_testDir);
}

void RuntimeConfigTest(const uint8_t *data, size_t size)
{
    bool isPermissionCheck = static_cast<bool>(*data);
    auto permissionCheckCallbackV2 = [isPermissionCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId, const std::string &deviceId, uint8_t flag) -> bool { return isPermissionCheck; };
    RuntimeConfig::SetPermissionCheckCallback(permissionCheckCallbackV2);
    auto permissionCheckCallbackV3 = [isPermissionCheck](const PermissionCheckParam &param, uint8_t flag) -> bool {
        return isPermissionCheck;
    };
    RuntimeConfig::SetPermissionCheckCallback(permissionCheckCallbackV3);
    bool isSyncActivationCheck = static_cast<bool>(*data);
    auto syncActivationCheck = [isSyncActivationCheck](const std::string &userId, const std::string &appId,
        const std::string &storeId) -> bool { return isSyncActivationCheck; };
    RuntimeConfig::SetSyncActivationCheckCallback(syncActivationCheck);
    auto syncActivationCheckV2 = [isSyncActivationCheck](const ActivationCheckParam &param) -> bool {
        return isSyncActivationCheck;
    };
    RuntimeConfig::SetSyncActivationCheckCallback(syncActivationCheckV2);
    FuzzerData fuzzerData(data, size);
    uint32_t instanceId = fuzzerData.GetUInt32();
    const int lenMod = 30; // 30 is mod for string vector size
    std::string userId = fuzzerData.GetString(instanceId % lenMod);
    std::string subUserId = fuzzerData.GetString(instanceId % lenMod);
    RuntimeConfig::SetPermissionConditionCallback([userId, subUserId](const PermissionConditionParam &param) {
        std::map<std::string, std::string> res;
        res.emplace(userId, subUserId);
        return res;
    });
    RuntimeConfig::IsProcessSystemApiAdapterValid();
    bool isAutoLaunch = static_cast<bool>(*data);
    auto autoLaunchRequestCallback = [isAutoLaunch](const std::string &identifier, AutoLaunchParam &param) -> bool {
        return isAutoLaunch;
    };
    auto dbType = static_cast<DBType>(data[0]);
    RuntimeConfig::SetAutoLaunchRequestCallback(autoLaunchRequestCallback, dbType);
    std::string appId = fuzzerData.GetString(instanceId % lenMod);
    std::string storeId = fuzzerData.GetString(instanceId % lenMod);
    RuntimeConfig::ReleaseAutoLaunch(userId, appId, storeId, dbType);
    std::vector<DBInfo> dbInfos;
    DBInfo dbInfo = {
        userId,
        appId,
        storeId,
        true,
        true
    };
    dbInfos.push_back(dbInfo);
    std::string device = fuzzerData.GetString(instanceId % lenMod);
    RuntimeConfig::NotifyDBInfos({ device }, dbInfos);
    RuntimeConfig::NotifyUserChanged();
}

void CombineTest(const uint8_t *data, size_t size)
{
    FuzzerData fuzzerData(data, size);
    uint32_t instanceId = fuzzerData.GetUInt32();
    const int lenMod = 30; // 30 is mod for string vector size
    std::string appId = fuzzerData.GetString(instanceId % lenMod);
    std::string userId = fuzzerData.GetString(instanceId % lenMod);
    std::string storeId = fuzzerData.GetString(instanceId % lenMod);
    RelationalStoreManager::GetDistributedTableName(appId, userId);
    RelationalStoreManager mgr(appId, userId, instanceId);
    g_mgr.GetDistributedTableName(appId, userId);
    g_mgr.GetDistributedLogTableName(userId);
    g_mgr.OpenStore(g_dbDir + appId + DB_SUFFIX, storeId, {}, g_delegate);
    g_mgr.GetRelationalStoreIdentifier(userId, appId, storeId, instanceId % 2); // 2 is mod num for last parameter
    int type = fuzzerData.GetInt();
    Bytes bytes = { type, type, type };
    auto status = static_cast<DBStatus>(data[0]);
    g_mgr.ParserQueryNodes(bytes, status);
    std::string key = fuzzerData.GetString(instanceId % lenMod);
    std::map<std::string, Type> primaryKey = {{ key, key }};
    auto collateType = static_cast<CollateType>(data[0]);
    std::map<std::string, CollateType> collateTypeMap = {{ key, collateType }};
    g_mgr.CalcPrimaryKeyHash(primaryKey, collateTypeMap);
    RuntimeConfig::SetProcessLabel(appId, userId);
    RuntimeConfig::SetTranslateToDeviceIdCallback([](const std::string &oriDevId, const StoreInfo &info) {
        return oriDevId + "_" + info.appId;
    });
    RuntimeConfigTest(data, size);
}
}


/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Setup();
    OHOS::CombineTest(data, size);
    OHOS::TearDown();
    return 0;
}
