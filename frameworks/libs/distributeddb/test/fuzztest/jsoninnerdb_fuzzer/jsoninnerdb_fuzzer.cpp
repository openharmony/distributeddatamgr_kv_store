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
#include "jsoninnerdb_fuzzer.h"

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

#include "grd_db_api_inner.h"
#include "grd_document_api_inner.h"
#include "grd_kv_api_inner.h"
#include "grd_resultset_api_inner.h"
#include "grd_sequence_api_inner.h"
#include "grd_type_inner.h"
#include "securec.h"

using namespace DocumentDB;

const char *TEST_DB = "./data";
const char *TEST_DB_FILE = "./data/testfile";
const char *COLLECTION_NAME = "collectionname";
const char *OPTION_STR = "{ \"maxdoc\" : 1000}";
const char *CONFIG_STR = "{}";
const char *DB_DIR_PATH = "./data/";
const int MAX_LEN_NUM = 600;
const int MAX_SIZE_NUM = 1000;

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

std::string GetMaxString()
{
    std::string str = "{";
    for (int i = 0; i <= MAX_SIZE_NUM; i++) {
        for (int j = 0; j <= MAX_LEN_NUM; j++) {
            str += "a";
        }
    }
    return str + "}";
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
        GRD_InsertDocInner(g_db, collectionName, item, 0);
    }
}

void SetUpTestCase()
{
    (void)RemoveDir(TEST_DB);
    MakeDir(TEST_DB);
    GRD_DBOpenInner(TEST_DB_FILE, CONFIG_STR, GRD_DB_OPEN_CREATE, &g_db);
    InsertData(g_db, COLLECTION_NAME);
    GRD_CreateCollectionInner(g_db, COLLECTION_NAME, OPTION_STR, 0);
}

void TearDownTestCase()
{
    GRD_DropCollectionInner(g_db, COLLECTION_NAME, 0);
    GRD_DBCloseInner(g_db, GRD_DB_CLOSE);
    (void)RemoveDir(TEST_DB);
    g_db = nullptr;
}
} // namespace

namespace {
void FindAndRelease(Query query)
{
    GRD_ResultSet *resultSet = nullptr;
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);
    resultSet = nullptr;
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_NextInner(resultSet);
    value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);

    const char *newCollName = "./student";
    resultSet = nullptr;
    GRD_FindDocInner(g_db, newCollName, query, 1, &resultSet);
    GRD_NextInner(resultSet);
    value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);
    resultSet = nullptr;
    GRD_FindDocInner(g_db, newCollName, query, 0, &resultSet);
    GRD_NextInner(resultSet);
    value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);
}
} // namespace

void FindAndReleaseFuzz(std::string document, std::string filter, Query query, const std::string &input)
{
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document.c_str(), 0);
    query.filter = filter.c_str();
    query.projection = "{\"field\": 0}";
    FindAndRelease(query);
    document = "{\"field1.field2\" [false], \"field1.field2.field3\": [3], "
               "\"field1.field4\": [null]}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document.c_str(), 0);
    query.filter = document.c_str();
    query.projection = "{\"field\": 0}";
    FindAndRelease(query);
    query.projection = "{\"field\": 1}";
    FindAndRelease(query);
    query.projection = "{\"field.field2\": 1}";
    FindAndRelease(query);
    document = "{\"field1\": {\"field2\": [{\"field3\":\"" + input + "\"}]}}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document.c_str(), 0);
    filter = "{\"field1.field2.field3\":\"" + input + "\"}";
    query.filter = filter.c_str();
    query.projection = "{\"field1.field2\": 1}";
    FindAndRelease(query);
    query.projection = "{\"field1.field2\": 0}";
    FindAndRelease(query);
    query.projection = "{\"field1.field3\": 1}";
    FindAndRelease(query);
    query.projection = "{\"field1.field3\": 0}";
    FindAndRelease(query);
    query.projection = "{\"field1.field2.field3\": 0, \"field1.field2.field3\": 0}";
    FindAndRelease(query);
    std::string projection = "{\"field1\": \"" + input + "\"}";
    query.projection = projection.c_str();
    FindAndRelease(query);
    projection = "{\"" + input + "\": true, \"" + input + "\": false}";
    query.projection = projection.c_str();
    FindAndRelease(query);
}

void NextFuzz(FuzzedDataProvider &provider)
{
    GRD_CreateCollectionInner(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input = provider.ConsumeRandomLengthString();
    std::string inputJson = "{" + input + "}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    Query query = { inputJson.c_str(), "{}" };
    FindAndRelease(query);
    std::string stringJson2 = "{ \"_id\":\"1\", \"field\":\"" + input + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, stringJson2.c_str(), 0);
    query.filter = "{\"_id\": \"1\"}";
    FindAndRelease(query);
    std::string filter2 = "{ \"_id\":\"1\", \"field\":\"" + input + " invalid\"}";
    query.filter = filter2.c_str();
    FindAndRelease(query);
    query.filter = "{\"_id\": \"3\"}";
    FindAndRelease(query);
    std::string stringJson3 = "{ \"_id\":\"2\", \"field\": [\"field2\", null, \"abc\", 123]}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, stringJson3.c_str(), 0);
    query.filter = "{\"field\": [\"field2\", null, \"abc\", 123]}";
    FindAndRelease(query);
    std::string document = "{\"field\": [\"" + input + "\",\"" + input + "\",\"" + input + "\"]}";
    size_t size = provider.ConsumeIntegral<size_t>();
    std::string filter = "{\"field." + std::to_string(size) + "\":\"" + input + "\"}";
    FindAndReleaseFuzz(document, filter, query, input);
    GRD_DropCollectionInner(g_db, COLLECTION_NAME, 0);
}

void DbOpenCloseFuzz(const char *dbFileVal, const char *configStr, GRD_DB *dbVal)
{
    int ret = GRD_DBOpenInner(dbFileVal, configStr, GRD_DB_OPEN_CREATE, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBCloseInner(dbVal, GRD_DB_CLOSE);
    }
}

void DbOpenOneFuzz(GRD_DB *dbVal)
{
    std::string path = "./documentFuzz.db";
    int ret = GRD_DBOpenInner(path.c_str(), "", GRD_DB_OPEN_ONLY, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBCloseInner(dbVal, GRD_DB_CLOSE);
    }
    (void)remove(path.c_str());

    path = "./document.db";
    DbOpenCloseFuzz(path.c_str(), R""({"pageSize":64, "redopubbufsize":4033})"", dbVal);
    DbOpenCloseFuzz(path.c_str(), R""({"pageSize":64, "redopubbufsize":4032})"", dbVal);
    DbOpenCloseFuzz(path.c_str(), R""({"redopubbufsize":255})"", dbVal);
    DbOpenCloseFuzz(path.c_str(), R""({"redopubbufsize":16385})"", dbVal);
}

void DbOpenFuzz(FuzzedDataProvider &provider)
{
    std::string dbFileData = provider.ConsumeRandomLengthString();
    std::string realDbFileData = DB_DIR_PATH + dbFileData;
    const char *dbFileVal = realDbFileData.data();
    std::string configStrData = provider.ConsumeRandomLengthString();
    const char *configStrVal = configStrData.data();
    GRD_DB *dbVal = nullptr;

    GRD_DropCollectionInner(g_db, COLLECTION_NAME, 0);
    std::string stringMax = GetMaxString();
    const char *configStrMaxLen = stringMax.data();
    DbOpenCloseFuzz(dbFileVal, configStrMaxLen, dbVal);

    std::string fieldStringValue = "{\"bufferPoolSize\": \"1024.5\"}";
    const char *configStrStringValue = fieldStringValue.data();
    DbOpenCloseFuzz(dbFileVal, configStrStringValue, dbVal);

    std::string fieldBoolValue = "{\"bufferPoolSize\":}";
    const char *configStrBool = fieldBoolValue.data();
    DbOpenCloseFuzz(dbFileVal, configStrBool, dbVal);

    std::string fieldStringValueAppend = "{\"bufferPoolSize\":\"8192\"}";
    const char *configStrStr = fieldStringValueAppend.data();
    DbOpenCloseFuzz(dbFileVal, configStrStr, dbVal);

    std::string fieldStringValueFlush = "{\"bufferPoolSize\":\"8192\",\"redoFlushBtTrx\":\"1\"}";
    const char *configStrFlush = fieldStringValueFlush.data();
    DbOpenCloseFuzz(dbFileVal, configStrFlush, dbVal);

    std::string fieldStringValueRedoBufsize = "{\"bufferPoolSize\":\"8192\",\"redoBufSize\":\"16384\"}";
    const char *configStrBs = fieldStringValueRedoBufsize.data();
    DbOpenCloseFuzz(dbFileVal, configStrBs, dbVal);

    std::string fieldStringValueMaxConnNum = "{\"bufferPoolSize\":\"8192\",\"maxConnNum\":\"1024\"}";
    const char *configStrMcn = fieldStringValueMaxConnNum.data();
    DbOpenCloseFuzz(dbFileVal, configStrMcn, dbVal);

    GRD_DropCollectionInner(g_db, COLLECTION_NAME, 0);
    std::string fieldStringValueAll = "{\"bufferPoolSize\":\"8192\",\"redoFlushBtTrx\":\"1\",\"redoBufSize\":"
                                        "\"16384\",\"maxConnNum\":\"1024\"}";
    const char *configStrAll = fieldStringValueAll.data();
    DbOpenCloseFuzz(dbFileVal, configStrAll, dbVal);

    std::string fieldLowNumber = "{\"bufferPoolSize\": 102}";
    const char *configStrLowNumber = fieldLowNumber.data();
    DbOpenCloseFuzz(dbFileVal, configStrLowNumber, dbVal);

    int ret = GRD_DBOpenInner(nullptr, configStrVal, GRD_DB_OPEN_CREATE, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBCloseInner(nullptr, GRD_DB_CLOSE);
    }
    DbOpenCloseFuzz(dbFileVal, configStrVal, dbVal);

    DbOpenOneFuzz(dbVal);
}

void DbCloseFuzz(FuzzedDataProvider &provider)
{
    std::string dbFileData = provider.ConsumeRandomLengthString();
    std::string realDbFileData = DB_DIR_PATH + dbFileData;
    const char *dbFileVal = realDbFileData.data();
    std::string configStrData = provider.ConsumeRandomLengthString();
    const char *configStrVal = configStrData.data();
    GRD_DB *dbVal = nullptr;
    DbOpenCloseFuzz(dbFileVal, configStrVal, dbVal);
}

void DbFlushFuzz(FuzzedDataProvider &provider)
{
    GRD_DB *db = nullptr;
    GRD_DB *db2 = nullptr;
    const uint32_t flags = provider.ConsumeIntegral<uint32_t>();
    int ret = GRD_DBOpenInner(TEST_DB_FILE, CONFIG_STR, GRD_DB_OPEN_CREATE, &db);
    if (ret == GRD_OK) {
        GRD_FlushInner(db, flags);
        GRD_DBCloseInner(db, GRD_DB_CLOSE);
    }
    GRD_FlushInner(db2, flags);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" {
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetUpTestCase();
    FuzzedDataProvider provider(data, size);
    OHOS::DbOpenFuzz(provider);
    OHOS::NextFuzz(provider);
    OHOS::DbCloseFuzz(provider);
    OHOS::DbFlushFuzz(provider);

    OHOS::TearDownTestCase();
    return 0;
}
}
