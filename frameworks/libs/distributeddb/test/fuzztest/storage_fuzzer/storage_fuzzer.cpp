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

#include "cloud/cloud_db_types.h"
#include "db_errno.h"
#include "distributeddb_data_generate_unit_test.h"
#include "relational_sync_able_storage.h"
#include "relational_store_instance.h"
#include "sqlite_relational_store.h"
#include "log_table_manager_factory.h"

#include "distributeddb_tools_test.h"
#include "log_print.h"
#include "fuzzer_data.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "relational_store_client.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "storage_proxy.h"
#include "store_observer.h"
#include "store_types.h"
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
static const char *STOREID = "CLIENT_STORE_ID";
static const char *DBSUFFIX = ".db";
static const std::string g_prefix = "naturalbase_rdb_aux_";
static std::map<std::string, ChangeProperties> g_triggerTableData;
static int g_triggeredCount = 0;
static std::string g_dbDir;
static std::string g_testDir;
static std::mutex g_mutex;
static std::condition_variable g_cv;
static bool g_alreadyNotify = false;
DistributedDB::StoreObserver::StoreChangedInfo g_changedData;
static constexpr const int MOD = 1000; // 1000 is mod
static constexpr const int DIVNUM = 10;
static constexpr const int COUNT_MOD = 50000; // 50000 is mod of count

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

    void FuzzTest(FuzzedDataProvider &fdp)
    {
        if (!SetAndGetLocalWaterMark(TABLE_NAME_1, fdp.ConsumeIntegral<uint64_t>())) {
            LOGE("Set and get local watermark unsuccess");
            return;
        }
        std::string cloudMark = fdp.ConsumeRandomLengthString();
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
    DistributedDBToolsTest::TestDirInit(g_testDir);
    g_dbDir = g_testDir + "/";
}

void TearDown()
{
    LOGI("Tear down");
    g_storageFuzzerTest->TearDown();
    if (g_storageFuzzerTest != nullptr) {
        delete g_storageFuzzerTest;
        g_storageFuzzerTest = nullptr;
    }
    g_alreadyNotify = false;
    DistributedDBToolsTest::RemoveTestDbFiles(g_testDir);
}

void CombineTest(FuzzedDataProvider &fdp)
{
    if (g_storageFuzzerTest == nullptr) {
        return;
    }
    g_storageFuzzerTest->FuzzTest(fdp);
}

class ClientStoreObserver : public StoreObserver {
public:
    virtual ~ClientStoreObserver() {}
    void OnChange(StoreChangedInfo &&data) override
    {
        g_changedData = data;
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.notify_one();
        g_alreadyNotify = true;
    }
};

void ClientObserverFunc(const ClientChangedData &clientChangedData)
{
    for (const auto &tableEntry : clientChangedData.tableData) {
        LOGD("client observer failed, table: %s", tableEntry.first.c_str());
        g_triggerTableData.insert_or_assign(tableEntry.first, tableEntry.second);
    }
    g_triggeredCount++;
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_alreadyNotify = true;
    }
    g_cv.notify_one();
}

void CreateTableForStoreObserver(sqlite3 *db, const std::string &tableName)
{
    std::string sql = "create table " + tableName + "(id INTEGER primary key, name TEXT);";
    RdbTestUtils::ExecSql(db, sql);
    sql = "create table no_" + tableName + "(id INTEGER, name TEXT);";
    RdbTestUtils::ExecSql(db, sql);
    sql = "create table mult_" + tableName + "(id INTEGER, name TEXT, age int, ";
    sql += "PRIMARY KEY (id, name));";
    RdbTestUtils::ExecSql(db, sql);
}

void InitLogicDeleteData(sqlite3 *&db, const std::string &tableName, uint32_t num)
{
    for (size_t i = 0; i < num; ++i) {
        std::string sql = "insert or replace into " + tableName + " VALUES('" + std::to_string(i) + "', 'zhangsan');";
        RdbTestUtils::ExecSql(db, sql);
    }
    std::string sql = "update " + g_prefix + tableName + "_log" + " SET flag = flag | 0x08";
    RdbTestUtils::ExecSql(db, sql);
}

void InitDataStatus(const std::string &tableName, uint32_t count, sqlite3 *db)
{
    int type = 4; // the num of different status
    for (uint32_t i = 1; i <= (type * count) / DIVNUM; i++) {
        std::string sql = "INSERT INTO " + tableName + " VALUES(" + std::to_string(i) + ", 'zhangsan" +
            std::to_string(i) + "');";
        RdbTestUtils::ExecSql(db, sql);
    }
    std::string countStr = std::to_string(count);
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET status=(CASE WHEN data_key<=" +
        countStr + " THEN 0 WHEN data_key>" + countStr + " AND data_key<=2*" + countStr + " THEN 1 WHEN data_key>2*" +
        countStr + " AND data_key<=3*" + countStr + " THEN 2 ELSE 3 END)";
    RdbTestUtils::ExecSql(db, sql);
}

void GetHashKey(const std::string &tableName, const std::string &condition, sqlite3 *db,
    std::vector<std::vector<uint8_t>> &hashKey)
{
    sqlite3_stmt *stmt = nullptr;
    std::string sql = "select hash_key from " + DBCommon::GetLogTableName(tableName) + " where " + condition;
    SQLiteUtils::GetStatement(db, sql, stmt);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::vector<uint8_t> blob;
        SQLiteUtils::GetColumnBlobValue(stmt, 0, blob);
        hashKey.push_back(blob);
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void ClientObserverFuzz(const std::string &tableName)
{
    sqlite3 *db = RdbTestUtils::CreateDataBase(g_dbDir + STOREID + DBSUFFIX);
    ClientObserver clientObserver = [](const ClientChangedData &data) { return ClientObserverFunc(data); };
    RegisterClientObserver(db, clientObserver);
    std::string sql = "insert into " + tableName + " VALUES(1, 'zhangsan'), (2, 'lisi'), (3, 'wangwu');";
    RdbTestUtils::ExecSql(db, sql);
    UnRegisterClientObserver(db);
    sqlite3_close_v2(db);
}

void StoreObserverFuzz(const std::string &tableName)
{
    sqlite3 *db = RdbTestUtils::CreateDataBase(g_dbDir + STOREID + DBSUFFIX);
    RdbTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;");
    CreateTableForStoreObserver(db, tableName);
    auto storeObserver = std::make_shared<ClientStoreObserver>();
    RegisterStoreObserver(db, storeObserver);
    UnregisterStoreObserver(db);
    auto storeObserver1 = std::make_shared<ClientStoreObserver>();
    RegisterStoreObserver(db, storeObserver1);
    UnregisterStoreObserver(db, storeObserver1);
    sqlite3_close_v2(db);
}

void DropLogicDeletedDataFuzz(std::string tableName, uint32_t num)
{
    sqlite3 *db = RdbTestUtils::CreateDataBase(g_dbDir + STOREID + DBSUFFIX);
    InitLogicDeleteData(db, tableName, num);
    DropLogicDeletedData(db, tableName, 0u);
    sqlite3_close_v2(db);
}

void LockAndUnLockFuzz(std::string tableName, uint32_t count)
{
    sqlite3 *db = RdbTestUtils::CreateDataBase(g_dbDir + STOREID + DBSUFFIX);
    InitDataStatus(tableName, count, db);
    std::vector<std::vector<uint8_t>> hashKey;
    GetHashKey(tableName, " 1=1 ", db, hashKey);
    Lock(tableName, hashKey, db);
    UnLock(tableName, hashKey, db);
    sqlite3_close_v2(db);
}

void CombineClientFuzzTest(FuzzedDataProvider &fdp)
{
    uint32_t len = fdp.ConsumeIntegralInRange<uint32_t>(0, MOD);
    std::string tableName = fdp.ConsumeRandomLengthString(len);
    ClientObserverFuzz(tableName);
    StoreObserverFuzz(tableName);
    uint32_t num = fdp.ConsumeIntegralInRange<uint32_t>(0, COUNT_MOD);
    DropLogicDeletedDataFuzz(tableName, num);
    uint32_t count = fdp.ConsumeIntegralInRange<uint32_t>(0, COUNT_MOD);
    LockAndUnLockFuzz(tableName, count);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::Setup();
    FuzzedDataProvider fdp(data, size);
    OHOS::CombineTest(fdp);
    OHOS::CombineClientFuzzTest(fdp);
    OHOS::TearDown();
    return 0;
}
