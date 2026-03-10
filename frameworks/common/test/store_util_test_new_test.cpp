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

class StoreUtilNewTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void StoreUtilNewTest::SetUpTestCase(void) { }

void StoreUtilNewTest::TearDownTestCase(void) { }

void StoreUtilNewTest::SetUp(void) { }

void StoreUtilNewTest::TearDown(void) { }

HWTEST_F(StoreUtilNewTest, GetDBSecurity, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(-1);
    ASSERT_TRUE(dbsecurity.securityLabel == DistributedDB::NOT_SET);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);

    dbsecurity = storeUtil_.GetDBSecurity(7);
    ASSERT_TRUE(dbsecurity.securityLabel == DistributedDB::NOT_SET);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);

    dbsecurity = storeUtil_.GetDBSecurity(5);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S3);
    ASSERT_TRUE(dbsecurity.securityFlag, DistributedDB::SECE);

    dbsecurity = storeUtil_.GetDBSecurity(6);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S4);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::NOT_SET, DistributedDB::ECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, dbSec.securityLabel);

    dbSec = { DistributedDB::S3, DistributedDB::ECE };
    security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S3_EX);
    dbSec = { DistributedDB::S3, DistributedDB::SECE };
    security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S3);

    dbSec = { DistributedDB::S4, DistributedDB::ECE };
    security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S4);
}

HWTEST_F(StoreUtilNewTest, GetDBMode, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_ONLY);

    dbMode = storeUtil_.GetDBMode(SyncMode::PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PULL_ONLY);

    dbMode = storeUtil_.GetDBMode(SyncMode::PUSH_PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_PULL);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_LOCAL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_NATIVE);

    mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_REMOTE);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN);

    mode = storeUtil_.GetObserverMode(SUBSCRIBE_TYPE_ALL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN | DistributedDB::OBSERVER_CHANGES_NATIVE);
}

HWTEST_F(StoreUtilNewTest, CheckPermissions001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_test_new_1";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);

    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test_new_1.txt";
    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CheckPermissions002, TestSize.Level1)
{
    std::string path = "/data/store_utils_test_new_2";
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

    std::string fileName = path + "/test_new_2.txt";
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

HWTEST_F(StoreUtilNewTest, SetDatabaseGid001, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid001New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_002.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    std::string BkfileName = path + "/autoBackupNew.bak";
    storeUtil_.SetServiceGid(BkfileName);
    fp = open(BkfileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(BkfileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(BkfileName);

    remove(fileName.c_str());
    remove(BkfileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(-1);
    ASSERT_TRUE(dbsecurity.securityLabel == DistributedDB::NOT_SET);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(0);
    ASSERT_TRUE(dbsecurity.securityLabel == DistributedDB::NOT_SET);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(1);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S1);
    ASSERT_TRUE(dbsecurity.securityFlag, DistributedDB::SECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(2);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S2);
    ASSERT_TRUE(dbsecurity.securityFlag, DistributedDB::SECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(3);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S3);
    ASSERT_TRUE(dbsecurity.securityFlag, DistributedDB::SECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(4);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S4);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S1, DistributedDB::SECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S1);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S2, DistributedDB::SECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S2);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S3, DistributedDB::SECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S3);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S4, DistributedDB::ECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S4);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S3, DistributedDB::ECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S3_EX);
}

HWTEST_F(StoreUtilNewTest, GetDBMode001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetDBMode002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PULL_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetDBMode003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH_PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_PULL);
}

HWTEST_F(StoreUtilNewTest, GetDBMode004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetDBMode005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PULL_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_LOCAL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_NATIVE);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_REMOTE);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SUBSCRIBE_TYPE_ALL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN | DistributedDB::OBSERVER_CHANGES_NATIVE);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_LOCAL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_NATIVE);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_REMOTE);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN);
}

HWTEST_F(StoreUtilNewTest, CheckPermissions003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_test_new_3";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);

    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test_new_3.txt";
    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CheckPermissions004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_test_new_4";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);

    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test_new_4.txt";
    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CheckPermissions005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_test_new_5";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);

    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test_new_5.txt";
    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CheckPermissions006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_test_new_6";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);

    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test_new_6.txt";
    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CheckPermissions007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_test_new_7";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);

    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    std::string fileName = path + "/test_new_7.txt";
    success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_FALSE(buf.st_mode & S_IRWXO);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CheckPermissions008, TestSize.Level1)
{
    std::string path = "/data/store_utils_test_new_8";
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

    std::string fileName = path + "/test_new_8.txt";
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

HWTEST_F(StoreUtilNewTest, CheckPermissions009, TestSize.Level1)
{
    std::string path = "/data/store_utils_test_new_9";
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

    std::string fileName = path + "/test_new_9.txt";
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

HWTEST_F(StoreUtilNewTest, CheckPermissions010, TestSize.Level1)
{
    std::string path = "/data/store_utils_test_new_10";
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

    std::string fileName = path + "/test_new_10.txt";
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

HWTEST_F(StoreUtilNewTest, SetDatabaseGid002, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid002New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_003.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid003, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid003New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_004.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid004, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid004New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_005.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid005, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid005New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_006.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid006, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid006New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_007.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid007, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid007New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_008.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid008, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid008New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_009.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid009, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid009New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_010.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetDatabaseGid010, TestSize.Level1)
{
    std::string path = "/data/test/SetDbDirGid010New";
    StoreUtil storeUtil_;
    storeUtil_.SetDatabaseGid("");
    storeUtil_.SetDatabaseGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);

    storeUtil_.SetDatabaseGid(path);
    std::string fileName = path + "/test_new_011.db";
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(fileName);
    auto fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRWXU | S_IRWXG | S_IRWXO));
    ASSERT_GE(fp, 0);
    close(fp);
    ret = stat(fileName.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(fileName);

    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_1";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_2";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_3";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_4";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_5";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_1";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_1.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_2";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_2.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_3";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_3.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_4";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_4.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_5";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_5.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_6";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_6.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_7";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_7.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_8";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_8.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest009, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_9";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_9.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest010, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_10";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_10.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, IsFileExistTest001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_file_exist_new_1";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/exist_test_new_1.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    bool exist = storeUtil_.IsFileExist(fileName);
    ASSERT_TRUE(exist);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, IsFileExistTest002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_file_exist_new_2";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/exist_test_new_2.txt";
    bool exist = storeUtil_.IsFileExist(fileName);
    ASSERT_FALSE(exist);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, IsFileExistTest003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_file_exist_new_3";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/exist_test_new_3.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    bool exist = storeUtil_.IsFileExist(fileName);
    ASSERT_TRUE(exist);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, IsFileExistTest004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_file_exist_new_4";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/exist_test_new_4.txt";
    bool exist = storeUtil_.IsFileExist(fileName);
    ASSERT_FALSE(exist);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, IsFileExistTest005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_file_exist_new_5";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/exist_test_new_5.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    bool exist = storeUtil_.IsFileExist(fileName);
    ASSERT_TRUE(exist);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RemoveTest001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_remove_new_1";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/remove_test_new_1.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    success = storeUtil_.Remove(fileName);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RemoveTest002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_remove_new_2";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/remove_test_new_2.txt";
    bool success = storeUtil_.Remove(fileName);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RemoveTest003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_remove_new_3";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/remove_test_new_3.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    success = storeUtil_.Remove(fileName);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RemoveTest004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_remove_new_4";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/remove_test_new_4.txt";
    bool success = storeUtil_.Remove(fileName);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RemoveTest005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_remove_new_5";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/remove_test_new_5.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    success = storeUtil_.Remove(fileName);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RenameTest001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_rename_new_1";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/rename_test_new_1.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    std::string newFileName = path + "/rename_test_new_new_1.txt";
    success = storeUtil_.Rename(fileName, newFileName);
    ASSERT_TRUE(success);
    remove(newFileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RenameTest002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_rename_new_2";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/rename_test_new_2.txt";
    std::string newFileName = path + "/rename_test_new_new_2.txt";
    bool success = storeUtil_.Rename(fileName, newFileName);
    ASSERT_FALSE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RenameTest003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_rename_new_3";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/rename_test_new_3.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    std::string newFileName = path + "/rename_test_new_new_3.txt";
    success = storeUtil_.Rename(fileName, newFileName);
    ASSERT_TRUE(success);
    remove(newFileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RenameTest004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_rename_new_4";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/rename_test_new_4.txt";
    std::string newFileName = path + "/rename_test_new_new_4.txt";
    bool success = storeUtil_.Rename(fileName, newFileName);
    ASSERT_FALSE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, RenameTest005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_rename_new_5";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/rename_test_new_5.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    std::string newFileName = path + "/rename_test_new_new_5.txt";
    success = storeUtil_.Rename(fileName, newFileName);
    ASSERT_TRUE(success);
    remove(newFileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::OK);
    ASSERT_TRUE(status, SUCCESS);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::DB_ERROR);
    ASSERT_TRUE(status, ERROR);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::BUSY);
    ASSERT_TRUE(status, Status::TIME_OUT);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::INVALID_ARGS);
    ASSERT_TRUE(status, INVALID_ARGUMENT);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::NOT_FOUND);
    ASSERT_TRUE(status, STORE_NOT_FOUND);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest001, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(BTREE);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::BTREE);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest002, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(HASH);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::HASH);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest003, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(BTREE);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::BTREE);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest004, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(HASH);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::HASH);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest005, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(BTREE);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::BTREE);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(HASH);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::HASH);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(BTREE);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::BTREE);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(HASH);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::HASH);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest009, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(BTREE);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::BTREE);
}

HWTEST_F(StoreUtilNewTest, GetDBIndexTypeTest010, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto indexType = storeUtil_.GetDBIndexType(HASH);
    ASSERT_TRUE(indexType, DistributedDB::IndexType::HASH);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(0);
    ASSERT_TRUE(dbsecurity.securityLabel == DistributedDB::NOT_SET);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(1);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S1);
    ASSERT_TRUE(dbsecurity.securityFlag, DistributedDB::SECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity009, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(2);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S2);
    ASSERT_TRUE(dbsecurity.securityFlag, DistributedDB::SECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity010, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(3);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S3);
    ASSERT_TRUE(dbsecurity.securityFlag, DistributedDB::SECE);
}

HWTEST_F(StoreUtilNewTest, GetDBSecurity011, TestSize.Level1)
{
    StoreUtil storeUtil_;
    auto dbsecurity = storeUtil_.GetDBSecurity(4);
    ASSERT_TRUE(dbsecurity.securityLabel, DistributedDB::S4);
    ASSERT_TRUE(dbsecurity.securityFlag == DistributedDB::ECE);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S1, DistributedDB::SECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S1);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S2, DistributedDB::SECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S2);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S3, DistributedDB::SECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S3);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel009, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S4, DistributedDB::ECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S4);
}

HWTEST_F(StoreUtilNewTest, GetSecLevel010, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBSecurity dbSec = { DistributedDB::S3, DistributedDB::ECE };
    int32_t security = storeUtil_.GetSecLevel(dbSec);
    ASSERT_TRUE(security, S3_EX);
}

HWTEST_F(StoreUtilNewTest, GetDBMode006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetDBMode007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PULL_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetDBMode008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH_PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_PULL);
}

HWTEST_F(StoreUtilNewTest, GetDBMode009, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PUSH);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PUSH_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetDBMode010, TestSize.Level1)
{
    StoreUtil storeUtil_;
    StoreUtil::DBMode dbMode = storeUtil_.GetDBMode(SyncMode::PULL);
    ASSERT_TRUE(dbMode, StoreUtil::DBMode::SYNC_MODE_PULL_ONLY);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_LOCAL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_NATIVE);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_REMOTE);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SUBSCRIBE_TYPE_ALL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN | DistributedDB::OBSERVER_CHANGES_NATIVE);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode009, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_LOCAL);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_NATIVE);
}

HWTEST_F(StoreUtilNewTest, GetObserverMode010, TestSize.Level1)
{
    StoreUtil storeUtil_;
    uint32_t mode = storeUtil_.GetObserverMode(SubscribeType::SUBSCRIBE_TYPE_REMOTE);
    ASSERT_TRUE(mode, DistributedDB::OBSERVER_CHANGES_FOREIGN);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::OK);
    ASSERT_TRUE(status, SUCCESS);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::DB_ERROR);
    ASSERT_TRUE(status, ERROR);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::BUSY);
    ASSERT_TRUE(status, Status::TIME_OUT);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest009, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::INVALID_ARGS);
    ASSERT_TRUE(status, INVALID_ARGUMENT);
}

HWTEST_F(StoreUtilNewTest, ConvertStatusTest010, TestSize.Level1)
{
    StoreUtil storeUtil_;
    Status status = storeUtil_.ConvertStatus(DistributedDB::DBStatus::NOT_FOUND);
    ASSERT_TRUE(status, STORE_NOT_FOUND);
}

HWTEST_F(StoreUtilNewTest, SetServiceGid001, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid001New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid002, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid002New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid003, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid003New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid004, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid004New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid005, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid005New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid006, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid006New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid007, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid007New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid008, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid008New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid009, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid009New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, SetServiceGid010, TestSize.Level1)
{
    std::string path = "/data/test/SetServiceGid010New";
    StoreUtil storeUtil_;
    storeUtil_.SetServiceGid("");
    storeUtil_.SetServiceGid(path);
    auto ret = mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    struct stat buf;
    ret = stat(path.c_str(), &buf);
    ASSERT_GE(ret, 0);
    ASSERT_TRUE(buf.st_mode & S_IRWXO);
    storeUtil_.SetServiceGid(path);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_6";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_7";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, InitPathTest008, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_init_path_new_8";
    bool success = storeUtil_.InitPath(path);
    ASSERT_TRUE(success);
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest011, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_11";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_11.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest012, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_12";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_12.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, CreateFileTest013, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_create_file_new_13";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/create_test_new_13.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, IsFileExistTest006, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_file_exist_new_6";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/exist_test_new_6.txt";
    bool success = storeUtil_.CreateFile(fileName);
    ASSERT_TRUE(success);
    bool exist = storeUtil_.IsFileExist(fileName);
    ASSERT_TRUE(exist);
    remove(fileName.c_str());
    rmdir(path.c_str());
}

HWTEST_F(StoreUtilNewTest, IsFileExistTest007, TestSize.Level1)
{
    StoreUtil storeUtil_;
    std::string path = "/data/store_utils_file_exist_new_7";
    mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    std::string fileName = path + "/exist_test_new_7.txt";
    bool exist = storeUtil_.IsFileExist(fileName);
    ASSERT_FALSE(exist);
    rmdir(path.c_str());
}
} // namespace OHOS::Test
