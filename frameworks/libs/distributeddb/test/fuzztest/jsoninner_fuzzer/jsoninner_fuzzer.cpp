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
#include "jsoninner_fuzzer.h"

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
const int NUM_THREE = 3;
const int HALF_HALF_BYTES = 256;
const int MORE_HALF_HALF_BYTES = 257;
const int LESS_HALF_BYTES = 511;
const int HALF_BYTES = 512;
const int LESS_MIDDLE_SIZE = 899;
const int MIDDLE_SIZE = 900;
const int MAX_SIZE_NUM = 1000;
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

void InsertDocOneFuzz(const std::string &documentData)
{
    std::string document2 = "{\"_id\":2,\"field\":\"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document2.c_str(), 0);
    std::string document3 = "{\"_id\":true,\"field\":\"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document3.c_str(), 0);
    std::string document4 = "{\"_id\" : null, \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document4.c_str(), 0);
    std::string document5 = "{\"_id\" : [\"2\"], \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document5.c_str(), 0);
    std::string document6 = "{\"_id\" : {\"val\" : \"2\"}, \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document6.c_str(), 0);
    std::string document7 = "{\"_id\" : \"3\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document7.c_str(), 0);
    std::string document9 = "{\"_id\" : \"4\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(NULL, COLLECTION_NAME, document9.c_str(), 0);
    std::string document10 = "{\"_id\" : \"5\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, NULL, document10.c_str(), 0);
    std::string document12 = "{\"_id\" : \"6\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document12.c_str(), 1);
    std::string document13 = "{\"_id\" : \"7\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document13.c_str(), INT_MAX);
    std::string document14 = "{\"_id\" : \"8\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document14.c_str(), INT_MIN);
    GRD_InsertDocInner(g_db, NULL, NULL, 0);
    std::string document15 = "{\"_id\" : \"9\", \"name\" : \"" + documentData + "\"}";
    std::string collectionName2(HALF_BYTES, 'a');
    GRD_InsertDocInner(g_db, collectionName2.c_str(), document15.c_str(), 0);
    const char *collectionName = "collection@!#";
    std::string document16 = "{\"_id\" : \"10\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, collectionName, document16.c_str(), GRD_OK);
    std::string collectionName1(MORE_HALF_HALF_BYTES, 'k');
    std::string document17 = "{\"_id\" : \"10\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, collectionName1.c_str(), document17.c_str(), GRD_OK);
    std::string document18 = "({\"level1\" : {\"level2\" : {\"level3\" : {\"level4\": {\"level5\" : 1}},\
        \"level3_2\" : \"" +
        documentData + "\"}},\"_id\":\"14\"})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document18.c_str(), 0);
    std::string document19 = "({\"level1\" : {\"level2\" : {\"level3\" : [{ \"level5\" : \"" + documentData + "\",\
        \"level5_2\":\"" +
        documentData + "\"}, \"" + documentData + "\",\"" + documentData + "\"], \"level3_2\":\
        \"" +
        documentData + "\"}}, \"_id\":\"14\"})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document19.c_str(), 0);
    std::string document20 = "({\"level1\" : {\"level2\" : {\"level3\" : { \"level4\" : \"" + documentData + "\"},\
        \"level3_2\" : \"" +
        documentData + "\"}}, \"_id\":\"14\"})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document20.c_str(), 0);
}

void InsertDocTwoFuzz(const std::string &documentData)
{
    std::string documentPart1 = "{ \"_id\" : \"15\", \"textVal\" : \" ";
    std::string documentPart2 = "\" }";
    std::string jsonVal = std::string(HALF_BYTES * ONE_BYTES - documentPart1.size() - documentPart2.size(), 'k');
    std::string document = documentPart1 + jsonVal + documentPart2;
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document.c_str(), 0);
    std::string jsonVal2 = std::string(HALF_BYTES * ONE_BYTES - 1 - documentPart1.size() - documentPart2.size(), 'k');
    std::string document21 = documentPart1 + jsonVal2 + documentPart2;
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document21.c_str(), 0);
    std::string document22 = "{\"_id\" : \"16\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document22.c_str(), 0);
    std::string document23 =
        "{\"_id\" : \"17\", \"level1\" : {\"level2\" : {\"level3\" : {\"level4\" : " + documentData + "\
        } } }, \"level1_2\" : \"" +
        documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document23.c_str(), 0);
    std::string document24 = documentData;
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document24.c_str(), 0);
    std::string document25 = "{\"name\" : \"" + documentData + "\", \"age\" : " + documentData + ",\
                            \"friend\" : {\"name\" : \" " +
        documentData + "\"}, \"_id\" : \"19\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document25.c_str(), 0);
    std::string collectionName3 = std::string(HALF_HALF_BYTES, 'k');
    std::string document26 = "{\"_id\" : \"22\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, collectionName3.c_str(), document26.c_str(), GRD_OK);
    std::string collectionName4 = "Aads_sd__23Asb_";
    std::string document27 = "{\"_id\" : \"23\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, collectionName4.c_str(), document27.c_str(), GRD_OK);
    std::string collectionName5 = "GRD_collectionName";
    std::string document28 = "{\"_id\" : \"24\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollectionInner(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDocInner(g_db, collectionName5.c_str(), document28.c_str(), 0);
    collectionName5 = "GM_SYS__collectionName";
    std::string document29 = "{\"_id\" : \"24_2\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollectionInner(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDocInner(g_db, collectionName5.c_str(), document29.c_str(), 0);

    collectionName5 = "grd_collectionName";
    std::string document30 = "{\"_id\" : \"24_3\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollectionInner(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDocInner(g_db, collectionName5.c_str(), document30.c_str(), 0);
}

void InsertDocThreeFuzz(const std::string &documentData)
{
    std::string collectionName5 = "gm_sys_collectionName";
    std::string document31 = "{\"_id\" : \"24_4\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollectionInner(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDocInner(g_db, collectionName5.c_str(), document31.c_str(), 0);

    collectionName5 = "gM_sYs_collectionName";
    std::string document32 = "{\"_id\" : \"24_5\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollectionInner(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDocInner(g_db, collectionName5.c_str(), document32.c_str(), 0);

    collectionName5 = "gRd_collectionName";
    std::string document33 = "{\"_id\" : \"24_6\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollectionInner(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDocInner(g_db, collectionName5.c_str(), document33.c_str(), 0);

    collectionName5 = "gRd@collectionName";
    std::string document34 = "{\"_id\" : \"24_7\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollectionInner(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDocInner(g_db, collectionName5.c_str(), document34.c_str(), 0);

    std::string document35 = "{\"_id\" : \"25_0\", \"level1\" : {\"level2\" : {\"level3\" :\
        {\"level4\" : \"" +
        documentData + "\"}}} , \"level1_2\" : \"" + documentData + "\" }";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document35.c_str(), 0);

    std::string document36 = "{\"_id\" : \"25_1\", \"class_name\" : \"" + documentData +
        "\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"" +
        documentData + "\", \"age\" : " + documentData + ", \"sex\" : \"male\"}, \
        { \"newName1\" : [\"" +
        documentData + "\", \"" + documentData + "\", 0, \"" + documentData + "\"] }]}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document36.c_str(), 0);

    std::string document37 = "{\"_id\" : \"25_2\", \"class_name\" : \"" + documentData +
        "\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"" +
        documentData + "\", \"age\" : " + documentData + ", \"sex\" : \"male\"}, \
        [\"" +
        documentData + "\", \"" + documentData + "\", 0, \"" + documentData + "\", {\"0ab\" : null}]]}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document37.c_str(), 0);

    std::string document38 = "{\"_id\" : \"25_3\", \"class_name\" : \"" + documentData +
        "\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"" +
        documentData + "\", \"age\" : " + documentData + ", \"sex\" : \"male\"}, \
        { \"newName1\" : [\"" +
        documentData + "\", \"" + documentData + "\", 0, \"" + documentData + "\", {\"level5\" : 1}] }]}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document38.c_str(), 0);
}

void InsertDocFourFuzz(const std::string &longId, const std::string &documentData)
{
    std::string document39 =
        "{\"_id\" : \"35\", \"A_aBdk_324_\" : \"" + documentData + "\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document39.c_str(), 0);

    std::string document40 = "{\"_id\" : \"35_2\", \"1A_aBdk_324_\" : \"" + documentData +
        "\", "
        "\"name\" : \"" +
        documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document40.c_str(), 0);
    std::string document41 = "{\"_id\" : \"36_0\", \"stringType\" : \"" + documentData +
        "\", \"numType\" : " + documentData + ", \"BoolType\" : true,\
                          \"nullType\" : null, \"arrayType\" : " +
        documentData + ", \"objectType\" : {\"A\" : 3}}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document41.c_str(), 0);

    std::string document42 = "({\"_id\" : \"38_0\", \"field2\" : 1.797693" + longId + "13486232e308})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document42.c_str(), 0);
    std::string document43 = "({\"_id\" : \"38_1\", \"t1\" : {\"field2\" : 1.797" + longId + "69313486232e308}})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document43.c_str(), 0);
    std::string document44 = "({\"_id\" : \"38_2\", \"t1\" : [1, 2, 1.79769313486" + longId + "232e308]})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document44.c_str(), 0);
    std::string document45 = "({\"_id\" : \"38_3\", \"t1\" : [1, 2, -1.7976931348623167" + longId + "E+308]})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document45.c_str(), 0);
    std::string document46 = "({\"_id\" : \"38_4\", \"t1\" : [1, 2, -1." + longId + "79769313486231570E+308]})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document46.c_str(), 0);
    std::string document47 = "({\"_id\" : \"38_5\", \"t1\" : [1, 2, 1.79769313486" + longId + "231570E+308]})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document47.c_str(), 0);

    std::string doc1 = "{\"_id\" : ";
    std::string doc2 = "\"";
    std::string doc4 = "\"";
    std::string doc5 = ", \"name\" : \"" + documentData + "\"}";
    std::string documentMiddle(MIDDLE_SIZE, 'k');
    std::string document48 = doc1 + doc2 + documentMiddle + doc4 + doc5;
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document48.c_str(), 0);
    std::string documentMiddle2(MAX_SIZE_NUM, 'k');
    document48 = doc1 + doc2 + documentMiddle2 + doc4 + doc5;
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document48.c_str(), 0);

    std::string document49 = "({\"_id\":\"0123\", \"num\":\"" + documentData + "\"})";
    std::string document50 = "({\"_id\":\"0123\", \"NUM\":\"" + documentData + "\"})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document49.c_str(), 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document50.c_str(), 0);

    std::string document51 = "({\"_id\":\"0123\", \"num.\":\"" + documentData + "\"})";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document51.c_str(), 0);

    const char *document52 = R""({})"";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document52, 0);
}

void InsertDocFuzz(FuzzedDataProvider &provider)
{
    std::string collectionNameData = provider.ConsumeRandomLengthString();
    const char *collectionNameVal = collectionNameData.data();
    std::string optionStrData = provider.ConsumeRandomLengthString();
    const char *optionStrVal = optionStrData.data();
    GRD_CreateCollectionInner(g_db, collectionNameVal, optionStrVal, 0);
    std::string documentData = provider.ConsumeRandomLengthString();
    const char *documentVal = documentData.data();
    GRD_InsertDocInner(g_db, collectionNameVal, documentVal, 0);
    GRD_InsertDocInner(g_db, collectionNameVal, documentVal, 0);
    std::string stringJson = "{" + documentData + "}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, stringJson.c_str(), 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, stringJson.c_str(), 0);
    std::string stringJson2 = "{\"_id\":\"1\",\"field\":\"" + documentData + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, stringJson2.c_str(), 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, stringJson2.c_str(), 0);
    std::string longId = "1";
    for (int i = 0; i < MAX_SIZE_NUM; i++) {
        longId += "1";
    }
    longId = "{\"_id\":\"" + longId + "\",\"field\":\"field_id\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, longId.c_str(), 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, "{\"_id\":123}", 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, "{\"field1.field2.field&*^&3\":\"anc\"}", 0);
    InsertDocOneFuzz(documentData);
    InsertDocTwoFuzz(documentData);
    InsertDocThreeFuzz(documentData);
    InsertDocFourFuzz(longId, documentData);

    const char *document53 = R""({"empty" : null})"";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document53, 0);
    GRD_DropCollectionInner(g_db, collectionNameVal, 0);
    GRD_DropCollectionInner(g_db, COLLECTION_NAME, 0);

    std::string documentNew1 = "{\"_id\" : ";
    std::string documentNew2 = "\"";
    std::string documentNew3 = "\"";
    std::string documentNew4 = ", \"name\" : \"Ori\"}";
    std::string document = documentNew1 + documentNew2 + documentData + documentNew3 + documentNew4;
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document.c_str(), 0);
}

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

void FindDocResultSetFuzz(const char *collName, const std::string &filter, const std::string &projectionInfo)
{
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, collName, query, 1, &resultSet);
    if (resultSet != nullptr) {
        GRD_NextInner(resultSet);
        char *valueResultSet = nullptr;
        GRD_GetValueInner(resultSet, &valueResultSet);
    }
    GRD_FreeResultSetInner(resultSet);
}

void FindDocWithFlagFuzz(const std::string &filter, const std::string &projectionInfo, int flag)
{
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, flag, &resultSet);
    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);
}

void FindDocNextTwiceFuzz(const std::string &filter, const std::string &projectionInfo)
{
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);
}

void FindDocZeroFuzz(const std::string &input)
{
    std::string filter = "{\"_id\" : \"6\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"_id\" : \"6\", \"name\":\"" + input + "\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"name\":\"" + input + "\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"_id\" : \"" + input + "\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"_id\" : 1}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"_id\" : [\"" + input + "\", 1]}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"_id\" : {\"" + input + "\" : \"1\"}}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"_id\" : true}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    filter = "{\"_id\" : null}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");

    const char *collName1 = "grd_type";
    const char *collName2 = "GM_SYS_sysfff";
    filter = "{\"_id\" : \"1\"}";
    FindDocResultSetFuzz(collName1, filter, R"({})");
    FindDocResultSetFuzz(collName2, filter, R"({})");

    filter = "{\"_id\" : \"100\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");
}

void FindDocOneFuzz()
{
    std::string filter = "{\"_id\" : \"6\"}";
    GRD_ResultSet *resultSet = nullptr;
    GRD_ResultSet *resultSet2 = nullptr;
    Query query = { filter.c_str(), "{}" };
    const char *collectionName = "DocumentDBFindTest024";
    GRD_CreateCollectionInner(g_db, collectionName, "", 0);
    InsertData(g_db, collectionName);
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FindDocInner(g_db, collectionName, query, 1, &resultSet2);

    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);

    GRD_NextInner(resultSet2);
    char *value2 = nullptr;
    GRD_GetValueInner(resultSet2, &value2);
    GRD_FreeValueInner(value2);

    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet2);
    GRD_GetValueInner(resultSet, &value);
    GRD_GetValueInner(resultSet2, &value);
    GRD_FreeResultSetInner(resultSet);
    GRD_FreeResultSetInner(resultSet2);
    GRD_DropCollectionInner(g_db, collectionName, 0);

    filter = "{\"_id\" : \"16\"}";
    resultSet = nullptr;
    std::string projectionInfo = "{\"name\": true, \"nested1.nested2.nested3.nested4\":true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_NextInner(resultSet);
    value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeResultSetInner(resultSet);

    projectionInfo = "{\"name\": true, \"nested1\":{\"nested2\":{\"nested3\":{\"nested4\":true}}}}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeResultSetInner(resultSet);
}

void FindDocTwoFuzz(const std::string &input)
{
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = "{\"name\": 0, \"nested1.nested2.nested3.nested4\":0}";
    std::string filter = "{\"_id\" : \"16\"}";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);

    projectionInfo = "{\"name\": \"" + input + "\", \"nested1.nested2.nested3.nested4\":\"" + input + "\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);

    filter = "{\"_id\" : \"7\"}";
    resultSet = nullptr;
    projectionInfo = "{\"name\": true, \"other_Info\":true, \"non_exist_field\":true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_NextInner(resultSet);
    value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);

    projectionInfo = "{\"name\": true, \"other_Info\":true, \" item \":true}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);
}

void FindDocThreeFuzz(const std::string &input)
{
    std::string filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = "{\"name\": true, \"other_Info.non_exist_field\":true}";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);
    projectionInfo = "{\"name\": true, \"other_Info\":{\"non_exist_field\":true}}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);
    projectionInfo = "{\"name\": true, \"other_Info.0\": true}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);
    filter = "{\"_id\" : \"4\"}";
    resultSet = nullptr;
    projectionInfo = "{\"personInfo\": true, \"personInfo.grade\": true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);

    filter = "{\"_id\" : \"7\"}";
    projectionInfo = "{\"non_exist_field\":true}";
    int flag = 0;
    FindDocWithFlagFuzz(filter, projectionInfo, flag);
    flag = 1;
    FindDocWithFlagFuzz(filter, projectionInfo, flag);
    filter = "{\"_id\" : \"7\"}";
    projectionInfo = "{\"name\":true, \"item\":true}";
    flag = 0;
    FindDocWithFlagFuzz(filter, projectionInfo, flag);
    flag = 1;
    projectionInfo = "{\"name\": " + input + ", \"item\": " + input + "}";
    FindDocWithFlagFuzz(filter, projectionInfo, flag);
    filter = "{\"_id\" : \"7\"}";
    projectionInfo = "{\"name\":false, \"item\":false}";
    flag = 0;
    FindDocWithFlagFuzz(filter, projectionInfo, flag);
    flag = 1;
    projectionInfo = "{\"name\": " + input + ", \"item\": " + input + "}";
    FindDocWithFlagFuzz(filter, projectionInfo, flag);
    filter = "{\"_id\" : \"4\"}";
    projectionInfo = "{\"name\": " + input + ", \"personInfo.grade1\": " + input + ", \
            \"personInfo.shool1\": " +
        input + ", \"personInfo.age1\": " + input + "}";
    flag = 0;
    FindDocWithFlagFuzz(filter, projectionInfo, flag);
    projectionInfo = "{\"name\": false, \"personInfo.grade1\": false, \
            \"personInfo.shool1\": false, \"personInfo.age1\": false}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);
}

void FindDocFourFuzz(const std::string &input)
{
    std::string filter = "{\"_id\" : \"4\"}";
    std::string projectionInfo =
        "{\"name\": " + input + ", \"personInfo.school\": " + input + ", \"personInfo.age\": " + input + "}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);

    filter = "{\"_id\" : \"17\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter.c_str(), "{}" };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, GRD_DOC_ID_DISPLAY, &resultSet);
    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeResultSetInner(resultSet);

    resultSet = nullptr;
    query = { filter.c_str(), "{}" };
    GRD_FindDocInner(g_db, "", query, 0, &resultSet);
    GRD_FindDocInner(g_db, nullptr, query, 0, &resultSet);

    filter = "{\"_id\" : \"4\"}";
    resultSet = nullptr;
    projectionInfo = "{\"name\":" + input + ", \"personInfo\":0, \"item\":" + input + "}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":true, \"personInfo\":0, \"item\":true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":\"\", \"personInfo\":0, \"item\":\"\"}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":false, \"personInfo\":1, \"item\":false";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":false, \"personInfo\":-1.123, \"item\":false";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":false, \"personInfo\":true, \"item\":false";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
}

void FindDocFiveFuzz(const std::string &input)
{
    std::string filter = "{\"_id\" : \"6\"}";
    std::string projectionInfo = "{\"name\":false, \"personInfo\": 0, \"item\":0}";
    int flag = 0;
    FindDocWithFlagFuzz(filter, projectionInfo, flag);

    filter = "{\"_id\" : \"18\"}";
    projectionInfo = "{\"name\":true, \"personInfo.age\": \"\", \"item\":1, \"color\":10, \"nonExist\" : "
                     "-100}";
    flag = 0;
    FindDocWithFlagFuzz(filter, projectionInfo, flag);

    GRD_ResultSet *resultSet = nullptr;
    projectionInfo = "{\"personInfo\":[true, " + input + "]}";
    flag = 1;
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, flag, &resultSet);
    projectionInfo = "{\"personInfo\":null}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, flag, &resultSet);
    resultSet = nullptr;
    projectionInfo = "{\"Name\":true, \"personInfo.age\": \"\", \"item\":" + input + ", \"COLOR\":" + input +
        ", \"nonExist\" : "
        "" +
        input + "}";
    flag = 0;
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, flag, &resultSet);
    GRD_NextInner(resultSet);
    GRD_FreeResultSetInner(resultSet);

    resultSet = nullptr;
    projectionInfo = "{\"Name\":" + input + ", \"personInfo.age\": false, \"personInfo.SCHOOL\": false, \"item\":\
        false, \"COLOR\":false, \"nonExist\" : false}";
    query = { filter.c_str(), projectionInfo.c_str() };
    char *value = nullptr;
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_NextInner(resultSet);
    GRD_FreeResultSetInner(resultSet);

    filter = "{\"_id\" : \"18\"}";
    resultSet = nullptr;
    query = { filter.c_str(), "{}" };
    std::string collectionName1(LESS_HALF_BYTES, 'a');
    GRD_FindDocInner(g_db, collectionName1.c_str(), query, 1, &resultSet);
    GRD_FreeResultSetInner(resultSet);
}

void FindDocSixFuzz(const std::string &input)
{
    std::string collectionName2(HALF_BYTES, 'a');
    std::string filter = "{\"_id\" : \"18\"}";
    Query query = { filter.c_str(), "{}" };
    GRD_ResultSet *resultSet = nullptr;
    GRD_FindDocInner(g_db, collectionName2.c_str(), query, 1, &resultSet);
    GRD_FindDocInner(g_db, "", query, 1, &resultSet);

    resultSet = nullptr;
    std::string projectionInfo = "{}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, NUM_THREE, &resultSet);
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, INT_MAX, &resultSet);
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, INT_MIN, &resultSet);
    GRD_FindDocInner(nullptr, COLLECTION_NAME, query, 0, &resultSet);
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, nullptr);
    query = { nullptr, nullptr };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);

    std::string document1 = "{\"_id\" : ";
    std::string document2 = "\"";
    std::string document4 = "\"";
    std::string document5 = "}";
    std::string documentMiddle(MIDDLE_SIZE, 'k');
    filter = document1 + document2 + documentMiddle + document4 + document5;
    resultSet = nullptr;
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    std::string documentMiddle2(LESS_MIDDLE_SIZE, 'k');
    filter = document1 + document2 + documentMiddle2 + document4 + document5;
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_FreeResultSetInner(resultSet);

    filter = "{\"personInfo\" : {\"school\":\"" + input + "\"}}";
    FindDocNextTwiceFuzz(filter, R"({})");

    projectionInfo = "{\"version\": " + input + "}";
    FindDocNextTwiceFuzz(filter, projectionInfo);

    projectionInfo = "({\"a00001\":" + input + ", \"a00001\":" + input + "})";
    FindDocNextTwiceFuzz(filter, projectionInfo);

    projectionInfo = "({\"abc123_.\":" + input + "})";
    FindDocNextTwiceFuzz(filter, projectionInfo);

    filter = "({\"abc123_.\":" + input + "})";
    FindDocNextTwiceFuzz(filter, projectionInfo);
}

void FindDocSevenFuzz(const std::string &input)
{
    std::string document064 = "{\"_id\" : \"64\", \"a\":" + input + ", \"doc64\" : " + input + "}";
    std::string document063 = "{\"_id\" : \"63\", \"a\":" + input + ", \"doc63\" : " + input + "}";
    std::string document062 = "{\"_id\" : \"62\", \"a\":" + input + ", \"doc62\" : " + input + "}";
    std::string document061 = "{\"_id\" : \"61\", \"a\":" + input + ", \"doc61\" : " + input + "}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document064.c_str(), 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document063.c_str(), 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document062.c_str(), 0);
    GRD_InsertDocInner(g_db, COLLECTION_NAME, document061.c_str(), 0);
    std::string filter = "{\"a\":" + input + "}";
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = R"({})";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_NextInner(resultSet);
    char *value = nullptr;
    GRD_GetValueInner(resultSet, &value);

    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);

    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);

    GRD_NextInner(resultSet);
    GRD_GetValueInner(resultSet, &value);
    GRD_FreeValueInner(value);
    GRD_FreeResultSetInner(resultSet);
}

void FindDocFuzzPlus(const std::string &input)
{
    FindDocZeroFuzz(input);
    std::string filter = "{\"_id\" : \"6\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, R"({})");
    FindDocOneFuzz();
    FindDocTwoFuzz(input);
    FindDocThreeFuzz(input);
    FindDocFourFuzz(input);
    FindDocFiveFuzz(input);
    FindDocSixFuzz(input);
    FindDocSevenFuzz(input);

    std::string document = "{\"a\":" + input + ", \"doc64\" : " + input + "}";
    filter = "{\"b\":" + input + "}";
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDocInner(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    filter = "{\"a\":" + input + "}";
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = R"({})";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_NextInner(resultSet);
    GRD_FreeResultSetInner(resultSet);
    filter = "{\"a\":" + input + "}";
    resultSet = nullptr;
    projectionInfo = R"({})";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_DropCollectionInner(g_db, COLLECTION_NAME, 0);
    GRD_NextInner(resultSet);
    char *valueNew = nullptr;
    GRD_GetValueInner(resultSet, &valueNew);
    GRD_FreeValueInner(valueNew);
    GRD_FreeResultSetInner(resultSet);
    filter = "{\"personInfo\" : {\"school\":" + input + "}" + "}";
    FindDocNextTwiceFuzz(filter, R"({})");
}
} // namespace

void FindDocFuzz(FuzzedDataProvider &provider)
{
    GRD_CreateCollectionInner(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input = provider.ConsumeRandomLengthString();
    std::string inputJson = "{\"field\":\"" + input + "\"}";
    GRD_InsertDocInner(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    Query query = { inputJson.c_str(), inputJson.c_str() };
    GRD_ResultSet *resultSet = nullptr;
    // ResultSet conflict
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSetInner(resultSet);
    resultSet = nullptr;
    size_t size = provider.ConsumeIntegral<size_t>();
    GRD_FindDocInner(g_db, input.c_str(), query, size, &resultSet);
    GRD_FreeResultSetInner(resultSet);
    GRD_FindDocInner(nullptr, input.c_str(), query, 1, &resultSet);
    query.filter = nullptr;
    GRD_FindDocInner(g_db, input.c_str(), query, 1, &resultSet);
    inputJson = "{\"field\": 0, \"field2\":" + input + "}";
    query.filter = "{}";
    query.projection = inputJson.c_str();
    resultSet = nullptr;
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSetInner(resultSet);
    inputJson = "{\"" + input + "\": 0}";
    query.projection = inputJson.c_str();
    resultSet = nullptr;
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSetInner(resultSet);
    inputJson = "{\"field\":[\"aaa\"," + input + "]}";
    resultSet = nullptr;
    query.projection = inputJson.c_str();
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSetInner(resultSet);
    query.filter = "{\"field\": false}";
    GRD_FindDocInner(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSetInner(resultSet);
    FindDocFuzzPlus(input);
    GRD_DropCollectionInner(g_db, COLLECTION_NAME, 0);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" {
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetUpTestCase();
    FuzzedDataProvider provider(data, size);
    OHOS::InsertDocFuzz(provider);
    OHOS::FindDocFuzz(provider);

    OHOS::TearDownTestCase();
    return 0;
}
}
