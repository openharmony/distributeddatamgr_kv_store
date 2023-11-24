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
#include "cloud_db_constant.h"
#include "cloud_db_types.h"
#include "db_common.h"
#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"
#include "sqlite_relational_utils.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"

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
    const string g_distributedSharedTableName4 = "naturalbase_rdb_aux_worker4_shared_log";
    std::string g_testDir;
    std::string g_dbDir;
    std::string g_storePath;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;
    std::shared_ptr<VirtualCloudDb> g_virtualCloudDb = nullptr;
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
    const std::vector<Field> g_cloudField4 = {
        {"id", TYPE_INDEX<int64_t>, true}, {"name", TYPE_INDEX<std::string>},
        {"height", TYPE_INDEX<double>}, {"photo", TYPE_INDEX<Bytes>},
        {"assets", TYPE_INDEX<Assets>}, {"age", TYPE_INDEX<int64_t>}
    };
    const int64_t g_syncWaitTime = 60;

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
        void InsertLocalSharedTableRecords(int64_t begin, int64_t count, const std::string &tableName);
        void BlockSync(const Query &query, RelationalStoreDelegate *delegate, DBStatus errCode = OK);
        void CheckCloudTableCount(const std::string &tableName, int64_t expectCount);
        void CloseDb();
        sqlite3 *db_ = nullptr;
    };

    void DistributedDBCloudInterfacesSetCloudSchemaTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        LOGD("Test dir is %s", g_testDir.c_str());
        g_dbDir = g_testDir + "/";
        g_storePath = g_dbDir + STORE_ID + DB_SUFFIX;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
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
        const std::string &tableName)
    {
        ASSERT_NE(db_, nullptr);
        for (int64_t i = begin; i < count; ++i) {
            string sql = "INSERT OR REPLACE INTO " + tableName +
                " (cloud_owner, cloud_privilege, id, name, height, married, photo) VALUES ('A', 'true', '" +
                std::to_string(i) + "', 'Local" + std::to_string(i) + "', '155.10', 'false', 'text');";
            ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
        }
    }

    void DistributedDBCloudInterfacesSetCloudSchemaTest::BlockSync(const Query &query,
        RelationalStoreDelegate *delegate, DBStatus errCode)
    {
        std::mutex dataMutex;
        std::condition_variable cv;
        bool finish = false;
        auto callback = [&cv, &dataMutex, &finish, &errCode](const std::map<std::string, SyncProcess> &process) {
            for (const auto &item: process) {
                if (item.second.process == DistributedDB::FINISHED) {
                    {
                        ASSERT_EQ(item.second.errCode, errCode);
                        std::lock_guard<std::mutex> autoLock(dataMutex);
                        finish = true;
                    }
                    cv.notify_one();
                }
            }
        };
        ASSERT_EQ(delegate->Sync({ "CLOUD" }, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), OK);
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
            .sharedTableName = "",
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
            .sharedTableName = "",
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
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
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
        CheckSharedTable({g_sharedTableName1, g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName1, g_distributedSharedTableName2});
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
         * @tc.expected: step2. return OK
         */
        dataBaseSchema.tables.clear();
        tableSchema = {
            .name = g_sharedTableName1,
            .sharedTableName = g_sharedTableName2,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
        CheckSharedTable({g_sharedTableName2});
        CheckDistributedSharedTable({g_distributedSharedTableName2});
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
         * @tc.steps:step1. local create table worker1
         * @tc.expected: step1. return OK
         */
        ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName1, CLOUD_COOPERATION), DBStatus::OK);

        /**
         * @tc.steps:step2. local exists worker1 then use SetCloudDbSchema
         * @tc.expected: step2. return INVALID_ARGS
         */
        DataBaseSchema dataBaseSchema;
        TableSchema tableSchema = {
            .name = g_tableName2,
            .sharedTableName = g_tableName1,
            .fields = g_cloudField1
        };
        dataBaseSchema.tables.push_back(tableSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::INVALID_ARGS);
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
        BlockSync(query, g_delegate, DBStatus::CLOUD_ERROR);

        g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
            if (extend.find(CloudDbConstant::VERSION_FIELD) != extend.end()) {
                extend[CloudDbConstant::VERSION_FIELD] = "";
            }
        });
        query = Query::Select().FromTable({ g_sharedTableName1 });
        BlockSync(query, g_delegate, DBStatus::CLOUD_ERROR);
    }
} // namespace