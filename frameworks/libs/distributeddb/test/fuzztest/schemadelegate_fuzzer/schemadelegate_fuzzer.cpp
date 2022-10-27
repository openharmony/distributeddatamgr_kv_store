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

#include "schemadelegate_fuzzer.h"

#include <stddef.h>
#include <stdint.h>

#include "distributeddb_tools_test.h"
#include "fuzzer_data.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_observer.h"
#include "platform_specific.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

namespace OHOS {
const std::string VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
    "\"SCHEMA_MODE\":\"STRICT\","
    "\"SCHEMA_DEFINE\":{"
        "\"field_name1\":\"BOOL\","
        "\"field_name2\":\"INTEGER, NOT NULL\""
    "},"
    "\"SCHEMA_INDEXES\":[\"$.field_name1\"]}";
const std::string VALID_SCHEMA_COMPA_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
    "\"SCHEMA_MODE\":\"COMPATIBLE\","
    "\"SCHEMA_DEFINE\":{"
        "\"field_name1\":\"BOOL\","
        "\"field_name2\":\"INTEGER, NOT NULL\""
    "},"
    "\"SCHEMA_INDEXES\":[\"$.field_name1\"]}";

void SchemaFuzzCURD(FuzzerData &fuzz, KvStoreNbDelegate *delegate)
{
    if (delegate == nullptr) {
        return;
    }

    Key key = fuzz.GetSequence(fuzz.GetInt());
    Value val = fuzz.GetSequence(fuzz.GetInt());
    delegate->Put(key, val);
    Value valGot;
    delegate->Get(key, valGot);
    delegate->Delete(key);

    int cnt = fuzz.GetInt() % 200;
    std::vector<Key> keys;
    std::vector<Value> values;
    std::vector<Entry> entries;
    for (int i = 0; i < cnt; i++) {
        Key key1 = fuzz.GetSequence(fuzz.GetInt());
        Value val1 = fuzz.GetSequence(fuzz.GetInt());
        keys.push_back(key1);
        values.push_back(val1);
        entries.emplace_back(Entry {key1, val1});
    }

    delegate->PutBatch(entries);

    std::vector<Entry> entriesGot;
    Key keyPrefix = fuzz.GetSequence(fuzz.GetInt());
    delegate->GetEntries(keyPrefix, entriesGot);

    std::vector<Entry> entriesGotByQuery;
    delegate->GetEntries(Query::Select(), entriesGotByQuery);

    KvStoreResultSet *readResultSet = nullptr;
    delegate->GetEntries(keyPrefix, readResultSet);
    delegate->CloseResultSet(readResultSet);

    delegate->GetEntries(Query::Select(), readResultSet);
    delegate->CloseResultSet(readResultSet);

    delegate->DeleteBatch(keys);
}

bool SchemaFuzzTest(const uint8_t* data, size_t size)
{
    FuzzerData fuzz(data, size);
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    kvManager.SetKvStoreConfig(config);
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    DistributedDB::KvStoreNbDelegate::Option option = {true, false, false};
    if (fuzz.GetInt() % 2 == 0) {
        option.schema = VALID_SCHEMA_STRICT_DEFINE;
    } else {
        option.schema = VALID_SCHEMA_COMPA_DEFINE;
    }

    kvManager.GetKvStore("distributed_nb_delegate_test", option,
        [&kvNbDelegatePtr] (DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    if (kvNbDelegatePtr == nullptr) {
        return false;
    }
    SchemaFuzzCURD(fuzz, kvNbDelegatePtr);
    kvManager.CloseKvStore(kvNbDelegatePtr);
    kvManager.DeleteKvStore("distributed_nb_delegate_test");
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::SchemaFuzzTest(data, size);
    return 0;
}

