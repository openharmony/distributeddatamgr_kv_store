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

#include "parsecheckfield_fuzzer.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "schema_object.h"
#include "schema_utils.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

static KvStoreConfig g_config;

namespace OHOS {
void CheckFieldName(FuzzedDataProvider &fdp)
{
    std::string schemaAttrString = fdp.ConsumeRandomLengthString();
    SchemaUtils::CheckFieldName(schemaAttrString);
}

void ParseFieldPath(FuzzedDataProvider &fdp)
{
    std::string schemaAttrString = fdp.ConsumeRandomLengthString();
    FieldPath outPath;
    SchemaUtils::ParseAndCheckFieldPath(schemaAttrString, outPath);
}

void CheckSchemaAttribute(FuzzedDataProvider &fdp)
{
    std::string schemaAttrString = fdp.ConsumeRandomLengthString();
    SchemaAttribute outAttr;
    SchemaUtils::ParseAndCheckSchemaAttribute(schemaAttrString, outAttr);
    SchemaUtils::ParseAndCheckSchemaAttribute(schemaAttrString, outAttr);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    DistributedDBToolsTest::TestDirInit(g_config.dataDir);
    FuzzedDataProvider fdp(data, size);
    OHOS::CheckFieldName(fdp);
    OHOS::ParseFieldPath(fdp);
    OHOS::CheckSchemaAttribute(fdp);

    DistributedDBToolsTest::RemoveTestDbFiles(g_config.dataDir);
    return 0;
}
