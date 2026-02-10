/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <filesystem>
#include <sys/stat.h>

#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "kvdb_properties.h"
#include "log_print.h"
#include "single_ver_database_oper.h"
#include "storage_engine_manager.h"
#include "platform_specific.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace testing;

namespace {
class DistributedDBStorageSingleVerDatabaseOperTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
protected:
    std::string testDir_;
    std::string currentDir_;
    std::string unpackedDir_;
    std::vector<std::string> unpackedDirs_;
    SQLiteSingleVerNaturalStore *singleVerNaturalStore_ = nullptr;
    SQLiteSingleVerStorageEngine *singleVerStorageEngine_ = nullptr;
};

void DistributedDBStorageSingleVerDatabaseOperTest::SetUpTestCase(void)
{
}

void DistributedDBStorageSingleVerDatabaseOperTest::TearDownTestCase(void)
{
}

void DistributedDBStorageSingleVerDatabaseOperTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    DistributedDBToolsUnitTest::TestDirInit(testDir_);
    singleVerNaturalStore_ = new (std::nothrow) SQLiteSingleVerNaturalStore();
    ASSERT_NE(singleVerNaturalStore_, nullptr);
    KvDBProperties property;
    property.SetStringProp(KvDBProperties::DATA_DIR, "");
    property.SetStringProp(KvDBProperties::STORE_ID, "TestDatabaseOper");
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, "TestDatabaseOper");
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
    int errCode = E_OK;
    singleVerStorageEngine_ =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(singleVerStorageEngine_, nullptr);
    currentDir_ = this->testDir_ + "/current";
    EXPECT_EQ(mkdir(currentDir_.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)), E_OK);
    this->unpackedDirs_.push_back(currentDir_);
    unpackedDir_ = this->testDir_ + "/unpacked_old";
    EXPECT_EQ(mkdir(unpackedDir_.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)), E_OK);
    this->unpackedDirs_.push_back(unpackedDir_);
}

void DistributedDBStorageSingleVerDatabaseOperTest::TearDown(void)
{
    for (const auto& dir : unpackedDirs_) {
        if (std::filesystem::exists(dir)) {
            std::filesystem::remove_all(dir);
        }
    }
    delete singleVerNaturalStore_;
    singleVerNaturalStore_ = nullptr;
    singleVerStorageEngine_ = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != E_OK) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: DatabaseOperationTest001
 * @tc.desc: test Rekey, Import and Export interface in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, DatabaseOperationTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create singleVerDatabaseOper with nullptr;
     * @tc.expected: OK.
     */
    std::unique_ptr<SingleVerDatabaseOper> singleVerDatabaseOper =
        std::make_unique<SingleVerDatabaseOper>(singleVerNaturalStore_, nullptr);
    ASSERT_NE(singleVerDatabaseOper, nullptr);

    /**
     * @tc.steps: step2. Test Rekey, Import and Export interface;
     * @tc.expected: OK.
     */
    CipherPassword passwd;
    EXPECT_EQ(singleVerDatabaseOper->Rekey(passwd), -E_INVALID_DB);
    EXPECT_EQ(singleVerDatabaseOper->Import("", passwd), -E_INVALID_DB);
    EXPECT_EQ(singleVerDatabaseOper->Export("", passwd), -E_INVALID_DB);
    KvDBProperties properties;
    EXPECT_EQ(singleVerDatabaseOper->RekeyRecover(properties), -E_INVALID_ARGS);
    EXPECT_EQ(singleVerDatabaseOper->ClearImportTempFile(properties), -E_INVALID_ARGS);
    EXPECT_EQ(singleVerDatabaseOper->ClearExportedTempFiles(properties), -E_INVALID_ARGS);
}

/**
 * @tc.name: DatabaseOperationTest002
 * @tc.desc: test Import and Export interface in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, DatabaseOperationTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create singleVerDatabaseOper;
     * @tc.expected: OK.
     */
    std::unique_ptr<SingleVerDatabaseOper> singleVerDatabaseOper =
        std::make_unique<SingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(singleVerDatabaseOper, nullptr);

    /**
     * @tc.steps: step2. Test Import and Export interface;
     * @tc.expected: OK.
     */
    CipherPassword passwd;
    EXPECT_EQ(singleVerDatabaseOper->Export("", passwd), -E_NOT_INIT);
    singleVerDatabaseOper->SetLocalDevId("device");
    EXPECT_EQ(singleVerDatabaseOper->Export("", passwd), -E_INVALID_ARGS);
}

/**
 * @tc.name: DatabaseOperationTest003
 * @tc.desc: test RecoverPrehandle
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, DatabaseOperationTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create singleVerDatabaseOper;
     * @tc.expected: OK.
     */
    std::unique_ptr<SingleVerDatabaseOper> singleVerDatabaseOper =
        std::make_unique<SingleVerDatabaseOper>(singleVerNaturalStore_, nullptr);
    ASSERT_NE(singleVerDatabaseOper, nullptr);
    /**
     * @tc.steps: step2. Create validProperty;
     * @tc.expected: OK.
     */
    KvDBProperties validProperty;
    validProperty.SetStringProp(KvDBProperties::DATA_DIR, testDir_);
    validProperty.SetStringProp(KvDBProperties::STORE_ID, "TestRecoverPrehandleError");
    validProperty.SetStringProp(KvDBProperties::IDENTIFIER_DIR, "TestRecoverPrehandleError");
    validProperty.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
    /**
     * @tc.steps: step3. Prepare file;
     * @tc.expected: OK.
     */
    std::string dbSubDir = KvDBProperties::GetStoreSubDirectory
        (validProperty.GetIntProp(KvDBProperties::DATABASE_TYPE, 0));
    std::string workDir = testDir_ + "/" + validProperty.GetStringProp(KvDBProperties::IDENTIFIER_DIR, "");
    std::string preCtrlFileName = workDir + "/" + dbSubDir + DBConstant::REKEY_FILENAME_POSTFIX_PRE;
    std::string backupDir = workDir + "/" + dbSubDir + DBConstant::PATH_BACKUP_POSTFIX;
    /**
     * @tc.steps: step4. Create the preCtrlFile and backupDir;
     * @tc.expected: OK.
     */
    EXPECT_EQ(OS::MakeDBDirectory(workDir), E_OK);
    EXPECT_EQ(OS::MakeDBDirectory(workDir + "/" + dbSubDir), E_OK);
    EXPECT_EQ(OS::MakeDBDirectory(backupDir), E_OK);
    std::string testFile = backupDir + "/test.txt";
    DistributedDB::OS::FileHandle* fileHandle = nullptr;
    DistributedDB::OS::FileHandle* preCtrlHandle = nullptr;
    EXPECT_EQ(OS::OpenFile(testFile, fileHandle), E_OK);
    if (fileHandle != nullptr) {
        OS::CloseFile(fileHandle);
        fileHandle = nullptr;
    }
    EXPECT_EQ(OS::OpenFile(preCtrlFileName, preCtrlHandle), E_OK);
    if (preCtrlHandle != nullptr) {
        OS::CloseFile(preCtrlHandle);
        preCtrlHandle = nullptr;
    }
    /**
     * @tc.steps: step5. Set backupDir read only;
     * @tc.expected: OK.
     */
    EXPECT_EQ(chmod(backupDir.c_str(), (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    if (access(backupDir.c_str(), W_OK) == 0) {
        LOGD("Modifying permissions is ineffective for execution\n");
        EXPECT_EQ(chmod(backupDir.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    } else {
        EXPECT_EQ(singleVerDatabaseOper->RekeyRecover(validProperty), -E_REMOVE_FILE);
        EXPECT_EQ(chmod(backupDir.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
        EXPECT_EQ(singleVerDatabaseOper->RekeyRecover(validProperty), E_OK);
    }
}

/**
 * @tc.name: DatabaseOperationTest004
 * @tc.desc: Test Rekey, Import and Export interface in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, DatabaseOperationTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create singleVerDatabaseOper with singleVerNaturalStore_ nullptr;
     * @tc.expected: OK.
     */
    std::unique_ptr<SingleVerDatabaseOper> singleVerDatabaseOper =
        std::make_unique<SingleVerDatabaseOper>(nullptr, singleVerStorageEngine_);
    ASSERT_NE(singleVerDatabaseOper, nullptr);

    /**
     * @tc.steps: step2. Test Rekey, Import and Export interface;
     * @tc.expected: E_INVALID_DB.
     */
    CipherPassword passwd;
    EXPECT_EQ(singleVerDatabaseOper->Rekey(passwd), -E_INVALID_DB);
    EXPECT_EQ(singleVerDatabaseOper->Import("", passwd), -E_INVALID_DB);
    EXPECT_EQ(singleVerDatabaseOper->Export("", passwd), -E_INVALID_DB);
}

class TestableSingleVerDatabaseOper : public DistributedDB::SingleVerDatabaseOper {
public:
    using SingleVerDatabaseOper::RekeyPreHandle;
    using SingleVerDatabaseOper::ImportUnpackedDatabase;
    using SingleVerDatabaseOper::BackupDb;
    using SingleVerDatabaseOper::CloseStorages;
    using SingleVerDatabaseOper::ExportAllDatabases;
    TestableSingleVerDatabaseOper(DistributedDB::SQLiteSingleVerNaturalStore *naturalStore,
        DistributedDB::SQLiteStorageEngine *storageEngine) : SingleVerDatabaseOper(naturalStore, storageEngine) {}
};

/**
 * @tc.name: BackupDbTest001
 * @tc.desc: test BackupDb function in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, BackupDbTest001, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    CipherPassword passwd;
    EXPECT_EQ(testSingleVerDatabaseOper->BackupDb(passwd), -E_INVALID_ARGS);
}

/**
 * @tc.name: CloseStoragesTest001
 * @tc.desc: test CloseStorages function in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, CloseStoragesTest001, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    EXPECT_EQ(testSingleVerDatabaseOper->CloseStorages(), -E_INVALID_ARGS);
}

/**
 * @tc.name: ExportMainDBTest001
 * @tc.desc: test ExportMainDB function in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ExportMainDBTest001, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string dbDir = "invalidDbDir";
    CipherPassword passwd;
    EXPECT_EQ(testSingleVerDatabaseOper->ExportAllDatabases(currentDir_, passwd, dbDir), -E_SQLITE_CANT_OPEN);
}

/**
 * @tc.name: ImportUnpackedDatabaseTest001
 * @tc.desc: ClearCurrentDatabase failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ImportUnpackedDatabaseTest001, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string current = this->testDir_ + "/nonexist";
    CipherPassword passwd;
    DistributedDB::ImportFileInfo info;
    info.currentDir = current;
    info.unpackedDir = unpackedDir_;
    EXPECT_EQ(testSingleVerDatabaseOper->ImportUnpackedDatabase(info, passwd, true), -E_SYSTEM_API_FAIL);
}

/**
 * @tc.name: ImportUnpackedMainDatabaseTest001
 * @tc.desc: Test OldMainDbExisted and export the unpacked old version(<3) main database to current error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ImportUnpackedMainDatabaseTest001, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string oldMainFile = unpackedDir_ + "/gen_natural_store.db";
    std::ofstream outFile(oldMainFile.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(outFile.is_open());
    outFile.close();
    CipherPassword passwd;
    DistributedDB::ImportFileInfo info;
    info.currentDir = currentDir_;
    info.unpackedDir = unpackedDir_;
    EXPECT_EQ(testSingleVerDatabaseOper->ImportUnpackedDatabase(info, passwd, true), -E_INVALID_FILE);
}

/**
 * @tc.name: ImportUnpackedMainDatabaseTest002
 * @tc.desc: Test MainDbExisted and export the unpacked main database to current error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ImportUnpackedMainDatabaseTest002, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string mainDir = unpackedDir_ + "main";
    EXPECT_EQ(mkdir(mainDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)), E_OK);
    this->unpackedDirs_.push_back(mainDir);
    std::string mainFile = mainDir + "/gen_natural_store.db";
    std::ofstream outFile(mainFile.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(outFile.is_open());
    outFile.close();
    CipherPassword passwd;
    DistributedDB::ImportFileInfo info;
    info.currentDir = currentDir_;
    info.unpackedDir = unpackedDir_;
    EXPECT_EQ(testSingleVerDatabaseOper->ImportUnpackedDatabase(info, passwd, true), -E_INVALID_FILE);
}

/**
 * @tc.name: ImportUnpackedMainDatabaseTest003
 * @tc.desc: Test MainDbExisted and check main file integrity error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ImportUnpackedMainDatabaseTest003, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string mainDir = unpackedDir_ + "main";
    EXPECT_EQ(mkdir(mainDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)), E_OK);
    this->unpackedDirs_.push_back(mainDir);
    std::string mainFile = mainDir + "/gen_natural_store.db";
    std::ofstream outFile(mainFile.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(outFile.is_open());
    outFile.close();
    EXPECT_EQ(chmod(mainFile.c_str(), S_IRUSR | S_IRGRP | S_IROTH), E_OK);
    CipherPassword passwd;
    DistributedDB::ImportFileInfo info;
    info.currentDir = currentDir_;
    info.unpackedDir = unpackedDir_;
    EXPECT_EQ(testSingleVerDatabaseOper->ImportUnpackedDatabase(info, passwd, true), -E_INVALID_FILE);
}

/**
 * @tc.name: ImportUnpackedMainDatabaseTest004
 * @tc.desc: Test OldMainDbExisted and check main file integrity error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ImportUnpackedMainDatabaseTest004, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string oldMainFile = unpackedDir_ + "/gen_natural_store.db";
    std::ofstream outFile(oldMainFile.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(outFile.is_open());
    outFile.close();
    EXPECT_EQ(chmod(oldMainFile.c_str(), S_IRUSR | S_IRGRP | S_IROTH), E_OK);
    CipherPassword passwd;
    DistributedDB::ImportFileInfo info;
    info.currentDir = currentDir_;
    info.unpackedDir = unpackedDir_;
    EXPECT_EQ(testSingleVerDatabaseOper->ImportUnpackedDatabase(info, passwd, true), -E_INVALID_FILE);
}

/**
 * @tc.name: ImportUnpackedMainDatabaseTest005
 * @tc.desc: Test OldMainDbExisted and no need to check integrity.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ImportUnpackedMainDatabaseTest005, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string oldMainFile = unpackedDir_ + "/gen_natural_store.db";
    std::ofstream outFile(oldMainFile.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(outFile.is_open());
    outFile.close();
    CipherPassword passwd;
    DistributedDB::ImportFileInfo info;
    info.currentDir = currentDir_;
    info.unpackedDir = unpackedDir_;
    EXPECT_EQ(testSingleVerDatabaseOper->ImportUnpackedDatabase(info, passwd, false), -E_INVALID_FILE);
}

/**
 * @tc.name: ImportUnpackedMetaDatabaseTest001
 * @tc.desc: Test ImportUnpackedMetaDatabase func.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, ImportUnpackedMetaDatabaseTest001, TestSize.Level0)
{
    std::unique_ptr<TestableSingleVerDatabaseOper> testSingleVerDatabaseOper =
        std::make_unique<TestableSingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(testSingleVerDatabaseOper, nullptr);
    std::string metaDir = unpackedDir_ + "meta";
    EXPECT_EQ(mkdir(metaDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)), E_OK);
    this->unpackedDirs_.push_back(metaDir);
    std::string metaFile = metaDir + "/meta.db";
    std::ofstream outFile(metaFile.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(outFile.is_open());
    outFile.close();
    CipherPassword passwd;
    DistributedDB::ImportFileInfo info;
    info.currentDir = currentDir_;
    info.unpackedDir = unpackedDir_;
    EXPECT_EQ(testSingleVerDatabaseOper->ImportUnpackedDatabase(info, passwd, false), -E_INVALID_FILE);
}

/**
 * @tc.name: GetStoreSubDirectoryTest001
 * @tc.desc: test different type
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, GetStoreSubDirectoryTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Change Type;
     * @tc.expected: LOCAL_SUB_DIR.
     */
    int type = DistributedDB::KvDBProperties::LOCAL_TYPE_SQLITE;
    EXPECT_EQ(KvDBProperties::GetStoreSubDirectory(type), DBConstant::LOCAL_SUB_DIR);
}

/**
 * @tc.name: GetStoreSubDirectoryTest002
 * @tc.desc: test different type
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, GetStoreSubDirectoryTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Change Type;
     * @tc.expected: MULTI_SUB_DIR.
     */
    int type = DistributedDB::KvDBProperties::MULTI_VER_TYPE_SQLITE;
    EXPECT_EQ(KvDBProperties::GetStoreSubDirectory(type), DBConstant::MULTI_SUB_DIR);
}

/**
 * @tc.name: GetStoreSubDirectoryTest003
 * @tc.desc: test different type
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, GetStoreSubDirectoryTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Change Type;
     * @tc.expected: unknown.
     */
    int type = 404;
    EXPECT_EQ(KvDBProperties::GetStoreSubDirectory(type), "unknown");
}
}