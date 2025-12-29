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

#include "relationalstoredelegateset_fuzzer.h"
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
#include "virtual_asset_loader.h"
#include "virtual_cloud_db.h"
#include "virtual_communicator_aggregator.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;
static constexpr const int MOD = 1024;
static constexpr const int MIN_MODE = static_cast<int>(DistributedTableMode::COLLABORATION);
static constexpr const int MAX_MODE = static_cast<int>(DistributedTableMode::SPLIT_BY_DEVICE) + 1;
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

void SetStoreConfigTest(FuzzedDataProvider &fdp)
{
    DistributedTableMode tableMode =
        static_cast<DistributedTableMode>(fdp.ConsumeIntegralInRange<int>(MIN_MODE, MAX_MODE));
    g_delegate->SetStoreConfig({ tableMode });
}

void SetCloudDBTest(FuzzedDataProvider &fdp)
{
    std::shared_ptr<ICloudDb> cloudDb = std::make_shared<VirtualCloudDb>();
    g_delegate->SetCloudDB(cloudDb);
}

void SetCloudDBSchemaTest(FuzzedDataProvider &fdp)
{
    DataBaseSchema dataBaseSchema;
    std::vector<Field> cloudField;
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        Field field = {
            .colName = fdp.ConsumeRandomLengthString(),
            .type = fdp.ConsumeIntegralInRange<int>(TYPE_INDEX<int64_t>, TYPE_INDEX<Assets>),
            .primary = fdp.ConsumeBool(),
            .nullable = fdp.ConsumeBool(),
        };
        cloudField.emplace_back(field);
    }
    TableSchema tableSchema = {
        .name = fdp.ConsumeRandomLengthString(),
        .sharedTableName = fdp.ConsumeRandomLengthString(),
        .fields = cloudField,
    };
    dataBaseSchema.tables.emplace_back(tableSchema);
    g_delegate->SetCloudDbSchema(dataBaseSchema);
}

void SetIAssetLoaderTest(FuzzedDataProvider &fdp)
{
    std::shared_ptr<IAssetLoader> loader = std::make_shared<VirtualAssetLoader>();
    g_delegate->SetIAssetLoader(loader);
}

void SetCloudSyncConfigTest(FuzzedDataProvider &fdp)
{
    CloudSyncConfig config = {
        .maxUploadCount = fdp.ConsumeIntegralInRange<int32_t>(
            CloudDbConstant::MIN_UPLOAD_BATCH_COUNT, CloudDbConstant::MAX_UPLOAD_BATCH_COUNT),
        .maxUploadSize =
            fdp.ConsumeIntegralInRange<int32_t>(CloudDbConstant::MIN_UPLOAD_SIZE, CloudDbConstant::MAX_UPLOAD_SIZE),
        .maxRetryConflictTimes = fdp.ConsumeIntegralInRange<int32_t>(CloudDbConstant::MIN_RETRY_CONFLICT_COUNTS, MOD),
        .isSupportEncrypt = fdp.ConsumeBool(),
    };
    g_delegate->SetCloudSyncConfig(config);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Setup();
    FuzzedDataProvider fdp(data, size);
    OHOS::SetStoreConfigTest(fdp);
    OHOS::SetCloudDBTest(fdp);
    OHOS::SetCloudDBSchemaTest(fdp);
    OHOS::SetIAssetLoaderTest(fdp);
    OHOS::SetCloudSyncConfigTest(fdp);
    OHOS::TearDown();
    return 0;
}
