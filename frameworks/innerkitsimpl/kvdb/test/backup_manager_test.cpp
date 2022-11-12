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

    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, KvStoreType type);
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
    kvStore_ = CreateKVStore("SingleKVStore", SINGLE_VERSION);
    if (kvStore_ == nullptr) {
        kvStore_ = CreateKVStore("SingleKVStore", SINGLE_VERSION);
    }
    ASSERT_NE(kvStore_, nullptr);
}

void BackupManagerTest::TearDown(void) {}

std::shared_ptr<SingleKvStore> BackupManagerTest::CreateKVStore(std::string storeIdTest, KvStoreType type)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/BackupManagerTest";
    SyncPolicy policy;
    policy.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "BackupManagerTest" };
    StoreId storeId = { storeIdTest };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
}
HWTEST_F(BackupManagerTest, BackUp001, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    auto status = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    auto status1 = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
    ASSERT_EQ(status1, SUCCESS);
    auto status2 = StoreManager::GetInstance().Delete({ "BackupManagerTest" }, { "SingleKVStore" }, baseDir);
    ASSERT_EQ(status2, SUCCESS);
}
/**
 * @tc.name: BackUp002
 * @tc.desc: the kvstore backup is INVALID_ARGUMENT
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(BackupManagerTest, BackUp002, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto baseDir = "";
    auto baseDir1 = "/data/service/el1/public/database/BackupManagerTest";
    auto status = kvStore_->Backup("testbackup", baseDir);
    auto status1 = kvStore_->Backup("", baseDir);
    auto status2 = kvStore_->Backup("autoBackup", baseDir1);
    ASSERT_EQ(status, INVALID_ARGUMENT);
    ASSERT_EQ(status1, INVALID_ARGUMENT);
    ASSERT_EQ(status2, INVALID_ARGUMENT);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    auto status3 = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
    ASSERT_EQ(status3, INVALID_ARGUMENT);
    auto status4 = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir1, results);
    ASSERT_EQ(status4, SUCCESS);
    auto status5 = StoreManager::GetInstance().Delete({ "BackupManagerTest" }, { "SingleKVStore" }, baseDir1);
    ASSERT_EQ(status5, SUCCESS);
}
///**
// * @tc.name: BackUp003
// * @tc.desc: the kvstore backup
// * @tc.type: FUNC
// * @tc.require:
// * @tc.author: Wang Kai
// */
//HWTEST_F(BackupManagerTest, BackUp003, TestSize.Level0)
//{
//    ASSERT_NE(kvStore_, nullptr);
//    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
//    auto status1 = kvStore_->Backup("testbackup1", baseDir);
//    auto status2 = kvStore_->Backup("testbackup2", baseDir);
//    auto status3 = kvStore_->Backup("testbackup3", baseDir);
//    auto status4 = kvStore_->Backup("testbackup4", baseDir);
//    auto status5 = kvStore_->Backup("testbackup5", baseDir);
//    ASSERT_EQ(status1, SUCCESS);
//    ASSERT_EQ(status2, SUCCESS);
//    ASSERT_EQ(status3, SUCCESS);
//    ASSERT_EQ(status4, SUCCESS);
//    ASSERT_EQ(status5, ERROR);
//    std::map<std::string, OHOS::DistributedKv::Status> results;
//    auto status6 = kvStore_->DeleteBackup({ "testbackup1", "testbackup2", "testbackup3", "testbackup4", "autoBackup" },
//        baseDir, results);
//    ASSERT_EQ(status6, SUCCESS);
//    auto status7 = StoreManager::GetInstance().Delete({ "BackupManagerTest" }, { "SingleKVStore" }, baseDir);
//    ASSERT_EQ(status7, SUCCESS);
//}
/**
 * @tc.name: BackUp003
 * @tc.desc: the kvstore backup file name is the same
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(BackupManagerTest, BackUp003, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    auto status = kvStore_->Backup("testbackup", baseDir);
    auto status1 = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(status1, SUCCESS);
    std::string path = { "/data/service/el1/public/database/BackupManagerTest/kvdb/backup/SingleKVStore/"
                         "testbackup.bk" };
    OHOS::DistributedKv::StoreUtil::CreateFile(path);
    auto status2 = StoreManager::GetInstance().Delete({ "BackupManagerTest" }, { "SingleKVStore" }, baseDir);
    ASSERT_EQ(status2, SUCCESS);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    auto status3 = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
    ASSERT_EQ(status3, SUCCESS);
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
    auto baseDir = "/data/service/el1/public/database/BackupManagerTest";
    auto baseDir1 = "";
    auto status = kvStore_->Backup("testbackup", baseDir);
    auto status1 = kvStore_->Restore("testbackup", baseDir);
    auto status2 = kvStore_->Restore("testbackup", baseDir1);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(status1, SUCCESS);
    ASSERT_EQ(status2, INVALID_ARGUMENT);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    auto status3 = kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
    ASSERT_EQ(status3, SUCCESS);
    auto status4 = StoreManager::GetInstance().Delete({ "BackupManagerTest" }, { "SingleKVStore" }, baseDir);
    ASSERT_EQ(status4, SUCCESS);
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