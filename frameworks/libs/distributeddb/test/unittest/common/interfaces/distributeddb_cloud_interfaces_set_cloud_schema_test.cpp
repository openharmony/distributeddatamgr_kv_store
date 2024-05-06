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
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "db_common.h"
#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "time_helper.h"
#include "sqlite_relational_utils.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"
#include "cloud_db_sync_utils_test.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char *DB_SUFFIX = ".db";
    constexpr const char *STORE_ID = "Relational_Store_ID";
    constexpr const char *CREATE_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS worker1(" \
        "name TEXT PRIMARY KEY," \
        "height REAL ," \
        "married BOOLEAN ," \
        "photo BLOB NOT NULL," \
        "asset BLOB," \
        "age INT);";
    constexpr const char *CREATE_TABLE_WITHOUT_PRIMARY_SQL =
        "CREATE TABLE IF NOT EXISTS worker2(" \
        "id INT," \
        "name TEXT," \
        "height REAL ," \
        "married BOOLEAN ," \
        "photo BLOB ," \
        "asset BLOB);";
    constexpr const char *CREATE_SHARED_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS worker5_shared(" \
        "name TEXT PRIMARY KEY," \
        "height REAL ," \
        "married BOOLEAN ," \
        "photo BLOB NOT NULL," \
        "asset BLOB," \
        "age INT);";
    const string g_tableName1 = "worker1";
    const string g_tableName2 = "worker2"; // do not have primary key
    const string g_tableName3 = "Worker1";
    const string g_tableName4 = "worker4";
    const string g_sharedTableName1 = "worker1_shared";
    const string g_sharedTableName2 = "worker2_shared"; // do not have primary key
    const string g_sharedTableName3 = "Worker1_Shared";
    const string g_sharedTableName4 = "worker4_shared";
    const string g_sharedTableName5 = "worker5_shared";
    const string g_distributedSharedTableName1 = "naturalbase_rdb_aux_worker1_shared_log";
    const string g_distributedSharedTableName2 = "naturalbase_rdb_aux_worker2_shared_log";
    const string g_distributedSharedTableName3 = "naturalbase_rdb_aux_Worker1_Shared_log";
    const string g_distributedSharedTableName4 = "naturalbase_rdb_aux_worker4_shared_log";
    std::string g_testDir;
    std::string g_dbDir;
    std::string g_storePath;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;
    std::shared_ptr<VirtualCloudDb> g_virtualCloudDb = nullptr;
    std::shared_ptr<VirtualCloudDataTranslate> g_virtualCloudDataTranslate;
    const std::vector<Field> g_cloudField1 = {
        {"id", TYPE_INDEX<int64_t>, true}, {"name", TYPE_INDEX<std::string>},
        {"height", TYPE_INDEX<double>}, {"married", TYPE_INDEX<bool>},
        {"photo", TYPE_INDEX<Bytes>}, {"asset", TYPE_INDEX<Asset>}
    };
    const std::vector<Field> g_cloudField2 = {
        {"id", TYPE_INDEX<int64_t>}, {"name", TYPE_INDEX<std::string>},
        {"height", TYPE_INDEX<double>}, {"married", TYPE_INDEX<bool>},
        {"photo", TYPE_INDEX<Bytes>}, {"asset", TYPE_INDEX<Asset>}
    };
    const std::vector<Field> g_cloudField3 = {
        {"id", TYPE_INDEX<int64_t>, true}, {"name", TYPE_INDEX<std::string>},
        {"height", TYPE_INDEX<double>}, {"married", TYPE_INDEX<bool>},
        {"photo", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>},
        {"asset", TYPE_INDEX<Asset>}
    };
    const std::vector<Field> g_cloudField4 = {
        {"id", TYPE_INDEX<int64_t>, true}, {"name", TYPE_INDEX<std::string>},
        {"height", TYPE_INDEX<double>}, {"photo", TYPE_INDEX<Bytes>},
        {"assets", TYPE_INDEX<Assets>}, {"age", TYPE_INDEX<int64_t>}
    };
    const int64_t g_syncWaitTime = 60;
    const Asset g_localAsset = {
        .version = 1, .name = "Phone", .assetId = "", .subpath = "/local/sync", .uri = "/local/sync",
        .modifyTime = "123456", .createTime = "", .size = "256", .hash = "ASE"
    };

    class DistributedDBCloudInterfacesSetCloudSchemaTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    protected:
        void CreateUserDBAndTable();
        void CheckSharedTable(const std::vector<std::string> &expectedTableName);
        void CheckDistributedSharedTable(const std::vector<std::string> &expectedTableName);
        void InsertLocalSharedTableRecords(int64_t begin, int64_t count, const std::string &tableName,
            bool isUpdate = false);
        void InsertCloudTableRecord(int64_t begin, int64_t count, bool isShare = true, int64_t beginGid = -1);
        void DeleteCloudTableRecord(int64_t beginGid, int64_t count, bool isShare = true);
        void BlockSync(const Query &query, RelationalStoreDelegate *delegate, DBStatus errCode = OK,
            SyncMode mode = SYNC_MODE_CLOUD_MERGE);
        void CheckCloudTableCount(const std::string &tableName, int64_t expectCount);
        void CloseDb();
        void InitCloudEnv();
        sqlite3 *db_ = nullptr;
        std::function<void(int64_t, VBucket &)> forkInsertFunc_;
    };

    void DistributedDBCloudInterfacesSetCloudSchemaTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        LOGD("Test dir is %s", g_testDir.c_str());
        g_dbDir = g_testDir + "/";
        g_storePath = g_dbDir + STORE_ID + DB_SUFFIX;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
        g_virtualCloudDataTranslate = std::make_shared<VirtualCloudDataTranslate>();
        RuntimeConfig::SetCloudTranslate(g_virtualCloudDataTranslate);
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::TearDownTestCase(void)
    {
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::SetUp(void)
    {
        db_ = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db_, nullptr);
        CreateUserDBAndTable();
        DBStatus status = g_mgr.OpenStore(g_storePath, STORE_ID, {}, g_delegate);
        ASSERT_EQ(status, OK);
        ASSERT_NE(g_delegate, nullptr);
        g_virtualCloudDb = std::make_shared<VirtualCloudDb>();
        ASSERT_EQ(g_delegate->SetCloudDB(g_virtualCloudDb), DBStatus::OK);
        ASSERT_EQ(g_delegate->SetIAssetLoader(std::make_shared<VirtualAssetLoader>()), DBStatus::OK);
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::TearDown(void)
    {
        g_virtualCloudDb->ForkUpload(nullptr);
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), OK);
        g_delegate = nullptr;
        EXPECT_EQ(sqlite3_close_v2(db_), SQLITE_OK);
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::CreateUserDBAndTable()
    {
        ASSERT_EQ(RelationalTestUtils::ExecSql(db_, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        ASSERT_EQ(RelationalTestUtils::ExecSql(db_, CREATE_TABLE_SQL), SQLITE_OK);
        ASSERT_EQ(RelationalTestUtils::ExecSql(db_, CREATE_TABLE_WITHOUT_PRIMARY_SQL), SQLITE_OK);
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::CheckSharedTable(
        const std::vector<std::string> &expectedTableName)
    {
        std::string sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND " \
            "name LIKE 'worker%_shared';";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        uint64_t index = 0;
        while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_TEXT);
            Type cloudValue;
            ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<std::string>, 0, cloudValue), E_OK);
            std::string actualTableName;
            ASSERT_EQ(CloudStorageUtils::GetValueFromOneField(cloudValue, actualTableName), E_OK);
            ASSERT_EQ(actualTableName, expectedTableName[index]);
            index++;
        }
        ASSERT_EQ(index, expectedTableName.size());
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::CheckDistributedSharedTable(
        const std::vector<std::string> &expectedTableName)
    {
        std::string sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND " \
            "name LIKE 'naturalbase_rdb_aux_%_log';";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        uint64_t index = 0;
        while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_TEXT);
            Type cloudValue;
            ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<std::string>, 0, cloudValue), E_OK);
            std::string actualTableName;
            ASSERT_EQ(CloudStorageUtils::GetValueFromOneField(cloudValue, actualTableName), E_OK);
            ASSERT_EQ(actualTableName, expectedTableName[index]);
            index++;
        }
        ASSERT_EQ(index, expectedTableName.size());
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::InsertLocalSharedTableRecords(int64_t begin, int64_t count,
        const std::string &tableName, bool isUpdate)
    {
        ASSERT_NE(db_, nullptr);
        std::vector<uint8_t> assetBlob = g_virtualCloudDataTranslate->AssetToBlob(g_localAsset);
        for (int64_t i = begin; i < count; ++i) {
            string sql = "";
            if (isUpdate) {
                sql = "INSERT OR REPLACE INTO " + tableName +
                    " (cloud_owner, cloud_privilege, id, name, height, married, photo, age, asset) VALUES ('A', " +
                    "'true', '" + std::to_string(i) + "', 'Local" + std::to_string(i) +
                    "', '155.10', 'false', 'text', '18', ?);";
            } else {
                sql = "INSERT OR REPLACE INTO " + tableName +
                    " (cloud_owner, cloud_privilege, id, name, height, married, photo, asset) VALUES ('A', 'true', '" +
                    std::to_string(i) + "', 'Local" + std::to_string(i) + "', '155.10', 'false', 'text', ?);";
            }
            sqlite3_stmt *stmt = nullptr;
            ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
            ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK);
            EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
            int errCode;
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }
    }

    void InsertSharingUri(int64_t index, VBucket &log)
    {
        log.insert_or_assign(CloudDbConstant::SHARING_RESOURCE_FIELD, std::string("uri:") + std::to_string(index));
    }

    std::string QueryResourceCountSql(const std::string &tableName)
    {
        return "SELECT COUNT(*) FROM " + DBCommon::GetLogTableName(tableName)
            + " where sharing_resource like 'uri%'";
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::InsertCloudTableRecord(int64_t begin, int64_t count,
        bool isShare, int64_t beginGid)
    {
        std::vector<uint8_t> photo(1, 'v');
        std::vector<VBucket> record;
        std::vector<VBucket> extend;
        Timestamp now = TimeHelper::GetSysCurrentTime();
        std::string tableName = g_tableName2;
        if (isShare) {
            tableName = g_sharedTableName1;
        }
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket data;
            if (isShare) {
                data.insert_or_assign("cloud_owner", std::string("ownerA"));
                data.insert_or_assign("cloud_privilege", std::string("true"));
            }
            data.insert_or_assign("id", i);
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", 166.0); // 166.0 is random double value
            data.insert_or_assign("married", false);
            data.insert_or_assign("photo", photo);
            record.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            if (beginGid >= 0) {
                log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(beginGid + i));
            }
            if (forkInsertFunc_) {
                forkInsertFunc_(i, log);
            }
            extend.push_back(log);
        }
        if (beginGid >= 0) {
            ASSERT_EQ(g_virtualCloudDb->BatchUpdate(tableName, std::move(record), extend), DBStatus::OK);
        } else {
            ASSERT_EQ(g_virtualCloudDb->BatchInsert(tableName, std::move(record), extend), DBStatus::OK);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::BlockSync(const Query &query,
        RelationalStoreDelegate *delegate, DBStatus errCode, SyncMode mode)
    {
        std::mutex dataMutex;
        std::condition_variable cv;
        bool finish = false;
        auto callback = [&cv, &dataMutex, &finish, &errCode](const std::map<std::string, SyncProcess> &process) {
            for (const auto &item: process) {
                if (item.second.process == DistributedDB::FINISHED) {
                    {
                        EXPECT_EQ(item.second.errCode, errCode);
                        std::lock_guard<std::mutex> autoLock(dataMutex);
                        finish = true;
                    }
                    cv.notify_one();
                }
            }
        };
        ASSERT_EQ(delegate->Sync({ "CLOUD" }, mode, query, callback, g_syncWaitTime), OK);
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::CheckCloudTableCount(const std::string &tableName,
        int64_t expectCount)
    {
        VBucket extend;
        extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
        int64_t realCount = 0;
        std::vector<VBucket> data;
        g_virtualCloudDb->Query(tableName, extend, data);
        for (size_t j = 0; j < data.size(); ++j) {
            auto entry = data[j].find(CloudDbConstant::DELETE_FIELD);
            if (entry != data[j].end() && std::get<bool>(entry->second)) {
                continue;
            }
            realCount++;
        }
        EXPECT_EQ(realCount, expectCount); // ExpectCount represents the total amount of cloud data.
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::CloseDb()
    {
        g_virtualCloudDb = nullptr;
        if (g_delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
            g_delegate = nullptr;
        }
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::InitCloudEnv()
    {
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName2, CLOUD_COOPERATION), DBStatus::OK);
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::DeleteCloudTableRecord(int64_t beginGid,
        int64_t count, bool isShare)
    {
        Timestamp now = TimeHelper::GetSysCurrentTime();
        std::vector<VBucket> extend;
        for (int64_t i = 0; i < count; ++i) {
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
            log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(beginGid + i));
            extend.push_back(log);
        }
        ASSERT_EQ(g_virtualCloudDb->BatchDelete(isShare ? g_sharedTableName1 : g_tableName2, extend), DBStatus::OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    /**
     * @tc.name: SetCloudDbSchemaTest001
     * @tc.desc: Test same table name for set cloud schema interface
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest001, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. table names are same but sharedtable names are empty
         * @tc.expected: step1. return INVALID_ARGS
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_tableName1 + "_shared",
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step2. table names are same but sharedtable names are different
         * @tc.expected: step2. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step3. one table name is uppercase and the other is lowercase
         * @tc.expected: step3. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName3,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
    }

    /**
     * @tc.name: SetCloudDbSchemaTest002
     * @tc.desc: Test same shared table name for set cloud schema interface
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest002, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. table names are different but sharedtable names are same
         * @tc.expected: step1. return INVALID_ARGS
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_tableName1 + "_shared",
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step2. table names are different but sharedtable names are same
         * @tc.expected: step2. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step3. one sharedtable name is uppercase and the other is lowercase
         * @tc.expected: step3. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        tableSchema = { g_tableName1, g_sharedTableName1, g_cloudField1 };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = { g_tableName2, g_sharedTableName3, g_cloudField2 };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step4. sharedtable name and table name is same
         * @tc.expected: step4. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_tableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
    }

    /**
     * @tc.name: SetCloudDbSchemaTest003
     * @tc.desc: Test same table name and shared table name for set cloud schema interface
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest003, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. table name and shared table name are same
         * @tc.expected: step1. return INVALID_ARGS
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_sharedTableName1,
            .sharedTableName = g_tableName2,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step2. shared table name and table name are same
         * @tc.expected: step2. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_tableName1,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
    }

    /**
     * @tc.name: SetCloudDbSchemaTest004
     * @tc.desc: Test same field for set cloud schema interface
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest004, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. fields contains same field
         * @tc.expected: step1. return INVALID_ARGS
         */
        DataBaseSchema dataBaseSchema;
        std::vector<Field> invalidFields = g_cloudField1;
        invalidFields.push_back({"name", TYPE_INDEX<std::string>});
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = invalidFields
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step2. fields contains cloud_owner
         * @tc.expected: step2. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        invalidFields = g_cloudField1;
        invalidFields.push_back({"cloud_owner", TYPE_INDEX<std::string>});
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = invalidFields
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step3. fields contains cloud_privilege
         * @tc.expected: step3. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        invalidFields = g_cloudField1;
        invalidFields.push_back({"cloud_privilege", TYPE_INDEX<std::string>});
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = invalidFields
        };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step4. fields contains same field but uppercase
         * @tc.expected: step4. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        invalidFields = g_cloudField1;
        invalidFields.push_back({"Name", TYPE_INDEX<std::string>});
        tableSchema = { g_tableName1, g_sharedTableName1, invalidFields };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step5. fields contains cloud_privilege field but uppercase
         * @tc.expected: step5. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        invalidFields = g_cloudField1;
        invalidFields.push_back({"Cloud_priVilege", TYPE_INDEX<std::string>});
        tableSchema = { g_tableName1, g_sharedTableName1, invalidFields };
        dataBaseSchema.tables.push_back(tableSchema);
        EXPECT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
    }

    /**
     * @tc.name: SetCloudDbSchemaTest005
     * @tc.desc: Check shared table is existed or not
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest005, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1});
        CheckDistributedSharedTable({g_distributedSharedTableName1});

        /**
         * @tc.steps:step2. re-SetCloudDbSchema and schema table changed
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName2});

        /**
         * @tc.steps:step3. re-SetCloudDbSchema and schema fields changed
         * @tc.expected: step3. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
        CheckSharedTable({g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName2});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest006
     * @tc.desc: Test SetCloudDbSchema in uppercase and lowercase
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest006, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName3,
            .sharedTableName = g_sharedTableName3,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName3, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName3, g_distributedSharedTableName2});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest007
     * @tc.desc: Test SetCloudDbSchema if need alter shared table name
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest007, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1});
        CheckDistributedSharedTable({g_distributedSharedTableName1});

        /**
         * @tc.steps:step2. re-SetCloudDbSchema and schema shared table changed
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName2});

        /**
         * @tc.steps:step3. re-SetCloudDbSchema and schema table changed
         * @tc.expected: step3. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName2});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest008
     * @tc.desc: Test SetCloudDbSchema when A-B C-D update B-D
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest008, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema A-B C-D
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});

        /**
         * @tc.steps:step2. re-SetCloudDbSchema B-D
         * @tc.expected: step2. return INVALID_ARGS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_sharedTableName1,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest009
     * @tc.desc: Test SetCloudDbSchema when A-B C-D update A-D
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest009, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});

        /**
         * @tc.steps:step2. re-SetCloudDbSchema and schema shared table changed
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName2});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest010
     * @tc.desc: Test SetCloudDbSchema when A-B C-D update A-E C-B
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest010, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});

        /**
         * @tc.steps:step2. re-SetCloudDbSchema and schema shared table changed
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName4,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName4, g_sharedTableName1});
        CheckDistributedSharedTable({g_distributedSharedTableName4, g_distributedSharedTableName1});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest011
     * @tc.desc: Test SetCloudDbSchema if local exists user table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest011, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. local exists worker1 then use SetCloudDbSchema
         * @tc.expected: step1. return INVALID_ARGS
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_tableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);

        /**
         * @tc.steps:step2. SetCloudDbSchema
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1});
        CheckDistributedSharedTable({g_distributedSharedTableName1});

        /**
         * @tc.steps:step3. add field and SetCloudDbSchema
         * @tc.expected: step3. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField3
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName2});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest012
     * @tc.desc: Test SetCloudDbSchema if local exists user table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest012, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        ASSERT_EQ(RelationalTestUtils::ExecSql(db_, CREATE_SHARED_TABLE_SQL), SQLITE_OK);
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});

        /**
         * @tc.steps:step2. re-SetCloudDbSchema and schema shared table changed
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName4,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName4,
            .sharedTableName = g_sharedTableName5,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
    }

    /**
     * @tc.name: SetCloudDbSchemaTest013
     * @tc.desc: Test SetCloudDbSchema after close db
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest013, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});

        /**
         * @tc.steps:step2. close db and then open store
         * @tc.expected: step2. return OK
         */
        CloseDb();
        DBStatus status = g_mgr.OpenStore(g_storePath, STORE_ID, {}, g_delegate);
        ASSERT_EQ(status, OK);
        g_virtualCloudDb = std::make_shared<VirtualCloudDb>();
        ASSERT_EQ(g_delegate->SetCloudDB(g_virtualCloudDb), DBStatus::OK);

        /**
         * @tc.steps:step3. re-SetCloudDbSchema and schema is same
         * @tc.expected: step3. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest014
     * @tc.desc: Test SetCloudDbSchema after set tracker table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest014, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetTrackerTable
         * @tc.expected: step1. return OK
         */
        TrackerSchema trackerSchema = {
            .tableName = g_tableName1,
            .extendColName = "married",
            .trackerColNames = {"married"}
        };
        ASSERT_EQ(g_delegate->SetTrackerTable(trackerSchema), DBStatus::OK);
        ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName1, CLOUD_COOPERATION), DBStatus::OK);

        /**
         * @tc.steps:step2. use SetCloudDbSchema
         * @tc.expected: step2. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
    }

    /**
     * @tc.name: SharedTableSync001
     * @tc.desc: Test sharedtable without primary key sync
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync001, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use set shared table without primary key
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField2
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);

        /**
         * @tc.steps:step2. insert local shared table records and sync
         * @tc.expected: step2. return OK
         */
        InsertLocalSharedTableRecords(0, 10, g_sharedTableName2);
        Query query = Query::Select().FromTable({ g_sharedTableName2 });
        BlockSync(query, g_delegate);
        CheckCloudTableCount(g_sharedTableName2, 10);
    }

    /**
     * @tc.name: SharedTableSync002
     * @tc.desc: Test sharedtable sync when version abnormal
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync002, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use set shared table
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);

        /**
         * @tc.steps:step2. insert local shared table records and sync
         * @tc.expected: step2. return OK
         */
        InsertLocalSharedTableRecords(0, 10, g_sharedTableName1);
        g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
            extend.erase(CloudDbConstant::VERSION_FIELD);
        });
        Query query = Query::Select().FromTable({ g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::OK);

        g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
            if (extend.find(CloudDbConstant::VERSION_FIELD) != extend.end()) {
                extend[CloudDbConstant::VERSION_FIELD] = "";
            }
        });
        query = Query::Select().FromTable({ g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::OK);
    }

    /**
     * @tc.name: SharedTableSync003
     * @tc.desc: Test weather the shared table is fill assetId under nochange
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: bty
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync003, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. insert local shared table records and sync
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        InsertLocalSharedTableRecords(0, 10, g_sharedTableName1); // 10 is records num
        Query query = Query::Select().FromTable({ g_sharedTableName1 });
        BlockSync(query, g_delegate);

        /**
         * @tc.steps:step2. sync again, cloud data containing assetId will be query
         * @tc.expected: step2. return OK
         */
        BlockSync(query, g_delegate);

        /**
         * @tc.steps:step3. check if the hash of assets in db is empty
         * @tc.expected: step3. OK
         */
        std::string sql = "SELECT asset from " + g_sharedTableName1;
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
            Type cloudValue;
            ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Asset>, 0, cloudValue), E_OK);
            std::vector<uint8_t> assetBlob;
            Asset asset;
            ASSERT_EQ(CloudStorageUtils::GetValueFromOneField(cloudValue, assetBlob), E_OK);
            ASSERT_EQ(RuntimeContext::GetInstance()->BlobToAsset(assetBlob, asset), E_OK);
            EXPECT_EQ(asset.assetId, "");
        }
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }

    /**
     * @tc.name: SharedTableSync004
     * @tc.desc: Test sharedtable sync when alter shared table name
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync004, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use set shared table
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1});

        /**
         * @tc.steps:step2. insert local shared table records and alter shared table name then sync
         * @tc.expected: step2. return OK
         */
        InsertLocalSharedTableRecords(0, 10, g_sharedTableName1);
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName5,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName5});
        Query query = Query::Select().FromTable({ g_sharedTableName5 });
        BlockSync(query, g_delegate);
        CheckCloudTableCount(g_sharedTableName5, 10);
    }

    /**
     * @tc.name: SharedTableSync006
     * @tc.desc: Test sharedtable sync when sharedtable add column
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync006, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. set shared table and sync
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);

        int localCount = 10;
        InsertLocalSharedTableRecords(0, localCount, g_sharedTableName1);
        Query query = Query::Select().FromTable({ g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::OK);
        CheckCloudTableCount(g_sharedTableName1, localCount);

        /**
         * @tc.steps:step2. add shared table column and sync
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField3
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        int cloudCount = 20;
        InsertLocalSharedTableRecords(localCount, cloudCount, g_sharedTableName1, true);
        BlockSync(query, g_delegate, DBStatus::OK);
        CheckCloudTableCount(g_sharedTableName1, cloudCount);
    }

    /**
     * @tc.name: SetCloudDbSchemaTest015
     * @tc.desc: Test SetCloudDbSchema sharedTableName is ""
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wangxiangdong
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest015, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. set sharedTableName ""
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = "",
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);

        /**
         * @tc.steps:step2. check sharedTable not exist
         * @tc.expected: step2. return OK
         */
        std::string sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND " \
            "name LIKE 'worker%_shared';";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    }

    /**
     * @tc.name: SetCloudDbSchemaTest016
     * @tc.desc: Test SetCloudDbSchema if it will delete sharedtable when set sharedtable is empty
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest016, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema to create g_sharedTableName1
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1});
        CheckDistributedSharedTable({g_distributedSharedTableName1});

        /**
         * @tc.steps:step2. re-SetCloudDbSchema and sharedtable name is empty
         * @tc.expected: step2. return INVALID_ARS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = "",
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        std::string sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND " \
            "name LIKE 'worker%_shared';";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));

        /**
         * @tc.steps:step3. re-SetCloudDbSchema and set sharedtable
         * @tc.expected: step3. return INVALID_ARS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = g_sharedTableName1,
            .fields = g_cloudField3
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName1});
        CheckDistributedSharedTable({g_distributedSharedTableName1});
    }

    /**
     * @tc.name: SetCloudDbSchemaTest017
     * @tc.desc: Test SetCloudDbSchema if table fields are less than old table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: chenchaohao
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SetCloudDbSchemaTest017, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. use SetCloudDbSchema
         * @tc.expected: step1. return OK
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName1,
            .sharedTableName = "",
            .fields = g_cloudField3
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);

        /**
         * @tc.steps:step2. re-SetCloudDbSchema and fields are less than old
         * @tc.expected: step2. return INVALID_ARS
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_tableName1,
            .sharedTableName = "",
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
    }

    /**
     * @tc.name: SharedTableSync007
     * @tc.desc: Test the sharing_resource for ins data into the cloud
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: bty
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync007, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. init cloud share data contains sharing_resource
         * @tc.expected: step1. return OK
         */
        InitCloudEnv();
        int cloudCount = 10;
        forkInsertFunc_ = InsertSharingUri;
        InsertCloudTableRecord(0, cloudCount, false);
        InsertCloudTableRecord(0, cloudCount);

        /**
         * @tc.steps:step2. sync
         * @tc.expected: step2. return OK
         */
        Query query = Query::Select().FromTable({ g_tableName2, g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::OK);
        forkInsertFunc_ = nullptr;

        /**
         * @tc.steps:step3. check sync insert
         * @tc.expected: step3. return OK
         */
        std::string sql = QueryResourceCountSql(g_tableName2);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
        sql = QueryResourceCountSql(g_sharedTableName1);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
    }

    /**
     * @tc.name: SharedTableSync008
     * @tc.desc: Test the sharing_resource for upd data into the cloud
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: bty
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync008, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. init cloud data and sync
         * @tc.expected: step1. return OK
         */
        InitCloudEnv();
        int cloudCount = 10;
        InsertCloudTableRecord(0, cloudCount, false);
        InsertCloudTableRecord(0, cloudCount);
        Query query = Query::Select().FromTable({ g_tableName2, g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::OK);

        /**
         * @tc.steps:step2. update cloud sharing_resource and sync
         * @tc.expected: step2. return OK
         */
        int beginGid = 0;
        forkInsertFunc_ = InsertSharingUri;
        InsertCloudTableRecord(0, cloudCount, false, beginGid);
        InsertCloudTableRecord(0, cloudCount, true, cloudCount);
        BlockSync(query, g_delegate, DBStatus::OK);
        forkInsertFunc_ = nullptr;

        /**
         * @tc.steps:step3. check sync update
         * @tc.expected: step3. return OK
         */
        std::string sql = QueryResourceCountSql(g_tableName2);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
        sql = QueryResourceCountSql(g_sharedTableName1);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);

        /**
         * @tc.steps:step4. remove cloud sharing_resource and sync
         * @tc.expected: step4. return OK
         */
        InsertCloudTableRecord(0, cloudCount, false, beginGid);
        InsertCloudTableRecord(0, cloudCount, true, cloudCount);
        BlockSync(query, g_delegate, DBStatus::OK);

        /**
         * @tc.steps:step5. check sync update
         * @tc.expected: step5. return OK
         */
        sql = QueryResourceCountSql(g_tableName2);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(0L), nullptr), SQLITE_OK);
        sql = QueryResourceCountSql(g_sharedTableName1);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(0L), nullptr), SQLITE_OK);
    }

    /**
     * @tc.name: SharedTableSync009
     * @tc.desc: Test the sharing_resource when the local data is newer than the cloud
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: bty
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync009, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. init cloud data and sync
         * @tc.expected: step1. return OK
         */
        InitCloudEnv();
        const std::vector<std::string> tables = { g_tableName2, g_sharedTableName1 };
        int cloudCount = 10;
        InsertCloudTableRecord(0, cloudCount, false);
        InsertCloudTableRecord(0, cloudCount);
        Query query = Query::Select().FromTable(tables);
        BlockSync(query, g_delegate, DBStatus::OK);

        /**
         * @tc.steps:step2. update cloud data, generate share uri
         * @tc.expected: step2. return OK
         */
        int beginGid = 0;
        forkInsertFunc_ = InsertSharingUri;
        InsertCloudTableRecord(0, cloudCount, false, beginGid);
        InsertCloudTableRecord(0, cloudCount, true, cloudCount);
        forkInsertFunc_ = nullptr;

        /**
         * @tc.steps:step3. update local data
         * @tc.expected: step3. return OK
         */
        for (const auto &tableName: tables) {
            std::string sql = "update " + tableName + " SET height='199';";
            sqlite3_stmt *stmt = nullptr;
            ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
            EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
            int errCode;
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }

        /**
         * @tc.steps:step4. sync and check count
         * @tc.expected: step4. return OK
         */
        BlockSync(query, g_delegate, DBStatus::OK);
        std::string sql = QueryResourceCountSql(g_tableName2);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
        sql = QueryResourceCountSql(g_sharedTableName1);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
        sql = "SELECT COUNT(*) FROM " + g_tableName2 + " where height='199'";
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
        sql = "SELECT COUNT(*) FROM " + g_sharedTableName1 + " where height='199'";
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
    }

    /**
     * @tc.name: SharedTableSync010
     * @tc.desc: Test the sharing_resource for del data into the cloud
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: bty
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync010, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. init cloud share data and sync
         * @tc.expected: step1. return OK
         */
        InitCloudEnv();
        int cloudCount = 10;
        forkInsertFunc_ = InsertSharingUri;
        InsertCloudTableRecord(0, cloudCount, false);
        InsertCloudTableRecord(0, cloudCount);
        Query query = Query::Select().FromTable({ g_tableName2, g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::OK);
        forkInsertFunc_ = nullptr;

        /**
         * @tc.steps:step2. delete cloud data
         * @tc.expected: step2. return OK
         */
        int beginGid = 0;
        int delCount = 5;
        DeleteCloudTableRecord(beginGid, delCount, false);
        DeleteCloudTableRecord(cloudCount, delCount);
        BlockSync(query, g_delegate, DBStatus::OK);

        /**
         * @tc.steps:step3. sync and check count
         * @tc.expected: step3. return OK
         */
        std::string sql = QueryResourceCountSql(g_tableName2);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount - delCount), nullptr), SQLITE_OK);
        sql = QueryResourceCountSql(g_sharedTableName1);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount - delCount), nullptr), SQLITE_OK);

        /**
         * @tc.steps:step4. remove device data and check count
         * @tc.expected: step4. return OK
         */
        g_delegate->RemoveDeviceData("", FLAG_ONLY);
        sql = QueryResourceCountSql(g_tableName2);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(0L), nullptr), SQLITE_OK);
        g_delegate->RemoveDeviceData("", CLEAR_SHARED_TABLE);
        sql = QueryResourceCountSql(g_sharedTableName1);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(0L), nullptr), SQLITE_OK);
    }

    /**
     * @tc.name: SharedTableSync011
     * @tc.desc: Test the sharing_resource when the local data is newer than the cloud
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: bty
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync011, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. init cloud data and sync
         * @tc.expected: step1. return OK
         */
        InitCloudEnv();
        const std::vector<std::string> tables = { g_tableName2, g_sharedTableName1 };
        int cloudCount = 10;
        InsertCloudTableRecord(0, cloudCount, false);
        InsertCloudTableRecord(0, cloudCount);
        Query query = Query::Select().FromTable(tables);
        BlockSync(query, g_delegate, DBStatus::OK);

        /**
         * @tc.steps:step2. update cloud data, generate share uri
         * @tc.expected: step2. return OK
         */
        int beginGid = 0;
        forkInsertFunc_ = InsertSharingUri;
        InsertCloudTableRecord(0, cloudCount, false, beginGid);
        InsertCloudTableRecord(0, cloudCount, true, cloudCount);
        forkInsertFunc_ = nullptr;

        /**
         * @tc.steps:step3. update local data
         * @tc.expected: step3. return OK
         */
        for (const auto &tableName: tables) {
            std::string sql = "update " + tableName + " SET height='199';";
            sqlite3_stmt *stmt = nullptr;
            ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
            EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
            int errCode;
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }

        /**
         * @tc.steps:step4. push sync and check count
         * @tc.expected: step4. return OK
         */
        BlockSync(query, g_delegate, DBStatus::OK, SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH);
        std::string sql = QueryResourceCountSql(g_tableName2);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
        sql = QueryResourceCountSql(g_sharedTableName1);
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
    }

    /**
     * @tc.name: SharedTableSync012
     * @tc.desc: Test the flag for uploaded data
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: bty
    */
    HWTEST_F(DistributedDBCloudInterfacesSetCloudSchemaTest, SharedTableSync012, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. init local share data and sync
         * @tc.expected: step1. return OK
         */
        InitCloudEnv();
        int cloudCount = 10;
        InsertLocalSharedTableRecords(0, 10, g_sharedTableName1);
        Query query = Query::Select().FromTable({ g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::OK);

        /**
         * @tc.steps:step2. update local share uri
         * @tc.expected: step2. return OK
         */
        std::string sql = "update " + DBCommon::GetLogTableName(g_sharedTableName1) + " SET sharing_resource='199';";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);

        /**
         * @tc.steps:step3. sync and check flag
         * @tc.expected: step3. return OK
         */
        BlockSync(query, g_delegate, DBStatus::OK);
        sql = "SELECT COUNT(*) FROM " + DBCommon::GetLogTableName(g_sharedTableName1) + " where flag&0x02=0x02";
        EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
    }
} // namespace