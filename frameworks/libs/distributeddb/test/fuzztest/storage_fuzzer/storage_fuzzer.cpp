/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "storage_fuzzer.h"

#include "db_errno.h"
#include "distributeddb_data_generate_unit_test.h"
#include "relational_sync_able_storage.h"
#include "relational_store_instance.h"
#include "sqlite_relational_store.h"
#include "log_table_manager_factory.h"

#include "cloud_db_types.h"
#include "distributeddb_tools_test.h"
#include "log_print.h"
#include "fuzzer_data.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "storage_proxy.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;

TableName TABLE_NAME_1 = "tableName1";
const auto STORE_ID = "Relational_Store_ID";
std::string TEST_DIR;
std::string STORE_PATH = "./g_store.db";
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
IRelationalStore *g_store = nullptr;
std::shared_ptr<StorageProxy> g_storageProxy = nullptr;

class StorageFuzzer {
public:
    void SetUp()
    {
        DistributedDBToolsTest::TestDirInit(TEST_DIR);
        LOGD("Test dir is %s", TEST_DIR.c_str());
        CreateDB();
        int ret = g_mgr.OpenStore(STORE_PATH, STORE_ID, RelationalStoreDelegate::Option {}, g_delegate);
        if (ret != DBStatus::OK) {
            LOGE("can not open store");
            return;
        }
        if (g_delegate == nullptr) {
            LOGE("unexpected g_delegate");
            return;
        }
        g_storageProxy = GetStorageProxy((ICloudSyncStorageInterface *) GetRelationalStore());
    }

    void TearDown()
    {
        if (g_delegate != nullptr) {
            if (g_mgr.CloseStore(g_delegate) != DBStatus::OK) {
                LOGE("Can not close store");
                return;
            }
            g_delegate = nullptr;
            g_storageProxy = nullptr;
        }
        if (DistributedDBToolsTest::RemoveTestDbFiles(TEST_DIR) != 0) {
            LOGE("rm test db files error.");
        }
    }

    void FuzzTest(const uint8_t* data, size_t size)
    {
        FuzzerData fuzzData(data, size);
        if (!SetAndGetLocalWaterMark(TABLE_NAME_1, fuzzData.GetUInt64())) {
            LOGE("Set and get local watermark unsuccess");
            return;
        }
        std::string cloudMark = fuzzData.GetString(size);
        if (!SetAndGetCloudWaterMark(TABLE_NAME_1, cloudMark)) {
            LOGE("Set and get cloud watermark unsuccess");
            return;
        }
    }

private:
    void CreateDB()
    {
        sqlite3 *db = nullptr;
        int errCode = sqlite3_open(STORE_PATH.c_str(), &db);
        if (errCode != SQLITE_OK) {
            LOGE("open db failed:%d", errCode);
            sqlite3_close(db);
            return;
        }

        const std::string sql = "PRAGMA journal_mode=WAL;";
        if (SQLiteUtils::ExecuteRawSQL(db, sql.c_str()) != E_OK) {
            LOGE("can not execute sql");
            return;
        }
        sqlite3_close(db);
    }

    void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
        RelationalDBProperties &properties)
    {
        properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
        properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
        properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
        properties.SetStringProp(RelationalDBProperties::STORE_ID, STORE_ID);
        std::string identifier = userId + "-" + appId + "-" + STORE_ID;
        std::string hashIdentifier = DBCommon::TransferHashString(identifier);
        properties.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
    }

    const RelationalSyncAbleStorage *GetRelationalStore()
    {
        RelationalDBProperties properties;
        InitStoreProp(STORE_PATH, APP_ID, USER_ID, properties);
        int errCode = E_OK;
        g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
        if (g_store == nullptr) {
            LOGE("Get db failed:%d", errCode);
            return nullptr;
        }
        return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
    }

    std::shared_ptr<StorageProxy> GetStorageProxy(ICloudSyncStorageInterface *store)
    {
        return StorageProxy::GetCloudDb(store);
    }

    bool SetAndGetLocalWaterMark(TableName &tableName, Timestamp mark)
    {
        if (g_storageProxy->PutLocalWaterMark(tableName, mark) != E_OK) {
            LOGE("Can not put local watermark");
            return false;
        }
        Timestamp retMark;
        if (g_storageProxy->GetLocalWaterMark(tableName, retMark) != E_OK) {
            LOGE("Can not get local watermark");
            return false;
        }
        if (retMark != mark) {
            LOGE("watermark in not conformed to expectation");
            return false;
        }
        return true;
    }

    bool SetAndGetCloudWaterMark(TableName &tableName, std::string &mark)
    {
        if (g_storageProxy->SetCloudWaterMark(tableName, mark) != E_OK) {
            LOGE("Can not set cloud watermark");
            return false;
        }
        std::string retMark;
        if (g_storageProxy->GetCloudWaterMark(tableName, retMark) != E_OK) {
            LOGE("Can not get cloud watermark");
            return false;
        }
        if (retMark != mark) {
            LOGE("watermark in not conformed to expectation");
            return false;
        }
        return true;
    }
};

StorageFuzzer *g_storageFuzzerTest = nullptr;

void Setup()
{
    LOGI("Set up");
    g_storageFuzzerTest = new(std::nothrow) StorageFuzzer();
    if (g_storageFuzzerTest == nullptr) {
        return;
    }
    g_storageFuzzerTest->SetUp();
}

void TearDown()
{
    LOGI("Tear down");
    g_storageFuzzerTest->TearDown();
    if (g_storageFuzzerTest != nullptr) {
        delete g_storageFuzzerTest;
        g_storageFuzzerTest = nullptr;
    }
}

void CombineTest(const uint8_t* data, size_t size)
{
    if (g_storageFuzzerTest == nullptr) {
        return;
    }
    g_storageFuzzerTest->FuzzTest(data, size);
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