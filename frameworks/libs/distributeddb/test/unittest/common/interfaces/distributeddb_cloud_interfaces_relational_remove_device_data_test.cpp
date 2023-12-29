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
#include "cloud/cloud_storage_utils.h"
#include "cloud/cloud_db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "process_system_api_adapter_impl.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "sqlite_relational_store.h"
#include "sqlite_relational_utils.h"
#include "store_observer.h"
#include "time_helper.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"
#include "mock_asset_loader.h"
#include "cloud_db_sync_utils_test.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_storeID = "Relational_Store_SYNC";
    const string g_tableName1 = "worker1";
    const string g_tableName2 = "worker2";
    const string g_tableName3 = "worker3";
    const string g_tableName4 = "worker4";
    const string DEVICE_CLOUD = "cloud_dev";
    const string DB_SUFFIX = ".db";
    const int64_t g_syncWaitTime = 60;
    const int g_arrayHalfSub = 2;
    int g_syncIndex = 0;
    string g_testDir;
    string g_storePath;
    std::mutex g_processMutex;
    std::condition_variable g_processCondition;
    std::shared_ptr<VirtualCloudDb> g_virtualCloudDb;
    std::shared_ptr<VirtualAssetLoader> g_virtualAssetLoader;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreObserverUnitTest *g_observer = nullptr;
    RelationalStoreDelegate *g_delegate = nullptr;
    SyncProcess g_syncProcess;
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
    const std::string DROP_INTEGER_PRIMARY_KEY_TABLE_SQL = "DROP TABLE " + g_tableName2 + ";";
    const std::string CREATE_LOCAL_TABLE_WITHOUT_PRIMARY_KEY_SQL =
            "CREATE TABLE IF NOT EXISTS " + g_tableName3 + "(" \
    "name TEXT," \
    "height REAL ," \
    "married BOOLEAN ," \
    "photo BLOB NOT NULL," \
    "assert BLOB," \
    "age INT);";
    const std::string INTEGER_PRIMARY_KEY_TABLE_SQL_WRONG_SYNC_MODE =
            "CREATE TABLE IF NOT EXISTS " + g_tableName4 + "(" \
    "id INTEGER PRIMARY KEY," \
    "name TEXT ," \
    "height REAL ," \
    "photo BLOB ," \
    "asserts BLOB," \
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
    const std::vector<std::string> g_shareTables = {g_tableName1 + CloudDbConstant::SHARED,
        g_tableName2 + CloudDbConstant::SHARED};
    const std::vector<std::string> g_tablesPKey = {g_cloudFiled1[0].colName, g_cloudFiled2[0].colName};
    const std::vector<string> g_prefix = {"Local", ""};
    const Asset g_localAsset = {
        .version = 1, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/local/sync",
        .modifyTime = "123456", .createTime = "", .size = "256", .hash = "ASE"
    };
    const Asset g_cloudAsset = {
        .version = 2, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/cloud/sync",
        .modifyTime = "123456", .createTime = "0", .size = "1024", .hash = "DEC"
    };

    void CreateUserDBAndTable(sqlite3 *&db)
    {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_LOCAL_TABLE_SQL), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, INTEGER_PRIMARY_KEY_TABLE_SQL), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_LOCAL_TABLE_WITHOUT_PRIMARY_KEY_SQL), SQLITE_OK);
    }

    void InsertUserTableRecord(sqlite3 *&db, int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull)
    {
        std::string photo(photoSize, 'v');
        int errCode;
        std::vector<uint8_t> assetBlob;
        for (int64_t i = begin; i < begin + count; ++i) {
            Asset asset = g_localAsset;
            asset.name = asset.name + std::to_string(i);
            RuntimeContext::GetInstance()->AssetToBlob(asset, assetBlob);
            string sql = "INSERT OR REPLACE INTO " + g_tableName1
                         + " (name, height, married, photo, assert, age) VALUES ('Local" + std::to_string(i) +
                         "', '175.8', '0', '" + photo + "', ? , '18');";
            sqlite3_stmt *stmt = nullptr;
            ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
            if (assetIsNull) {
                ASSERT_EQ(sqlite3_bind_null(stmt, 1), SQLITE_OK);
            } else {
                ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK);
            }
            EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }
        for (int64_t i = begin; i < begin + count; ++i) {
            std::vector<Asset> assets;
            Asset asset = g_localAsset;
            asset.name = g_localAsset.name + std::to_string(i);
            assets.push_back(asset);
            asset.name = g_localAsset.name + std::to_string(i + 1);
            assets.push_back(asset);
            RuntimeContext::GetInstance()->AssetsToBlob(assets, assetBlob);
            string sql = "INSERT OR REPLACE INTO " + g_tableName2
                         + " (id, name, height, photo, asserts, age) VALUES ('" + std::to_string(i) + "', 'Local"
                         + std::to_string(i) + "', '155.10', '"+ photo + "',  ? , '21');";
            sqlite3_stmt *stmt = nullptr;
            ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
            if (assetIsNull) {
                ASSERT_EQ(sqlite3_bind_null(stmt, 1), E_OK);
            } else {
                ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK);
            }
            EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
            SQLiteUtils::ResetStatement(stmt, true, errCode);
        }
        LOGD("insert user record worker1[primary key]:[Local%" PRId64 " - Local%" PRId64
            ") , worker2[primary key]:[%" PRId64 "- %" PRId64")", begin, count, begin, count);
    }

    void InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull)
    {
        std::vector<uint8_t> photo(photoSize, 'v');
        std::vector<VBucket> record1;
        std::vector<VBucket> extend1;
        std::vector<VBucket> record2;
        std::vector<VBucket> extend2;
        Timestamp now = TimeHelper::GetSysCurrentTime();
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket data;
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", 166.0); // 166.0 is random double value
            data.insert_or_assign("married", false);
            data.insert_or_assign("photo", photo);
            data.insert_or_assign("age", 13L);
            Asset asset = g_cloudAsset;
            asset.name = asset.name + std::to_string(i);
            assetIsNull ? data.insert_or_assign("assert", Nil()) : data.insert_or_assign("assert", asset);
            record1.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            extend1.push_back(log);

            std::vector<Asset> assets;
            data.insert_or_assign("id", i);
            data.insert_or_assign("height", 180.3); // 180.3 is random double value
            for (int64_t j = i; j <= i + 2; j++) { // 2 extra num
                asset.name = g_cloudAsset.name + std::to_string(j);
                assets.push_back(asset);
            }
            data.erase("assert");
            data.erase("married");
            assetIsNull ? data.insert_or_assign("asserts", Nil()) : data.insert_or_assign("asserts", assets);
            record2.push_back(data);
            extend2.push_back(log);
        }
        ASSERT_EQ(g_virtualCloudDb->BatchInsert(g_tableName1, std::move(record1), extend1), DBStatus::OK);
        ASSERT_EQ(g_virtualCloudDb->BatchInsert(g_tableName2, std::move(record2), extend2), DBStatus::OK);
        LOGD("insert cloud record worker1[primary key]:[cloud%" PRId64 " - cloud%" PRId64
            ") , worker2[primary key]:[%" PRId64 "- %" PRId64")", begin, count, begin, count);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void DeleteUserTableRecord(sqlite3 *&db, int64_t begin, int64_t count)
    {
        for (size_t i = 0; i < g_tables.size(); i++) {
            string updateAge = "Delete from " + g_tables[i] + " where " + g_tablesPKey[i] + " in (";
            for (int64_t j = begin; j < count; ++j) {
                updateAge += "'" + g_prefix[i] + std::to_string(j) + "',";
            }
            updateAge.pop_back();
            updateAge += ");";
            ASSERT_EQ(RelationalTestUtils::ExecSql(db, updateAge), SQLITE_OK);
        }
        LOGD("delete local record worker1[primary key]:[local%" PRId64 " - local%" PRId64
            ") , worker2[primary key]:[%" PRId64 "- %" PRId64")", begin, count, begin, count);
    }

    void DeleteCloudTableRecordByGid(int64_t begin, int64_t count)
    {
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket data;
            data.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i));
            ASSERT_EQ(g_virtualCloudDb->DeleteByGid(g_tableName1, data), DBStatus::OK);
        }
        LOGD("delete cloud record worker[primary key]:[cloud%" PRId64 " - cloud%" PRId64")", begin, count);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
    {
        TableSchema tableSchema1 = {
            .name = g_tableName1,
            .sharedTableName = g_tableName1 + "_shared",
            .fields = g_cloudFiled1
        };
        TableSchema tableSchema2 = {
            .name = g_tableName2,
            .sharedTableName = g_tableName2 + "_shared",
            .fields = g_cloudFiled2
        };
        TableSchema tableSchemaWithOutPrimaryKey = {
            .name = g_tableName3,
            .sharedTableName = g_tableName3 + "_shared",
            .fields = g_cloudFiledWithOutPrimaryKey3
        };
        TableSchema tableSchema4 = {
            .name = g_tableName4,
            .sharedTableName = g_tableName4 + "_shared",
            .fields = g_cloudFiled2
        };
        dataBaseSchema.tables.push_back(tableSchema1);
        dataBaseSchema.tables.push_back(tableSchema2);
        dataBaseSchema.tables.push_back(tableSchemaWithOutPrimaryKey);
        dataBaseSchema.tables.push_back(tableSchema4);
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

    void CheckCloudTotalCount(const std::vector<std::string> &tableNames, std::vector<int64_t> expectCounts)
    {
        VBucket extend;
        extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
        for (size_t i = 0; i < tableNames.size(); ++i) {
            int64_t realCount = 0;
            std::vector<VBucket> data;
            g_virtualCloudDb->Query(tableNames[i], extend, data);
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

    void CheckCloudRecordNum(sqlite3 *&db, std::vector<std::string> tableList, std::vector<int> countList)
    {
        int i = 0;
        for (const auto &tableName: tableList) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(tableName) +
                " where device = 'cloud'" + " and cloud_gid is not null and cloud_gid != '' and flag & 0x2 = 0;";
            EXPECT_EQ(sqlite3_exec(db, sql.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(countList[i]), nullptr), SQLITE_OK);
            i++;
        }
    }

    void CheckLocalLogCount(sqlite3 *&db, const std::vector<std::string> &tableList, const std::vector<int> &countList)
    {
        int i = 0;
        for (const auto &tableName: tableList) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(tableName);
            EXPECT_EQ(sqlite3_exec(db, sql.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(countList[i]), nullptr), SQLITE_OK);
            i++;
        }
    }

    void CheckCleanLogNum(sqlite3 *&db, const std::vector<std::string> tableList, int count)
    {
        for (const auto &tableName: tableList) {
            std::string sql1 = "select count(*) from " + DBCommon::GetLogTableName(tableName) +
                " where device = 'cloud';";
            EXPECT_EQ(sqlite3_exec(db, sql1.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
            std::string sql2 = "select count(*) from " + DBCommon::GetLogTableName(tableName) +
                " where cloud_gid " + " is not null and cloud_gid != '';";
            EXPECT_EQ(sqlite3_exec(db, sql2.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
            std::string sql3 = "select count(*) from " + DBCommon::GetLogTableName(tableName) +
                " where flag & 0x02 = 0;";
            EXPECT_EQ(sqlite3_exec(db, sql3.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
        }
    }

    void CheckCleanDataAndLogNum(sqlite3 *&db, const std::vector<std::string> tableList, int count,
        std::vector<int> localNum)
    {
        int i = 0;
        for (const auto &tableName: tableList) {
            std::string sql1 = "select count(*) from " + DBCommon::GetLogTableName(tableName) +
                " where device = 'cloud';";
            EXPECT_EQ(sqlite3_exec(db, sql1.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
            std::string sql2 = "select count(*) from " + DBCommon::GetLogTableName(tableName) + " where cloud_gid "
                " is not null and cloud_gid != '';";
            EXPECT_EQ(sqlite3_exec(db, sql2.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
            std::string sql3 = "select count(*) from " + DBCommon::GetLogTableName(tableName) +
                " where flag & 0x02 = 0;";
            EXPECT_EQ(sqlite3_exec(db, sql3.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
            std::string local_sql = "select count(*) from " + tableName +";";
            EXPECT_EQ(sqlite3_exec(db, local_sql.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(localNum[i]), nullptr), SQLITE_OK);
            i++;
        }
    }

    void InitProcessForCleanCloudData1(const uint32_t &cloudCount, std::vector<SyncProcess> &expectProcess)
    {
        // cloudCount also means data count in one batch
        expectProcess.clear();
        std::vector<TableProcessInfo> infos;
        uint32_t index = 1;
        infos.push_back(TableProcessInfo{
            FINISHED, {index, cloudCount, cloudCount, 0}, {0, 0, 0, 0}
        });
        infos.push_back(TableProcessInfo{
            PREPARED, {0, 0, 0, 0}, {0, 0, 0, 0}
        });

        infos.push_back(TableProcessInfo{
            FINISHED, {index, cloudCount, cloudCount, 0}, {0, 0, 0, 0}
        });
        infos.push_back(TableProcessInfo{
            FINISHED, {index, cloudCount, cloudCount, 0}, {0, 0, 0, 0}
        });

        for (size_t i = 0; i < infos.size() / g_arrayHalfSub; ++i) {
            SyncProcess syncProcess;
            syncProcess.errCode = OK;
            syncProcess.process = i == infos.size() ? FINISHED : PROCESSING;
            syncProcess.tableProcess.insert_or_assign(g_tables[0], std::move(infos[g_arrayHalfSub * i]));
            syncProcess.tableProcess.insert_or_assign(g_tables[1], std::move(infos[g_arrayHalfSub * i + 1]));
            expectProcess.push_back(syncProcess);
        }
    }

    void GetCallback(SyncProcess &syncProcess, CloudSyncStatusCallback &callback,
        std::vector<SyncProcess> &expectProcess)
    {
        g_syncIndex = 0;
        callback = [&syncProcess, &expectProcess](const std::map<std::string, SyncProcess> &process) {
            LOGI("devices size = %d", process.size());
            ASSERT_EQ(process.size(), 1u);
            syncProcess = std::move(process.begin()->second);
            ASSERT_EQ(process.begin()->first, DEVICE_CLOUD);
            ASSERT_NE(syncProcess.tableProcess.empty(), true);
            LOGI("current sync process status:%d, db status:%d ", syncProcess.process, syncProcess.errCode);
            std::for_each(g_tables.begin(), g_tables.end(), [&](const auto &item) {
                auto table1 = syncProcess.tableProcess.find(item);
                if (table1 != syncProcess.tableProcess.end()) {
                    LOGI("table[%s], table process status:%d, [downloadInfo](batchIndex:%u, total:%u, successCount:%u, "
                         "failCount:%u) [uploadInfo](batchIndex:%u, total:%u, successCount:%u,failCount:%u",
                         item.c_str(), table1->second.process, table1->second.downLoadInfo.batchIndex,
                         table1->second.downLoadInfo.total, table1->second.downLoadInfo.successCount,
                         table1->second.downLoadInfo.failCount, table1->second.upLoadInfo.batchIndex,
                         table1->second.upLoadInfo.total, table1->second.upLoadInfo.successCount,
                         table1->second.upLoadInfo.failCount);
                }
            });
            if (expectProcess.empty()) {
                if (syncProcess.process == FINISHED) {
                    g_processCondition.notify_one();
                }
                return;
            }
            ASSERT_LE(static_cast<size_t>(g_syncIndex), expectProcess.size());
            for (size_t i = 0; i < g_tables.size() && static_cast<size_t>(g_syncIndex) < expectProcess.size(); ++i) {
                SyncProcess head = expectProcess[g_syncIndex];
                for (auto &expect : head.tableProcess) {
                    auto real = syncProcess.tableProcess.find(expect.first);
                    ASSERT_NE(real, syncProcess.tableProcess.end());
                    EXPECT_EQ(expect.second.process, real->second.process);
                    EXPECT_EQ(expect.second.downLoadInfo.batchIndex, real->second.downLoadInfo.batchIndex);
                    EXPECT_EQ(expect.second.downLoadInfo.total, real->second.downLoadInfo.total);
                    EXPECT_EQ(expect.second.downLoadInfo.successCount, real->second.downLoadInfo.successCount);
                    EXPECT_EQ(expect.second.downLoadInfo.failCount, real->second.downLoadInfo.failCount);
                    EXPECT_EQ(expect.second.upLoadInfo.batchIndex, real->second.upLoadInfo.batchIndex);
                    EXPECT_EQ(expect.second.upLoadInfo.total, real->second.upLoadInfo.total);
                    EXPECT_EQ(expect.second.upLoadInfo.successCount, real->second.upLoadInfo.successCount);
                    EXPECT_EQ(expect.second.upLoadInfo.failCount, real->second.upLoadInfo.failCount);
                }
            }
            g_syncIndex++;
            if (syncProcess.process == FINISHED) {
                g_processCondition.notify_one();
            }
        };
    }

    void CopySharedDataFromOriginalTable(sqlite3 *&db, const std::vector<std::string> &tableNames)
    {
        for (const auto &tableName: tableNames) {
            std::string sql = "INSERT OR REPLACE INTO " + tableName + CloudDbConstant::SHARED + " SELECT " +
                "*," + std::string(DBConstant::SQLITE_INNER_ROWID) + ",''" + " FROM " + tableName;
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        }
    }

    void CheckCloudSharedRecordNum(sqlite3 *&db, std::vector<std::string> tableList, std::vector<int> countList)
    {
        int i = 0;
        for (const auto &tableName: tableList) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(tableName);
            EXPECT_EQ(sqlite3_exec(db, sql.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(countList[i]), nullptr), SQLITE_OK);
            sql = "select count(*) from " + tableName;
            EXPECT_EQ(sqlite3_exec(db, sql.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(countList[i]), nullptr), SQLITE_OK);
            i++;
        }
    }

    void WaitForSyncFinish(SyncProcess &syncProcess, const int64_t &waitTime)
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(waitTime), [&syncProcess]() {
            return syncProcess.process == FINISHED;
        });
        ASSERT_EQ(result, true);
        LOGD("-------------------sync end--------------");
    }

    void CloseDb()
    {
        delete g_observer;
        g_virtualCloudDb = nullptr;
        if (g_delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
            g_delegate = nullptr;
        }
    }

    class DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    protected:
        sqlite3 *db = nullptr;
    };

    void DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        g_storePath = g_testDir + "/" + g_storeID + DB_SUFFIX;
        LOGI("The test db is:%s", g_testDir.c_str());
        RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
    }

    void DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest::TearDownTestCase(void)
    {}

    void DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest::SetUp(void)
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
            g_delegate), DBStatus::OK);
        ASSERT_NE(g_delegate, nullptr);
        ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName1, CLOUD_COOPERATION), DBStatus::OK);
        ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName2, CLOUD_COOPERATION), DBStatus::OK);
        ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName3, CLOUD_COOPERATION), DBStatus::OK);
        g_virtualCloudDb = make_shared<VirtualCloudDb>();
        g_virtualAssetLoader = make_shared<VirtualAssetLoader>();
        g_syncProcess = {};
        ASSERT_EQ(g_delegate->SetCloudDB(g_virtualCloudDb), DBStatus::OK);
        ASSERT_EQ(g_delegate->SetIAssetLoader(g_virtualAssetLoader), DBStatus::OK);
        // sync before setting cloud db schema,it should return SCHEMA_MISMATCH
        Query query = Query::Select().FromTable(g_tables);
        CloudSyncStatusCallback callback;
        ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime),
            DBStatus::SCHEMA_MISMATCH);
        DataBaseSchema dataBaseSchema;
        GetCloudDbSchema(dataBaseSchema);
        ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    }

    void DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest::TearDown(void)
    {
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
            LOGE("rm test db files error.");
        }
    }

/*
 * @tc.name: CleanCloudDataTest001
 * @tc.desc: Test FLAG_ONLY mode of RemoveDeviceData, and invalid mode else.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest001, TestSize.Level0)
{
    int64_t paddingSize = 10;
    int localCount = 10;
    int cloudCount = 20;
    InsertCloudTableRecord(0, cloudCount, paddingSize, false);
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    Query query = Query::Select().FromTable(g_tables);
    std::vector<SyncProcess> expectProcess;
    InitProcessForCleanCloudData1(cloudCount, expectProcess);
    CloudSyncStatusCallback callback;
    GetCallback(g_syncProcess, callback, expectProcess);
    ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_FORCE_PULL, query, callback, g_syncWaitTime),
        DBStatus::OK);
    WaitForSyncFinish(g_syncProcess, g_syncWaitTime);
    std::string device = "";
    CheckCloudRecordNum(db, g_tables, {20, 20});
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, FLAG_ONLY), DBStatus::OK);
    CheckCleanLogNum(db, g_tables, 0);

    ASSERT_EQ(g_delegate->RemoveDeviceData(device, ClearMode(BUTT + 1)), DBStatus::INVALID_ARGS);
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, ClearMode(-1)), DBStatus::INVALID_ARGS);

    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest002
 * @tc.desc: Test FLAG_AND_DATA mode of RemoveDeviceData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest002, TestSize.Level0)
{
    int64_t paddingSize = 10;
    int localCount = 10;
    int cloudCount = 20;
    InsertCloudTableRecord(0, cloudCount, paddingSize, false);
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    Query query = Query::Select().FromTable(g_tables);
    std::vector<SyncProcess> expectProcess;
    InitProcessForCleanCloudData1(cloudCount, expectProcess);
    CloudSyncStatusCallback callback;
    GetCallback(g_syncProcess, callback, expectProcess);
    ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_FORCE_PULL, query, callback, g_syncWaitTime),
        DBStatus::OK);
    WaitForSyncFinish(g_syncProcess, g_syncWaitTime);
    std::string device = "";
    CheckCloudRecordNum(db, g_tables, {20, 20});    // 20 means cloud data num
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, FLAG_AND_DATA), DBStatus::OK);
    CheckCleanDataAndLogNum(db, g_tables, 0, {localCount, 0});
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest003
 * @tc.desc: Test FLAG_ONLY mode of RemoveDeviceData concurrently with Sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. make data: 10 records on local and 20 records on cloud
     */
    int64_t paddingSize = 10;
    int localCount = 10;
    int cloudCount = 20;
    InsertCloudTableRecord(0, cloudCount, paddingSize, false);
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    /**
     * @tc.steps: step2. call Sync with cloud force pull strategy, and after that, local will has 20 records.
     */
    Query query = Query::Select().FromTable(g_tables);
    std::vector<SyncProcess> expectProcess;
    InitProcessForCleanCloudData1(cloudCount, expectProcess);
    CloudSyncStatusCallback callback;
    GetCallback(g_syncProcess, callback, expectProcess);
    ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_FORCE_PULL, query, callback, g_syncWaitTime),
        DBStatus::OK);
    WaitForSyncFinish(g_syncProcess, g_syncWaitTime);
    CheckCloudRecordNum(db, g_tables, {20, 20});    // 20 means cloud data num

    /**
     * @tc.steps: step3. insert 10 records into local, so local will has 20 local records and 20 cloud records.
     */
    InsertUserTableRecord(db, 21, localCount, paddingSize, false);  // 21 means insert start index
    /**
     * @tc.steps: step4. call RemoveDeviceData synchronize with Sync with cloud force push strategy.
     */
    g_syncProcess = {};
    std::vector<SyncProcess> expectProcess2;
    InitProcessForCleanCloudData1(cloudCount, expectProcess2);
    CloudSyncStatusCallback callback2;
    GetCallback(g_syncProcess, callback2, expectProcess2);
    std::string device = "";

    std::thread thread1([&]() {
        ASSERT_EQ(g_delegate->RemoveDeviceData(device, FLAG_AND_DATA), DBStatus::OK);
    });
    std::thread thread2([&]() {
        ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_FORCE_PULL, query, callback2, g_syncWaitTime),
            DBStatus::OK);
        LOGD("-------------------sync end--------------");
    });
    thread1.join();
    thread2.join();
    WaitForSyncFinish(g_syncProcess, g_syncWaitTime);
    CheckCleanLogNum(db, g_tables, 20);
    LOGD("================================== test clean cloud data 003 end ===================================");
    CloseDb();
}

static void InitGetCloudSyncTaskCountTest001(sqlite3 *&db)
{
    int64_t localCount = 20;
    int64_t cloudCount = 10;
    int64_t paddingSize = 100;
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    InsertCloudTableRecord(0, cloudCount, paddingSize, false);
}
/*
 * @tc.name: GetCloudSyncTaskCountTest001
 * @tc.desc: Test FLAG_ONLY mode of RemoveDeviceData concurrently with Sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, GetCloudSyncTaskCountTest001, TestSize.Level0)
{
    InitGetCloudSyncTaskCountTest001(db);
    Query query = Query::Select().FromTable(g_tables);
    std::mutex dataMutex1, dataMutex2;
    std::condition_variable cv1, cv2;
    bool finish1 = false, finish2 = false;
    /**
     * @tc.steps: step1. Call Sync once.
     * @tc.expected: OK.
     */
    CloudSyncStatusCallback callback1 = [&dataMutex1, &cv1, &finish1](
        const std::map<std::string, SyncProcess> &process) {
        std::map<std::string, SyncProcess> syncProcess;
        {
            std::lock_guard<std::mutex> autoLock(dataMutex1);
            syncProcess = process;
            if (syncProcess[DEVICE_CLOUD].process == FINISHED) {
                finish1 = true;
            }
        }
        cv1.notify_one();
    };
    /**
     * @tc.steps: step2. Call Sync twice.
     * @tc.expected: OK.
     */
    ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback1, g_syncWaitTime), DBStatus::OK);

    CloudSyncStatusCallback callback2 = [&dataMutex2, &cv2, &finish2](
        const std::map<std::string, SyncProcess> &process) {
        std::map<std::string, SyncProcess> syncProcess;
        {
            std::lock_guard<std::mutex> autoLock(dataMutex2);
            syncProcess = process;
            if (syncProcess[DEVICE_CLOUD].process == FINISHED) {
                finish2 = true;
            }
        }
        cv2.notify_one();
    };
    ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback2, g_syncWaitTime), DBStatus::OK);
    /**
     * @tc.steps: step3. Call Get Cloud Sync Task Count
     * @tc.expected: OK.
     */
    EXPECT_EQ(g_delegate->GetCloudSyncTaskCount(), 2);  // 2 is task count
    /**
     * @tc.steps: step3. Wait For Sync Task Finished
     * @tc.expected: OK.
     */
    {
        std::unique_lock<std::mutex> uniqueLock(dataMutex1);
        cv1.wait(uniqueLock, [&finish1] {
            return finish1;
        });
    }
    {
        std::unique_lock<std::mutex> uniqueLock(dataMutex2);
        cv2.wait(uniqueLock, [&finish2] {
            return finish2;
        });
    }
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest004
 * @tc.desc: Test  RemoveDeviceData when cloudSchema doesn't have local table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest004, TestSize.Level0)
{
    DataBaseSchema dataBaseSchema;
    TableSchema tableSchema1 = {
        .name = "table_not_existed",
        .sharedTableName = "table_not_existed_shared",
        .fields = g_cloudFiled1
    };
    dataBaseSchema.tables.push_back(tableSchema1);
    GetCloudDbSchema(dataBaseSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    std::string device = "";
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, FLAG_AND_DATA), DBStatus::OK);
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest005
 * @tc.desc: Test RemoveDeviceData when cloud data is deleted
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. cloud and device data is same
     * @tc.expected: OK.
     */
    int64_t paddingSize = 10; // 10 is padding size
    int64_t cloudCount = 10; // 10 is cloud count
    InsertCloudTableRecord(0, cloudCount, paddingSize, true);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);

    /**
     * @tc.steps: step2. cloud delete data and merge
     * @tc.expected: OK.
     */
    DeleteCloudTableRecordByGid(0, cloudCount);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudRecordNum(db, g_tables, {0, 10}); // 10 is cloud record num in table2 log
    CheckCloudTotalCount(g_tables, {0, 10}); // // 10 is cloud data num in table2

    /**
     * @tc.steps: step3. removedevicedata FLAG_AND_DATA and check log
     * @tc.expected: OK.
     */
    std::string device = "";
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, FLAG_AND_DATA), DBStatus::OK);
    CheckCleanDataAndLogNum(db, g_tables, 0, {0, 0});
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest006
 * @tc.desc: Test FLAG_ONLY mode of RemoveDeviceData before Sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. make data: 10 records on local
     */
    int64_t paddingSize = 10;
    int localCount = 10;
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    /**
     * @tc.steps: step2. call Sync with cloud merge strategy, and after that, local will has 20 records.
     */
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    LOGW("check 10-10");
    CheckCloudTotalCount(g_tables, {10, 10}); // 10 is cloud data num in table2
    g_virtualCloudDb->ClearAllData();
    LOGW("check 0-0");
    CheckCloudTotalCount(g_tables, {0, 0}); // 0 is cloud data num in table2
    /**
     * @tc.steps: step3. removedevicedata FLAG_AND_DATA and sync again
     * @tc.expected: OK.
     */
    std::string device;
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, DistributedDB::FLAG_ONLY), DBStatus::OK);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    LOGW("check 10-10");
    CheckCloudTotalCount(g_tables, {10, 10}); // 10 is cloud data num in table2
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest007
 * @tc.desc: Test CLEAR_SHARED_TABLE mode of RemoveDeviceData before Sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. make data: 10 records on local
     */
    int64_t paddingSize = 1;
    int localCount = 10;
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    /**
     * @tc.steps: step2. call Sync with cloud merge strategy, and after that, local will has 20 records.
     */
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudTotalCount(g_tables, {10, 10}); // 10 is cloud data num
    g_virtualCloudDb->ClearAllData();
    CheckCloudTotalCount(g_tables, {0, 0});
    /**
     * @tc.steps: step3. removedevicedata in CLEAR_SHARED_TABLE mode will not delete unShare table data
     * @tc.expected: OK.
     */
    std::string device;
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, DistributedDB::CLEAR_SHARED_TABLE), DBStatus::OK);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudTotalCount(g_tables, {0, 0});
    /**
     * @tc.steps: step4. copy db data to share table,then sync to check total count
     * @tc.expected: OK.
     */
    CopySharedDataFromOriginalTable(db, g_tables);
    CloudDBSyncUtilsTest::callSync(g_shareTables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudTotalCount(g_tables, {0, 0});
    CheckCloudTotalCount(g_shareTables, {10, 10}); // 10 is cloud data num
    g_virtualCloudDb->ClearAllData();
    CheckCloudTotalCount(g_shareTables, {0, 0});
    /**
     * @tc.steps: step5. removedevicedata in CLEAR_SHARED_TABLE mode,then sync and check data
     * @tc.expected: OK.
     */
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, DistributedDB::CLEAR_SHARED_TABLE), DBStatus::OK);
    CheckCloudSharedRecordNum(db, g_shareTables, {0, 0, 0, 0});
    CloudDBSyncUtilsTest::callSync(g_shareTables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudTotalCount(g_shareTables, {0, 0});
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest008
 * @tc.desc: Test CLEAR_SHARED_TABLE mode of RemoveDeviceData after close DB
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. make data: 10 records on local
     */
    int64_t paddingSize = 1;
    int localCount = 10;
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    /**
     * @tc.steps: step2. call Sync with cloud merge strategy, and after that, local will has 20 records.
     */
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudTotalCount(g_tables, {10, 10}); // 10 is cloud data num
    g_virtualCloudDb->ClearAllData();
    CheckCloudTotalCount(g_tables, {0, 0});
    /**
     * @tc.steps: step3. removedevicedata in CLEAR_SHARED_TABLE mode will not delete unShare table data
     * @tc.expected: OK.
     */
    std::string device;
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, DistributedDB::CLEAR_SHARED_TABLE), DBStatus::OK);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudTotalCount(g_tables, {0, 0});
    /**
     * @tc.steps: step4. copy db data to share table,then sync to check total count
     * @tc.expected: OK.
     */
    CopySharedDataFromOriginalTable(db, g_tables);
    CloudDBSyncUtilsTest::callSync(g_shareTables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    CheckCloudTotalCount(g_tables, {0, 0});
    CheckCloudTotalCount(g_shareTables, {10, 10}); // 10 is cloud data num
    g_virtualCloudDb->ClearAllData();
    CheckCloudTotalCount(g_shareTables, {0, 0});
    /**
     * @tc.steps: step5. removedevicedata in CLEAR_SHARED_TABLE mode after close db, then sync and check data
     * @tc.expected: OK.
     */
    CloseDb();
    g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
    ASSERT_NE(g_observer, nullptr);
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option { .observer = g_observer },
        g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, DistributedDB::CLEAR_SHARED_TABLE), DBStatus::OK);
    CheckCloudSharedRecordNum(db, g_shareTables, {0, 0, 0, 0});
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest008
 * @tc.desc: Test RemoveDeviceData after Sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest009, TestSize.Level0)
{
    /**
     * @tc.steps: step1. make data: 10 records on local
     */
    int64_t paddingSize = 10;
    int localCount = 10;
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    /**
     * @tc.steps: step2. call Sync with cloud merge strategy, and after that, local will has 10 records.
     */
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    LOGW("check 10-10");
    CheckCloudTotalCount(g_tables, {10, 10}); // 10 is cloud data num in table2
    /**
     * @tc.steps: step3. remove cloud and sync again
     * @tc.expected: OK.
     */
    DeleteCloudTableRecordByGid(0, localCount);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    g_delegate->RemoveDeviceData();
    CheckLocalLogCount(db, { g_tableName1 }, { localCount });
    CloseDb();
}

/*
 * @tc.name: CleanCloudDataTest010
 * @tc.desc: Test if log is delete when removedevicedata after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalRemoveDeviceDataTest, CleanCloudDataTest010, TestSize.Level0)
{
    /**
     * @tc.steps: step1. make data: 10 records on local
     */
    int64_t paddingSize = 10;
    int localCount = 10;
    InsertUserTableRecord(db, 0, localCount, paddingSize, false);
    /**
     * @tc.steps: step2. call Sync with cloud merge strategy, and after that, local will has 10 records.
     */
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    LOGW("check 10-10");
    CheckCloudTotalCount(g_tables, {10, 10}); // 10 is cloud data num in table2
    /**
     * @tc.steps: step3. remove cloud and sync again
     * @tc.expected: OK.
     */
    int deleteCount = 5;
    DeleteCloudTableRecordByGid(0, deleteCount);
    DeleteUserTableRecord(db, deleteCount, localCount);
    CloudDBSyncUtilsTest::callSync(g_tables, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, g_delegate);
    std::string device;
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, DistributedDB::FLAG_ONLY), DBStatus::OK);
    CheckLocalLogCount(db, { g_tableName1 }, { deleteCount });
    CloseDb();
}
}
#endif // RELATIONAL_STORE