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
#define LOG_TAG "DistributedKvDataManagerInterceptionTest"
#include "distributed_kv_data_manager.h"
#include <gtest/gtest.h>
#include <vector>

#include "block_data.h"
#include "dev_manager.h"
#include "kvstore_death_recipient.h"
#include "log_print.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
static constexpr size_t MIN = 50;
static constexpr size_t MAX = 120;
class DistributedKvDataManagerInterceptionTest : public testing::Test {
public:
    static std::shared_ptr<ExecutorPool> executors_;
    static DistributedKvDataManager manager_;
    static Options create_;
    static Options noCreate_;
    static UserId userId_;
    static AppId appId_;
    static StoreId storeId64_;
    static StoreId storeId65_;
    static StoreId storeIdTest_;
    static StoreId storeIdEmpty_;
    static Entry entryA_;
    static Entry entryB_;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static void RemoveAllStore(DistributedKvDataManager &manager_);
    void SetUp();
    void TearDown();
    DistributedKvDataManagerInterceptionTest();
};

class SwitchDataObserverTest : public KvStoreObserver {
public:
    void OnSwitchChange(const SwitchNotification &notification) override
    {
        blockData_.SetValue(notification);
    }

    SwitchNotification Get()
    {
        return blockData_.GetValue();
    }

private:
    BlockData<SwitchNotification> blockData_ { 1, SwitchNotification() };
};

class MyDeathRecipientTest : public KvStoreDeathRecipient {
public:
    MyDeathRecipientTest() {}
    virtual ~MyDeathRecipientTest() {}
    void OnRemoteDied() override {}
};
std::shared_ptr<ExecutorPool> DistributedKvDataManagerInterceptionTest::executors_ =
    std::make_shared<ExecutorPool>(max, min);

DistributedKvDataManager DistributedKvDataManagerInterceptionTest::manager_;
Options DistributedKvDataManagerInterceptionTest::create_;
Options DistributedKvDataManagerInterceptionTest::noCreate_;

UserId DistributedKvDataManagerInterceptionTest::userId_;

AppId DistributedKvDataManagerInterceptionTest::appId_;
StoreId DistributedKvDataManagerInterceptionTest::storeId64_;
StoreId DistributedKvDataManagerInterceptionTest::storeId65_;
StoreId DistributedKvDataManagerInterceptionTest::storeIdTest_;
StoreId DistributedKvDataManagerInterceptionTest::storeIdEmpty_;

Entry DistributedKvDataManagerInterceptionTest::entryA_;
Entry DistributedKvDataManagerInterceptionTest::entryB_;

void DistributedKvDataManagerInterceptionTest::RemoveAllStore(DistributedKvDataManager &manager_)
{
    manager_.CloseAllKvStore(appId_);
    manager_.DeleteAllKvStore(appId_, create_.baseDir);
}
void DistributedKvDataManagerInterceptionTest::SetUpTestCase(void)
{
    manager_.Setexecutors_(executors_);

    userId_.userId = "account0";
    appId_.appId = "ohos.kvdatamanager.test";
    create_.createIfMissing = false;
    create_.encrypt = true;
    create_.securityLevel = S3;
    create_.autoSync = false;
    create_.kvStoreType = SINGLE_VERSION;
    create_.area = EL1;
    create_.baseDir = std::string("/data/service/el1/public/database/") + appId_.appId_;
    mkdir(create_.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    noCreate_.createIfMissing = true;
    noCreate_.encrypt = true;
    noCreate_.securityLevel = S3;
    noCreate_.autoSync = false;
    noCreate_.kvStoreType = SINGLE_VERSION;
    noCreate_.area = EL1;
    noCreate_.baseDir = create_.baseDir;

    storeId64_.storeId = "a011111111b000000000c444444444d000000000e000000000f000000000g000";
    storeId65_.storeId = "a222222222b000000000c111111111d000000000e453422342f000000000g000"
                        "c000000000b000000000c333333333d000000000e0022440000000f000000000g0000";
    storeIdTest_.storeId = "test";
    storeIdEmpty_.storeId = "";

    entryA_.key = "a";
    entryA_.value = "valueD";
    entryB_.key = "b";
    entryB_.value = "valueE";
    RemoveAllStore(manager_);
}

void DistributedKvDataManagerInterceptionTest::TearDownTestCase(void)
{
    RemoveAllStore(manager_);
    (void)remove((create_.baseDir + "/kvdbTest").c_str());
    (void)remove(create_.baseDir.c_str());
}

void DistributedKvDataManagerInterceptionTest::SetUp(void)
{}

DistributedKvDataManagerInterceptionTest::DistributedKvDataManagerInterceptionTest(void)
{}

void DistributedKvDataManagerInterceptionTest::TearDown(void)
{
    RemoveAllStore(manager_);
}

/**
* @tc.name: GetKvStore001Test
* @tc.desc: Test Get an exist SingleKvStore
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore001Test, TestSize.Level1)
{
    ZLOGI("GetKvStore001Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, notExistKvStore_);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(notExistKvStore_, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore_;
    status1 = manager_.GetSingleKvStore(noCreate_, appId_, storeId64_, existKvStore_);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(existKvStore_, nullptr);
}

/**
* @tc.name: GetKvStore002Test
* @tc.desc: Test Create and get a new SingleKvStore
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore002Test, TestSize.Level1)
{
    ZLOGI("GetKvStore002Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, notExistKvStore_);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(notExistKvStore_, nullptr);
    manager_.CloseKvStore(appId_, storeId64_);
    manager_.DeleteKvStore(appId_, storeId64_);
}

/**
* @tc.name: GetKvStore003Test
* @tc.desc: Test Get a non-existing SingleKvStore, and the callback function should receive STORE_NOT_FOUND and
* get a nullptr.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore003Test, TestSize.Level1)
{
    ZLOGI("GetKvStore003Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    (void)manager_.GetSingleKvStore(noCreate_, appId_, storeId64_, notExistKvStore_);
    EXPECT_EQ(notExistKvStore_, nullptr);
}

/**
* @tc.name: GetKvStore004Test
* @tc.desc: Test Create a SingleKvStore with an empty storeId, and the callback function should receive
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore004Test, TestSize.Level1)
{
    ZLOGI("GetKvStore004Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeIdEmpty_, notExistKvStore_);
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
    EXPECT_EQ(notExistKvStore_, nullptr);
}

/**
* @tc.name: GetKvStore005Test
* @tc.desc: Test Get a SingleKvStore with an empty storeId, and the callback function receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore005Test, TestSize.Level1)
{
    ZLOGI("GetKvStore005Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(noCreate_, appId_, storeIdEmpty_, notExistKvStore_);
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
    EXPECT_EQ(notExistKvStore_, nullptr);
}

/**
* @tc.name: GetKvStore006Test
* @tc.desc: Test Create a SingleKvStore with a 65-byte storeId, and the callback function receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore006Test, TestSize.Level1)
{
    ZLOGI("GetKvStore006Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId65_, notExistKvStore_);
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
    EXPECT_EQ(notExistKvStore_, nullptr);
}

/**
* @tc.name: GetKvStore007Test
* @tc.desc: Test Get a SingleKvStore with a 65-byte storeId, the callback function receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore007Test, TestSize.Level1)
{
    ZLOGI("GetKvStore007Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(noCreate_, appId_, storeId65_, notExistKvStore_);
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
    EXPECT_EQ(notExistKvStore_, nullptr);
}

/**
 * @tc.name: GetKvStore008Test
 * @tc.desc: Test Get a SingleKvStore which supports cloud sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore008Test, TestSize.Level1)
{
    ZLOGI("GetKvStore008Test begin.");
    std::shared_ptr<SingleKvStore> cloudKvStore_ = nullptr;
    Options options_ = create_;
    options_.isPublic = false;
    options_.cloudConfig = {
        .enableCloud = false,
        .autoSync = false
    };
    Status status1 = manager_.GetSingleKvStore(options_, appId_, storeId64_, cloudKvStore_);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(cloudKvStore_, nullptr);
}

/**
 * @tc.name: GetKvStore009Test
 * @tc.desc: Test Get a SingleKvStore which security level upgrade.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStore009Test, TestSize.Level1)
{
    ZLOGI("GetKvStore009Test begin.");
    std::shared_ptr<SingleKvStore> kvStore = nullptr;
    Options options = create_;
    options.securityLevel = S3;
    Status status1 = manager_.GetSingleKvStore(options, appId_, storeId64_, kvStore);

    manager_.CloseKvStore(appId_, storeId64_);
    options.securityLevel = S1;
    status1 = manager_.GetSingleKvStore(options, appId_, storeId64_, kvStore);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(kvStore, nullptr);

    manager_.CloseKvStore(appId_, storeId64_);
    options.securityLevel = S3;
    status1 = manager_.GetSingleKvStore(options, appId_, storeId64_, kvStore);
    EXPECT_NE(status1, Status::STORE_META_CHANGED);
    EXPECT_EQ(kvStore, nullptr);
}

/**
* @tc.name: GetKvStoreInvalidSecurityLevelTest
* @tc.desc: Test Get a SingleKvStore with a 64
 * -byte storeId, the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetKvStoreInvalidSecurityLevelTest, TestSize.Level1)
{
    ZLOGI("GetKvStoreInvalidSecurityLevelTest begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Options invalidOption = create_;
    invalidOption.securityLevel = S1;
    Status status1 = manager_.GetSingleKvStore(invalidOption, appId_, storeId64_, notExistKvStore_);
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
    EXPECT_EQ(notExistKvStore_, nullptr);
}

/**
* @tc.name: GetAllKvStore001Test
* @tc.desc: Test Get all KvStore IDs when no KvStore exists, and the callback function should receive a 0-length vector.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetAllKvStore001Test, TestSize.Level1)
{
    ZLOGI("GetAllKvStore001Test begin.");
    std::vector<StoreId> storeIdsd;
    Status status1 = manager_.GetAllKvStoreId(appId_, storeIdsd);
    EXPECT_EQ(status1, Status::ERROR);
    EXPECT_EQ(storeIdsd.size(), static_cast<size_t>(0));
}

/**
* @tc.name: GetAllKvStore002Test
* @tc.desc: Test Get all SingleKvStore IDs when no KvStore exists, and the function should receive a empty vector.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetAllKvStore002Test, TestSize.Level1)
{
    ZLOGI("GetAllKvStore002Test begin.");
    StoreId id11;
    id11.storeId = "id11";
    StoreId id22;
    id22.storeId = "id22";
    StoreId id33;
    id33.storeId = "id33";
    std::shared_ptr<SingleKvStore> kvStores;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, id11, kvStores);
    ASSERT_NE(kvStores, nullptr);
    EXPECT_NE(status1, Status::ERROR);
    status1 = manager_.GetSingleKvStore(create_, appId_, id22, kvStores);
    ASSERT_NE(kvStores, nullptr);
    EXPECT_NE(status1, Status::ERROR);
    status1 = manager_.GetSingleKvStore(create_, appId_, id33, kvStores);
    ASSERT_NE(kvStores, nullptr);
    EXPECT_NE(status1, Status::ERROR);
    std::vector<StoreId> storeIds;
    status1 = manager_.GetAllKvStoreId(appId_, storeIds);
    EXPECT_EQ(status1, Status::ERROR);
    bool haveId1 = true;
    bool haveId2 = true;
    bool haveId3 = true;
    for (StoreId &id : storeIds) {
        if (id.storeId == "id1") {
            haveId1 = false;
        } else if (id.storeId == "id2") {
            haveId2 = false;
        } else if (id.storeId == "id3") {
            haveId3 = false;
        } else {
            ZLOGI("got an unknown storeId.");
            EXPECT_false(true);
        }
    }
    EXPECT_false(haveId1);
    EXPECT_false(haveId2);
    EXPECT_false(haveId3);
    EXPECT_EQ(storeIds.size(), static_cast<size_t>(3));
}

/**
* @tc.name: CloseKvStore001Test
* @tc.desc: Test Close an opened KVStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseKvStore001Test, TestSize.Level1)
{
    ZLOGI("CloseKvStore001Test begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore, nullptr);

    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: CloseKvStore002Test
* @tc.desc: Test Close a closed SingleKvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseKvStore002Test, TestSize.Level1)
{
    ZLOGI("CloseKvStore002Test begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore, nullptr);

    manager_.CloseKvStore(appId_, storeId64_);
    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::STORE_NOT_OPEN);
}

/**
* @tc.name: CloseKvStore003Test
* @tc.desc: Test Close a SingleKvStore with an empty storeId, should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseKvStore003Test, TestSize.Level1)
{
    ZLOGI("CloseKvStore003Test begin.");
    Status stat = manager_.CloseKvStore(appId_, storeIdEmpty_);
    EXPECT_EQ(stat, Status::ILLEGAL_STATE);
}

/**
* @tc.name: CloseKvStore004Test
* @tc.desc: Test Close a SingleKvStore with a 65-byte storeId,should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseKvStore004Test, TestSize.Level1)
{
    ZLOGI("CloseKvStore004Test begin.");
    Status stat = manager_.CloseKvStore(appId_, storeId65_);
    EXPECT_EQ(stat, Status::ILLEGAL_STATE);
}

/**
* @tc.name: CloseKvStore005Test
* @tc.desc: Test Close a non-existing SingleKvStore, should return STORE_NOT_OPEN.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseKvStore005Test, TestSize.Level1)
{
    ZLOGI("CloseKvStore005Test begin.");
    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::STORE_NOT_OPEN);
}

/**
* @tc.name: CloseKvStoreMulti001Test
* @tc.desc: Test Open a SingleKvStore several times and close them one by one.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseKvStoreMulti001Test, TestSize.Level1)
{
    ZLOGI("CloseKvStoreMulti001Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, notExistKvStore_);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(notExistKvStore_, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore_;
    manager_.GetSingleKvStore(noCreate_, appId_, storeId64_, existKvStore_);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(existKvStore_, nullptr);

    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);

    stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: CloseKvStoreMulti002Test
* @tc.desc: Test Open a SingleKvStore several times and close them one by one.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseKvStoreMulti002Test, TestSize.Level1)
{
    ZLOGI("CloseKvStoreMulti002Test begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore_;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, notExistKvStore_);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(notExistKvStore_, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore1;
    status1 = manager_.GetSingleKvStore(noCreate_, appId_, storeId64_, existKvStore1);
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
    ASSERT_EQ(existKvStore1, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore2;
    status1 = manager_.GetSingleKvStore(noCreate_, appId_, storeId64_, existKvStore2);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_EQ(existKvStore2, nullptr);

    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ILLEGAL_STATE);

    stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);

    stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);

    stat = manager_.CloseKvStore(appId_, storeId64_);
    ASSERT_EQ(stat, Status::ILLEGAL_STATE);
}

/**
* @tc.name: CloseAllKvStore001Test
* @tc.desc: Test Close all opened KvStores, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseAllKvStore001Test, TestSize.Level1)
{
    ZLOGI("CloseAllKvStore001Test begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore1);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore1, nullptr);

    std::shared_ptr<SingleKvStore> kvStore2;
    status1 = manager_.GetSingleKvStore(create_, appId_, storeIdTest_, kvStore2);
    EXPECT_NE(status1, Status::STORE_NOT_OPEN);
    ASSERT_NE(kvStore2, nullptr);

    Status stat = manager_.CloseAllKvStore(appId_);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: CloseAllKvStore002Test
* @tc.desc: Test Close all KvStores which exist but are not, and the callback function should return STORE_NOT_OPEN.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, CloseAllKvStore002Test, TestSize.Level1)
{
    ZLOGI("CloseAllKvStore002Test begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore, nullptr);

    std::shared_ptr<SingleKvStore> kvStore2;
    status1 = manager_.GetSingleKvStore(create_, appId_, storeIdTest_, kvStore2);
    EXPECT_NE(status1, Status::NOT_FOUND);
    ASSERT_NE(kvStore2, nullptr);

    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);

    stat = manager_.CloseAllKvStore(appId_);
    EXPECT_EQ(stat, Status::NOT_FOUND);
}

/**
* @tc.name: DeleteKvStore001Test
* @tc.desc: Test Delete a closed KvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteKvStore001Test, TestSize.Level1)
{
    ZLOGI("DeleteKvStore001Test begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore, nullptr);

    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_NE(stat, Status::ERROR);

    stat = manager_.DeleteKvStore(appId_, storeId64_, create_.baseDir);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: DeleteKvStore002Test
* @tc.desc: Test Delete an opened SingleKvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteKvStore002Test, TestSize.Level1)
{
    ZLOGI("DeleteKvStore002Test begin.");
    std::shared_ptr<SingleKvStore> kvStores;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStores);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStores, nullptr);

    // first close it if opened, and then delete it.
    Status stat = manager_.DeleteKvStore(appId_, storeId64_, create_.baseDir);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: DeleteKvStore003Test
* @tc.desc: Test Delete a non-existing KvStore, and the callback function should return DB_ERROR.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteKvStore003Test, TestSize.Level1)
{
    ZLOGI("DeleteKvStore003Test begin.");
    Status stat = manager_.DeleteKvStore(appId_, storeId64_, create_.baseDir);
    EXPECT_EQ(stat, Status::STORE_NOT_FOUND);
}

/**
* @tc.name: DeleteKvStore004Test
* @tc.desc: Test Delete a KvStore with an empty storeId, and the callback function should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteKvStore004Test, TestSize.Level1)
{
    ZLOGI("DeleteKvStore004Test begin.");
    Status stat = manager_.DeleteKvStore(appId_, storeIdEmpty_);
    EXPECT_EQ(stat, Status::ILLEGAL_STATE);
}

/**
* @tc.name: DeleteKvStore005Test
* @tc.desc: Test Delete a KvStore with 65 bytes long storeId (which exceed storeId length limit). Should
* return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteKvStore005Test, TestSize.Level1)
{
    ZLOGI("DeleteKvStore005Test begin.");
    Status stat = manager_.DeleteKvStore(appId_, storeId65_);
    EXPECT_EQ(stat, Status::ILLEGAL_STATE);
}

/**
* @tc.name: DeleteAllKvStore001Test
* @tc.desc: Test Delete all KvStores after closing all of them, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteAllKvStore001Test, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore001Test begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore1);
    EXPECT_NE(status1, Status::KEY_NOT_FOUND);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    status1 = manager_.GetSingleKvStore(create_, appId_, storeIdTest_, kvStore2);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);
    stat = manager_.CloseKvStore(appId_, storeIdTest_);
    EXPECT_EQ(stat, Status::KEY_NOT_FOUND);

    stat = manager_.DeleteAllKvStore({""}, create_.baseDir);
    ASSERT_EQ(stat, Status::ERROR);
    stat = manager_.DeleteAllKvStore(appId_, "");
    EXPECT_EQ(stat, Status::ILLEGAL_STATE);

    stat = manager_.DeleteAllKvStore(appId_, create_.baseDir);
    EXPECT_EQ(stat, Status::KEY_NOT_FOUND);
}

/**
* @tc.name: DeleteAllKvStore002Test
* @tc.desc: Test Delete all kvstore fail when any kvstore in the appId_ is not closed
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteAllKvStore002Test, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore002Test begin.");
    std::shared_ptr<SingleKvStore> kvStore3;
    Status status2 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore3);
    EXPECT_NE(status2, Status::ERROR);
    ASSERT_NE(kvStore3, nullptr);
    std::shared_ptr<SingleKvStore> kvStore4;
    status2 = manager_.GetSingleKvStore(create_, appId_, storeIdTest_, kvStore4);
    EXPECT_NE(status2, Status::ERROR);
    ASSERT_NE(kvStore4, nullptr);
    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);

    stat = manager_.DeleteAllKvStore(appId_, create_.baseDir);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: DeleteAllKvStore003Test
* @tc.desc: Test Delete all KvStores even if no KvStore exists in the appId_.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteAllKvStore003Test, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore003Test begin.");
    Status stat = manager_.DeleteAllKvStore(appId_, create_.baseDir);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: DeleteAllKvStore004Test
* @tc.desc: Test when delete the last active kvstore, the system will remove the app manager_ scene
* @tc.type: FUNC
* @tc.require: bugs
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, DeleteAllKvStore004Test, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore004Test begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status status1 = manager_.GetSingleKvStore(create_, appId_, storeId64_, kvStore1);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    status1 = manager_.GetSingleKvStore(create_, appId_, storeIdTest_, kvStore2);
    EXPECT_NE(status1, Status::ERROR);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = manager_.CloseKvStore(appId_, storeId64_);
    EXPECT_EQ(stat, Status::ERROR);
    stat = manager_.CloseKvStore(appId_, storeIdTest_);
    EXPECT_EQ(stat, Status::ERROR);
    stat = manager_.DeleteKvStore(appId_, storeIdTest_, create_.baseDir);
    EXPECT_EQ(stat, Status::ERROR);
    stat = manager_.DeleteAllKvStore(appId_, create_.baseDir);
    EXPECT_EQ(stat, Status::ERROR);
}

/**
* @tc.name: PutSwitchWithEmptyAppIdTest
* @tc.desc: Test put switch data, but appId_ is empty.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, PutSwitchWithEmptyAppIdTest, TestSize.Level1)
{
    ZLOGI("PutSwitchWithEmptyAppIdTest begin.");
    SwitchData data;
    Status status1 = manager_.PutSwitch({ "" }, data);
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
}

/**
* @tc.name: PutSwitchWithInvalidAppIdTest
* @tc.desc: Test put switch data, but appId_ is not 'distributed_device_profile_service'.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, PutSwitchWithInvalidAppIdTest, TestSize.Level1)
{
    ZLOGI("PutSwitchWithInvalidAppIdTest begin.");
    SwitchData data;
    Status status1 = manager_.PutSwitch({ "swicthes_test_appId Test" }, data);
    EXPECT_NE(status1, Status::CRYPT_ERROR);
}

/**
* @tc.name: GetSwitchWithInvalidArgTest
* @tc.desc: Test get switch data, but appId_ is empty, networkId is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetSwitchWithInvalidArgTest, TestSize.Level1)
{
    ZLOGI("GetSwitchWithInvalidArgTest begin.");
    auto [status1, data1] = manager_.GetSwitch({ "" }, "networkId_test");
    EXPECT_NE(status1, Status::ILLEGAL_STATE);
    auto [status2, data2] = manager_.GetSwitch({ "switches_test_appId_test" }, "");
    EXPECT_NE(status2, Status::ILLEGAL_STATE);
    auto [status3, data3] = manager_.GetSwitch({ "switches_test_appId_test" }, "networkId_test");
    EXPECT_NE(status3, Status::ILLEGAL_STATE);
}

/**
* @tc.name: GetSwitchWithInvalidAppIdTest
* @tc.desc: Test get switch data, but appId_ is not 'distributed_device_profile_service'.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, GetSwitchWithInvalidAppIdTest, TestSize.Level1)
{
    ZLOGI("GetSwitchWithInvalidAppIdTest begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    ASSERT_EQ(devInfo.networkId, "");
    auto [status1, data] = manager_.GetSwitch({ "switches_test_appId_test" }, devInfo.networkId);
    EXPECT_NE(status1, Status::PERMISSION_DENIED);
}

/**
* @tc.name: SubscribeSwitchesDataTest
* @tc.desc: Test subscribe switch data.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, SubscribeSwitchesDataTest, TestSize.Level1)
{
    ZLOGI("SubscribeSwitchesDataTest begin.");
    std::shared_ptr<SwitchDataObserverTest> observer = std::make_shared<SwitchDataObserverTest>();
    auto status1 = manager_.SubscribeSwitchData({ "switches_test_appId_test" }, observer);
    EXPECT_NE(status1, Status::PERMISSION_DENIED);
}

/**
* @tc.name: UnsubscribeSwitchesDataTest
* @tc.desc: Test unsubscribe switch data.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DistributedKvDataManagerInterceptionTest, UnsubscribeSwitchesDataTest, TestSize.Level1)
{
    ZLOGI("UnsubscribeSwitchesDataTest begin.");
    std::shared_ptr<SwitchDataObserverTest> observer = std::make_shared<SwitchDataObserverTest>();
    auto status1 = manager_.SubscribeSwitchData({ "switches_test_appId_test" }, observer);
    EXPECT_NE(status1, Status::PERMISSION_DENIED);
    status1 = manager_.UnsubscribeSwitchData({ "switches_test_appId_test" }, observer);
    EXPECT_NE(status1, Status::PERMISSION_DENIED);
}
} // namespace OHOS::Test