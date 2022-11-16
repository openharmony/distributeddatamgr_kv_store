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

#include "block_data.h"
#include "dev_manager.h"
#include "distributed_kv_data_manager.h"
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

    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, KvStoreType type, bool encrypt);
    std::shared_ptr<SingleKvStore> kvStore_;
};

void BackupManagerTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/BackupManagerTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
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
    kvStore_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false);
    if (kvStore_ == nullptr) {
        kvStore_ = CreateKVStore("SingleKVStore", SINGLE_VERSION, false);
    }
    ASSERT_NE(kvStore_, nullptr);
}

void BackupManagerTest::TearDown(void)
{
    AppId appId = { "BackupManagerTest" };
    StoreId storeId = { "SingleKVStore" };
    kvStore_ = nullptr;
    auto status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    status = StoreManager::GetInstance().Delete({ "BackupManagerTest" }, { "SingleKVStore" }, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

std::shared_ptr<SingleKvStore> BackupManagerTest::CreateKVStore(std::string storeIdTest, KvStoreType type, bool encrypt)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.encrypt = encrypt;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/BackupManagerTest";
    SyncPolicy policy;
    policy.type = PolicyType::TERM_OF_SYNC_VALIDITY;
    int value = 100;
    policy.value.emplace<1>(value);
    options.policies.emplace_back(policy);

    AppId appId = { "BackupManagerTest" };
    StoreId storeId = { storeIdTest };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
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
    std::map<std::string, OHOS::DistributedKv::Status> results;
    status = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
    ASSERT_EQ(status, SUCCESS);
}
/**
 * @tc.name: BackUp
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
    std::map<std::string, OHOS::DistributedKv::Status> results;
    status = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
    ASSERT_EQ(status, INVALID_ARGUMENT);
    status = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir1, results);
    ASSERT_EQ(status, SUCCESS);
}
/**
 * @tc.name: BackUp003
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
    std::string path = { "/data/service/el1/public/database/BackupManagerTest/kvdb/backup/SingleKVStore/"
                         "testbackup.bak.bk" };
    OHOS::DistributedKv::StoreUtil::CreateFile(path);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    status = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
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
    ASSERT_EQ(std::string ("Put Value"),value.ToString());
    status = kvStore_->Restore("testbackup", baseDir1);
    ASSERT_EQ(status, INVALID_ARGUMENT);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    status = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
    ASSERT_EQ(status, SUCCESS);
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
}
/**
* @tc.name: HaveResidueKey
* @tc.desc: creat encrypted kvstore and have residue key
* @tc.type: FUNC
* @tc.require:
* @tc.author: Wang Kai
*/
HWTEST_F(BackupManagerTest, HaveResidueKey, TestSize.Level0)
{
    AppId appId = { "BackupManagerTest" };
    StoreId storeId = { "SingleKVStoreEncrypt" };
    std::shared_ptr<SingleKvStore> kvStoreEncrypt_;
    kvStoreEncrypt_ = CreateKVStore(storeId, SINGLE_VERSION, true);
    ASSERT_NE(kvStoreEncrypt_, nullptr);

    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    std::string path = { "/data/service/el1/public/database/StoreUtilTest/key/Prefix_backup_SingleKVStore.key.bk" };
    OHOS::DistributedKv::StoreUtil::CreateFile(path);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    auto status = kvStoreEncrypt_->DeleteBackup({ "autoBackup" }, baseDir, results);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
}