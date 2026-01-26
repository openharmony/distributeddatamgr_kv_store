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

#include "kv_general_ut.h"
#include "distributeddb_tools_unit_test.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDBUnitTest;
class DistributedDBBasicKVTest : public KVGeneralUt {
public:
    void SetUp() override;
};

void DistributedDBBasicKVTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto storeInfo3 = GetStoreInfo3();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo3, "dev3"), E_OK);
}

#ifdef USE_DISTRIBUTEDDB_DEVICE
/**
 * @tc.name: ExampleSync001
 * @tc.desc: Test sync from dev1 to dev2.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicKVTest, ExampleSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync to dev2
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockPush(storeInfo1, storeInfo2);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev2's schemaVersion and softwareVersion both equal to dev1's meta
     * @tc.expected: step2. version both equal.
     */
    auto [errCode, version] = GetRemoteSoftwareVersion(storeInfo1, "dev2", DBConstant::DEFAULT_USER);
    EXPECT_EQ(errCode, OK);
    EXPECT_EQ(version, static_cast<uint64_t>(SOFTWARE_VERSION_CURRENT));
    std::tie(errCode, version) = GetRemoteSchemaVersion(storeInfo1, "dev2", DBConstant::DEFAULT_USER);
    EXPECT_EQ(errCode, OK);
    EXPECT_NE(version, 0);
    uint64_t store2SchemaVersion = 0;
    std::tie(errCode, store2SchemaVersion) = GetLocalSchemaVersion(storeInfo2);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(version, store2SchemaVersion);
}

/**
 * @tc.name: ExampleSync002
 * @tc.desc: Test sync from dev1 to dev2 with 2 packet.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicKVTest, ExampleSync002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k1,v1) and (k2,v2)
     * @tc.expected: step1. put ok.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    EXPECT_EQ(store1->Put(k1, v1), OK);
    Key k2 = {'k', '2'};
    Value v2 = {'v', '2'};
    EXPECT_EQ(store1->Put(k2, v2), OK);
    /**
     * @tc.steps: step2. dev1 sync to dev2 with mtu=1
     * @tc.expected: step2. sync ok.
     */
    SetMtu("dev1", 1);
    BlockPush(storeInfo1, storeInfo2);
}
#endif

/**
 * @tc.name: WhitelistKvGet001
 * @tc.desc: Test kv get interface for whitelist.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xd
 */
HWTEST_F(DistributedDBBasicKVTest, WhitelistKvGet001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set whitelist appId, put (k,v)
     * @tc.expected: step1. get (k,v) result.
     */
    auto storeInfo3 = GetStoreInfo3();
    auto store3 = GetDelegate(storeInfo3);
    ASSERT_NE(store3, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store3->Put({'k'}, expectValue), OK);
    Value actualValue;
    EXPECT_EQ(store3->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. with transaction, set whitelist appId, put (k2,v)
     * @tc.expected: step2. get (k2,v) result.
     */
    store3->StartTransaction();
    EXPECT_EQ(store3->Put({'k', '2'}, expectValue), OK);
    Value actualValue2;
    EXPECT_EQ(store3->Get({'k', '2'}, actualValue2), OK);
    EXPECT_EQ(actualValue2, expectValue);
    store3->Commit();
    /**
     * @tc.steps: step3. do not set whitelist appId, put (k,v)
     * @tc.expected: step3. get (k,v) result.
     */
    auto storeInfo2 = GetStoreInfo2();
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->Put({'k'}, expectValue), OK);
    Value actualValue3;
    EXPECT_EQ(store2->Get({'k'}, actualValue3), OK);
    EXPECT_EQ(actualValue3, expectValue);
}

/**
 * @tc.name: WhitelistKvGet002
 * @tc.desc: Test Get if isinwhite
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicKVTest, WhitelistKvGet002, TestSize.Level0)
{
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    Value actualValue;
    EXPECT_EQ(store1->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);

    EXPECT_EQ(store1->StartTransaction(), OK);
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    EXPECT_EQ(store1->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    EXPECT_EQ(store1->Commit(), OK);

    auto storeInfo2 = GetStoreInfo2();
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->Put({'k'}, expectValue), OK);
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);

    EXPECT_EQ(store2->StartTransaction(), OK);
    EXPECT_EQ(store2->Put({'k'}, expectValue), OK);
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    EXPECT_EQ(store2->Commit(), OK);
}

/**
 * @tc.name: LocalPut001
 * @tc.desc: Test kv local put.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicKVTest, LocalPut001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k1,v1) and (k2,v2)
     * @tc.expected: step1. put ok.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    Value actualValue;
    EXPECT_EQ(store1->PutLocal(k1, v1), OK);
    EXPECT_EQ(store1->GetLocal(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);
    Key k2 = {'k', '2'};
    Value v2 = {'v', '2'};
    EXPECT_EQ(store1->PutLocal(k2, v2), OK);
    EXPECT_EQ(store1->GetLocal(k2, actualValue), OK);
    EXPECT_EQ(v2, actualValue);
}

/**
 * @tc.name: LocalPut002
 * @tc.desc: Test kv local put.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicKVTest, LocalPut002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put batch (k1,v1) and (k2,v2)
     * @tc.expected: step1. put ok.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    std::vector<Entry> entries;
    entries.push_back({k1, v1});
    Key k2 = {'k', '2'};
    Value v2 = {'v', '2'};
    entries.push_back({k2, v2});
    ASSERT_EQ(store1->PutLocalBatch(entries), OK);
    Value actualValue;
    EXPECT_EQ(store1->GetLocal(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);
    EXPECT_EQ(store1->GetLocal(k2, actualValue), OK);
    EXPECT_EQ(v2, actualValue);
}

/**
 * @tc.name: LocalPut003
 * @tc.desc: Test kv local put and publish.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicKVTest, LocalPut003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k1,v1)
     * @tc.expected: step1. put ok.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    Value actualValue;
    EXPECT_EQ(store1->PutLocal(k1, v1), OK);
    EXPECT_EQ(store1->GetLocal(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);
    /**
     * @tc.steps: step2. dev1 PublishLocal
     * @tc.expected: step2. local not exist (k1,v1), sync exist (k1,v1).
     */
    EXPECT_EQ(store1->PublishLocal(k1, true, false, nullptr), OK);
    EXPECT_EQ(store1->GetLocal(k1, actualValue), NOT_FOUND);
    EXPECT_EQ(store1->Get(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);
}


/**
 * @tc.name: LocalPut004
 * @tc.desc: Test kv put and unpublish.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicKVTest, LocalPut004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k1,v1)
     * @tc.expected: step1. put ok.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    Value actualValue;
    EXPECT_EQ(store1->Put(k1, v1), OK);
    EXPECT_EQ(store1->Get(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);
    /**
     * @tc.steps: step2. dev1 unpublished to local
     * @tc.expected: step2. local exist (k1,v1), sync not exist (k1,v1).
     */
    EXPECT_EQ(store1->UnpublishToLocal(k1, true, false), OK);
    EXPECT_EQ(store1->Get(k1, actualValue), NOT_FOUND);
    EXPECT_EQ(store1->GetLocal(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);
}

/**
 * @tc.name: GetKvStore001
 * @tc.desc: Test different path manager to open db with the same storeInfo
 * @tc.type: FUNC
 * @tc.author: bty
 */
HWTEST_F(DistributedDBBasicKVTest, GetKvStore001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. path is A, open store1, put (k1,v1)
     * @tc.expected: step1. put ok.
     */
    CloseAllDelegate();
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    KvStoreConfig cfg1;
    cfg1.dataDir = testDir + "/kv1";
    DBCommon::CreateDirectory(cfg1.dataDir);
    SetKvStoreConfig(cfg1);
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    Value actualValue;
    EXPECT_EQ(store1->Put(k1, v1), OK);
    EXPECT_EQ(store1->Get(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);

    /**
     * @tc.steps: step2. path is B, open store1, get k1
     * @tc.expected: step2. k1 not found.
     */
    KvStoreConfig cfg2;
    cfg2.dataDir = testDir + "/kv2";
    DBCommon::CreateDirectory(cfg2.dataDir);
    SetKvStoreConfig(cfg2);
    auto storeInfo2 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_NE(store2->Get(k1, actualValue), OK);
    EXPECT_EQ(store2->Put(k1, v1), OK);
    EXPECT_EQ(store2->Get(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue);

    /**
     * @tc.steps: step3. close db
     * @tc.expected: step3. ok.
     */
    CloseDelegate(storeInfo2);
    EXPECT_EQ(DeleteKvStore(storeInfo2), OK);
}

/**
 * @tc.name: GetKvStore002
 * @tc.desc: Test open the memory db twice on different paths
 * @tc.type: FUNC
 * @tc.author: bty
 */
HWTEST_F(DistributedDBBasicKVTest, GetKvStore002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. path is A, open memory store1, put (k1,v1)
     * @tc.expected: step1. put ok.
     */
    CloseAllDelegate();
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    KvStoreConfig cfg1;
    cfg1.dataDir = testDir + "/kv1";
    DBCommon::CreateDirectory(cfg1.dataDir);
    SetKvStoreConfig(cfg1);
    KvStoreNbDelegate::Option option;
    option.isMemoryDb = true;
    SetOption(option);
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    EXPECT_EQ(store1->Put(k1, v1), OK);

    /**
     * @tc.steps: step2. path is B, open memory store1, get k1
     * @tc.expected: step2. k1 not found.
     */
    KvStoreConfig cfg2;
    cfg2.dataDir = testDir + "/kv2";
    DBCommon::CreateDirectory(cfg2.dataDir);
    SetKvStoreConfig(cfg2);
    auto storeInfo2 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_EQ(store2->Get(k1, actualValue), OK);

    /**
     * @tc.steps: step3. delete kv store
     * @tc.expected: step3. success.
     */
    CloseDelegate(storeInfo2);
    SetKvStoreConfig(cfg1);
    EXPECT_EQ(DeleteKvStore(storeInfo1), BUSY);
    CloseDelegate(storeInfo1);
    EXPECT_EQ(DeleteKvStore(storeInfo1), NOT_FOUND);
}

/**
 * @tc.name: GetKvStore003
 * @tc.desc: Test use different paths to open db and memory db
 * @tc.type: FUNC
 * @tc.author: bty
 */
HWTEST_F(DistributedDBBasicKVTest, GetKvStore003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. path is A, open store1, put (k1,v1)
     * @tc.expected: step1. put ok.
     */
    CloseAllDelegate();
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    KvStoreConfig cfg1;
    cfg1.dataDir = testDir + "/kv1";
    DBCommon::CreateDirectory(cfg1.dataDir);
    SetKvStoreConfig(cfg1);
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    EXPECT_EQ(store1->Put(k1, v1), OK);

    /**
     * @tc.steps: step2. path is B, open memory store1, get k1
     * @tc.expected: step2. k1 not found.
     */
    KvStoreNbDelegate::Option option;
    option.isMemoryDb = true;
    SetOption(option);
    KvStoreConfig cfg2;
    cfg2.dataDir = testDir + "/kv2";
    DBCommon::CreateDirectory(cfg2.dataDir);
    SetKvStoreConfig(cfg2);
    auto storeInfo2 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_NE(store2->Get(k1, actualValue), OK);

    /**
     * @tc.steps: step3. path is A, open memory store1, get k1
     * @tc.expected: step3. k1 not found.
     */
    SetKvStoreConfig(cfg1);
    auto storeInfo3 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo3, "dev3"), E_OK);
    auto store3 = GetDelegate(storeInfo3);
    ASSERT_NE(store3, nullptr);
    EXPECT_NE(store3->Get(k1, actualValue), OK);
}

/**
 * @tc.name: GetKvStore004
 * @tc.desc: Test use different paths to open db and memory db
 * @tc.type: FUNC
 * @tc.author: bty
 */
HWTEST_F(DistributedDBBasicKVTest, GetKvStore004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. path is A, open memory store1, put (k1,v1)
     * @tc.expected: step1. put ok.
     */
    CloseAllDelegate();
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    KvStoreConfig cfg1;
    cfg1.dataDir = testDir + "/kv1";
    DBCommon::CreateDirectory(cfg1.dataDir);
    SetKvStoreConfig(cfg1);
    KvStoreNbDelegate::Option option;
    option.isMemoryDb = true;
    SetOption(option);
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    EXPECT_EQ(store1->Put(k1, v1), OK);

    /**
     * @tc.steps: step2. path is B, open store1, get k1
     * @tc.expected: step2. k1 not found.
     */
    option.isMemoryDb = false;
    SetOption(option);
    KvStoreConfig cfg2;
    cfg2.dataDir = testDir + "/kv2";
    DBCommon::CreateDirectory(cfg2.dataDir);
    SetKvStoreConfig(cfg2);
    auto storeInfo2 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_NE(store2->Get(k1, actualValue), OK);
    EXPECT_EQ(store2->Put(k1, v1), OK);

    /**
     * @tc.steps: step3. path is A, open store1, get k1
     * @tc.expected: step3. k1 not found.
     */
    SetKvStoreConfig(cfg1);
    auto storeInfo3 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo3, "dev3"), E_OK);
    auto store3 = GetDelegate(storeInfo3);
    ASSERT_NE(store3, nullptr);
    EXPECT_NE(store3->Get(k1, actualValue), OK);
}

/**
 * @tc.name: GetKvStore005
 * @tc.desc: Test use createDirByStoreIdOnly to open different path db
 * @tc.type: FUNC
 * @tc.author: bty
 */
HWTEST_F(DistributedDBBasicKVTest, GetKvStore005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. path is A, open store1, set createDirByStoreIdOnly, put (k1,v1)
     * @tc.expected: step1. put ok.
     */
    CloseAllDelegate();
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    KvStoreConfig cfg1;
    cfg1.dataDir = testDir + "/kv1";
    DBCommon::CreateDirectory(cfg1.dataDir);
    SetKvStoreConfig(cfg1);
    KvStoreNbDelegate::Option option;
    option.createDirByStoreIdOnly = true;
    SetOption(option);
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    EXPECT_EQ(store1->Put(k1, v1), OK);

    /**
     * @tc.steps: step2. path is B, open store1, get k1
     * @tc.expected: step2. k1 not found.
     */
    option.createDirByStoreIdOnly = false;
    SetOption(option);
    KvStoreConfig cfg2;
    cfg2.dataDir = testDir + "/kv2";
    DBCommon::CreateDirectory(cfg2.dataDir);
    SetKvStoreConfig(cfg2);
    auto storeInfo2 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_NE(store2->Get(k1, actualValue), OK);
}

/**
 * @tc.name: GetKvStore006
 * @tc.desc: Test enable when open db with different paths
 * @tc.type: FUNC
 * @tc.author: bty
 */
HWTEST_F(DistributedDBBasicKVTest, GetKvStore006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. path is A, open store1, put (k1,v1), enable
     * @tc.expected: step1. ok.
     */
    CloseAllDelegate();
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    KvStoreConfig cfg1;
    cfg1.dataDir = testDir + "/kv1";
    DBCommon::CreateDirectory(cfg1.dataDir);
    SetKvStoreConfig(cfg1);
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    EXPECT_EQ(store1->Put(k1, v1), OK);
    KvStoreDelegateManager manager1(storeInfo1.appId, storeInfo1.userId);
    manager1.SetKvStoreConfig(cfg1);
    AutoLaunchOption autoLaunchOption;
    autoLaunchOption.dataDir = cfg1.dataDir;
    AutoLaunchNotifier notifer;
    EXPECT_EQ(manager1.EnableKvStoreAutoLaunch(storeInfo1.userId, storeInfo1.appId, storeInfo1.storeId,
        autoLaunchOption, notifer), OK);
    
    /**
     * @tc.steps: step2. path is b, open store1, enable
     * @tc.expected: step2. ALREADY_SET.
     */
    KvStoreConfig cfg2;
    cfg2.dataDir = testDir + "/kv2";
    DBCommon::CreateDirectory(cfg2.dataDir);
    SetKvStoreConfig(cfg2);
    auto storeInfo2 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_NE(store2->Get(k1, actualValue), OK);
    KvStoreDelegateManager manager2(storeInfo2.appId, storeInfo2.userId);
    manager2.SetKvStoreConfig(cfg2);
    autoLaunchOption.dataDir = cfg2.dataDir;
    EXPECT_EQ(manager2.EnableKvStoreAutoLaunch(storeInfo2.userId, storeInfo2.appId, storeInfo2.storeId,
        autoLaunchOption, notifer), ALREADY_SET);
    EXPECT_EQ(manager1.DisableKvStoreAutoLaunch(storeInfo1.userId, storeInfo1.appId, storeInfo1.storeId), OK);
}
} // namespace DistributedDB