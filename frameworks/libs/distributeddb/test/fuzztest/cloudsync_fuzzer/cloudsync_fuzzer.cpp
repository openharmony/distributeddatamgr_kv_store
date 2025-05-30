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
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer_data.h"
#include "kv_store_nb_delegate.h"
#include "log_print.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "time_helper.h"
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
KvStoreDelegateManager g_mgr(DistributedDBUnitTest::APP_ID, DistributedDBUnitTest::USER_ID);
CloudSyncOption g_CloudSyncOption;
const std::string USER_ID_2 = "user2";
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

    static void InitDbData(sqlite3 *&db, FuzzedDataProvider &fdp)
    {
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(db, g_insertLocalDataSql, stmt);
        if (errCode != E_OK) {
            return;
        }
        size_t size = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
        for (size_t i = 0; i <= size; ++i) {
            std::string idStr = fdp.ConsumeRandomLengthString();
            errCode = SQLiteUtils::BindTextToStatement(stmt, 1, idStr);
            if (errCode != E_OK) {
                break;
            }
            int photoSize = fdp.ConsumeIntegralInRange<int>(1, MOD);
            std::vector<uint8_t> photo = fdp.ConsumeBytes<uint8_t>(photoSize);
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

    void InitDbAsset(FuzzedDataProvider &fdp)
    {
        sqlite3_stmt *stmt = nullptr;
        int errCode = SQLiteUtils::GetStatement(db_, g_insertLocalAssetSql, stmt);
        if (errCode != E_OK) {
            return;
        }
        size_t size = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
        for (size_t i = 0; i <= size; ++i) {
            std::string idStr = fdp.ConsumeRandomLengthString();
            errCode = SQLiteUtils::BindTextToStatement(stmt, 1, idStr);
            if (errCode != E_OK) {
                break;
            }
            size_t photoSize = fdp.ConsumeIntegralInRange<size_t>(1, MOD);
            std::vector<uint8_t> photo = fdp.ConsumeBytes<uint8_t>(photoSize);
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
        g_CloudSyncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
        g_CloudSyncOption.users.push_back(DistributedDBUnitTest::USER_ID);
        g_CloudSyncOption.devices.push_back("cloud");
        config_.dataDir = testDir_;
        g_mgr.SetKvStoreConfig(config_);
        KvStoreNbDelegate::Option option1;
        GetKvStore(kvDelegatePtrS1_, DistributedDBUnitTest::STORE_ID_1, option1);
        KvStoreNbDelegate::Option option2;
        GetKvStore(kvDelegatePtrS2_, DistributedDBUnitTest::STORE_ID_2, option2);
        virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
        virtualCloudDb1_ = std::make_shared<VirtualCloudDb>();
        virtualCloudDb2_ = std::make_shared<VirtualCloudDb>();
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
        CloseKvStore(kvDelegatePtrS1_, DistributedDBUnitTest::STORE_ID_1);
        CloseKvStore(kvDelegatePtrS2_, DistributedDBUnitTest::STORE_ID_2);
        virtualCloudDb1_ = nullptr;
        virtualCloudDb2_ = nullptr;
        virtualAssetLoader_ = nullptr;
        if (DistributedDBToolsTest::RemoveTestDbFiles(testDir_) != 0) {
            LOGE("rm test db files error.");
        }
    }

    void KvBlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus, CloudSyncOption option,
        int expectSyncResult = OK)
    {
        if (delegate == nullptr) {
            return;
        }
        std::mutex dataMutex;
        std::condition_variable cv;
        bool finish = false;
        SyncProcess last;
        auto callback = [expectDBStatus, &last, &cv, &dataMutex, &finish, &option](
            const std::map<std::string, SyncProcess> &process) {
            size_t notifyCnt = 0;
            for (const auto &item : process) {
                LOGD("user = %s, status = %d", item.first.c_str(), item.second.process);
                if (item.second.process != DistributedDB::FINISHED) {
                    continue;
                }
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    notifyCnt++;
                    std::set<std::string> userSet(option.users.begin(), option.users.end());
                    if (notifyCnt == userSet.size()) {
                        finish = true;
                        last = item.second;
                        cv.notify_one();
                    }
                }
            }
        };
        auto result = delegate->Sync(option, callback);
        if (result == OK) {
            std::unique_lock<std::mutex> uniqueLock(dataMutex);
            cv.wait(uniqueLock, [&finish]() {
                return finish;
            });
        }
        lastProcess_ = last;
    }

    static DataBaseSchema GetDataBaseSchema(bool invalidSchema)
    {
        DataBaseSchema schema;
        TableSchema tableSchema;
        tableSchema.name = invalidSchema ? "invalid_schema_name" : CloudDbConstant::CLOUD_KV_TABLE_NAME;
        Field field;
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_KEY;
        field.type = TYPE_INDEX<std::string>;
        field.primary = true;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE;
        field.primary = false;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_VALUE;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE_CREATE_TIME;
        field.type = TYPE_INDEX<int64_t>;
        tableSchema.fields.push_back(field);
        schema.tables.push_back(tableSchema);
        return schema;
    }

    void GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId, KvStoreNbDelegate::Option option,
        int securityLabel = NOT_SET, bool invalidSchema = false)
    {
        DBStatus openRet = OK;
        option.secOption.securityLabel = securityLabel;
        g_mgr.GetKvStore(storeId, option, [&openRet, &delegate](DBStatus status, KvStoreNbDelegate *openDelegate) {
            openRet = status;
            delegate = openDelegate;
        });
        if (delegate == nullptr) {
            return;
        }
        std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
        cloudDbs[DistributedDBUnitTest::USER_ID] = virtualCloudDb1_;
        cloudDbs[USER_ID_2] = virtualCloudDb2_;
        delegate->SetCloudDB(cloudDbs);
        std::map<std::string, DataBaseSchema> schemas;
        schemas[DistributedDBUnitTest::USER_ID] = GetDataBaseSchema(invalidSchema);
        schemas[USER_ID_2] = GetDataBaseSchema(invalidSchema);
        delegate->SetCloudDbSchema(schemas);
    }

    static void CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId)
    {
        if (delegate != nullptr) {
            g_mgr.CloseKvStore(delegate);
            delegate = nullptr;
            DBStatus status = g_mgr.DeleteKvStore(storeId);
            LOGD("delete kv store status %d store %s", status, storeId.c_str());
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

    void OptionBlockSync(const std::vector<std::string> &devices, SyncMode mode = SYNC_MODE_CLOUD_MERGE)
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
        CloudSyncOption option;
        option.mode = mode;
        option.devices = devices;
        option.query = query;
        option.waitTime = DBConstant::MAX_TIMEOUT;
        DBStatus status = delegate_->Sync(option, callback);
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

    void KvNormalSync()
    {
        Key key = {'k'};
        Value expectValue = {'v'};
        kvDelegatePtrS1_->Put(key, expectValue);
        KvBlockSync(kvDelegatePtrS1_, OK, g_CloudSyncOption);
        KvBlockSync(kvDelegatePtrS2_, OK, g_CloudSyncOption);
        Value actualValue;
        kvDelegatePtrS2_->Get(key, actualValue);
    }

    void RandomModeSync(FuzzedDataProvider &fdp)
    {
        size_t syncModeLen = sizeof(SyncMode);
        auto mode = static_cast<SyncMode>(fdp.ConsumeIntegralInRange<uint32_t>(0, syncModeLen));
        LOGI("[RandomModeSync] select mode %d", static_cast<int>(mode));
        BlockSync(mode);
    }

    void DataChangeSync(FuzzedDataProvider &fdp)
    {
        SetCloudDbSchema(delegate_);
        InitDbData(db_, fdp);
        BlockSync();
        CloudSyncConfig config;
        int maxUploadSize = fdp.ConsumeIntegral<int>();
        int maxUploadCount = fdp.ConsumeIntegral<int>();
        config.maxUploadSize = maxUploadSize;
        config.maxUploadCount = maxUploadCount;
        delegate_->SetCloudSyncConfig(config);
        kvDelegatePtrS1_->SetCloudSyncConfig(config);
        uint32_t len = fdp.ConsumeIntegralInRange<uint32_t>(0, MOD);
        std::string version = fdp.ConsumeRandomLengthString();
        kvDelegatePtrS1_->SetGenCloudVersionCallback([version](const std::string &origin) {
            return origin + version;
        });
        KvNormalSync();
        int count = fdp.ConsumeIntegral<int>();
        kvDelegatePtrS1_->GetCount(Query::Select(), count);
        std::string device = fdp.ConsumeRandomLengthString();
        kvDelegatePtrS1_->GetCloudVersion(device);
        std::vector<std::string> devices;
        for (uint32_t i = 0; i< len; i++) {
            devices.push_back(fdp.ConsumeRandomLengthString());
        }
        OptionBlockSync(devices);
    }

    void InitAssets(FuzzedDataProvider &fdp)
    {
        size_t size = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
        for (size_t i = 0; i <= size; ++i) {
            std::string nameStr = fdp.ConsumeRandomLengthString(MOD);
            localAssets_.push_back(GenAsset(nameStr, fdp.ConsumeBytesAsString(1)));
        }
    }

    void AssetChangeSync(FuzzedDataProvider &fdp)
    {
        SetCloudDbSchema(delegate_);
        InitAssets(fdp);
        InitDbAsset(fdp);
        BlockSync();
    }

    void RandomModeRemoveDeviceData(FuzzedDataProvider &fdp)
    {
        SetCloudDbSchema(delegate_);
        int64_t cloudCount = 10;
        int64_t paddingSize = 10;
        InsertCloudTableRecord(0, cloudCount, paddingSize, false);
        BlockSync();
        size_t modeLen = sizeof(ClearMode);
        auto mode = static_cast<ClearMode>(fdp.ConsumeIntegralInRange<uint32_t>(0, modeLen));
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
    KvStoreNbDelegate* kvDelegatePtrS1_ = nullptr;
    KvStoreNbDelegate* kvDelegatePtrS2_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb1_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb2_ = nullptr;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_;
    std::shared_ptr<RelationalStoreManager> mgr_;
    KvStoreConfig config_;
    SyncProcess lastProcess_;
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

void CombineTest(FuzzedDataProvider &fdp)
{
    if (g_cloudSyncTest == nullptr) {
        return;
    }
    g_cloudSyncTest->NormalSync();
    g_cloudSyncTest->KvNormalSync();
    g_cloudSyncTest->RandomModeSync(fdp);
    g_cloudSyncTest->DataChangeSync(fdp);
    g_cloudSyncTest->AssetChangeSync(fdp);
    g_cloudSyncTest->RandomModeRemoveDeviceData(fdp);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::Setup();
    FuzzedDataProvider fdp(data, size);
    OHOS::CombineTest(fdp);
    OHOS::TearDown();
    return 0;
}
