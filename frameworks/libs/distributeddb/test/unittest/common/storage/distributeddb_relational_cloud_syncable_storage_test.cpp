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
#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "cloud/cloud_db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_table_manager_factory.h"
#include "query_sync_object.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_sync_able_storage.h"
#include "runtime_config.h"
#include "sqlite_relational_store.h"
#include "time_helper.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"


using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_storeID = "Relational_Store_ID";
string g_tableName = "cloudData";
string g_logTblName;
string g_testDir;
string g_storePath;
const Timestamp g_startTime = 100000;
const int g_deleteFlag = 0x01;
const int g_localFlag = 0x02;
const std::string CREATE_LOCAL_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS " + g_tableName + "(" \
    "name TEXT ," \
    "height REAL ," \
    "married INT ," \
    "photo BLOB ," \
    "assert BLOB," \
    "asserts BLOB," \
    "age INT);";
const std::vector<Field> g_cloudFiled = {
    {"name", TYPE_INDEX<std::string>}, {"age", TYPE_INDEX<int64_t>},
    {"height", TYPE_INDEX<double>}, {"married", TYPE_INDEX<bool>}, {"photo", TYPE_INDEX<Bytes>},
    {"assert", TYPE_INDEX<Asset>}, {"asserts", TYPE_INDEX<Assets>}
};
const Asset g_localAsset = {
    .version = 1, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/local/sync",
    .modifyTime = "123456", .createTime = "", .size = "256", .hash = " ",
    .flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE), .status = static_cast<uint32_t>(AssetStatus::NORMAL),
    .timestamp = 0L
};
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
IRelationalStore *g_store = nullptr;
ICloudSyncStorageInterface *g_cloudStore = nullptr;
std::shared_ptr<StorageProxy> g_storageProxy = nullptr;
TableSchema g_tableSchema;

void CreateDB()
{
    sqlite3 *db = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        sqlite3_close(db);
        return;
    }

    const string sql =
        "PRAGMA journal_mode=WAL;";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    sqlite3_close(db);
}

void CreateLogTable()
{
    TableInfo table;
    table.SetTableName(g_tableName);
    table.SetTableSyncType(TableSyncType::CLOUD_COOPERATION);
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    auto tableManager =
        LogTableManagerFactory::GetTableManager(DistributedTableMode::COLLABORATION, TableSyncType::CLOUD_COOPERATION);
    int errCode = tableManager->CreateRelationalLogTable(db, table);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close(db);
}

void CreateAndInitUserTable(int64_t count, int64_t photoSize, const Asset &expect)
{
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);

    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, CREATE_LOCAL_TABLE_SQL), E_OK);
    std::string photo(photoSize, 'v');
    std::vector<uint8_t> assetBlob;
    std::vector<uint8_t> assetsBlob;
    Asset asset = expect;
    int id = 0;
    Assets assets;
    asset.name = expect.name + std::to_string(id++);
    assets.push_back(asset);
    asset.name = expect.name + std::to_string(id++);
    assets.push_back(asset);
    int errCode;
    ASSERT_EQ(RuntimeContext::GetInstance()->AssetToBlob(expect, assetBlob), E_OK);
    ASSERT_EQ(RuntimeContext::GetInstance()->AssetsToBlob(assets, assetsBlob), E_OK);
    for (int i = 1; i <= count; ++i) {
        string sql = "INSERT OR REPLACE INTO " + g_tableName +
            " (name, height, married, photo, assert, asserts, age) VALUES ('Tom" + std::to_string(i) +
            "', '175.8', '0', '" + photo + "', ? , ?,  '18');";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        if (SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false) != E_OK) { // 1 is asset index
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }
        if (SQLiteUtils::BindBlobToStatement(stmt, 2, assetsBlob, false) != E_OK) { // 2 is index of asserts
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
    sqlite3_close(db);
}

void InitLogData(int64_t insCount, int64_t updCount, int64_t delCount, int64_t excludeCount)
{
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    std::string flag;
    std::string cloudGid;
    for (int64_t i = 1; i <= insCount + updCount + delCount + excludeCount; ++i) {
        std::string index = std::to_string(i);
        if (i <= insCount) {
            flag = std::to_string(g_localFlag);
            cloudGid = "''";
        } else if (i > insCount && i <= insCount + updCount) {
            flag = std::to_string(g_localFlag);
            cloudGid = "'" + g_storeID + index + "'";
        } else if (i > (insCount + updCount) && i <= (insCount + updCount + delCount)) {
            flag = std::to_string(g_localFlag | g_deleteFlag);
            cloudGid = "'" + g_storeID + index + "'";
        } else {
            flag = std::to_string(g_localFlag | g_deleteFlag);
            cloudGid = "''";
        }
        Bytes hashKey(index.begin(), index.end());
        string sql = "INSERT OR REPLACE INTO " + g_logTblName +
            " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cloud_gid)" +
            " VALUES ('" + std::to_string(i) + "', '', '', '" +  std::to_string(g_startTime + i) + "', '" +
            std::to_string(g_startTime + i) + "','" + flag + "', ? , " + cloudGid + ");";
        sqlite3_stmt *stmt = nullptr;
        int errCode = E_OK;
        EXPECT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        EXPECT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, hashKey, false), E_OK);
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
    sqlite3_close(db);
}

void InitLogGid(int64_t count)
{
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    for (int i = 1; i <= count; i++) {
        string sql = "update " + g_logTblName + " set cloud_gid = '" + std::to_string(i) +
            "' where data_key = " + std::to_string(i);
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    }
    sqlite3_close(db);
}

void UpdateLogGidAndHashKey(int64_t count)
{
    sqlite3 *db = nullptr;
    int errCode;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    for (int i = 1; i <= count; i++) {
        std::string id = std::to_string(i);
        string sql = "update " + g_logTblName + " set cloud_gid = '" + id +
            "' , hash_key = ? where data_key = " + id;
        sqlite3_stmt *stmt = nullptr;
        EXPECT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        Bytes hashKey(id.begin(), id.end());
        EXPECT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, hashKey, false), E_OK);
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
    sqlite3_close(db);
}

void CheckGetAsset(VBucket &assets, uint32_t status)
{
    EXPECT_EQ(assets.size(), 2u);
    ASSERT_TRUE(assets["assert"].index() == TYPE_INDEX<Asset>);
    ASSERT_TRUE(assets["asserts"].index() == TYPE_INDEX<Assets>);
    Asset data1 = std::get<Asset>(assets["assert"]);
    ASSERT_EQ(data1.status, status);
    Assets data2 = std::get<Assets>(assets["asserts"]);
    ASSERT_GT(data2.size(), 0u);
    ASSERT_EQ(data2[0].status, static_cast<uint32_t>(AssetStatus::NORMAL));
}

void InitLogicDelete(int64_t count)
{
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    for (int i = 1; i <= count; i++) {
        string sql = "update " + g_logTblName + " set flag = flag | 0x09"
                     " where data_key = " + std::to_string(i);
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    }
    sqlite3_close(db);
}

void ConstructMultiDownloadData(int64_t count, DownloadData &downloadData, std::vector<OpType> &opTypes)
{
    for (size_t i = 1; i <= opTypes.size(); i++) {
        Asset asset = g_localAsset;
        Assets assets;
        VBucket vBucket;
        if (i <= 2) { // 2 is deleted or insert type
            asset.flag = static_cast<uint32_t>(i == 1 ? AssetOpType::DELETE : AssetOpType::INSERT);
            vBucket[CloudDbConstant::GID_FIELD] = (i == 1 ? std::to_string(i) : std::to_string(count + i));
        } else {
            asset.flag = static_cast<uint32_t>(AssetOpType::UPDATE);
            vBucket[CloudDbConstant::GID_FIELD] = std::to_string(i);
        }
        vBucket["assert"] = asset;
        asset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
        assets.push_back(asset);
        asset.flag = static_cast<uint32_t>(AssetOpType::INSERT);
        assets.push_back(asset);
        asset.flag = static_cast<uint32_t>(AssetOpType::DELETE);
        assets.push_back(asset);
        asset.flag = static_cast<uint32_t>(AssetOpType::UPDATE);
        assets.push_back(asset);
        vBucket["asserts"] = assets;
        std::string name = "lisi" + std::to_string(i);
        vBucket["name"] = name;
        vBucket["age"] = (int64_t)i;
        int64_t mTime = 12345679L + i;
        vBucket[CloudDbConstant::MODIFY_FIELD] = mTime;
        vBucket[CloudDbConstant::CREATE_FIELD] = mTime;
        downloadData.data.push_back(vBucket);
    }
    downloadData.opType = opTypes;
}

void AddVersionToDownloadData(DownloadData &downloadData)
{
    for (size_t i = 0; i < downloadData.data.size(); i++) {
        downloadData.data[i].insert_or_assign(CloudDbConstant::VERSION_FIELD, std::string("11"));
    }
}

void AddCloudOwnerToDownloadData(DownloadData &downloadData)
{
    for (size_t i = 0; i < downloadData.data.size(); i++) {
        downloadData.data[i].insert_or_assign(CloudDbConstant::CLOUD_OWNER, std::to_string(i));
    }
}

void SetDbSchema(const TableSchema &tableSchema)
{
    DataBaseSchema dataBaseSchema;
    dataBaseSchema.tables.push_back(tableSchema);
    EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), E_OK);
}

void InitUserDataForAssetTest(int64_t insCount, int64_t photoSize, const Asset &expect)
{
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, CREATE_LOCAL_TABLE_SQL), E_OK);
    sqlite3_close(db);
    EXPECT_EQ(g_delegate->CreateDistributedTable(g_tableName, DistributedDB::CLOUD_COOPERATION), OK);
    CreateAndInitUserTable(insCount, photoSize, expect);
    SetDbSchema(g_tableSchema);
}

int QueryCountCallback(void *data, int count, char **colValue, char **colName)
{
    if (count != 1) {
        return 0;
    }
    auto expectCount = reinterpret_cast<int64_t>(data);
    EXPECT_EQ(strtol(colValue[0], nullptr, 10), expectCount); // 10: decimal
    return 0;
}

void fillCloudAssetTest(int64_t count, AssetStatus statusType, bool isDownloadSuccess)
{
    VBucket vBucket;
    vBucket[CloudDbConstant::GID_FIELD] = std::to_string(1);
    for (int i = 0; i < 4; i ++) { // 4 is AssetStatus Num
        Asset asset = g_localAsset;
        asset.flag = i;
        asset.status = static_cast<uint32_t>(statusType);
        asset.timestamp = g_startTime;
        Assets assets;
        for (int j = 0; j < 4; j++) { // 4 is AssetStatus Num
            Asset temp = g_localAsset;
            temp.flag = j;
            temp.status = static_cast<uint32_t>(statusType);
            temp.timestamp = g_startTime + j;
            assets.push_back(temp);
        }
        vBucket["assert"] = asset;
        vBucket["asserts"] = assets;
        ASSERT_EQ(g_storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
        ASSERT_EQ(g_storageProxy->FillCloudAssetForDownload(g_tableName, vBucket, isDownloadSuccess), E_OK);
        ASSERT_EQ(g_storageProxy->Commit(), E_OK);
    }
}

void UpdateLocalAsset(const std::string &tableName, Asset &asset, int64_t rowid)
{
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql = "UPDATE " + tableName + " SET assert = ? where rowid = '" + std::to_string(rowid) + "';";
    std::vector<uint8_t> assetBlob;
    int errCode;
    RuntimeContext::GetInstance()->AssetToBlob(asset, assetBlob);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    if (SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false) == E_OK) {
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    sqlite3_close(db);
}

void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
    RelationalDBProperties &properties)
{
    properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
    properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
    properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
    properties.SetStringProp(RelationalDBProperties::STORE_ID, g_storeID);
    std::string identifier = userId + "-" + appId + "-" + g_storeID;
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

std::shared_ptr<StorageProxy> GetStorageProxy(ICloudSyncStorageInterface *store)
{
    return StorageProxy::GetCloudDb(store);
}

class DistributedDBRelationalCloudSyncableStorageTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};


void DistributedDBRelationalCloudSyncableStorageTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/cloudDataTest.db";
    g_logTblName = DBConstant::RELATIONAL_PREFIX + g_tableName + "_log";
    LOGI("The test db is:%s", g_testDir.c_str());
    RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
    g_tableSchema.name = g_tableName;
    g_tableSchema.sharedTableName = g_tableName + "_shared";
    g_tableSchema.fields = g_cloudFiled;
}

void DistributedDBRelationalCloudSyncableStorageTest::TearDownTestCase(void)
{}

void DistributedDBRelationalCloudSyncableStorageTest::SetUp(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    LOGD("Test dir is %s", g_testDir.c_str());
    CreateDB();
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    g_cloudStore = (ICloudSyncStorageInterface *) GetRelationalStore();
    ASSERT_NE(g_cloudStore, nullptr);
    g_storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(g_storageProxy, nullptr);
    ASSERT_EQ(g_delegate->SetIAssetLoader(std::make_shared<VirtualAssetLoader>()), DBStatus::OK);
}

void DistributedDBRelationalCloudSyncableStorageTest::TearDown(void)
{
    RefObject::DecObjRef(g_store);
    if (g_delegate != nullptr) {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
        g_delegate = nullptr;
    }
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: MetaDataTest001
 * @tc.desc: Test PutMetaData and GetMetaData from ICloudSyncStorageInterface impl class
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, MetaDataTest001, TestSize.Level0)
{
    EXPECT_EQ(g_cloudStore->PutMetaData(KEY_1, VALUE_2), E_OK);
    EXPECT_EQ(g_cloudStore->PutMetaData(KEY_1, VALUE_3), E_OK);

    Value value;
    EXPECT_EQ(g_cloudStore->GetMetaData(KEY_1, value), E_OK);
    EXPECT_EQ(value, VALUE_3);
}

/**
 * @tc.name: MetaDataTest002
 * @tc.desc: The GetMetaData supports key sizes up to 1024
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, MetaDataTest002, TestSize.Level0)
{
    const string str(DBConstant::MAX_KEY_SIZE, 'k');
    const Key key(str.begin(), str.end());
    EXPECT_EQ(g_cloudStore->PutMetaData(key, VALUE_2), E_OK);
    Value value;
    EXPECT_EQ(g_cloudStore->GetMetaData(key, value), E_OK);
    EXPECT_EQ(value, VALUE_2);

    const string maxStr(DBConstant::MAX_KEY_SIZE + 1, 'k');
    const Key maxKey(maxStr.begin(), maxStr.end());
    EXPECT_EQ(g_cloudStore->PutMetaData(maxKey, VALUE_3), E_OK);
    EXPECT_EQ(g_cloudStore->GetMetaData(maxKey, value), -E_INVALID_ARGS);
}

/**
 * @tc.name: TransactionTest001
 * @tc.desc: No write transaction in the current store, meta interface can called
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
  */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest001, TestSize.Level0)
{
    /**
     * @tc.steps: allow get or put meta in read transaction
     * @tc.expected: Succeed, return OK.
     */
    EXPECT_EQ(g_cloudStore->StartTransaction(TransactType::DEFERRED), E_OK);
    g_cloudStore->PutMetaData(KEY_1, VALUE_1);
    EXPECT_EQ(g_cloudStore->Rollback(), E_OK);
    g_cloudStore->PutMetaData(KEY_2, VALUE_2);

    Value value;
    EXPECT_EQ(g_cloudStore->StartTransaction(TransactType::DEFERRED), E_OK);
    EXPECT_EQ(g_cloudStore->GetMetaData(KEY_1, value), E_OK);
    EXPECT_EQ(g_cloudStore->GetMetaData(KEY_2, value), E_OK);
    g_cloudStore->PutMetaData(KEY_3, VALUE_3);
    EXPECT_EQ(g_cloudStore->GetMetaData(KEY_3, value), E_OK);
    EXPECT_EQ(g_cloudStore->Commit(), E_OK);
    EXPECT_EQ(g_cloudStore->GetMetaData(KEY_3, value), E_OK);
}

/**
 * @tc.name: TransactionTest002
 * @tc.desc: Test transaction interface from StorageProxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest002, TestSize.Level0)
{
    Timestamp cloudTime = 666888;
    Timestamp localTime;
    EXPECT_EQ(g_storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);

    /**
     * @tc.steps: allow get or put waterMark in read transaction
     * @tc.expected: Succeed, return OK.
     */
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->PutLocalWaterMark(g_tableName, cloudTime), E_OK);
    EXPECT_EQ(g_storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
    EXPECT_EQ(cloudTime, localTime);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);
    EXPECT_EQ(g_storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);

    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
    cloudTime = 999666;
    EXPECT_EQ(g_storageProxy->PutLocalWaterMark(g_tableName, cloudTime), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
    EXPECT_EQ(g_storageProxy->PutLocalWaterMark(g_tableName, cloudTime), E_OK);
    EXPECT_EQ(g_storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
    EXPECT_EQ(cloudTime, localTime);

    /**
     * @tc.steps: not allow get or put waterMark in write transaction
     * @tc.expected: return -E_BUSY.
     */
    EXPECT_EQ(g_storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    EXPECT_EQ(g_storageProxy->GetLocalWaterMark(g_tableName, localTime), -E_BUSY);
    EXPECT_EQ(g_storageProxy->PutLocalWaterMark(g_tableName, cloudTime), -E_BUSY);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);
    EXPECT_EQ(g_storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
}

/**
 * @tc.name: TransactionTest003
 * @tc.desc: Repeatedly call transaction interface from StorageProxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest003, TestSize.Level0)
{
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);

    /**
     * @tc.steps: Repeated start transactions is not allowed
     * @tc.expected: return -E_TRANSACT_STATE.
     */
    EXPECT_EQ(g_storageProxy->StartTransaction(), -E_TRANSACT_STATE);

    /**
     * @tc.steps: Repeated commit is not allowed
     * @tc.expected: return -E_INVALID_DB.
     */
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), -E_INVALID_DB);

    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);

    /**
     * @tc.steps: Repeated Rollback is not allowed
     * @tc.expected: return -E_INVALID_DB.
     */
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);
    EXPECT_EQ(g_storageProxy->Rollback(), -E_INVALID_DB);
}

/**
 * @tc.name: TransactionTest004
 * @tc.desc: Call transaction after close storageProxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
  */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest004, TestSize.Level0)
{
    /**
     * @tc.steps: transaction is not allowed after closing the proxy
     * @tc.expected: return -E_INVALID_DB.
     */
    EXPECT_EQ(g_storageProxy->Close(), E_OK);
    EXPECT_EQ(g_storageProxy->StartTransaction(), -E_INVALID_DB);
    EXPECT_EQ(g_storageProxy->Commit(), -E_INVALID_DB);
    EXPECT_EQ(g_storageProxy->Rollback(), -E_INVALID_DB);

    g_storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(g_storageProxy, nullptr);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);

    /**
     * @tc.steps: close proxy is not allowed before the transaction has been commit or rollback
     * @tc.expected: return -E_BUSY.
     */
    EXPECT_EQ(g_storageProxy->Close(), -E_BUSY);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);
    EXPECT_EQ(g_storageProxy->Close(), E_OK);
}

/**
 * @tc.name: GetUploadCount001
 * @tc.desc: Test getUploadCount by ICloudSyncStorageInterface
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetUploadCount001, TestSize.Level0)
{
    /**
     * @tc.steps: Table does not exist
     * @tc.expected: return -SQLITE_ERROR.
     */
    int64_t resCount = 0;
    QuerySyncObject query;
    query.SetTableName(g_tableName);
    EXPECT_EQ(g_cloudStore->GetUploadCount(query, g_startTime, false, false, resCount), -E_INVALID_QUERY_FORMAT);

    CreateLogTable();
    int64_t insCount = 100;
    CreateAndInitUserTable(insCount, insCount, g_localAsset);
    InitLogData(insCount, insCount, insCount, insCount);
    EXPECT_EQ(g_cloudStore->GetUploadCount(query, g_startTime, false, false, resCount), E_OK);
    EXPECT_EQ(resCount, insCount + insCount + insCount);

    /**
     * @tc.steps: There are no matching data anymore
     * @tc.expected: count is 0 and return E_OK.
     */
    Timestamp invalidTime = g_startTime + g_startTime;
    EXPECT_EQ(g_cloudStore->GetUploadCount(query, invalidTime, false, false, resCount), E_OK);
    EXPECT_EQ(resCount, 0);
}

/**
 * @tc.name: GetUploadCount002
 * @tc.desc: Test getUploadCount by storageProxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
  */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetUploadCount002, TestSize.Level0)
{
    CreateLogTable();
    int64_t insCount = 100;
    InitLogData(insCount, insCount, 0, insCount);
    CreateAndInitUserTable(insCount, insCount, g_localAsset);
    int64_t resCount = 0;

    /**
     * @tc.steps: GetUploadCount must be called under transaction
     * @tc.expected: return -E_TRANSACT_STATE.
     */
    EXPECT_EQ(g_storageProxy->GetUploadCount(g_tableName, g_startTime, false, resCount), -E_TRANSACT_STATE);

    int timeOffset = 30;
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->GetUploadCount(g_tableName, g_startTime + timeOffset, false, resCount), E_OK);
    EXPECT_EQ(resCount, insCount + insCount - timeOffset);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);

    /**
     * @tc.steps: GetUploadCount also can be called under write transaction
     * @tc.expected: return E_OK.
     */
    EXPECT_EQ(g_storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    EXPECT_EQ(g_storageProxy->GetUploadCount(g_tableName, g_startTime + timeOffset, false, resCount), E_OK);
    EXPECT_EQ(resCount, insCount + insCount - timeOffset);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: GetUploadCount003
 * @tc.desc: Test getUploadCount exclude condition of (deleteFlag and cloud_gid is '')
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetUploadCount003, TestSize.Level0)
{
    CreateLogTable();
    int64_t insCount = 100;
    CreateAndInitUserTable(insCount, insCount, g_localAsset);
    InitLogData(0, 0, insCount, insCount);
    int64_t resCount = 0;

    /**
     * @tc.steps: GetUploadCount must be called under transaction
     * @tc.expected: return -E_TRANSACT_STATE.
     */
    EXPECT_EQ(g_storageProxy->GetUploadCount(g_tableName, g_startTime, false, resCount), -E_TRANSACT_STATE);

    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->GetUploadCount(g_tableName, g_startTime, false, resCount), E_OK);
    EXPECT_EQ(resCount, insCount);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: FillCloudGid001
 * @tc.desc: FillCloudGid with invalid parm
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudGid001, TestSize.Level0)
{
    CreateLogTable();
    int64_t insCount = 100;
    InitLogData(insCount, 0, insCount, insCount);
    CloudSyncData syncData(g_tableName);
    SetDbSchema(g_tableSchema);

    /**
     * @tc.steps: rowid set is empty
     * @tc.expected: return -E_INVALID_ARGS.
     */
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), -E_INVALID_ARGS);
    syncData.insData.rowid.push_back(1);
    syncData.insData.rowid.push_back(2); // 2 is random id

    /**
     * @tc.steps: insData set is empty
     * @tc.expected: return -E_INVALID_ARGS.
     */
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), -E_INVALID_ARGS);
    VBucket bucket1;
    bucket1.insert_or_assign(g_tableName, g_tableName);
    bucket1.insert_or_assign(CloudDbConstant::GID_FIELD, 1L);
    syncData.insData.extend.push_back(bucket1);

    /**
     * @tc.steps: the size of rowid and insData is not equal
     * @tc.expected: return -E_INVALID_ARGS.
     */
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), -E_INVALID_ARGS);

    /**
     * @tc.steps: table name is empty
     * @tc.expected: return -SQLITE_ERROR.
     */
    VBucket bucket2;
    bucket2.insert_or_assign(CloudDbConstant::CREATE_FIELD, 2L); // 2L is random field
    syncData.insData.extend.push_back(bucket2);
    syncData.tableName = "";
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), -E_NOT_FOUND);

    /**
     * @tc.steps: the field type does not match
     * @tc.expected: return -E_INVALID_DATA.
     */
    syncData.tableName = g_tableName;
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), -E_INVALID_DATA);

    /**
     * @tc.steps: missing field GID_FIELD
     * @tc.expected: return -E_INVALID_ARGS.
     */
    syncData.insData.extend.clear();
    Timestamp now = TimeHelper::GetSysCurrentTime();
    bucket1.insert_or_assign(CloudDbConstant::GID_FIELD, std::string("1"));
    bucket2.insert_or_assign(CloudDbConstant::CREATE_FIELD, std::string("2"));
    syncData.insData.timestamp.push_back((int64_t)now / CloudDbConstant::TEN_THOUSAND);
    syncData.insData.timestamp.push_back((int64_t)now / CloudDbConstant::TEN_THOUSAND);
    syncData.insData.extend.push_back(bucket1);
    syncData.insData.extend.push_back(bucket2);
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), -E_INVALID_ARGS);

    syncData.insData.extend.pop_back();
    bucket2.insert_or_assign(CloudDbConstant::GID_FIELD, std::string("2"));
    syncData.insData.extend.push_back(bucket2);
    syncData.insData.hashKey.push_back({'3', '3'});
    syncData.insData.hashKey.push_back({'3', '3'});
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), E_OK);

    /**
     * @tc.steps: table name is not exists
     * @tc.expected: return -SQLITE_ERROR.
     */
    syncData.tableName = "noneTable";
    EXPECT_EQ(g_cloudStore->FillCloudLogAndAsset(OpType::INSERT, syncData, false, false), -E_NOT_FOUND);
}

/**
 * @tc.name: GetCloudData003
 * @tc.desc: ReleaseContinueToken required when GetCloudDataNext interrupt
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData003, TestSize.Level0)
{
    CreateLogTable();
    int64_t insCount = 1024;
    int64_t photoSize = 1024 * 8;
    InitLogData(insCount, insCount, insCount, insCount);
    CreateAndInitUserTable(2 * insCount, photoSize, g_localAsset); // 2 is insert,update type data

    SetDbSchema(g_tableSchema);
    ContinueToken token = nullptr;
    CloudSyncData cloudSyncData(g_tableName);
    EXPECT_EQ(g_storageProxy->ReleaseContinueToken(token), E_OK);
    EXPECT_EQ(g_storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    ASSERT_EQ(g_storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), -E_UNFINISHED);
    ASSERT_EQ(g_storageProxy->ReleaseContinueToken(token), E_OK);
    token = nullptr;
    EXPECT_EQ(g_storageProxy->GetCloudDataNext(token, cloudSyncData), -E_INVALID_ARGS);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);

    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    ASSERT_EQ(g_storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), -E_UNFINISHED);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);
    ASSERT_EQ(g_storageProxy->ReleaseContinueToken(token), E_OK);
}

/**
 * @tc.name: GetCloudData004
 * @tc.desc: Test get cloudData when asset or assets is NULL
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData004, TestSize.Level0)
{
    CreateLogTable();
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitLogData(insCount, insCount, insCount, insCount);
    CreateAndInitUserTable(3 * insCount, photoSize, g_localAsset); // 3 is insert,update and delete type data

    SetDbSchema(g_tableSchema);
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, "UPDATE " + g_tableName + " SET assert = NULL, asserts = NULL;"), E_OK);
    sqlite3_close(db);
    ContinueToken token = nullptr;
    CloudSyncData cloudSyncData(g_tableName);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), E_OK);
    EXPECT_NE(cloudSyncData.insData.record.size(), 0u);
    for (const auto &item: cloudSyncData.insData.record) {
        auto assert = item.find("assert");
        auto asserts = item.find("asserts");
        ASSERT_NE(assert, item.end());
        ASSERT_NE(asserts, item.end());
        EXPECT_EQ(assert->second.index(), static_cast<size_t>(TYPE_INDEX<Nil>));
        EXPECT_EQ(asserts->second.index(), static_cast<size_t>(TYPE_INDEX<Nil>));
    }
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: GetCloudData005
 * @tc.desc: Commit the transaction before getCloudData finished
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData005, TestSize.Level0)
{
    CreateLogTable();
    int64_t insCount = 1024;
    int64_t photoSize = 1024 * 8;
    InitLogData(insCount, insCount, insCount, insCount);
    CreateAndInitUserTable(2 * insCount, photoSize, g_localAsset); // 2 is insert,update type data

    SetDbSchema(g_tableSchema);
    ContinueToken token = nullptr;
    CloudSyncData cloudSyncData(g_tableName);
    EXPECT_EQ(g_storageProxy->ReleaseContinueToken(token), E_OK);
    EXPECT_EQ(g_storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    ASSERT_EQ(g_storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), -E_UNFINISHED);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);

    /**
     * @tc.steps: GetCloudDataNext after the transaction ends, token will released internally
     * @tc.expected: return -E_INVALID_DB.
     */
    ASSERT_EQ(g_cloudStore->GetCloudDataNext(token, cloudSyncData), -E_INVALID_DB);
}

/**
 * @tc.name: GetCloudData006
 * @tc.desc: Test get cloud data which contains invalid status asset
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Init data and set asset status to invalid num
     * @tc.expected: step1. return ok.
     */
    CreateLogTable();
    int64_t insCount = 1024;
    int64_t photoSize = 1024;
    InitLogData(insCount, insCount, insCount, insCount);
    CreateAndInitUserTable(2 * insCount, photoSize, g_localAsset); // 2 is insert,update type data
    Asset asset = g_localAsset;
    asset.status = static_cast<uint32_t>(AssetStatus::UPDATE) + 1;
    UpdateLocalAsset(g_tableName, asset, 2L); // 2 is rowid
    SetDbSchema(g_tableSchema);

    /**
     * @tc.steps:step2. Get cloud data
     * @tc.expected: step2. return -E_CLOUD_INVALID_ASSET.
     */
    ContinueToken token = nullptr;
    CloudSyncData cloudSyncData;
    EXPECT_EQ(g_storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    ASSERT_EQ(g_storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), -E_CLOUD_INVALID_ASSET);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);
}

/**
 * @tc.name: GetInfoByPrimaryKeyOrGid001
 * @tc.desc: Test the query of the GetInfoByPrimaryKeyOrGid interface to obtain assets.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetInfoByPrimaryKeyOrGid001, TestSize.Level0)
{
    int64_t insCount = 100;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    InitLogGid(insCount);

    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    for (int i = 1; i <= insCount; i++) {
        VBucket vBucket;
        vBucket[CloudDbConstant::GID_FIELD] = std::to_string(i);
        VBucket assetInfo;
        DataInfoWithLog dataInfo;
        ASSERT_EQ(g_storageProxy->GetInfoByPrimaryKeyOrGid(g_tableName, vBucket, dataInfo, assetInfo), E_OK);
        ASSERT_EQ(dataInfo.logInfo.cloudGid, std::to_string(i));
        auto entry1 = assetInfo.find("assert");
        auto entry2 = assetInfo.find("asserts");
        ASSERT_NE(entry1, assetInfo.end());
        ASSERT_NE(entry2, assetInfo.end());
        Asset asset = std::get<Asset>(entry1->second);
        EXPECT_EQ(asset.name, "Phone");
        Assets assets = std::get<Assets>(entry2->second);
        int id = 0;
        for (const auto &item: assets) {
            EXPECT_EQ(item.name, "Phone" + std::to_string(id++));
        }
    }
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: GetInfoByPrimaryKeyOrGid002
 * @tc.desc: Test the query of the GetInfoByPrimaryKeyOrGid interface to obtain assets.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetInfoByPrimaryKeyOrGid002, TestSize.Level0)
{
    int64_t insCount = 5;
    int64_t photoSize = 1;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    InitLogGid(insCount);
    InitLogicDelete(insCount);

    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    for (int i = 1; i <= insCount; i++) {
        VBucket vBucket;
        vBucket[CloudDbConstant::GID_FIELD] = std::to_string(i);
        VBucket assetInfo;
        DataInfoWithLog dataInfo;
        ASSERT_EQ(g_storageProxy->GetInfoByPrimaryKeyOrGid(g_tableName, vBucket, dataInfo, assetInfo), E_OK);
        ASSERT_EQ(dataInfo.logInfo.cloudGid, std::to_string(i));
        EXPECT_EQ(assetInfo.size(), 0u);
    }
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, PutCloudSyncData001, TestSize.Level0)
{
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    InitLogGid(insCount);

    DownloadData downloadData;
    std::vector<OpType> opTypes = { OpType::DELETE, OpType::INSERT, OpType::UPDATE,
                                    OpType::UPDATE, OpType::NOT_HANDLE };
    ConstructMultiDownloadData(insCount, downloadData, opTypes);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->PutCloudSyncData(g_tableName, downloadData), E_OK);
    ContinueToken token = nullptr;
    CloudSyncData cloudSyncData;
    ASSERT_EQ(g_storageProxy->GetCloudData(g_tableName, 0L, token, cloudSyncData), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudAsset001, TestSize.Level0)
{
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    InitLogGid(insCount);
    fillCloudAssetTest(insCount, AssetStatus::NORMAL, false);
    fillCloudAssetTest(insCount, AssetStatus::DOWNLOADING, false);
    fillCloudAssetTest(insCount, AssetStatus::ABNORMAL, false);
    fillCloudAssetTest(insCount, AssetStatus::NORMAL, true);
    fillCloudAssetTest(insCount, AssetStatus::DOWNLOADING, true);
    fillCloudAssetTest(insCount, AssetStatus::ABNORMAL, true);
}

HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudAsset002, TestSize.Level0)
{
    int64_t insCount = 10;
    int64_t photoSize = 10;
    Asset asset1 = g_localAsset;
    asset1.status = static_cast<uint32_t>(AssetStatus::DELETE | AssetStatus::UPLOADING);
    InitUserDataForAssetTest(insCount, photoSize, asset1);
    InitLogGid(insCount);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, "SELECT timestamp, hash_key FROM " + DBCommon::GetLogTableName(g_tableName)
        + " WHERE data_key = 1;", stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t timeStamp = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    Bytes hashKey;
    int errCode = SQLiteUtils::GetColumnBlobValue(stmt, 1, hashKey); // 1 is hash_key index
    EXPECT_EQ(errCode, E_OK);
    SQLiteUtils::ResetStatement(stmt, true, errCode);

    CloudSyncData syncData(g_tableName);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    ASSERT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE, syncData), E_OK);
    syncData.updData.rowid.push_back(1L);
    VBucket bucket1;
    Asset asset = g_localAsset;
    asset.size = "888";
    asset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
    asset.status = static_cast<uint32_t>(AssetStatus::DELETE | AssetStatus::UPLOADING);
    bucket1.insert_or_assign("assert", asset);
    int id = 0;
    Assets assets;
    asset.name = asset1.name + std::to_string(id++);
    assets.push_back(asset);
    asset.name = asset1.name + std::to_string(id++);
    assets.push_back(asset);
    bucket1.insert_or_assign("asserts", assets);
    syncData.updData.assets.push_back(bucket1);
    syncData.updData.extend.push_back(bucket1);
    syncData.updData.timestamp.push_back(timeStamp);
    syncData.updData.hashKey.push_back(hashKey);
    ASSERT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE, syncData), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);

    ASSERT_EQ(SQLiteUtils::GetStatement(db, "SELECT assert, asserts FROM " + g_tableName + " WHERE rowid = 1;",
        stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_NULL);
    ASSERT_EQ(sqlite3_column_type(stmt, 1), SQLITE_NULL);
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    sqlite3_close(db);
}

/**
 * @tc.name: FillCloudAsset003
 * @tc.desc: The twice fill have different assert columns
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudAsset003, TestSize.Level0)
{
    int64_t insCount = 2;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    InitLogGid(insCount);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, "SELECT timestamp FROM " + DBCommon::GetLogTableName(g_tableName) +
        " WHERE data_key in ('1', '2');", stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t timeStamp1 = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    int64_t timeStamp2 = static_cast<int64_t>(sqlite3_column_int64(stmt, 1));
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    sqlite3_close(db);

    CloudSyncData syncData(g_tableName);
    syncData.updData.rowid.push_back(1L);
    syncData.updData.rowid.push_back(2L);
    VBucket bucket1, bucket2;
    Asset asset = g_localAsset;
    asset.size = "888";
    asset.status = static_cast<uint32_t>(AssetStatus::UPDATE);
    Assets assets;
    assets.push_back(asset);
    assets.push_back(asset);
    bucket1.insert_or_assign("assert", asset);
    bucket2.insert_or_assign("assert", asset);
    bucket2.insert_or_assign("asserts", assets);
    syncData.updData.assets.push_back(bucket1);
    syncData.updData.assets.push_back(bucket2);
    syncData.updData.extend.push_back(bucket1);
    syncData.updData.extend.push_back(bucket2);
    syncData.updData.timestamp.push_back(timeStamp1);
    syncData.updData.timestamp.push_back(timeStamp2);
    syncData.updData.hashKey.push_back({ 1 });
    syncData.updData.hashKey.push_back({ 2 });
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    ASSERT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE, syncData), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: FillCloudAsset004
 * @tc.desc: Test fill asset for insert type
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudAsset004, TestSize.Level0)
{
    int64_t insCount = 2;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, "SELECT timestamp FROM " + DBCommon::GetLogTableName(g_tableName) +
        " WHERE data_key in ('1', '2');", stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    std::vector<int64_t> timeVector;
    timeVector.push_back(static_cast<int64_t>(sqlite3_column_int64(stmt, 0)));
    timeVector.push_back(static_cast<int64_t>(sqlite3_column_int64(stmt, 1)));
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    sqlite3_close(db);

    CloudSyncData syncData(g_tableName);
    for (int64_t i = 1; i <= insCount; ++i) {
        syncData.insData.rowid.push_back(i);
        VBucket bucket1;
        bucket1.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i));
        syncData.insData.extend.push_back(bucket1);

        VBucket bucket2;
        Asset asset = g_localAsset;
        asset.size = "888";
        Assets assets;
        assets.push_back(asset);
        assets.push_back(asset);
        bucket2.insert_or_assign("assert", asset);
        bucket2.insert_or_assign("asserts", assets);
        syncData.insData.assets.push_back(bucket2);
        syncData.insData.timestamp.push_back(timeVector[i - 1]);
        syncData.insData.hashKey.push_back({ 1 });
    }
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    ASSERT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::INSERT, syncData), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/*
 * @tc.name: CalPrimaryKeyHash001
 * @tc.desc: Test CalcPrimaryKeyHash interface when primary key is string
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, CalPrimaryKeyHash001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. local insert one data, primary key is string
     * @tc.expected: OK.
     */
    std::string tableName = "user2";
    const std::string CREATE_LOCAL_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS " + tableName + "(" \
        "name TEXT PRIMARY KEY, age INT);";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_LOCAL_TABLE_SQL), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName, CLOUD_COOPERATION), DBStatus::OK);
    std::string name = "Local0";
    std::map<std::string, Type> primaryKey = {{"name", name}};
    std::map<std::string, CollateType> collateType = {{"name", CollateType::COLLATE_NONE}};
    string sql = "INSERT OR REPLACE INTO user2(name, age) VALUES ('Local" + std::to_string(0) + "', '18');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    std::vector<uint8_t> result = RelationalStoreManager::CalcPrimaryKeyHash(primaryKey, collateType);
    EXPECT_NE(result.size(), 0u);
    std::string logTableName = RelationalStoreManager::GetDistributedLogTableName(tableName);
    /**
     * @tc.steps: step1. query timestamp use hashKey
     * @tc.expected: OK.
     */
    std::string querysql = "select timestamp/10000 from " + logTableName + " where hash_key=?";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, querysql, statement);
    EXPECT_EQ(errCode, E_OK);
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, result); // 1 means hashkey index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return;
    }
    errCode = SQLiteUtils::StepWithRetry(statement, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        Timestamp timestamp = static_cast<Timestamp>(sqlite3_column_int64(statement, 0));
        LOGD("get timestamp = %" PRIu64, timestamp);
        errCode = E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
    }
    EXPECT_EQ(errCode, E_OK);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    sqlite3_close(db);
}

/*
 * @tc.name: CalPrimaryKeyHash002
 * @tc.desc: Test CalcPrimaryKeyHash interface when primary key is int
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, CalPrimaryKeyHash002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. local insert one data, primary key is int
     * @tc.expected: OK.
     */
    std::string tableName = "user3";
    const std::string CREATE_LOCAL_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS " + tableName + "(" \
        "id INT PRIMARY KEY, name TEXT);";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_LOCAL_TABLE_SQL), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName, CLOUD_COOPERATION), DBStatus::OK);
    int64_t id = 1;
    std::map<std::string, Type> primaryKey = {{"id", id}};
    std::map<std::string, CollateType> collateType = {{"id", CollateType::COLLATE_NONE}};
    std::string sql = "INSERT OR REPLACE INTO " + tableName + " (id, name) VALUES ('" + '1' + "', 'Local" +
        std::to_string(0) + "');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    std::vector<uint8_t> result = RelationalStoreManager::CalcPrimaryKeyHash(primaryKey, collateType);
    EXPECT_NE(result.size(), 0u);
    std::string logTableName = RelationalStoreManager::GetDistributedLogTableName(tableName);
    /**
     * @tc.steps: step1. query timestamp use hashKey
     * @tc.expected: OK.
     */
    std::string querysql = "select timestamp/10000 from " + logTableName + " where hash_key=?";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, querysql, statement);
    EXPECT_EQ(errCode, E_OK);
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, result); // 1 means hashkey index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return;
    }
    errCode = SQLiteUtils::StepWithRetry(statement, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        Timestamp timestamp = static_cast<Timestamp>(sqlite3_column_int64(statement, 0));
        LOGD("get timestamp = %" PRIu64, timestamp);
        errCode = E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
    }
    EXPECT_EQ(errCode, E_OK);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    sqlite3_close(db);
}

/*
 * @tc.name: FillCloudVersion001
 * @tc.desc: Test FillCloudLogAndAsset interface when opType is UPDATE_VERSION
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudVersion001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. init data
     * @tc.expected: OK.
     */
    int64_t insCount = 100;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);

    /**
     * @tc.steps: step2. FillCloudLogAndAsset which isShared is false, but extend val is empty
     * @tc.expected: OK.
     */
    CloudSyncData syncData(g_tableName);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE_VERSION, syncData), E_OK);

    /**
     * @tc.steps: step3. FillCloudLogAndAsset which isShared is true, but extend val is empty
     * @tc.expected: OK.
     */
    syncData.isShared = true;
    EXPECT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE_VERSION, syncData), E_OK);

    /**
     * @tc.steps: step4. the extend size is not equal to the hashKey size
     * @tc.expected: -E_INVALID_ARGS.
     */
    VBucket extend1;
    extend1.insert_or_assign(CloudDbConstant::VERSION_FIELD, std::string("1"));
    syncData.delData.extend.push_back(extend1);
    syncData.updData.extend.push_back(extend1);
    syncData.insData.extend.push_back(extend1);
    EXPECT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE_VERSION, syncData), -E_INVALID_ARGS);

    /**
     * @tc.steps: step5. the extend size is equal to the hashKey size
     * @tc.expected: OK.
     */
    std::string hashKeyStr = "1";
    Bytes hashKey(hashKeyStr.begin(), hashKeyStr.end());
    syncData.delData.hashKey.push_back(hashKey);
    syncData.updData.hashKey.push_back(hashKey);
    syncData.insData.hashKey.push_back(hashKey);
    EXPECT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE_VERSION, syncData), E_OK);

    /**
     * @tc.steps: step6. insert not contain version no effect update
     * @tc.expected: OK.
     */
    syncData.insData.extend[0].erase(CloudDbConstant::VERSION_FIELD);
    EXPECT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::UPDATE_VERSION, syncData), E_OK);

    /**
     * @tc.steps: step7. insert not contain version effect insert
     * @tc.expected: OK.
     */
    EXPECT_EQ(g_storageProxy->FillCloudLogAndAsset(OpType::INSERT_VERSION, syncData), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/*
 * @tc.name: PutCloudSyncVersion001
 * @tc.desc: Test PutCloudSyncData interface that table is share
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, PutCloudSyncVersion001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. table type is shareTable, but downloadData is not contains version
     * @tc.expected: E_OK.
     */
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    InitLogGid(insCount);
    DownloadData downloadData;
    std::vector<OpType> opTypes = { OpType::INSERT, OpType::INSERT, OpType::DELETE, OpType::UPDATE,
        OpType::ONLY_UPDATE_GID, OpType::NOT_HANDLE };
    ConstructMultiDownloadData(insCount, downloadData, opTypes);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->PutCloudSyncData(g_tableName + CloudDbConstant::SHARED, downloadData), E_OK);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);

    /**
     * @tc.steps: step2. PutCloudSyncData and check table row num
     * @tc.expected: E_OK.
     */
    AddVersionToDownloadData(downloadData);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->PutCloudSyncData(g_tableName + CloudDbConstant::SHARED, downloadData), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    std::string querySql = "SELECT COUNT(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName +
        CloudDbConstant::SHARED + "_log";
    EXPECT_EQ(sqlite3_exec(db, querySql.c_str(),
        QueryCountCallback, reinterpret_cast<void *>(2L), nullptr), SQLITE_OK);
    sqlite3_close(db);
}

/*
 * @tc.name: PutCloudSyncVersion002
 * @tc.desc: Test PutCloudSyncData interface that download data is not contains version
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, PutCloudSyncVersion002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. table type is shareTable, but downloadData is not contains version
     * @tc.expected: E_OK.
     */
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    InitLogGid(insCount);
    DownloadData downloadData;
    std::vector<OpType> opTypes = { OpType::INSERT, OpType::INSERT, OpType::INSERT };
    ConstructMultiDownloadData(insCount, downloadData, opTypes);
    AddVersionToDownloadData(downloadData);
    AddCloudOwnerToDownloadData(downloadData);
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->PutCloudSyncData(g_tableName + CloudDbConstant::SHARED, downloadData), E_OK);

    /**
     * @tc.steps: step2. test opType of DELETE,UPDATE_TIMESTAMP,and CLEAR_GID without version field
     * @tc.expected: E_OK.
     */
    downloadData.opType = { OpType::DELETE, OpType::UPDATE_TIMESTAMP, OpType::CLEAR_GID };
    for (size_t i = 0; i < downloadData.data.size(); i++) {
        downloadData.data[i].erase(CloudDbConstant::VERSION_FIELD);
    }
    EXPECT_EQ(g_storageProxy->PutCloudSyncData(g_tableName + CloudDbConstant::SHARED, downloadData), E_OK);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, getAsset001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. both gid and hashKey are empty
     * @tc.expected: -E_INVALID_ARGS.
     */
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    UpdateLogGidAndHashKey(insCount);
    VBucket assets;
    std::string gid;
    Bytes hashKey;
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, -E_INVALID_ARGS);
    EXPECT_EQ(assets.size(), 0u);

    /**
     * @tc.steps: step2. gid is empty, but hashKey is 2
     * @tc.expected: E_OK.
     */
    Asset asset = g_localAsset;
    asset.status = static_cast<uint32_t>(AssetStatus::UPDATE);
    UpdateLocalAsset(g_tableName, asset, 2L); // 2 is rowid
    std::string pk = "2";
    hashKey.assign(pk.begin(), pk.end());
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, E_OK);
    CheckGetAsset(assets, static_cast<uint32_t>(AssetStatus::UPDATE));

    /**
     * @tc.steps: step3. gid is empty, but hashKey out of range
     * @tc.expected: -E_NOT_FOUND.
     */
    assets = {};
    pk = "11";
    hashKey.assign(pk.begin(), pk.end());
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, -E_NOT_FOUND);
    EXPECT_EQ(assets.size(), 0u);

    /**
     * @tc.steps: step4. hashKey is empty, but gid is 4
     * @tc.expected: E_OK.
     */
    gid = "2";
    pk = {};
    assets = {};
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, E_OK);
    CheckGetAsset(assets, static_cast<uint32_t>(AssetStatus::UPDATE));

    /**
     * @tc.steps: step5. hashKey is empty, but gid is out of range
     * @tc.expected: -E_NOT_FOUND.
     */
    gid = "11";
    assets = {};
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, -E_NOT_FOUND);
    EXPECT_EQ(assets.size(), 0u);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, getAsset002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. hashKey is 2, or gid is 4
     * @tc.expected: E_OK.
     */
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitUserDataForAssetTest(insCount, photoSize, g_localAsset);
    UpdateLogGidAndHashKey(insCount);
    VBucket assets;
    Bytes hashKey;
    Asset asset = g_localAsset;
    asset.status = static_cast<uint32_t>(AssetStatus::INSERT);
    UpdateLocalAsset(g_tableName, asset, 4L); // 4 is rowid
    std::string gid = "4";
    std::string pk = "2";
    hashKey.assign(pk.begin(), pk.end());
    EXPECT_EQ(g_storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, E_OK);
    CheckGetAsset(assets, static_cast<uint32_t>(AssetStatus::INSERT));

    /**
     * @tc.steps: step2. hashKey is 1, or gid is 11
     * @tc.expected: E_OK.
     */
    assets = {};
    gid = "11";
    pk = "1";
    hashKey.assign(pk.begin(), pk.end());
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, -E_CLOUD_GID_MISMATCH);
    CheckGetAsset(assets, static_cast<uint32_t>(AssetStatus::NORMAL));

    /**
     * @tc.steps: step3. hashKey is 12, or gid is 11
     * @tc.expected: -E_NOT_FOUND.
     */
    assets = {};
    pk = "12";
    hashKey.assign(pk.begin(), pk.end());
    EXPECT_EQ(g_storageProxy->GetAssetsByGidOrHashKey(g_tableName, gid, hashKey, assets).first, -E_NOT_FOUND);
    EXPECT_EQ(assets.size(), 0u);
    EXPECT_EQ(g_storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: GetCloudData007
 * @tc.desc: Test get cloud data which contains abnormal status asset
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Init data and set asset status to abnormal
     * @tc.expected: step1. return ok.
     */
    CreateLogTable();
    int64_t insCount = 10;
    int64_t photoSize = 1024 * 100;
    InitLogData(insCount * 3, insCount * 3, 1, insCount); // 3 is multiple
    CreateAndInitUserTable(insCount, photoSize, g_localAsset); // 2 is insert,update type data
    Asset asset = g_localAsset;
    asset.status = static_cast<uint32_t>(AssetStatus::ABNORMAL);
    insCount = 50;
    CreateAndInitUserTable(insCount, photoSize, asset);
    SetDbSchema(g_tableSchema);

    /**
     * @tc.steps:step2. Get all cloud data at once
     * @tc.expected: step2. return E_OK.
     */
    ContinueToken token = nullptr;
    CloudSyncData cloudSyncData;
    EXPECT_EQ(g_storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    ASSERT_EQ(g_storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), E_OK);
    EXPECT_EQ(g_storageProxy->Rollback(), E_OK);
}
}
#endif // RELATIONAL_STORE
