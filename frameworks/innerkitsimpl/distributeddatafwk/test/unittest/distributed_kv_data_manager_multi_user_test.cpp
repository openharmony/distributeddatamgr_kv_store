/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "distributed_kv_data_manager.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
static constexpr const char *APP_ID = "kv_multi_user_test";
static constexpr const char *FIRST_STORE = "firstStoreId";
static constexpr const char *SECOND_STORE = "secondStoreId";
static constexpr int32_t USER_ID = 100;
static constexpr const char *DATA_DIR = "/data/service/el1/100/database";
class DistributedKvDataManagerMultiUserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager manager;
    static Options normalOptions;
    static Options encryptOptions;
    static AppId appId;
    static StoreId firstStoreId;
    static StoreId secondStoreId;
};

DistributedKvDataManager DistributedKvDataManagerMultiUserTest::manager;
Options DistributedKvDataManagerMultiUserTest::normalOptions;
Options DistributedKvDataManagerMultiUserTest::encryptOptions;
AppId DistributedKvDataManagerMultiUserTest::appId;
StoreId DistributedKvDataManagerMultiUserTest::firstStoreId;
StoreId DistributedKvDataManagerMultiUserTest::secondStoreId;

void DistributedKvDataManagerMultiUserTest::SetUpTestCase(void)
{
    appId.appId = APP_ID;
    firstStoreId.storeId = FIRST_STORE;
    secondStoreId.storeId = SECOND_STORE;

    std::string dataDir = DATA_DIR + appId.appId;
    mkdir(dataDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    normalOptions.createIfMissing = true;
    normalOptions.securityLevel = S1;
    normalOptions.kvStoreType = SINGLE_VERSION;
    normalOptions.area = EL1;
    normalOptions.subUser = USER_ID;
    normalOptions.baseDir = dataDir;
    normalOptions.encrypt = false;

    encryptOptions.createIfMissing = true;
    encryptOptions.securityLevel = S1;
    encryptOptions.kvStoreType = SINGLE_VERSION;
    encryptOptions.area = EL1;
    encryptOptions.subUser = USER_ID;
    encryptOptions.baseDir = dataDir;
    encryptOptions.encrypt = true;
}

void DistributedKvDataManagerMultiUserTest::TearDownTestCase(void)
{
    remove(normalOptions.baseDir.c_str());
}

void DistributedKvDataManagerMultiUserTest::SetUp()
{
}

void DistributedKvDataManagerMultiUserTest::TearDown()
{
    manager.CloseAllKvStore(appId, USER_ID);
    manager.DeleteAllKvStore(appId, normalOptions.baseDir, USER_ID);
}

/**
* @tc.name: GetKvStoreTest001
* @tc.desc: get normal kv store with sub user
* @tc.type: FUNC
*/
HWTEST_F(DistributedKvDataManagerMultiUserTest, GetKvStoreTest001, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(normalOptions, appId, firstStoreId, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);
}

/**
* @tc.name: GetKvStoreTest002
* @tc.desc: get encrypt kv store with sub user
* @tc.type: FUNC
*/
HWTEST_F(DistributedKvDataManagerMultiUserTest, GetKvStoreTest002, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(encryptOptions, appId, firstStoreId, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);
}

/**
* @tc.name: GetAllKvStoreIdTest
* @tc.desc: get all store id with sub user
* @tc.type: FUNC
*/
HWTEST_F(DistributedKvDataManagerMultiUserTest, GetAllKvStoreId001, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> firstKvStore;
    auto status = manager.GetSingleKvStore(normalOptions, appId, firstStoreId, firstKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(firstKvStore, nullptr);

    std::shared_ptr<SingleKvStore> secondKvStore;
    status = manager.GetSingleKvStore(normalOptions, appId, secondStoreId, secondKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(secondKvStore, nullptr);

    std::vector<StoreId> storeIds;
    status = manager.GetAllKvStoreId(appId, storeIds, normalOptions.subUser);
    ASSERT_EQ(status, Status::SUCCESS);

    ASSERT_EQ(storeIds.size(), static_cast<size_t>(2));
    ASSERT_NE(storeIds[0].storeId, storeIds[1].storeId);
    ASSERT_TRUE(storeIds[0].storeId == firstStoreId.storeId || storeIds[0].storeId == secondStoreId.storeId);
    ASSERT_TRUE(storeIds[1].storeId == secondStoreId.storeId || storeIds[0].storeId == secondStoreId.storeId);
}

/**
* @tc.name: CloseKvStoreTest001
* @tc.desc: close kv store with sub user
* @tc.type: FUNC
*/
HWTEST_F(DistributedKvDataManagerMultiUserTest, CloseKvStoreTest001, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(normalOptions, appId, firstStoreId, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    status = manager.CloseKvStore(appId, firstStoreId, normalOptions.subUser);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: CloseKvStoreTest002
* @tc.desc: close kv store with sub user
* @tc.type: FUNC
*/
HWTEST_F(DistributedKvDataManagerMultiUserTest, CloseKvStoreTest002, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(normalOptions, appId, firstStoreId, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    status = manager.CloseKvStore(appId, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: CloseKvStoreTest003
* @tc.desc: delete kv store with sub user
* @tc.type: FUNC
*/
HWTEST_F(DistributedKvDataManagerMultiUserTest, CloseKvStoreTest003, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(normalOptions, appId, firstStoreId, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    status = manager.CloseKvStore(appId, firstStoreId, normalOptions.subUser);
    ASSERT_EQ(status, Status::SUCCESS);
    kvStore = nullptr;

    status = manager.DeleteKvStore(appId, firstStoreId, normalOptions.baseDir, normalOptions.subUser);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: DeleteKvStoreTest002
* @tc.desc: delete all kv store with sub user
* @tc.type: FUNC
*/
HWTEST_F(DistributedKvDataManagerMultiUserTest, DeleteKvStoreTest002, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> firstKvStore;
    auto status = manager.GetSingleKvStore(normalOptions, appId, firstStoreId, firstKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(firstKvStore, nullptr);

    std::shared_ptr<SingleKvStore> secondKvStore;
    status = manager.GetSingleKvStore(normalOptions, appId, secondStoreId, secondKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(secondKvStore, nullptr);

    status = manager.CloseKvStore(appId, firstStoreId, normalOptions.subUser);
    ASSERT_EQ(status, Status::SUCCESS);
    firstKvStore = nullptr;

    status = manager.CloseKvStore(appId, secondStoreId, normalOptions.subUser);
    ASSERT_EQ(status, Status::SUCCESS);
    secondKvStore = nullptr;

    status = manager.DeleteAllKvStore(appId, normalOptions.baseDir, normalOptions.subUser);
    ASSERT_EQ(status, Status::SUCCESS);
}
} // namespace OHOS::Test