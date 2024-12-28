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

#define LOG_TAG "KvManagerTest"

#include "kv_mana.h"

#include <gtest/gtest.h>
#include <ipc_skeleton.h>

#include "executor_pool.h"
#include "kv_store_nb_delegate_mock.h"
#include "kv_types.h"
#include "snapshot/machine_status.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using AssetVal = OHOS::CommonType::AssetVal;
using RestoreSta = OHOS::DistributedKv::KvStoreManager::RestoreSta;
namespace OHOS::Test {

class KvManagerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

protected:
    Asset asset_;
    std::string uri;
    std::string appId = "kvManagerTest_appid";
    uint64_t seque = 10;
    uint64_t seque2 = 20;
    uint64_t seque3 = 30;
    std::string userId = "100";
    std::string bundleName_ = "com.examples.hmos.notepad";
    OHOS::KvStore::AssetBindInfo assetBindInfo;
    pidt pid = 10;
    uint32_t tokenId = 100;
    std::string sessionId = "123123";
    std::vector<uint8_t> dataMap;
    std::string deviceId_ = "70010054583239";
    AssetVal assetVal;
};

void KvManagerTest::SetUp()
{
    uri = "file:://com.examples.hmos.notepad/dataMap/storage/el2/distributedfiles/dir/asset1.jpg";
    AssetVal assetValue{
        .id = "test_name",
        .name = uri,
        .uri = uri,
        .hash = "modifyTime_size",
        .path = "/dataMap/storage/el2",
        .createTime = "2024.07.23",
        .modifyTime = "modifyTime",
        .size = "size",
    };
    Asset asset{
        .name = "test_name",
        .uri = uri,
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
    };
    asset_ = asset;

    assetVal = assetValue;

    dataMap.push_back(10); // 10 is for testing
    dataMap.push_back(20); // 20 is for testing
    dataMap.push_back(30); // 30 is for testing

    OHOS::KvStore::AssetBindInfo AssetBindInfo{
        .storeName = "store_test",
        .tableName = "table_test",
        .field = "attachment",
        .assetName = "asset1.jpg",
    };
    assetBindInfo = AssetBindInfo;
}

void KvManagerTest::TearDown() {}

/**
* @tc.name: DeleteNotifier001
* @tc.desc: DeleteNotifier test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, DeleteNotifier001Virtual, TestSize.Level0)
{
    auto syncManagerTest = SequenceSyncManager::GetInstance();
    auto status = syncManagerTest->DeleteNotifier(seque, userId);
    EXPECT_EQ(status, SequenceSyncManager::ERR_SID_NOT_EXIST);
}

/**
* @tc.name: Process001
* @tc.desc: Process test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, Process001Virtual, TestSize.Level0)
{
    auto syncManagerTest = SequenceSyncManager::GetInstance();
    std::map<std::string, DistributedDB::DBStatus> resTest;
    resTest = {{ "test_kv", DistributedDB::DBStatus::OK }};

    std::function<void(const std::map<std::string, int32_t> &resTest)> fun;
    fun = [](const std::map<std::string, int32_t> &resTest) {
        return resTest;
    };
    auto status = syncManagerTest->Process(seque, resTest, userId);
    EXPECT_EQ(status, SequenceSyncManager::ERR_SID_NOT_EXIST);
    syncManagerTest->seqIdCallbackRelations_.emplace(seque, fun);
    status = syncManagerTest->Process(seque, resTest, userId);
    EXPECT_EQ(status, SequenceSyncManager::SUCCESS_USER_HAS_FINISHED);
}

/**
* @tc.name: DeleteNotifierNoLock001
* @tc.desc: DeleteNotifierNoLock test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, DeleteNotifierNoLock001Virtual, TestSize.Level0)
{
    auto syncManagerTest = SequenceSyncManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &resTest)> fun;
    fun = [](const std::map<std::string, int32_t> &resTest) {
        return resTest;
    };
    syncManagerTest->seqIdCallbackRelations_.emplace(seque, fun);
    std::vector<uint64_t> seqIds = {seque, seque2, seque3};
    std::string userId = "user_1";
    auto status = syncManagerTest->DeleteNotifierNoLock(seque, userId);
    EXPECT_EQ(status, SequenceSyncManager::SUCCESS_USER_HAS_FINISHED);
    syncManagerTest->userIdSeqIdRelations_[userId] = seqIds;
    status = syncManagerTest->DeleteNotifierNoLock(seque, userId);
    EXPECT_EQ(status, SequenceSyncManager::SUCCESS_USER_IN_USE);
}

/**
* @tc.name: Clear001
* @tc.desc: Clear test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, Clear001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    auto status = mana->Clear();
    EXPECT_EQ(status, OHOS::DistributedKv::OBJECT_STORE_NOT_FOUND);
}

/**
* @tc.name: registerAndUnregisterRemoteCallback001
* @tc.desc: test RegisterRemoteCallback and UnregisterRemoteCallback.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, registerAndUnregisterRemoteCallback001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    sptr<IRemoteKv> callback;
    mana->RegisterRemoteCallback(bundleName_, sessionId, pid, tokenId, callback);
    KvStoreManager::CallbackInfo callbackInfo = mana->callbacks_.Find(tokenId).second;
    std::string prefix = bundleName_ + sessionId;
    ASSERT_NE(callbackInfo.observers_.find(prefix), callbackInfo.observers_.end());
    mana->UnregisterRemoteCallback(bundleName_, pid, tokenId, sessionId);
    callbackInfo = mana->callbacks_.Find(tokenId).second;
    EXPECT_EQ(callbackInfo.observers_.find(prefix), callbackInfo.observers_.end());
}

/**
* @tc.name: registerAndUnregisterRemoteCallback002
* @tc.desc: abnormal use cases.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, registerAndUnregisterRemoteCallback002Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    sptr<IRemoteKv> callback;
    uint32_t tokenId = 101;
    mana->RegisterRemoteCallback("", sessionId, pid, tokenId, callback);
    mana->RegisterRemoteCallback(bundleName_, "", pid, tokenId, callback);
    mana->RegisterRemoteCallback("", "", pid, tokenId, callback);
    EXPECT_EQ(mana->callbacks_.Find(tokenId).first, false);
    mana->UnregisterRemoteCallback("", pid, tokenId, sessionId);
}

/**
* @tc.name: NotifyDataChanged001
* @tc.desc: NotifyDataChanged test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, NotifyDataChanged001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::string bundle1_ = "com.examples.ophm.notepad";
    std::string kvKey = bundle1_ + sessionId;
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> dataMap;
    std::map<std::string, std::vector<uint8_t>> dataMap1;
    std::vector<uint8_t> data1_;
    data1_.push_back(RestoreSta::DATA_READY);
    data1_.push_back(RestoreSta::ASSETS_READY);
    data1_.push_back(RestoreSta::ALL_READY);
    dataMap1 = {{ "kvKey", data1_ }};
    dataMap = {{ kvKey, dataMap1 }};
    std::shared_ptr<ExecutorPool> exeTest = std::make_shared<ExecutorPool>(5, 3); // executorTest pool
    mana->SetThreadPool(exeTest);
    EXPECT_EQ(mana->restore.Find(kvKey).first, false);
    mana->NotifyDataChanged(dataMap, {});
    EXPECT_EQ(mana->restore.Find(kvKey).second, RestoreSta::DATA_READY);
}

/**
* @tc.name: NotifyAssetsReady001
* @tc.desc: NotifyAssetsReady test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, NotifyAssetsReady001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::string kvKey = bundleName_ + sessionId;
    std::string srcNetwork = "1";
    EXPECT_EQ(mana->restore.Find(kvKey).first, false);
    mana->NotifyAssetsReady(kvKey, srcNetwork);
    EXPECT_EQ(mana->restore.Find(kvKey).second, RestoreSta::ASSETS_READY);
    mana->restore.Clear();
    mana->restore.Insert(kvKey, RestoreSta::DATA_READY);
    mana->NotifyAssetsReady(kvKey, srcNetwork);
    EXPECT_EQ(mana->restore.Find(kvKey).second, RestoreSta::ALL_READY);
}

/**
 * @tc.name: NotifyAssetsReady002
 * @tc.desc: NotifyAssetsReady test.
 * @tc.type: FUNC
 */
HWTEST_F(KvManagerTest, NotifyAssetsReady002Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::string kvKey="com.example.myapplicaiton123456";
    std::string srcNetwork = "654321";

    mana->restore.Clear();
    mana->NotifyAssetsStart(kvKey, srcNetwork);
    auto [has00, value00] = mana->restore.Find(kvKey);
    ASSERT_TRUE(has00);
    EXPECT_EQ(value00, RestoreSta::NONE);

    mana->restore.Clear();
    mana->NotifyAssetsReady(kvKey, srcNetwork);
    auto [has1, value1] = mana->restore.Find(kvKey);
    ASSERT_TRUE(has1);
    EXPECT_EQ(value1, RestoreSta::ASSETS_READY);

    mana->restore.Clear();
    mana->restore.Insert(kvKey, RestoreSta::DATA_NOTIFIED);
    mana->NotifyAssetsReady(kvKey, srcNetwork);
    auto [has2, value2] = mana->restore.Find(kvKey);
    ASSERT_TRUE(has2);
    EXPECT_EQ(value2, RestoreSta::ALL_READY);

    mana->restore.Clear();
    mana->restore.Insert(kvKey, RestoreSta::DATA_READY);
    mana->NotifyAssetsReady(kvKey, srcNetwork);
    auto [has3, value3] = mana->restore.Find(kvKey);
    ASSERT_TRUE(has3);
    EXPECT_EQ(value3, RestoreSta::ALL_READY);
    mana->restore.Clear();
}

/**
* @tc.name: NotifyChange001
* @tc.desc: NotifyChange test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, NotifyChange001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> dataMap;
    std::map<std::string, std::vector<uint8_t>> dataMap1;
    std::vector<uint8_t> data1_;
    data1_.push_back(RestoreSta::DATA_READY);
    dataMap.push_back(RestoreSta::ALL_READY);
    dataMap = {{ "test_kv", dataMap }};
    dataMap1 = {{ "p_###SAVEINFO###001", data1_ }};
    mana->NotifyChange(dataMap1);
    mana->NotifyChange(dataMap);
}

/**
 * @tc.name: NotifyChange002
 * @tc.desc: NotifyChange test.
 * @tc.type: FUNC
 */
HWTEST_F(KvManagerTest, NotifyChange002Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::shared_ptr<ExecutorPool> executorTest = std::make_shared<ExecutorPool>(1, 0);
    mana->SetThreadPool(executorTest);
    std::map<std::string, std::vector<uint8_t>> dataMap{};
    std::vector<uint8_t> value{0};
    std::string bundleName = "com.example.application";
    std::string sessionId = "123456";
    std::string source = "source";
    std::string target = "target";
    std::string timestamp = "1234567890";
    KvStoreManager::SaveInfo saveInfo(bundleName, sessionId, source, target, timestamp);
    std::string saveInfoStr = DistributedData::Serializable::Marshall(saveInfo);
    auto saveInfoValue = std::vector<uint8_t>(saveInfoStr.begin(), saveInfoStr.end());
    std::string prefix = saveInfo.ToPropertyPrefix();
    std::string assetPrefix = prefix + "p_asset";
    dataMap.insert_or_assign(prefix + "p_###SAVEINFO###", saveInfoValue);
    dataMap.insert_or_assign(prefix + "p_data", value);
    dataMap.insert_or_assign(assetPrefix + KvStore::NAME_SUFFIX, value);
    dataMap.insert_or_assign(assetPrefix + KvStore::URI_SUFFIX, value);
    dataMap.insert_or_assign(assetPrefix + KvStore::PATH_SUFFIX, value);
    dataMap.insert_or_assign(assetPrefix + KvStore::CREATE_TIME_SUFFIX, value);
    dataMap.insert_or_assign(assetPrefix + KvStore::MODIFY_TIME_SUFFIX, value);
    dataMap.insert_or_assign(assetPrefix + KvStore::SIZE_SUFFIX, value);
    dataMap.insert_or_assign("testkey", value);
    mana->NotifyChange(dataMap);
    ASSERT_TRUE(mana->restore.Contains(bundleName+sessionId));
    auto [has, taskId] = mana->kvTimer_.Find(bundleName+sessionId);
    ASSERT_TRUE(has);
    mana->restore.Clear();
    mana->exeTest->Remove(taskId);
    mana->kvTimer_.Clear();
}

/**
 * @tc.name: ComputeStatus001
 * @tc.desc: ComputeStatus.test
 * @tc.type: FUNC
 */
HWTEST_F(KvManagerTest, ComputeStatus001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::shared_ptr<ExecutorPool> executorTest = std::make_shared<ExecutorPool>(1, 0);
    mana->SetThreadPool(executorTest);
    std::string kvKey="com.example.myapplicaiton123456";
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> dataMap{};
    mana->restore.Clear();
    mana->ComputeStatus(kvKey, {}, dataMap);
    auto [has00, value00] = mana->restore.Find(kvKey);
    ASSERT_TRUE(has00);
    EXPECT_EQ(value00, RestoreSta::DATA_READY);
    auto [has1, taskId1] = mana->kvTimer_.Find(kvKey);
    ASSERT_TRUE(has1);
    mana->exeTest->Remove(taskId1);
    mana->kvTimer_.Clear();
    mana->restore.Clear();

    mana->restore.Insert(kvKey, RestoreSta::ASSETS_READY);
    mana->ComputeStatus(kvKey, {}, dataMap);
    auto [has2, value2] = mana->restore.Find(kvKey);
    ASSERT_TRUE(has2);
    EXPECT_EQ(value2, RestoreSta::ALL_READY);
    auto [has3, taskId3] = mana->kvTimer_.Find(kvKey);
    EXPECT_FALSE(has3);
    mana->restore.Clear();
}

/**
* @tc.name: Open001
* @tc.desc: Open test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, Open001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    mana->kvStoreDelegateManager_ = nullptr;
    auto status = mana->Open();
    EXPECT_EQ(status, DistributedKv::OBJECT_INNER_ERROR);
    std::string dataDir = "/dataMap/app/el2/100/database";
    mana->SetData(dataDir, userId);
    mana->delegate = nullptr;
    status = mana->Open();
    EXPECT_EQ(status, DistributedKv::OBJECT_SUCCESS);
    mana->delegate = mana->OpenKvKvStore();
    status = mana->Open();
    EXPECT_EQ(status, DistributedKv::OBJECT_SUCCESS);
}

/**
* @tc.name: OnAssetChanged001
* @tc.desc: OnAssetChanged test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, OnAssetChanged001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::shared_ptr<Snapshot> snapshot = std::make_shared<KvSnapshot>();
    auto snapKey = appId + "_" + sessionId;
    auto status = mana->OnAssetChanged(tokenId, appId, sessionId, deviceId_, assetVal);
    EXPECT_EQ(status, DistributedKv::OBJECT_INNER_ERROR);
    mana->snapshot.Insert(snapKey, snapshot);
    status = mana->OnAssetChanged(tokenId, appId, sessionId, deviceId_, assetVal);
    EXPECT_EQ(status, DistributedKv::OBJECT_SUCCESS);
}

/**
* @tc.name: DeleteSnapshot001
* @tc.desc: DeleteSnapshot test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, DeleteSnapshot001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::shared_ptr<Snapshot> snapshot = std::make_shared<KvSnapshot>();
    auto snapKey = bundleName_ + "_" + sessionId;
    auto snapshots = mana->snapshot.Find(snapKey).second;
    EXPECT_EQ(snapshots, nullptr);
    mana->DeleteSnapshot(bundleName_, sessionId);

    mana->snapshot.Insert(snapKey, snapshot);
    snapshots = mana->snapshot.Find(snapKey).second;
    ASSERT_NE(snapshots, nullptr);
    mana->DeleteSnapshot(bundleName_, sessionId);
}

/**
* @tc.name: OpenKvKvStore001
* @tc.desc: OpenKvKvStore test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, OpenKvKvStore001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    mana->kvDataListener_ = nullptr;
    EXPECT_EQ(mana->kvDataListener_, nullptr);
    mana->OpenKvKvStore();
    ASSERT_NE(mana->kvDataListener_, nullptr);
    mana->OpenKvKvStore();
}

/**
* @tc.name: FlushClosedStore001
* @tc.desc: FlushClosedStore test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, FlushClosedStore001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    mana->isSyncing_ = true;
    mana->syncCount = 10; // test syncCount
    mana->delegate = nullptr;
    mana->FlushClosedStore();
    mana->isSyncing_ = false;
    mana->FlushClosedStore();
    mana->syncCount = 0; // test syncCount
    mana->FlushClosedStore();
    mana->delegate = mana->OpenKvKvStore();
    mana->FlushClosedStore();
}

/**
* @tc.name: Close001
* @tc.desc: Close test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, Close001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    mana->syncCount = 1; // test syncCount
    mana->Close();
    EXPECT_EQ(mana->syncCount, 1); // 1 is for testing
    mana->delegate = mana->OpenKvKvStore();
    mana->Close();
    EXPECT_EQ(mana->syncCount, 0); // 0 is for testing
}

/**
* @tc.name: SyncOnStore001
* @tc.desc: SyncOnStore test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, SyncOnStore001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &resTest)> fun;
    fun = [](const std::map<std::string, int32_t> &resTest) {
        return resTest;
    };
    std::string prefix = "KvManagerTest";
    std::vector<std::string> deviceList;
    deviceList.push_back("local");
    deviceList.push_back("local1");
    auto status = mana->SyncOnStore(prefix, deviceList, fun);
    EXPECT_EQ(status, OBJECT_SUCCESS);
}

/**
* @tc.name: RevokeSaveToStore001
* @tc.desc: RetrieveFromStore test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, RevokeSaveToStore001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    DistributedDB::KvStoreNbDelegateMock mockDelegate;
    mana->delegate = &mockDelegate;
    std::vector<uint8_t> id;
    id.push_back(1);  // for testing
    id.push_back(2);  // for testing
    std::map<std::string, std::vector<uint8_t>> resTest;
    resTest = {{ "test_kv", id }};
    auto status = mana->RetrieveFromStore(appId, sessionId, resTest);
    EXPECT_EQ(status, OBJECT_SUCCESS);
}

/**
* @tc.name: SyncCompleted001
* @tc.desc: SyncCompleted test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(KvManagerTest, SyncCompleted001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    auto syncManagerTest = SequenceSyncManager::GetInstance();
    std::map<std::string, DistributedDB::DBStatus> resTest;
    resTest = {{ "test_kv", DistributedDB::DBStatus::OK }};
    std::function<void(const std::map<std::string, int32_t> &resTest)> fun;
    fun = [](const std::map<std::string, int32_t> &resTest) {
        return resTest;
    };
    mana->userId = "99";
    std::vector<uint64_t> userId;
    userId.push_back(99);
    userId.push_back(100);
    mana->SyncCompleted(resTest, seque);
    syncManagerTest->userIdSeqIdRelations_ = {{ "test_kv", userId }};
    mana->SyncCompleted(resTest, seque);
    userId.clear();
    syncManagerTest->seqIdCallbackRelations_.emplace(seque, fun);
    mana->SyncCompleted(resTest, seque);
    userId.push_back(99);
    userId.push_back(100);
    mana->SyncCompleted(resTest, seque);
}

/**
* @tc.name: SplitEntryKey001
* @tc.desc: SplitEntryKey test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, SplitEntryKey001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::string key1 = "";
    std::string key2 = "KvManagerTest";
    auto status = mana->SplitEntryKey(key1);
    EXPECT_EQ(status.empty(), true);
    status = mana->SplitEntryKey(key2);
    EXPECT_EQ(status.empty(), true);
}

/**
* @tc.name: SplitEntryKey002
* @tc.desc: SplitEntryKey test.
* @tc.type: FUNC
*/
HWTEST_F(KvManagerTest, SplitEntryKey002Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::string key1 = "com.example.myapplication_sessionIdsource_target_1234567890_p_propertyName";
    auto resTest = mana->SplitEntryKey(key1);
    EXPECT_EQ(resTest[0], "com.example.myapplication");
    EXPECT_EQ(resTest[1], "sessionId");
    EXPECT_EQ(resTest[2], "source");
    EXPECT_EQ(resTest[3], "target");
    EXPECT_EQ(resTest[4], "1234567890");
    EXPECT_EQ(resTest[5], "p_propertyName");

    std::string key2 = "com.example.myapplication_sessionIdsource_target_000_p_propertyName";
    resTest = mana->SplitEntryKey(key2);
    ASSERT_TRUE(resTest.empty());

    std::string key3 = "com.example.myapplicationsessionIdsourcetarget_123456_p_propertyName";
    resTest = mana->SplitEntryKey(key3);
    ASSERT_TRUE(resTest.empty());

    std::string key4 = "com.example.myapplicationsessionIdsource_target_12345890_p_propertyName";
    resTest = mana->SplitEntryKey(key4);
    ASSERT_TRUE(resTest.empty());

    std::string key5 = "com.example.myapplicationsessionIdsource_target_34567890_p_propertyName";
    resTest = mana->SplitEntryKey(key5);
    ASSERT_TRUE(resTest.empty());
}

/**
* @tc.name: ProcessOldEntry001
* @tc.desc: ProcessOldEntry test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, ProcessOldEntry001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    mana->delegate = mana->OpenKvKvStore();
    std::vector<DistributedDB::Entry> entries;
    auto status = mana->delegate->GetEntries(std::vector<uint8_t>(appId.begin(), appId.end()), entries);
    EXPECT_EQ(status, DistributedDB::DBStatus::NOT_FOUND);
    mana->ProcessOldEntry(appId);

    DistributedDB::KvStoreNbDelegateMock mockDelegate;
    mana->delegate = &mockDelegate;
    status = mana->delegate->GetEntries(std::vector<uint8_t>(appId.begin(), appId.end()), entries);
    EXPECT_EQ(status, DistributedDB::DBStatus::OK);
    mana->ProcessOldEntry(appId);
}

/**
* @tc.name: ProcessSyncCallback001
* @tc.desc: ProcessSyncCallback test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, ProcessSyncCallback001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::map<std::string, int32_t> resTest;
    mana->ProcessSyncCallback(resTest, appId, sessionId, deviceId_);
    resTest.insert({"local", 1}); // for testing
    EXPECT_EQ(resTest.empty(), false);
    ASSERT_NE(resTest.find("local"), resTest.end());
    mana->ProcessSyncCallback(resTest, appId, sessionId, deviceId_);
}

/**
* @tc.name: IsAssetComplete001
* @tc.desc: IsAssetComplete test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, IsAssetComplete001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> resTest;
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    std::string assetPrefix = "IsAssetComplete_test";
    resTest.insert({assetPrefix, completes});
    auto status = mana->IsAssetComplete(resTest, assetPrefix);
    EXPECT_EQ(status, false);
    resTest.insert({assetPrefix + KvStore::NAME_SUFFIX, completes});
    status = mana->IsAssetComplete(resTest, assetPrefix);
    EXPECT_EQ(status, false);
    resTest.insert({assetPrefix + KvStore::URI_SUFFIX, completes});
    status = mana->IsAssetComplete(resTest, assetPrefix);
    EXPECT_EQ(status, false);
    resTest.insert({assetPrefix + KvStore::PATH_SUFFIX, completes});
    status = mana->IsAssetComplete(resTest, assetPrefix);
    EXPECT_EQ(status, false);
    resTest.insert({assetPrefix + KvStore::CREATE_TIME_SUFFIX, completes});
    status = mana->IsAssetComplete(resTest, assetPrefix);
    EXPECT_EQ(status, false);
    resTest.insert({assetPrefix + KvStore::MODIFY_TIME_SUFFIX, completes});
    status = mana->IsAssetComplete(resTest, assetPrefix);
    EXPECT_EQ(status, false);
    resTest.insert({assetPrefix + KvStore::SIZE_SUFFIX, completes});
    status = mana->IsAssetComplete(resTest, assetPrefix);
    EXPECT_EQ(status, true);
}

/**
* @tc.name: GetAssetsFromDBRecords001
* @tc.desc: GetAssetsFromDBRecords test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvManagerTest, GetAssetsFromDBRecords001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> resTest;
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    std::string assetPrefix = "IsAssetComplete_test";
    resTest.insert({assetPrefix, completes});
    resTest.insert({assetPrefix + KvStore::NAME_SUFFIX, completes});
    resTest.insert({assetPrefix + KvStore::URI_SUFFIX, completes});
    resTest.insert({assetPrefix + KvStore::MODIFY_TIME_SUFFIX, completes});
    resTest.insert({assetPrefix + KvStore::SIZE_SUFFIX, completes});
    auto status = mana->GetAssetsFromDBRecords(resTest);
    EXPECT_EQ(status.empty(), false);
}

/**
* @tc.name: GetAssetsFromDBRecords002
* @tc.desc: GetAssetsFromDBRecords test.
* @tc.type: FUNC
*/
HWTEST_F(KvManagerTest, GetAssetsFromDBRecords002Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> status;

    std::vector<uint8_t> value00{0};
    std::string data0 = "[STRING]test";
    value00.insert(value00.end(), data0.begin(), data0.end());

    std::vector<uint8_t> value1{0};
    std::string dataMap1 = "(string)test";
    value1.insert(value1.end(), dataMap1.begin(), dataMap1.end());

    std::string prefix = "bundleName_sessionIdsource_target_timestamp";
    std::string dataKey = prefix + "_p_data";
    std::string assetPrefix0 = prefix + "_p_asset0";
    std::string assetPrefix1 = prefix + "_p_asset1";

    status.insert({dataKey, value00});
    auto assets = mana->GetAssetsFromDBRecords(status);
    ASSERT_TRUE(assets.empty());

    status.clear();
    status.insert({assetPrefix0 + KvStore::URI_SUFFIX, value00});
    assets = mana->GetAssetsFromDBRecords(status);
    ASSERT_TRUE(assets.empty());

    status.clear();
    status.insert({assetPrefix1 + KvStore::NAME_SUFFIX, value1});
    assets = mana->GetAssetsFromDBRecords(status);
    ASSERT_TRUE(assets.empty());

    status.clear();
    status.insert({assetPrefix0 + KvStore::NAME_SUFFIX, value00});
    status.insert({assetPrefix0 + KvStore::URI_SUFFIX, value00});
    status.insert({assetPrefix0 + KvStore::MODIFY_TIME_SUFFIX, value00});
    status.insert({assetPrefix0 + KvStore::SIZE_SUFFIX, value00});
    assets = mana->GetAssetsFromDBRecords(status);
    EXPECT_EQ(assets.size(), 1);
    EXPECT_EQ(assets[0].name, "test");
    EXPECT_EQ(assets[0].uri, "test");
    EXPECT_EQ(assets[0].modifyTime, "test");
    EXPECT_EQ(assets[0].size, "test");
    EXPECT_EQ(assets[0].hash, "test_test");

    assets = mana->GetAssetsFromDBRecords(status);
    EXPECT_EQ(assets.size(), 1);
    EXPECT_EQ(assets[0].name, "(string)test");
    EXPECT_EQ(assets[0].uri, "(string)test");
    EXPECT_EQ(assets[0].modifyTime, "(string)test");
    EXPECT_EQ(assets[0].size, "(string)test");
    EXPECT_EQ(assets[0].hash, "(string)test_(string)test");

    status.clear();
    status.insert({assetPrefix1 + KvStore::NAME_SUFFIX, value1});
    status.insert({assetPrefix1 + KvStore::URI_SUFFIX, value1});
    status.insert({assetPrefix1 + KvStore::MODIFY_TIME_SUFFIX, value1});
    status.insert({assetPrefix1 + KvStore::SIZE_SUFFIX, value1});
}

/**
* @tc.name: RegisterAssetsLister001
* @tc.desc: RegisterAssetsLister test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, RegisterAssetsLister001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    mana->kvAssetsSendListener_ = nullptr;
    mana->kvAssetsRecvListener_ = nullptr;
    auto status = mana->RegisterAssetsLister();
    EXPECT_EQ(status, true);
    mana->kvAssetsSendListener_ = new KvAssetsSendListener();
    mana->kvAssetsRecvListener_ = new KvAssetsRecvListener();;
    status = mana->RegisterAssetsLister();
    EXPECT_EQ(status, true);
}

/**
* @tc.name: RegisterAssetsLister001
* @tc.desc: PushAssets test.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(KvManagerTest, PushAssets001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> dataMap;
    std::string assetPrefix = "PushAssets_test";
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    dataMap.insert({assetPrefix, completes});
    auto status = mana->PushAssets(appId, appId, sessionId, dataMap, deviceId_);
    EXPECT_EQ(status, DistributedKv::OBJECT_SUCCESS);
}

/**
* @tc.name: AddNotifier001
* @tc.desc: AddNotifie and DeleteNotifier test.
* @tc.type: FUNC
*/
HWTEST_F(KvManagerTest, AddNotifier001Virtual, TestSize.Level0)
{
    auto syncManagerTest = SequenceSyncManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &resTest)> fun;
    fun = [](const std::map<std::string, int32_t> &resTest) {
        return resTest;
    };
    auto seque = syncManagerTest->AddNotifier(userId, fun);
    auto status = syncManagerTest->DeleteNotifier(seque, userId);
    EXPECT_EQ(status, SequenceSyncManager::SUCCESS_USER_HAS_FINISHED);
}

/**
* @tc.name: AddNotifier002
* @tc.desc: AddNotifie and DeleteNotifier test.
* @tc.type: FUNC
*/
HWTEST_F(KvManagerTest, AddNotifier002Virtual, TestSize.Level0)
{
    auto syncManagerTest = SequenceSyncManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &resTest)> fun;
    fun = [](const std::map<std::string, int32_t> &resTest) {
        return resTest;
    };
    auto sequeId = syncManagerTest->AddNotifier(userId, fun);
    ASSERT_NE(sequeId, seque);
    auto status = syncManagerTest->DeleteNotifier(seque, userId);
    EXPECT_EQ(status, SequenceSyncManager::ERR_SID_NOT_EXIST);
}

/**
* @tc.name: BindAsset 001
* @tc.desc: BindAsset test.
* @tc.type: FUNC
*/
HWTEST_F(KvManagerTest, BindAsset001Virtual, TestSize.Level0)
{
    auto mana = KvStoreManager::GetInstance();
    std::string bundleName = "BindAsset";
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    auto status = mana->BindAsset(tokenId, bundleName, sessionId, assetVal, assetBindInfo);
    EXPECT_EQ(status, DistributedKv::OBJECT_DBSTATUS_ERROR);
}
} // namespace OHOS::Test
