/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "relationalstoredelegateget_fuzzer.h"
#include "cloud/cloud_store_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb/result_set.h"
#include "distributeddb_tools_test.h"
#include "fuzzer_data.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "log_print.h"
#include "query.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_context.h"
#include "store_observer.h"
#include "store_types.h"
#include "virtual_communicator_aggregator.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;
static constexpr const int MOD = 1024;
static constexpr const int MAX_LEVEL = 2;
static constexpr const int MIN_SYNC_MODE = static_cast<int>(SyncMode::SYNC_MODE_PUSH_ONLY);
static constexpr const int MAX_SYNC_MODE = static_cast<int>(SyncMode::SYNC_MODE_CLOUD_FORCE_PULL) + 1;
static constexpr const uint32_t MIN_LOCK_ACTION = static_cast<uint32_t>(LockAction::NONE);
static constexpr const uint32_t MAX_LOCK_ACTION = static_cast<uint32_t>(LockAction::DOWNLOAD) + 1;
constexpr const char *DB_SUFFIX = ".db";
constexpr const char *STORE_ID = "Relational_Store_ID";
const std::string DEVICE_A = "DEVICE_A";
std::string g_testDir;
std::string g_dbDir;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
sqlite3 *g_db = nullptr;
RelationalStoreDelegate *g_delegate = nullptr;
VirtualCommunicatorAggregator *g_communicatorAggregator = nullptr;
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
        LOGE("sqlite3_close_v2 faile");
    }
    g_db = nullptr;
    DistributedDBToolsTest::RemoveTestDbFiles(g_testDir);
}

void GetCloudTaskStatusTest(FuzzedDataProvider &fdp)
{
    uint64_t taskId = fdp.ConsumeIntegral<uint64_t>();
    (void)g_delegate->GetCloudTaskStatus(taskId);
}

void GetDownloadingAssetsCountTest(FuzzedDataProvider &fdp)
{
    (void)g_delegate->GetDownloadingAssetsCount();
}

void GetDeviceSyncTaskCountTest(FuzzedDataProvider &fdp)
{
    (void)g_delegate->GetDeviceSyncTaskCount();
}

void SyncDeviceFuzz(FuzzedDataProvider &fdp)
{
    std::vector<std::string> devices;
    std::string device = fdp.ConsumeRandomLengthString();
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        devices.emplace_back(device);
    }
    SyncMode mode = static_cast<SyncMode>(fdp.ConsumeIntegralInRange<int>(MIN_SYNC_MODE, MAX_SYNC_MODE));
    const std::function<void(DBStatus)> checkFinish;
    std::condition_variable callbackCv;
    SyncProcessCallback syncCallback = [checkFinish, &callbackCv](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item : process) {
            if (item.second.process == DistributedDB::FINISHED) {
                if (checkFinish) {
                    checkFinish(item.second.errCode);
                }
                callbackCv.notify_one();
            }
        }
    };
    int64_t waitTime = fdp.ConsumeIntegral<int64_t>();
    Query query = Query::Select();
    g_delegate->Sync(devices, mode, query, syncCallback, waitTime);
    std::vector<std::string> users;
    std::string user = fdp.ConsumeRandomLengthString();
    for (int i = 0; i < size; i++) {
        users.emplace_back(user);
    }
    CloudSyncOption option = {
        .devices = devices,
        .mode = mode,
        .query = query,
        .waitTime = waitTime,
        .priorityTask = fdp.ConsumeBool(),
        .priorityLevel = fdp.ConsumeIntegralInRange<int>(0, MAX_LEVEL), // Valid in [0, 2].
        .compensatedSyncOnly = fdp.ConsumeBool(),
        .users = users,
        .merge = fdp.ConsumeBool(),
        .lockAction = static_cast<LockAction>(fdp.ConsumeIntegralInRange<uint32_t>(MIN_LOCK_ACTION, MAX_LOCK_ACTION)),
        .prepareTraceId = fdp.ConsumeRandomLengthString(),
        .asyncDownloadAssets = fdp.ConsumeBool(),
    };
    g_delegate->Sync(option, syncCallback);
    uint64_t taskId = fdp.ConsumeIntegral<uint64_t>();
    g_delegate->Sync(option, syncCallback, taskId);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Setup();
    FuzzedDataProvider fdp(data, size);
    OHOS::GetCloudTaskStatusTest(fdp);
    OHOS::GetDownloadingAssetsCountTest(fdp);
    OHOS::GetDeviceSyncTaskCountTest(fdp);
    OHOS::SyncDeviceFuzz(fdp);
    OHOS::TearDown();
    return 0;
}
