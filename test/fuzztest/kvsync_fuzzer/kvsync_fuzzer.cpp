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

#include "kvsync_fuzzer.h"
#include "kv/kv_dbtypes.h"
#include "kv/kv_dbconstant.h"
#include "distributeddbvData_generate_unit_test.h"
#include "distributeddbtools_test.h"
#include "fuzzer_vData.h"
#include "kv_store_nb_kvDelegate.h"
#include "logprint.h"
#include "relational_store_kvDelegate.h"
#include "relational_store_manager.h"
#include "runtime_kvConfig.h"
#include "time_helper.h"
#include "virtual_assetTest_loader.h"
#include "virtual_kv_vData_translate.h"
#include "virtual_kv_db.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;

static constexpr const int MOD = 1000; // 1000 is mod
class KvSyncContext {
public:
    void WaitForFinishTest()
    {
        std::unique_lock<std::mutex> unique(mutex);
        LOGI("begin wait");
        cv.wait_for(unique, std::chrono::milliseconds(DBConstant::MAX_TIMEOUT), [this]() {
            return finished;
        });
        LOGI("end wait");
    }

    void FinishAndNotifyTest()
    {
        {
            std::lock_guard<std::mutex> autoLock(mutex);
            finished = true;
        }
        cv.notify_one();
    }

private:
    std::condition_variable cv;
    std::mutex mutex;
    bool finished = false;
};
class KvSyncTest {
public:
    static void ExecSqlFuzz(sqlite3 *db, const std::string &sql)
    {
        if (db == nullptr || sql.empty()) {
            return;
        }
        int errCodeTest = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        if (errCodeTest != SQLITE_OK) {
            LOGE("Execute sql failed. err: %d", errCodeTest);
        }
    }

    static void CreateTableFuzz(sqlite3 *&db)
    {
        ExecSqlFuzz(db, "PRAGMA journal_modeTest=WAL;");
        ExecSqlFuzz(db, gcreateLocalTableSql);
    }

    static void SetKvDbSchemaFuzz(RelationalStoreDelegate *kvDelegate)
    {
        DataBaseSchema vDataSchema;
        const std::vector<Field> kvFiled = {
            {"name", TYPE_INDEX<std::string>, true}, {"height", TYPE_INDEX<double>},
            {"married", TYPE_INDEX<bool>}, {"photoTest", TYPE_INDEX<Bytes>, false, false},
            {"assert", TYPE_INDEX<Asset>},  {"asserts", TYPE_INDEX<Assets>}, {"age", TYPE_INDEX<int64_t>}
        };
        TableSchema tableSchemaTest = {
            .name = gtable,
            .fieldTests = kvFiled
        };
        vDataSchema.tables.push_back(tableSchemaTest);
        kvDelegate->SetKvDbSchemaFuzz(vDataSchema);
        kvDelegate->CreateDistributedTable(gtable, CLOUD_COOPERATION);
    }

    static void InitDbData(sqlite3 *&db, const uint8_t *vData, size_t size)
    {
        sqlite3_stmt *stmt = nullptr;
        int errCodeTest = SQLiteUtils::GetStatement(db, ginsertLocalDataSql, stmt);
        if (errCodeTest != E_OK) {
            return;
        }
        FuzzerData fuzzerDataTest(vData, size);
        uint32_t len = fuzzerDataTest.GetUInt32() % MOD;
        for (size_t i = 0; i <= size; ++i) {
            std::string idStr = fuzzerDataTest.GetString(len);
            errCodeTest = SQLiteUtils::BindTextToStatement(stmt, 1, idStr);
            if (errCodeTest != E_OK) {
                break;
            }
            std::vector<uint8_t> photoTest = fuzzerDataTest.GetSequence(fuzzerDataTest.GetInt());
            errCodeTest = SQLiteUtils::BindBlobToStatement(stmt, 2, photoTest); // 2 is index of photoTest
            if (errCodeTest != E_OK) {
                break;
            }
            errCodeTest = SQLiteUtils::StepWithRetry(stmt);
            if (errCodeTest != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
                break;
            }
            SQLiteUtils::ResetStatement(stmt, false, errCodeTest);
        }
        SQLiteUtils::ResetStatement(stmt, true, errCodeTest);
    }

    static Asset GenAsset(const std::string &name, const std::string &hash)
    {
        Asset assetTest;
        assetTest.name = name;
        assetTest.hash = hash;
        return assetTest;
    }

    void InitDbAssetFuzz(const uint8_t *vData, size_t size)
    {
        if (vData == nullptr || size == 0) {
            return;
        }
        sqlite3_stmt *stmt = nullptr;
        int errCodeTest = SQLiteUtils::GetStatement(db, ginsertLocalAssetSql, stmt);
        if (errCodeTest != E_OK) {
            return;
        }
        FuzzerData fuzzerDataTest(vData, size);
        uint32_t len = fuzzerDataTest.GetUInt32() % MOD;
        for (size_t i = 0; i <= size; ++i) {
            std::string idStr = fuzzerDataTest.GetString(len);
            errCodeTest = SQLiteUtils::BindTextToStatement(stmt, 1, idStr);
            if (errCodeTest != E_OK) {
                break;
            }
            std::vector<uint8_t> photoTest = fuzzerDataTest.GetSequence(fuzzerDataTest.GetInt());
            errCodeTest = SQLiteUtils::BindBlobToStatement(stmt, 2, photoTest); // 2 is index of photoTest
            if (errCodeTest != E_OK) {
                break;
            }
            std::vector<uint8_t> assetTest;
            RuntimeContext::GetInstance()->AssetToBlob(localAssets[i % size], assetTest);
            errCodeTest = SQLiteUtils::BindBlobToStatement(stmt, 3, assetTest); // 3 is index of assert
            if (errCodeTest != E_OK) {
                break;
            }

            std::vector<Asset> assetTestVec(localAssets.begin() + (i % size), localAssets.end());
            RuntimeContext::GetInstance()->AssetsToBlob(assetTestVec, assetTest);
            errCodeTest = SQLiteUtils::BindBlobToStatement(stmt, 4, assetTest); // 4 is index of asserts
            if (errCodeTest != E_OK) {
                break;
            }
            errCodeTest = SQLiteUtils::StepWithRetry(stmt);
            if (errCodeTest != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
                break;
            }
            SQLiteUtils::ResetStatement(stmt, false, errCodeTest);
        }
        SQLiteUtils::ResetStatement(stmt, true, errCodeTest);
    }

    void InsertKvTableRecordFuzz(int64_t begin, int64_t count, int64_t photoTestSize, bool assetTestIsNull)
    {
        std::vector<uint8_t> photoTest(photoTestSize, 'v');
        std::vector<VBucket> record1;
        std::vector<VBucket> extend1;
        double randomHeight = 166.0; // 166.0 is random double value
        int64_t randomAge = 13L; // 13 is random int64_t value
        Timestamp now = TimeHelper::GetSysCurrentTime();
        for (int64_t i = begin; i < begin + count; ++i) {
            VBucket vData;
            vData.insert_or_assign("name", "Kv" + std::to_string(i));
            vData.insert_or_assign("height", randomHeight);
            vData.insert_or_assign("married", false);
            vData.insert_or_assign("photoTest", photoTest);
            vData.insert_or_assign("age", randomAge);
            Asset assetTest = gkvAsset;
            assetTest.name = assetTest.name + std::to_string(i);
            assetTestIsNull ? vData.insert_or_assign("assert", Nil()) : vData.insert_or_assign("assert", assetTest);
            record1.push_back(vData);
            VBucket log;
            log.insert_or_assign(KvDbConstant::CREATE_FIELD,
                static_cast<int64_t>(now / KvDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(KvDbConstant::MODIFY_FIELD,
                static_cast<int64_t>(now / KvDbConstant::TEN_THOUSAND + i));
            log.insert_or_assign(KvDbConstant::DELETE_FIELD, false);
            extend1.push_back(log);
        }
        examKvDb->BatchInsert(gtable, std::move(record1), extend1);
        LOGD("insert kv record worker1[primary key]:[kv%" PRId64 " - kv%" PRId64 ")", begin, count);
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void SetUp()
    {
        DistributedDBToolsTest::TestDirInit(testDir);
        storePathTest = testDir + "/" + gstoreId + gdbSuffix;
        LOGI("The test db is:%s", testDir.c_str());
        RuntimeConfig::SetKvTranslate(std::make_shared<VirtualKvDataTranslate>());
        LOGD("Test dir is %s", testDir.c_str());
        db = RdbTestUtils::CreateDataBase(storePathTest);
        if (db == nullptr) {
            return;
        }
        CreateTableFuzz(db);
        mgr = std::make_shared<RelationalStoreManager>("APP_ID", "USER_ID");
        RelationalStoreDelegate::Option optionTest;
        mgr->OpenStore(storePathTest, "STORE_ID", optionTest, kvDelegate);
        gKvSyncOption.modeTest = SyncMode::SYNC_MODE_CLOUD_MERGE;
        gKvSyncOption.users.push_back(DistributedDBUnitTest::USER_ID);
        gKvSyncOption.devices.push_back("kv");
        kvConfig.vDataDir = testDir;
        gmgr.SetKvStoreConfig(kvConfig);
        KvStoreNbDelegate::Option optionTest1;
        GetKvStoreFuzz(kvDelegatePtrS1, DistributedDBUnitTest::STORE_ID_1, optionTest1);
        KvStoreNbDelegate::Option optionTest2;
        GetKvStoreFuzz(kvDelegatePtrS2, DistributedDBUnitTest::STORE_ID_2, optionTest2);
        examKvDb = std::make_shared<VirtualKvDb>();
        examKvDb1 = std::make_shared<VirtualKvDb>();
        examKvDb2 = std::make_shared<VirtualKvDb>();
        virtualAssetLoader = std::make_shared<VirtualAssetLoader>();
        kvDelegate->SetKvDB(examKvDb);
        kvDelegate->SetIAssetLoader(virtualAssetLoader);
    }

    void TearDown()
    {
        if (kvDelegate != nullptr) {
            DBStatus errCodeTest = mgr->CloseStore(kvDelegate);
            LOGI("kvDelegate close with errCodeTest %d", static_cast<int>(errCodeTest));
            kvDelegate = nullptr;
        }
        if (db != nullptr) {
            int errCodeTest = sqlite3_close_v2(db);
            LOGI("sqlite close with errCodeTest %d", errCodeTest);
        }
        examKvDb = nullptr;
        CloseKvStoreFuzz(kvDelegatePtrS1, DistributedDBUnitTest::STORE_ID_1);
        CloseKvStoreFuzz(kvDelegatePtrS2, DistributedDBUnitTest::STORE_ID_2);
        examKvDb1 = nullptr;
        examKvDb2 = nullptr;
        virtualAssetLoader = nullptr;
        if (DistributedDBToolsTest::RemoveTestDbFiles(testDir) != 0) {
            LOGE("rm test db files error.");
        }
    }

    void KvBlockSyncFuzz(KvStoreNbDelegate *kvDelegate, DBStatus expectDBStatus, KvSyncOption optionTest,
        int expectSyncResult = OK)
    {
        if (kvDelegate == nullptr) {
            return;
        }
        std::mutex virDataMutex;
        std::condition_variable cv;
        bool finish = false;
        SyncProcess last;
        auto callback = [expectDBStatus, &last, &cv, &virDataMutex, &finish, &optionTest](
            const std::map<std::string, SyncProcess> &process) {
            size_t notifyCnt = 0;
            for (const auto &item : process) {
                LOGD("user = %s, dbStatus = %d", item.first.c_str(), item.second.process);
                if (item.second.process != DistributedDB::FINISHED) {
                    continue;
                }
                {
                    std::lock_guard<std::mutex> autoLock(virDataMutex);
                    notifyCnt++;
                    std::set<std::string> userSet(optionTest.users.begin(), optionTest.users.end());
                    if (notifyCnt == userSet.size()) {
                        finish = true;
                        last = item.second;
                        cv.notify_one();
                    }
                }
            }
        };
        auto result = kvDelegate->Sync(optionTest, callback);
        if (result == OK) {
            std::unique_lock<std::mutex> unique(virDataMutex);
            cv.wait(unique, [&finish]() {
                return finish;
            });
        }
        lastProcess = last;
    }

    static DataBaseSchema GetDataBaseSchema(bool invalidSchema)
    {
        DataBaseSchema schemaTest;
        TableSchema tableSchemaTest;
        tableSchemaTest.name = invalidSchema ? "invalid_schemaTest_name" : KvDbConstant::CLOUD_KV_TABLE_NAME;
        Field fieldTest;
        fieldTest.colName = KvDbConstant::CLOUD_KV_FIELD_KEY;
        fieldTest.type = TYPE_INDEX<std::string>;
        fieldTest.primary = true;
        fieldTest.colName = KvDbConstant::CLOUD_KV_FIELD_DEVICE;
        fieldTest.primary = false;
        tableSchemaTest.fieldTests.push_back(fieldTest);
        tableSchemaTest.fieldTests.push_back(fieldTest);
        fieldTest.colName = KvDbConstant::CLOUD_KV_FIELD_ORI_DEVICE;
        tableSchemaTest.fieldTests.push_back(fieldTest);
        fieldTest.colName = KvDbConstant::CLOUD_KV_FIELD_VALUE;
        tableSchemaTest.fieldTests.push_back(fieldTest);
        fieldTest.colName = KvDbConstant::CLOUD_KV_FIELD_DEVICE_CREATE_TIME;
        fieldTest.type = TYPE_INDEX<int64_t>;
        tableSchemaTest.fieldTests.push_back(fieldTest);
        schemaTest.tables.push_back(tableSchemaTest);
        return schemaTest;
    }

    void GetKvStoreFuzz(KvStoreNbDelegate *&kvDelegate, const std::string &storeId, KvDelegate::Option optionTest,
        int securityLabel = NOT_SET, bool invalidSchema = false)
    {
        DBStatus openRet = OK;
        optionTest.secOption.securityLabel = securityLabel;
        gmgr.GetKvStoreFuzz(storeId, optionTest, [&openRet, &kvDelegate](KvStoreNbDelegate *openDelegate) {
            openRet = dbStatus;
            kvDelegate = openDelegate;
        });
        if (kvDelegate == nullptr) {
            return;
        }
        std::map<std::string, DataBaseSchema> schemaTests;
        schemaTests[DistributedDBUnitTest::USER_ID] = GetDataBaseSchema(invalidSchema);
        schemaTests[USER_ID] = GetDataBaseSchema(invalidSchema);
        kvDelegate->SetKvDbSchemaFuzz(schemaTests);
        std::map<std::string, std::shared_ptr<IKvDb>> kvDbs;
        kvDbs[DistributedDBUnitTest::USER_ID] = examKvDb1;
        kvDbs[USER_ID] = examKvDb2;
        kvDelegate->SetKvDB(kvDbs);
    }

    static void CloseKvStoreFuzz(KvStoreNbDelegate *&kvDelegate, const std::string &storeId)
    {
        if (kvDelegate != nullptr) {
            gmgr.CloseKvStoreFuzz(kvDelegate);
            kvDelegate = nullptr;
            DBStatus dbStatus = gmgr.DeleteKvStore(storeId);
            LOGD("delete kv store dbStatus %d store %s", dbStatus, storeId.c_str());
        }
    }

    void BlockSyncFuzz(SyncMode modeTest = SYNC_MODE_CLOUD_MERGE)
    {
        Query dbQuery = Query::Select().FromTable({ gtable });
        auto kvContext = std::make_shared<KvSyncContext>();
        auto callback = [kvContext](const std::map<std::string, SyncProcess> &onProcess) {
            for (const auto &item : onProcess) {
                if (item.second.process == ProcessStatus::FINISHED) {
                    kvContext->FinishAndNotifyTest();
                }
            }
        };
        DBStatus dbStatus = kvDelegate->Sync({gdeviceKv}, modeTest, dbQuery, callback, DBConstant::MAX_TIMEOUT);
        if (dbStatus != OK) {
            return;
        }
        kvContext->WaitForFinishTest();
    }

    void OptionBlockSyncFuzz(const std::vector<std::string> &devices, SyncMode modeTest = SYNC_MODE_CLOUD_MERGE)
    {
        Query dbQuery = Query::Select().FromTable({ gtable });
        auto kvContext = std::make_shared<KvSyncContext>();
        auto callback = [kvContext](const std::map<std::string, SyncProcess> &onProcess) {
            for (const auto &item : onProcess) {
                if (item.second.process == ProcessStatus::FINISHED) {
                    kvContext->FinishAndNotifyTest();
                }
            }
        };
        KvSyncOption optionTest;
        optionTest.modeTest = modeTest;
        optionTest.devices = devices;
        optionTest.dbQuery = dbQuery;
        optionTest.waitTime = DBConstant::MAX_TIMEOUT;
        DBStatus dbStatus = kvDelegate->Sync(optionTest, callback);
        if (dbStatus != OK) {
            return;
        }
        kvContext->WaitForFinishTest();
    }

    void NormalSync()
    {
        BlockSyncFuzz();
        SetKvDbSchemaFuzz(kvDelegate);
        BlockSyncFuzz();
    }

    void KvNormalSync()
    {
        Key key = {'k'};
        Value expectValue = {'v'};
        kvDelegatePtrS1->Put(key, expectValue);
        KvBlockSyncFuzz(kvDelegatePtrS1, OK, gKvSyncOption);
        KvBlockSyncFuzz(kvDelegatePtrS2, OK, gKvSyncOption);
        Value actualValue;
        kvDelegatePtrS2->Get(key, actualValue);
    }

    void RandomModeSyncFuzz(const uint8_t *vData, size_t size)
    {
        if (size == 0) {
            BlockSyncFuzz();
            return;
        }
        auto modeTest = static_cast<SyncMode>(vData[0]);
        LOGI("[RandomModeSyncFuzz] select modeTest %d", static_cast<int>(modeTest));
        BlockSyncFuzz(modeTest);
    }

    void DataChangeSyncFuzz(const uint8_t *vData, size_t size)
    {
        SetKvDbSchemaFuzz(kvDelegate);
        if (size == 0) {
            return;
        }
        InitDbData(db, vData, size);
        BlockSyncFuzz();
        FuzzerData fuzzerDataTest(vData, size);
        KvSyncConfig kvConfig;
        int maxUploadSize11 = fuzzerDataTest.GetInt();
        int maxUploadCount = fuzzerDataTest.GetInt();
        kvConfig.maxUploadSize11 = maxUploadSize11;
        kvConfig.maxUploadCount = maxUploadCount;
        kvDelegate->SetKvSyncConfig(kvConfig);
        kvDelegatePtrS1->SetKvSyncConfig(kvConfig);
        uint32_t len = fuzzerDataTest.GetUInt32() % MOD;
        std::string version = fuzzerDataTest.GetString(len);
        kvDelegatePtrS1->SetGenKvVersionCallback([version](const std::string &origin) {
            return origin + version;
        });
        KvNormalSync();
        int count = fuzzerDataTest.GetInt();
        kvDelegatePtrS1->GetCount(Query::Select(), count);
        std::string device = fuzzerDataTest.GetString(len);
        kvDelegatePtrS1->GetKvVersion(device);
        std::vector<std::string> devices = fuzzerDataTest.GetStringVector(len);
        OptionBlockSyncFuzz(devices);
    }

    void InitAssetsFuzz(const uint8_t *vData, size_t size)
    {
        FuzzerData fuzzerDataTest(vData, size);
        uint32_t len = fuzzerDataTest.GetUInt32() % MOD;
        for (size_t i = 0; i <= size; ++i) {
            std::string nameStr = fuzzerDataTest.GetString(len);
            localAssets.push_back(GenAsset(nameStr, std::to_string(vData[0])));
        }
    }

    void AssetChangeSyncFuzz(const uint8_t *vData, size_t size)
    {
        SetKvDbSchemaFuzz(kvDelegate);
        if (size == 0) {
            return;
        }
        InitAssetsFuzz(vData, size);
        InitDbAssetFuzz(vData, size);
        BlockSyncFuzz();
    }

    void RandomModeRemoveDeviceDataFuzz(const uint8_t *vData, size_t size)
    {
        if (size == 0) {
            return;
        }
        SetKvDbSchemaFuzz(kvDelegate);
        int64_t kvCount = 10;
        int64_t paddingSize = 10;
        InsertKvTableRecordFuzz(0, kvCount, paddingSize, false);
        BlockSyncFuzz();
        auto modeTest = static_cast<ClearMode>(vData[0]);
        LOGI("[RandomModeRemoveDeviceDataFuzz] select modeTest %d", static_cast<int>(modeTest));
        if (modeTest == DEFAULT) {
            return;
        }
        std::string device = "";
        kvDelegate->RemoveDeviceData(device, modeTest);
    }
private:
    std::string testDir;
    std::string storePathTest;
    sqlite3 *db = nullptr;
    RelationalStoreDelegate *kvDelegate = nullptr;
    KvStoreNbDelegate* kvDelegatePtrS1 = nullptr;
    KvStoreNbDelegate* kvDelegatePtrS2 = nullptr;
    std::shared_ptr<VirtualKvDb> examKvDb;
    std::shared_ptr<VirtualKvDb> examKvDb1 = nullptr;
    std::shared_ptr<VirtualKvDb> examKvDb2 = nullptr;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader;
    std::shared_ptr<RelationalStoreManager> mgr;
    KvStoreConfig kvConfig;
    SyncProcess lastProcess;
    Assets localAssets;
};
KvSyncTest *g_kvSyncFuzzTest = nullptr;

void Setup()
{
    LOGI("Set up");
    g_kvSyncFuzzTest = new(std::nothrow) KvSyncTest();
    if (g_kvSyncFuzzTest == nullptr) {
        return;
    }
    g_kvSyncFuzzTest->SetUp();
}

void TearDown()
{
    LOGI("Tear down");
    g_kvSyncFuzzTest->TearDown();
    if (g_kvSyncFuzzTest != nullptr) {
        delete g_kvSyncFuzzTest;
        g_kvSyncFuzzTest = nullptr;
    }
}

void CombineFuzzTest(const uint8_t *vData, size_t size)
{
    if (g_kvSyncFuzzTest == nullptr) {
        return;
    }
    g_kvSyncFuzzTest->NormalSync();
    g_kvSyncFuzzTest->KvNormalSync();
    g_kvSyncFuzzTest->RandomModeSyncFuzz(vData, size);
    g_kvSyncFuzzTest->DataChangeSyncFuzz(vData, size);
    g_kvSyncFuzzTest->AssetChangeSyncFuzz(vData, size);
    g_kvSyncFuzzTest->RandomModeRemoveDeviceDataFuzz(vData, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *vData, size_t size)
{
    OHOS::Setup();
    OHOS::CombineFuzzTest(vData, size);
    OHOS::TearDown();
    return 0;
}
