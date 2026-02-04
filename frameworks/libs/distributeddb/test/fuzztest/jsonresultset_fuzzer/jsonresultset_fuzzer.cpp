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
#include "jsonresultset_fuzzer.h"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "fuzzer/FuzzedDataProvider.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_base/grd_resultset_api.h"
#include "grd_document/grd_document_api.h"
#include "grd_kv/grd_kv_api.h"
#include "grd_resultset_inner.h"
#include "securec.h"

const char *TEST_DB = "./data";
const char *TEST_DB_FILE = "./data/testfile";
const char *COLLECTION_NAME = "collectionname";
const char *OPTION_STR = "{ \"maxdoc\" : 1000}";
const char *CONFIG_STR = "{}";
const int SMALL_PREFIX_LEN = 5;
const int NUM_NINETY_EIGHT = 98;
const int BATCH_COUNT = 100;
const int MAX_SIZE_NUM = 1000;
const int CURSOR_COUNT = 50000;
const int VECTOR_SIZE = 100000;

static GRD_DB *g_db = nullptr;

namespace OHOS {
namespace {
int RemoveDir(const char *dir)
{
    if (dir == nullptr) {
        return -1;
    }
    if (access(dir, F_OK) != 0) {
        return 0;
    }
    struct stat dirStat;
    if (stat(dir, &dirStat) < 0) {
        return -1;
    }

    char dirName[PATH_MAX];
    DIR *dirPtr = nullptr;
    struct dirent *dr = nullptr;
    if (S_ISREG(dirStat.st_mode)) { // normal file
        remove(dir);
    } else if (S_ISDIR(dirStat.st_mode)) {
        dirPtr = opendir(dir);
        while ((dr = readdir(dirPtr)) != nullptr) {
            // ignore . and ..
            if ((strcmp(".", dr->d_name) == 0) || (strcmp("..", dr->d_name) == 0)) {
                continue;
            }
            if (sprintf_s(dirName, sizeof(dirName), "%s / %s", dir, dr->d_name) < 0) {
                (void)RemoveDir(dirName);
                closedir(dirPtr);
                rmdir(dir);
                return -1;
            }
            (void)RemoveDir(dirName);
        }
        closedir(dirPtr);
        rmdir(dir); // remove empty dir
    } else {
        return -1;
    }
    return 0;
}

void MakeDir(const char *dir)
{
    std::string tmpPath;
    const char *pcur = dir;

    while (*pcur++ != '\0') {
        tmpPath.push_back(*(pcur - 1));
        if ((*pcur == '/' || *pcur == '\0') && access(tmpPath.c_str(), 0) != 0 && !tmpPath.empty()) {
            if (mkdir(tmpPath.c_str(), (S_IRUSR | S_IWUSR | S_IXUSR)) != 0) {
                return;
            }
        }
    }
}
} // namespace

static const char *DOCUMENT1 = "{\"_id\" : \"1\", \"name\":\"doc1\",\"item\":\"journal\",\"personInfo\":\
    {\"school\":\"AB\", \"age\" : 51}}";
static const char *DOCUMENT2 = "{\"_id\" : \"2\", \"name\":\"doc2\",\"item\": 1, \"personInfo\":\
    [1, \"my string\", {\"school\":\"AB\", \"age\" : 51}, true, {\"school\":\"CD\", \"age\" : 15}, false]}";
static const char *DOCUMENT3 = "{\"_id\" : \"3\", \"name\":\"doc3\",\"item\":\"notebook\",\"personInfo\":\
    [{\"school\":\"C\", \"age\" : 5}]}";
static const char *DOCUMENT4 = "{\"_id\" : \"4\", \"name\":\"doc4\",\"item\":\"paper\",\"personInfo\":\
    {\"grade\" : 1, \"school\":\"A\", \"age\" : 18}}";
static const char *DOCUMENT5 = "{\"_id\" : \"5\", \"name\":\"doc5\",\"item\":\"journal\",\"personInfo\":\
    [{\"sex\" : \"woma\", \"school\" : \"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *DOCUMENT6 = "{\"_id\" : \"6\", \"name\":\"doc6\",\"item\":false,\"personInfo\":\
    [{\"school\":\"B\", \"teacher\" : \"mike\", \"age\" : 15},\
    {\"school\":\"C\", \"teacher\" : \"moon\", \"age\" : 20}]}";

static const char *DOCUMENT7 = "{\"_id\" : \"7\", \"name\":\"doc7\",\"item\":\"fruit\",\"other_Info\":\
    [{\"school\":\"BX\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *DOCUMENT8 = "{\"_id\" : \"8\", \"name\":\"doc8\",\"item\":true,\"personInfo\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *DOCUMENT9 = "{\"_id\" : \"9\", \"name\":\"doc9\",\"item\": true}";
static const char *DOCUMENT10 = "{\"_id\" : \"10\", \"name\":\"doc10\", \"parent\" : \"kate\"}";
static const char *DOCUMENT11 = "{\"_id\" : \"11\", \"name\":\"doc11\", \"other\" : \"null\"}";
static const char *DOCUMENT12 = "{\"_id\" : \"12\", \"name\":\"doc12\",\"other\" : null}";
static const char *DOCUMENT13 = "{\"_id\" : \"13\", \"name\":\"doc13\",\"item\" : \"shoes\",\"personInfo\":\
    {\"school\":\"AB\", \"age\" : 15}}";
static const char *DOCUMENT14 = "{\"_id\" : \"14\", \"name\":\"doc14\",\"item\" : true,\"personInfo\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 85}]}";
static const char *DOCUMENT15 = "{\"_id\" : \"15\", \"name\":\"doc15\",\"personInfo\":[{\"school\":\"C\", \"age\" : "
                              "5}]}";
static const char *DOCUMENT16 = "{\"_id\" : \"16\", \"name\":\"doc16\", \"nested1\":{\"nested2\":{\"nested3\":\
    {\"nested4\":\"ABC\", \"field2\":\"CCC\"}}}}";
static const char *DOCUMENT17 = "{\"_id\" : \"17\", \"name\":\"doc17\",\"personInfo\":\"oh,ok\"}";
static const char *DOCUMENT18 = "{\"_id\" : \"18\", \"name\":\"doc18\",\"item\" : \"mobile phone\",\"personInfo\":\
    {\"school\":\"DD\", \"age\":66}, \"color\":\"blue\"}";
static const char *DOCUMENT19 = "{\"_id\" : \"19\", \"name\":\"doc19\",\"ITEM\" : true,\"PERSONINFO\":\
    {\"school\":\"AB\", \"age\":15}}";
static const char *DOCUMENT20 = "{\"_id\" : \"20\", \"name\":\"doc20\",\"ITEM\" : true,\"personInfo\":\
    [{\"SCHOOL\":\"B\", \"AGE\":15}, {\"SCHOOL\":\"C\", \"AGE\":35}]}";
static const char *DOCUMENT23 = "{\"_id\" : \"23\", \"name\":\"doc22\",\"ITEM\" : "
                              "true,\"personInfo\":[{\"school\":\"b\", \"age\":15}, [{\"school\":\"doc23\"}, 10, "
                              "{\"school\":\"doc23\"}, true, {\"school\":\"y\"}], {\"school\":\"b\"}]}";
static std::vector<const char *> g_data = { DOCUMENT1, DOCUMENT2, DOCUMENT3, DOCUMENT4, DOCUMENT5,
    DOCUMENT6, DOCUMENT7, DOCUMENT8, DOCUMENT9, DOCUMENT10, DOCUMENT11, DOCUMENT12, DOCUMENT13,
    DOCUMENT14, DOCUMENT15, DOCUMENT16, DOCUMENT17, DOCUMENT18, DOCUMENT19, DOCUMENT20, DOCUMENT23 };

namespace {
static void InsertData(GRD_DB *g_db, const char *collectionName)
{
    for (const auto &item : g_data) {
        GRD_InsertDoc(g_db, collectionName, item, 0);
    }
}

void SetUpTestCase()
{
    (void)RemoveDir(TEST_DB);
    MakeDir(TEST_DB);
    GRD_DBOpen(TEST_DB_FILE, CONFIG_STR, GRD_DB_OPEN_CREATE, &g_db);
    InsertData(g_db, COLLECTION_NAME);
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
}

void TearDownTestCase()
{
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    GRD_DBClose(g_db, GRD_DB_CLOSE);
    (void)RemoveDir(TEST_DB);
    g_db = nullptr;
}
} // namespace

void GetValueFuzz(FuzzedDataProvider &provider)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input = provider.ConsumeRandomLengthString();
    std::string inputJson = "{" + input + "}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    char *value = nullptr;
    Query query = { inputJson.c_str(), "{}" };
    GRD_ResultSet *resultSet = nullptr;
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
}

void FreeResultSetFuzz()
{
    GRD_ResultSet *resultSet = nullptr;
    GRD_FreeResultSet(resultSet);
    resultSet = new GRD_ResultSet;
    resultSet->resultSet_ = DocumentDB::ResultSet();
    GRD_FreeResultSet(resultSet);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
}

void DbCloseResultSetFuzz()
{
    GRD_DB *db = nullptr;
    GRD_DB *db2 = nullptr;
    int ret = GRD_DBOpen(TEST_DB_FILE, CONFIG_STR, GRD_DB_OPEN_CREATE, &db);
    int errCode = GRD_DBOpen(TEST_DB_FILE, CONFIG_STR, GRD_DB_OPEN_CREATE, &db2);
    if (ret == GRD_OK) {
        GRD_CreateCollection(db, "collection1", "{\"maxdoc\" : 5}", 0);

        GRD_ResultSet *resultSet = nullptr;
        Query query = { "{}", "{}" };
        GRD_FindDoc(db, "collection1", query, 0, &resultSet);

        GRD_FreeResultSet(resultSet);

        GRD_DBClose(db, GRD_DB_CLOSE);
    }

    if (errCode == GRD_OK) {
        GRD_DBClose(db2, GRD_DB_CLOSE);
    }
}

void DbOpenCloseFuzz(const char *dbFileVal, const char *configStr, GRD_DB *dbVal)
{
    int ret = GRD_DBOpen(dbFileVal, configStr, GRD_DB_OPEN_CREATE, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBClose(dbVal, GRD_DB_CLOSE);
    }
}

void TestGrdDbApGrdGetItem002Fuzz()
{
    const char *config = CONFIG_STR;
    GRD_DB *db = nullptr;
    DbOpenCloseFuzz(TEST_DB_FILE, config, db);
    GRD_IndexPreload(nullptr, COLLECTION_NAME);
    GRD_IndexPreload(nullptr, "invalid_name");
    GRD_IndexPreload(g_db, COLLECTION_NAME);
    std::string smallPrefix = std::string(SMALL_PREFIX_LEN, 'a');
    for (uint32_t i = 0; i < NUM_NINETY_EIGHT; ++i) {
        std::string v = smallPrefix + std::to_string(i);
        GRD_KVItemT key = { &i, sizeof(uint32_t) };
        GRD_KVItemT value = { reinterpret_cast<void *>(v.data()), static_cast<uint32_t>(v.size()) + 1 };
        GRD_KVPut(g_db, COLLECTION_NAME, &key, &value);

        GRD_KVItemT getValue = { nullptr, 0 };
        GRD_KVGet(g_db, COLLECTION_NAME, &key, &getValue);
        GRD_KVFreeItem(&getValue);

        GRD_KVDel(g_db, COLLECTION_NAME, &key);
    }
    GRD_Flush(g_db, 1);

    uint32_t begin = 0;
    uint32_t end = MAX_SIZE_NUM;
    GRD_FilterOptionT option = {};
    option.mode = KV_SCAN_RANGE;
    option.begin = { &begin, sizeof(uint32_t) };
    option.end = { &end, sizeof(uint32_t) };
    GRD_ResultSet *resultSet = nullptr;
    GRD_KVFilter(g_db, COLLECTION_NAME, &option, &resultSet);

    uint32_t keySize;
    uint32_t valueSize;
    GRD_KVGetSize(resultSet, &keySize, &valueSize);

    resultSet = nullptr;
    uint32_t i = 32;
    GRD_KVItemT key = { &i, sizeof(uint32_t) };
    GRD_KVScan(g_db, COLLECTION_NAME, &key, KV_SCAN_EQUAL_OR_LESS_KEY, &resultSet);

    GRD_Prev(resultSet);
    GRD_Next(resultSet);

    GRD_FreeResultSet(resultSet);
}

void TestGrdKvBatchCoupling003Fuzz()
{
    const char *config = CONFIG_STR;
    GRD_DB *db = nullptr;
    DbOpenCloseFuzz(TEST_DB_FILE, config, db);

    GRD_ResultSet *resultSet = nullptr;
    GRD_KVScan(g_db, COLLECTION_NAME, nullptr, KV_SCAN_PREFIX, &resultSet);

    for (uint32_t i = 0; i < CURSOR_COUNT; i++) {
        GRD_Next(resultSet);
    }

    GRD_KVBatchT *batchDel = nullptr;
    std::vector<std::string> keySet;
    std::vector<std::string> valueSet;
    for (uint32_t i = 0; i < VECTOR_SIZE; i++) {
        std::string key(MAX_SIZE_NUM, 'a');
        key += std::to_string(i);
        keySet.emplace_back(key);
        std::string value = std::to_string(i);
        valueSet.emplace_back(value);
    }

    for (int j = 0; j < BATCH_COUNT; j++) {
        GRD_KVBatchPrepare(BATCH_COUNT, &batchDel);
        GRD_KVBatchPut(g_db, COLLECTION_NAME, batchDel);
        for (uint16_t i = 0; i < BATCH_COUNT; i++) {
            char *batchKey = const_cast<char *>(keySet[CURSOR_COUNT + j * BATCH_COUNT + i].c_str());
            char *batchValue = const_cast<char *>(valueSet[CURSOR_COUNT + j * BATCH_COUNT + i].c_str());
            GRD_KVBatchPushback(static_cast<void *>(batchKey),
                static_cast<uint32_t>(keySet[CURSOR_COUNT + j * BATCH_COUNT + i].length()) + 1,
                static_cast<void *>(batchValue),
                static_cast<uint32_t>(valueSet[CURSOR_COUNT + j * BATCH_COUNT + i].length()) + 1,
                batchDel);
        }

        GRD_KVBatchDel(g_db, COLLECTION_NAME, batchDel);

        GRD_KVBatchDestroy(batchDel);
    }

    GRD_Next(resultSet);
    GRD_KVItemT keyItem = { nullptr, 0 };
    GRD_KVItemT valueItem = { nullptr, 0 };
    GRD_Fetch(resultSet, &keyItem, &valueItem);
    GRD_KVFreeItem(&keyItem);
    GRD_KVFreeItem(&valueItem);
    GRD_FreeResultSet(resultSet);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" {
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetUpTestCase();
    FuzzedDataProvider provider(data, size);
    OHOS::GetValueFuzz(provider);
    OHOS::FreeResultSetFuzz();
    OHOS::TestGrdDbApGrdGetItem002Fuzz();
    OHOS::TestGrdKvBatchCoupling003Fuzz();
    OHOS::DbCloseResultSetFuzz();

    OHOS::TearDownTestCase();
    return 0;
}
}
