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
#include <iostream>
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"
#include "distributeddb_data_generate_unit_test.h"
#include "relational_store_instance.h"
#include "sqlite_relational_store.h"
#include "store_observer.h"
#include "log_table_manager_factory.h"
#include "cloud_db_constant.h"
#include "virtual_cloud_db.h"
#include "time_helper.h"
#include "runtime_config.h"
#include "virtual_cloud_data_translate.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_storeID = "Relational_Store_SYNC";
    const string g_tableName1 = "worker1";
    const string g_tableName2 = "worker2";
    const string g_tableName3 = "worker3";
    const string DEVICE_CLOUD = "cloud";
    const string DB_SUFFIX = ".db";
    const int64_t g_syncWaitTime = 10;
    string g_testDir;
    string g_storePath;
    std::mutex g_processMutex;
    std::condition_variable g_processCondition;
    std::shared_ptr<VirtualCloudDb> g_virtualCloudDb;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreObserverUnitTest *g_observer = nullptr;
    using CloudSyncStatusCallback = std::function<void(const std::map<std::string, SyncProcess> &onProcess)>;
    const std::string CREATE_LOCAL_TABLE_SQL =
            "CREATE TABLE IF NOT EXISTS " + g_tableName1 + "(" \
    "name TEXT PRIMARY KEY," \
    "height REAL ," \
    "married BOOLEAN ," \
    "photo BLOB NOT NULL," \
    "assert BLOB," \
    "age INT);";
    const std::string INTEGER_PRIMARY_KEY_TABLE_SQL =
            "CREATE TABLE IF NOT EXISTS " + g_tableName2 + "(" \
    "id INTEGER PRIMARY KEY," \
    "name TEXT ," \
    "height REAL ," \
    "photo BLOB ," \
    "asserts BLOB," \
    "age INT);";
    const std::string CREATE_LOCAL_TABLE_WITHOUT_PRIMARY_KEY_SQL =
            "CREATE TABLE IF NOT EXISTS " + g_tableName3 + "(" \
    "name TEXT," \
    "height REAL ," \
    "married BOOLEAN ," \
    "photo BLOB NOT NULL," \
    "assert BLOB," \
    "age INT);";
    const std::vector<Field> g_cloudFiled1 = {
        {"name", TYPE_INDEX<std::string>, true}, {"height", TYPE_INDEX<double>},
        {"married", TYPE_INDEX<bool>}, {"photo", TYPE_INDEX<Bytes>, false, false},
        {"assert", TYPE_INDEX<Asset>}, {"age", TYPE_INDEX<int64_t>}
    };
    const std::vector<Field> g_invalidCloudFiled1 = {
        {"name", TYPE_INDEX<std::string>, true}, {"height", TYPE_INDEX<int>},
        {"married", TYPE_INDEX<bool>}, {"photo", TYPE_INDEX<Bytes>, false, false},
        {"assert", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
    };
    const std::vector<Field> g_cloudFiled2 = {
        {"id", TYPE_INDEX<int64_t>, true}, {"name", TYPE_INDEX<std::string>},
        {"height", TYPE_INDEX<double>},  {"photo", TYPE_INDEX<Bytes>},
        {"asserts", TYPE_INDEX<Assets>}, {"age", TYPE_INDEX<int64_t>}
    };
    const std::vector<Field> g_cloudFiledWithOutPrimaryKey3 = {
        {"name", TYPE_INDEX<std::string>, false, true}, {"height", TYPE_INDEX<double>},
        {"married", TYPE_INDEX<bool>}, {"photo", TYPE_INDEX<Bytes>, false, false},
        {"assert", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
    };
    const std::vector<std::string> g_tables = {g_tableName1, g_tableName2};
    const std::vector<std::string> g_tablesPKey = {g_cloudFiled1[0].colName, g_cloudFiled2[0].colName};
    const std::vector<string> g_prefix = {"Local", ""};
    const Asset g_localAsset = {
        .version = 1, .name = "Phone", .uri = "/local/sync", .modifyTime = "123456", .createTime = "",
        .size = "256", .hash = " "
    };
    const Asset g_cloudAsset = {
        .version = 2, .name = "CloudPhone", .uri = "/cloud/sync",.modifyTime = "123456",
        .createTime = "0", .size = "1024", .hash = "A56"
    };

    void CreateUserDBAndTable(sqlite3 *&db)
    {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_LOCAL_TABLE_SQL), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, INTEGER_PRIMARY_KEY_TABLE_SQL), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_LOCAL_TABLE_WITHOUT_PRIMARY_KEY_SQL), SQLITE_OK);
    }

    void InsertUserTableRecord(sqlite3 *&db, int64_t begin, int64_t count, int64_t photoSize)
    {
        std::string photo(photoSize, 'v');
        std::vector<uint8_t> assetBlob;
        RuntimeContext::GetInstance()->AssetToBlob(g_localAsset, assetBlob);
        int errCode;
        for (int64_t i = begin; i < count; ++i) {
            string sql = "INSERT OR REPLACE INTO " + g_tableName1
                         + " (name, height, married, photo, assert, age) VALUES ('Local" + std::to_string(i) +
                         "', '175.8', '0', '" + photo + "', ? , '18');";
            sqlite3_stmt *stmt = nullptr;
            ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
            if (SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false) != E_OK) {
                SQLiteUtils::ResetStatement(stmt, true, errCode);
            }
            EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }
        std::vector<Asset> assets;
        assets.push_back(g_localAsset);
        assets.push_back(g_localAsset);
        RuntimeContext::GetInstance()->AssetsToBlob(assets, assetBlob);
        for (int64_t i = begin; i < count; ++i) {
            string sql = "INSERT OR REPLACE INTO " + g_tableName2
                         + " (id, name, height, photo, asserts, age) VALUES ('" + std::to_string(i) + "', 'Local"
                         + std::to_string(i) + "', '155.10', '"+ photo + "',  ? , '21');";
            sqlite3_stmt *stmt = nullptr;
            ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
            if (SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false) != E_OK) {
                SQLiteUtils::ResetStatement(stmt, true, errCode);
            }
            EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }
        LOGD("insert user record worker1[primary key]:[local%d - cloud%d) , worker2[primary key]:[%d - %d)",
            begin, count, begin, count);
    }

    void UpdateUserTableRecord(sqlite3 *&db, int64_t begin, int64_t count)
    {
        for (size_t i = 0; i < g_tables.size(); i++) {
            string updateAge = "UPDATE " + g_tables[i] + " SET age = '99' where " + g_tablesPKey[i]
                               + " in (";
            for (int64_t j = begin; j < begin + count; ++j) {
                updateAge += "'" + g_prefix[i] + std::to_string(j) + "',";
            }
            updateAge.pop_back();
            updateAge += ");";
            ASSERT_EQ(RelationalTestUtils::ExecSql(db, updateAge), SQLITE_OK);
        }
        LOGD("update local record worker1[primary key]:[local%d - local%d) , worker2[primary key]:[%d - %d)",
            begin, count, begin, count);
    }

    void DeleteUserTableRecord(sqlite3 *&db, int64_t begin, int64_t count)
    {
        for (size_t i = 0; i < g_tables.size(); i++) {
            string updateAge = "Delete from " + g_tables[i] + " where " + g_tablesPKey[i]
                               + " in (";
            for (int64_t j = begin; j < count; ++j) {
                updateAge += "'" + g_prefix[i] + std::to_string(j) + "',";
            }
            updateAge.pop_back();
            updateAge += ");";
            ASSERT_EQ(RelationalTestUtils::ExecSql(db, updateAge), SQLITE_OK);
        }
        LOGD("delete local record worker1[primary key]:[local%d - local%d) , worker2[primary key]:[%d - %d)",
             begin, count, begin, count);
    }

    void InsertRecordWithoutPk2LocalAndCloud(sqlite3 *&db, int64_t begin, int64_t count, int photoSize)
    {
        std::vector<uint8_t> photo(photoSize, 'v');
        std::string photoLocal(photoSize, 'v');
        Asset asset = { .version = 1, .name = "Phone" };
        std::vector<uint8_t> assetBlob;
        RuntimeContext::GetInstance()->BlobToAsset(assetBlob, asset);
        std::string assetStr(assetBlob.begin(), assetBlob.end());
        std::vector<VBucket> record1;
        std::vector<VBucket> extend1;
        for (int64_t i = begin; i < count; ++i) {
            Timestamp now = TimeHelper::GetSysCurrentTime();
            VBucket data;
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", 166.0); // 166.0 is random double value
            data.insert_or_assign("married", (bool)0);
            data.insert_or_assign("photo", photo);
            data.insert_or_assign("assert", KEY_1);
            data.insert_or_assign("age", 13L);
            record1.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now);
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now);
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            extend1.push_back(log);
        }
        int errCode = g_virtualCloudDb->BatchInsert(g_tableName3, std::move(record1), extend1);
        ASSERT_EQ(errCode, DBStatus::OK);
        for (int64_t i = begin; i < count; ++i) {
            string sql = "INSERT OR REPLACE INTO " + g_tableName3
                         + " (name, height, married, photo, assert, age) VALUES ('Local" + std::to_string(i) +
                         "', '175.8', '0', '" + photoLocal + "', '" + assetStr + "', '18');";
            ASSERT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        }
    }

    void InsertCloudTableRecord(int64_t begin, int64_t count, int64_t  photoSize)
    {
        std::vector<uint8_t> photo(photoSize, 'v');
        std::vector<VBucket> record1;
        std::vector<VBucket> extend1;
        for (int64_t i = begin; i < count; ++i) {
            Timestamp now = TimeHelper::GetSysCurrentTime();
            VBucket data;
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", 166.0); // 166.0 is random double value
            data.insert_or_assign("married", false);
            data.insert_or_assign("photo", photo);
            data.insert_or_assign("assert", g_cloudAsset);
            data.insert_or_assign("age", 13L);
            record1.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now);
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now);
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            extend1.push_back(log);
        }
        int errCode = g_virtualCloudDb->BatchInsert(g_tableName1, std::move(record1), extend1);
        ASSERT_EQ(errCode, DBStatus::OK);

        std::vector<VBucket> record2;
        std::vector<VBucket> extend2;
        std::vector<Asset> assets;
        assets.push_back(g_cloudAsset);
        assets.push_back(g_cloudAsset);
        for (int64_t i = begin; i < count; ++i) {
            Timestamp now = TimeHelper::GetSysCurrentTime();
            VBucket data;
            data.insert_or_assign("id", i);
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", 180.3); // 180.3 is random double value
            data.insert_or_assign("photo", photo);
            data.insert_or_assign("asserts", assets);
            data.insert_or_assign("age", 28L);
            record2.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now);
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now);
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            extend2.push_back(log);
        }
        errCode = g_virtualCloudDb->BatchInsert(g_tableName2, std::move(record2), extend2);
        ASSERT_EQ(errCode, DBStatus::OK);
        LOGD("insert cloud record worker1[primary key]:[cloud%d - cloud%d) , worker2[primary key]:[%d - %d)",
            begin, count, begin, count);
    }

    int QueryCountCallback(void *data, int count, char **colValue, char **colName)
    {
        if (count != 1) {
            return 0;
        }
        auto expectCount = reinterpret_cast<int64_t>(data);
        EXPECT_EQ(strtol(colValue[0], nullptr, 10), expectCount);
        return 0;
    }

    void CheckDownloadResult(sqlite3 *&db, std::vector<int64_t> expectCounts)
    {
        for (size_t i = 0; i < g_tables.size(); ++i) {
            string queryDownload = "select count(*) from " + g_tables[i] + " where name "
                                   + " like 'Cloud%'";
            EXPECT_EQ(sqlite3_exec(db, queryDownload.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(expectCounts[i]), nullptr), SQLITE_OK);
        }
    }

    void CheckCloudTotalCount(std::vector<int64_t> expectCounts)
    {
        VBucket extend;
        extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
        for (size_t i = 0; i < g_tables.size(); ++i) {
            int64_t realCount = 0;
            std::vector<VBucket> data;
            g_virtualCloudDb->Query(g_tables[i], extend, data);
            for (size_t j = 0; j < data.size(); ++j) {
                auto entry = data[j].find(CloudDbConstant::DELETE_FIELD);
                if (entry != data[j].end() && std::get<bool>(entry->second)) {
                    continue;
                }
                realCount++;
            }
            EXPECT_EQ(realCount, expectCounts[i]); // ExpectCount represents the total amount of cloud data.
        }
    }

    void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
    {
        TableSchema tableSchema1 = {
            .name = g_tableName1,
            .fields = g_cloudFiled1
        };
        TableSchema tableSchema2 = {
            .name = g_tableName2,
            .fields = g_cloudFiled2
        };
        TableSchema tableSchemaWithOutPrimaryKey = {
            .name = g_tableName3,
            .fields = g_cloudFiledWithOutPrimaryKey3
        };
        dataBaseSchema.tables.push_back(tableSchema1);
        dataBaseSchema.tables.push_back(tableSchema2);
        dataBaseSchema.tables.push_back(tableSchemaWithOutPrimaryKey);
    }


    void GetInvalidCloudDbSchema(DataBaseSchema &dataBaseSchema)
    {
        TableSchema tableSchema1 = {
            .name = g_tableName1,
            .fields = g_invalidCloudFiled1
        };
        TableSchema tableSchema2 = {
            .name = g_tableName1,
            .fields = g_cloudFiled2
        };
        dataBaseSchema.tables.push_back(tableSchema1);
        dataBaseSchema.tables.push_back(tableSchema2);
    }

    void GetCallback(SyncProcess &syncProcess, CloudSyncStatusCallback &callback)
    {
        callback = [&syncProcess](const std::map<std::string, SyncProcess> &process) {
            LOGI("devices size = %d", process.size());
            ASSERT_EQ(process.size(), 1u);
            syncProcess = std::move(process.begin()->second);
            ASSERT_EQ(process.begin()->first, DEVICE_CLOUD);
            LOGI("current sync process status:%d, db status:%d ", syncProcess.process, syncProcess.errCode);
            if (syncProcess.tableProcess.empty()) {
                LOGI("sync process tableProcess is empty");
                return;
            }
            std::for_each(g_tables.begin(), g_tables.end(), [&](const auto &item) {
                auto table1 = syncProcess.tableProcess.find(item);
                if (table1 != syncProcess.tableProcess.end()) {
                    LOGI("table[%s], sync process status:%d, [downloadInfo](batchIndex:%u, total:%u, successCount:%u, "
                         "failCount:%u) [uploadInfo](batchIndex:%u, total:%u, successCount:%u,failCount:%u",
                         item.c_str(), table1->second.process, table1->second.downLoadInfo.batchIndex,
                         table1->second.downLoadInfo.total, table1->second.downLoadInfo.successCount,
                         table1->second.downLoadInfo.failCount, table1->second.upLoadInfo.batchIndex,
                         table1->second.upLoadInfo.total, table1->second.upLoadInfo.successCount,
                         table1->second.upLoadInfo.failCount);
                }
            });
            if (syncProcess.process == FINISHED) {
                g_processCondition.notify_one();
            }
        };
    }

    void WaitForSyncFinish(SyncProcess &syncProcess, const int64_t &waitTime)
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(waitTime), [&syncProcess]() {
            return syncProcess.process == FINISHED;
        });
        ASSERT_EQ(result, true);
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

    class MockPrintChangedDataStoreObserver : public DistributedDB::StoreObserver {
        void OnChange(Origin origin, const std::string &originalId, ChangedData &&data) {
            LOGW("changedData tableName [%s]", data.tableName.c_str());
            if (data.tableName == g_tableName1) {
                for (const auto &fieldName : data.field) {
                    LOGW("fieldName [%s]", fieldName.c_str());
                }
                for (uint64_t i = ChangeType::OP_INSERT; i < ChangeType::OP_BUTT; ++i) {
                    for (size_t j = 0; j < data.primaryData[i].size(); j++) {
                        for (size_t k = 0; k < data.primaryData[i][j].size(); k++) {
                            LOGW("tableName1 ID %s", std::get<std::string>(data.primaryData[i][j][k]).c_str());
                        }
                    }
                }
            }
            if (data.tableName == g_tableName2) {
                for (const auto &fieldName : data.field) {
                    LOGW("fieldName [%s]", fieldName.c_str());
                }
                for (uint64_t i = ChangeType::OP_INSERT; i < ChangeType::OP_BUTT; ++i) {
                    for (size_t j = 0; j < data.primaryData[i].size(); j++) {
                        for (size_t k = 0; k < data.primaryData[i][j].size(); k++) {
                            LOGW("tableName1 ID %d", std::get<std::int64_t>(data.primaryData[i][j][k]));
                        }
                    }
                }
            }
        };
    };


    class DistributedDBCloudInterfacesRelationalSyncTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    protected:
        sqlite3 *db = nullptr;
        RelationalStoreDelegate *delegate = nullptr;
    };


    void DistributedDBCloudInterfacesRelationalSyncTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        g_storePath = g_testDir + "/" + g_storeID + DB_SUFFIX;
        LOGI("The test db is:%s", g_testDir.c_str());
        RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
    }

    void DistributedDBCloudInterfacesRelationalSyncTest::TearDownTestCase(void)
    {}

    void DistributedDBCloudInterfacesRelationalSyncTest::SetUp(void)
    {
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
            LOGE("rm test db files error.");
        }
        DistributedDBToolsUnitTest::PrintTestCaseInfo();
        LOGD("Test dir is %s", g_testDir.c_str());
        db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        CreateUserDBAndTable(db);

        g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        ASSERT_NE(g_observer, nullptr);
        ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option { .observer = g_observer },
            delegate), DBStatus::OK);
        ASSERT_NE(delegate, nullptr);
        ASSERT_EQ(delegate->CreateDistributedTable(g_tableName1, CLOUD_COOPERATION), DBStatus::OK);
        ASSERT_EQ(delegate->CreateDistributedTable(g_tableName2, CLOUD_COOPERATION), DBStatus::OK);
        ASSERT_EQ(delegate->CreateDistributedTable(g_tableName3, CLOUD_COOPERATION), DBStatus::OK);
        g_virtualCloudDb = make_shared<VirtualCloudDb>();
        ASSERT_EQ(delegate->SetCloudDB(g_virtualCloudDb), DBStatus::OK);
        // sync before setting cloud db schema,it should return SCHEMA_NOT_SET
        Query query = Query::Select().FromTable(g_tables);
        CloudSyncStatusCallback callback;
        ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime),
            DBStatus::SCHEMA_MISMATCH);
        DataBaseSchema dataBaseSchema;
        GetCloudDbSchema(dataBaseSchema);
        ASSERT_EQ(delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    }

    void DistributedDBCloudInterfacesRelationalSyncTest::TearDown(void)
    {
        g_virtualCloudDb = nullptr;
        if (delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(delegate), DBStatus::OK);
            delegate = nullptr;
        }
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
            LOGE("rm test db files error.");
        }
    }

/**
 * @tc.name: CloudSyncTest001
 * @tc.desc: Cloud data is older than local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, CloudSyncTest001, TestSize.Level0)
{
    int64_t paddingSize = 10;
    ChangedData changedDataForTable1;
    ChangedData changedDataForTable2;
    changedDataForTable1.tableName = g_tableName1;
    changedDataForTable2.tableName = g_tableName2;
    changedDataForTable1.field.push_back(std::string("name"));
    changedDataForTable2.field.push_back(std::string("id"));
    for (int i = 0; i < 20; i++) {
        changedDataForTable1.primaryData[ChangeType::OP_INSERT].push_back({"Cloud" + std::to_string(i)});
        changedDataForTable2.primaryData[ChangeType::OP_INSERT].push_back({(int64_t)i + 10});
    }
    g_observer->SetExpectedResult(changedDataForTable1);
    g_observer->SetExpectedResult(changedDataForTable2);
    InsertCloudTableRecord(0, 20, paddingSize);
    InsertUserTableRecord(db, 0, 10, paddingSize);
    Query query = Query::Select().FromTable(g_tables);
    SyncProcess syncProcess;
    CloudSyncStatusCallback callback;
    GetCallback(syncProcess, callback);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    LOGD("expect download:worker1[primary key]:[cloud0 - cloud20), worker2[primary key]:[10 - 20)");
    CheckDownloadResult(db, {20L, 10L});
    LOGD("expect upload:worker1[primary key]:[local0 - local10), worker2[primary key]:[0 - 10)");
    CheckCloudTotalCount({30L, 20L});
}

/**
 * @tc.name: CloudSyncTest002
 * @tc.desc: Local data is older than cloud data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, CloudSyncTest002, TestSize.Level0)
{
    int64_t paddingSize = 100;
    InsertUserTableRecord(db, 0, 20, paddingSize);
    InsertCloudTableRecord(0, 10, paddingSize);
    Query query = Query::Select().FromTable(g_tables);
    SyncProcess syncProcess;
    CloudSyncStatusCallback callback;
    GetCallback(syncProcess, callback);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);
    LOGD("expect download:worker1[primary key]:[cloud0 - cloud10), worker2[primary key]:[0 - 10)");
    CheckDownloadResult(db, {10L, 10L});
    LOGD("expect upload:worker1[primary key]:[local0 - local20), worker2[primary key]:[10 - 20)");
    CheckCloudTotalCount({30L, 20L});
}

/**
 * @tc.name: CloudSyncTest003
 * @tc.desc: test with update and delete operator
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, CloudSyncTest003, TestSize.Level0)
{
    int64_t paddingSize = 10;
    InsertCloudTableRecord(0, 20, paddingSize);
    InsertUserTableRecord(db, 0, 20, paddingSize);
    Query query = Query::Select().FromTable(g_tables);
    SyncProcess syncProcess;
    CloudSyncStatusCallback callback;
    GetCallback(syncProcess, callback);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);
    LOGD("expect download:worker1[primary key]:[cloud0 - cloud20), worker2[primary key]:none");
    CheckDownloadResult(db, {20L, 0L});
    LOGD("expect upload:worker1[primary key]:[local0 - local20), worker2[primary key]:[0 - 20)");
    CheckCloudTotalCount({40L, 20L});

    UpdateUserTableRecord(db, 5, 10);
    syncProcess = {};
    LOGD("sync after update-----------------------");
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);
    LOGD("sync finish-----------------------");

    LOGD("expect get result upload worker1[primary key]:[local5 - local15)");
    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
    std::vector<VBucket> data1;
    g_virtualCloudDb->Query(g_tables[0], extend, data1);
    for (int j = 25; j < 35; ++j) {
        EXPECT_EQ(std::get<int64_t>(data1[j]["age"]), 99);
    }

    LOGD("expect get result upload worker2[primary key]:[5 - 15)");
    std::vector<VBucket> data2;
    g_virtualCloudDb->Query(g_tables[1], extend, data2);
    for (int j = 5; j < 15; ++j) {
        EXPECT_EQ(std::get<int64_t>(data2[j]["age"]), 99);
    }

    DeleteUserTableRecord(db, 0, 3);
    syncProcess = {};

    LOGD("sync after delete-----------------------");
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);
    LOGD("sync finish-----------------------");

    LOGD("expect upload:worker1[primary key]:[local0 - local3), worker2[primary key]:[0 - 3)");
    CheckCloudTotalCount({37L, 17L});
}

/**
 * @tc.name: CloudSyncTest004
 * @tc.desc: Random write of local and cloud data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, CloudSyncTest004, TestSize.Level0)
{
    vector<thread> threads;
    threads.emplace_back(InsertCloudTableRecord, 0, 30, 1024);
    threads.emplace_back(InsertUserTableRecord, std::ref(db), 0, 30, 1024);
    for (auto &thread: threads) {
        thread.join();
    }
    Query query = Query::Select().FromTable(g_tables);
    int64_t waitTime = 30;
    SyncProcess syncProcess;
    CloudSyncStatusCallback callback;
    GetCallback(syncProcess, callback);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, waitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, waitTime);
}

/**
 * @tc.name: CloudSyncTest005
 * @tc.desc: sync with device sync query
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, CloudSyncTest005, TestSize.Level0)
{
    Query query = Query::Select().FromTable(g_tables).OrderBy("123", true);
    int64_t waitTime = 30;
    SyncProcess syncProcess;
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, nullptr, waitTime), DBStatus::NOT_SUPPORT);
}

/**
 * @tc.name: CloudSyncTest006
 * @tc.desc: Firstly set a correct schema, and then null or invalid schema
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wanyi
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, CloudSyncTest006, TestSize.Level0)
{
    int64_t paddingSize = 10;
    ChangedData changedDataForTable1;
    ChangedData changedDataForTable2;
    changedDataForTable1.tableName = g_tableName1;
    changedDataForTable2.tableName = g_tableName2;
    changedDataForTable1.field.push_back(std::string("name"));
    changedDataForTable2.field.push_back(std::string("id"));
    for (int i = 0; i < 20; i++) {
        changedDataForTable1.primaryData[ChangeType::OP_INSERT].push_back({"Cloud" + std::to_string(i)});
        changedDataForTable2.primaryData[ChangeType::OP_INSERT].push_back({(int64_t)i + 10});
    }
    g_observer->SetExpectedResult(changedDataForTable1);
    g_observer->SetExpectedResult(changedDataForTable2);
    InsertCloudTableRecord(0, 20, paddingSize);
    InsertUserTableRecord(db, 0, 10, paddingSize);
    Query query = Query::Select().FromTable(g_tables);
    SyncProcess syncProcess;
    CloudSyncStatusCallback callback;
    GetCallback(syncProcess, callback);
    // Set correct cloudDbSchema (correct version)
    DataBaseSchema correctSchema;
    GetCloudDbSchema(correctSchema);
    ASSERT_EQ(delegate->SetCloudDbSchema(correctSchema), DBStatus::OK);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();
    LOGD("expect download:worker1[primary key]:[cloud0 - cloud20), worker2[primary key]:[10 - 20)");
    CheckDownloadResult(db, {20L, 10L});
    LOGD("expect upload:worker1[primary key]:[local0 - local10), worker2[primary key]:[0 - 10)");
    CheckCloudTotalCount({30L, 20L});

    // Reset cloudDbSchema (invalid version - null)
    DataBaseSchema nullSchema;
    ASSERT_EQ(delegate->SetCloudDbSchema(nullSchema), DBStatus::OK);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime),
        DBStatus::SCHEMA_MISMATCH);

    // Reset cloudDbSchema (invalid version - field mismatch)
    DataBaseSchema invalidSchema;
    GetInvalidCloudDbSchema(invalidSchema);
    ASSERT_EQ(delegate->SetCloudDbSchema(invalidSchema), DBStatus::OK);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime),
        DBStatus::SCHEMA_MISMATCH);
}

/**
 * @tc.name: CloudSyncTest007
 * @tc.desc: Check the asset types after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, CloudSyncTest007, TestSize.Level1)
{
    int64_t paddingSize = 100;
    InsertUserTableRecord(db, 0, 20, paddingSize);
    InsertCloudTableRecord(0, 10, paddingSize);
    Query query = Query::Select().FromTable(g_tables);
    SyncProcess syncProcess;
    CloudSyncStatusCallback callback;
    GetCallback(syncProcess, callback);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);

    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
    std::vector<VBucket> data1;
    g_virtualCloudDb->Query(g_tables[0], extend, data1);
    for (size_t j = 0; j < data1.size(); ++j) {
        auto entry = data1[j].find("assert");
        ASSERT_NE(entry, data1[j].end());
        Asset asset = std::get<Asset>(entry->second);
        Asset baseAsset = j >= 10 ? g_localAsset : g_cloudAsset;
        EXPECT_EQ(asset.version, baseAsset.version);
        EXPECT_EQ(asset.name, baseAsset.name);
        EXPECT_EQ(asset.uri, baseAsset.uri);
        EXPECT_EQ(asset.modifyTime, baseAsset.modifyTime);
        EXPECT_EQ(asset.createTime, baseAsset.createTime);
        EXPECT_EQ(asset.size, baseAsset.size);
        EXPECT_EQ(asset.hash, baseAsset.hash);
    }

    std::vector<VBucket> data2;
    g_virtualCloudDb->Query(g_tables[1], extend, data2);
    for (size_t j = 0; j < data2.size(); ++j) {
        auto entry = data2[j].find("asserts");
        ASSERT_NE(entry, data2[j].end());
        Assets assets = std::get<Assets>(entry->second);
        Asset baseAsset = j >= 10 ? g_localAsset : g_cloudAsset;
        for (const auto &asset: assets) {
            EXPECT_EQ(asset.version, baseAsset.version);
            EXPECT_EQ(asset.name, baseAsset.name);
            EXPECT_EQ(asset.uri, baseAsset.uri);
            EXPECT_EQ(asset.modifyTime, baseAsset.modifyTime);
            EXPECT_EQ(asset.createTime, baseAsset.createTime);
            EXPECT_EQ(asset.size, baseAsset.size);
            EXPECT_EQ(asset.hash, baseAsset.hash);
        }
    }
}

/*
 * @tc.name: CloudSyncTest005
 * @tc.desc: Notify data without primary key
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalSyncTest, DataNotifier001, TestSize.Level0)
{
    InsertRecordWithoutPk2LocalAndCloud(db, 0, 20, 10);
    Query query = Query::Select().FromTable({g_tableName3});
    SyncProcess syncProcess;
    CloudSyncStatusCallback callback;
    GetCallback(syncProcess, callback);
    ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), DBStatus::OK);
    WaitForSyncFinish(syncProcess, g_syncWaitTime);
}

}
#endif // RELATIONAL_STORE