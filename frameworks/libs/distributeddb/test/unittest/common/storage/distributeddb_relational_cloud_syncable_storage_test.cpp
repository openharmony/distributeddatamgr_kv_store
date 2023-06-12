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

#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"
#include "distributeddb_data_generate_unit_test.h"
#include "relational_sync_able_storage.h"
#include "relational_store_instance.h"
#include "sqlite_relational_store.h"
#include "log_table_manager_factory.h"
#include "cloud_db_constant.h"
#include "runtime_config.h"
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
    .version = 1, .name = "Phone", .uri = "/local/sync", .modifyTime = "123456", .createTime = "",
    .size = "256", .hash = " "
};
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
IRelationalStore *g_store = nullptr;
ICloudSyncStorageInterface *g_cloudStore = nullptr;

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

void CreateAndInitUserTable(int64_t count, int64_t photoSize)
{
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);

    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, CREATE_LOCAL_TABLE_SQL), E_OK);
    std::string photo(photoSize, 'v');
    std::vector<uint8_t> assetBlob;
    std::vector<uint8_t> assetsBlob;
    Assets assets;
    assets.push_back(g_localAsset);
    assets.push_back(g_localAsset);
    int errCode;
    ASSERT_EQ(RuntimeContext::GetInstance()->AssetToBlob(g_localAsset, assetBlob), E_OK);
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
        if (i <= insCount) {
            flag = std::to_string(g_localFlag);
            cloudGid = "''";
        } else if (i > insCount && i <= insCount + updCount) {
            flag = std::to_string(g_localFlag);
            cloudGid = "'" + g_storeID + std::to_string(i) + "'";
        } else if (i > (insCount + updCount) && i <= (insCount + updCount + delCount)) {
            flag = std::to_string(g_localFlag | g_deleteFlag);
            cloudGid = "'" + g_storeID + std::to_string(i) + "'";
        } else {
            flag = std::to_string(g_localFlag | g_deleteFlag);
            cloudGid = "''";
        }
        string sql = "INSERT OR REPLACE INTO " + g_logTblName +
            " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cloud_gid)" +
            " VALUES ('" + std::to_string(i) + "', '', '', '" +  std::to_string(g_startTime + i) + "', '" +
            std::to_string(g_startTime + i) + "','" + flag + "','" + std::to_string(i) + "', " + cloudGid + ");";
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    }
    sqlite3_close(db);
}

void SetDbSchema(const TableSchema &tableSchema)
{
    DataBaseSchema dataBaseSchema;
    dataBaseSchema.tables.push_back(tableSchema);
    EXPECT_EQ(g_cloudStore->SetCloudDbSchema(dataBaseSchema), E_OK);
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
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, MetaDataTest001, TestSize.Level1)
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
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, MetaDataTest002, TestSize.Level1)
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
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest001, TestSize.Level1)
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
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest002, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);

    LocalWaterMark cloudTime = 666888;
    LocalWaterMark localTime;
    EXPECT_EQ(storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);

    /**
     * @tc.steps: allow get or put waterMark in read transaction
     * @tc.expected: Succeed, return OK.
     */
    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->PutLocalWaterMark(g_tableName, cloudTime), E_OK);
    EXPECT_EQ(storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
    EXPECT_EQ(cloudTime, localTime);
    EXPECT_EQ(storageProxy->Rollback(), E_OK);
    EXPECT_EQ(storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);

    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
    cloudTime = 999666;
    EXPECT_EQ(storageProxy->PutLocalWaterMark(g_tableName, cloudTime), E_OK);
    EXPECT_EQ(storageProxy->Commit(), E_OK);
    EXPECT_EQ(storageProxy->PutLocalWaterMark(g_tableName, cloudTime), E_OK);
    EXPECT_EQ(storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
    EXPECT_EQ(cloudTime, localTime);

    /**
     * @tc.steps: not allow get or put waterMark in write transaction
     * @tc.expected: return -E_BUSY.
     */
    EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    EXPECT_EQ(storageProxy->GetLocalWaterMark(g_tableName, localTime), -E_BUSY);
    EXPECT_EQ(storageProxy->PutLocalWaterMark(g_tableName, cloudTime), -E_BUSY);
    EXPECT_EQ(storageProxy->Rollback(), E_OK);
    EXPECT_EQ(storageProxy->GetLocalWaterMark(g_tableName, localTime), E_OK);
}

/**
 * @tc.name: TransactionTest003
 * @tc.desc: Repeatedly call transaction interface from StorageProxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest003, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);

    /**
     * @tc.steps: Repeated start transactions is not allowed
     * @tc.expected: return -E_TRANSACT_STATE.
     */
    EXPECT_EQ(storageProxy->StartTransaction(), -E_TRANSACT_STATE);

    /**
     * @tc.steps: Repeated commit is not allowed
     * @tc.expected: return -E_INVALID_DB.
     */
    EXPECT_EQ(storageProxy->Commit(), E_OK);
    EXPECT_EQ(storageProxy->Commit(), -E_INVALID_DB);

    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);

    /**
     * @tc.steps: Repeated Rollback is not allowed
     * @tc.expected: return -E_INVALID_DB.
     */
    EXPECT_EQ(storageProxy->Rollback(), E_OK);
    EXPECT_EQ(storageProxy->Rollback(), -E_INVALID_DB);
}

/**
 * @tc.name: TransactionTest004
 * @tc.desc: Call transaction after close storageProxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
  */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, TransactionTest004, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);

    /**
     * @tc.steps: transaction is not allowed after closing the proxy
     * @tc.expected: return -E_INVALID_DB.
     */
    EXPECT_EQ(storageProxy->Close(), E_OK);
    EXPECT_EQ(storageProxy->StartTransaction(), -E_INVALID_DB);
    EXPECT_EQ(storageProxy->Commit(), -E_INVALID_DB);
    EXPECT_EQ(storageProxy->Rollback(), -E_INVALID_DB);

    storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);

    /**
     * @tc.steps: close proxy is not allowed before the transaction has been commit or rollback
     * @tc.expected: return -E_BUSY.
     */
    EXPECT_EQ(storageProxy->Close(), -E_BUSY);
    EXPECT_EQ(storageProxy->Rollback(), E_OK);
    EXPECT_EQ(storageProxy->Close(), E_OK);
}

/**
 * @tc.name: GetUploadCount001
 * @tc.desc: Test getUploadCount by ICloudSyncStorageInterface
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetUploadCount001, TestSize.Level1)
{
    /**
     * @tc.steps: Table does not exist
     * @tc.expected: return -SQLITE_ERROR.
     */
    int64_t resCount = 0;
    EXPECT_EQ(g_cloudStore->GetUploadCount(g_tableName, g_startTime, resCount), -SQLITE_ERROR);

    CreateLogTable();
    int64_t insCount = 100;
    InitLogData(insCount, insCount, insCount, insCount);
    EXPECT_EQ(g_cloudStore->GetUploadCount(g_tableName, g_startTime, resCount), E_OK);
    EXPECT_EQ(resCount, insCount + insCount + insCount);

    /**
     * @tc.steps: There are no matching data anymore
     * @tc.expected: count is 0 and return E_OK.
     */
    Timestamp invalidTime = g_startTime + g_startTime;
    EXPECT_EQ(g_cloudStore->GetUploadCount(g_tableName, invalidTime, resCount), E_OK);
    EXPECT_EQ(resCount, 0);
}

/**
 * @tc.name: GetUploadCount002
 * @tc.desc: Test getUploadCount by storageProxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
  */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetUploadCount002, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 100;
    InitLogData(insCount, insCount, 0, insCount);
    int64_t resCount = 0;

    /**
     * @tc.steps: GetUploadCount must be called under transaction
     * @tc.expected: return -E_TRANSACT_STATE.
     */
    EXPECT_EQ(storageProxy->GetUploadCount(g_tableName, g_startTime, resCount), -E_TRANSACT_STATE);

    int timeOffset = 30;
    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->GetUploadCount(g_tableName, g_startTime + timeOffset, resCount), E_OK);
    EXPECT_EQ(resCount, insCount + insCount - timeOffset);
    EXPECT_EQ(storageProxy->Rollback(), E_OK);

    /**
     * @tc.steps: GetUploadCount also can be called under write transaction
     * @tc.expected: return E_OK.
     */
    EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    EXPECT_EQ(storageProxy->GetUploadCount(g_tableName, g_startTime + timeOffset, resCount), E_OK);
    EXPECT_EQ(resCount, insCount + insCount - timeOffset);
    EXPECT_EQ(storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: GetUploadCount003
 * @tc.desc: Test getUploadCount exclude condition of (deleteFlag and cloud_gid is '')
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetUploadCount003, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 100;
    InitLogData(0, 0, insCount, insCount);
    int64_t resCount = 0;

    /**
     * @tc.steps: GetUploadCount must be called under transaction
     * @tc.expected: return -E_TRANSACT_STATE.
     */
    EXPECT_EQ(storageProxy->GetUploadCount(g_tableName, g_startTime, resCount), -E_TRANSACT_STATE);

    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->GetUploadCount(g_tableName, g_startTime, resCount), E_OK);
    EXPECT_EQ(resCount, insCount);
    EXPECT_EQ(storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: FillCloudGid001
 * @tc.desc: FillCloudGid with invalid parm
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudGid001, TestSize.Level1)
{
    CreateLogTable();
    int64_t insCount = 100;
    InitLogData(insCount, 0, insCount, insCount);
    CloudSyncData syncData;

    /**
     * @tc.steps: rowid set is empty
     * @tc.expected: return -E_INVALID_ARGS.
     */
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), -E_INVALID_ARGS);
    syncData.insData.rowid.push_back(1);
    syncData.insData.rowid.push_back(2); // 2 is random id

    /**
     * @tc.steps: insData set is empty
     * @tc.expected: return -E_INVALID_ARGS.
     */
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), -E_INVALID_ARGS);
    VBucket bucket1;
    bucket1.insert_or_assign(g_tableName, g_tableName);
    bucket1.insert_or_assign(CloudDbConstant::GID_FIELD, 1L);
    syncData.insData.extend.push_back(bucket1);

    /**
     * @tc.steps: the size of rowid and insData is not equal
     * @tc.expected: return -E_INVALID_ARGS.
     */
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), -E_INVALID_ARGS);

    /**
     * @tc.steps: table name is empty
     * @tc.expected: return -SQLITE_ERROR.
     */
    VBucket bucket2;
    bucket2.insert_or_assign(CloudDbConstant::CREATE_FIELD, 2L); // 2L is random field
    syncData.insData.extend.push_back(bucket2);
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), -SQLITE_ERROR);

    /**
     * @tc.steps: the field type does not match
     * @tc.expected: return -E_INVALID_DATA.
     */
    syncData.tableName = g_tableName;
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), -E_INVALID_DATA);

    /**
     * @tc.steps: missing field GID_FIELD
     * @tc.expected: return -E_INVALID_ARGS.
     */
    syncData.insData.extend.clear();
    bucket1.insert_or_assign(CloudDbConstant::GID_FIELD, std::string("1"));
    bucket2.insert_or_assign(CloudDbConstant::CREATE_FIELD, std::string("2"));
    syncData.insData.extend.push_back(bucket1);
    syncData.insData.extend.push_back(bucket2);
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), -E_INVALID_ARGS);

    syncData.insData.extend.pop_back();
    bucket2.insert_or_assign(CloudDbConstant::GID_FIELD, std::string("2"));
    syncData.insData.extend.push_back(bucket2);
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), E_OK);

    /**
     * @tc.steps: table name is not exists
     * @tc.expected: return -SQLITE_ERROR.
     */
    syncData.tableName = "noneTable";
    EXPECT_EQ(g_cloudStore->FillCloudGid(syncData), -SQLITE_ERROR);
}

/**
 * @tc.name: FillCloudGid002
 * @tc.desc: Test whether the num of gid after fill are correct
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudGid002, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 100;
    int64_t updCount = 50;
    int64_t delCount = 50;
    InitLogData(insCount, updCount, delCount, insCount);

    CloudSyncData syncData(g_tableName);
    for (int64_t i = 1; i <= 3 * insCount; ++i) { // 3 is insert,update and delete type data
        syncData.insData.rowid.push_back(i);
        VBucket bucket1;
        bucket1.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(g_startTime + i));
        syncData.insData.extend.push_back(bucket1);
    }
    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->FillCloudGid(syncData), E_OK);
    EXPECT_EQ(storageProxy->Commit(), E_OK);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    std::string querySql = "SELECT COUNT(*) FROM " + g_logTblName + " WHERE cloud_gid in (";
    for (int64_t i = 1; i <= (insCount + updCount + delCount); ++i) {
        querySql += "'" + std::to_string(g_startTime + i) + "',";
    }
    querySql.pop_back();
    querySql += ");";
    EXPECT_EQ(sqlite3_exec(db, querySql.c_str(),
        QueryCountCallback, reinterpret_cast<void *>(insCount + updCount + delCount), nullptr), SQLITE_OK);
    sqlite3_close(db);
}

/**
 * @tc.name: FillCloudGid003
 * @tc.desc: Test FillCloudGid after in write transaction
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, FillCloudGid003, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 10;
    InitLogData(insCount, insCount, insCount, insCount);
    CloudSyncData syncData(g_tableName);
    for (int64_t i = 1; i <= (insCount + insCount + insCount); ++i) {
        syncData.insData.rowid.push_back(i);
        VBucket bucket1;
        bucket1.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(g_startTime + i));
        syncData.insData.extend.push_back(bucket1);
    }

    /**
     * @tc.steps: FillCloudGid is not allowed after starting write transaction
     * @tc.expected: return -E_BUSY.
     */
    EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    EXPECT_EQ(storageProxy->FillCloudGid(syncData), -E_BUSY);
    EXPECT_EQ(storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: GetCloudData001
 * @tc.desc: Test GetCloudData,whether the result count and type of data are correct
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData001, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 100;
    int64_t updCount = 50;
    int64_t delCount = 25;
    int64_t photoSize = 10;
    InitLogData(insCount, updCount, delCount, insCount);
    CreateAndInitUserTable(3 * insCount, photoSize); // 3 is insert,update and delete type data

    ContinueToken token;
    CloudSyncData cloudSyncData;
    TableSchema tableSchema;
    tableSchema.name = g_tableName;
    tableSchema.fields = g_cloudFiled;
    SetDbSchema(tableSchema);

    /**
     * @tc.steps: There is currently no handle under the transaction
     * @tc.expected: return -E_INVALID_DB.
     */
    int timeOffset = 10;
    EXPECT_EQ(g_cloudStore->GetCloudData(tableSchema, g_startTime + timeOffset, token, cloudSyncData), -E_INVALID_DB);

    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->GetCloudData(g_tableName, g_startTime + timeOffset, token, cloudSyncData), E_OK);
    EXPECT_EQ(storageProxy->Commit(), E_OK);
    EXPECT_EQ(cloudSyncData.insData.record.size() + cloudSyncData.updData.record.size() +
        cloudSyncData.delData.record.size(), static_cast<uint64_t>(insCount + updCount + delCount - timeOffset));
    ASSERT_EQ(cloudSyncData.insData.record.size(), static_cast<uint64_t>(insCount - timeOffset));
    ASSERT_EQ(cloudSyncData.updData.record.size(), static_cast<uint64_t>(updCount));
    ASSERT_EQ(cloudSyncData.delData.record.size(), static_cast<uint64_t>(delCount));

    EXPECT_EQ(cloudSyncData.insData.record[0].find(CloudDbConstant::GID_FIELD), cloudSyncData.insData.record[0].end());
    EXPECT_NE(cloudSyncData.updData.record[0].find(CloudDbConstant::GID_FIELD), cloudSyncData.insData.record[0].end());
    EXPECT_NE(cloudSyncData.delData.record[0].find(CloudDbConstant::GID_FIELD), cloudSyncData.insData.record[0].end());


    /**
     * @tc.steps: GetCloudData also can be called under write transaction
     * @tc.expected: return E_OK.
     */
    EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    EXPECT_EQ(storageProxy->GetCloudData(g_tableName, g_startTime + timeOffset, token, cloudSyncData), E_OK);
    EXPECT_EQ(storageProxy->Commit(), E_OK);
}

/**
 * @tc.name: GetCloudData002
 * @tc.desc: The maximum return data size of GetCloudData is less than 8M
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData002, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 1024;
    int64_t photoSize = 1024 * 2;
    InitLogData(insCount, insCount, insCount, insCount);
    CreateAndInitUserTable(3 * insCount, photoSize); // 3 is insert,update and delete type data

    TableSchema tableSchema;
    tableSchema.name = g_tableName;
    tableSchema.fields = g_cloudFiled;
    SetDbSchema(tableSchema);

    /**
     * @tc.steps: GetCloudData has not finished querying yet.
     * @tc.expected: return -E_UNFINISHED.
     */
    ContinueToken token;
    CloudSyncData cloudSyncData1;
    int timeOffset = 10;
    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->GetCloudData(g_tableName, g_startTime + timeOffset, token, cloudSyncData1), -E_UNFINISHED);
    EXPECT_LT(cloudSyncData1.insData.record.size() + cloudSyncData1.updData.record.size() +
        cloudSyncData1.delData.record.size(), static_cast<uint64_t>(insCount));
    EXPECT_EQ(cloudSyncData1.delData.record.size(), 0u);

    CloudSyncData cloudSyncData2;
    EXPECT_EQ(storageProxy->GetCloudDataNext(token, cloudSyncData2), -E_UNFINISHED);
    EXPECT_LT(cloudSyncData2.insData.record.size() + cloudSyncData2.updData.record.size() +
        cloudSyncData2.delData.record.size(), static_cast<uint64_t>(insCount));

    CloudSyncData cloudSyncData3;
    EXPECT_EQ(storageProxy->GetCloudDataNext(token, cloudSyncData3), E_OK);
    EXPECT_GT(cloudSyncData3.insData.record.size() + cloudSyncData3.updData.record.size() +
        cloudSyncData3.delData.record.size(), static_cast<uint64_t>(insCount));
    EXPECT_EQ(cloudSyncData3.insData.record.size(), 0u);

    /**
     * @tc.steps: Finished querying, the token has been release.
     * @tc.expected: return -E_INVALID_ARGS.
     */
    EXPECT_EQ(storageProxy->GetCloudDataNext(token, cloudSyncData3), -E_INVALID_ARGS);
    EXPECT_EQ(storageProxy->Rollback(), E_OK);
}

/**
 * @tc.name: GetCloudData003
 * @tc.desc: ReleaseContinueToken required when GetCloudDataNext interrupt
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData003, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 1024;
    int64_t photoSize = 1024 * 8;
    InitLogData(insCount, insCount, insCount, insCount);
    CreateAndInitUserTable(2 * insCount, photoSize); // 2 is insert,update type data

    TableSchema tableSchema;
    tableSchema.name = g_tableName;
    tableSchema.fields = g_cloudFiled;
    SetDbSchema(tableSchema);

    ContinueToken token;
    CloudSyncData cloudSyncData;
    EXPECT_EQ(storageProxy->ReleaseContinueToken(token), E_OK);
    EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);
    ASSERT_EQ(storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), -E_UNFINISHED);
    ASSERT_EQ(storageProxy->ReleaseContinueToken(token), E_OK);
    token = nullptr;
    EXPECT_EQ(storageProxy->GetCloudDataNext(token, cloudSyncData), -E_INVALID_ARGS);
    EXPECT_EQ(storageProxy->Rollback(), E_OK);

    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    ASSERT_EQ(storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), -E_UNFINISHED);
    EXPECT_EQ(storageProxy->Rollback(), E_OK);
    ASSERT_EQ(storageProxy->ReleaseContinueToken(token), E_OK);
}

/**
 * @tc.name: GetCloudData004
 * @tc.desc: Test get cloudData when asset or assets is NULL
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalCloudSyncableStorageTest, GetCloudData004, TestSize.Level1)
{
    std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
    ASSERT_NE(storageProxy, nullptr);
    CreateLogTable();
    int64_t insCount = 10;
    int64_t photoSize = 10;
    InitLogData(insCount, insCount, insCount, insCount);
    CreateAndInitUserTable(3 * insCount, photoSize); // 3 is insert,update and delete type data

    TableSchema tableSchema;
    tableSchema.name = g_tableName;
    tableSchema.fields = g_cloudFiled;
    SetDbSchema(tableSchema);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, "UPDATE " + g_tableName + " SET assert = NULL, asserts = NULL;"), E_OK);
    sqlite3_close(db);
    ContinueToken token;
    CloudSyncData cloudSyncData;
    EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
    EXPECT_EQ(storageProxy->GetCloudData(g_tableName, g_startTime, token, cloudSyncData), E_OK);
    EXPECT_NE(cloudSyncData.insData.record.size(), 0u);
    for (const auto &item: cloudSyncData.insData.record) {
        auto assert = item.find("assert");
        auto asserts = item.find("asserts");
        ASSERT_NE(assert, item.end());
        ASSERT_NE(asserts, item.end());
        EXPECT_EQ(assert->second.index(), static_cast<size_t>(TYPE_INDEX<Asset>));
        EXPECT_EQ(asserts->second.index(), static_cast<size_t>(TYPE_INDEX<Assets>));
    }
    EXPECT_EQ(storageProxy->Commit(), E_OK);
}
}
#endif // RELATIONAL_STORE