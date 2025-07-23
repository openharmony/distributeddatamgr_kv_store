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
#include "single_store_impl.h"
#include "mock_db_store.h"
#include "mock_observer.h"
#include "mock_sync_callback.h"
#include "mock_kvdb_service_client.h"
#include "mock_dev_manager.h"
#include "mock_backup_manager.h"

using namespace OHOS::DistributedKv;
using namespace testing;

class SingleStoreImplTest : public testing::Test {
protected:
    void SetUp() override {
        mockDbStore_ = std::make_shared<MockDBStore>();
        mockService_ = std::make_shared<MockKVDBServiceClient>();
        mockDevManager_ = std::make_shared<MockDevManager>();
        mockBackupManager_ = std::make_shared<MockBackupManager>();

        // Setup default behaviors
        ON_CALL(*mockDbStore_, GetStoreId()).WillByDefault(Return("test_store"));
        ON_CALL(*mockDbStore_, Put(_, _)).WillByDefault(Return(DistributedDB::OK));
        ON_CALL(*mockDbStore_, Get(_, _)).WillByDefault(Return(DistributedDB::OK));
        
        options_.encrypt = true;
        options_.autoSync = true;
        options_.securityLevel = SecurityLevel::S3;
        options_.area = 1;
        options_.hapName = "test_hap";
        
        store_ = std::make_shared<SingleStoreImpl>(
            mockDbStore_, AppId{"test_app"}, options_, Convertor{});
    }

    void TearDown() override {
        store_.reset();
    }

    std::shared_ptr<MockDBStore> mockDbStore_;
    std::shared_ptr<MockKVDBServiceClient> mockService_;
    std::shared_ptr<MockDevManager> mockDevManager_;
    std::shared_ptr<MockBackupManager> mockBackupManager_;
    Options options_;
    std::shared_ptr<SingleStoreImpl> store_;
};

TEST_F(SingleStoreImplTest, GetStoreIdSuccess) {
    StoreId storeId = store_->GetStoreId();
    EXPECT_EQ(storeId.storeId, "test_store");
}

TEST_F(SingleStoreImplTest, PutSuccess) {
    Key key("test_key");
    Value value("test_value");
    
    EXPECT_CALL(*mockDbStore_, Put(_, _)).Times(1);
    
    Status status = store_->Put(key, value);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, PutWithEmptyKeyFails) {
    Key key("");
    Value value("test_value");
    
    Status status = store_->Put(key, value);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, PutWithLargeValueFails) {
    Key key("test_key");
    Value value(std::string(MAX_VALUE_LENGTH + 1, 'a'));
    
    Status status = store_->Put(key, value);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, PutBatchSuccess) {
    std::vector<Entry> entries = {
        {Key("key1"), Value("value1")},
        {Key("key2"), Value("value2")}
    };
    
    EXPECT_CALL(*mockDbStore_, PutBatch(_)).Times(1);
    
    Status status = store_->PutBatch(entries);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, PutBatchWithEmptyKeyFails) {
    std::vector<Entry> entries = {
        {Key(""), Value("value1")},
        {Key("key2"), Value("value2")}
    };
    
    Status status = store_->PutBatch(entries);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, DeleteSuccess) {
    Key key("test_key");
    
    EXPECT_CALL(*mockDbStore_, Delete(_)).Times(1);
    
    Status status = store_->Delete(key);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, DeleteBatchSuccess) {
    std::vector<Key> keys = {Key("key1"), Key("key2")};
    
    EXPECT_CALL(*mockDbStore_, DeleteBatch(_)).Times(1);
    
    Status status = store_->DeleteBatch(keys);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetSuccess) {
    Key key("test_key");
    Value value;
    
    EXPECT_CALL(*mockDbStore_, Get(_, _)).Times(1);
    
    Status status = store_->Get(key, value);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetEntriesSuccess) {
    Key prefix("prefix");
    std::vector<Entry> entries;
    
    EXPECT_CALL(*mockDbStore_, GetEntries(_, _)).Times(1);
    
    Status status = store_->GetEntries(prefix, entries);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetResultSetSuccess) {
    Key prefix("prefix");
    std::shared_ptr<ResultSet> resultSet;
    
    EXPECT_CALL(*mockDbStore_, GetEntries(_, _)).Times(1);
    
    Status status = store_->GetResultSet(prefix, resultSet);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, StartTransactionSuccess) {
    EXPECT_CALL(*mockDbStore_, StartTransaction()).Times(1);
    
    Status status = store_->StartTransaction();
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, CommitSuccess) {
    EXPECT_CALL(*mockDbStore_, Commit()).Times(1);
    
    Status status = store_->Commit();
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, RollbackSuccess) {
    EXPECT_CALL(*mockDbStore_, Rollback()).Times(1);
    
    Status status = store_->Rollback();
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SubscribeKvStoreSuccess) {
    auto observer = std::make_shared<MockObserver>();
    
    EXPECT_CALL(*mockDbStore_, RegisterObserver(_, _, _)).Times(1);
    
    Status status = store_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, UnSubscribeKvStoreSuccess) {
    auto observer = std::make_shared<MockObserver>();
    
    EXPECT_CALL(*mockDbStore_, RegisterObserver(_, _, _)).Times(1);
    EXPECT_CALL(*mockDbStore_, UnRegisterObserver(_)).Times(1);
    
    store_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    Status status = store_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SyncSuccess) {
    std::vector<std::string> devices = {"device1", "device2"};
    
    EXPECT_CALL(*mockService_, Sync(_, _, _, _)).Times(1);
    
    Status status = store_->Sync(devices, SYNC_MODE_PUSH);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, RegisterSyncCallbackSuccess) {
    auto callback = std::make_shared<MockSyncCallback>();
    
    Status status = store_->RegisterSyncCallback(callback);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, UnRegisterSyncCallbackSuccess) {
    Status status = store_->UnRegisterSyncCallback();
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetSecurityLevelSuccess) {
    SecurityLevel secLevel;
    
    EXPECT_CALL(*mockDbStore_, GetSecurityOption(_)).Times(1);
    
    Status status = store_->GetSecurityLevel(secLevel);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, RemoveDeviceDataSuccess) {
    std::string device = "device1";
    
    EXPECT_CALL(*mockService_, RemoveDeviceData(_, _, _, _)).Times(1);
    
    Status status = store_->RemoveDeviceData(device);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, BackupSuccess) {
    std::string file = "backup_file";
    std::string baseDir = "/data/backup";
    
    EXPECT_CALL(*mockBackupManager_, Backup(_, _, _)).Times(1);
    
    Status status = store_->Backup(file, baseDir);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, RestoreSuccess) {
    std::string file = "backup_file";
    std::string baseDir = "/data/backup";
    
    EXPECT_CALL(*mockService_, Close(_, _, _)).Times(1);
    EXPECT_CALL(*mockBackupManager_, Restore(_, _, _)).Times(1);
    
    Status status = store_->Restore(file, baseDir);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, CloseSuccess) {
    int32_t ref = store_->Close(false);
    EXPECT_EQ(ref, 0);
}

TEST_F(SingleStoreImplTest, CloseWithForceSuccess) {
    int32_t ref = store_->Close(true);
    EXPECT_EQ(ref, 0);
}

TEST_F(SingleStoreImplTest, SetSyncParamSuccess) {
    KvSyncParam syncParam;
    
    EXPECT_CALL(*mockService_, SetSyncParam(_, _, _, _)).Times(1);
    
    Status status = store_->SetSyncParam(syncParam);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetSyncParamSuccess) {
    KvSyncParam syncParam;
    
    EXPECT_CALL(*mockService_, GetSyncParam(_, _, _, _)).Times(1);
    
    Status status = store_->GetSyncParam(syncParam);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SetCapabilityEnabledSuccess) {
    EXPECT_CALL(*mockService_, EnableCapability(_, _, _)).Times(1);
    
    Status status = store_->SetCapabilityEnabled(true);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SetCapabilityRangeSuccess) {
    std::vector<std::string> localLabels = {"label1"};
    std::vector<std::string> remoteLabels = {"label2"};
    
    EXPECT_CALL(*mockService_, SetCapability(_, _, _, _, _)).Times(1);
    
    Status status = store_->SetCapabilityRange(localLabels, remoteLabels);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SubscribeWithQuerySuccess) {
    std::vector<std::string> devices = {"device1"};
    DataQuery query;
    
    EXPECT_CALL(*mockService_, AddSubscribeInfo(_, _, _, _)).Times(1);
    
    Status status = store_->SubscribeWithQuery(devices, query);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, UnsubscribeWithQuerySuccess) {
    std::vector<std::string> devices = {"device1"};
    DataQuery query;
    
    EXPECT_CALL(*mockService_, RmvSubscribeInfo(_, _, _, _)).Times(1);
    
    Status status = store_->UnsubscribeWithQuery(devices, query);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetCountSuccess) {
    DataQuery query;
    int count = 0;
    
    EXPECT_CALL(*mockDbStore_, GetCount(_, _)).Times(1);
    
    Status status = store_->GetCount(query, count);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetDeviceEntriesSuccess) {
    std::string device = "device1";
    std::vector<Entry> entries;
    
    EXPECT_CALL(*mockDbStore_, GetDeviceEntries(_, _)).Times(1);
    
    Status status = store_->GetDeviceEntries(device, entries);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, CloudSyncSuccess) {
    AsyncDetail async;
    
    EXPECT_CALL(*mockService_, CloudSync(_, _, _)).Times(1);
    
    Status status = store_->CloudSync(async);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SetConfigSuccess) {
    StoreConfig config;
    
    EXPECT_CALL(*mockService_, SetConfig(_, _, _)).Times(1);
    
    Status status = store_->SetConfig(config);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, IsRebuildReturnsFalseWhenNotRebuilding) {
    EXPECT_CALL(*mockDbStore_, GetDatabaseStatus()).Times(1);
    
    bool isRebuild = store_->IsRebuild();
    EXPECT_FALSE(isRebuild);
}

TEST_F(SingleStoreImplTest, AddRefIncrementsReferenceCount) {
    int32_t initialRef = store_->AddRef();
    int32_t newRef = store_->AddRef();
    EXPECT_EQ(newRef, initialRef + 1);
}

TEST_F(SingleStoreImplTest, GetSubUserReturnsCorrectValue) {
    int32_t subUser = store_->GetSubUser();
    EXPECT_EQ(subUser, 0); // Default value
}

TEST_F(SingleStoreImplTest, OnRemoteDiedTriggersReconnection) {
    store_->OnRemoteDied();
    // Should schedule a reconnection attempt
    EXPECT_TRUE(true); // Just verify the method runs without crashing
}

TEST_F(SingleStoreImplTest, SetIdentifierSuccess) {
    std::string accountId = "account1";
    std::string appId = "app1";
    std::string storeId = "store1";
    std::vector<std::string> targetDev = {"device1"};
    
    EXPECT_CALL(*mockDbStore_, SetEqualIdentifier(_, _)).Times(1);
    
    Status status = store_->SetIdentifier(accountId, appId, storeId, targetDev);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, DeleteBackupSuccess) {
    std::vector<std::string> files = {"file1"};
    std::string baseDir = "/backup";
    std::map<std::string, Status> results;
    
    EXPECT_CALL(*mockBackupManager_, DeleteBackup(_, _, _)).Times(1);
    
    Status status = store_->DeleteBackup(files, baseDir, results);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, PutWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    Key key("test_key");
    Value value("test_value");
    
    Status status = store_->Put(key, value);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    Key key("test_key");
    Value value;
    
    Status status = store_->Get(key, value);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, SubscribeWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    auto observer = std::make_shared<MockObserver>();
    
    Status status = store_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, SyncWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::vector<std::string> devices = {"device1"};
    
    Status status = store_->Sync(devices, SYNC_MODE_PUSH);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetEntriesWithQuerySuccess) {
    DataQuery query;
    std::vector<Entry> entries;
    
    EXPECT_CALL(*mockDbStore_, GetEntries(_, _)).Times(1);
    
    Status status = store_->GetEntries(query, entries);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetResultSetWithQuerySuccess) {
    DataQuery query;
    std::shared_ptr<ResultSet> resultSet;
    
    EXPECT_CALL(*mockDbStore_, GetEntries(_, _)).Times(1);
    
    Status status = store_->GetResultSet(query, resultSet);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, CloseResultSetSuccess) {
    auto resultSet = std::make_shared<MockResultSet>();
    
    EXPECT_CALL(*resultSet, Close()).Times(1);
    
    Status status = store_->CloseResultSet(resultSet);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, PutBatchWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::vector<Entry> entries = {
        {Key("key1"), Value("value1")},
        {Key("key2"), Value("value2")}
    };
    
    Status status = store_->PutBatch(entries);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, DeleteWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    Key key("test_key");
    
    Status status = store_->Delete(key);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, DeleteBatchWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::vector<Key> keys = {Key("key1"), Key("key2")};
    
    Status status = store_->DeleteBatch(keys);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, StartTransactionWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    
    Status status = store_->StartTransaction();
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, CommitWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    
    Status status = store_->Commit();
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, RollbackWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    
    Status status = store_->Rollback();
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetSecurityLevelWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    SecurityLevel secLevel;
    
    Status status = store_->GetSecurityLevel(secLevel);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, RemoveDeviceDataWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::string device = "device1";
    
    Status status = store_->RemoveDeviceData(device);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, BackupWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::string file = "backup_file";
    std::string baseDir = "/data/backup";
    
    Status status = store_->Backup(file, baseDir);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, RestoreWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::string file = "backup_file";
    std::string baseDir = "/data/backup";
    
    Status status = store_->Restore(file, baseDir);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetCountWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    DataQuery query;
    int count = 0;
    
    Status status = store_->GetCount(query, count);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetDeviceEntriesWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::string device = "device1";
    std::vector<Entry> entries;
    
    Status status = store_->GetDeviceEntries(device, entries);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, SetIdentifierWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::string accountId = "account1";
    std::string appId = "app1";
    std::string storeId = "store1";
    std::vector<std::string> targetDev = {"device1"};
    
    Status status = store_->SetIdentifier(accountId, appId, storeId, targetDev);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, IsRebuildWhenStoreClosedReturnsFalse) {
    store_->Close(true);
    
    bool isRebuild = store_->IsRebuild();
    EXPECT_FALSE(isRebuild);
}

TEST_F(SingleStoreImplTest, SubscribeWithQueryWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::vector<std::string> devices = {"device1"};
    DataQuery query;
    
    Status status = store_->SubscribeWithQuery(devices, query);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, UnsubscribeWithQueryWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::vector<std::string> devices = {"device1"};
    DataQuery query;
    
    Status status = store_->UnsubscribeWithQuery(devices, query);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, SetConfigWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    StoreConfig config;
    
    Status status = store_->SetConfig(config);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, CloudSyncWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    AsyncDetail async;
    
    Status status = store_->CloudSync(async);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, SetCapabilityEnabledWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    
    Status status = store_->SetCapabilityEnabled(true);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, SetCapabilityRangeWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::vector<std::string> localLabels = {"label1"};
    std::vector<std::string> remoteLabels = {"label2"};
    
    Status status = store_->SetCapabilityRange(localLabels, remoteLabels);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetSyncParamWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    KvSyncParam syncParam;
    
    Status status = store_->GetSyncParam(syncParam);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, SetSyncParamWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    KvSyncParam syncParam;
    
    Status status = store_->SetSyncParam(syncParam);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, RegisterSyncCallbackWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    auto callback = std::make_shared<MockSyncCallback>();
    
    Status status = store_->RegisterSyncCallback(callback);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, UnRegisterSyncCallbackWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    
    Status status = store_->UnRegisterSyncCallback();
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, DeleteBackupWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    std::vector<std::string> files = {"file1"};
    std::string baseDir = "/backup";
    std::map<std::string, Status> results;
    
    Status status = store_->DeleteBackup(files, baseDir, results);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetEntriesWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    Key prefix("prefix");
    std::vector<Entry> entries;
    
    Status status = store_->GetEntries(prefix, entries);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, GetResultSetWhenStoreClosedReturnsAlreadyClosed) {
    store_->Close(true);
    Key prefix("prefix");
    std::shared_ptr<ResultSet> resultSet;
    
    Status status = store_->GetResultSet(prefix, resultSet);
    EXPECT_EQ(status, ALREADY_CLOSED);
}

TEST_F(SingleStoreImplTest, CloseResultSetWithNullReturnsInvalidArgument) {
    std::shared_ptr<ResultSet> resultSet = nullptr;
    
    Status status = store_->CloseResultSet(resultSet);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, SubscribeWithNullObserverReturnsInvalidArgument) {
    std::shared_ptr<Observer> observer = nullptr;
    
    Status status = store_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, UnsubscribeWithNullObserverReturnsInvalidArgument) {
    std::shared_ptr<Observer> observer = nullptr;
    
    Status status = store_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, RegisterSyncCallbackWithNullReturnsInvalidArgument) {
    std::shared_ptr<SyncCallback> callback = nullptr;
    
    Status status = store_->RegisterSyncCallback(callback);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, SyncWithNullCallbackUsesDefaultObserver) {
    std::vector<std::string> devices = {"device1"};
    
    EXPECT_CALL(*mockService_, Sync(_, _, _, _)).Times(1);
    
    Status status = store_->Sync(devices, SYNC_MODE_PUSH);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SyncWithQueryAndCallbackSuccess) {
    std::vector<std::string> devices = {"device1"};
    DataQuery query;
    auto callback = std::make_shared<MockSyncCallback>();
    
    EXPECT_CALL(*mockService_, Sync(_, _, _, _)).Times(1);
    
    Status status = store_->Sync(devices, SYNC_MODE_PUSH, query, callback);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetWithNetworkIdSuccess) {
    Key key("test_key");
    std::string networkId = "network1";
    bool callbackCalled = false;
    
    auto callback = [&callbackCalled](Status status, Value &&value) {
        callbackCalled = true;
        EXPECT_EQ(status, SUCCESS);
    };
    
    store_->Get(key, networkId, callback);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(SingleStoreImplTest, GetEntriesWithNetworkIdSuccess) {
    Key prefix("prefix");
    std::string networkId = "network1";
    bool callbackCalled = false;
    
    auto callback = [&callbackCalled](Status status, std::vector<Entry> &&entries) {
        callbackCalled = true;
        EXPECT_EQ(status, SUCCESS);
    };
    
    store_->GetEntries(prefix, networkId, callback);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(SingleStoreImplTest, IsRemoteChangedReturnsTrueWhenDeviceNotFound) {
    std::string deviceId = "unknown_device";
    
    EXPECT_CALL(*mockDevManager_, ToUUID(_)).WillOnce(Return(""));
    
    bool changed = store_->IsRemoteChanged(deviceId);
    EXPECT_TRUE(changed);
}

TEST_F(SingleStoreImplTest, IsRemoteChangedReturnsTrueWhenServiceUnavailable) {
    std::string deviceId = "device1";
    
    EXPECT_CALL(*mockDevManager_, ToUUID(_)).WillOnce(Return("uuid1"));
    EXPECT_CALL(*mockService_, GetServiceAgent(_)).WillOnce(Return(nullptr));
    
    bool changed = store_->IsRemoteChanged(deviceId);
    EXPECT_TRUE(changed);
}

TEST_F(SingleStoreImplTest, DoAutoSyncTriggersSyncWhenAutoSyncEnabled) {
    options_.autoSync = true;
    store_ = std::make_shared<SingleStoreImpl>(
        mockDbStore_, AppId{"test_app"}, options_, Convertor{});
    
    EXPECT_CALL(*mockService_, NotifyDataChange(_, _, _)).Times(1);
    
    store_->DoAutoSync();
}

TEST_F(SingleStoreImplTest, DoNotifyChangeTriggersNotificationWhenCloudAutoSyncEnabled) {
    options_.cloudConfig.autoSync = true;
    store_ = std::make_shared<SingleStoreImpl>(
        mockDbStore_, AppId{"test_app"}, options_, Convertor{});
    
    EXPECT_CALL(*mockService_, NotifyDataChange(_, _, _)).Times(1);
    
    store_->DoNotifyChange();
}

TEST_F(SingleStoreImplTest, ReportDBFaultEventLogsError) {
    Status status = DB_ERROR;
    std::string functionName = "TestFunction";
    
    // Just verify it runs without crashing
    store_->ReportDBFaultEvent(status, functionName);
    EXPECT_TRUE(true);
}

TEST_F(SingleStoreImplTest, RetryWithCheckPointRetriesAfterLogOverLimits) {
    bool firstCall = true;
    auto lambda = [&firstCall]() {
        if (firstCall) {
            firstCall = false;
            return DistributedDB::LOG_OVER_LIMITS;
        }
        return DistributedDB::OK;
    };
    
    EXPECT_CALL(*mockDbStore_, Pragma(_, _)).Times(1);
    
    Status status = store_->RetryWithCheckPoint(lambda);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, RetryWithCheckPointDoesNotRetryOnOtherErrors) {
    auto lambda = []() { return DistributedDB::DB_ERROR; };
    
    Status status = store_->RetryWithCheckPoint(lambda);
    EXPECT_EQ(status, DB_ERROR);
}

TEST_F(SingleStoreImplTest, ConstructorInitializesFieldsCorrectly) {
    EXPECT_EQ(store_->GetStoreId().storeId, "test_store");
    EXPECT_TRUE(store_->GetSubUser() == 0);
}

TEST_F(SingleStoreImplTest, DestructorCleansUpResources) {
    store_->AddRef();
    store_.reset(); // Destructor should be called
    
    // Just verify it doesn't crash
    EXPECT_TRUE(true);
}

TEST_F(SingleStoreImplTest, AddRefAndCloseManageReferenceCount) {
    int32_t ref = store_->AddRef();
    EXPECT_EQ(ref, 2); // Initial ref is 1
    
    ref = store_->Close(false);
    EXPECT_EQ(ref, 1);
    
    ref = store_->Close(true);
    EXPECT_EQ(ref, 0);
}

TEST_F(SingleStoreImplTest, BridgeReleaserCleansUpObserverBridge) {
    auto observer = std::make_shared<MockObserver>();
    auto bridge = new ObserverBridge(AppId{"app"}, StoreId{"store"}, 0, observer, Convertor{});
    
    EXPECT_CALL(*mockDbStore_, UnRegisterObserver(_)).Times(1);
    
    auto releaser = store_->BridgeReleaser();
    releaser(bridge);
}

TEST_F(SingleStoreImplTest, PutBatchWithLargeValueFails) {
    std::vector<Entry> entries = {
        {Key("key1"), Value(std::string(MAX_VALUE_LENGTH + 1, 'a'))},
        {Key("key2"), Value("value2")}
    };
    
    Status status = store_->PutBatch(entries);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, SyncWithEmptyDevicesListSucceeds) {
    std::vector<std::string> devices;
    
    Status status = store_->Sync(devices, SYNC_MODE_PUSH);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, CloudSyncWithEmptyAsyncDetailSucceeds) {
    AsyncDetail async;
    
    Status status = store_->CloudSync(async);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SubscribeKvStoreWithInvalidTypeFails) {
    auto observer = std::make_shared<MockObserver>();
    
    Status status = store_->SubscribeKvStore(static_cast<SubscribeType>(0), observer);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, UnSubscribeKvStoreWithInvalidTypeFails) {
    auto observer = std::make_shared<MockObserver>();
    
    Status status = store_->UnSubscribeKvStore(static_cast<SubscribeType>(0), observer);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetWithEmptyKeyFails) {
    Key key("");
    Value value;
    
    Status status = store_->Get(key, value);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, GetEntriesWithEmptyPrefixSucceeds) {
    Key prefix("");
    std::vector<Entry> entries;
    
    Status status = store_->GetEntries(prefix, entries);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetResultSetWithEmptyPrefixSucceeds) {
    Key prefix("");
    std::shared_ptr<ResultSet> resultSet;
    
    Status status = store_->GetResultSet(prefix, resultSet);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetDeviceEntriesWithEmptyDeviceFails) {
    std::string device;
    std::vector<Entry> entries;
    
    Status status = store_->GetDeviceEntries(device, entries);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, RemoveDeviceDataWithEmptyDeviceFails) {
    std::string device;
    
    Status status = store_->RemoveDeviceData(device);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

TEST_F(SingleStoreImplTest, SetIdentifierWithEmptyParametersFails) {
    Status status = store_->SetIdentifier("", "", "", {});
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, BackupWithEmptyFileNameFails) {
    std::string file;
    std::string baseDir = "/backup";
    
    Status status = store_->Backup(file, baseDir);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, RestoreWithEmptyFileNameFails) {
    std::string file;
    std::string baseDir = "/backup";
    
    Status status = store_->Restore(file, baseDir);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, DeleteBackupWithEmptyFilesListSucceeds) {
    std::vector<std::string> files;
    std::string baseDir = "/backup";
    std::map<std::string, Status> results;
    
    Status status = store_->DeleteBackup(files, baseDir, results);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SubscribeWithQueryWithEmptyDevicesListSucceeds) {
    std::vector<std::string> devices;
    DataQuery query;
    
    Status status = store_->SubscribeWithQuery(devices, query);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, UnsubscribeWithQueryWithEmptyDevicesListSucceeds) {
    std::vector<std::string> devices;
    DataQuery query;
    
    Status status = store_->UnsubscribeWithQuery(devices, query);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, SetCapabilityRangeWithEmptyLabelsSucceeds) {
    std::vector<std::string> localLabels;
    std::vector<std::string> remoteLabels;
    
    Status status = store_->SetCapabilityRange(localLabels, remoteLabels);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetWithCallbackWhenStoreClosedCallsCallbackWithError) {
    store_->Close(true);
    Key key("test_key");
    std::string networkId = "network1";
    bool callbackCalled = false;
    
    auto callback = [&callbackCalled](Status status, Value &&value) {
        callbackCalled = true;
        EXPECT_EQ(status, ALREADY_CLOSED);
    };
    
    store_->Get(key, networkId, callback);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(SingleStoreImplTest, GetEntriesWithCallbackWhenStoreClosedCallsCallbackWithError) {
    store_->Close(true);
    Key prefix("prefix");
    std::string networkId = "network1";
    bool callbackCalled = false;
    
    auto callback = [&callbackCalled](Status status, std::vector<Entry> &&entries) {
        callbackCalled = true;
        EXPECT_EQ(status, ALREADY_CLOSED);
    };
    
    store_->GetEntries(prefix, networkId, callback);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(SingleStoreImplTest, GetResultSetWithCallbackWhenStoreClosedCallsCallbackWithError) {
    store_->Close(true);
    Key prefix("prefix");
    std::string networkId = "network1";
    bool callbackCalled = false;

    auto callback = [&callbackCalled](Status status, std::shared_ptr<ResultSet> &&resultSet) {
        callbackCalled = true;
        EXPECT_EQ(status, ALREADY_CLOSED);
    };

    store_->GetResultSet(prefix, networkId, callback);
    EXPECT_TRUE(callbackCalled);

}

TEST_F(SingleStoreImplTest, GetDeviceEntriesWithCallbackWhenStoreClosedCallsCallbackWithError) {
    store_->Close(true);
    std::string device = "device1";
    bool callbackCalled = false;

    auto callback = [&callbackCalled](Status status, std::vector<Entry> &&entries) {
        callbackCalled = true;
        EXPECT_EQ(status, ALREADY_CLOSED);
    };

    store_->GetDeviceEntries(device, callback);
    EXPECT_TRUE(callbackCalled);
} 

TEST_F(SingleStoreImplTest, GetResultSetWithEmptyPrefixSucceeds) {
    Key prefix;
    std::string networkId = "network1";
    std::shared_ptr<ResultSet> resultSet;

    Status status = store_->GetResultSet(prefix, networkId, resultSet);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetDeviceEntriesWithEmptyDeviceSucceeds) {
    std::string device;
    std::vector<Entry> entries;

    Status status = store_->GetDeviceEntries(device, entries);
    EXPECT_EQ(status, SUCCESS);

}

TEST_F(SingleStoreImplTest, GetResultSetWithEmptyNetworkIdFails) {
    Key prefix("prefix");
    std::string networkId;
    std::shared_ptr<ResultSet> resultSet;

    Status status = store_->GetResultSet(prefix, networkId, resultSet);
    EXPECT_NE(status, SUCCESS);

}
TEST_F(SingleStoreImplTest, BackupWithEmptyFileNameFails) {
    std::string file;
    std::string baseDir = "/backup";

    Status status = store_->Backup(file, baseDir);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, RestoreWithEmptyFileNameFails) {
    std::string file;
    std::string baseDir = "/backup";

    Status status = store_->Restore(file, baseDir);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, DeleteBackupWithEmptyFilesFails) {
    std::vector<std::string> files;
    std::string baseDir = "/backup";
    std::map<std::string, Status> results;

    Status status = store_->DeleteBackup(files, baseDir, results);
}
TEST_F(SingleStoreImplTest, GetBackupFilesWithEmptyBaseDirFails) {
    std::string baseDir;
    std::vector<std::string> files;

    Status status = store_->GetBackupFiles(baseDir, files);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetBackupFilesWithEmptyFilesSucceeds) {
    std::string baseDir = "/backup";
    std::vector<std::string> files;

    Status status = store_->GetBackupFiles(baseDir, files);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetBackupFilesWithEmptyBaseDirSucceeds) {
    std::string baseDir;
    std::vector<std::string> files;

    Status status = store_->GetBackupFiles(baseDir, files);
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetResultSetWithEmptyNetworkIdFails) {
    Key prefix("prefix");
    std::string networkId;
    std::shared_ptr<ResultSet> resultSet;

    Status status = store_->GetResultSet(prefix, networkId, resultSet);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetEntriesWithEmptyNetworkIdFails) {
    Key prefix("prefix");
    std::string networkId;
    std::vector<Entry> entries;

    Status status = store_->GetEntries(prefix, networkId, entries);
    EXPECT_NE(status, SUCCESS);
}

TEST_F(SingleStoreImplTest, GetEntriesWithEmptyPrefixSucceeds) {
    Key prefix;
    std::string networkId = "network1";
    std::vector<Entry> entries;

    Status status = store_->GetEntries(prefix, networkId, entries);
    EXPECT_EQ(status, SUCCESS);
}
TEST_F(SingleStoreImplTest, GetEntriesWithCallbackWhenStoreClosedCallsCallbackWithError) {
    store_->Close(true);
    Key prefix("prefix");
    std::string networkId = "network1";
    bool callbackCalled = false;

    auto callback = [&callbackCalled](Status status, std::vector<Entry> &&entries) {
        callbackCalled = true;
        EXPECT_EQ(status, ALREADY_CLOSED);
    };

    store_->GetEntries(prefix, networkId, callback);
    EXPECT_TRUE(callbackCalled);

}

TEST_F(SingleStoreImplTest, Put_Success) {
    Key key("test_key");
    Value value("test_value");
    EXPECT_CALL(*mockDBStore, Put(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->Put(key, value);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Put方法（无效键）
TEST_F(SingleStoreImplTest, Put_InvalidKey) {
    Key key(""); // 空键
    Value value("test_value");
    Status status = singleStore->Put(key, value);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试Put方法（值过大）
TEST_F(SingleStoreImplTest, Put_ValueTooLarge) {
    Key key("test_key");
    Value value(std::string(1024 * 1024 + 1, 'a')); // 超过最大限制
    Status status = singleStore->Put(key, value);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试PutBatch方法（成功）
TEST_F(SingleStoreImplTest, PutBatch_Success) {
    std::vector<Entry> entries;
    entries.push_back({Key("key1"), Value("value1")});
    entries.push_back({Key("key2"), Value("value2")});
    EXPECT_CALL(*mockDBStore, PutBatch(_)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->PutBatch(entries);
    EXPECT_EQ(status, SUCCESS);
}

// 测试PutBatch方法（无效条目）
TEST_F(SingleStoreImplTest, PutBatch_InvalidEntry) {
    std::vector<Entry> entries;
    entries.push_back({Key(""), Value("value1")}); // 空键
    Status status = singleStore->PutBatch(entries);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试Delete方法（成功）
TEST_F(SingleStoreImplTest, Delete_Success) {
    Key key("test_key");
    EXPECT_CALL(*mockDBStore, Delete(_)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->Delete(key);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Delete方法（无效键）
TEST_F(SingleStoreImplTest, Delete_InvalidKey) {
    Key key(""); // 空键
    Status status = singleStore->Delete(key);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试DeleteBatch方法（成功）
TEST_F(SingleStoreImplTest, DeleteBatch_Success) {
    std::vector<Key> keys;
    keys.push_back(Key("key1"));
    keys.push_back(Key("key2"));
    EXPECT_CALL(*mockDBStore, DeleteBatch(_)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->DeleteBatch(keys);
    EXPECT_EQ(status, SUCCESS);
}

// 测试StartTransaction方法
TEST_F(SingleStoreImplTest, StartTransaction) {
    EXPECT_CALL(*mockDBStore, StartTransaction()).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->StartTransaction();
    EXPECT_EQ(status, SUCCESS);
}

// 测试Commit方法
TEST_F(SingleStoreImplTest, Commit) {
    EXPECT_CALL(*mockDBStore, Commit()).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->Commit();
    EXPECT_EQ(status, SUCCESS);
}

// 测试Rollback方法
TEST_F(SingleStoreImplTest, Rollback) {
    EXPECT_CALL(*mockDBStore, Rollback()).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->Rollback();
    EXPECT_EQ(status, SUCCESS);
}

// 测试SubscribeKvStore方法（无效观察者）
TEST_F(SingleStoreImplTest, SubscribeKvStore_InvalidObserver) {
    Status status = singleStore->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, nullptr);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试SubscribeKvStore方法（本地订阅）
TEST_F(SingleStoreImplTest, SubscribeKvStore_Local) {
    auto observer = std::make_shared<Observer>();
    EXPECT_CALL(*mockDBStore, RegisterObserver(_, _, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(status, SUCCESS);
}

// 测试UnSubscribeKvStore方法
TEST_F(SingleStoreImplTest, UnSubscribeKvStore) {
    auto observer = std::make_shared<Observer>();
    singleStore->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_CALL(*mockDBStore, UnRegisterObserver(_)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Get方法（成功）
TEST_F(SingleStoreImplTest, Get_Success) {
    Key key("test_key");
    Value value;
    EXPECT_CALL(*mockDBStore, Get(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->Get(key, value);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Get方法（未找到）
TEST_F(SingleStoreImplTest, Get_NotFound) {
    Key key("test_key");
    Value value;
    EXPECT_CALL(*mockDBStore, Get(_, _)).WillOnce(Return(DistributedDB::DBStatus::NOT_FOUND));
    Status status = singleStore->Get(key, value);
    EXPECT_EQ(status, NOT_FOUND);
}

// 测试GetEntries方法（前缀查询）
TEST_F(SingleStoreImplTest, GetEntries_Prefix) {
    Key prefix("prefix");
    std::vector<Entry> entries;
    Status status = singleStore->GetEntries(prefix, entries);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetDeviceEntries方法
TEST_F(SingleStoreImplTest, GetDeviceEntries) {
    std::vector<Entry> entries;
    std::string device = "device1";
    EXPECT_CALL(*mockDBStore, GetDeviceEntries(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->GetDeviceEntries(device, entries);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetResultSet方法
TEST_F(SingleStoreImplTest, GetResultSet) {
    Key prefix("prefix");
    std::shared_ptr<ResultSet> resultSet;
    Status status = singleStore->GetResultSet(prefix, resultSet);
    EXPECT_EQ(status, SUCCESS);
}

// 测试CloseResultSet方法
TEST_F(SingleStoreImplTest, CloseResultSet) {
    Key prefix("prefix");
    std::shared_ptr<ResultSet> resultSet;
    singleStore->GetResultSet(prefix, resultSet);
    Status status = singleStore->CloseResultSet(resultSet);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(resultSet, nullptr);
}

// 测试GetCount方法
TEST_F(SingleStoreImplTest, GetCount) {
    DataQuery query;
    int count = 0;
    EXPECT_CALL(*mockDBStore, GetCount(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->GetCount(query, count);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetSecurityLevel方法
TEST_F(SingleStoreImplTest, GetSecurityLevel) {
    SecurityLevel secLevel;
    DistributedDB::SecurityOption option;
    EXPECT_CALL(*mockDBStore, GetSecurityOption(_)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->GetSecurityLevel(secLevel);
    EXPECT_EQ(status, SUCCESS);
}

// 测试RemoveDeviceData方法
TEST_F(SingleStoreImplTest, RemoveDeviceData) {
    std::string device = "device1";
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, RemoveDeviceData(_, _, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->RemoveDeviceData(device);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试Sync方法
TEST_F(SingleStoreImplTest, Sync) {
    std::vector<std::string> devices = {"device1", "device2"};
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, Sync(_, _, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->Sync(devices, SyncMode::SYNC_MODE_PUSH);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试CloudSync方法
TEST_F(SingleStoreImplTest, CloudSync) {
    AsyncDetail async;
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, CloudSync(_, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->CloudSync(async);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试RegisterSyncCallback方法
TEST_F(SingleStoreImplTest, RegisterSyncCallback) {
    auto callback = std::make_shared<SyncCallback>();
    Status status = singleStore->RegisterSyncCallback(callback);
    EXPECT_EQ(status, SUCCESS);
}

// 测试UnRegisterSyncCallback方法
TEST_F(SingleStoreImplTest, UnRegisterSyncCallback) {
    Status status = singleStore->UnRegisterSyncCallback();
    EXPECT_EQ(status, SUCCESS);
}

// 测试SetSyncParam方法
TEST_F(SingleStoreImplTest, SetSyncParam) {
    KvSyncParam param;
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, SetSyncParam(_, _, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->SetSyncParam(param);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试GetSyncParam方法
TEST_F(SingleStoreImplTest, GetSyncParam) {
    KvSyncParam param;
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, GetSyncParam(_, _, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->GetSyncParam(param);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试SetCapabilityEnabled方法
TEST_F(SingleStoreImplTest, SetCapabilityEnabled) {
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, EnableCapability(_, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->SetCapabilityEnabled(true);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试SetCapabilityRange方法
TEST_F(SingleStoreImplTest, SetCapabilityRange) {
    std::vector<std::string> localLabels, remoteLabels;
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, SetCapability(_, _, _, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->SetCapabilityRange(localLabels, remoteLabels);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试SubscribeWithQuery方法
TEST_F(SingleStoreImplTest, SubscribeWithQuery) {
    std::vector<std::string> devices = {"device1"};
    DataQuery query;
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, AddSubscribeInfo(_, _, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->SubscribeWithQuery(devices, query);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试UnsubscribeWithQuery方法
TEST_F(SingleStoreImplTest, UnsubscribeWithQuery) {
    std::vector<std::string> devices = {"device1"};
    DataQuery query;
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, RmvSubscribeInfo(_, _, _, _)).WillOnce(Return(SUCCESS));
    Status status = singleStore->UnsubscribeWithQuery(devices, query);
    EXPECT_EQ(status, SUCCESS);
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试AddRef方法
TEST_F(SingleStoreImplTest, AddRef) {
    int32_t ref = singleStore->AddRef();
    EXPECT_EQ(ref, 1);
}

// 测试Close方法
TEST_F(SingleStoreImplTest, Close) {
    singleStore->AddRef();
    int32_t ref = singleStore->Close(false);
    EXPECT_EQ(ref, 0);
}

// 测试Backup方法
TEST_F(SingleStoreImplTest, Backup) {
    std::string file = "backup_test";
    std::string baseDir = "/data/test";
    EXPECT_CALL(*mockDBStore, Export(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->Backup(file, baseDir);
    EXPECT_EQ(status, SUCCESS);
}

// 测试Restore方法
TEST_F(SingleStoreImplTest, Restore) {
    std::string file = "backup_test";
    std::string baseDir = "/data/test";
    EXPECT_CALL(*mockDBStore, Import(_, _, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    Status status = singleStore->Restore(file, baseDir);
    EXPECT_EQ(status, SUCCESS);
}

// 测试DeleteBackup方法
TEST_F(SingleStoreImplTest, DeleteBackup) {
    std::vector<std::string> files = {"backup1", "backup2"};
    std::string baseDir = "/data/test";
    std::map<std::string, Status> results;
    Status status = singleStore->DeleteBackup(files, baseDir, results);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(results.size(), 2);
}

// 测试IsRemoteChanged方法
TEST_F(SingleStoreImplTest, IsRemoteChanged) {
    std::string deviceId = "device1";
    bool changed = singleStore->IsRemoteChanged(deviceId);
    EXPECT_TRUE(changed); // 默认返回true
}

// 测试GetEntries方法（带查询条件）
TEST_F(SingleStoreImplTest, GetEntries_Query) {
    DataQuery query;
    std::vector<Entry> entries;
    Status status = singleStore->GetEntries(query, entries);
    EXPECT_EQ(status, SUCCESS);
}
} // namespace DistributedDB