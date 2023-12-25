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

#include "cloudsync_fuzzer.h"
#include "cloud/cloud_db_types.h"
#include "cloud/cloud_db_constant.h"
#include "time_helper.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "log_print.h"
#include "fuzzer_data.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;

static const char *g_deviceCloud = "cloud_dev";
static const char *g_storeId = "STORE_ID";
static const char *g_dbSuffix = ".db";
static const char *g_table = "worker1";
static const char *g_createLocalTableSql =
    "CREATE TABLE IF NOT EXISTS worker1(" \
    "name TEXT PRIMARY KEY," \
    "height REAL DEFAULT 123.4," \
    "married BOOLEAN DEFAULT false," \
    "photo BLOB NOT NULL," \
    "assert BLOB DEFAULT NULL," \
    "asserts BLOB DEFAULT NULL," \
    "age INT DEFAULT 1);";
static const char *g_insertLocalDataSql =
    "INSERT OR REPLACE INTO worker1(name, photo) VALUES(?, ?);";
static const char *g_insertLocalAssetSql =
    "INSERT OR REPLACE INTO worker1(name, photo, assert, asserts) VALUES(?, ?, ?, ?);";
static const Asset g_localAsset = {
    .version = 1, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/local/sync",
    .modifyTime = "123456", .createTime = "", .size = "256", .hash = "ASE"
};
static const Asset g_cloudAsset = {
    .version = 2, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/cloud/sync",
    .modifyTime = "123456", .createTime = "0", .size = "1024", .hash = "DEC"
};
static constexpr const int MOD = 1000; // 1000 is mod
class CloudSyncContext {
public:
    void FinishAndNotify()
    {
        {
            std::lock_guard<std::mutex> autoLock(mutex_);
            finished_ = true;
        }
        cv_.notify_one();
    }

    void WaitForFinish()
    {
        std::unique_lock<std::mutex> uniqueLock(mutex_);
        LOGI("begin wait");
        cv_.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MAX_TIMEOUT), [this]() {
            return finished_;
        });
        LOGI("end wait");
    }
private:
    std::condition_variable cv_;
    std::mutex mutex_;
    bool finished_ = false;
};
class CloudSyncTest {
public:
    static void ExecSql(sqlite3 *db, const std::string &sql)
    {
        if (db == nullptr || sql.empty()) {
            return;
        }
        int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        if (errCode != SQLITE_OK) {
            LOGE("Execute sql failed. err: %d", errCode);
        }
    }

    static void CreateTable(sqlite3 *&db)
    {
        ExecSql(db, "PRAGMA journal_mode=WAL;");
        ExecSql(db, g_createLocalTableSql);
    }

    static void SetCloudDbSchema(RelationalStoreDelegate *delegate)
    {
        DataBaseSchema dataBaseSchema;
        const std::vector<Field> cloudFiled = {
            {"name", TYPE_INDEX<std::string>, true}, {"height", TYPE_INDEX<double>},
            {"married", TYPE_INDEX<bool>}, {"photo", TYPE_INDEX<Bytes>, false, false},
            {"assert", TYPE_INDEX<Asset>},  {"asserts", TYPE_INDEX<Assets>}, {"age", TYPE_INDEX<int64_t>}
        };
        TableSchema tableSchema = {
            .name = g_table,
            .fields = cloudFiled
        };
        dataBaseSchema.tables.push_back(tableSchema);
        delegate->SetCloudDbSchema(dataBaseSchema);
        delegate->CreateDistributedTable(g_table, CLOUD_COOPERATION);
    }

    static void InitDbData(sqlite3 *&db, const uint8_t *data, size_t size)
    {
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(db, g_insertLocalDataSql, stmt);
        if (errCode != E_OK) {
            return;
        }
        FuzzerData fuzzerData(data, size);
        uint32_t len = fuzzerData.GetUInt32() % MOD;
        for (size_t i = 0; i <= size; ++i) {
            std::string idStr = fuzzerData.GetString(len);
            errCode = SQLiteUtils::BindTextToStatement(stmt, 1, idStr);
            if (errCode != E_OK) {
                break;
            }
            std::vector<uint8_t> photo = fuzzerData.GetSequence(fuzzerData.GetInt());
            errCode = SQLiteUtils::BindBlobToStatement(stmt, 2, photo); // 2 is index of photo
            if (errCode != E_OK) {
                break;
            }
            errCode = SQLiteUtils::StepWithRetry(stmt);
            if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
                break;
            }
            SQLiteUtils::ResetStatement(stmt, false, errCode);
        }
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }

    static Asset GenAsset(const std::string &name, const std::string &hash)
    {
        Asset asset;
        asset.name = name;
        asset.hash = hash;
        return asset;
    }

    void InitDbAsset(const uint8_t* data, size_t size)
    {
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(db_, g_insertLocalAssetSql, stmt);
        if (errCode != E_OK) {
            return;
        }
        FuzzerData fuzzerData(data, size);
        uint32_t len = fuzzerData.GetUInt32() % MOD;
        for (size_t i = 0; i <= size; ++i) {
            std::string idStr = fuzzerData.GetString(len);
            errCode = SQLiteUtils::BindTextToStatement(stmt, 1, idStr);
            if (errCode != E_OK) {
                break;
            }
            std::vector<uint8_t> photo = fuzzerData.GetSequence(fuzzerData.GetInt());
            errCode = SQLiteUtils::BindBlobToStatement(stmt, 2, photo); // 2 is index of photo
            if (errCode != E_OK) {
                break;
            }
            std::vector<uint8_t> assetBlob;
            RuntimeContext::GetInstance()->AssetToBlob(localAssets_[i % size], assetBlob);
            errCode = SQLiteUtils::BindBlobToStatement(stmt, 3, assetBlob); // 3 is index of assert
            if (errCode != E_OK) {
                break;
            }

            std::vector<Asset> assetVec(localAssets_.begin() + (i % size), localAssets_.end());
            RuntimeContext::GetInstance()->AssetsToBlob(assetVec, assetBlob);
            errCode = SQLiteUtils::BindBlobToStatement(stmt, 4, assetBlob); // 4 is index of asserts
            if (errCode != E_OK) {
                break;
            }
            errCode = SQLiteUtils::StepWithRetry(stmt);
            if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
                break;
            }
            SQLiteUtils::ResetStatement(stmt, false, errCode);
        }
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }

    void InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull)
    {
        std::vector<uint8_t> photo(photoSize, 'v');
        std::vector<VBucket> record1;
        std::vector<VBucket> extend1;
        double randomHeight = 166.0; // 166.0 is random double value
        int64_t randomAge = 13L; // 13 is random int64_t value
        Timestamp now = TimeHelper::GetSysCurrentTime();
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket data;
            data.insert_or_assign("name", "Cloud" + std::to_string(i));
            data.insert_or_assign("height", randomHeight);
            data.insert_or_assign("married", false);
            data.insert_or_assign("photo", photo);
            data.insert_or_assign("age", randomAge);
            Asset asset = g_cloudAsset;
            asset.name = asset.name + std::to_string(i);
            assetIsNull ? data.insert_or_assign("assert", Nil()) : data.insert_or_assign("assert", asset);
            record1.push_back(data);
            VBucket log;
            log.insert_or_assign(CloudDbConstant::CREATE_FIELD,
                static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(CloudDbConstant::MODIFY_FIELD,
                static_cast<int64_t>(now / CloudDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
            extend1.push_back(log);
        }
        virtualCloudDb_->BatchInsert(g_table, std::move(record1), extend1);
        LOGD("insert cloud record worker1[primary key]:[cloud%" PRId64 " - cloud%" PRId64 ")", begin, count);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void SetUp()
    {
        DistributedDBToolsTest::TestDirInit(testDir_);
        storePath_ = testDir_ + "/" + g_storeId + g_dbSuffix;
        LOGI("The test db is:%s", testDir_.c_str());
        RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
        LOGD("Test dir is %s", testDir_.c_str());
        db_ = RdbTestUtils::CreateDataBase(storePath_);
        if (db_ == nullptr) {
            return;
        }
        CreateTable(db_);
        mgr_ = std::make_shared<RelationalStoreManager>("APP_ID", "USER_ID");
        RelationalStoreDelegate::Option option;
        mgr_->OpenStore(storePath_, "STORE_ID", option, delegate_);
        virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
        virtualAssetLoader_ = std::make_shared<VirtualAssetLoader>();
        delegate_->SetCloudDB(virtualCloudDb_);
        delegate_->SetIAssetLoader(virtualAssetLoader_);
    }

    void TearDown()
    {
        if (delegate_ != nullptr) {
            DBStatus errCode = mgr_->CloseStore(delegate_);
            LOGI("delegate close with errCode %d", static_cast<int>(errCode));
            delegate_ = nullptr;
        }
        if (db_ != nullptr) {
            int errCode = sqlite3_close_v2(db_);
            LOGI("sqlite close with errCode %d", errCode);
        }
        virtualCloudDb_ = nullptr;
        virtualAssetLoader_ = nullptr;
        if (DistributedDBToolsTest::RemoveTestDbFiles(testDir_) != 0) {
            LOGE("rm test db files error.");
        }
    }

    void BlockSync(SyncMode mode = SYNC_MODE_CLOUD_MERGE)
    {
        Query query = Query::Select().FromTable({ g_table });
        auto context = std::make_shared<CloudSyncContext>();
        auto callback = [context](const std::map<std::string, SyncProcess> &onProcess) {
            for (const auto &item : onProcess) {
                if (item.second.process == ProcessStatus::FINISHED) {
                    context->FinishAndNotify();
                }
            }
        };
        DBStatus status = delegate_->Sync({g_deviceCloud}, mode, query, callback, DBConstant::MAX_TIMEOUT);
        if (status != OK) {
            return;
        }
        context->WaitForFinish();
    }

    void NormalSync()
    {
        BlockSync();
        SetCloudDbSchema(delegate_);
        BlockSync();
    }

    void RandomModeSync(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            BlockSync();
            return;
        }
        auto mode = static_cast<SyncMode>(data[0]);
        LOGI("[RandomModeSync] select mode %d", static_cast<int>(mode));
        BlockSync(mode);
    }

    void DataChangeSync(const uint8_t* data, size_t size)
    {
        SetCloudDbSchema(delegate_);
        if (size == 0) {
            return;
        }
        InitDbData(db_, data, size);
        BlockSync();
    }

    void InitAssets(const uint8_t* data, size_t size)
    {
        FuzzerData fuzzerData(data, size);
        uint32_t len = fuzzerData.GetUInt32() % MOD;
        for (size_t i = 0; i <= size; ++i) {
            std::string nameStr = fuzzerData.GetString(len);
            localAssets_.push_back(GenAsset(nameStr, std::to_string(data[0])));
        }
    }

    void AssetChangeSync(const uint8_t* data, size_t size)
    {
        SetCloudDbSchema(delegate_);
        if (size == 0) {
            return;
        }
        InitAssets(data, size);
        InitDbAsset(data, size);
        BlockSync();
    }

    void RandomModeRemoveDeviceData(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        SetCloudDbSchema(delegate_);
        int64_t cloudCount = 10;
        int64_t paddingSize = 10;
        InsertCloudTableRecord(0, cloudCount, paddingSize, false);
        BlockSync();
        auto mode = static_cast<ClearMode>(data[0]);
        LOGI("[RandomModeRemoveDeviceData] select mode %d", static_cast<int>(mode));
        if (mode == DEFAULT) {
            return;
        }
        std::string device = "";
        delegate_->RemoveDeviceData(device, mode);
    }
private:
    std::string testDir_;
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_;
    std::shared_ptr<RelationalStoreManager> mgr_;
    Assets localAssets_;
};
CloudSyncTest *g_cloudSyncTest = nullptr;

void Setup()
{
    LOGI("Set up");
    g_cloudSyncTest = new(std::nothrow) CloudSyncTest();
    if (g_cloudSyncTest == nullptr) {
        return;
    }
    g_cloudSyncTest->SetUp();
}

void TearDown()
{
    LOGI("Tear down");
    g_cloudSyncTest->TearDown();
    if (g_cloudSyncTest != nullptr) {
        delete g_cloudSyncTest;
        g_cloudSyncTest = nullptr;
    }
}

void CombineTest(const uint8_t* data, size_t size)
{
    if (g_cloudSyncTest == nullptr) {
        return;
    }
    g_cloudSyncTest->NormalSync();
    g_cloudSyncTest->RandomModeSync(data, size);
    g_cloudSyncTest->DataChangeSync(data, size);
    g_cloudSyncTest->AssetChangeSync(data, size);
    g_cloudSyncTest->RandomModeRemoveDeviceData(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::Setup();
    OHOS::CombineTest(data, size);
    OHOS::TearDown();
    return 0;
}