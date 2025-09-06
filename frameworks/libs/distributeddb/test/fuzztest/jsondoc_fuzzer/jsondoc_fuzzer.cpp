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
#include "jsondoc_fuzzer.h"

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

constexpr const char *TEST_DB = "./data";
constexpr const char *TEST_DB_FILE = "./data/testfile";
constexpr const char *COLLECTION_NAME = "collectionname";
constexpr const char *OPTION_STR = "{ \"maxdoc\" : 1000}";
constexpr const char *CONFIG_STR = "{}";
const int NUM_FIFTEEN = 15;
const int BATCH_COUNT = 100;
const int LESS_HALF_BYTES = 511;
const int MORE_HALF_BYTES = 513;
const int ONE_BYTES = 1024;

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

static constexpr const char *DOCUMENT1 = "{\"_id\" : \"1\", \"name\":\"doc1\",\"item\":\"journal\",\"personInfo\":\
    {\"school\":\"AB\", \"age\" : 51}}";
static constexpr const char *DOCUMENT2 = "{\"_id\" : \"2\", \"name\":\"doc2\",\"item\": 1, \"personInfo\":\
    [1, \"my string\", {\"school\":\"AB\", \"age\" : 51}, true, {\"school\":\"CD\", \"age\" : 15}, false]}";
static constexpr const char *DOCUMENT3 = "{\"_id\" : \"3\", \"name\":\"doc3\",\"item\":\"notebook\",\"personInfo\":\
    [{\"school\":\"C\", \"age\" : 5}]}";
static constexpr const char *DOCUMENT4 = "{\"_id\" : \"4\", \"name\":\"doc4\",\"item\":\"paper\",\"personInfo\":\
    {\"grade\" : 1, \"school\":\"A\", \"age\" : 18}}";
static constexpr const char *DOCUMENT5 = "{\"_id\" : \"5\", \"name\":\"doc5\",\"item\":\"journal\",\"personInfo\":\
    [{\"sex\" : \"woma\", \"school\" : \"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static constexpr const char *DOCUMENT6 = "{\"_id\" : \"6\", \"name\":\"doc6\",\"item\":false,\"personInfo\":\
    [{\"school\":\"B\", \"teacher\" : \"mike\", \"age\" : 15},\
    {\"school\":\"C\", \"teacher\" : \"moon\", \"age\" : 20}]}";

static constexpr const char *DOCUMENT7 = "{\"_id\" : \"7\", \"name\":\"doc7\",\"item\":\"fruit\",\"other_Info\":\
    [{\"school\":\"BX\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static constexpr const char *DOCUMENT8 = "{\"_id\" : \"8\", \"name\":\"doc8\",\"item\":true,\"personInfo\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static constexpr const char *DOCUMENT9 = "{\"_id\" : \"9\", \"name\":\"doc9\",\"item\": true}";
static constexpr const char *DOCUMENT10 = "{\"_id\" : \"10\", \"name\":\"doc10\", \"parent\" : \"kate\"}";
static constexpr const char *DOCUMENT11 = "{\"_id\" : \"11\", \"name\":\"doc11\", \"other\" : \"null\"}";
static constexpr const char *DOCUMENT12 = "{\"_id\" : \"12\", \"name\":\"doc12\",\"other\" : null}";
static constexpr const char *DOCUMENT13 = "{\"_id\" : \"13\", \"name\":\"doc13\",\"item\" : \"shoes\",\"personInfo\":\
    {\"school\":\"AB\", \"age\" : 15}}";
static constexpr const char *DOCUMENT14 = "{\"_id\" : \"14\", \"name\":\"doc14\",\"item\" : true,\"personInfo\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 85}]}";
static constexpr const char *DOCUMENT15 = "{\"_id\" : \"15\", \"name\":\"doc15\",\"personInfo\":\
    [{\"school\":\"C\", \"age\" : 5}]}";
static constexpr const char *DOCUMENT16 = "{\"_id\" : \"16\", \"name\":\"doc16\", \"nested1\":{\"nested2\":\
    {\"nested3\":{\"nested4\":\"ABC\", \"field2\":\"CCC\"}}}}";
static constexpr const char *DOCUMENT17 = "{\"_id\" : \"17\", \"name\":\"doc17\",\"personInfo\":\"oh,ok\"}";
static constexpr const char *DOCUMENT18 = "{\"_id\" : \"18\", \"name\":\"doc18\",\"item\" : \"mobile phone\",\
    \"personInfo\":{\"school\":\"DD\", \"age\":66}, \"color\":\"blue\"}";
static constexpr const char *DOCUMENT19 = "{\"_id\" : \"19\", \"name\":\"doc19\",\"ITEM\" : true,\"PERSONINFO\":\
    {\"school\":\"AB\", \"age\":15}}";
static constexpr const char *DOCUMENT20 = "{\"_id\" : \"20\", \"name\":\"doc20\",\"ITEM\" : true,\"personInfo\":\
    [{\"SCHOOL\":\"B\", \"AGE\":15}, {\"SCHOOL\":\"C\", \"AGE\":35}]}";
static constexpr const char *DOCUMENT23 = "{\"_id\" : \"23\", \"name\":\"doc22\",\"ITEM\" : "
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

void UpdateDocOneFuzz(std::string s, const std::string &input)
{
    std::string inputJson = "{\"field5\": \"" + s + "\"}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"name\":\"doc6\"}", inputJson.c_str(), 1);
    inputJson = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"name\":\"doc7\"}", inputJson.c_str(), 1);
    inputJson = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"name\":\"doc8\"}", inputJson.c_str(), 1);

    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, inputJson.c_str(), "{}", "{}", 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, input.c_str(), "{}", 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{}", input.c_str(), 0);
    GRD_UpdateDoc(nullptr, COLLECTION_NAME, "{}", "{}", 0);
    inputJson = "{\"_id\":\"2\", \"field\":" + input + "}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field\":" + input + "}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    s.clear();
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }
    inputJson = "{\"field5\": \"" + s + "\"}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"name\":\"doc6\"}", "{\"c0.c1\":false}", 1);
    inputJson = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"name\":\"doc7\"}", "{\"c0.c1\":null}", 1);
    inputJson = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"name\":\"doc8\"}", "{\"c0.0\":\"ac\"}", 1);
}

void UpdateDocTwoFuzz(const char *newCollName, std::string s, const std::string &input)
{
    std::string inputJson = "{\"field5\": \"" + s + "\"}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_UpdateDoc(g_db, newCollName, "{\"name\":\"doc6\"}", inputJson.c_str(), 1);
    inputJson = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_UpdateDoc(g_db, newCollName, "{\"name\":\"doc7\"}", inputJson.c_str(), 1);
    inputJson = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_UpdateDoc(g_db, newCollName, "{\"name\":\"doc8\"}", inputJson.c_str(), 1);

    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, inputJson.c_str(), "{}", "{}", 0);
    GRD_UpdateDoc(g_db, newCollName, input.c_str(), "{}", 0);
    GRD_UpdateDoc(g_db, newCollName, "{}", input.c_str(), 0);
    GRD_UpdateDoc(nullptr, newCollName, "{}", "{}", 0);
    inputJson = "{\"_id\":\"2\", \"field\":" + input + "}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field\":" + input + "}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    s.clear();
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }
    inputJson = "{\"field5\": \"" + s + "\"}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_InsertDoc(g_db, newCollName, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, newCollName, "{\"name\":\"doc6\"}", "{\"c0.c1\":false}", 1);
    inputJson = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_InsertDoc(g_db, newCollName, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, newCollName, "{\"name\":\"doc7\"}", "{\"c0.c1\":null}", 1);
    inputJson = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_InsertDoc(g_db, newCollName, inputJson.c_str(), 0);
    GRD_UpdateDoc(g_db, newCollName, "{\"name\":\"doc8\"}", "{\"c0.0\":\"ac\"}", 1);
}

void UpdateDocFilterFuzz()
{
    const char *filter = "{\"_id\" : \"1\"}";
    const char *updata2 = "{\"objectInfo.child.child\" : {\"child\":{\"child\":null}}}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, filter, updata2, 0);

    const char *filter1 = "{\"_id\" : \"1\"}";
    const char *updata1 = "{\"_id\" : \"6\"}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, filter1, updata1, 0);

    const char *filter2 = "{\"_id\" : \"1\"}";
    const char *updata3 = "{\"age$\" : \"21\"}";
    const char *updata4 = "{\"bonus..traffic\" : 100}";
    const char *updata5 = "{\"0item\" : 100}";
    const char *updata6 = "{\"item\" : 1.79769313486232e308}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, filter2, updata3, 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, filter2, updata4, 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, filter2, updata5, 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, filter2, updata6, 0);

    const char *filter3 = "{\"_id\" : \"1\"}";
    const char *updata7 = "{\"age\" : 21}";
    GRD_UpdateDoc(g_db, NULL, filter3, updata7, 0);
    GRD_UpdateDoc(g_db, "", filter3, updata7, 0);
    GRD_UpdateDoc(NULL, COLLECTION_NAME, filter3, updata7, 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, NULL, updata7, 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, filter3, NULL, 0);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
}

void UpdateDocFuzz(FuzzedDataProvider &provider)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input = provider.ConsumeRandomLengthString();
    std::string inputJson = "{\"_id\":\"2\", \"field\": \"aaa\", "
                            "\"subject\":\"aaaaaaaaaaa\", \"test1\": true, "
                            "\"test2\": null}";

    GRD_UpdateDoc(g_db, inputJson.c_str(), "{}", "{}", 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, input.c_str(), "{}", 0);
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{}", input.c_str(), 0);
    GRD_UpdateDoc(nullptr, COLLECTION_NAME, "{}", "{}", 0);
    inputJson = "{\"_id\":\"2\", \"field\":" + input + "}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field\":" + input + "}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpdateDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    std::string s;
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }
    UpdateDocOneFuzz(s, input);

    const char *newCollName = "./student";
    GRD_UpdateDoc(g_db, inputJson.c_str(), "{}", "{}", 0);
    GRD_UpdateDoc(g_db, newCollName, input.c_str(), "{}", 0);
    GRD_UpdateDoc(g_db, newCollName, "{}", input.c_str(), 0);
    GRD_UpdateDoc(nullptr, newCollName, "{}", "{}", 0);
    inputJson = "{\"_id\":\"2\", \"field\":" + input + "}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field\":" + input + "}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    inputJson = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpdateDoc(g_db, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    s.clear();
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }

    UpdateDocTwoFuzz(newCollName, s, input);
    UpdateDocFilterFuzz();
}

void UpsertDocNewFuzz(const std::string &input, GRD_DB *db1)
{
    GRD_CreateCollection(g_db, "student", "", 0);
    std::string documentNew =
        "{\"name\": {\"first\":[\"XXX\", \"BBB\", \"CCC\"], \"last\":\"moray\", \"field\":" + input + "}";
    std::string updateDocNew = R""({"name.last.AA.B":"Mnado"})"";
    GRD_UpsertDoc(g_db, "student", R""({"_id":"10001"})"", documentNew.c_str(), 0);
    GRD_UpsertDoc(db1, "student", R""({"_id":"10001"})"", updateDocNew.c_str(), 1);
    GRD_DropCollection(g_db, "student", 0);

    GRD_CreateCollection(g_db, "student", "", 0);
    documentNew = "{\"name\":[\"Tmn\", \"BB\", \"Alice\"], \"field\":" + input + "}";
    updateDocNew = R""({"name.first":"GG"})"";
    GRD_UpsertDoc(g_db, "student", R""({"_id":"10002"})"", documentNew.c_str(), 0);
    GRD_UpsertDoc(db1, "student", R""({"_id":"10002"})"", updateDocNew.c_str(), 1);
}

void UpsertDocFuzz(FuzzedDataProvider &provider)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input = provider.ConsumeRandomLengthString();
    std::string inputJsonNoId = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    std::string inputJson = "{\"_id\":\"1\", \"field\": " + input + "}";

    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    GRD_UpsertDoc(g_db, input.c_str(), "{}", "{}", 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), "{}", 0);
    inputJson = "{\"_id\":\"1\", \"field\":" + input + "}";
    GRD_UpsertDoc(g_db, COLLECTION_NAME, "{\"_id\":\"1\"}", inputJson.c_str(), 0);
    inputJson = "{\"_id\":\"1\", \"field\":\"" + input + "\"}";
    GRD_UpsertDoc(g_db, COLLECTION_NAME, "{\"_id\":\"1\"}", inputJson.c_str(), 0);
    inputJson = "{\"field\":" + input + "}";
    GRD_UpsertDoc(g_db, COLLECTION_NAME, "{\"_id\":\"1\"}", inputJson.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, "{\"_id\":\"1\"}", inputJson.c_str(), 1);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 1);
    GRD_UpsertDoc(nullptr, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), 1);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, nullptr, inputJson.c_str(), 1);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, "{\"_id\":\"2\"}", inputJson.c_str(), BATCH_COUNT);

    const char *newCollName = "./student";
    GRD_DB *db1 = nullptr;
    GRD_CreateCollection(db1, newCollName, "", 0);
    GRD_InsertDoc(db1, newCollName, inputJson.c_str(), 0);
    GRD_UpsertDoc(db1, input.c_str(), "{}", "{}", 0);
    GRD_UpsertDoc(db1, newCollName, inputJson.c_str(), "{}", 0);
    inputJson = "{\"_id\":\"1\", \"field\":" + input + "}";
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"1\"}", inputJson.c_str(), 0);
    inputJson = "{\"_id\":\"1\", \"field\":\"" + input + "\"}";
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"1\"}", inputJson.c_str(), 0);
    inputJson = "{\"field\":" + input + "}";
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"1\"}", inputJson.c_str(), 0);
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 0);
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"1\"}", inputJson.c_str(), 1);
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 1);
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"newID1999\"}", inputJsonNoId.c_str(), 0);
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"newID1999a\"}", inputJsonNoId.c_str(), 1);
    GRD_UpsertDoc(nullptr, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), 1);
    GRD_UpsertDoc(db1, newCollName, nullptr, inputJson.c_str(), 1);
    GRD_UpsertDoc(db1, newCollName, "{\"_id\":\"2\"}", inputJson.c_str(), BATCH_COUNT);
    GRD_DropCollection(db1, newCollName, 0);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);

    UpsertDocNewFuzz(input, db1);
}

void DeleteDocResultFuzz(const std::string &input)
{
    const char *filter = "{\"age\" : 15}";
    GRD_DeleteDoc(g_db, COLLECTION_NAME, filter, 0);
    const char *filter1 = "{\"_id\" : \"1\", \"age\" : 15}";
    GRD_DeleteDoc(g_db, COLLECTION_NAME, filter1, 0);
    std::string filter2 = "{\"_id\" : \"1\", \"name\" : " + input + "}";
    GRD_DeleteDoc(g_db, COLLECTION_NAME, filter2.c_str(), 1);
    GRD_DeleteDoc(g_db, NULL, filter2.c_str(), 0);
    GRD_DeleteDoc(g_db, "", filter2.c_str(), 1);
    GRD_DeleteDoc(g_db, COLLECTION_NAME, filter2.c_str(), 0);

    const char *filter3 = "{\"_id\" : \"1\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter3, "{}" };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_DeleteDoc(g_db, COLLECTION_NAME, filter3, 0);

    const char *filter4 = "{\"_id\" : \"1\"}";
    std::string collectionName1(LESS_HALF_BYTES, 'a');
    GRD_CreateCollection(g_db, collectionName1.c_str(), "", 0);
    GRD_DeleteDoc(g_db, collectionName1.c_str(), filter4, 0);
    GRD_DropCollection(g_db, collectionName1.c_str(), 0);
    GRD_FreeResultSet(resultSet);

    GRD_DeleteDoc(NULL, COLLECTION_NAME, filter2.c_str(), 0);
    GRD_DeleteDoc(g_db, NULL, filter2.c_str(), 0);
    GRD_DeleteDoc(g_db, "", filter2.c_str(), 0);
    GRD_DeleteDoc(g_db, COLLECTION_NAME, NULL, 0);
    GRD_DeleteDoc(g_db, COLLECTION_NAME, "", 0);
    GRD_DeleteDoc(g_db, "notExisted", filter2.c_str(), 0);

    std::vector<std::string> filterVec = { R"({"_id" : 1})", R"({"_id":[1, 2]})", R"({"_id" : {"t1" : 1}})",
        R"({"_id":null})", R"({"_id":true})", R"({"_id" : 1.333})", R"({"_id" : -2.0})" };
    for (const auto &item : filterVec) {
        GRD_DeleteDoc(g_db, COLLECTION_NAME, item.c_str(), 0);
    }
    GRD_DeleteDoc(g_db, COLLECTION_NAME, filter2.c_str(), 0);

    std::string filter5 = "{\"subject.info\" : \"" + input + "\"}";
    resultSet = nullptr;
    query = { filter2.c_str(), "{}" };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_DeleteDoc(g_db, COLLECTION_NAME, filter5.c_str(), 0);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    GRD_FreeResultSet(resultSet);
}

void DeleteDocFuzz(FuzzedDataProvider &provider)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input = provider.ConsumeRandomLengthString();
    std::string inputJson = "{\"field\":" + input + "}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    GRD_DeleteDoc(g_db, input.c_str(), "{}", 0);
    GRD_DeleteDoc(g_db, nullptr, "{}", 0);
    std::string longName = "";
    for (int i = 0; i < MORE_HALF_BYTES; i++) {
        longName += 'a';
    }
    GRD_DeleteDoc(g_db, "grd_name", "{}", 0);
    GRD_DeleteDoc(g_db, longName.c_str(), "{}", 0);
    GRD_DeleteDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    inputJson = "{\"field1\":" + input + "}";
    GRD_DeleteDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    inputJson = "{\"field\":\"" + input + "\"}";
    GRD_DeleteDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    inputJson = "{\"field1\":\"" + input + "\"}";
    GRD_DeleteDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);

    const char *document1 = "{  \
        \"_id\" : \"1\", \
        \"name\": \"xiaoming\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"math\", \"English\", \"music\", {\"info\" : \"exam\"}] \
    }";
    const char *document2 = "{  \
        \"_id\" : \"2\", \
        \"name\": \"ori\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"math\", \"English\", \"music\"] \
    }";
    const char *document3 = "{  \
        \"_id\" : \"3\", \
        \"name\": \"David\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"Sing\", \"Jump\", \"Rap\", \"BasketBall\"] \
    }";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document1, 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, document2, 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, document3, 0);
    GRD_DeleteDoc(g_db, COLLECTION_NAME, "{}", 0);

    DeleteDocResultFuzz(input);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" {
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetUpTestCase();
    FuzzedDataProvider provider(data, size);
    OHOS::UpdateDocFuzz(provider);
    OHOS::UpsertDocFuzz(provider);
    OHOS::DeleteDocFuzz(provider);

    OHOS::TearDownTestCase();
    return 0;
}
}
