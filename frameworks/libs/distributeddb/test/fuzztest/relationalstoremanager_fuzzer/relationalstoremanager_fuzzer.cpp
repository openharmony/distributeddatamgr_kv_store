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
#include "virtual_communicator_aggregator.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;

constexpr const char* DB_SUFFIX = ".db";
constexpr const char* STORE_ID = "Relational_Store_ID";
const std::string DEVICE_A = "DEVICE_A";
std::string g_testDir;
std::string g_dbDir;
sqlite3 *g_db = nullptr;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;

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

void CombineTest(const uint8_t* data, size_t size)
{
    FuzzerData fuzzerData(data, size);
    uint32_t instanceId = fuzzerData.GetUInt32();
    std::string appId = fuzzerData.GetString(instanceId % 30);
    std::string userId = fuzzerData.GetString(instanceId % 30);
    std::string storeId = fuzzerData.GetString(instanceId % 30);
    RelationalStoreManager::GetDistributedTableName(appId, userId);
    RelationalStoreManager mgr(appId, userId, instanceId);
    g_mgr.GetDistributedTableName(appId, userId);
    g_mgr.OpenStore(g_dbDir + appId + DB_SUFFIX, storeId, {}, g_delegate);
    g_mgr.GetRelationalStoreIdentifier(userId, appId, storeId, instanceId % 2); // 2 is mod num for last parameter
    RuntimeConfig::SetProcessLabel(appId, userId);
}
}


/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::Setup();
    OHOS::CombineTest(data, size);
    OHOS::TearDown();
    return 0;
}