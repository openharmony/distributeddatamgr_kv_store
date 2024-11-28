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

static StoreId storeId = { "single_test" };
static AppId appId = { "rekey" };
static Options options = {
    .encrypt = false,
    .securityLevel = S1,
    .area = EL1,
    .kvStoreType = SINGLE_VERSION,
    .baseDir = "/data/service/el1/public/database/rekey",
};

class StoreFactoryMockTest : public testing::Test {
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

void StoreFactoryMockTest::SetUp()
{
    std::string baseDir = "/data/service/el1/public/database/rekey";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void StoreFactoryMockTest::TearDown()
{
    (void)remove("/data/service/el1/public/database/rekey");
}

void StoreFactoryMockTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase enter";
    storeUtilMock = make_shared<StoreUtilMock>();
    StoreUtilMock::storeUtil = storeUtilMock;
    securityManagerMock = make_shared<SecurityManagerMock>();
    BSecurityManager::securityManager = securityManagerMock;
}

void StoreFactoryMockTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase enter";
    StoreUtilMock::storeUtil = nullptr;
    storeUtilMock = nullptr;
    BSecurityManager::securityManager = nullptr;
    securityManagerMock = nullptr;
}

/**
 * @tc.name: RekeyRecover_001
 * @tc.desc: Rekey recover test.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: cao zhijun
 */
HWTEST_F(StoreFactoryMockTest, RekeyRecover_001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(INFO) << "StoreFactoryMockTest-begin RekeyRecover_001";
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
        GTEST_LOG_(INFO) << "StoreFactoryMockTest-an exception occurred by GetSrcPath.";
    }
    GTEST_LOG_(INFO) << "StoreFactoryMockTest-end RekeyRecover_001";
}
} // namespace OHOS::DistributedKv