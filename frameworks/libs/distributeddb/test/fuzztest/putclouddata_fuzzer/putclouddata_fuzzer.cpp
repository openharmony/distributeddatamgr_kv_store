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

#include "putclouddata_fuzzer.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer_data.h"
#include "cloud/cloud_db_types.h"
#include "icloud_sync_storage_interface.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_sync_able_storage.h"
#include "sqlite_relational_store.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;

constexpr const char *DB_SUFFIX = ".db";
constexpr const char *STORE_ID = "Relational_Store_ID";
std::string g_dbDir;
std::string g_testDir;
std::string g_tableName = "sync_data";
std::string g_storePath;
std::string g_gid = "abcd";
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
IRelationalStore *g_store = nullptr;
RelationalStoreDelegate *g_delegate = nullptr;
ICloudSyncStorageInterface *g_cloudStore = nullptr;

sqlite3 *CreateDataBase(const std::string &dbUri)
{
    LOGD("Create database: %s", dbUri.c_str());
    sqlite3 *db = nullptr;
    if (int r = sqlite3_open_v2(dbUri.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        LOGE("Open database [%s] failed. %d", dbUri.c_str(), r);
        if (db != nullptr) {
            (void)sqlite3_close_v2(db);
            db = nullptr;
        }
    }
    return db;
}

int ExecSql(sqlite3 *db, const std::string &sql)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK && errMsg != nullptr) {
        LOGE("Execute sql failed. %d err: %s", errCode, errMsg);
    }
    sqlite3_free(errMsg);
    return errCode;
}

void CreatDB()
{
    sqlite3 *db = CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ExecSql(db, "PRAGMA journal_mode=WAL;");
    sqlite3_close_v2(db);
}

void SetCloudSchema()
{
    TableSchema tableSchema;
    Field field1 = { "id", TYPE_INDEX<int64_t>, true, false };
    Field field2 = { "name", TYPE_INDEX<std::string>, false, true };
    Field field3 = { "age", TYPE_INDEX<double>, false, true };
    Field field4 = { "sex", TYPE_INDEX<bool>, false, true };
    Field field5 = { "image", TYPE_INDEX<Bytes>, false, true };
    tableSchema = { g_tableName, { field1, field2, field3, field4, field5} };

    DataBaseSchema dbSchema;
    dbSchema.tables = { tableSchema };
    g_cloudStore->SetCloudDbSchema(dbSchema);
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
    InitStoreProp(g_storePath, APP_ID, USER_ID, properties);
    int errCode = E_OK;
    g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
    if (g_store == nullptr) {
        LOGE("Get db failed:%d", errCode);
        return nullptr;
    }
    return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
}

enum class PrimaryKeyType {
    NO_PRIMARY_KEY,
    SINGLE_PRIMARY_KEY,
    COMPOSITE_PRIMARY_KEY
};

void SetCloudSchema(PrimaryKeyType pkType, bool nullable)
{
    TableSchema tableSchema;
    bool isIdPk = pkType == PrimaryKeyType::SINGLE_PRIMARY_KEY || pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY;
    Field field1 = { "id", TYPE_INDEX<int64_t>, isIdPk, !isIdPk };
    Field field2 = { "name", TYPE_INDEX<std::string>, pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY, true };
    Field field3 = { "age", TYPE_INDEX<double>, false, true };
    Field field4 = { "sex", TYPE_INDEX<bool>, false, nullable };
    Field field5 = { "image", TYPE_INDEX<Bytes>, false, true };
    tableSchema = { g_tableName, { field1, field2, field3, field4, field5} };

    DataBaseSchema dbSchema;
    dbSchema.tables = { tableSchema };
    g_cloudStore->SetCloudDbSchema(dbSchema);
}

int InsertData(const std::string &tableName, sqlite3 *db, PrimaryKeyType pkType)
{
    std::string sql;
    if (pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY) {
        sql = "insert into " + tableName + "(id, name, age)" \
            " values(1, 'zhangsan1', 10.1), (1, 'zhangsan2', 10.1), (2, 'zhangsan1', 10.0), (3, 'zhangsan3', 30),"
            " (4, 'zhangsan4', 40.123), (5, 'zhangsan5', 50.123);";
    } else {
        sql = "insert into " + tableName + "(id, name)" \
            " values(1, 'zhangsan1'), (2, 'zhangsan2'), (3, 'zhangsan3'), (4, 'zhangsan4'),"
            " (5, 'zhangsan5'), (6, 'zhangsan6'), (7, 'zhangsan7');";
    }
    if (ExecSql(db, sql) != E_OK) {
        LOGE("insert data failed");
        return -1;
    }

    /**
     * @tc.steps:step4. preset cloud gid.
     * @tc.expected: step4. return ok.
     */
    for (int i = 0; i < 7; i++) { // update first 7 records
        if (i == 4) { // 4 is id
            sql = "update " + DBCommon::GetLogTableName(tableName) + " set cloud_gid = '" +
                g_gid + std::to_string(i) + "', flag = 6 where data_key = " + std::to_string(i + 1);
        } else {
            sql = "update " + DBCommon::GetLogTableName(tableName) + " set cloud_gid = '" +
                g_gid + std::to_string(i) + "' where data_key = " + std::to_string(i + 1);
        }
        if (ExecSql(db, sql) != E_OK) {
            LOGE("modify gid failed");
            return -1;
        }
        if (pkType != PrimaryKeyType::COMPOSITE_PRIMARY_KEY && i == 6) { // 6 is index
            sql = "delete from " + tableName + " where id = 7;";
            ExecSql(db, sql);
        }
    }
    return 0;
}

int PrepareDataBase(const std::string &tableName, PrimaryKeyType pkType, bool nullable = true)
{
    /**
     * @tc.steps:step1. create table.
     * @tc.expected: step1. return ok.
     */
    sqlite3 *db = CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    if (db == nullptr) {
        LOGE("create db failed");
        return -1;
    }
    std::string sql;
    if (pkType == PrimaryKeyType::SINGLE_PRIMARY_KEY) {
        sql = "create table " + tableName + "(id int primary key, name TEXT, age REAL, sex INTEGER, image BLOB);";
    } else if (pkType == PrimaryKeyType::NO_PRIMARY_KEY) {
        sql = "create table " + tableName + "(id int, name TEXT, age REAL, sex INTEGER, image BLOB);";
    } else {
        sql = "create table " + tableName + "(id int, name TEXT, age REAL, sex INTEGER, image BLOB," \
            " PRIMARY KEY(id, name))";
    }
    if (ExecSql(db, sql) != E_OK) {
        LOGE("create table failed");
        sqlite3_close_v2(db);
        return -1;
    }

    /**
     * @tc.steps:step2. create distributed table with CLOUD_COOPERATION mode.
     * @tc.expected: step2. return ok.
     */
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, g_delegate);
    if (status != OK) {
        LOGE("OpenStore failed = %d", status);
        sqlite3_close_v2(db);
        return -1;
    }
    if (g_delegate->CreateDistributedTable(tableName, DistributedDB::CLOUD_COOPERATION) != OK) {
        LOGE("create distributed table failed");
        sqlite3_close_v2(db);
        return -1;
    }

    /**
     * @tc.steps:step3. insert some row.
     * @tc.expected: step3. return ok.
     */
    if (InsertData(tableName, db, pkType) != 0) {
        sqlite3_close_v2(db);
        return -1;
    }

    sqlite3_close_v2(db);
    SetCloudSchema(pkType, nullable);
    return 0;
}

void Setup()
{
    CreatDB();
    g_cloudStore = (ICloudSyncStorageInterface *) GetRelationalStore();
    if (PrepareDataBase(g_tableName, PrimaryKeyType::SINGLE_PRIMARY_KEY) != 0) {
        return;
    }
}

void TearDown()
{
    RefObject::DecObjRef(g_store);
    g_mgr.CloseStore(g_delegate);
    g_delegate = nullptr;
    DistributedDBToolsTest::RemoveTestDbFiles(g_testDir);
}

void CombineTest(const uint8_t* data, size_t size)
{
    std::shared_ptr<StorageProxy> storageProxy = StorageProxy::GetCloudDb(g_cloudStore);
    if (storageProxy == nullptr) {
        LOGE("storageProxy is null");
        return;
    }
    if (storageProxy->StartTransaction(TransactType::IMMEDIATE) != E_OK) {
        LOGE("start transaction failed in put cloud data fuzz");
        return;
    }

    DownloadData downloadData;
    FuzzerData fuzzerData(data, size);
    uint32_t count = fuzzerData.GetUInt32() % 100; // 100 is max record count
    for (uint32_t i = 0; i < count; i++) {
        VBucket vBucket;
        vBucket["id"] = fuzzerData.GetUInt32();

        int32_t strLen = fuzzerData.GetUInt32() % 1000; // 1000 is mod
        std::string name = fuzzerData.GetString(strLen);
        vBucket["name"] = name;
        vBucket["age"] = fuzzerData.GetUInt32();

        vBucket["image"] = std::vector<uint8_t>(1, i);
        std::string gid = fuzzerData.GetString(strLen);
        vBucket[CloudDbConstant::GID_FIELD] = gid;
        uint64_t ctime = fuzzerData.GetUInt64();
        int64_t rCtime = ctime > INT64_MAX ? INT64_MAX : ctime;
        vBucket[CloudDbConstant::CREATE_FIELD] = rCtime;
        uint64_t mtime = fuzzerData.GetUInt64();
        int64_t rMtime = mtime > INT64_MAX ? INT64_MAX : ctime;
        vBucket[CloudDbConstant::MODIFY_FIELD] = rMtime;
        downloadData.data.push_back(vBucket);
        downloadData.opType.push_back(static_cast<OpType>(fuzzerData.GetUInt32()));

        DataInfoWithLog dataInfoWithLog;
        VBucket assetInfo;
        storageProxy->GetInfoByPrimaryKeyOrGid(g_tableName, vBucket, dataInfoWithLog, assetInfo);
    }

    storageProxy->PutCloudSyncData(g_tableName, downloadData);
    storageProxy->Commit();
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