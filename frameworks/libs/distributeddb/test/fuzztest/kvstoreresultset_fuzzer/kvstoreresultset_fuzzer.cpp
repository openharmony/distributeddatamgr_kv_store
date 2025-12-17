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

#include "kvstoreresultset_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "process_communicator_test_stub.h"

using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace std;

namespace OHOS {
static auto g_kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
static constexpr const int MOD = 1024; // 1024 is mod
static constexpr const int MIN_COLUMN_TYPE = static_cast<int>(ResultSet::ColumnType::INVALID_TYPE);
static constexpr const int MAX_COLUMN_TYPE = static_cast<int>(ResultSet::ColumnType::NULL_VALUE) + 1;
void ResultSetFuzzer(FuzzedDataProvider &fdp)
{
    KvStoreNbDelegate::Option option = {true, false, true};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    g_kvManager.GetKvStore("distributed_nb_delegate_result_set_test", option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate * kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    if (kvNbDelegatePtr == nullptr) {
        return;
    }

    Key testKey;
    Value testValue;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
    int byteLen = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (size_t i = 0; i < size; i++) {
        testKey = fdp.ConsumeBytes<uint8_t>(byteLen);
        testValue = fdp.ConsumeBytes<uint8_t>(byteLen);
        kvNbDelegatePtr->Put(testKey, testValue);
    }

    Key keyPrefix;
    KvStoreResultSet *readResultSet = nullptr;
    kvNbDelegatePtr->GetEntries(keyPrefix, readResultSet);
    if (readResultSet != nullptr) {
        readResultSet->GetCount();
        readResultSet->GetPosition();
        readResultSet->MoveToNext();
        readResultSet->MoveToPrevious();
        readResultSet->MoveToFirst();
        readResultSet->MoveToLast();

        if (size == 0) {
            return;
        }
        auto pos = fdp.ConsumeIntegralInRange<size_t>(0, size);
        readResultSet->MoveToPosition(pos++);
        readResultSet->Move(0 - pos);
        readResultSet->IsFirst();
        readResultSet->IsLast();
        readResultSet->IsBeforeFirst();
        readResultSet->IsAfterLast();
        readResultSet->IsClosed();
        readResultSet->Close();
        kvNbDelegatePtr->CloseResultSet(readResultSet);
    }

    g_kvManager.CloseKvStore(kvNbDelegatePtr);
    g_kvManager.DeleteKvStore("distributed_nb_delegate_result_set_test");
}

void GetColumnFuzzer(KvStoreResultSet *readResultSet, FuzzedDataProvider &fdp)
{
    std::vector<std::string> colNames;
    std::string columnName = fdp.ConsumeRandomLengthString();
    int size = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (int i = 0; i < size; i++) {
        colNames.emplace_back(columnName);
    }
    readResultSet->GetColumnNames(colNames);
    int columnIndex = fdp.ConsumeIntegralInRange<int>(0, MOD);
    readResultSet->GetColumnIndex(columnName, columnIndex);
    ResultSet::ColumnType columnType =
        static_cast<ResultSet::ColumnType>(fdp.ConsumeIntegralInRange<int>(MIN_COLUMN_TYPE, MAX_COLUMN_TYPE));
    readResultSet->GetColumnType(columnIndex, columnType);
    readResultSet->GetColumnName(columnIndex, columnName);
    std::vector<uint8_t> valueBlob = fdp.ConsumeBytes<uint8_t>(size);
    readResultSet->Get(columnIndex, valueBlob);
    std::string valueString = fdp.ConsumeRandomLengthString();
    readResultSet->Get(columnIndex, valueString);
    int64_t valueLong = fdp.ConsumeIntegral<int64_t>();
    readResultSet->Get(columnIndex, valueLong);
    double valueDouble = fdp.ConsumeFloatingPoint<double>();
    readResultSet->Get(columnIndex, valueDouble);
    bool isNull = fdp.ConsumeBool();
    readResultSet->IsColumnNull(columnIndex, isNull);
    std::map<std::string, VariantData> data = {};
    std::vector<VariantData> variants = {};
    variants.emplace_back(valueBlob);
    variants.emplace_back(valueString);
    variants.emplace_back(valueLong);
    variants.emplace_back(valueDouble);
    for (int i = 0; i < variants.size(); i++) {
        data[columnName] = variants[i];
    }
    readResultSet->GetRow(data);
}

void ResultSetColumnFuzzer(FuzzedDataProvider &fdp)
{
    KvStoreNbDelegate::Option option = {true, false, true};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    g_kvManager.GetKvStore("distributed_nb_delegate_result_set_test", option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate *kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    if (kvNbDelegatePtr == nullptr) {
        return;
    }

    Key testKey;
    Value testValue;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
    int byteLen = fdp.ConsumeIntegralInRange<int>(0, MOD);
    for (size_t i = 0; i < size; i++) {
        testKey = fdp.ConsumeBytes<uint8_t>(byteLen);
        testValue = fdp.ConsumeBytes<uint8_t>(byteLen);
        kvNbDelegatePtr->Put(testKey, testValue);
    }

    Key keyPrefix;
    KvStoreResultSet *readResultSet = nullptr;
    kvNbDelegatePtr->GetEntries(keyPrefix, readResultSet);
    if (readResultSet != nullptr) {
        readResultSet->GetCount();
        readResultSet->MoveToFirst();

        if (size == 0) {
            return;
        }
        Entry entry;
        entry.key = fdp.ConsumeBytes<uint8_t>(byteLen);
        entry.value = fdp.ConsumeBytes<uint8_t>(byteLen);
        readResultSet->GetEntry(entry);
        GetColumnFuzzer(readResultSet, fdp);
        kvNbDelegatePtr->CloseResultSet(readResultSet);
    }

    g_kvManager.CloseKvStore(kvNbDelegatePtr);
    g_kvManager.DeleteKvStore("distributed_nb_delegate_result_set_test");
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // u32 4 bytes
    if (size < 4) {
        return 0;
    }
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    OHOS::g_kvManager.SetKvStoreConfig(config);
    FuzzedDataProvider fdp(data, size);
    OHOS::ResultSetFuzzer(fdp);
    OHOS::ResultSetColumnFuzzer(fdp);
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    return 0;
}

