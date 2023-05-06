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


using namespace testing::ext;
using namespace  DistributedDB;
using namespace  DistributedDBUnitTest;

namespace {
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

    enum class PrimaryKeyType {
        NO_PRIMARY_KEY,
        SINGLE_PRIMARY_KEY,
        COMPOSITE_PRIMARY_KEY
    };

    enum class GidType {
        GID_EMPTY,
        GID_MATCH,
        GID_MISMATCH
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
        Field field1 = { "id", TYPE_INDEX<int64_t>,
            pkType == PrimaryKeyType::SINGLE_PRIMARY_KEY || pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY, true };
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
         * @tc.steps:step3. insert 4 row.
         * @tc.expected: step3. return ok.
         */
        if (pkType == PrimaryKeyType::COMPOSITE_PRIMARY_KEY) {
            sql = "insert into " + tableName + "(id, name, age)" \
                " values(1, 'zhangsan1', 10.1), (1, 'zhangsan2', 10.1), (2, 'zhangsan1', 10.0), (3, 'zhangsan3', 30);";
        } else {
            sql = "insert into " + tableName + "(id, name)" \
                " values(1, 'zhangsan1'), (2, 'zhangsan2'), (3, 'zhangsan3'), (4, 'zhangsan4');";
        }
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

        /**
         * @tc.steps:step3. preset cloud gid.
         * @tc.expected: step3. return ok.
         */
        for (int i = 0; i < 3; i++) { // update first 3 records
            sql = "update " + DBCommon::GetLogTableName(tableName) + " set cloud_gid = '" + g_gid + std::to_string(i) +
                "' where data_key = " + std::to_string(i + 1);
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
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

    void GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType pkType, const std::string &gidStr, int64_t id,
        int expectCode, bool compositePkMatch = false)
    {
        /**
         * @tc.steps:step1. create db, create table.
         * @tc.expected: step1. return ok.
         */
        PrepareDataBase(g_tableName, pkType);

        /**
         * @tc.steps:step2. call GetLogInfoByPrimaryKeyOrGid.
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
        LogInfo logInfo;
        EXPECT_EQ(storageProxy->GetLogInfoByPrimaryKeyOrGid(g_tableName, vBucket, logInfo), expectCode);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest001
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has single primary key and gid = "", id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest001, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, "", 100L, -E_NOT_FOUND);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest002
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has single primary key and gid = "", id = 1;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest002, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, "", 1, E_OK);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest003
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has single primary key and gid = abcd0, id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest003, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, g_gid + std::to_string(0), 100L, E_OK);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest004
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has single primary key and gid = abcd0, id = 2, which will
     * match two records;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest004, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, g_gid + std::to_string(0), 2L,
            -E_INVALID_DATA);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest005
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has single primary key and gid = abcd100, id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest005, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, g_gid + std::to_string(100), 100L,
            -E_NOT_FOUND);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest006
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has no primary key and gid = abcd0, id = 100;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest006, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::NO_PRIMARY_KEY, g_gid + std::to_string(0), 100L, E_OK);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest007
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has no primary key and gid = "", id = 1;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest007, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::NO_PRIMARY_KEY, "", 1L, -E_INVALID_DATA);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest008
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has no primary key and gid = abcd100, id = 1;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest008, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::NO_PRIMARY_KEY, g_gid + std::to_string(100), 1L,
            -E_NOT_FOUND);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest009
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has composite primary key and gid = "", primary key match;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest009, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, "", 1L, E_OK, true);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest010
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has composite primary key and gid = "",
     * primary key mismatch;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest010, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, "", 1L, -E_NOT_FOUND, false);
    }

    /**
     * @tc.name: GetLogInfoByPrimaryKeyOrGidTest011
     * @tc.desc: Test GetLogInfoByPrimaryKeyOrGid when table has composite primary key and gid match,
     * primary key mismatch
     * primary key mismatch;
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, GetLogInfoByPrimaryKeyOrGidTest011, TestSize.Level0)
    {
        GetLogInfoByPrimaryKeyOrGidTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, "abcd0", 11L, E_OK, false);
    }

    void ConstructDownloadData(DownloadData &downloadData, GidType gidType, bool nullable, bool vBucketContains)
    {
        for (int i = 0; i < 5; i++) { // 5 is record counts
            VBucket vBucket;
            if (i == 3) { // 3 is record index
                vBucket["id"] = 2L + i; // id = 4 already pre_insert
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

        downloadData.opType = { OpType::UPDATE, OpType::DELETE, OpType::ONLY_UPDATE_GID,
            OpType::INSERT, OpType::NOT_HANDLE };
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
        SaveCloudDataTest(PrimaryKeyType::NO_PRIMARY_KEY, GidType::GID_EMPTY, true, true, -E_INVALID_DATA);
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
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, GidType::GID_EMPTY);
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
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, GidType::GID_EMPTY, true, false);
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
        SaveCloudDataTest(PrimaryKeyType::SINGLE_PRIMARY_KEY, GidType::GID_EMPTY, false, false, -E_INVALID_DATA);
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
     * @tc.desc: Test save cloud data into table with composite primary key, gid is empty, primary key mismatch
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshijie
     */
    HWTEST_F(DistributedDBCloudSaveCloudDataTest, PutCloudSyncDataTest011, TestSize.Level0)
    {
        SaveCloudDataTest(PrimaryKeyType::COMPOSITE_PRIMARY_KEY, GidType::GID_EMPTY);
    }

    void ConstructMultiDownloadData(DownloadData &downloadData, GidType gidType)
    {
        for (int i = 0; i < 5; i++) { // 5 is record counts
            VBucket vBucket;
            if (i == 0) {
                vBucket["id"] = 1L + i;
            } else {
                vBucket["id"] = 10L + i; // 10 is id offset for cloud data
            }

            std::string name = "lisi" + std::to_string(i);
            vBucket["name"] = name;
            vBucket["age"] = 100.0 + i; // 100.0 is offset for cloud data
            vBucket["sex"] = i % 2 ? true : false; // 2 is mod

            vBucket["image"] = std::vector<uint8_t>(1, i);
            std::string gid;
            if (gidType == GidType::GID_MATCH) {
                gid = g_gid + std::to_string(i);
            } else if (gidType == GidType::GID_EMPTY) {
                std::string emptyGid = "";
                gid = emptyGid;
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

        downloadData.opType = { OpType::UPDATE, OpType::INSERT, OpType::INSERT,
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
}
