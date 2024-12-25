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
#define LOG_TAG "DistributedKvDataManagerVirtualTest"

#include <gtest/gtest.h>
#include <memory>
#include "distributed_kv_data_manager.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_observer.h"
#include "kv_store_death_recipient.h"
#include "process_communication_impl.h"
#include "process_system_api_adapter_impl.h"
#include "store_manager.h"
#include "store_util.h"
#include "runtime_config.h"
#include "kv_store_service_death_notifier.h"
#include "dds_trace.h"
#include "kvstore_service_death_notifier.h"
#include "kv_store_death_recipient.h"
#include "kv_store_observer.h"
#include "kv_store_delegate_manager.h"
#include "process_communication_impl.h"
#include "process_system_api_adapter_impl.h"
#include "store_manager.h"
#include "store_util.h"
#include "runtime_config.h"
#include "kv_store_service_death_notifier.h"
#include "dds_trace.h"

using namespace OHOS::DistributedKv;
using namespace testing;
using namespace testing::ext;
namespace OHOS::Test {
static constexpr size_t NUM_MIN_V = 5;
static constexpr size_t NUM_MAX_V = 12;
class DistributedKvDataManagerVirtualTest : public testing::Test {
public:
    static std::shared_ptr<ExecutorPool> executors;
    static DistributedKvDataManager managerVirtual;
    static Options createVirtual;
    static Options noCreateVirtual;
    static UserId userIdVirtual;
    static AppId appIdVirtual;
    static StoreId storeId64Virtual;
    static StoreId storeId65Virtual;
    static StoreId storeIdTestVirtual;
    static StoreId storeIdEmptyVirtual;
    static Entry entryAVirtual;
    static Entry entryBVirtual;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};
std::shared_ptr<ExecutorPool> DistributedKvDataManagerTest::executors =
    std::make_shared<ExecutorPool>(NUM_MAX_V, NUM_MIN_V);

void DistributedKvDataManagerVirtualTest::SetUpTestCase(void)
{}

void DistributedKvDataManagerVirtualTest::TearDownTestCase(void)
{}

void DistributedKvDataManagerVirtualTest::SetUp(void)
{}

void DistributedKvDataManagerVirtualTest::TearDown(void)
{}

class ChangeNotificationVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void ChangeNotificationVirtualTest::SetUpTestCase(void)
{}

void ChangeNotificationVirtualTest::TearDownTestCase(void)
{}

void ChangeNotificationVirtualTest::SetUp(void)
{}

void ChangeNotificationVirtualTest::TearDown(void)
{}

/**
 * @tc.name: GetKvStore001
 * @tc.desc: Get an exist SingleKvStore
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlGetKvStore001
 */
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore001, TestSize.Level1)
{
    ZLOGI("GetKvStore001 begin.");
    DistributedKvDataManager managerVirtual;
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(notExistKvStoreVirtual, nullptr);

    std::shared_ptr<SingleKvStore> existKvStoreVirtual;
    statusVirtual =
        managerVirtual.GetSingleKvStore(noCreateVirtual, appIdVirtual, storeId64Virtual, existKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(existKvStoreVirtual, nullptr);
}

/**
* @tc.name: GetKvStore002
* @tc.desc: Create and get a new SingleKvStore
* @tc.type: FUNC
* @tc.require: GetKvStore002
* @tc.author:  sqlGetKvStore002
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore002, TestSize.Level1)
{
    ZLOGI("GetKvStore002 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(notExistKvStoreVirtual, nullptr);
    managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    managerVirtual.DeleteKvStore(appIdVirtual, storeId64Virtual);
}

/**
* @tc.name: GetKvStore003
* @tc.desc: Get a non-existing SingleKvStore, and the callback function should receive STORE_NOT_FOUND and
* get a nullptr.
* @tc.type: FUNC
* @tc.require: GetKvStore003
* @tc.author:  sqlGetKvStore003
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore003, TestSize.Level1)
{
    ZLOGI("GetKvStore003 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    (void)managerVirtual.GetSingleKvStore(noCreateVirtual, appIdVirtual, storeId64Virtual, notExistKvStoreVirtual);
    EXPECT_EQ(notExistKvStoreVirtual, nullptr);
}

/**
* @tc.name: GetKvStore004
* @tc.desc: Create a SingleKvStore with an empty storeId, and the callback function should receive
* @tc.type: FUNC
* @tc.require: GetKvStore004
* @tc.author:  sqlGetKvStore004
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore004, TestSize.Level1)
{
    ZLOGI("GetKvStore004 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeIdEmptyVirtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStoreVirtual, nullptr);
}

/**
* @tc.name: GetKvStore005
* @tc.desc: Get a SingleKvStore with an empty storeId, and the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require: GetKvStore005
* @tc.author:  sqlGetKvStore005
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore005, TestSize.Level1)
{
    ZLOGI("GetKvStore005 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(noCreateVirtual, appIdVirtual, storeIdEmptyVirtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStoreVirtual, nullptr);
}

/**
* @tc.name: GetKvStore006
* @tc.desc: Create a SingleKvStore with a 65-byte storeId, and the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require: GetKvStore006
* @tc.author:  sqlGetKvStore006
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore006, TestSize.Level1)
{
    ZLOGI("GetKvStore006 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId65Virtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStoreVirtual, nullptr);
}

/**
* @tc.name: GetKvStore007
* @tc.desc: Get a SingleKvStore with a 65-byte storeId, the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require: GetKvStore007
* @tc.author:  sqlGetKvStore007
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore007, TestSize.Level1)
{
    ZLOGI("GetKvStore007 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(noCreateVirtual, appIdVirtual, storeId65Virtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStoreVirtual, nullptr);
}

/**
 * @tc.name: GetKvStore008
 * @tc.desc: Get a SingleKvStore which supports cloud sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlGetKvStore008
 */
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore008, TestSize.Level1)
{
    ZLOGI("GetKvStore008 begin.");
    std::shared_ptr<SingleKvStore> cloudKvStore = nullptr;
    Options options = createVirtual;
    options.isPublic = true;
    options.cloudConfig = {
        .enableCloud = true,
        .autoSync = true
    };
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(options, appIdVirtual, storeId64Virtual, cloudKvStore);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(cloudKvStore, nullptr);
}

/**
 * @tc.name: GetKvStore009
 * @tc.desc: Get a SingleKvStore which security level upgrade.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:  sqlGetKvStore009
 */
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStore009, TestSize.Level1)
{
    ZLOGI("GetKvStore009 begin.");
    std::shared_ptr<SingleKvStore> kvStore = nullptr;
    Options options = createVirtual;
    options.securityLevel = S1;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(options, appIdVirtual, storeId64Virtual, kvStore);

    managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    options.securityLevel = S2;
    statusVirtual =
        managerVirtual.GetSingleKvStore(options, appIdVirtual, storeId64Virtual, kvStore);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(kvStore, nullptr);

    managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    options.securityLevel = S1;
    statusVirtual =
        managerVirtual.GetSingleKvStore(options, appIdVirtual, storeId64Virtual, kvStore);
    ASSERT_EQ(statusVirtual, Status::STORE_META_CHANGED);
    EXPECT_EQ(kvStore, nullptr);
}

/**
* @tc.name: GetKvStoreInvalidSecurityLevel
* @tc.desc: Get a SingleKvStore with a 64
 * -byte storeId, the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require: SR000IIM2J AR000IIMLL
* @tc.author:  sqlGetKvStoreInvalidSecurityLevel
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetKvStoreInvalidSecurityLevel, TestSize.Level1)
{
    ZLOGI("GetKvStoreInvalidSecurityLevel begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Options invalidOption = createVirtual;
    invalidOption.securityLevel = INVALID_LABEL;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(invalidOption, appIdVirtual, storeId64Virtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStoreVirtual, nullptr);
}

/**
* @tc.name: GetAllKvStore001
* @tc.desc: Get all KvStore IDs when no KvStore exists, and the callback function should receive a 0-length vector.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlGetAllKvStore001
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetAllKvStore001, TestSize.Level1)
{
    ZLOGI("GetAllKvStore001 begin.");
    std::vector<StoreId> storeIds;
    Status statusVirtual = managerVirtual.GetAllKvStoreId(appIdVirtual, storeIds);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_EQ(storeIds.size(), static_cast<size_t>(0));
}

/**
* @tc.name: GetAllKvStore002
* @tc.desc: Get all SingleKvStore IDs when no KvStore exists, and the callback function should receive a empty vector.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlGetAllKvStore002
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetAllKvStore002, TestSize.Level1)
{
    ZLOGI("GetAllKvStore002 begin.");
    StoreId id1;
    id1.storeId = "id1";
    StoreId id2;
    id2.storeId = "id2";
    StoreId id3;
    id3.storeId = "id3";
    std::shared_ptr<SingleKvStore> kvStore;
    Status statusVirtual = managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, id1, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    statusVirtual = managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, id2, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    statusVirtual = managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, id3, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    std::vector<StoreId> storeIds;
    statusVirtual = managerVirtual.GetAllKvStoreId(appIdVirtual, storeIds);
    EXPECT_EQ(statusVirtual, Status::SUCCESS);
    bool haveId1 = false;
    bool haveId2 = false;
    bool haveId3 = false;
    for (StoreId &id : storeIds) {
        if (id.storeId == "id1") {
            haveId1 = true;
        } else if (id.storeId == "id2") {
            haveId2 = true;
        } else if (id.storeId == "id3") {
            haveId3 = true;
        } else {
            ZLOGI("got an unknown storeId.");
            EXPECT_TRUE(false);
        }
    }
    EXPECT_TRUE(haveId1);
    EXPECT_TRUE(haveId2);
    EXPECT_TRUE(haveId3);
    EXPECT_EQ(storeIds.size(), static_cast<size_t>(3));
}

/**
* @tc.name: CloseKvStore001
* @tc.desc: Close an opened KVStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlCloseKvStore001
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseKvStore001, TestSize.Level1)
{
    ZLOGI("CloseKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseKvStore002
* @tc.desc: Close a closed SingleKvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlCloseKvStore002
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseKvStore002, TestSize.Level1)
{
    ZLOGI("CloseKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::STORE_NOT_OPEN);
}

/**
* @tc.name: CloseKvStore003
* @tc.desc: Close a SingleKvStore with an empty storeId, and the callback function should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlCloseKvStore003
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseKvStore003, TestSize.Level1)
{
    ZLOGI("CloseKvStore003 begin.");
    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeIdEmptyVirtual);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: CloseKvStore004
* @tc.desc: Close a SingleKvStore with a 65-byte storeId, and the callback function should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlCloseKvStore004
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseKvStore004, TestSize.Level1)
{
    ZLOGI("CloseKvStore004 begin.");
    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId65Virtual);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: CloseKvStore005
* @tc.desc: Close a non-existing SingleKvStore, and the callback function should return STORE_NOT_OPEN.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseKvStore005, TestSize.Level1)
{
    ZLOGI("CloseKvStore005 begin.");
    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::STORE_NOT_OPEN);
}

/**
* @tc.name: CloseKvStoreMulti001
* @tc.desc: Open a SingleKvStore several times and close them one by one.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000CSKRU
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseKvStoreMulti001, TestSize.Level1)
{
    ZLOGI("CloseKvStoreMulti001 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(notExistKvStoreVirtual, nullptr);

    std::shared_ptr<SingleKvStore> existKvStoreVirtual;
    managerVirtual.GetSingleKvStore(noCreateVirtual, appIdVirtual, storeId64Virtual, existKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(existKvStoreVirtual, nullptr);

    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseKvStoreMulti002
* @tc.desc: Open a SingleKvStore several times and close them one by one.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000CSKRU
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseKvStoreMulti002, TestSize.Level1)
{
    ZLOGI("CloseKvStoreMulti002 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStoreVirtual;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, notExistKvStoreVirtual);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(notExistKvStoreVirtual, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore1;
    statusVirtual =
        managerVirtual.GetSingleKvStore(noCreateVirtual, appIdVirtual, storeId64Virtual, existKvStore1);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(existKvStore1, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore2;
    statusVirtual =
        managerVirtual.GetSingleKvStore(noCreateVirtual, appIdVirtual, storeId64Virtual, existKvStore2);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    EXPECT_NE(existKvStore2, nullptr);

    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_NE(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseAllKvStore001
* @tc.desc: Close all opened KvStores, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseAllKvStore001, TestSize.Level1)
{
    ZLOGI("CloseAllKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore1);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);

    std::shared_ptr<SingleKvStore> kvStore2;
    statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeIdTestVirtual, kvStore2);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);

    Status stat = managerVirtual.CloseAllKvStore(appIdVirtual);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseAllKvStore002
* @tc.desc: Close all KvStores which exist but are not opened, and the callback function should return STORE_NOT_OPEN.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, CloseAllKvStore002, TestSize.Level1)
{
    ZLOGI("CloseAllKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    std::shared_ptr<SingleKvStore> kvStore2;
    statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeIdTestVirtual, kvStore2);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);

    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.CloseAllKvStore(appIdVirtual);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteKvStore001
* @tc.desc: Delete a closed KvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteKvStore001, TestSize.Level1)
{
    ZLOGI("DeleteKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    ASSERT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.DeleteKvStore(appIdVirtual, storeId64Virtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteKvStore002
* @tc.desc: Delete an opened SingleKvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteKvStore002, TestSize.Level1)
{
    ZLOGI("DeleteKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    // first close it if opened, and then delete it.
    Status stat = managerVirtual.DeleteKvStore(appIdVirtual, storeId64Virtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteKvStore003
* @tc.desc: Delete a non-existing KvStore, and the callback function should return DB_ERROR.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteKvStore003, TestSize.Level1)
{
    ZLOGI("DeleteKvStore003 begin.");
    Status stat = managerVirtual.DeleteKvStore(appIdVirtual, storeId64Virtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::STORE_NOT_FOUND);
}

/**
* @tc.name: DeleteKvStore004
* @tc.desc: Delete a KvStore with an empty storeId, and the callback function should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteKvStore004, TestSize.Level1)
{
    ZLOGI("DeleteKvStore004 begin.");
    Status stat = managerVirtual.DeleteKvStore(appIdVirtual, storeIdEmptyVirtual);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: DeleteKvStore005
* @tc.desc: Delete a KvStore with 65 bytes long storeId (which exceed storeId length limit). Should
* return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteKvStore005, TestSize.Level1)
{
    ZLOGI("DeleteKvStore005 begin.");
    Status stat = managerVirtual.DeleteKvStore(appIdVirtual, storeId65Virtual);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: DeleteAllKvStore001
* @tc.desc: Delete all KvStores after closing all of them, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteAllKvStore001, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore1);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeIdTestVirtual, kvStore2);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = managerVirtual.CloseKvStore(appIdVirtual, storeIdTestVirtual);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.DeleteAllKvStore({""}, createVirtual.baseDir);
    EXPECT_NE(stat, Status::SUCCESS);
    stat = managerVirtual.DeleteAllKvStore(appIdVirtual, "");
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);

    stat = managerVirtual.DeleteAllKvStore(appIdVirtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteAllKvStore002
* @tc.desc: Delete all kvstore fail when any kvstore in the appIdVirtual is not closed
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteAllKvStore002, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore1);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeIdTestVirtual, kvStore2);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = managerVirtual.DeleteAllKvStore(appIdVirtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteAllKvStore003
* @tc.desc: Delete all KvStores even if no KvStore exists in the appIdVirtual.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author:  sqlliqiao
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteAllKvStore003, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore003 begin.");
    Status stat = managerVirtual.DeleteAllKvStore(appIdVirtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteAllKvStore004
* @tc.desc: when delete the last active kvstore, the system will remove the app managerVirtual scene
* @tc.type: FUNC
* @tc.require: bugs
* @tc.author:  sqlSven Wang
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, DeleteAllKvStore004, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore004 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeId64Virtual, kvStore1);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    statusVirtual =
        managerVirtual.GetSingleKvStore(createVirtual, appIdVirtual, storeIdTestVirtual, kvStore2);
    ASSERT_EQ(statusVirtual, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = managerVirtual.CloseKvStore(appIdVirtual, storeId64Virtual);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = managerVirtual.CloseKvStore(appIdVirtual, storeIdTestVirtual);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = managerVirtual.DeleteKvStore(appIdVirtual, storeIdTestVirtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = managerVirtual.DeleteAllKvStore(appIdVirtual, createVirtual.baseDir);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: PutSwitchWithEmptyAppId
* @tc.desc: put switch data, but appIdVirtual is empty.
* @tc.type: FUNC
* @tc.require:
* @tc.author:  sqlzuojiangjiang
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, PutSwitchWithEmptyAppId, TestSize.Level1)
{
    ZLOGI("PutSwitchWithEmptyAppId begin.");
    SwitchData data;
    Status statusVirtual = managerVirtual.PutSwitch({ "" }, data);
    ASSERT_EQ(statusVirtual, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: PutSwitchWithInvalidAppId
* @tc.desc: put switch data, but appIdVirtual is not 'distributed_device_profile_service'.
* @tc.type: FUNC
* @tc.require:
* @tc.author:  sqlzuojiangjiang
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, PutSwitchWithInvalidAppId, TestSize.Level1)
{
    ZLOGI("PutSwitchWithInvalidAppId begin.");
    SwitchData data;
    Status statusVirtual = managerVirtual.PutSwitch({ "swicthes_test_appId" }, data);
    ASSERT_EQ(statusVirtual, Status::PERMISSION_DENIED);
}

/**
* @tc.name: GetSwitchWithInvalidArg
* @tc.desc: get switch data, but appIdVirtual is empty, networkId is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author:  sqlzuojiangjiang
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetSwitchWithInvalidArg, TestSize.Level1)
{
    ZLOGI("GetSwitchWithInvalidArg begin.");
    auto [status1, data1] = managerVirtual.GetSwitch({ "" }, "networkId_test");
    ASSERT_EQ(status1, Status::INVALID_ARGUMENT);
    auto [status2, data2] = managerVirtual.GetSwitch({ "switches_test_appId" }, "");
    ASSERT_EQ(status2, Status::INVALID_ARGUMENT);
    auto [status3, data3] = managerVirtual.GetSwitch({ "switches_test_appId" }, "networkId_test");
    ASSERT_EQ(status3, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: GetSwitchWithInvalidAppId
* @tc.desc: get switch data, but appIdVirtual is not 'distributed_device_profile_service'.
* @tc.type: FUNC
* @tc.require:
* @tc.author:  sqlzuojiangjiang
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, GetSwitchWithInvalidAppId, TestSize.Level1)
{
    ZLOGI("GetSwitchWithInvalidAppId begin.");
    auto devInfo = DevManager::GetInstance().GetLocalDevice();
    EXPECT_NE(devInfo.networkId, "");
    auto [statusVirtual, data] = managerVirtual.GetSwitch({ "switches_test_appId" }, devInfo.networkId);
    ASSERT_EQ(statusVirtual, Status::PERMISSION_DENIED);
}

/**
* @tc.name: SubscribeSwitchesData
* @tc.desc: subscribe switch data.
* @tc.type: FUNC
* @tc.require:
* @tc.author:  sqlzuojiangjiang
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, SubscribeSwitchesData, TestSize.Level1)
{
    ZLOGI("SubscribeSwitchesData begin.");
    std::shared_ptr<SwitchDataObserver> observer = std::make_shared<SwitchDataObserver>();
    auto statusVirtual = managerVirtual.SubscribeSwitchData({ "switches_test_appId" }, observer);
    ASSERT_EQ(statusVirtual, Status::PERMISSION_DENIED);
}

/**
* @tc.name: UnsubscribeSwitchesData
* @tc.desc: unsubscribe switch data.
* @tc.type: FUNC
* @tc.require:
* @tc.author:  sqlzuojiangjiang
*/
HWTEST_F(DistributedKvDataManagerVirtualTest, UnsubscribeSwitchesData, TestSize.Level1)
{
    ZLOGI("UnsubscribeSwitchesData begin.");
    std::shared_ptr<SwitchDataObserver> observer = std::make_shared<SwitchDataObserver>();
    auto statusVirtual = managerVirtual.SubscribeSwitchData({ "switches_test_appId" }, observer);
    ASSERT_EQ(statusVirtual, Status::PERMISSION_DENIED);
    statusVirtual = managerVirtual.UnsubscribeSwitchData({ "switches_test_appId" }, observer);
    ASSERT_EQ(statusVirtual, Status::PERMISSION_DENIED);
}
} // namespace OHOS::Test