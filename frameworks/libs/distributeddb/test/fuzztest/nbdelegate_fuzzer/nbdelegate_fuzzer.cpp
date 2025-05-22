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
#include "distributeddb_tools_test.h"
#include "fuzzer_data.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_observer.h"
#include "platform_specific.h"

class KvStoreNbDelegateCURDFuzzer {
    /* Keep C++ file names the same as the class name. */
};

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
static constexpr const int HUNDRED = 100;
static constexpr const int MOD = 1024;
static constexpr const int PASSWDLEN = 20;
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
    auto count = fdp.ConsumeIntegralInRange<int>(0, 1024);
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = fdp.ConsumeBytes<uint8_t>(i);
        keys.push_back(entry.key);
        entry.value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, HUNDRED));
        entries.push_back(entry);
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

void TestCRUD(const Key &key, const Value &value, KvStoreNbDelegate *kvNbDelegatePtr)
{
    Value valueRead;
    kvNbDelegatePtr->PutLocal(key, value);
    kvNbDelegatePtr->GetLocal(key, valueRead);
    kvNbDelegatePtr->DeleteLocal(key);
    kvNbDelegatePtr->Put(key, value);
    kvNbDelegatePtr->Put(key, value);
    kvNbDelegatePtr->UpdateKey([](const Key &origin, Key &newKey) {
        newKey = origin;
        newKey.push_back('0');
    });
    std::vector<Entry> vect;
    kvNbDelegatePtr->GetEntries(key, vect);
    vect.clear();
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
    auto observer = new (std::nothrow) KvStoreObserverFuzzTest;
    if ((observer == nullptr) || (kvNbDelegatePtr == nullptr)) {
        return;
    }

    Key key = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));/* 1024 is max */
    Value value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));
    kvNbDelegatePtr->RegisterObserver(key, fdp.ConsumeIntegral<size_t>(), observer);
    kvNbDelegatePtr->SetConflictNotifier(fdp.ConsumeIntegral<size_t>(), [](const KvStoreNbConflictData &data) {
        (void)data.GetType();
    });
    TestCRUD(key, value, kvNbDelegatePtr);
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
    delete observer;
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
    int len = fdp.ConsumeIntegralInRange<int>(0, 100); // set min 100
    std::string fileName = fdp.ConsumeRandomLengthString(len);
    std::string mulitExportFileName = DirPath + "/" + fileName + ".db";
    kvNbDelegatePtr->Export(mulitExportFileName, passwd);
    kvNbDelegatePtr->Import(mulitExportFileName, passwd);
}

void CombineTest(FuzzedDataProvider &fdp, KvStoreNbDelegate::Option &option)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    kvManager.SetKvStoreConfig(config);
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    kvManager.GetKvStore("distributed_nb_delegate_test", option,
        [&kvNbDelegatePtr] (DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    FuzzCURD(fdp, kvNbDelegatePtr);
    if (option.isEncryptedDb) {
        EncryptOperation(fdp, config.dataDir, kvNbDelegatePtr);
    }
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
    option = {true, true, false};
    OHOS::CombineTest(fdp, option);
    return 0;
}
