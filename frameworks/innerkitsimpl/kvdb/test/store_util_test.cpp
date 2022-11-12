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

#include "store_util.h"

#include <gtest/gtest.h>
#include <vector>

#include "store_manager.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class StoreUtilTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void StoreUtilTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/StoreUtilTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void StoreUtilTest::TearDownTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/StoreUtilTest";
    StoreManager::GetInstance().Delete({ "StoreUtilTest" }, { "SingleKVStore" }, baseDir);

    (void)remove("/data/service/el1/public/database/StoreUtilTest/key");
    (void)remove("/data/service/el1/public/database/StoreUtilTest/kvdb");
    (void)remove("/data/service/el1/public/database/StoreUtilTest");
}

void StoreUtilTest::SetUp(void) {}

void StoreUtilTest::TearDown(void)
{
    auto baseDir = "/data/service/el1/public/database/StoreUtilTest";
    StoreManager::GetInstance().Delete({ "StoreUtilTest" }, { "SingleKVStore" }, baseDir);
}
/**
* @tc.name: CreateKVStoreNO_LABEL
* @tc.desc: creat kvstore with securityLevel of NO_LABEL
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(StoreUtilTest, CreateKVStoreNO_LABEL, TestSize.Level1)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = NO_LABEL;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/StoreUtilTest";
    SyncPolicy policy;
    policy.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "StoreUtilTest" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    auto kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status1 = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status1, SUCCESS);
    ASSERT_EQ(securityLevel, NO_LABEL);
}
/**
* @tc.name: CreateKVStoreS2
* @tc.desc: creat kvstore with securityLevel of S2
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(StoreUtilTest, CreateKVStoreS2, TestSize.Level1)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S2;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/StoreUtilTest";
    SyncPolicy policy;
    policy.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "StoreUtilTest" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    auto kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status1 = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status1, SUCCESS);
    ASSERT_EQ(securityLevel, S2);
}
/**
* @tc.name: CreateKVStoreS3
* @tc.desc: creat kvstore with securityLevel of S3
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(StoreUtilTest, CreateKVStoreS3, TestSize.Level1)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S3;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/StoreUtilTest";
    SyncPolicy policy;
    policy.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "StoreUtilTest" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    auto kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status1 = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status1, SUCCESS);
    ASSERT_EQ(securityLevel, S3);
}
/**
* @tc.name: CreateKVStoreS4
* @tc.desc: creat kvstore with securityLevel of S4
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(StoreUtilTest, CreateKVStoreS4, TestSize.Level1)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S4;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/StoreUtilTest";
    SyncPolicy policy;
    policy.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "StoreUtilTest" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    auto kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status1 = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status1, SUCCESS);
    ASSERT_EQ(securityLevel, S4);
}

/**
* @tc.name: CreateKVStoreS0
* @tc.desc: creat kvstore with securityLevel of S0
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(StoreUtilTest, CreateKVStoreS0, TestSize.Level1)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S0;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/StoreUtilTest";
    SyncPolicy policy;
    policy.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "StoreUtilTest" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    auto kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status1 = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status1, SUCCESS);
    ASSERT_EQ(securityLevel, S0);
}

/**
* @tc.name: CreateKVStoreEncrypt
* @tc.desc: creat encrypted kvstore and backup
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(StoreUtilTest, CreateKVStoreEncrypt, TestSize.Level1)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.encrypt = true;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/StoreUtilTest";
    SyncPolicy policy;
    policy.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "StoreUtilTest" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    auto kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status1 = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status1, SUCCESS);
    auto baseDir = "/data/service/el1/public/database/StoreUtilTest";
    auto status2 = kvStore_->Backup("testbackup", baseDir);
    ASSERT_EQ(status2, SUCCESS);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    kvStore_->DeleteBackup({ "testbackup", "autoBackup" }, baseDir, results);
}

/**
* @tc.name: HaveResidueKey
* @tc.desc: creat encrypted kvstore and have residue key
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangkai
*/
HWTEST_F(StoreUtilTest, HaveResidueKey, TestSize.Level1)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.encrypt = true;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/StoreUtilTest";
    SyncPolicy policy;
    policy.type = PolicyType::TERM_OF_SYNC_VALIDITY;
    policy.value.emplace<1>(100);
    options.policies.emplace_back(policy);

    AppId appId = { "StoreUtilTest" };
    StoreId storeId = { "SingleKVStore" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    auto kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore_, nullptr);
    SecurityLevel securityLevel = NO_LABEL;
    auto status1 = kvStore_->GetSecurityLevel(securityLevel);
    ASSERT_EQ(status1, SUCCESS);
    auto baseDir = "/data/service/el1/public/database/StoreUtilTest";
    std::string path = { "/data/service/el1/public/database/StoreUtilTest/key/Prefix_backup_SingleKVStore.bk" };
    OHOS::DistributedKv::StoreUtil::CreateFile(path);
    StoreManager::GetInstance().Delete({ "StoreUtilTest" }, { "SingleKVStore" }, baseDir);
    std::map<std::string, OHOS::DistributedKv::Status> results;
    kvStore_->DeleteBackup({ "autoBackup" }, baseDir, results);
}