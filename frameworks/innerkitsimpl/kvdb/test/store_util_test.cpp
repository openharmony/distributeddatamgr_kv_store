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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <vector>

#include "store_manager.h"
#include "types.h"
namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class StoreUtilTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void StoreUtilTest::SetUpTestCase(void) { }

void StoreUtilTest::TearDownTestCase(void) { }

void StoreUtilTest::SetUp(void) { }

void StoreUtilTest::TearDown(void) { }
/**
 * @tc.name: GetDBSecurity
 * @tc.desc: get db security
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(StoreUtilTest, GetDBSecurity, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(-1);
    ASSERT_EQ(dbsecurity.securityLabel, DistributedDB::NOT_SET);
    ASSERT_EQ(dbsecurity.securityFlag, DistributedDB::ECE);

    dbsecurity = storeUtil_.GetDBSecurity(7);
    ASSERT_EQ(dbsecurity.securityLabel, DistributedDB::NOT_SET);
    ASSERT_EQ(dbsecurity.securityFlag, DistributedDB::ECE);

    dbsecurity = storeUtil_.GetDBSecurity(5);
    ASSERT_EQ(dbsecurity.securityLabel, DistributedDB::S3);
    ASSERT_EQ(dbsecurity.securityFlag, DistributedDB::SECE);

    dbsecurity = storeUtil_.GetDBSecurity(6);
    ASSERT_EQ(dbsecurity.securityLabel, DistributedDB::S4);
    ASSERT_EQ(dbsecurity.securityFlag, DistributedDB::ECE);
}
/**
 * @tc.name: GetSecLevel
 * @tc.desc: get secLevel
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(StoreUtilTest, GetSecLevel, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::NOT_SET, DistributedDB::ECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_EQ(security, dbSec.securityLabel);

    dbSec = { DistributedDB::S3, DistributedDB::ECE };
    security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_EQ(security, S3_EX);
    dbSec = { DistributedDB::S3, DistributedDB::SECE };
    security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_EQ(security, S3);

    dbSec = { DistributedDB::S4, DistributedDB::ECE };
    security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_EQ(security, S4);
}
/**
 * @tc.name: GetDBMode
 * @tc.desc: get db mode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(StoreUtilTest, GetDBMode, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH);
    ASSERT_EQ(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_ONLY);

    dbMode = storeUtil_.GetDBMode(SyncMode::PULL);
    ASSERT_EQ(dbMode, StoreUtil::DBMode::SYNC_MODE_PULL_ONLY);

    dbMode = storeUtil_.GetDBMode(SyncMode::PUSH_PULL);
    ASSERT_EQ(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_PULL);
}
/**
 * @tc.name: GetObserverMode
 * @tc.desc: get observer mode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Wang Kai
 */
HWTEST_F(StoreUtilTest, GetObserverMode, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_LOCAL);
    ASSERT_EQ(mode, DistributedDB::OBSERVER_CHANGES_NATIVE);

    mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_REMOTE);
    ASSERT_EQ(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN);

    mode = storeUtil_.GetObserverMode(SUBSCRIBE_TYPE_ALL);
    ASSERT_EQ(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN | DistributedDB::OBSERVER_CHANGES_NATIVE);
}
/**
 * @tc.name: CheckPermissions001
 * @tc.desc: Check if the permissions for the first file creation are normal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Shao Yuanzhao
 */
HWTEST_F(StoreUtilTest, CheckPermissions001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_test1";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);

    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test1.txt";
    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}
/**
 * @tc.name: CheckPermissions002
 * @tc.desc: Check if updating existing file permissions is correct
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Shao Yuanzhao
 */
HWTEST_F(StoreUtilTest, CheckPermissions002, TestSize.Level1)
{
    std::string path = "/data/store_utils_test2";
    int ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    StoreUtil storeUtil_;
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test2.txt";
    int fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}
} // namespace OHOS::Test