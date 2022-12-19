/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "BackupManagerTest"
#include <condition_variable>
#include <gtest/gtest.h>
#include <vector>

#include "dev_manager.h"
#include "store_manager.h"
#include "store_util.h"
#include "sys/stat.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class BackupManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    Status DeleteBackUpFiles(std::shared_ptr<SingleKvStore> kvStore, std::string baseDir, StoreId storeId);
    void MkdirPath(std::string baseDir, AppId appId, StoreId storeId);

    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, std::string appIdTest, std::string baseDir,
        KvStoreType type, bool encrypt);
    std::shared_ptr<SingleKvStore> kvStore_;
};

void BackupManagerTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/BackupManagerTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    mkdir((baseDir + "/key").c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void BackupManagerTest::TearDownTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/BackupManagerTest";
    StoreManager::GetInstance().Delete({ "BackupManagerTest" }, { "SingleKVStore" }, baseDir);

    (void)remove("/data/service/el1/public/database/BackupManagerTest/key");
    (void)remove("/data/service/el1/public/database/BackupManagerTest/kvdb");
    (void)remove("/data/service/el1/public/database/BackupManagerTest");
}

void BackupManagerTest::SetUp(void)
{
    std::string baseDir = "/data/service/el1/public/database/BackupManagerTest";
    kvStore_ = CreateKVStore("SingleKVStore", "BackupManagerTest", baseDir, SINGLE_VERSION, false);
    if (kvStore_ == nullptr) {
        kvStore_ = CreateKVStore("SingleKVStore", "BackupManagerTest", baseDir, SINGLE_VERSION, false);
    }
    ASSERT_NE(kvStore_, nullptr);
}

void BackupManagerTest::TearDown(void)
{
    AppId appId = { "BackupManagerTest" };
    StoreId storeId = { "SingleKVStore" };
    std::string baseDir = "/data/service/el1/public/database/BackupManagerTest";
    auto status = DeleteBackUpFiles(kvStore_, baseDir, storeId);
    ASSERT_EQ(status, SUCCESS);
    kvStore_ = nullptr;
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

std::shared_ptr<SingleKvStore> BackupManagerTest::CreateKVStore(std::string storeIdTest, std::string appIdTest,
    std::string baseDir, KvStoreType type, bool encrypt)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.encrypt = encrypt;
    options.area = EL1;
    options.baseDir = baseDir;

    AppId appId = { appIdTest };
    StoreId storeId = { storeIdTest };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
}
Status BackupManagerTest::DeleteBackUpFiles(std::shared_ptr<SingleKvStore> kvStore, string baseDir, StoreId storeId)
{
    std::vector<string> backupnames;
    auto files = StoreUtil::GetFiles(baseDir + "/kvdb/backup/" + storeId.storeId);
    for (auto file : files) {
        auto filename = file.name.substr(0, file.name.length() - 4);
        backupnames.emplace_back(filename);
    }
    if (backupnames.empty()) {
        return SUCCESS;
    }
    std::map<std::string, OHOS::DistributedKv::Status> results;
    return kvStore->DeleteBackup(backupnames, baseDir, results);
}
void BackupManagerTest::MkdirPath(std::string baseDir, AppId appId, StoreId storeId)
{
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    mkdir((baseDir + "/key").c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    mkdir((baseDir + "/kvdb").c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    mkdir((baseDir + "/kvdb/backup").c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    std::string backUpToPath = { baseDir + "/kvdb/backup/" + storeId.storeId };
    mkdir(backUpToPath.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    std::string backUpFile = backUpToPath + "/" + "testbackup.bak";
    auto fl = OHOS::DistributedKv::StoreUtil::CreateFile(backUpFile);
    ASSERT_EQ(fl, true);
}

/**
* @tc.name: BackUp
* @tc.desc: the kvstore back up
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, BackUp, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    auto status = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
}
/**
* @tc.name: BackUpInvalidArguments
* @tc.desc: the kvstore back up and the arguments is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, BackUpInvalidArguments, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto baseDir = "";
    auto baseDir1 = "/data/service/el1/public/database/BackupManagerTest";
    auto status = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status, INVALID_ARGUMENT);
    status = kvStore_->Backup("", baseDir);
    ASSERT_EQ(status, INVALID_ARGUMENT);
    status = kvStore_->Backup("autoBackup", baseDir1);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}
/**
* @tc.name: BackUpSameFile
* @tc.desc: the kvstore back up the same file
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, BackUpSameFile, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    auto status = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
}
/**
* @tc.name: ReStore
* @tc.desc: the kvstore ReStore
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, ReStore, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto status = kvStore_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore_->Get({ "Put Test" }, value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(std::string("Put Value"), value.ToString());

    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    auto baseDir1 = "";
    status = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Delete("Put Test");
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Restore("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
    value = {};
    status = kvStore_->Get("Put Test", value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(std::string("Put Value"), value.ToString());
    status = kvStore_->Restore("testbackup", baseDir1);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}
/**
* @tc.name: DeleteBackup
* @tc.desc: the kvstore DeleteBackup
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, DeleteBackup, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    std::string file1 = "testbackup1";
    std::string file2 = "testbackup2";
    std::string file3 = "testbackup3";
    std::string file4 = "autoBackup";
    kvStore_->Backup(file1, baseDir);
    kvStore_->Backup(file2, baseDir);
    kvStore_->Backup(file3, baseDir);
    vector<std::string> files = { file1, file2, file3, file4 };
    std::map<std::string, OHOS::DistributedKv::Status> results;
    auto status = kvStore_->DeleteBackup(files, baseDir, results);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->DeleteBackup(files, "", results);
    ASSERT_EQ(status, INVALID_ARGUMENT);
}
/**
* @tc.name: RollbackKey
* @tc.desc: rollback the key
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, RollbackKey, TestSize.Level0)
{
    AppId appId = { "BackupManagerTestRollBackKey" };
    StoreId storeId = { "SingleKVStoreEncrypt" };
    std::string baseDir = "/data/service/el1/public/database/" + appId.appId;
    MkdirPath(baseDir, appId, storeId);

    std::string buildKeyFile = "Prefix_backup_SingleKVStoreEncrypt_testbackup.key";
    std::string path = { baseDir + "/key/" + buildKeyFile + ".bk" };
    auto fl = OHOS::DistributedKv::StoreUtil::CreateFile(path);
    ASSERT_EQ(fl, true);

    std::string autoBackUpFile = baseDir + "/kvdb/backup/" + storeId.storeId + "/" + "autoBackup.bak";
    fl = OHOS::DistributedKv::StoreUtil::CreateFile(autoBackUpFile);
    ASSERT_EQ(fl, true);

    std::shared_ptr<SingleKvStore> kvStoreEncrypt;
    kvStoreEncrypt = CreateKVStore(storeId.storeId, appId.appId, baseDir, SINGLE_VERSION, true);
    ASSERT_NE(kvStoreEncrypt, nullptr);

    auto files = StoreUtil::GetFiles(baseDir + "/kvdb/backup/" + storeId.storeId);
    auto keyfiles = StoreUtil::GetFiles(baseDir + "/key");
    bool keyFlag = false;
    for (auto keyfile : keyfiles) {
        auto keyfilename = keyfile.name;
        if (keyfilename == buildKeyFile + ".bk") {
            keyFlag = true;
        }
    }
    ASSERT_EQ(keyFlag, false);

    auto status = DeleteBackUpFiles(kvStoreEncrypt, baseDir, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: RollbackData
* @tc.desc: rollback the data
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, RollbackData, TestSize.Level0)
{
    AppId appId = { "BackupManagerTestRollBackData" };
    StoreId storeId = { "SingleKVStoreEncrypt" };
    std::string baseDir = "/data/service/el1/public/database/" + appId.appId;
    MkdirPath(baseDir, appId, storeId);

    std::string buildBackUpFile = baseDir + "/kvdb/backup/" + storeId.storeId + "/" + "testbackup.bak.bk";
    auto fl = OHOS::DistributedKv::StoreUtil::CreateFile(buildBackUpFile);
    ASSERT_EQ(fl, true);

    std::shared_ptr<SingleKvStore> kvStoreEncrypt;
    kvStoreEncrypt = CreateKVStore(storeId.storeId, appId.appId, baseDir, SINGLE_VERSION, true);
    ASSERT_NE(kvStoreEncrypt, nullptr);

    auto files = StoreUtil::GetFiles(baseDir + "/kvdb/backup/" + storeId.storeId);
    bool dataFlag = false;
    for (auto datafile : files) {
        auto datafilename = datafile.name;
        if (datafilename == "testbackup.bak.bk") {
            dataFlag = true;
        }
    }
    ASSERT_EQ(dataFlag, false);

    auto status = DeleteBackUpFiles(kvStoreEncrypt, baseDir, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: Rollback
* @tc.desc: rollback the key
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, Rollback, TestSize.Level0)
{
    AppId appId = { "BackupManagerTestRollBack" };
    StoreId storeId = { "SingleKVStoreEncrypt" };
    std::string baseDir = "/data/service/el1/public/database/" + appId.appId;
    MkdirPath(baseDir, appId, storeId);

    std::string buildKeyFile = "Prefix_backup_SingleKVStoreEncrypt_testbackup.key";
    std::string path = { baseDir + "/key/" + buildKeyFile + ".bk" };
    auto fl = OHOS::DistributedKv::StoreUtil::CreateFile(path);
    ASSERT_EQ(fl, true);

    std::string buildBackUpFile = baseDir + "/kvdb/backup/" + storeId.storeId + "/" + "testbackup.bak.bk";
    fl = OHOS::DistributedKv::StoreUtil::CreateFile(buildBackUpFile);
    ASSERT_EQ(fl, true);

    std::shared_ptr<SingleKvStore> kvStoreEncrypt;
    kvStoreEncrypt = CreateKVStore(storeId.storeId, appId.appId, baseDir, SINGLE_VERSION, true);
    ASSERT_NE(kvStoreEncrypt, nullptr);

    auto files = StoreUtil::GetFiles(baseDir + "/kvdb/backup/" + storeId.storeId);
    auto keyfiles = StoreUtil::GetFiles(baseDir + "/key");
    bool keyFlag = false;
    for (auto keyfile : keyfiles) {
        auto keyfilename = keyfile.name;
        if (keyfilename == buildKeyFile + ".bk") {
            keyFlag = true;
        }
    }
    ASSERT_EQ(keyFlag, false);

    bool dataFlag = false;
    for (auto datafile : files) {
        auto datafilename = datafile.name;
        if (datafilename == "testbackup.bak.bk") {
            dataFlag = true;
        }
    }
    ASSERT_EQ(dataFlag, false);

    auto status = DeleteBackUpFiles(kvStoreEncrypt, baseDir, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: CleanTmp
* @tc.desc: Clean up temporary data
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, CleanTmp, TestSize.Level0)
{
    AppId appId = { "BackupManagerTestCleanTmp" };
    StoreId storeId = { "SingleKVStoreEncrypt" };
    std::string baseDir = "/data/service/el1/public/database/" + appId.appId;
    MkdirPath(baseDir, appId, storeId);

    std::string buildKeyFile = "Prefix_backup_SingleKVStoreEncrypt_testbackup.key";
    std::string path = { baseDir + "/key/" + buildKeyFile };
    auto fl = OHOS::DistributedKv::StoreUtil::CreateFile(path);
    ASSERT_EQ(fl, true);

    std::string buildBackUpFile = baseDir + "/kvdb/backup/" + storeId.storeId + "/" + "testbackup.bak.bk";
    fl = OHOS::DistributedKv::StoreUtil::CreateFile(buildBackUpFile);
    ASSERT_EQ(fl, true);

    std::shared_ptr<SingleKvStore> kvStoreEncrypt;
    kvStoreEncrypt = CreateKVStore(storeId.storeId, appId.appId, baseDir, SINGLE_VERSION, true);
    ASSERT_NE(kvStoreEncrypt, nullptr);

    auto files = StoreUtil::GetFiles(baseDir + "/kvdb/backup/" + storeId.storeId);
    auto keyfiles = StoreUtil::GetFiles(baseDir + "/key");
    bool keyFlag = false;
    for (auto keyfile : keyfiles) {
        auto keyfilename = keyfile.name;
        if (keyfilename == buildKeyFile + ".bk") {
            keyFlag = true;
        }
    }
    ASSERT_EQ(keyFlag, false);

    bool dataFlag = false;
    for (auto datafile : files) {
        auto datafilename = datafile.name;
        if (datafilename == "testbackup.bak.bk") {
            dataFlag = true;
        }
    }
    ASSERT_EQ(dataFlag, false);

    auto status = DeleteBackUpFiles(kvStoreEncrypt, baseDir, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: BackUpEncrypt
* @tc.desc: Back up an encrypt database and restore
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, BackUpEntry, TestSize.Level0)
{
    AppId appId = { "BackupManagerTest" };
    StoreId storeId = { "SingleKVStoreEncrypt" };
    std::string baseDir = "/data/service/el1/public/database/BackupManagerTest";

    auto kvStoreEncrypt = CreateKVStore(storeId.storeId, "BackupManagerTest", baseDir, SINGLE_VERSION, true);
    if (kvStoreEncrypt == nullptr) {
        kvStoreEncrypt = CreateKVStore(storeId.storeId, "BackupManagerTest", baseDir, SINGLE_VERSION, true);
    }
    ASSERT_NE(kvStoreEncrypt, nullptr);

    auto status = kvStoreEncrypt->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStoreEncrypt->Get({ "Put Test" }, value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(std::string("Put Value"), value.ToString());

    status = kvStoreEncrypt->Backup("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
    status = kvStoreEncrypt->Delete("Put Test");
    ASSERT_EQ(status, SUCCESS);
    status = kvStoreEncrypt->Restore("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
    value = {};
    status = kvStoreEncrypt->Get("Put Test", value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(std::string("Put Value"), value.ToString());

    status = DeleteBackUpFiles(kvStoreEncrypt, baseDir, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}