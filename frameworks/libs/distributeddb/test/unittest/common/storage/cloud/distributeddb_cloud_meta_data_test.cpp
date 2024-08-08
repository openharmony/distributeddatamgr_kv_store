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
#include "cloud/cloud_meta_data.h"

#include <gtest/gtest.h>

#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"
#include "distributeddb_data_generate_unit_test.h"
#include "relational_sync_able_storage.h"
#include "relational_store_instance.h"
#include "sqlite_relational_store.h"
#include "log_table_manager_factory.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr auto TABLE_NAME_1 = "tableName1";
    constexpr auto TABLE_NAME_2 = "tableName2";
    const string STORE_ID = "Relational_Store_ID";
    const string TABLE_NAME = "cloudData";
    string TEST_DIR;
    string g_storePath;
    string g_dbDir;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;
    IRelationalStore *g_store = nullptr;
    std::shared_ptr<StorageProxy> g_storageProxy = nullptr;

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
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql.c_str()), E_OK);
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

    void SetAndGetWaterMark(TableName tableName, Timestamp mark)
    {
        Timestamp retMark;
        EXPECT_EQ(g_storageProxy->PutLocalWaterMark(tableName, mark), E_OK);
        EXPECT_EQ(g_storageProxy->GetLocalWaterMark(tableName, retMark), E_OK);
        EXPECT_EQ(retMark, mark);
    }

    void SetAndGetWaterMark(TableName tableName, std::string mark)
    {
        std::string retMark;
        EXPECT_EQ(g_storageProxy->SetCloudWaterMark(tableName, mark), E_OK);
        EXPECT_EQ(g_storageProxy->GetCloudWaterMark(tableName, retMark), E_OK);
        EXPECT_EQ(retMark, mark);
    }

    class DistributedDBCloudMetaDataTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    };

    void DistributedDBCloudMetaDataTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(TEST_DIR);
        LOGD("test dir is %s", TEST_DIR.c_str());
        g_dbDir = TEST_DIR + "/";
        g_storePath =  g_dbDir + STORE_ID + ".db";
        DistributedDBToolsUnitTest::RemoveTestDbFiles(TEST_DIR);
    }

    void DistributedDBCloudMetaDataTest::TearDownTestCase(void)
    {
    }

    void DistributedDBCloudMetaDataTest::SetUp(void)
    {
        DistributedDBToolsUnitTest::PrintTestCaseInfo();
        LOGD("Test dir is %s", TEST_DIR.c_str());
        CreateDB();
        ASSERT_EQ(g_mgr.OpenStore(g_storePath, STORE_ID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
        ASSERT_NE(g_delegate, nullptr);
        g_storageProxy = GetStorageProxy((ICloudSyncStorageInterface *) GetRelationalStore());
    }

    void DistributedDBCloudMetaDataTest::TearDown(void)
    {
        RefObject::DecObjRef(g_store);
        if (g_delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
            g_delegate = nullptr;
            g_storageProxy = nullptr;
        }
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(TEST_DIR) != 0) {
            LOGE("rm test db files error.");
        }
    }

    /**
     * @tc.name: CloudMetaDataTest001
     * @tc.desc: Set and get local water mark with various value
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, CloudMetaDataTest001, TestSize.Level0)
    {
        SetAndGetWaterMark(TABLE_NAME_1, 123); // 123 is a random normal value, not magic number
        SetAndGetWaterMark(TABLE_NAME_1, 0); // 0 is used for test, not magic number
        SetAndGetWaterMark(TABLE_NAME_1, -1); // -1 is used for test, not magic number
        SetAndGetWaterMark(TABLE_NAME_1, UINT64_MAX);
        SetAndGetWaterMark(TABLE_NAME_1, UINT64_MAX + 1);
    }

    /**
     * @tc.name: CloudMetaDataTest002
     * @tc.desc: Set and get cloud water mark with various value
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, CloudMetaDataTest002, TestSize.Level0)
    {
        SetAndGetWaterMark(TABLE_NAME_1, "");
        SetAndGetWaterMark(TABLE_NAME_1, "123");
        SetAndGetWaterMark(TABLE_NAME_1, "1234567891012112345678910121");
        SetAndGetWaterMark(TABLE_NAME_1, "ABCDEFGABCDEFGABCDEFGABCDEFG");
        SetAndGetWaterMark(TABLE_NAME_1, "abcdefgabcdefgabcdefgabcdefg");
        SetAndGetWaterMark(TABLE_NAME_1, "ABCDEFGABCDEFGabcdefgabcdefg");
        SetAndGetWaterMark(TABLE_NAME_1, "123456_GABEFGab@中文字符cdefg");
    }

    /**
     * @tc.name: CloudMetaDataTest003
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, CloudMetaDataTest003, TestSize.Level0)
    {
        std::string retMark;
        EXPECT_EQ(g_storageProxy->GetCloudWaterMark(TABLE_NAME_2, retMark), E_OK);
        EXPECT_EQ(retMark, "");

        Timestamp retLocalMark;
        EXPECT_EQ(g_storageProxy->GetLocalWaterMark(TABLE_NAME_2, retLocalMark), E_OK);
        EXPECT_EQ(retLocalMark, 0u);
    }
        
    /**
    * @tc.name: AbnormalStorageProxyTest001
    * @tc.desc: Check StorageProxy interfaces when para is invalid.
    * @tc.type: FUNC
    * @tc.require: DTS2024073106613
    * @tc.author: suyue
    */
    HWTEST_F(DistributedDBCloudMetaDataTest, AbnormalStorageProxyTest001, TestSize.Level0)
    {
        /**
         * @tc.steps: step1. Call interfaces when class para and db is null.
         * @tc.expected: step1. return -E_INVALID_DB.
         */
        StorageProxy proxyObj(nullptr);
        Timestamp retLocalMark;
        EXPECT_EQ(proxyObj.GetLocalWaterMark(TABLE_NAME_1, retLocalMark), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.GetLocalWaterMarkByMode(TABLE_NAME_1, CloudWaterType::INSERT, retLocalMark), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.PutLocalWaterMark(TABLE_NAME_1, retLocalMark), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.PutWaterMarkByMode(TABLE_NAME_1, CloudWaterType::INSERT, retLocalMark), -E_INVALID_DB);
        proxyObj.ReleaseUploadRecord(TABLE_NAME_1, CloudWaterType::INSERT, retLocalMark);
        int64_t count = 0;
        EXPECT_EQ(proxyObj.GetUploadCount(TABLE_NAME_1, retLocalMark, true, count), -E_INVALID_DB);
        const QuerySyncObject query;
        EXPECT_EQ(proxyObj.GetUploadCount(query, retLocalMark, true, true, count), -E_INVALID_DB);
        ContinueToken continueStmtToken;
        CloudSyncData cloudDataResult;
        EXPECT_EQ(proxyObj.GetCloudData(query, retLocalMark, continueStmtToken, cloudDataResult), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.GetCloudDataNext(continueStmtToken, cloudDataResult), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.GetUploadCount(query, true, true, true, count), -E_INVALID_DB);
        std::vector<std::string> strVec = {};
        EXPECT_EQ(proxyObj.GetCloudGid(query, true, true, strVec), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.CheckSchema(strVec), -E_INVALID_DB);
        std::vector<Field> assetFields;
        EXPECT_EQ(proxyObj.GetPrimaryColNamesWithAssetsFields(TABLE_NAME_1, strVec, assetFields), -E_INVALID_DB);
        std::vector<std::string> colNames = {"testColName"};
        EXPECT_EQ(proxyObj.GetPrimaryColNamesWithAssetsFields(TABLE_NAME_1, colNames, assetFields), -E_INVALID_ARGS);
        VBucket assetInfo;
        DataInfoWithLog log;
        EXPECT_EQ(proxyObj.GetInfoByPrimaryKeyOrGid(TABLE_NAME_1, assetInfo, log, assetInfo),
            -E_INVALID_DB);
        EXPECT_EQ(proxyObj.FillCloudAssetForDownload(TABLE_NAME_1, assetInfo, true), -E_INVALID_DB);
        const Bytes hashKey;
        std::pair<int, uint32_t> res = proxyObj.GetAssetsByGidOrHashKey(TABLE_NAME_1, "", hashKey, assetInfo);
        EXPECT_EQ(res.first, -E_INVALID_DB);
        std::string str = proxyObj.GetIdentify();
        const std::string emptyStr = "";
        EXPECT_TRUE(str.compare(0, str.length(), emptyStr) == 0);
        std::pair<int, CloudSyncData> ver = proxyObj.GetLocalCloudVersion();
        EXPECT_EQ(ver.first, -E_INTERNAL_ERROR);
        CloudSyncConfig cfg = proxyObj.GetCloudSyncConfig();
        EXPECT_EQ(cfg.maxRetryConflictTimes, -1);
        std::string cloudMark = "test";
        EXPECT_EQ(proxyObj.GetCloudWaterMark(TABLE_NAME_1, cloudMark), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.SetCloudWaterMark(TABLE_NAME_1, cloudMark), -E_INVALID_DB);
        const RelationalSchemaObject localSchema;
        std::vector<Asset> assets = {};
        EXPECT_EQ(proxyObj.CleanCloudData(ClearMode::DEFAULT, strVec, localSchema, assets), -E_INVALID_DB);
    }

    /**
    * @tc.name: AbnormalStorageProxyTest002
    * @tc.desc: Check StorageProxy interfaces when para is invalid.
    * @tc.type: FUNC
    * @tc.require: DTS2024073106613
    * @tc.author: suyue
    */
    HWTEST_F(DistributedDBCloudMetaDataTest, AbnormalStorageProxyTest002, TestSize.Level0)
    {
        /**
         * @tc.steps: step1. Call interfaces when class para and db is null.
         * @tc.expected: step1. return -E_INVALID_DB.
         */
        StorageProxy proxyObj(nullptr);
        DownloadData downloadData;
        EXPECT_EQ(proxyObj.PutCloudSyncData(TABLE_NAME_1, downloadData), -E_INVALID_DB);
        const std::set<std::string> gidFilters = {};
        EXPECT_EQ(proxyObj.MarkFlagAsConsistent(TABLE_NAME_1, downloadData, gidFilters), -E_INVALID_DB);
        bool isSharedTable = true;
        EXPECT_EQ(proxyObj.IsSharedTable(TABLE_NAME_1, isSharedTable), -E_INVALID_DB);
        CloudSyncData data;
        proxyObj.FillCloudGidIfSuccess(OpType::INSERT, data);
        const CloudTaskConfig config;
        proxyObj.SetCloudTaskConfig(config);
        std::vector<QuerySyncObject> syncQuery;
        EXPECT_EQ(proxyObj.GetCompensatedSyncQuery(syncQuery), -E_INVALID_DB);
        proxyObj.OnSyncFinish();
        proxyObj.OnUploadStart();
        std::shared_ptr<DataBaseSchema> cloudSchema;
        EXPECT_EQ(proxyObj.GetCloudDbSchema(cloudSchema), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.CheckSchema(TABLE_NAME_1), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.NotifyChangedData(TABLE_NAME_1, {}), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.SetLogTriggerStatus(true), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.CleanWaterMark(TABLE_NAME_1), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.CleanWaterMarkInMemory(TABLE_NAME_1), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.CreateTempSyncTrigger(TABLE_NAME_1), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.IsTableExistReference(TABLE_NAME_1), false);
        EXPECT_EQ(proxyObj.ClearAllTempSyncTrigger(), -E_INVALID_DB);
        EXPECT_EQ(proxyObj.SetIAssetLoader(nullptr), -E_INVALID_DB);
    }

    /**
     * @tc.name: AbnormalSyncAbleStorageTest001
     * @tc.desc: Check RelationalSyncAbleStorage interfaces.
     * @tc.type: FUNC
     * @tc.require: DTS2024073106613
     * @tc.author: suyue
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, AbnormalSyncAbleStorageTest001, TestSize.Level0)
    {
        /**
         * @tc.steps: step1. Call interfaces when RelationalSyncAbleStorage class para is null.
         * @tc.expected: step1. return -E_INVALID_DB.
         */
        RelationalSyncAbleStorage obj(nullptr);
        std::vector<Key> keys;
        EXPECT_EQ(obj.DeleteMetaData(keys), -E_INVALID_DB);
        EXPECT_EQ(obj.GetAllMetaKeys(keys), -E_INVALID_DB);
        Key key;
        Value value;
        EXPECT_EQ(obj.GetMetaData(key, value), -E_INVALID_DB);
        EXPECT_EQ(obj.PutMetaData(key, value), -E_INVALID_DB);
        EXPECT_EQ(obj.PutMetaData(key, value, true), -E_INVALID_DB);
        EXPECT_EQ(obj.DeleteMetaDataByPrefixKey(key), -E_INVALID_DB);
        Timestamp timestamp;
        obj.GetMaxTimestamp(timestamp);
        EXPECT_EQ(obj.GetMaxTimestamp(TABLE_NAME_1, timestamp), -E_INVALID_DB);
        QuerySyncObject querySync;
        int64_t count;
        EXPECT_EQ(obj.GetUploadCount(querySync, timestamp, true, true, count), -E_INVALID_DB);
        TableSchema tableSchema;
        ContinueToken token;
        CloudSyncData data;
        EXPECT_EQ(obj.GetCloudData(tableSchema, querySync, timestamp, token, data), -E_INVALID_DB);
        std::vector<Timestamp> timestampVec;
        EXPECT_EQ(obj.GetAllUploadCount(querySync, timestampVec, true, true, count), -E_INVALID_DB);
        std::vector<std::string> cloudGid;
        EXPECT_EQ(obj.GetCloudGid(tableSchema, querySync, true, true, cloudGid), -E_INVALID_DB);
        EXPECT_EQ(obj.CheckQueryValid(querySync), -E_INVALID_DB);
        std::vector<QuerySyncObject> resVec;
        EXPECT_EQ(obj.LocalDataChanged(0, resVec), -E_NOT_SUPPORT);
        VBucket assetInfo;
        DataInfoWithLog log;
        EXPECT_EQ(obj.GetInfoByPrimaryKeyOrGid(TABLE_NAME_1, assetInfo, log, assetInfo), -E_INVALID_DB);
        EXPECT_EQ(obj.FillCloudAssetForDownload(TABLE_NAME_1, assetInfo, true), -E_INVALID_DB);
        DownloadData downloadData;
        EXPECT_EQ(obj.PutCloudSyncData(TABLE_NAME_1, downloadData), -E_INVALID_DB);
        std::vector<std::string> strVec = {};
        std::vector<Asset> assets = {};
        const RelationalSchemaObject localSchema;
        EXPECT_EQ(obj.CleanCloudData(ClearMode::DEFAULT, strVec, localSchema, assets), -E_INVALID_DB);
        EXPECT_EQ(obj.SetLogTriggerStatus(true), -E_INVALID_DB);
    }

    /**
     * @tc.name: AbnormalSyncAbleStorageTest002
     * @tc.desc: Check RelationalSyncAbleStorage interfaces.
     * @tc.type: FUNC
     * @tc.require: DTS2024073106613
     * @tc.author: suyue
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, AbnormalSyncAbleStorageTest002, TestSize.Level0)
    {
        /**
         * @tc.steps: step1. Call interfaces when RelationalSyncAbleStorage class para is null.
         * @tc.expected: step1. return -E_INVALID_DB.
         */
        RelationalSyncAbleStorage obj(nullptr);
        CloudSyncData cloudDataResult;
        EXPECT_EQ(obj.FillCloudLogAndAsset(OpType::INSERT, cloudDataResult, true, true), -E_INVALID_DB);
        std::string str = obj.GetIdentify();
        const std::string emptyStr = "";
        EXPECT_TRUE(str.compare(0, str.length(), emptyStr) == 0);
        EXPECT_EQ(obj.CreateTempSyncTrigger(""), -E_INVALID_DB);
        ChangeProperties properties;
        EXPECT_EQ(obj.GetAndResetServerObserverData(TABLE_NAME_1, properties), -E_INVALID_DB);
        std::vector<VBucket> records;
        EXPECT_EQ(obj.UpsertData(RecordStatus::WAIT_COMPENSATED_SYNC, TABLE_NAME_1, records), -E_INVALID_DB);
        LogInfo logInfo;
        EXPECT_EQ(obj.UpdateRecordFlag(TABLE_NAME_1, true, logInfo), -E_INVALID_DB);
        CloudSyncConfig config;
        obj.SetCloudSyncConfig(config);
        EXPECT_EQ(obj.SaveRemoteDeviceSchema(TABLE_NAME_1, "", 0), -E_INVALID_ARGS);
        EXPECT_EQ(obj.RemoveDeviceData(TABLE_NAME_1, true), -E_NOT_SUPPORT);
        EXPECT_EQ(obj.StartTransaction(TransactType::DEFERRED), -E_INVALID_DB);
        EXPECT_EQ(obj.ClearAllTempSyncTrigger(), -E_INVALID_DB);
        EXPECT_EQ(obj.SetIAssetLoader(nullptr), -E_INVALID_DB);
    }
}
