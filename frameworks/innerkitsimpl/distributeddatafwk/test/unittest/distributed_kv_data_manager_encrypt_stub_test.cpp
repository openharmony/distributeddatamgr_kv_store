/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#define LOG_TAG "DistributedKvDataManagerEncryptStubTest"
#include <gtest/gtest.h>

#include "distributed_kv_data_manager.h"
#include "file_ex.h"
#include "kvstore_death_recipient.h"
#include "log_print.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;

class DistributedKvDataManagerEncryptStubTest : public testing::Test {
public:
    static DistributedKvDataManager manager1;
    static Options createEnc1;

    static UserId userId1;

    static AppId appId1;
    static StoreId storeId1;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    static void RemoveAllStore(DistributedKvDataManager &manager1);

    void SetUp();
    void TearDown();
    DistributedKvDataManagerEncryptStubTest();
    virtual ~DistributedKvDataManagerEncryptStubTest();
};

class MyDeathRecipient : public KvStoreDeathRecipient {
public:
    MyDeathRecipient() { }
    virtual ~MyDeathRecipient() { }
    void OnRemoteDied() override { }
};

DistributedKvDataManager DistributedKvDataManagerEncryptStubTest::manager1;
Options DistributedKvDataManagerEncryptStubTest::createEnc1;

UserId DistributedKvDataManagerEncryptStubTest::userId1;

AppId DistributedKvDataManagerEncryptStubTest::appId1;
StoreId DistributedKvDataManagerEncryptStubTest::storeId1;

void DistributedKvDataManagerEncryptStubTest::RemoveAllStore(DistributedKvDataManager &manager1)
{
    manager1.CloseAllKvStore(appId1);
    manager1.DeleteKvStore(appId1, storeId1, createEnc1.baseDir);
    manager1.DeleteAllKvStore(appId1, createEnc1.baseDir);
}
void DistributedKvDataManagerEncryptStubTest::SetUpTestCase(void)
{
    createEnc1.createIfMissing = false;
    createEnc1.encrypt = false;
    createEnc1.securityLevel = S2;
    createEnc1.autoSync = false;
    createEnc1.kvStoreType = SINGLE_VERSION;

    userId1.userId1 = "account10";
    appId1.appId1 = "com.ohos.service";

    storeId1.storeId1 = "EncryptStoreIdTest";

    createEnc1.area = EL3;
    createEnc1.baseDir = std::string("/data/service/el2/public/database/") + appId1.appId1;
    mkdir(createEnc1.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void DistributedKvDataManagerEncryptStubTest::TearDownTestCase(void)
{
    RemoveAllStore(manager1);
    (void)remove((createEnc1.baseDir + "/kvdb").c_str());
    (void)remove(createEnc1.baseDir.c_str());
}

void DistributedKvDataManagerEncryptStubTest::SetUp(void) { }

DistributedKvDataManagerEncryptStubTest::DistributedKvDataManagerEncryptStubTest(void) { }

DistributedKvDataManagerEncryptStubTest::~DistributedKvDataManagerEncryptStubTest(void) { }

void DistributedKvDataManagerEncryptStubTest::TearDown(void) { }

/**
 * @tc.name: kvstore_ddm_createEncryptedStore_001
 * @tc.desc: Create an encrypted KvStore.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedKvDataManagerEncryptStubTest, kvstore_ddm_createEncryptedStore_001, TestSize.Level1)
{
    ZLOGI("kvstore_ddm_createEncryptedStore_001 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager1.GetSingleKvStore(createEnc1, appId1, storeId1, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    Key key = "ageTest";
    Value value = "18";
    status = kvStore->Put(key, value);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong";

    // get value from kvstore.
    Value valueRet1;
    Status statusRet = kvStore->Get(key, valueRet1);
    EXPECT_EQ(Status::SUCCESS, statusRet) << "get data return wrong";

    EXPECT_EQ(value, valueRet1) << "value and valueRet1 are not";
}