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

#include "nbdelegate_fuzzer.h"
#include <list>
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "distributeddb_tools_test.h"
#include "fuzzer_data.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_observer.h"
#include "platform_specific.h"
#include "runtime_context.h"
#include "query.h"
#include "virtual_cloud_db.h"

class KvStoreNbDelegateCURDFuzzer {
    /* Keep C++ file names the same as the class name. */
};

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
static constexpr const int HUNDRED = 100;
static constexpr const int MOD = 1024;
static constexpr const int PASSWDLEN = 20;
static constexpr const int MAX_PAGE_SIZE = 32;
static constexpr const int MAX_CACHE_SIZE = 2048;
static constexpr const int MAX_LEVEL = 2;
static constexpr const int MIN_SYNC_MODE = static_cast<int>(SyncMode::SYNC_MODE_PUSH_ONLY);
static constexpr const int MAX_SYNC_MODE = static_cast<int>(SyncMode::SYNC_MODE_CLOUD_FORCE_PULL) + 1;
static constexpr const uint32_t MIN_LOCK_ACTION = static_cast<uint32_t>(LockAction::NONE);
static constexpr const uint32_t MAX_LOCK_ACTION = static_cast<uint32_t>(LockAction::DOWNLOAD) + 1;
static constexpr const int MIN_CIPHER_TYPE = static_cast<int>(CipherType::DEFAULT);
static constexpr const int MAX_CIPHER_TYPE = static_cast<int>(CipherType::AES_256_GCM) + 1;
static constexpr const uint32_t MIN_INDEX_TYPE = static_cast<uint32_t>(IndexType::BTREE);
static constexpr const uint32_t MAX_INDEX_TYPE = static_cast<uint32_t>(IndexType::HASH) + 1;
class KvStoreObserverFuzzTest : public DistributedDB::KvStoreObserver {
public:
    KvStoreObserverFuzzTest();
    ~KvStoreObserverFuzzTest() = default;
    KvStoreObserverFuzzTest(const KvStoreObserverFuzzTest &) = delete;
    KvStoreObserverFuzzTest& operator=(const KvStoreObserverFuzzTest &) = delete;
    KvStoreObserverFuzzTest(KvStoreObserverFuzzTest &&) = delete;
    KvStoreObserverFuzzTest& operator=(KvStoreObserverFuzzTest &&) = delete;
    // callback function will be called when the db data is changed.
    void OnChange(const DistributedDB::KvStoreChangedData &);

    // reset the callCount_ to zero.
    void ResetToZero();
    // get callback results.
    unsigned long GetCallCount() const;
    const std::list<DistributedDB::Entry> &GetEntriesInserted() const;
    const std::list<DistributedDB::Entry> &GetEntriesUpdated() const;
    const std::list<DistributedDB::Entry> &GetEntriesDeleted() const;
    bool IsCleared() const;

private:
    unsigned long callCount_ = 0;
    bool isCleared_ = false;
    std::list<DistributedDB::Entry> inserted_ {};
    std::list<DistributedDB::Entry> updated_ {};
    std::list<DistributedDB::Entry> deleted_ {};
};

KvStoreObserverFuzzTest::KvStoreObserverFuzzTest()
{
    callCount_ = 0;
}

void KvStoreObserverFuzzTest::OnChange(const KvStoreChangedData &data)
{
    callCount_++;
    inserted_ = data.GetEntriesInserted();
    updated_ = data.GetEntriesUpdated();
    deleted_ = data.GetEntriesDeleted();
    isCleared_ = data.IsCleared();
}

void KvStoreObserverFuzzTest::ResetToZero()
{
    callCount_ = 0;
    isCleared_ = false;
    inserted_.clear();
    updated_.clear();
    deleted_.clear();
}

unsigned long KvStoreObserverFuzzTest::GetCallCount() const
{
    return callCount_;
}

const std::list<Entry> &KvStoreObserverFuzzTest::GetEntriesInserted() const
{
    return inserted_;
}

const std::list<Entry> &KvStoreObserverFuzzTest::GetEntriesUpdated() const
{
    return updated_;
}
const std::list<Entry> &KvStoreObserverFuzzTest::GetEntriesDeleted() const
{
    return deleted_;
}

bool KvStoreObserverFuzzTest::IsCleared() const
{
    return isCleared_;
}

std::vector<Entry> CreateEntries(FuzzedDataProvider &fdp, std::vector<Key> &keys)
{
    std::vector<Entry> entries;
    // key'length is less than 1024.
    auto count = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = fdp.ConsumeBytes<uint8_t>(i);
        keys.emplace_back(entry.key);
        entry.value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, HUNDRED));
        entries.emplace_back(entry);
    }
    return entries;
}

void FuzzSetInterceptorTest(KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    kvNbDelegatePtr->SetPushDataInterceptor(
        [](InterceptedData &data, const std::string &sourceID, const std::string &targetID) {
            int errCode = OK;
            auto entries = data.GetEntries();
            for (size_t i = 0; i < entries.size(); i++) {
                if (entries[i].key.empty() || entries[i].key.at(0) != 'A') {
                    continue;
                }
                auto newKey = entries[i].key;
                newKey[0] = 'B';
                errCode = data.ModifyKey(i, newKey);
                if (errCode != OK) {
                    break;
                }
            }
            return errCode;
        }
    );
    kvNbDelegatePtr->SetReceiveDataInterceptor(
        [](InterceptedData &data, const std::string &sourceID, const std::string &targetID) {
            int errCode = OK;
            auto entries = data.GetEntries();
            for (size_t i = 0; i < entries.size(); i++) {
                Key newKey;
                errCode = data.ModifyKey(i, newKey);
                if (errCode != OK) {
                    return errCode;
                }
            }
            return errCode;
        }
    );
}

void SetRemotePushFinishedNotifyFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    int pushFinishedFlag = 0;
    kvNbDelegatePtr->SetRemotePushFinishedNotify(
        [&](const RemotePushNotifyInfo &info) {
            pushFinishedFlag = fdp.ConsumeIntegral<int>();
    });
}

void SetEqualIdentifierFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    std::vector<std::string> targets;
    std::string target = fdp.ConsumeRandomLengthString();
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        targets.emplace_back(target);
    }
    std::string identifier = fdp.ConsumeRandomLengthString();
    kvNbDelegatePtr->SetEqualIdentifier(identifier, targets);
}

void SyncCloudFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    std::vector<std::string> devices;
    std::string device = fdp.ConsumeRandomLengthString();
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        devices.emplace_back(device);
    }
    SyncMode mode = static_cast<SyncMode>(fdp.ConsumeIntegralInRange<int>(MIN_SYNC_MODE, MAX_SYNC_MODE));
    Query query = Query::Select();
    int64_t waitTime = fdp.ConsumeIntegral<int64_t>();
    bool priorityTask = fdp.ConsumeBool();
    int32_t priorityLevel = fdp.ConsumeIntegralInRange<int>(0, MAX_LEVEL);
    bool compensatedSyncOnly = fdp.ConsumeBool();
    std::vector<std::string> users;
    std::string user = fdp.ConsumeRandomLengthString();
    for (int i = 0; i < size; i++) {
        users.emplace_back(user);
    }
    bool merge = fdp.ConsumeBool();
    LockAction lockAction =
        static_cast<LockAction>(fdp.ConsumeIntegralInRange<uint32_t>(MIN_LOCK_ACTION, MAX_LOCK_ACTION));
    std::string prepareTraceId = fdp.ConsumeRandomLengthString();
    bool asyncDownloadAssets = fdp.ConsumeBool();
    CloudSyncOption option = {
        .devices = devices,
        .mode = mode,
        .query = query,
        .waitTime = waitTime,
        .priorityTask = priorityTask,
        .priorityLevel = priorityLevel,
        .compensatedSyncOnly = compensatedSyncOnly,
        .users = users,
        .merge = merge,
        .lockAction = lockAction,
        .prepareTraceId = prepareTraceId,
        .asyncDownloadAssets = asyncDownloadAssets,
    };
    const std::function<void(DBStatus)> checkFinish;
    std::condition_variable callbackCv;
    SyncProcessCallback syncCallback = [checkFinish, &callbackCv](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item : process) {
            if (item.second.process == DistributedDB::FINISHED) {
                if (checkFinish) {
                    checkFinish(item.second.errCode);
                }
                callbackCv.notify_one();
            }
        }
    };
    kvNbDelegatePtr->Sync(option, syncCallback);
}

void SyncDeviceFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    std::vector<std::string> devices;
    std::string device = fdp.ConsumeRandomLengthString();
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        devices.emplace_back(device);
    }
    SyncMode mode = static_cast<SyncMode>(fdp.ConsumeIntegralInRange<int>(MIN_SYNC_MODE, MAX_SYNC_MODE));
    bool isSync = false;
    auto syncCallback = [&](const std::map<std::string, DistributedDB::DBStatus> &devicesMap) {
        if (devicesMap.find(device) != devicesMap.end()) {
            isSync = devicesMap.at(device) == DBStatus::OK;
        }
    };
    bool wait = fdp.ConsumeBool();
    kvNbDelegatePtr->Sync(devices, mode, syncCallback, wait);
    Query query = Query::Select();
    kvNbDelegatePtr->Sync(devices, mode, syncCallback, query, wait);
    DeviceSyncOption option = {
        .devices = devices,
        .mode = mode,
        .query = query,
        .isQuery = fdp.ConsumeBool(),
        .isWait = fdp.ConsumeBool(),
        .isRetry = fdp.ConsumeBool(),
    };
    std::mutex cancelMtx;
    std::condition_variable cancelCv;
    bool cancelFinished = false;
    DeviceSyncProcessCallback onProcess = [&](const std::map<std::string, DeviceSyncProcess> &processMap) {
        bool isAllCancel = true;
        for (auto &process : processMap) {
            if (process.second.errCode != COMM_FAILURE) {
                isAllCancel = false;
            }
        }
        if (isAllCancel) {
            std::unique_lock<std::mutex> lock(cancelMtx);
            cancelFinished = true;
            cancelCv.notify_all();
        }
    };
    kvNbDelegatePtr->Sync(option, onProcess);
    kvNbDelegatePtr->Sync(option, syncCallback);
    uint32_t syncId = fdp.ConsumeIntegral<uint32_t>();
    kvNbDelegatePtr->CancelSync(syncId);
}

void GetCloudVersionFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    std::string version = fdp.ConsumeRandomLengthString();
    kvNbDelegatePtr->SetGenCloudVersionCallback([&](const std::string &origin) {
        return origin + version;
    });
    std::string device = fdp.ConsumeRandomLengthString();
    kvNbDelegatePtr->GetCloudVersion(device);
}

void SetCloudSyncConfigFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    CloudSyncConfig config = {
        .maxUploadCount = fdp.ConsumeIntegralInRange<int32_t>(
            CloudDbConstant::MIN_UPLOAD_BATCH_COUNT, CloudDbConstant::MAX_UPLOAD_BATCH_COUNT),
        .maxUploadSize =
            fdp.ConsumeIntegralInRange<int32_t>(CloudDbConstant::MIN_UPLOAD_SIZE, CloudDbConstant::MAX_UPLOAD_SIZE),
        .maxRetryConflictTimes = fdp.ConsumeIntegralInRange<int32_t>(CloudDbConstant::MIN_RETRY_CONFLICT_COUNTS, MOD),
        .isSupportEncrypt = fdp.ConsumeBool(),
    };
    kvNbDelegatePtr->SetCloudSyncConfig(config);
}

void SetCloudDBFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDBs;
    std::shared_ptr<ICloudDb> cloudDb = std::make_shared<VirtualCloudDb>();
    std::string userId = "user" + std::to_string(fdp.ConsumeIntegralInRange<int>(0, MOD));
    cloudDBs[userId] = cloudDb;
    kvNbDelegatePtr->SetCloudDB(cloudDBs);
}

void SetCloudDBSchemaFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    std::map<std::string, DataBaseSchema> schema;
    DataBaseSchema dataBaseSchema;
    std::vector<Field> cloudField;
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        Field field = {
            .colName = fdp.ConsumeRandomLengthString(),
            .type = fdp.ConsumeIntegralInRange<int>(TYPE_INDEX<int64_t>, TYPE_INDEX<Assets>),
            .primary = fdp.ConsumeBool(),
            .nullable = fdp.ConsumeBool(),
        };
        cloudField.emplace_back(field);
    }
    TableSchema tableSchema = {
        .name = fdp.ConsumeRandomLengthString(),
        .sharedTableName = fdp.ConsumeRandomLengthString(),
        .fields = cloudField,
    };
    dataBaseSchema.tables.emplace_back(tableSchema);
    std::string userId = fdp.ConsumeRandomLengthString();
    schema[userId] = dataBaseSchema;
    kvNbDelegatePtr->SetCloudDbSchema(schema);
}

void RemoteQueryFuzz(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    std::vector<std::string> devices;
    std::string device = fdp.ConsumeRandomLengthString();
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        devices.emplace_back(device);
    }
    bool isSync = false;
    auto subscribeCallback = [&](const std::map<std::string, DistributedDB::DBStatus> &devicesMap) {
        if (devicesMap.find(device) != devicesMap.end()) {
            isSync = devicesMap.at(device) == DBStatus::OK;
        }
    };
    Query query = Query::Select();
    bool wait = fdp.ConsumeBool();
    kvNbDelegatePtr->SubscribeRemoteQuery(devices, subscribeCallback, query, wait);
    kvNbDelegatePtr->UnSubscribeRemoteQuery(devices, subscribeCallback, query, wait);
}

void TestCRUD(const Key &key, const Value &value, KvStoreNbDelegate *kvNbDelegatePtr, FuzzedDataProvider &fdp)
{
    Value valueRead;
    kvNbDelegatePtr->PutLocal(key, value);
    kvNbDelegatePtr->GetLocal(key, valueRead);
    kvNbDelegatePtr->DeleteLocal(key);
    kvNbDelegatePtr->Put(key, value);
    kvNbDelegatePtr->Put(key, value);
    kvNbDelegatePtr->UpdateKey([](const Key &origin, Key &newKey) {
        newKey = origin;
        newKey.emplace_back('0');
    });
    std::vector<Entry> vect;
    kvNbDelegatePtr->GetEntries(key, vect);
    std::vector<Entry> vectQuery;
    Query query = Query::Select();
    kvNbDelegatePtr->GetEntries(query, vectQuery);
    KvStoreResultSet *results = nullptr;
    kvNbDelegatePtr->GetEntries(query, results);
    vect.clear();
    vectQuery.clear();
    int count = fdp.ConsumeIntegral<int>();
    kvNbDelegatePtr->GetCount(query, count);
    kvNbDelegatePtr->GetLocalEntries(key, vect);
    std::vector<Key> vectKeys;
    kvNbDelegatePtr->GetKeys(key, vectKeys);
    kvNbDelegatePtr->Delete(key);
    kvNbDelegatePtr->Get(key, valueRead);
}

void GetDeviceEntriesTest(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    const int lenMod = 30; // 30 is mod for string vector size
    std::string device = fdp.ConsumeRandomLengthString(fdp.ConsumeIntegralInRange<size_t>(0, lenMod));
    kvNbDelegatePtr->GetWatermarkInfo(device);
    kvNbDelegatePtr->GetSyncDataSize(device);
    std::vector<Entry> vect;
    kvNbDelegatePtr->GetDeviceEntries(device, vect);
}

void RemoveDeviceDataByMode(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    auto mode = static_cast<ClearMode>(fdp.ConsumeIntegral<uint32_t>());
    LOGI("[RemoveDeviceDataByMode] select mode %d", static_cast<int>(mode));
    if (mode == DEFAULT) {
        return;
    }
    const int lenMod = 30; // 30 is mod for string vector size
    std::string device = fdp.ConsumeRandomLengthString(fdp.ConsumeIntegralInRange<size_t>(0, lenMod));
    std::string user = fdp.ConsumeRandomLengthString(fdp.ConsumeIntegralInRange<size_t>(0, lenMod));
    kvNbDelegatePtr->RemoveDeviceData();
    kvNbDelegatePtr->RemoveDeviceData(device, mode);
    kvNbDelegatePtr->RemoveDeviceData(device, user, mode);
}

void FuzzCURD(FuzzedDataProvider &fdp, KvStoreNbDelegate *kvNbDelegatePtr)
{
    std::shared_ptr<KvStoreObserverFuzzTest> observer = std::make_shared<KvStoreObserverFuzzTest>();
    if ((observer == nullptr) || (kvNbDelegatePtr == nullptr)) {
        return;
    }

    Key key = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));/* 1024 is max */
    Value value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));
    kvNbDelegatePtr->RegisterObserver(key, fdp.ConsumeIntegral<size_t>(), observer);
    kvNbDelegatePtr->SetConflictNotifier(fdp.ConsumeIntegral<size_t>(), [](const KvStoreNbConflictData &data) {
        (void)data.GetType();
    });
    TestCRUD(key, value, kvNbDelegatePtr, fdp);
    kvNbDelegatePtr->StartTransaction();
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(fdp, keys);
    kvNbDelegatePtr->PutBatch(tmp);
    if (!keys.empty()) {
        /* random deletePublic updateTimestamp 2 */
        bool deletePublic = fdp.ConsumeBool(); // use index 0 and 1
        bool updateTimestamp = fdp.ConsumeBool(); // use index 2 and 1
        kvNbDelegatePtr->UnpublishToLocal(keys[0], deletePublic, updateTimestamp);
    }
    kvNbDelegatePtr->DeleteBatch(keys);
    kvNbDelegatePtr->Rollback();
    kvNbDelegatePtr->Commit();
    kvNbDelegatePtr->UnRegisterObserver(observer);
    observer = nullptr;
    kvNbDelegatePtr->PutLocalBatch(tmp);
    kvNbDelegatePtr->DeleteLocalBatch(keys);
    std::string tmpStoreId = kvNbDelegatePtr->GetStoreId();
    SecurityOption secOption;
    kvNbDelegatePtr->GetSecurityOption(secOption);
    kvNbDelegatePtr->CheckIntegrity();
    FuzzSetInterceptorTest(kvNbDelegatePtr);
    if (!keys.empty()) {
        bool deleteLocal = fdp.ConsumeBool(); // use index 0 and 1
        bool updateTimestamp = fdp.ConsumeBool(); // use index 2 and 1
        /* random deletePublic updateTimestamp 2 */
        kvNbDelegatePtr->PublishLocal(keys[0], deleteLocal, updateTimestamp, nullptr);
    }
    kvNbDelegatePtr->DeleteBatch(keys);
    kvNbDelegatePtr->GetTaskCount();
    std::string rawString = fdp.ConsumeRandomLengthString(fdp.ConsumeIntegralInRange<size_t>(0, MOD));
    kvNbDelegatePtr->RemoveDeviceData(rawString);
    RemoveDeviceDataByMode(fdp, kvNbDelegatePtr);
    GetDeviceEntriesTest(fdp, kvNbDelegatePtr);
    RemoteQueryFuzz(fdp, kvNbDelegatePtr);
}

void EncryptOperation(FuzzedDataProvider &fdp, std::string &DirPath, KvStoreNbDelegate *kvNbDelegatePtr)
{
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    CipherPassword passwd;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t* val = static_cast<uint8_t*>(new uint8_t[size]);
    fdp.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    kvNbDelegatePtr->Rekey(passwd);
    int len = fdp.ConsumeIntegralInRange<int>(0, HUNDRED); // set min 100
    std::string fileName = fdp.ConsumeRandomLengthString(len);
    std::string mulitExportFileName = DirPath + "/" + fileName + ".db";
    kvNbDelegatePtr->Export(mulitExportFileName, passwd);
    kvNbDelegatePtr->Import(mulitExportFileName, passwd);
}

CipherPassword GetPassWord(FuzzedDataProvider &fdp)
{
    CipherPassword passwd;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t *val = static_cast<uint8_t*>(new uint8_t[size]);
    fdp.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    return passwd;
}

KvStoreNbDelegate::Option GetKvStoreNbDelegateOption(FuzzedDataProvider &fdp)
{
    bool createIfNecessary = fdp.ConsumeBool();
    bool isMemoryDb = fdp.ConsumeBool();
    bool isEncryptedDb = fdp.ConsumeBool();
    CipherType cipher = static_cast<CipherType>(fdp.ConsumeIntegralInRange<int>(MIN_CIPHER_TYPE, MAX_CIPHER_TYPE));
    CipherPassword passwd = GetPassWord(fdp);
    std::string schema = fdp.ConsumeRandomLengthString();
    bool createDirByStoreIdOnly = fdp.ConsumeBool();
    int securityLabel = fdp.ConsumeIntegralInRange<int>(SecurityLabel::INVALID_SEC_LABEL, SecurityLabel::S4);
    int securityFlag = fdp.ConsumeIntegralInRange<int>(SecurityFlag::INVALID_SEC_FLAG, SecurityFlag::SECE);
    SecurityOption secOption = {static_cast<SecurityLabel>(securityLabel), static_cast<SecurityFlag>(securityFlag)};
    std::shared_ptr<KvStoreObserver> observer = nullptr;
    size_t bytesSize = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
    Key key = fdp.ConsumeBytes<uint8_t>(bytesSize);
    unsigned int mode = fdp.ConsumeIntegralInRange<unsigned int>(
        ObserverMode::OBSERVER_CHANGES_NATIVE, ObserverMode::OBSERVER_CHANGES_DATA);
    int conflictType = fdp.ConsumeIntegral<int>();
    KvStoreNbConflictNotifier notifier = nullptr;
    int conflictResolvePolicy =
        fdp.ConsumeIntegralInRange<int>(ConflictResolvePolicy::LAST_WIN, ConflictResolvePolicy::DEVICE_COLLABORATION);
    bool isNeedIntegrityCheck = fdp.ConsumeBool();
    bool isNeedRmCorruptedDb = fdp.ConsumeBool();
    bool isNeedCompressOnSync = fdp.ConsumeBool();
    uint8_t compressionRate = fdp.ConsumeIntegralInRange<uint8_t>(1, HUNDRED); // Valid in [1, 100].
    bool syncDualTupleMode = fdp.ConsumeBool();
    bool localOnly = fdp.ConsumeBool();
    std::string storageEngineType = fdp.ConsumeRandomLengthString();
    Rdconfig rdConfig = {
        .readOnly = fdp.ConsumeBool(),
        .type = static_cast<IndexType>(fdp.ConsumeIntegralInRange<uint32_t>(MIN_INDEX_TYPE, MAX_INDEX_TYPE)),
        .pageSize = fdp.ConsumeIntegralInRange<uint32_t>(1, MAX_PAGE_SIZE), // Valid in [1, 32].
        .cacheSize = fdp.ConsumeIntegralInRange<uint32_t>(1, MAX_CACHE_SIZE), // Valid in [1, 2048].
    };
    return {createIfNecessary,
            isMemoryDb,
            isEncryptedDb,
            cipher,
            passwd,
            schema,
            createDirByStoreIdOnly,
            secOption,
            observer,
            key,
            mode,
            conflictType,
            notifier,
            conflictResolvePolicy,
            isNeedIntegrityCheck,
            isNeedRmCorruptedDb,
            isNeedCompressOnSync,
            compressionRate,
            syncDualTupleMode,
            localOnly,
            storageEngineType,
            rdConfig};
}

void StoreManagerFuzz(FuzzedDataProvider &fdp)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    std::string dataDir = fdp.ConsumeRandomLengthString();
    KvStoreConfig config = { dataDir };
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    kvManager.SetKvStoreConfig(config);
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    KvStoreNbDelegate::Option option = GetKvStoreNbDelegateOption(fdp);
    const std::string storeId = fdp.ConsumeRandomLengthString();
    kvManager.GetKvStore(storeId, option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate *kvDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvDelegate;
            }
        });
    bool isCloseImmediately = fdp.ConsumeBool();
    kvManager.CloseKvStore(kvNbDelegatePtr, isCloseImmediately);
    kvManager.DeleteKvStore(storeId);
}

void CombineTest(FuzzedDataProvider &fdp, KvStoreNbDelegate::Option &option)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    kvManager.SetKvStoreConfig(config);
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    kvManager.GetKvStore("distributed_nb_delegate_test", option,
        [&kvNbDelegatePtr] (DBStatus status, KvStoreNbDelegate *kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    FuzzCURD(fdp, kvNbDelegatePtr);
    SetCloudDBFuzz(fdp, kvNbDelegatePtr);
    SetCloudDBSchemaFuzz(fdp, kvNbDelegatePtr);
    SetRemotePushFinishedNotifyFuzz(fdp, kvNbDelegatePtr);
    SetEqualIdentifierFuzz(fdp, kvNbDelegatePtr);
    SyncDeviceFuzz(fdp, kvNbDelegatePtr);
    SyncCloudFuzz(fdp, kvNbDelegatePtr);
    GetCloudVersionFuzz(fdp, kvNbDelegatePtr);
    SetCloudSyncConfigFuzz(fdp, kvNbDelegatePtr);
    if (option.isEncryptedDb) {
        EncryptOperation(fdp, config.dataDir, kvNbDelegatePtr);
    }
    RuntimeContext::GetInstance()->StopTaskPool();
    kvManager.CloseKvStore(kvNbDelegatePtr);
    kvManager.DeleteKvStore("distributed_nb_delegate_test");
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    DistributedDB::KvStoreNbDelegate::Option option = {true, false, false};
    FuzzedDataProvider fdp(data, size);
    OHOS::CombineTest(fdp, option);
    OHOS::StoreManagerFuzz(fdp);
    option = {true, true, false};
    OHOS::CombineTest(fdp, option);
    return 0;
}
