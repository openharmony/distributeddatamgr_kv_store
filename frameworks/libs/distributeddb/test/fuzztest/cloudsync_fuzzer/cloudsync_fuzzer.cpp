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
#include "cloud_db_types.h"
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
    "height REAL ," \
    "married BOOLEAN ," \
    "photo BLOB NOT NULL," \
    "assert BLOB," \
    "age INT);";
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
            LOGE("Execute sql failed. %d err: %s", errCode);
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
            {"assert", TYPE_INDEX<Asset>}, {"age", TYPE_INDEX<int64_t>}
        };
        TableSchema tableSchema = {
            .name = g_table,
            .fields = cloudFiled
        };
        dataBaseSchema.tables.push_back(tableSchema);
        delegate->SetCloudDbSchema(dataBaseSchema);
        delegate->CreateDistributedTable(g_table, CLOUD_COOPERATION);
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
private:
    std::string testDir_;
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_;
    std::shared_ptr<RelationalStoreManager> mgr_;
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