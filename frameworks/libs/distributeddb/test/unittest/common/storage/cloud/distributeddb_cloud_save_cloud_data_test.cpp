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

#include <gtest/gtest.h>

#include "cloud/cloud_storage_utils.h"
#include "cloud_store_types.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "icloud_sync_storage_interface.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_sync_able_storage.h"
#include "sqlite_relational_store.h"
#include "storage_proxy.h"
#include "virtual_cloud_data_translate.h"
#include "runtime_config.h"


using namespace testing::ext;
using namespace  DistributedDB;
using namespace  DistributedDBUnitTest;

namespace {
    constexpr const char *DB_SUFFIX = ".db";
    constexpr const char *STORE_ID = "Relational_Store_ID";
    std::string g_dbDir;
    std::string g_testDir;
    std::string g_tableName = "sync_data";
    std::string g_assetTableName = "asset_sync_data";
    std::string g_storePath;
    std::string g_gid = "abcd";
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    IRelationalStore *g_store = nullptr;
    RelationalStoreDelegate *g_delegate = nullptr;
    ICloudSyncStorageInterface *g_cloudStore = nullptr;
    constexpr const int64_t BASE_MODIFY_TIME = 12345678L;
    constexpr const int64_t BASE_CREATE_TIME = 12345679L;

    enum class PrimaryKeyType {
        NO_PRIMARY_KEY,
        SINGLE_PRIMARY_KEY,
        COMPOSITE_PRIMARY_KEY
    };

    enum class GidType {
        GID_EMPTY,
        GID_MATCH,
        GID_MISMATCH,
        GID_INVALID
    };

    class DistributedDBCloudSaveCloudDataTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp() override;
        void TearDown() override;
    };

    void CreatDB()
    {
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
        EXPECT_NE(db, nullptr);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), E_OK);
    }

    void SetCloudSchema(PrimaryKeyType pkType, bool nullable)
    {
        TableSchema tableSchema;
        bool isIdPk = pkType == PrimaryKeyType::SINGLE_PRIMARY_KEY || pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY;
        Field field1 = { "id", TYPE_INDEX<int64_t>, isIdPk, !isIdPk };
        Field field2 = { "name", TYPE_INDEX<std::string>, pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY, true };
        Field field3 = { "age", TYPE_INDEX<double>, pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY, true };
        Field field4 = { "sex", TYPE_INDEX<bool>, false, nullable };
        Field field5 = { "image", TYPE_INDEX<Bytes>, false, true };
        tableSchema = { g_tableName, { field1, field2, field3, field4, field5} };

        DataBaseSchema dbSchema;
        dbSchema.tables = { tableSchema };
        g_cloudStore->SetCloudDbSchema(dbSchema);
    }

    void PrepareDataBase(const std::string &tableName, PrimaryKeyType pkType, bool nullable = true)
    {
        /**
         * @tc.steps:step1. create table.
         * @tc.expected: step1. return ok.
         */
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
        EXPECT_NE(db, nullptr);
        std::string sql;
        if (pkType == PrimaryKeyType::SINGLE_PRIMARY_KEY) {
            sql = "create table " + tableName + "(id int primary key, name TEXT, age REAL, sex INTEGER, image BLOB);";
        } else if (pkType == PrimaryKeyType::NO_PRIMARY_KEY) {
            sql = "create table " + tableName + "(id int, name TEXT, age REAL, sex INTEGER, image BLOB);";
        } else {
            sql = "create table " + tableName + "(id int, name TEXT, age REAL, sex INTEGER, image BLOB," \
                " PRIMARY KEY(id, name, age))";
        }
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

        /**
         * @tc.steps:step2. create distributed table with CLOUD_COOPERATION mode.
         * @tc.expected: step2. return ok.
         */
        EXPECT_EQ(g_delegate->CreateDistributedTable(tableName, DistributedDB::CLOUD_COOPERATION), OK);

        /**
         * @tc.steps:step3. insert some row.
         * @tc.expected: step3. return ok.
         */
        if (pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY) {
            sql = "insert into " + tableName + "(id, name, age)" \
                " values(1, 'zhangsan1', 10.1), (1, 'zhangsan2', 10.1), (2, 'zhangsan1', 10.0), (3, 'zhangsan3', 30),"
                " (4, 'zhangsan4', 40.123), (5, 'zhangsan5', 50.123);";
        } else {
            sql = "insert into " + tableName + "(id, name)" \
                " values(1, 'zhangsan1'), (2, 'zhangsan2'), (3, 'zhangsan3'), (4, 'zhangsan4'),"
                " (5, 'zhangsan5'), (6, 'zhangsan6'), (7, 'zhangsan7');";
        }
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

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
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
            if (pkType != PrimaryKeyType::COMPOSITE_PRIMARY_KEY && i == 6) { // 6 is index
                sql = "delete from " + tableName + " where id = 7;";
                EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
            }
        }

        EXPECT_EQ(sqlite3_close_v2(db), E_OK);

        SetCloudSchema(pkType, nullable);
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

    void DistributedDBCloudSaveCloudDataTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        LOGD("Test dir is %s", g_testDir.c_str());
        g_dbDir = g_testDir + "/";
        g_storePath = g_dbDir + STORE_ID + DB_SUFFIX;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    void DistributedDBCloudSaveCloudDataTest::TearDownTestCase(void)
    {
    }

    void DistributedDBCloudSaveCloudDataTest::SetUp()
    {
        CreatDB();
        DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, g_delegate);
        EXPECT_EQ(status, OK);
        ASSERT_NE(g_delegate, nullptr);
        g_cloudStore = (ICloudSyncStorageInterface *) GetRelationalStore();
        ASSERT_NE(g_cloudStore, nullptr);
    }

    void DistributedDBCloudSaveCloudDataTest::TearDown()
    {
        RefObject::DecObjRef(g_store);
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), OK);
        g_delegate = nullptr;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    std::shared_ptr<StorageProxy> GetStorageProxy(ICloudSyncStorageInterface *store)
    {
        return StorageProxy::GetCloudDb(store);
    }

    void GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType pkType, const std::string &gidStr, int64_t id,
        int expectCode, bool compositePkMatch = false)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, pkType);

        /**
         * @tc.steps:step2. call GetInfoByPrimaryKeyOrGid.
         * @tc.expected: step2. return expectCode.
         */
        std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
        ASSERT_NE(storageProxy, nullptr);
        EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
        VBucket vBucket;
        vBucket["id"] = id ;
        if (compositePkMatch) {
            std::string name = "zhangsan1";
            vBucket["name"] = name;
            vBucket["age"] = 10.1; // 10.1 is test age
        } else {
            std::string name = "zhangsan100";
            vBucket["name"] = name;
            vBucket["age"] = 10.11; // 10.11 is test age
        }
        vBucket[CloudDbConstant::GID_FIELD] = gidStr;
        DataInfoWithLog dataInfoWithLog;
        VBucket assetInfo;
        EXPECT_EQ(storageProxy->GetInfoByPrimaryKeyOrGid(g_tableName, vBucket, dataInfoWithLog, assetInfo), expectCode);
        if (expectCode == E_OK) {
            if (pkType == PrimaryKeyType::SINGLE_PRIMARY_KEY) {
                int64_t val = -1;
                // id is pk
                EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket("id", dataInfoWithLog.primaryKeys, val), E_OK);
                LOGD("ID = %d", val);
            } else if (pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY) {
                EXPECT_TRUE(dataInfoWithLog.primaryKeys.find("id") != dataInfoWithLog.primaryKeys.end());
                std::string name;
                EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket("name", dataInfoWithLog.primaryKeys, name), E_OK);
                LOGD("name = %s", name.c_str());
            } else {
                EXPECT_EQ(dataInfoWithLog.primaryKeys.size(), 0u);
            }
        }
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest001
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has single primary key and gid = "", id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest001, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, "", 100L, -E_NOT_FOUND);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest002
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has single primary key and gid = "", id = 1;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest002, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, "", 1, E_OK);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest003
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has single primary key and gid = abcd0, id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest003, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, g_gid + std::to_string(0), 100L, E_OK);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest004
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has single primary key and gid = abcd0, id = 2, which will
     * match two records;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest004, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, g_gid + std::to_string(0), 2L,
            -E_CLOUD_ERROR);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest005
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has single primary key and gid = abcd100, id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest005, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, g_gid + std::to_string(100), 100L,
            -E_NOT_FOUND);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest006
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has no primary key and gid = abcd0, id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest006, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::NO_PRIMARY_KEY, g_gid + std::to_string(0), 100L, E_OK);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest007
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has no primary key and gid = "", id = 1;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest007, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::NO_PRIMARY_KEY, "", 1L, -E_CLOUD_ERROR);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest008
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has no primary key and gid = abcd100, id = 1;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest008, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::NO_PRIMARY_KEY, g_gid + std::to_string(100), 1L,
            -E_NOT_FOUND);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest009
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has composite primary key and gid = "", primary key match;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest009, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, "", 1L, E_OK, true);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest010
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has composite primary key and gid = "",
     * primary key mismatch;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest010, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, "", 1L, -E_NOT_FOUND, false);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest011
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when table has composite primary key and gid match,
     * primary key mismatch
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest011, TestSize.Level0)
    {
        GetInfoByPrimaryKeyOrGidTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, "abcd0", 11L, E_OK, false);
    }

    void VbucketWithoutPrimaryDataTest(PrimaryKeyType pkType)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, pkType);

        /**
         * @tc.steps:step2. call GetInfoByPrimaryKeyOrGid.
         * @tc.expected: step2. return E_OK.
         */
        std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
        ASSERT_NE(storageProxy, nullptr);
        EXPECT_EQ(storageProxy->StartTransaction(), E_OK);
        VBucket vBucket;
        std::string gid = g_gid + std::to_string(0);
        vBucket[CloudDbConstant::GID_FIELD] = gid;
        DataInfoWithLog dataInfoWithLog;
        VBucket assetInfo;
        EXPECT_EQ(storageProxy->GetInfoByPrimaryKeyOrGid(g_tableName, vBucket, dataInfoWithLog, assetInfo), E_OK);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest012
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when vbucket doesn't contain pk data and gid match,
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest012, TestSize.Level0)
    {
        VbucketWithoutPrimaryDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY);
    }

    /**
     * @tc.name: GetInfoByPrimaryKeyOrGidTest013
     * @tc.desc: Test GetInfoByPrimaryKeyOrGid when vbucket doesn't contain pk(composite pk) data and gid match,
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetInfoByPrimaryKeyOrGidTest013, TestSize.Level0)
    {
        VbucketWithoutPrimaryDataTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY);
    }

    void ConstructDownloadData(DownloadData &downloadData, GidType gidType, bool nullable, bool vBucketContains)
    {
        for (int i = 0; i < 7; i++) { // 7 is record counts
            VBucket vBucket;
            if (i == 3) { // 3 is record index
                vBucket["id"] = 4L + i; // id = 5, 6 already pre_insert
            } else {
                vBucket["id"] = 1L + i;
            }

            std::string name = "lisi" + std::to_string(i);
            vBucket["name"] = name;
            vBucket["age"] = 100.0 + i; // 100.0 is offset for cloud data
            if (vBucketContains) {
                vBucket["sex"] = i % 2 ? true : false; // 2 is mod
            }

            vBucket["image"] = std::vector<uint8_t>(1, i);
            std::string gid;
            if (gidType == GidType::GID_MATCH) {
                gid = g_gid + std::to_string(i);
            } else if (gidType == GidType::GID_EMPTY) {
                std::string emptyGid = "";
                gid = emptyGid;
            } else if (gidType == GidType::GID_INVALID) {
                std::string invalidGid = "abc'd";
                gid = invalidGid;
            } else {
                gid = std::to_string(i) + g_gid;
            }

            vBucket[CloudDbConstant::GID_FIELD] = gid;
            int64_t cTime = 12345678L + i;
            vBucket[CloudDbConstant::CREATE_FIELD] = cTime;
            int64_t mTime = 12345679L + i;
            vBucket[CloudDbConstant::MODIFY_FIELD] = mTime;
            downloadData.data.push_back(vBucket);
        }

        downloadData.opType = { OpType::UPDATE, OpType::DELETE, OpType::ONLY_UPDATE_GID, OpType::INSERT,
            OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO, OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE, OpType::NOT_HANDLE };
    }

    void SaveCloudDataTest(PrimaryKeyType pkType, GidType gidType = GidType::GID_MATCH, bool nullable = true,
        bool vBucketContains = true, int expectCode = E_OK)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, pkType, nullable);

        /**
         * @tc.steps:step2. call PutCloudSyncData
         * @tc.expected: step2. return ok.
         */
        std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
        ASSERT_NE(storageProxy, nullptr);
        EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);

        DownloadData downloadData;
        ConstructDownloadData(downloadData, gidType, nullable, vBucketContains);
        EXPECT_EQ(storageProxy->PutCloudSyncData(g_tableName, downloadData), expectCode);
        if (expectCode == E_OK) {
            for (size_t i = 0; i < downloadData.opType.size(); i++) {
                if (downloadData.opType[i] == OpType::INSERT) {
                    EXPECT_TRUE(downloadData.data[i].find(CloudDbConstant::ROW_ID_FIELD_NAME) !=
                        downloadData.data[i].end());
                }
            }
        }
        EXPECT_EQ(storageProxy->Commit(), E_OK);
    }

    /**
     * @tc.name: PutCloudSyncDataTest001
     * @tc.desc: Test save cloud data into table with no primary key, gid match
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest001, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::NO_PRIMARY_KEY);
        // there is one log record with cloud_gid = abcd3(id = 7 will delete first, then insert again)
        std::string sql = "select count(data_key) from " + DBCommon::GetLogTableName(g_tableName) +
            " where cloud_gid = '" + g_gid + std::to_string(3) + "'"; // 3 is gid index
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
        EXPECT_NE(db, nullptr);
        int errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [] (sqlite3_stmt *stmt) {
            EXPECT_EQ(sqlite3_column_int64(stmt, 0), 1); // will get only 1 log record
            return OK;
        });
        EXPECT_EQ(errCode, SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), E_OK);
    }

    /**
     * @tc.name: PutCloudSyncDataTest002
     * @tc.desc: Test save cloud data into table with no primary key, gid mismatch
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest002, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::NO_PRIMARY_KEY, GidType::GID_MISMATCH);
    }

    /**
     * @tc.name: PutCloudSyncDataTest003
     * @tc.desc: Test save cloud data into table with no primary key, gid is empty
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest003, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::NO_PRIMARY_KEY, GidType::GID_EMPTY, true, true, -E_CLOUD_ERROR);
    }

    /**
     * @tc.name: PutCloudSyncDataTest004
     * @tc.desc: Test save cloud data into table with single primary key, gid match
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest004, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY);
    }

    /**
     * @tc.name: PutCloudSyncDataTest005
     * @tc.desc: Test save cloud data into table with single primary key, gid mismatch
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest005, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, GidType::GID_MISMATCH);
    }

    /**
     * @tc.name: PutCloudSyncDataTest006
     * @tc.desc: Test save cloud data into table with single primary key, gid is empty
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest006, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, GidType::GID_EMPTY, true, true, -E_CLOUD_ERROR);
    }

    /**
     * @tc.name: PutCloudSyncDataTest007
     * @tc.desc: Test save cloud data into table with single primary key, gid is empty, cloud field less than schema,
     * field can be null
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest007, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, GidType::GID_MATCH, true, false);
    }

    /**
     * @tc.name: PutCloudSyncDataTest008
     * @tc.desc: Test save cloud data into table with single primary key, gid is empty, cloud field less than schema,
     * field can not be null
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest008, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, GidType::GID_EMPTY, false, false, -E_CLOUD_ERROR);
    }

    /**
     * @tc.name: PutCloudSyncDataTest009
     * @tc.desc: Test save cloud data into table with composite primary key, gid match, primary key mismatch
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest009, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY);
    }

    /**
     * @tc.name: PutCloudSyncDataTest010
     * @tc.desc: Test save cloud data into table with composite primary key, gid mismatch, primary key mismatch
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest010, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, GidType::GID_MISMATCH);
    }

    /**
     * @tc.name: PutCloudSyncDataTest011
     * @tc.desc: Test save cloud data into table with composite primary key, invalid gid
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest011, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, GidType::GID_INVALID, true, true, -E_CLOUD_ERROR);
    }

    void ConstructMultiDownloadData(DownloadData &downloadData, GidType gidType)
    {
        for (int i = 0; i < 6; i++) { // 6 is record counts
            VBucket vBucket;
            if (i < 1) { // UPDATE_TIMESTAMP doesn't contain pk
                vBucket["id"] = 1L + i;
            } else if (i > 1) {
                vBucket["id"] = 10L + i; // 10 is id offset for cloud data
            }

            std::string name = "lisi" + std::to_string(i);
            vBucket["name"] = name;
            vBucket["age"] = 100.0 + i; // 100.0 is offset for cloud data
            vBucket["sex"] = i % 2 ? true : false; // 2 is mod

            vBucket["image"] = std::vector<uint8_t>(1, i);
            std::string gid;
            if (gidType == GidType::GID_MATCH) {
                if (i <= 1) { // first 2 exists in local
                    gid = g_gid + std::to_string(i);
                } else {
                    gid = g_gid + std::to_string(10 + i); // 10 is id offset for cloud data
                }
            } else if (gidType == GidType::GID_EMPTY) {
                std::string emptyGid = "";
                gid = emptyGid;
            } else {
                gid = std::to_string(i) + g_gid;
            }

            vBucket[CloudDbConstant::GID_FIELD] = gid;
            int64_t cTime = BASE_CREATE_TIME + i;
            vBucket[CloudDbConstant::CREATE_FIELD] = cTime;
            int64_t mTime = BASE_MODIFY_TIME + i;
            vBucket[CloudDbConstant::MODIFY_FIELD] = mTime;
            downloadData.data.push_back(vBucket);
        }

        downloadData.opType = { OpType::UPDATE, OpType::UPDATE_TIMESTAMP, OpType::INSERT, OpType::INSERT,
            OpType::INSERT, OpType::NOT_HANDLE };
    }

    /**
     * @tc.name: PutCloudSyncDataTest012
     * @tc.desc: Test save cloud data into table with no primary key, multi cloud data
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest012, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, PrimaryKeyType::NO_PRIMARY_KEY, true);

        /**
         * @tc.steps:step2. call PutCloudSyncData
         * @tc.expected: step2. return ok.
         */
        std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
        ASSERT_NE(storageProxy, nullptr);
        EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);

        DownloadData downloadData;
        ConstructMultiDownloadData(downloadData, GidType::GID_MATCH);
        EXPECT_EQ(storageProxy->PutCloudSyncData(g_tableName, downloadData), E_OK);
        EXPECT_EQ(storageProxy->Commit(), E_OK);
    }

    /**
     * @tc.name: PutCloudSyncDataTest013
     * @tc.desc: Test save cloud data with type = update_timestamp
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest013, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, PrimaryKeyType::SINGLE_PRIMARY_KEY, true);

        std::string sql = "delete from " + g_tableName + " where id = 2";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
        EXPECT_NE(db, nullptr);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        /**
         * @tc.steps:step2. call PutCloudSyncData
         * @tc.expected: step2. return ok.
         */
        std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
        ASSERT_NE(storageProxy, nullptr);
        EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);

        DownloadData downloadData;
        ConstructMultiDownloadData(downloadData, GidType::GID_MATCH);
        EXPECT_EQ(storageProxy->PutCloudSyncData(g_tableName, downloadData), E_OK);
        EXPECT_EQ(storageProxy->Commit(), E_OK);

        /**
         * @tc.steps:step3. verify data
         * @tc.expected: step3. verify data ok.
         */
        sql = "select device, timestamp, flag from " + DBCommon::GetLogTableName(g_tableName) +
            " where data_key = -1 and cloud_gid = ''";
        int count = 0;
        int errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [&count] (sqlite3_stmt *stmt) {
            std::string device = "cloud";
            std::vector<uint8_t> deviceVec;
            (void)SQLiteUtils::GetColumnBlobValue(stmt, 0, deviceVec);    // 0 is device
            std::string getDevice;
            DBCommon::VectorToString(deviceVec, getDevice);
            EXPECT_EQ(device, getDevice);
            EXPECT_EQ(sqlite3_column_int64(stmt, 1), BASE_MODIFY_TIME + 1);
            EXPECT_EQ(sqlite3_column_int(stmt, 2), 1); // 2 is flag
            count++;
            return OK;
        });
        EXPECT_EQ(errCode, E_OK);
        EXPECT_EQ(count, 1);
        EXPECT_EQ(sqlite3_close_v2(db), E_OK);
    }

    /**
     * @tc.name: PutCloudSyncDataTest014
     * @tc.desc: Test PutCloudSyncData when vbucket doesn't contain pk data and gid match,
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest014, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, PrimaryKeyType::SINGLE_PRIMARY_KEY);

        /**
         * @tc.steps:step2. construct data without primary key value, call PutCloudSyncData.
         * @tc.expected: step2. return E_OK.
         */
        std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
        ASSERT_NE(storageProxy, nullptr);
        EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);

        DownloadData downloadData;
        VBucket vBucket;
        std::string gid = g_gid + std::to_string(0);
        vBucket[CloudDbConstant::GID_FIELD] = gid;
        vBucket[CloudDbConstant::MODIFY_FIELD] = BASE_MODIFY_TIME;
        downloadData.data.push_back(vBucket);
        downloadData.opType = { OpType::DELETE };
        EXPECT_EQ(storageProxy->PutCloudSyncData(g_tableName, downloadData), E_OK);
        EXPECT_EQ(storageProxy->Commit(), E_OK);
    }

    /**
     * @tc.name: PutCloudSyncDataTest015
     * @tc.desc: Test clear gid and ONLY_UPDATE_GID
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest015, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, PrimaryKeyType::SINGLE_PRIMARY_KEY);

        /**
         * @tc.steps:step2. construct data type = clear_gid, call PutCloudSyncData.
         * @tc.expected: step2. return E_OK.
         */
        std::shared_ptr<StorageProxy> storageProxy = GetStorageProxy(g_cloudStore);
        ASSERT_NE(storageProxy, nullptr);
        EXPECT_EQ(storageProxy->StartTransaction(TransactType::IMMEDIATE), E_OK);

        DownloadData downloadData;
        for (int i = 0; i < 2; i++) { // 2 is record count
            VBucket vBucket;
            std::string gid = g_gid + std::to_string(i * 4); // 4 is data index
            vBucket[CloudDbConstant::GID_FIELD] = gid;
            vBucket[CloudDbConstant::MODIFY_FIELD] = BASE_MODIFY_TIME;
            downloadData.data.push_back(vBucket);
        }
        downloadData.opType = { OpType::ONLY_UPDATE_GID, OpType::CLEAR_GID };
        EXPECT_EQ(storageProxy->PutCloudSyncData(g_tableName, downloadData), E_OK);
        EXPECT_EQ(storageProxy->Commit(), E_OK);

        /**
         * @tc.steps:step3. verify data
         * @tc.expected: step3. verify data ok.
         */
        std::string sql = "select cloud_gid, flag from " + DBCommon::GetLogTableName(g_tableName) +
            " where data_key = 1 or data_key = 5";
        int count = 0;
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
        EXPECT_NE(db, nullptr);
        int errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [&count] (sqlite3_stmt *stmt) {
            std::string gid = "";
            if (count == 0) {
                gid = g_gid + "0";
            }
            const unsigned char *val = sqlite3_column_text(stmt, 0);
            EXPECT_TRUE(val != nullptr);
            std::string getGid = reinterpret_cast<const char *>(val);
            LOGD("GET GID = %s", getGid.c_str());
            EXPECT_EQ(getGid, gid);
            if (count == 1) {
                int flag = sqlite3_column_int(stmt, 1);
                EXPECT_EQ(flag & 0x04, 0); // 0x04 is binay num of b100, clear gid will clear 2th bit of flag
            }
            count++;
            return OK;
        });
        EXPECT_EQ(errCode, E_OK);
        EXPECT_EQ(count, 2); // 2 is result count
        EXPECT_EQ(sqlite3_close_v2(db), E_OK);
    }
}
