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
#include "json_fuzzer.h"

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
const char *DB_DIR_PATH = "./data/";
const char *NO_EXIST_COLLECTION_NAME = "no_exisit";
const int NUM_THREE = 3;
const int SMALL_PREFIX_LEN = 5;
const int NUM_FIFTEEN = 15;
const int NUM_NINETY_EIGHT = 98;
const int BATCH_COUNT = 100;
const int HALF_HALF_BYTES = 256;
const int MORE_HALF_HALF_BYTES = 257;
const int LESS_HALF_BYTES = 511;
const int MORE_HALF_BYTES = 513;
const int HALF_BYTES = 512;
const int MAX_LEN_NUM = 600;
const int LESS_MIDDLE_SIZE = 899;
const int MIDDLE_SIZE = 900;
const int MAX_SIZE_NUM = 1000;
const int ONE_BYTES = 1024;
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

std::string getMaxString()
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

void InsertDocOneFuzz(const std::string &documentData)
{
    std::string document2 = "{\"_id\":2,\"field\":\"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document2.c_str(), 0);
    std::string document3 = "{\"_id\":true,\"field\":\"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document3.c_str(), 0);
    std::string document4 = "{\"_id\" : null, \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document4.c_str(), 0);
    std::string document5 = "{\"_id\" : [\"2\"], \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document5.c_str(), 0);
    std::string document6 = "{\"_id\" : {\"val\" : \"2\"}, \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document6.c_str(), 0);
    std::string document7 = "{\"_id\" : \"3\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document7.c_str(), 0);
    std::string document9 = "{\"_id\" : \"4\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(NULL, COLLECTION_NAME, document9.c_str(), 0);
    std::string document10 = "{\"_id\" : \"5\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, NULL, document10.c_str(), 0);
    std::string document12 = "{\"_id\" : \"6\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document12.c_str(), 1);
    std::string document13 = "{\"_id\" : \"7\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document13.c_str(), INT_MAX);
    std::string document14 = "{\"_id\" : \"8\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document14.c_str(), INT_MIN);
    GRD_InsertDoc(g_db, NULL, NULL, 0);
    std::string document15 = "{\"_id\" : \"9\", \"name\" : \"" + documentData + "\"}";
    std::string collectionName2(HALF_BYTES, 'a');
    GRD_InsertDoc(g_db, collectionName2.c_str(), document15.c_str(), 0);
    const char *collectionName = "collection@!#";
    std::string document16 = "{\"_id\" : \"10\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, collectionName, document16.c_str(), GRD_OK);
    std::string collectionName1(MORE_HALF_HALF_BYTES, 'k');
    std::string document17 = "{\"_id\" : \"10\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, collectionName1.c_str(), document17.c_str(), GRD_OK);
    std::string document18 = "({\"level1\" : {\"level2\" : {\"level3\" : {\"level4\": {\"level5\" : 1}},\
        \"level3_2\" : \"" +
        documentData + "\"}},\"_id\":\"14\"})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document18.c_str(), 0);
    std::string document19 = "({\"level1\" : {\"level2\" : {\"level3\" : [{ \"level5\" : \"" + documentData + "\",\
        \"level5_2\":\"" +
        documentData + "\"}, \"" + documentData + "\",\"" + documentData + "\"], \"level3_2\":\
        \"" +
        documentData + "\"}}, \"_id\":\"14\"})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document19.c_str(), 0);
    std::string document20 = "({\"level1\" : {\"level2\" : {\"level3\" : { \"level4\" : \"" + documentData + "\"},\
        \"level3_2\" : \"" +
        documentData + "\"}}, \"_id\":\"14\"})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document20.c_str(), 0);
}

void InsertDocTwoFuzz(const std::string &documentData)
{
    std::string documentPart1 = "{ \"_id\" : \"15\", \"textVal\" : \" ";
    std::string documentPart2 = "\" }";
    std::string jsonVal = std::string(HALF_BYTES * ONE_BYTES - documentPart1.size() - documentPart2.size(), 'k');
    std::string document = documentPart1 + jsonVal + documentPart2;
    GRD_InsertDoc(g_db, COLLECTION_NAME, document.c_str(), 0);
    std::string jsonVal2 = std::string(HALF_BYTES * ONE_BYTES - 1 - documentPart1.size() - documentPart2.size(), 'k');
    std::string document21 = documentPart1 + jsonVal2 + documentPart2;
    GRD_InsertDoc(g_db, COLLECTION_NAME, document21.c_str(), 0);
    std::string document22 = "{\"_id\" : \"16\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document22.c_str(), 0);
    std::string document23 =
        "{\"_id\" : \"17\", \"level1\" : {\"level2\" : {\"level3\" : {\"level4\" : " + documentData + "\
        } } }, \"level1_2\" : \"" +
        documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document23.c_str(), 0);
    std::string document24 = documentData;
    GRD_InsertDoc(g_db, COLLECTION_NAME, document24.c_str(), 0);
    std::string document25 = "{\"name\" : \"" + documentData + "\", \"age\" : " + documentData + ",\
                            \"friend\" : {\"name\" : \" " +
        documentData + "\"}, \"_id\" : \"19\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document25.c_str(), 0);
    std::string collectionName3 = std::string(HALF_HALF_BYTES, 'k');
    std::string document26 = "{\"_id\" : \"22\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, collectionName3.c_str(), document26.c_str(), GRD_OK);
    std::string collectionName4 = "Aads_sd__23Asb_";
    std::string document27 = "{\"_id\" : \"23\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, collectionName4.c_str(), document27.c_str(), GRD_OK);
    std::string collectionName5 = "GRD_collectionName";
    std::string document28 = "{\"_id\" : \"24\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollection(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDoc(g_db, collectionName5.c_str(), document28.c_str(), 0);
    collectionName5 = "GM_SYS__collectionName";
    std::string document29 = "{\"_id\" : \"24_2\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollection(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDoc(g_db, collectionName5.c_str(), document29.c_str(), 0);

    collectionName5 = "grd_collectionName";
    std::string document30 = "{\"_id\" : \"24_3\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollection(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDoc(g_db, collectionName5.c_str(), document30.c_str(), 0);
}

void InsertDocThreeFuzz(const std::string &documentData)
{
    std::string collectionName5 = "gm_sys_collectionName";
    std::string document31 = "{\"_id\" : \"24_4\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollection(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDoc(g_db, collectionName5.c_str(), document31.c_str(), 0);

    collectionName5 = "gM_sYs_collectionName";
    std::string document32 = "{\"_id\" : \"24_5\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollection(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDoc(g_db, collectionName5.c_str(), document32.c_str(), 0);

    collectionName5 = "gRd_collectionName";
    std::string document33 = "{\"_id\" : \"24_6\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollection(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDoc(g_db, collectionName5.c_str(), document33.c_str(), 0);

    collectionName5 = "gRd@collectionName";
    std::string document34 = "{\"_id\" : \"24_7\", \"name\" : \"" + documentData + "\"}";
    GRD_CreateCollection(g_db, collectionName5.c_str(), "", 0);
    GRD_InsertDoc(g_db, collectionName5.c_str(), document34.c_str(), 0);

    std::string document35 = "{\"_id\" : \"25_0\", \"level1\" : {\"level2\" : {\"level3\" :\
        {\"level4\" : \"" +
        documentData + "\"}}} , \"level1_2\" : \"" + documentData + "\" }";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document35.c_str(), 0);

    std::string document36 = "{\"_id\" : \"25_1\", \"class_name\" : \"" + documentData +
        "\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"" +
        documentData + "\", \"age\" : " + documentData + ", \"sex\" : \"male\"}, \
        { \"newName1\" : [\"" +
        documentData + "\", \"" + documentData + "\", 0, \"" + documentData + "\"] }]}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document36.c_str(), 0);

    std::string document37 = "{\"_id\" : \"25_2\", \"class_name\" : \"" + documentData +
        "\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"" +
        documentData + "\", \"age\" : " + documentData + ", \"sex\" : \"male\"}, \
        [\"" +
        documentData + "\", \"" + documentData + "\", 0, \"" + documentData + "\", {\"0ab\" : null}]]}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document37.c_str(), 0);

    std::string document38 = "{\"_id\" : \"25_3\", \"class_name\" : \"" + documentData +
        "\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"" +
        documentData + "\", \"age\" : " + documentData + ", \"sex\" : \"male\"}, \
        { \"newName1\" : [\"" +
        documentData + "\", \"" + documentData + "\", 0, \"" + documentData + "\", {\"level5\" : 1}] }]}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document38.c_str(), 0);
}

void InsertDocFourFuzz(const std::string &longId, const std::string &documentData)
{
    std::string document39 =
        "{\"_id\" : \"35\", \"A_aBdk_324_\" : \"" + documentData + "\", \"name\" : \"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document39.c_str(), 0);

    std::string document40 = "{\"_id\" : \"35_2\", \"1A_aBdk_324_\" : \"" + documentData +
        "\", "
        "\"name\" : \"" +
        documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document40.c_str(), 0);
    std::string document41 = "{\"_id\" : \"36_0\", \"stringType\" : \"" + documentData +
        "\", \"numType\" : " + documentData + ", \"BoolType\" : true,\
                          \"nullType\" : null, \"arrayType\" : " +
        documentData + ", \"objectType\" : {\"A\" : 3}}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document41.c_str(), 0);

    std::string document42 = "({\"_id\" : \"38_0\", \"field2\" : 1.797693" + longId + "13486232e308})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document42.c_str(), 0);
    std::string document43 = "({\"_id\" : \"38_1\", \"t1\" : {\"field2\" : 1.797" + longId + "69313486232e308}})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document43.c_str(), 0);
    std::string document44 = "({\"_id\" : \"38_2\", \"t1\" : [1, 2, 1.79769313486" + longId + "232e308]})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document44.c_str(), 0);
    std::string document45 = "({\"_id\" : \"38_3\", \"t1\" : [1, 2, -1.7976931348623167" + longId + "E+308]})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document45.c_str(), 0);
    std::string document46 = "({\"_id\" : \"38_4\", \"t1\" : [1, 2, -1." + longId + "79769313486231570E+308]})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document46.c_str(), 0);
    std::string document47 = "({\"_id\" : \"38_5\", \"t1\" : [1, 2, 1.79769313486" + longId + "231570E+308]})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document47.c_str(), 0);

    std::string doc1 = "{\"_id\" : ";
    std::string doc2 = "\"";
    std::string doc4 = "\"";
    std::string doc5 = ", \"name\" : \"" + documentData + "\"}";
    std::string documentMiddle(MIDDLE_SIZE, 'k');
    std::string document48 = doc1 + doc2 + documentMiddle + doc4 + doc5;
    GRD_InsertDoc(g_db, COLLECTION_NAME, document48.c_str(), 0);
    std::string documentMiddle2(MAX_SIZE_NUM, 'k');
    document48 = doc1 + doc2 + documentMiddle2 + doc4 + doc5;
    GRD_InsertDoc(g_db, COLLECTION_NAME, document48.c_str(), 0);

    std::string document49 = "({\"_id\":\"0123\", \"num\":\"" + documentData + "\"})";
    std::string document50 = "({\"_id\":\"0123\", \"NUM\":\"" + documentData + "\"})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document49.c_str(), 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, document50.c_str(), 0);

    std::string document51 = "({\"_id\":\"0123\", \"num.\":\"" + documentData + "\"})";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document51.c_str(), 0);

    const char *document52 = R""({})"";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document52, 0);
}

void InsertDocFuzz(const uint8_t *data, size_t size)
{
    std::string collectionNameData(reinterpret_cast<const char *>(data), size);
    const char *collectionNameVal = collectionNameData.data();
    std::string optionStrData(reinterpret_cast<const char *>(data), size);
    const char *optionStrVal = optionStrData.data();
    GRD_CreateCollection(g_db, collectionNameVal, optionStrVal, 0);
    std::string documentData(reinterpret_cast<const char *>(data), size);
    const char *documentVal = documentData.data();
    GRD_InsertDoc(g_db, collectionNameVal, documentVal, 0);
    GRD_InsertDoc(g_db, collectionNameVal, documentVal, 0);
    std::string stringJson = "{" + documentData + "}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, stringJson.c_str(), 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, stringJson.c_str(), 0);
    std::string stringJson2 = "{\"_id\":\"1\",\"field\":\"" + documentData + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, stringJson2.c_str(), 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, stringJson2.c_str(), 0);
    std::string longId = "1";
    for (int i = 0; i < MAX_SIZE_NUM; i++) {
        longId += "1";
    }
    longId = "{\"_id\":\"" + longId + "\",\"field\":\"field_id\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, longId.c_str(), 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, "{\"_id\":123}", 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, "{\"field1.field2.field&*^&3\":\"anc\"}", 0);
    InsertDocOneFuzz(documentData);
    InsertDocTwoFuzz(documentData);
    InsertDocThreeFuzz(documentData);
    InsertDocFourFuzz(longId, documentData);

    const char *document53 = R""({"empty" : null})"";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document53, 0);
    GRD_DropCollection(g_db, collectionNameVal, 0);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);

    std::string documentNew1 = "{\"_id\" : ";
    std::string documentNew2 = "\"";
    std::string documentNew3 = "\"";
    std::string documentNew4 = ", \"name\" : \"Ori\"}";
    std::string document = documentNew1 + documentNew2 + documentData + documentNew3 + documentNew4;
    GRD_InsertDoc(g_db, COLLECTION_NAME, document.c_str(), 0);
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

void FindDocResultSetFuzz(const char *collName, const std::string &filter, const std::string &projectionInfo)
{
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, collName, query, 1, &resultSet);
    if (resultSet != nullptr) {
        GRD_Next(resultSet);
        char *valueResultSet = nullptr;
        GRD_GetValue(resultSet, &valueResultSet);
    }
    GRD_FreeResultSet(resultSet);
}

void FindDocWithFlagFuzz(const std::string &filter, const std::string &projectionInfo, int flag)
{
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet);
    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
}

void FindDocNextTwiceFuzz(const std::string &filter, const std::string &projectionInfo)
{
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
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
    GRD_CreateCollection(g_db, collectionName, "", 0);
    InsertData(g_db, collectionName);
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FindDoc(g_db, collectionName, query, 1, &resultSet2);

    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);

    GRD_Next(resultSet2);
    char *value2 = nullptr;
    GRD_GetValue(resultSet2, &value2);
    GRD_FreeValue(value2);

    GRD_Next(resultSet);
    GRD_Next(resultSet2);
    GRD_GetValue(resultSet, &value);
    GRD_GetValue(resultSet2, &value);
    GRD_FreeResultSet(resultSet);
    GRD_FreeResultSet(resultSet2);
    GRD_DropCollection(g_db, collectionName, 0);

    filter = "{\"_id\" : \"16\"}";
    resultSet = nullptr;
    std::string projectionInfo = "{\"name\": true, \"nested1.nested2.nested3.nested4\":true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_Next(resultSet);
    value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_FreeResultSet(resultSet);

    projectionInfo = "{\"name\": true, \"nested1\":{\"nested2\":{\"nested3\":{\"nested4\":true}}}}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_FreeResultSet(resultSet);
}

void FindDocTwoFuzz(const std::string &input)
{
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = "{\"name\": 0, \"nested1.nested2.nested3.nested4\":0}";
    std::string filter = "{\"_id\" : \"16\"}";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);

    projectionInfo = "{\"name\": \"" + input + "\", \"nested1.nested2.nested3.nested4\":\"" + input + "\"}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);

    filter = "{\"_id\" : \"7\"}";
    resultSet = nullptr;
    projectionInfo = "{\"name\": true, \"other_Info\":true, \"non_exist_field\":true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_Next(resultSet);
    value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);

    projectionInfo = "{\"name\": true, \"other_Info\":true, \" item \":true}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);
}

void FindDocThreeFuzz(const std::string &input)
{
    std::string filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = "{\"name\": true, \"other_Info.non_exist_field\":true}";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
    projectionInfo = "{\"name\": true, \"other_Info\":{\"non_exist_field\":true}}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);
    projectionInfo = "{\"name\": true, \"other_Info.0\": true}";
    FindDocResultSetFuzz(COLLECTION_NAME, filter, projectionInfo);
    filter = "{\"_id\" : \"4\"}";
    resultSet = nullptr;
    projectionInfo = "{\"personInfo\": true, \"personInfo.grade\": true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);

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
    GRD_FindDoc(g_db, COLLECTION_NAME, query, GRD_DOC_ID_DISPLAY, &resultSet);
    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_FreeResultSet(resultSet);

    resultSet = nullptr;
    query = { filter.c_str(), "{}" };
    GRD_FindDoc(g_db, "", query, 0, &resultSet);
    GRD_FindDoc(g_db, nullptr, query, 0, &resultSet);

    filter = "{\"_id\" : \"4\"}";
    resultSet = nullptr;
    projectionInfo = "{\"name\":" + input + ", \"personInfo\":0, \"item\":" + input + "}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":true, \"personInfo\":0, \"item\":true}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":\"\", \"personInfo\":0, \"item\":\"\"}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":false, \"personInfo\":1, \"item\":false";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":false, \"personInfo\":-1.123, \"item\":false";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);

    projectionInfo = "{\"name\":false, \"personInfo\":true, \"item\":false";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
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
    GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet);
    projectionInfo = "{\"personInfo\":null}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet);
    resultSet = nullptr;
    projectionInfo = "{\"Name\":true, \"personInfo.age\": \"\", \"item\":" + input + ", \"COLOR\":" + input +
        ", \"nonExist\" : "
        "" +
        input + "}";
    flag = 0;
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet);
    GRD_Next(resultSet);
    GRD_FreeResultSet(resultSet);

    resultSet = nullptr;
    projectionInfo = "{\"Name\":" + input + ", \"personInfo.age\": false, \"personInfo.SCHOOL\": false, \"item\":\
        false, \"COLOR\":false, \"nonExist\" : false}";
    query = { filter.c_str(), projectionInfo.c_str() };
    char *value = nullptr;
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_Next(resultSet);
    GRD_FreeResultSet(resultSet);

    filter = "{\"_id\" : \"18\"}";
    resultSet = nullptr;
    query = { filter.c_str(), "{}" };
    std::string collectionName1(LESS_HALF_BYTES, 'a');
    GRD_FindDoc(g_db, collectionName1.c_str(), query, 1, &resultSet);
    GRD_FreeResultSet(resultSet);
}

void FindDocSixFuzz(const std::string &input)
{
    std::string collectionName2(HALF_BYTES, 'a');
    std::string filter = "{\"_id\" : \"18\"}";
    Query query = { filter.c_str(), "{}" };
    GRD_ResultSet *resultSet = nullptr;
    GRD_FindDoc(g_db, collectionName2.c_str(), query, 1, &resultSet);
    GRD_FindDoc(g_db, "", query, 1, &resultSet);

    resultSet = nullptr;
    std::string projectionInfo = "{}";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, NUM_THREE, &resultSet);
    GRD_FindDoc(g_db, COLLECTION_NAME, query, INT_MAX, &resultSet);
    GRD_FindDoc(g_db, COLLECTION_NAME, query, INT_MIN, &resultSet);
    GRD_FindDoc(nullptr, COLLECTION_NAME, query, 0, &resultSet);
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, nullptr);
    query = { nullptr, nullptr };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);

    std::string document1 = "{\"_id\" : ";
    std::string document2 = "\"";
    std::string document4 = "\"";
    std::string document5 = "}";
    std::string documentMiddle(MIDDLE_SIZE, 'k');
    filter = document1 + document2 + documentMiddle + document4 + document5;
    resultSet = nullptr;
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    std::string documentMiddle2(LESS_MIDDLE_SIZE, 'k');
    filter = document1 + document2 + documentMiddle2 + document4 + document5;
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_FreeResultSet(resultSet);

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
    GRD_InsertDoc(g_db, COLLECTION_NAME, document064.c_str(), 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, document063.c_str(), 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, document062.c_str(), 0);
    GRD_InsertDoc(g_db, COLLECTION_NAME, document061.c_str(), 0);
    std::string filter = "{\"a\":" + input + "}";
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = R"({})";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);

    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);

    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);

    GRD_Next(resultSet);
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
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
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    GRD_UpsertDoc(g_db, COLLECTION_NAME, filter.c_str(), document.c_str(), 0);
    filter = "{\"a\":" + input + "}";
    GRD_ResultSet *resultSet = nullptr;
    std::string projectionInfo = R"({})";
    Query query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_Next(resultSet);
    GRD_FreeResultSet(resultSet);
    filter = "{\"a\":" + input + "}";
    resultSet = nullptr;
    projectionInfo = R"({})";
    query = { filter.c_str(), projectionInfo.c_str() };
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    GRD_Next(resultSet);
    char *valueNew = nullptr;
    GRD_GetValue(resultSet, &valueNew);
    GRD_FreeValue(valueNew);
    GRD_FreeResultSet(resultSet);
    filter = "{\"personInfo\" : {\"school\":" + input + "}" + "}";
    FindDocNextTwiceFuzz(filter, R"({})");
}
} // namespace

void FindDocFuzz(const uint8_t *data, size_t size)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson = "{\"field\":\"" + input + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    Query query = { inputJson.c_str(), inputJson.c_str() };
    GRD_ResultSet *resultSet = nullptr;
    // ResultSet conflict
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSet(resultSet);
    resultSet = nullptr;
    GRD_FindDoc(g_db, input.c_str(), query, size, &resultSet);
    GRD_FreeResultSet(resultSet);
    GRD_FindDoc(nullptr, input.c_str(), query, 1, &resultSet);
    query.filter = nullptr;
    GRD_FindDoc(g_db, input.c_str(), query, 1, &resultSet);
    inputJson = "{\"field\": 0, \"field2\":" + input + "}";
    query.filter = "{}";
    query.projection = inputJson.c_str();
    resultSet = nullptr;
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSet(resultSet);
    inputJson = "{\"" + input + "\": 0}";
    query.projection = inputJson.c_str();
    resultSet = nullptr;
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSet(resultSet);
    inputJson = "{\"field\":[\"aaa\"," + input + "]}";
    resultSet = nullptr;
    query.projection = inputJson.c_str();
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSet(resultSet);
    query.filter = "{\"field\": false}";
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_FreeResultSet(resultSet);
    FindDocFuzzPlus(input);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
}

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

void UpdateDocFuzz(const uint8_t *data, size_t size)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
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

void UpsertDocFuzz(const uint8_t *data, size_t size)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
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

void DeleteDocFuzz(const uint8_t *data, size_t size)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
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

namespace {
void FindAndRelease(Query query)
{
    GRD_ResultSet *resultSet = nullptr;
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet);
    GRD_Next(resultSet);
    char *value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
    resultSet = nullptr;
    GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet);
    GRD_Next(resultSet);
    value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);

    const char *newCollName = "./student";
    resultSet = nullptr;
    GRD_FindDoc(g_db, newCollName, query, 1, &resultSet);
    GRD_Next(resultSet);
    value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
    resultSet = nullptr;
    GRD_FindDoc(g_db, newCollName, query, 0, &resultSet);
    GRD_Next(resultSet);
    value = nullptr;
    GRD_GetValue(resultSet, &value);
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
}
} // namespace

void FindAndReleaseFuzz(std::string document, std::string filter, Query query, const std::string &input)
{
    GRD_InsertDoc(g_db, COLLECTION_NAME, document.c_str(), 0);
    query.filter = filter.c_str();
    query.projection = "{\"field\": 0}";
    FindAndRelease(query);
    document = "{\"field1.field2\" [false], \"field1.field2.field3\": [3], "
               "\"field1.field4\": [null]}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document.c_str(), 0);
    query.filter = document.c_str();
    query.projection = "{\"field\": 0}";
    FindAndRelease(query);
    query.projection = "{\"field\": 1}";
    FindAndRelease(query);
    query.projection = "{\"field.field2\": 1}";
    FindAndRelease(query);
    document = "{\"field1\": {\"field2\": [{\"field3\":\"" + input + "\"}]}}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, document.c_str(), 0);
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

void NextFuzz(const uint8_t *data, size_t size)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson = "{" + input + "}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, inputJson.c_str(), 0);
    Query query = { inputJson.c_str(), "{}" };
    FindAndRelease(query);
    std::string stringJson2 = "{ \"_id\":\"1\", \"field\":\"" + input + "\"}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, stringJson2.c_str(), 0);
    query.filter = "{\"_id\": \"1\"}";
    FindAndRelease(query);
    std::string filter2 = "{ \"_id\":\"1\", \"field\":\"" + input + " invalid\"}";
    query.filter = filter2.c_str();
    FindAndRelease(query);
    query.filter = "{\"_id\": \"3\"}";
    FindAndRelease(query);
    std::string stringJson3 = "{ \"_id\":\"2\", \"field\": [\"field2\", null, \"abc\", 123]}";
    GRD_InsertDoc(g_db, COLLECTION_NAME, stringJson3.c_str(), 0);
    query.filter = "{\"field\": [\"field2\", null, \"abc\", 123]}";
    FindAndRelease(query);
    std::string document = "{\"field\": [\"" + input + "\",\"" + input + "\",\"" + input + "\"]}";
    std::string filter = "{\"field." + std::to_string(size) + "\":\"" + input + "\"}";
    FindAndReleaseFuzz(document, filter, query, input);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
}

void GetValueFuzz(const uint8_t *data, size_t size)
{
    GRD_CreateCollection(g_db, COLLECTION_NAME, OPTION_STR, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
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

void DbOpenCloseFuzz(const char *dbFileVal, const char *configStr, GRD_DB *dbVal)
{
    int ret = GRD_DBOpen(dbFileVal, configStr, GRD_DB_OPEN_CREATE, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBClose(dbVal, GRD_DB_CLOSE);
    }
}

void DbOpenOneFuzz(GRD_DB *dbVal)
{
    std::string path = "./documentFuzz.db";
    int ret = GRD_DBOpen(path.c_str(), "", GRD_DB_OPEN_ONLY, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBClose(dbVal, GRD_DB_CLOSE);
    }
    (void)remove(path.c_str());

    path = "./document.db";
    DbOpenCloseFuzz(path.c_str(), R""({"pageSize":64, "redopubbufsize":4033})"", dbVal);
    DbOpenCloseFuzz(path.c_str(), R""({"pageSize":64, "redopubbufsize":4032})"", dbVal);
    DbOpenCloseFuzz(path.c_str(), R""({"redopubbufsize":255})"", dbVal);
    DbOpenCloseFuzz(path.c_str(), R""({"redopubbufsize":16385})"", dbVal);
}

void DbOpenFuzz(const uint8_t *data, size_t size)
{
    std::string dbFileData(reinterpret_cast<const char *>(data), size);
    std::string realDbFileData = DB_DIR_PATH + dbFileData;
    const char *dbFileVal = realDbFileData.data();
    std::string configStrData(reinterpret_cast<const char *>(data), size);
    const char *configStrVal = configStrData.data();
    GRD_DB *dbVal = nullptr;

    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    std::string stringMax = getMaxString();
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

    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    std::string fieldStringValueAll = "{\"bufferPoolSize\":\"8192\",\"redoFlushBtTrx\":\"1\",\"redoBufSize\":"
                                        "\"16384\",\"maxConnNum\":\"1024\"}";
    const char *configStrAll = fieldStringValueAll.data();
    DbOpenCloseFuzz(dbFileVal, configStrAll, dbVal);

    std::string fieldLowNumber = "{\"bufferPoolSize\": 102}";
    const char *configStrLowNumber = fieldLowNumber.data();
    DbOpenCloseFuzz(dbFileVal, configStrLowNumber, dbVal);

    int ret = GRD_DBOpen(nullptr, configStrVal, GRD_DB_OPEN_CREATE, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBClose(nullptr, GRD_DB_CLOSE);
    }
    DbOpenCloseFuzz(dbFileVal, configStrVal, dbVal);

    DbOpenOneFuzz(dbVal);
}

void DbCloseFuzz(const uint8_t *data, size_t size)
{
    std::string dbFileData(reinterpret_cast<const char *>(data), size);
    std::string realDbFileData = DB_DIR_PATH + dbFileData;
    const char *dbFileVal = realDbFileData.data();
    std::string configStrData(reinterpret_cast<const char *>(data), size);
    const char *configStrVal = configStrData.data();
    GRD_DB *dbVal = nullptr;
    DbOpenCloseFuzz(dbFileVal, configStrVal, dbVal);
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

void CreateCollectionFuzz(const uint8_t *data, size_t size)
{
    std::string collectionNameData(reinterpret_cast<const char *>(data), size);
    const char *collectionNameVal = collectionNameData.data();
    std::string optionStrData(reinterpret_cast<const char *>(data), size);
    const char *optionStrVal = optionStrData.data();
    GRD_CreateCollection(nullptr, collectionNameVal, optionStrVal, 0);
    GRD_CreateCollection(g_db, collectionNameVal, optionStrVal, 0);
    const char *optionStr = nullptr;
    GRD_CreateCollection(g_db, COLLECTION_NAME, optionStr, 0);
    GRD_CreateCollection(g_db, COLLECTION_NAME, "{\"maxdoc\":5, \"unexpected_max_doc\":32}", 0);
    GRD_CreateCollection(g_db, COLLECTION_NAME, "{}", 0);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    std::string optStr = "{\"maxdoc\":" + optionStrData + "}";
    GRD_CreateCollection(g_db, COLLECTION_NAME, optStr.c_str(), 0);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    GRD_CreateCollection(g_db, COLLECTION_NAME, optStr.c_str(), MAX_SIZE_NUM);
    optStr = "{\"maxdoc\": 5}";
    GRD_CreateCollection(g_db, COLLECTION_NAME, optStr.c_str(), 0);
    GRD_CreateCollection(g_db, COLLECTION_NAME, optStr.c_str(), 1);
    GRD_DropCollection(g_db, COLLECTION_NAME, 0);
    GRD_DropCollection(g_db, COLLECTION_NAME, MAX_SIZE_NUM);
    GRD_DropCollection(g_db, collectionNameVal, 0);
}

void DropCollectionFuzz(const uint8_t *data, size_t size)
{
    std::string collectionNameData(reinterpret_cast<const char *>(data), size);
    const char *collectionNameVal = collectionNameData.data();
    std::string optionStrData(reinterpret_cast<const char *>(data), size);
    const char *optionStrVal = optionStrData.data();
    GRD_CreateCollection(g_db, collectionNameVal, optionStrVal, 0);
    GRD_DropCollection(nullptr, collectionNameVal, 0);
    GRD_DropCollection(g_db, collectionNameVal, 0);
}

void DbFlushFuzz(const uint8_t *data, size_t size)
{
    GRD_DB *db = nullptr;
    GRD_DB *db2 = nullptr;
    const uint32_t flags = reinterpret_cast<const uint32_t>(data);
    int ret = GRD_DBOpen(TEST_DB_FILE, CONFIG_STR, GRD_DB_OPEN_CREATE, &db);
    if (ret == GRD_OK) {
        GRD_Flush(db, flags);
        GRD_DBClose(db, GRD_DB_CLOSE);
    }
    GRD_Flush(db2, flags);
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

    uint32_t begin = 0, end = MAX_SIZE_NUM;
    GRD_FilterOptionT option = {};
    option.mode = KV_SCAN_RANGE;
    option.begin = { &begin, sizeof(uint32_t) };
    option.end = { &end, sizeof(uint32_t) };
    GRD_ResultSet *resultSet = nullptr;
    GRD_KVFilter(g_db, COLLECTION_NAME, &option, &resultSet);

    uint32_t keySize, valueSize;
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

    OHOS::DbOpenFuzz(data, size);
    OHOS::CreateCollectionFuzz(data, size);
    OHOS::DropCollectionFuzz(data, size);
    OHOS::InsertDocFuzz(data, size);
    OHOS::FindDocFuzz(data, size);
    OHOS::UpdateDocFuzz(data, size);
    OHOS::UpsertDocFuzz(data, size);
    OHOS::DeleteDocFuzz(data, size);
    OHOS::NextFuzz(data, size);
    OHOS::GetValueFuzz(data, size);
    OHOS::FreeResultSetFuzz();
    OHOS::TestGrdDbApGrdGetItem002Fuzz();
    OHOS::TestGrdKvBatchCoupling003Fuzz();
    OHOS::DbCloseFuzz(data, size);
    OHOS::DbCloseResultSetFuzz();
    OHOS::DbFlushFuzz(data, size);

    OHOS::TearDownTestCase();
    return 0;
}
}
