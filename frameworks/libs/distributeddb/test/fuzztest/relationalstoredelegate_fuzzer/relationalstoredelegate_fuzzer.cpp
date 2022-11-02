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

#include "relationalstoredelegate_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "log_print.h"
#include "fuzzer_data.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "virtual_communicator_aggregator.h"
#include "query.h"
#include "store_types.h"
#include "result_set.h"
#include "runtime_context.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;

constexpr const char* DB_SUFFIX = ".db";
constexpr const char* STORE_ID = "Relational_Store_ID";
const std::string DEVICE_A = "DEVICE_A";
std::string g_testDir;
std::string g_dbDir;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
sqlite3 *g_db = nullptr;
RelationalStoreDelegate *g_delegate = nullptr;
VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
const std::string NORMAL_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS sync_data(" \
    "key         BLOB NOT NULL UNIQUE," \
    "value       BLOB," \
    "timestamp   INT  NOT NULL," \
    "flag        INT  NOT NULL," \
    "device      BLOB," \
    "ori_device  BLOB," \
    "hash_key    BLOB PRIMARY KEY NOT NULL," \
    "w_timestamp INT," \
    "UNIQUE(device, ori_device));" \
    "CREATE INDEX key_index ON sync_data (key, flag);";

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
    if (RdbTestUtils::ExecSql(g_db, "PRAGMA journal_mode=WAL;") != SQLITE_OK) {
        return;
    }
    if (RdbTestUtils::ExecSql(g_db, NORMAL_CREATE_TABLE_SQL) != SQLITE_OK) {
        return;
    }
    if (RdbTestUtils::CreateDeviceTable(g_db, "sync_data", DEVICE_A) != 0) {
        return;
    }
    LOGD("open store");
    if (g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, g_delegate) != E_OK) {
        LOGE("fuzz open store faile");
    }
}

void TearDown()
{
    LOGD("close store");
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
    if (g_delegate == nullptr) {
        LOGI("delegate is null");
        return;
    }
    FuzzerData fuzzerData(data, size);
    uint32_t len = fuzzerData.GetUInt32();
    const int lenMod = 30; // 30 is mod for string vector size
    std::string tableName = fuzzerData.GetString(len % lenMod);
    g_delegate->CreateDistributedTable(tableName);

    std::vector<std::string> device = fuzzerData.GetStringVector(len % lenMod);
    Query query = Query::Select();
    int index = len % 3; // 3 is the mod
    SyncMode mode = SyncMode::SYNC_MODE_PUSH_ONLY;
    if (index == 1) {
        mode = SyncMode::SYNC_MODE_PULL_ONLY;
    } else if (index == 2) { // 2 is the remainder
        mode = SyncMode::SYNC_MODE_PUSH_PULL;
    }
    g_delegate->Sync(device, mode, query, nullptr, len % 2); // 2 is mod num for wait parameter
    std::string deviceId = device.size() > 0 ? device[0] : tableName;
    g_delegate->RemoveDeviceData(deviceId);
    g_delegate->RemoveDeviceData(deviceId, tableName);

    RemoteCondition rc = { tableName, device };
    std::shared_ptr<ResultSet> resultSet = nullptr;
    uint64_t timeout = len;
    g_delegate->RemoteQuery(deviceId, rc, timeout, resultSet);
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