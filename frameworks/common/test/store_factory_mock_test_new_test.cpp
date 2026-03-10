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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>

#include "include/security_manager_mock.h"
#include "include/store_util_mock.h"
#include "store_factory.h"

namespace OHOS::DistributedKv {
using namespace std;
using namespace testing;

static StoreId storeId = { "single_test_new" };
static AppId appId = { "rekey_new" };
static Options options = {
    .encrypt = false,
    .securityLevel = S1,
    .area = EL1,
    .kvStoreType = SINGLE_VERSION,
    .baseDir = "/data/service/el1/public/database/rekey_new",
};

class StoreFactoryMockNewTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

public:
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using DBPassword = SecurityManager::DBPassword;
    static inline shared_ptr<StoreUtilMock> storeUtilMock = nullptr;
    static inline shared_ptr<SecurityManagerMock> securityManagerMock = nullptr;
};

void StoreFactoryMockNewTest::SetUp()
{
    std::string baseDir = "/data/service/el1/public/database/rekey_new";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void StoreFactoryMockNewTest::TearDown()
{
    (void)remove("/data/service/el1/public/database/rekey_new");
}

void StoreFactoryMockNewTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase enter";
    storeUtilMock = make_shared<StoreUtilMock>();
    StoreUtilMock::storeUtil = storeUtilMock;
    securityManagerMock = make_shared<SecurityManagerMock>();
    BSecurityManager::securityManager = securityManagerMock;
}

void StoreFactoryMockNewTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase enter";
    StoreUtilMock::storeUtil = nullptr;
    storeUtilMock = nullptr;
    BSecurityManager::securityManager = nullptr;
    securityManagerMock = nullptr;
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_001";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).WillOnce(Return(SecurityManager::DBPassword()));
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(ERROR)).WillOnce(Return(ERROR));
        status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(ERROR));
        status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred by GetSrcPath.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_001";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_002, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_002";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).WillOnce(Return(SecurityManager::DBPassword()));
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_002";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_003, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_003";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_003";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_004, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_004";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(DB_ERROR)).WillOnce(Return(DB_ERROR));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_004";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_005, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_005";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(ERROR)).WillOnce(Return(ERROR));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_005";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_006, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_006";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).WillOnce(Return(SecurityManager::DBPassword()));
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(false));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_006";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_007, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_007";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_007";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_008, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_008";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_008";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_009, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_009";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(DB_ERROR));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(0);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_009";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_010, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_010";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(Status::TIME_OUT)).WillOnce(Return(Status::TIME_OUT));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_010";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_011, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_011";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(INVALID_ARGUMENT)).WillOnce(Return(INVALID_ARGUMENT));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_011";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_012, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_012";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(Status::INVALID_ARGUMENT)).WillOnce(Return(Status::INVALID_ARGUMENT));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_012";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_013, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_013";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(Status::INVALID_ARGUMENT)).WillOnce(Return(Status::INVALID_ARGUMENT));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_013";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_014, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_014";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SECURITY_LEVEL_ERROR)).WillOnce(Return(SECURITY_LEVEL_ERROR));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_014";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_015, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_015";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(Status::TIME_OUT)).WillOnce(Return(Status::TIME_OUT));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_015";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_016, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_016";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(NOT_SUPPORT)).WillOnce(Return(NOT_SUPPORT));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_016";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_017, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_017";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(OVER_MAX_LIMITS)).WillOnce(Return(OVER_MAX_LIMITS));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_017";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_018, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_018";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(STORE_NOT_FOUND)).WillOnce(Return(STORE_NOT_FOUND));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_018";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_019, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_019";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(Status::KEY_NOT_FOUND)).WillOnce(Return(Status::KEY_NOT_FOUND));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_019";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_020, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_020";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(Status::NOT_FOUND)).WillOnce(Return(Status::NOT_FOUND));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_020";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_021, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_021";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(STORE_ALREADY_SUBSCRIBE)).WillOnce(Return(STORE_ALREADY_SUBSCRIBE));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_021";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_022, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_022";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(STORE_NOT_SUBSCRIBE)).WillOnce(Return(STORE_NOT_SUBSCRIBE));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_022";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_023, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_023";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(Status::DEVICE_NOT_FOUND)).WillOnce(Return(Status::DEVICE_NOT_FOUND));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_023";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_024, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_024";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(ALREADY_CLOSED)).WillOnce(Return(ALREADY_CLOSED));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_024";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_025, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_025";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(DB_ERROR)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_025";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_026, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_026";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(DB_ERROR));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_026";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_027, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_027";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(DB_ERROR));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(0);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_027";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_028, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_028";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(ERROR));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(0);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_028";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_029, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_029";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(false));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status != SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_029";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_030, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_030";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_030";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_031, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_031";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_031";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_032, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_032";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_032";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_033, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_033";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_033";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_034, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_034";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_034";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_035, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_035";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_035";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_036, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_036";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_036";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_037, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_037";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_037";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_038, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_038";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_038";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_039, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_039";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_039";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_040, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_040";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_040";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_041, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_041";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_041";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_042, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_042";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_042";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_043, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_043";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_043";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_044, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_044";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_044";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_045, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_045";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_045";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_046, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_046";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_046";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_047, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_047";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_047";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_048, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_048";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_048";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_049, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_049";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_049";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_050, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_050";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_050";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_051, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_051";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_051";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_052, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_052";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_052";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_053, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_053";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_053";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_054, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_054";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_054";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_055, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_055";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_055";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_056, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_056";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_056";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_057, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_057";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_057";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_058, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_058";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_058";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_059, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_059";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_059";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_060, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_060";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_060";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_061, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_061";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_061";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_062, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_062";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_062";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_063, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_063";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_063";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_064, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_064";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_064";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_065, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_065";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_065";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_066, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_066";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_066";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_067, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_067";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_067";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_068, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_068";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_068";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_069, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_069";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(true)).WillOnce(Return(true));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(2);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*storeUtilMock, Remove(_)).WillOnce(Return(true));
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_069";
}

HWTEST_F(StoreFactoryMockNewTest, RekeyRecover_070, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-begin RekeyRecover_070";
    try {
        std::string path = options.GetDatabaseDir();
        std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
        DBPassword dbPassword;

        EXPECT_CALL(*storeUtilMock, IsFileExist(_)).WillOnce(Return(false)).WillOnce(Return(false));
        EXPECT_CALL(*securityManagerMock, GetDBPassword(_, _, _)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBSecurity(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, GetDBIndexType(_)).Times(1);
        EXPECT_CALL(*storeUtilMock, ConvertStatus(_)).WillOnce(Return(SUCCESS));
        EXPECT_CALL(*securityManagerMock, SaveDBPassword(_, _, _)).Times(1);
        auto status = StoreFactory::GetInstance().RekeyRecover(storeId, path, dbPassword, dbManager, options);
        EXPECT_TRUE(status == SUCCESS);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-an exception occurred.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockNewTest-end RekeyRecover_070";
}
} // namespace OHOS::DistributedKv
