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
#include "cloud_db_sync_utils_test.h"

using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace DistributedDB {
    string g_storeID = "Relational_Store_SYNC";
    const string TABLE_NAME = "worker";
    const string DEVICE_CLOUD = "cloud_dev";
    const int64_t SYNC_WAIT_TIME = 60;
    int g_syncIndex = 0;
    string g_storePath = "";
    std::mutex g_processMutex;
    std::condition_variable g_processCondition;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    SyncProcess g_syncProcess;
    using CloudSyncStatusCallback = std::function<void(const std::map<std::string, SyncProcess> &onProcess)>;
    const std::vector<std::string> g_tables = {TABLE_NAME};
    const Asset g_cloudAsset1 = {
        .version = 2, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/cloud/sync",
        .modifyTime = "123456", .createTime = "0", .size = "1024", .hash = "DEC"
    };
    const Asset g_cloudAsset2 = {
        .version = 2, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/cloud/sync",
        .modifyTime = "123456", .createTime = "0", .size = "1024", .hash = "UPDATE"
    };

    void CloudDBSyncUtilsTest::SetStorePath(const std::string &path)
    {
        g_storePath = path;
    }

    void CloudDBSyncUtilsTest::InitSyncUtils(const std::vector<Field> &cloudField,
        RelationalStoreObserverUnitTest *&observer, std::shared_ptr<VirtualCloudDb> &virtualCloudDb,
        std::shared_ptr<VirtualAssetLoader> &virtualAssetLoader, RelationalStoreDelegate *&delegate)
    {
        observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        ASSERT_NE(observer, nullptr);
        ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option { .observer = observer },
            delegate), DBStatus::OK);
        ASSERT_NE(delegate, nullptr);
        ASSERT_EQ(delegate->CreateDistributedTable(TABLE_NAME, CLOUD_COOPERATION), DBStatus::OK);
        virtualCloudDb = make_shared<VirtualCloudDb>();
        virtualAssetLoader = make_shared<VirtualAssetLoader>();
        g_syncProcess = {};
        ASSERT_EQ(delegate->SetCloudDB(virtualCloudDb), DBStatus::OK);
        ASSERT_EQ(delegate->SetIAssetLoader(virtualAssetLoader), DBStatus::OK);
        // sync before setting cloud db schema,it should return SCHEMA_MISMATCH
        Query query = Query::Select().FromTable(g_tables);
        CloudSyncStatusCallback callback;
        ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, SYNC_WAIT_TIME),
            DBStatus::SCHEMA_MISMATCH);
        DataBaseSchema dataBaseSchema;
        GetCloudDbSchema(TABLE_NAME, cloudField, dataBaseSchema);
        ASSERT_EQ(delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    }

    void CloudDBSyncUtilsTest::CreateUserDBAndTable(sqlite3 *&db, std::string sql)
    {
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    }

    void CloudDBSyncUtilsTest::InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb)
    {
        std::vector<uint8_t> photo(photoSize, 'v');
        std::vector<VBucket> record;
        std::vector<VBucket> extend;
        Timestamp now = TimeHelper::GetSysCurrentTime();
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket data;
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", 166.0); // 166.0 is random double value
            data.insert_or_assign("married", false);
            data.insert_or_assign("photo", photo);
            data.insert_or_assign("age", 13L); // 13 is random int64_t value
            Asset asset = g_cloudAsset1;
            asset.name = asset.name + std::to_string(i);
            assetIsNull ? data.insert_or_assign("asset", Nil()) : data.insert_or_assign("asset", asset);
            record.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD,
                static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD,
                static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            extend.push_back(log);
        }
        ASSERT_EQ(virtualCloudDb->BatchInsert(TABLE_NAME, std::move(record), extend), DBStatus::OK);
        LOGD("insert cloud record worker[primary key]:[cloud%" PRId64 " - cloud%" PRId64")", begin, count);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void CloudDBSyncUtilsTest::UpdateCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb)
    {
        std::vector<uint8_t> photo(photoSize, 'v');
        std::vector<VBucket> record;
        std::vector<VBucket> extend;
        Timestamp now = TimeHelper::GetSysCurrentTime();
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket data;
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", 188.0); // 188.0 is random double value
            data.insert_or_assign("married", false);
            data.insert_or_assign("photo", photo);
            data.insert_or_assign("age", 13L); // 13 is random int64_t value
            Asset asset = g_cloudAsset2;
            asset.name = asset.name + std::to_string(i);
            assetIsNull ? data.insert_or_assign("asset", Nil()) : data.insert_or_assign("asset", asset);
            record.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD,
                static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD,
                static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            log.insert_or_assign(CloudDbConstant::GID_FIELD, to_string(i));
            extend.push_back(log);
        }
        ASSERT_EQ(virtualCloudDb->BatchUpdate(TABLE_NAME, std::move(record), extend), DBStatus::OK);
        LOGD("update cloud record worker[primary key]:[cloud%" PRId64 " - cloud%" PRId64")", begin, count);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void CloudDBSyncUtilsTest::DeleteCloudTableRecordByGid(int64_t begin, int64_t count,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb)
    {
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket data;
            data.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i));
            ASSERT_EQ(virtualCloudDb->DeleteByGid(TABLE_NAME, data), DBStatus::OK);
        }
        LOGD("delete cloud record worker[primary key]:[cloud%" PRId64 " - cloud%" PRId64")", begin, count);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void CloudDBSyncUtilsTest::DeleteUserTableRecord(sqlite3 *&db, int64_t begin, int64_t count)
    {
        for (size_t i = 0; i < g_tables.size(); i++) {
            for (int64_t j = begin; j < begin + count; j++) {
                string sql = "Delete from " + g_tables[i] + " where name = 'Cloud" + std::to_string(j) + "';";
                ASSERT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
            }
        }
    }

    void CloudDBSyncUtilsTest::GetCallback(SyncProcess &syncProcess, CloudSyncStatusCallback &callback,
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

    void CloudDBSyncUtilsTest::CheckCloudTotalCount(std::vector<int64_t> expectCounts,
        const std::shared_ptr<VirtualCloudDb> &virtualCloudDb)
    {
        VBucket extend;
        extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
        for (size_t i = 0; i < g_tables.size(); ++i) {
            int64_t realCount = 0;
            std::vector<VBucket> data;
            virtualCloudDb->Query(g_tables[i], extend, data);
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

    void CloudDBSyncUtilsTest::WaitForSyncFinish(SyncProcess &syncProcess, const int64_t &waitTime)
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(waitTime), [&syncProcess]() {
            return syncProcess.process == FINISHED;
        });
        ASSERT_EQ(result, true);
        LOGD("-------------------sync end--------------");
    }

    void CloudDBSyncUtilsTest::callSync(const std::vector<std::string> &tableNames, SyncMode mode, DBStatus dbStatus,
        RelationalStoreDelegate *&delegate)
    {
        g_syncProcess = {};
        Query query = Query::Select().FromTable(tableNames);
        std::vector<SyncProcess> expectProcess;
        CloudSyncStatusCallback callback;
        GetCallback(g_syncProcess, callback, expectProcess);
        ASSERT_EQ(delegate->Sync({DEVICE_CLOUD}, mode, query, callback, SYNC_WAIT_TIME), dbStatus);
        if (dbStatus == DBStatus::OK) {
            WaitForSyncFinish(g_syncProcess, SYNC_WAIT_TIME);
        }
    }

    void CloudDBSyncUtilsTest::CloseDb(RelationalStoreObserverUnitTest *&observer,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb, RelationalStoreDelegate *&delegate)
    {
        delete observer;
        virtualCloudDb = nullptr;
        if (delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(delegate), DBStatus::OK);
            delegate = nullptr;
        }
    }

    int CloudDBSyncUtilsTest::QueryCountCallback(void *data, int count, char **colValue, char **colName)
    {
        if (count != 1) {
            return 0;
        }
        auto expectCount = reinterpret_cast<int64_t>(data);
        EXPECT_EQ(strtol(colValue[0], nullptr, 10), expectCount); // 10: decimal
        return 0;
    }

    void CloudDBSyncUtilsTest::CheckDownloadResult(sqlite3 *&db, std::vector<int64_t> expectCounts,
        const std::string &keyStr)
    {
        for (size_t i = 0; i < g_tables.size(); ++i) {
            string queryDownload = "select count(*) from " + g_tables[i] + " where name " +
                " like '" + keyStr + "%'";
            EXPECT_EQ(sqlite3_exec(db, queryDownload.c_str(), QueryCountCallback,
                reinterpret_cast<void *>(expectCounts[i]), nullptr), SQLITE_OK);
        }
    }

    void CloudDBSyncUtilsTest::CheckLocalRecordNum(sqlite3 *&db, const std::string &tableName, int count)
    {
        std::string sql = "select count(*) from " + tableName + ";";
        EXPECT_EQ(sqlite3_exec(db, sql.c_str(), QueryCountCallback,
            reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
    }

    void CloudDBSyncUtilsTest::GetCloudDbSchema(const std::string &tableName, const std::vector<Field> &cloudField,
        DataBaseSchema &dataBaseSchema)
    {
        TableSchema tableSchema = {
            .name = tableName,
            .sharedTableName = tableName + "_shared",
            .fields = cloudField
        };
        dataBaseSchema.tables.push_back(tableSchema);
    }

    void CloudDBSyncUtilsTest::InitStoreProp(const std::string &storePath, const std::string &appId,
        const std::string &userId, const std::string &storeId, RelationalDBProperties &properties)
    {
        properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
        properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
        properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
        properties.SetStringProp(RelationalDBProperties::STORE_ID, storeId);
        std::string identifier = userId + "-" + appId + "-" + storeId;
        std::string hashIdentifier = DBCommon::TransferHashString(identifier);
        properties.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
    }

    void CloudDBSyncUtilsTest::CheckCount(sqlite3 *db, const std::string &sql, int64_t count)
    {
        EXPECT_EQ(sqlite3_exec(db, sql.c_str(), QueryCountCallback, reinterpret_cast<void *>(count), nullptr),
            SQLITE_OK);
    }

    void CloudDBSyncUtilsTest::GetHashKey(const std::string &tableName, const std::string &condition, sqlite3 *db,
        std::vector<std::vector<uint8_t>> &hashKey)
    {
        sqlite3_stmt *stmt = nullptr;
        std::string sql = "select hash_key from " + DBCommon::GetLogTableName(tableName) + " where " + condition;
        EXPECT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::vector<uint8_t> blob;
            EXPECT_EQ(SQLiteUtils::GetColumnBlobValue(stmt, 0, blob), E_OK);
            hashKey.push_back(blob);
        }
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
}
