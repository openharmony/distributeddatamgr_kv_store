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
#include "jsonservice_fuzzer.h"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>

#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_base/grd_resultset_api.h"
#include "grd_documentVirtual/grd_documentVirtual_api.h"
#include "grd_kv/grd_kv_api.h"
#include "grd_resultset_inner.h"
#include "securec.h"
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

const char *TEST_DB = "./data";
const char *TEST_FILE = "./data/testfile";
const char *COLLECT_NAME = "collectionname";
const char *OPTION_STRING = "{ \"maxdoc\" : 1000}";
const char *CONFIG_STRING = "{}";
const char *DB_DIR_PATH = "./data/";
const char *NO_EXIST_COLLECTION_NAME = "no_exisit";
const int NUM_THREE = 3;
const int SMALL_PREFIX_LENGTH = 5;
const int NUM_FIFTEEN = 15;
const int NUM_NINETY_EIGHT = 98;
const int BATCH_COUNT = 100;
const int LESS_HALF_BYTES = 511;
const int HALF_BYTES = 512;
const int MORE_HALF_BYTES = 513;
const int HALF_HALF_BYTES = 256;
const int MORE_HALF_HALF_BYTES = 257;
const int MAX_LEN_NUM = 600;
const int LESS_MIDDLE_SIZE_TEST = 899;
const int MIDDLE_SIZE_TEST = 900;
const int MAX_SIZE_NUM_TEST = 1000;
const int ONE_BYTES = 1024;
const int CURSOR_COUNT_TEST = 50000;
const int VECTOR_SIZE = 100000;

static GRD_DB *g_datatest = nullptr;

namespace OHOS {
namespace {
int RemoveDirTest(const char *dirTest)
{
    if (dirTest == nullptr) {
        return -1;
    }
    struct stat dirStatus;
    if (stat(dirTest, &dirStatus) < 0) {
        return -1;
    }
    if (access(dirTest, F_OK) != 0) {
        return 0;
    }
    char dirNameVirtual[PATH_MAX];
    DIR *dirPtrTest = nullptr;
    struct dirent *dr = nullptr;
    if (S_ISREG(dirStatus.st_mode)) { // normal file
        remove(dirTest);
    } else if (S_ISDIR(dirStatus.st_mode)) {
        dirPtrTest = opendir(dirTest);
        while ((dr = readdir(dirPtrTest)) != nullptr) {
            // ignore . and ..
            if ((strcmp(".", dr->d_name) == 0) || (strcmp("..", dr->d_name) == 0)) {
                continue;
            }
            if (sprintf_s(dirNameVirtual, sizeof(dirNameVirtual), "%s / %s", dirTest, dr->d_name) < 0) {
                (void)RemoveDirTest(dirNameVirtual);
                closedir(dirPtrTest);
                rmdir(dirTest);
                return -1;
            }
            (void)RemoveDirTest(dirNameVirtual);
        }
        closedir(dirPtrTest);
        rmdir(dirTest); // remove empty dirTest
    } else {
        return -1;
    }
    return 0;
}

void MakeDirctionary(const char *dirTest)
{
    std::string tmpPathVirtual;
    const char *pcur = dirTest;

    while (*pcur++ != '\0') {
        tmpPathVirtual.push_back(*(pcur - 1));
        if ((*pcur == '/' || *pcur == '\0') && access(tmpPathVirtual.c_str(), 0) != 0 && !tmpPathVirtual.empty()) {
            if (mkdir(tmpPathVirtual.c_str(), (S_IRUSR | S_IWUSR | S_IXUSR)) != 0) {
                return;
            }
        }
    }
}

std::string GetMaxString()
{
    std::string str = "{";
    for (int i = 0; i <= MAX_SIZE_NUM_TEST; i++) {
        for (int j = 0; j <= MAX_LEN_NUM; j++) {
            str += "a";
        }
    }
    return str + "}";
}
} // namespace

void InsertDocOneFuzzTest(const std::string &documentVirtualData)
{
    std::string documentVirtual2 = "{\"id\":2,\"field\":\"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual2.c_str(), 0);
    std::string documentVirtual3 = "{\"id\":true,\"field\":\"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual3.c_str(), 0);
    std::string documentVirtual4 = "{\"id\" : null, \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual4.c_str(), 0);
    std::string documentVirtual5 = "{\"id\" : [\"2\"], \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual5.c_str(), 0);
    std::string documentVirtual6 = "{\"id\" : {\"val\" : \"2\"}, \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual6.c_str(), 0);
    std::string documentVirtual7 = "{\"id\" : \"3\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual7.c_str(), 0);
    std::string documentVirtual9 = "{\"id\" : \"4\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(NULL, COLLECT_NAME, documentVirtual9.c_str(), 0);
    std::string documentVirtual10 = "{\"id\" : \"5\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, NULL, documentVirtual10.c_str(), 0);
    std::string documentVirtual12 = "{\"id\" : \"6\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual12.c_str(), 1);
    std::string documentVirtual13 = "{\"id\" : \"7\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual13.c_str(), INT_MAX);
    std::string documentVirtual14 = "{\"id\" : \"8\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual14.c_str(), INT_MIN);
    GRD_InsertDocument(g_datatest, NULL, NULL, 0);
    std::string documentVirtual15 = "{\"id\" : \"9\", \"name\" : \"" + documentVirtualData + "\"}";
    std::string collectName2(HALF_BYTES, 'a');
    GRD_InsertDocument(g_datatest, collectName2.c_str(), documentVirtual15.c_str(), 0);
    const char *collectName = "collection@!#";
    std::string documentVirtual18 = "{\"id\" : \"18\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual18.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual19.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual20.c_str(), 0);
    std::string documentVirtual21 = "{\"id\" : \"21\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, collectName, documentVirtual21.c_str(), GRD_OK);
    std::string collectName1(HALF_HALF_BYTES, 'k');
}

void InsertDocTwoFuzzTest(const std::string &documentVirtualData)
{
    std::string documentVirtualPart1 = "{ \"id\" : \"15\", \"textVal\" : \" ";
    std::string documentVirtualPart2 = "\" }";
    std::string jsonVal =
        std::string(HALF_BYTES * ONE_BYTES - documentVirtualPart1.size() - documentVirtualPart2.size(), 'k');
    std::string documentVirtual = documentVirtualPart1 + jsonVal + documentVirtualPart2;
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual.c_str(), 0);
    std::string jsonVal2 =
        std::string(HALF_BYTES * ONE_BYTES - 1 - documentVirtualPart1.size() - documentVirtualPart2.size(), 'k');
    std::string documentVirtual21 = documentVirtualPart1 + jsonVal2 + documentVirtualPart2;
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual21.c_str(), 0);
    std::string documentVirtual22 = "{\"id\" : \"16\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual22.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual23.c_str(), 0);
    std::string documentVirtual24 = documentVirtualData;
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual24.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual25.c_str(), 0);
    std::string collectName3 = std::string(HALF_HALF_BYTES, 'k');
    std::string documentVirtual26 = "{\"id\" : \"22\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, collectName3.c_str(), documentVirtual26.c_str(), GRD_OK);
    std::string collectName4 = "Aads_sd__23Asb_";
    std::string documentVirtual27 = "{\"id\" : \"23\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, collectName4.c_str(), documentVirtual27.c_str(), GRD_OK);
    std::string collectName5 = "GRD_collectName";
    std::string documentVirtual28 = "{\"id\" : \"24\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual28.c_str(), 0);
    collectName5 = "GM_SYS__collectName";
    std::string documentVirtual29 = "{\"id\" : \"24_2\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual29.c_str(), 0);

    collectName5 = "grd_collectName";
    std::string documentVirtual30 = "{\"id\" : \"24_3\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual30.c_str(), 0);
}

void InsertDocThreeFuzzTest(const std::string &documentVirtualData)
{
    std::string collectName5 = "gm_sys_collectName";
    std::string documentVirtual31 = "{\"id\" : \"24_4\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual31.c_str(), 0);

    collectName5 = "gM_sYs_collectName";
    std::string documentVirtual32 = "{\"id\" : \"24_5\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual32.c_str(), 0);

    collectName5 = "gRd_collectName";
    std::string documentVirtual33 = "{\"id\" : \"24_6\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual33.c_str(), 0);

    collectName5 = "gRd@collectName";
    std::string documentVirtual34 = "{\"id\" : \"24_7\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual34.c_str(), 0);

    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual35.c_str(), 0);

    std::string documentVirtual36 = "{\"id\" : \"36_4\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_CreateCollect(g_datatest, collectName5.c_str(), "", 0);
    GRD_InsertDocument(g_datatest, collectName5.c_str(), documentVirtual36.c_str(), 0);

    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual36.c_str(), 0);
    std::string documentVirtual37 = "{\"id\" : \"37_4\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual37.c_str(), 0);
    std::string documentVirtual38 = "{\"id\" : \"38_8\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual38.c_str(), 0);
}

void InsertDocFourFuzzTest(const std::string &longId, const std::string &documentVirtualData)
{
    std::string documentVirtual39 =
        "{\"id\" : \"35\", \"A_aBdk_324_\" : \"" + documentVirtualData +
        "\", \"name\" : \"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual39.c_str(), 0);

    std::string documentVirtual40 = "{\"id\" : \"35_2\", \"1A_aBdk_324_\" : \"" + documentVirtualData +
        "\", "
        "\"name\" : \"" +
        documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual40.c_str(), 0);
    std::string documentVirtual41 = "{\"id\" : \"36_0\", \"stringType\" : \"" + documentVirtualData +
        documentVirtualData + ", \"objectType\" : {\"A\" : 3}}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual41.c_str(), 0);

    std::string documentVirtual42 = "({\"id\" : \"38_0\", \"field2\" : 1.7693" + longId + "13486232e308})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual42.c_str(), 0);
    std::string documentVirtual43 =
        "({\"id\" : \"38_1\", \"t1\" : {\"field2\" : 1.797" + longId + "693134862e308}})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual43.c_str(), 0);
    std::string documentVirtual44 = "({\"id\" : \"38_2\", \"t1\" : [1, 2, 1.797313486" + longId + "232e308]})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual44.c_str(), 0);
    std::string documentVirtual45 = "({\"id\" : \"38_3\", \"t1\" : [1, 2, -1.79763486167" + longId + "E+308]})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual45.c_str(), 0);
    std::string documentVirtual46 =
        "({\"id\" : \"38_4\", \"t1\" : [1, 2, -1." + longId + "79763486231570E+308]})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual46.c_str(), 0);
    std::string documentVirtual47 =
        "({\"id\" : \"38_5\", \"t1\" : [1, 2, 1.79769313486" + longId + "2370E+308]})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual47.c_str(), 0);

    std::string doc1 = "{\"id\" : ";
    std::string doc2 = "\"";
    std::string doc4 = "\"";
    std::string doc5 = ", \"name\" : \"" + documentVirtualData + "\"}";
    std::string documentVirtualMiddle(MIDDLE_SIZE_TEST, 'k');
    std::string documentVirtual48 = doc1 + doc2 + documentVirtualMiddle + doc4 + doc5;
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual48.c_str(), 0);
    std::string documentVirtualMiddle2(MAX_SIZE_NUM_TEST, 'k');
    documentVirtual48 = doc1 + doc2 + documentVirtualMiddle2 + doc4 + doc5;
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual48.c_str(), 0);

    std::string documentVirtual49 = "({\"id\":\"0123\", \"num\":\"" + documentVirtualData + "\"})";
    std::string documentVirtual50 = "({\"id\":\"0123\", \"NUM\":\"" + documentVirtualData + "\"})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual49.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual50.c_str(), 0);

    std::string documentVirtual51 = "({\"id\":\"0123\", \"num.\":\"" + documentVirtualData + "\"})";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual51.c_str(), 0);

    const char *documentVirtual52 = R""({})"";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual52, 0);
}

void InsertDocFuzzTest(const uint8_t *data, size_t size)
{
    std::string collectNameData(reinterpret_cast<const char *>(data), size);
    const char *collectNameVal = collectNameData.data();
    std::string optionStrData(reinterpret_cast<const char *>(data), size);
    const char *optionStrVal = optionStrData.data();
    GRD_CreateCollect(g_datatest, collectNameVal, optionStrVal, 0);
    std::string documentVirtualData(reinterpret_cast<const char *>(data), size);
    const char *documentVirtualVal = documentVirtualData.data();
    GRD_InsertDocument(g_datatest, collectNameVal, documentVirtualVal, 0);
    GRD_InsertDocument(g_datatest, collectNameVal, documentVirtualVal, 0);
    std::string strJson = "{" + documentVirtualData + "}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, strJson.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, strJson.c_str(), 0);
    std::string stringJson2 = "{\"id\":\"1\",\"field\":\"" + documentVirtualData + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, stringJson2.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, stringJson2.c_str(), 0);
    std::string longId = "1";
    for (int i = 0; i < MAX_SIZE_NUM_TEST; i++) {
        longId += "1";
    }
    longId = "{\"id\":\"" + longId + "\",\"field\":\"field_id\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, longId.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, "{\"id\":123}", 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, "{\"field1.field2.field&*^&3\":\"anc\"}", 0);
    InsertDocOneFuzzTest(documentVirtualData);
    InsertDocTwoFuzzTest(documentVirtualData);
    InsertDocThreeFuzzTest(documentVirtualData);
    InsertDocFourFuzzTest(longId, documentVirtualData);

    const char *documentVirtual53 = R""({"empty" : null})"";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual53, 0);
    GRD_DropCollect(g_datatest, collectNameVal, 0);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);

    std::string documentVirtualNew1 = "{\"id\" : ";
    std::string documentVirtualNew2 = "\"";
    std::string documentVirtualNew3 = "\"";
    std::string documentVirtualNew4 = ", \"name\" : \"Ori\"}";
    std::string documentVirtual = documentVirtualNew1 +
        documentVirtualNew2 + documentVirtualData + documentVirtualNew3 + documentVirtualNew4;
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual.c_str(), 0);
}

static const char *DOCU1 = "{\"id\" : \"1\", \"name\":\"doc1\",\"item\":\"journal\",\"person\":\
    {\"school\":\"AB\", \"age\" : 51}}";
static const char *DOCU2 = "{\"id\" : \"2\", \"name\":\"doc2\",\"item\": 1, \"person\":\
    [1, \"my string\", {\"school\":\"AB\", \"age\" : 51}, true, {\"school\":\"CD\", \"age\" : 15}, false]}";
static const char *DOCU3 = "{\"id\" : \"3\", \"name\":\"doc3\",\"item\":\"notebook\",\"person\":\
    [{\"school\":\"C\", \"age\" : 5}]}";
static const char *DOCU4 = "{\"id\" : \"4\", \"name\":\"doc4\",\"item\":\"paper\",\"person\":\
    {\"grade\" : 1, \"school\":\"A\", \"age\" : 18}}";
static const char *DOCU5 = "{\"id\" : \"5\", \"name\":\"doc5\",\"item\":\"journal\",\"person\":\
    [{\"sex\" : \"woma\", \"school\" : \"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *DOCU6 = "{\"id\" : \"6\", \"name\":\"doc6\",\"item\":false,\"person\":\
    [{\"school\":\"B\", \"teacher\" : \"mike\", \"age\" : 15},\
    {\"school\":\"C\", \"teacher\" : \"moon\", \"age\" : 20}]}";

static const char *DOCU7 = "{\"id\" : \"7\", \"name\":\"doc7\",\"item\":\"fruit\",\"other_Info\":\
    [{\"school\":\"BX\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *DOCU8 = "{\"id\" : \"8\", \"name\":\"doc8\",\"item\":true,\"person\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *DOCU9 = "{\"id\" : \"9\", \"name\":\"doc9\",\"item\": true}";
static const char *DOCU10 = "{\"id\" : \"10\", \"name\":\"doc10\", \"parent\" : \"kate\"}";
static const char *DOCU11 = "{\"id\" : \"11\", \"name\":\"doc11\", \"other\" : \"null\"}";
static const char *DOCU12 = "{\"id\" : \"12\", \"name\":\"doc12\",\"other\" : null}";
static const char *DOCU13 = "{\"id\" : \"13\", \"name\":\"doc13\",\"item\" : \"shoes\",\"person\":\
    {\"school\":\"AB\", \"age\" : 15}}";
static const char *DOCU14 = "{\"id\" : \"14\", \"name\":\"doc14\",\"item\" : true,\"person\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 85}]}";
static const char *DOCU15 = "{\"id\" : \"15\", \"name\":\"doc15\",\"person\":[{\"school\":\"C\", \"age\" : "
                                  "5}]}";
static const char *DOCU16 = "{\"id\" : \"16\", \"name\":\"doc16\", \"nested1\":{\"nested2\":{\"nested3\":\
    {\"nested4\":\"ABC\", \"field2\":\"CCC\"}}}}";
static const char *DOCU17 = "{\"id\" : \"17\", \"name\":\"doc17\",\"person\":\"oh,ok\"}";
static const char *DOCU18 = "{\"id\" : \"18\", \"name\":\"doc18\",\"item\" : \"mobile phone\",\"person\":\
    {\"school\":\"DD\", \"age\":66}, \"color\":\"blue\"}";
static const char *DOCU19 = "{\"id\" : \"19\", \"name\":\"doc19\",\"ITEM\" : true,\"PERSONINFO\":\
    {\"school\":\"AB\", \"age\":15}}";
static const char *DOCU20 = "{\"id\" : \"20\", \"name\":\"doc20\",\"ITEM\" : true,\"person\":\
    [{\"SCHOOL\":\"B\", \"AGE\":15}, {\"SCHOOL\":\"C\", \"AGE\":35}]}";
static const char *DOCU23 = "{\"id\" : \"23\", \"name\":\"doc22\",\"ITEM\" : "
                                  "true,\"person\":[{\"school\":\"b\", \"age\":15}, [{\"school\":\"doc23\"}, 10, "
                                  "{\"school\":\"doc23\"}, true, {\"school\":\"y\"}], {\"school\":\"b\"}]}";
static std::vector<const char *> g_datatest = { DOCU1, DOCU2, DOCU3, DOCU4, DOCU5,
    DOCU6, DOCU7, DOCU8, DOCU9, DOCU10, DOCU11, DOCU12, DOCU13,
    DOCU14, DOCU15, DOCU16, DOCU17, DOCU18, DOCU19, DOCU20, DOCU23 };

namespace {
static void InsertData(GRD_DB *g_datatest, const char *collectName)
{
    for (const auto &item : g_datatest) {
        GRD_InsertDocument(g_datatest, collectName, item, 0);
    }
}

void SetUpTestCase()
{
    (void)RemoveDirTest(TEST_DB);
    MakeDirctionary(TEST_DB);
    GRD_DBOpen(TEST_FILE, CONFIG_STRING, GRD_OPEN_CREATE, &g_datatest);
    InsertData(g_datatest, COLLECT_NAME);
    GRD_CreateCollect(g_datatest, COLLECT_NAME, OPTION_STRING, 0);
}

void TearDownTestCase()
{
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    GRD_DBClose(g_datatest, GRD_CLOSE);
    (void)RemoveDirTest(TEST_DB);
    g_datatest = nullptr;
}

void FindDocumentResultSetFuzzTest(const char *colName, const std::string &filt, const std::string &projectInfo)
{
    GRD_ResultSet *reSet = nullptr;
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, colName, query, 1, &reSet);
    if (reSet != nullptr) {
        GRD_Next(reSet);
        char *valueResult = nullptr;
        GRD_GetVal(reSet, &valueResult);
    }
    GRD_FreeRe(reSet);
}

void FindDocumentWithFlagFuzzTest(const std::string &filt, const std::string &projectInfo, int flag)
{
    GRD_ResultSet *reSet = nullptr;
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, flag, &reSet);
    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
}

void FindDocumentNextTwiceFuzzTest(const std::string &filt, const std::string &projectInfo)
{
    GRD_ResultSet *reSet = nullptr;
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
}

void FindDocumentZeroFuzzTest(const std::string &input)
{
    std::string filt = "{\"id\" : \"6\"}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"id\" : \"6\", \"name\":\"" + input + "\"}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"name\":\"" + input + "\"}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"id\" : \"" + input + "\"}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"id\" : 1}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"id\" : [\"" + input + "\", 1]}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"id\" : {\"" + input + "\" : \"1\"}}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"id\" : true}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    filt = "{\"id\" : null}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");

    const char *colName1 = "grd_type";
    const char *colName2 = "GM_SYS_sysfff";
    filt = "{\"id\" : \"1\"}";
    FindDocumentResultSetFuzzTest(colName1, filt, R"({})");
    FindDocumentResultSetFuzzTest(colName2, filt, R"({})");

    filt = "{\"id\" : \"100\"}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");
}

void FindDocumentOneFuzzTest()
{
    std::string filt = "{\"id\" : \"6\"}";
    GRD_ResultSet *reSet = nullptr;
    GRD_ResultSet *reSet2 = nullptr;
    Query query = { filt.c_str(), "{}" };
    const char *collectName = "DocumentDBFindTest024";
    GRD_CreateCollect(g_datatest, collectName, "", 0);
    InsertData(g_datatest, collectName);
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_FindDocument(g_datatest, collectName, query, 1, &reSet2);

    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);

    GRD_Next(reSet2);
    char *value2 = nullptr;
    GRD_GetVal(reSet2, &value2);
    GRD_FreeVal(value2);

    GRD_Next(reSet);
    GRD_Next(reSet2);
    GRD_GetVal(reSet, &value);
    GRD_GetVal(reSet2, &value);
    GRD_FreeRe(reSet);
    GRD_FreeRe(reSet2);
    GRD_DropCollect(g_datatest, collectName, 0);

    filt = "{\"id\" : \"16\"}";
    reSet = nullptr;
    std::string projectInfo = "{\"name\": true, \"nested1.nested2.nested3.nested4\":true}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_Next(reSet);
    value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);
    GRD_FreeRe(reSet);

    projectInfo = "{\"name\": true, \"nested1\":{\"nested2\":{\"nested3\":{\"nested4\":true}}}}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);
    GRD_FreeRe(reSet);
}

void FindDocumentTwoFuzzTest(const std::string &input)
{
    GRD_ResultSet *reSet = nullptr;
    std::string projectInfo = "{\"name\": 0, \"nested1.nested2.nested3.nested4\":0}";
    std::string filt = "{\"id\" : \"16\"}";
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);

    projectInfo = "{\"name\": \"" + input + "\", \"nested1.nested2.nested3.nested4\":\"" + input + "\"}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, projectInfo);

    filt = "{\"id\" : \"7\"}";
    reSet = nullptr;
    projectInfo = "{\"name\": true, \"other_Info\":true, \"non_exist_field\":true}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_Next(reSet);
    value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);

    projectInfo = "{\"name\": true, \"other_Info\":true, \" item \":true}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, projectInfo);
}

void FindDocumentThreeFuzzTest(const std::string &input)
{
    std::string filt = "{\"id\" : \"7\"}";
    GRD_ResultSet *reSet = nullptr;
    std::string projectInfo = "{\"name\": true, \"other_Info.non_exist_field\":true}";
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
    projectInfo = "{\"name\": true, \"other_Info\":{\"non_exist_field\":true}}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, projectInfo);
    projectInfo = "{\"name\": true, \"other_Info.0\": true}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, projectInfo);
    filt = "{\"id\" : \"4\"}";
    reSet = nullptr;
    projectInfo = "{\"person\": true, \"person.grade\": true}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);

    filt = "{\"id\" : \"7\"}";
    projectInfo = "{\"non_exist_field\":true}";
    int flag = 0;
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);
    flag = 1;
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);
    filt = "{\"id\" : \"7\"}";
    projectInfo = "{\"name\":true, \"item\":true}";
    flag = 0;
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);
    flag = 1;
    projectInfo = "{\"name\": " + input + ", \"item\": " + input + "}";
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);
    filt = "{\"id\" : \"7\"}";
    projectInfo = "{\"name\":false, \"item\":false}";
    flag = 0;
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);
    flag = 1;
    projectInfo = "{\"name\": " + input + ", \"item\": " + input + "}";
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);
    filt = "{\"id\" : \"4\"}";
    flag = 0;
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);
    projectInfo = "{\"name\": false, \"person.grade1\": false, \
            \"person.shool1\": false, \"person.age1\": false}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, projectInfo);
}

void FindDocumentFourFuzzTest(const std::string &input)
{
    std::string filt = "{\"id\" : \"4\"}";
    std::string projectInfo =
        "{\"name\": " + input + ", \"person.school\": " + input + ", \"person.age\": " + input + "}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, projectInfo);

    filt = "{\"id\" : \"17\"}";
    GRD_ResultSet *reSet = nullptr;
    Query query = { filt.c_str(), "{}" };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, GRD_DOC_ID_DISPLAY, &reSet);
    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);
    GRD_FreeRe(reSet);

    reSet = nullptr;
    query = { filt.c_str(), "{}" };
    GRD_FindDocument(g_datatest, "", query, 0, &reSet);
    GRD_FindDocument(g_datatest, nullptr, query, 0, &reSet);

    filt = "{\"id\" : \"4\"}";
    reSet = nullptr;
    projectInfo = "{\"name\":" + input + ", \"person\":0, \"item\":" + input + "}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);

    projectInfo = "{\"name\":true, \"person\":0, \"item\":true}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);

    projectInfo = "{\"name\":\"\", \"person\":0, \"item\":\"\"}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);

    projectInfo = "{\"name\":false, \"person\":1, \"item\":false";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);

    projectInfo = "{\"name\":false, \"person\":-1.123, \"item\":false";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);

    projectInfo = "{\"name\":false, \"person\":true, \"item\":false";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
}

void FindDocumentFiveFuzzTest(const std::string &input)
{
    std::string filt = "{\"id\" : \"6\"}";
    std::string projectInfo = "{\"name\":false, \"person\": 0, \"item\":0}";
    int flag = 0;
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);

    filt = "{\"id\" : \"18\"}";
    projectInfo = "{\"name\":true, \"person.age\": \"\", \"item\":1, \"color\":10, \"nonExist\" : "
                     "-100}";
    flag = 0;
    FindDocumentWithFlagFuzzTest(filt, projectInfo, flag);

    GRD_ResultSet *reSet = nullptr;
    projectInfo = "{\"person\":[true, " + input + "]}";
    flag = 1;
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, flag, &reSet);
    projectInfo = "{\"person\":null}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, flag, &reSet);
    reSet = nullptr;
    projectInfo = "{\"Name\":true, \"person.age\": \"\", \"item\":" + input + ", \"COLOR\":" + input +
        ", \"nonExist\" : "
        "" +
        input + "}";
    flag = 0;
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, flag, &reSet);
    GRD_Next(reSet);
    GRD_FreeRe(reSet);

    reSet = nullptr;
    projectInfo = "{\"Name\":" + input + ", \"person.age\": false, \"person.SCHOOL\": false, \"item\":\
        false, \"COLOR\":false, \"nonExist\" : false}";
    query = { filt.c_str(), projectInfo.c_str() };
    char *value = nullptr;
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_Next(reSet);
    GRD_FreeRe(reSet);

    filt = "{\"id\" : \"18\"}";
    reSet = nullptr;
    query = { filt.c_str(), "{}" };
    std::string collectName1(LESS_HALF_BYTES, 'a');
    GRD_FindDocument(g_datatest, collectName1.c_str(), query, 1, &reSet);
    GRD_FreeRe(reSet);
}

void FindDocumentSixFuzzTest(const std::string &input)
{
    std::string collectName2(HALF_BYTES, 'a');
    std::string filt = "{\"id\" : \"18\"}";
    Query query = { filt.c_str(), "{}" };
    GRD_ResultSet *reSet = nullptr;
    GRD_FindDocument(g_datatest, collectName2.c_str(), query, 1, &reSet);
    GRD_FindDocument(g_datatest, "", query, 1, &reSet);

    reSet = nullptr;
    std::string projectInfo = "{}";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, NUM_THREE, &reSet);
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, INT_MAX, &reSet);
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, INT_MIN, &reSet);
    GRD_FindDocument(nullptr, COLLECT_NAME, query, 0, &reSet);
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, nullptr);
    query = { nullptr, nullptr };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);

    std::string documentVirtual1 = "{\"id\" : ";
    std::string documentVirtual2 = "\"";
    std::string documentVirtual4 = "\"";
    std::string documentVirtual5 = "}";
    std::string documentVirtualMiddle(MIDDLE_SIZE_TEST, 'k');
    filt = documentVirtual1 + documentVirtual2 + documentVirtualMiddle + documentVirtual4 + documentVirtual5;
    reSet = nullptr;
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    std::string documentVirtualMiddle2(LESS_MIDDLE_SIZE_TEST, 'k');
    filt = documentVirtual1 + documentVirtual2 + documentVirtualMiddle2 + documentVirtual4 + documentVirtual5;
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_FreeRe(reSet);

    filt = "{\"person\" : {\"school\":\"" + input + "\"}}";
    FindDocumentNextTwiceFuzzTest(filt, R"({})");

    projectInfo = "{\"version\": " + input + "}";
    FindDocumentNextTwiceFuzzTest(filt, projectInfo);

    projectInfo = "({\"a00001\":" + input + ", \"a00001\":" + input + "})";
    FindDocumentNextTwiceFuzzTest(filt, projectInfo);

    projectInfo = "({\"abc123_.\":" + input + "})";
    FindDocumentNextTwiceFuzzTest(filt, projectInfo);

    filt = "({\"abc123_.\":" + input + "})";
    FindDocumentNextTwiceFuzzTest(filt, projectInfo);
}

void FindDocumentSevenFuzzTest(const std::string &input)
{
    std::string documentVirtual064 = "{\"id\" : \"64\", \"a\":" + input + ", \"doc64\" : " + input + "}";
    std::string documentVirtual063 = "{\"id\" : \"63\", \"a\":" + input + ", \"doc63\" : " + input + "}";
    std::string documentVirtual062 = "{\"id\" : \"62\", \"a\":" + input + ", \"doc62\" : " + input + "}";
    std::string documentVirtual061 = "{\"id\" : \"61\", \"a\":" + input + ", \"doc61\" : " + input + "}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual064.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual063.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual062.c_str(), 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual061.c_str(), 0);
    std::string filt = "{\"a\":" + input + "}";
    GRD_ResultSet *reSet = nullptr;
    std::string projectInfo = R"({})";
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);

    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);

    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);

    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
}

void FindDocumentFuzzTestPlus(const std::string &input)
{
    FindDocumentZeroFuzzTest(input);
    std::string filt = "{\"id\" : \"6\"}";
    FindDocumentResultSetFuzzTest(COLLECT_NAME, filt, R"({})");
    FindDocumentOneFuzzTest();
    FindDocumentTwoFuzzTest(input);
    FindDocumentThreeFuzzTest(input);
    FindDocumentFourFuzzTest(input);
    FindDocumentFiveFuzzTest(input);
    FindDocumentSixFuzzTest(input);
    FindDocumentSevenFuzzTest(input);

    std::string documentVirtual = "{\"a\":" + input + ", \"doc64\" : " + input + "}";
    filt = "{\"b\":" + input + "}";
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, filt.c_str(), documentVirtual.c_str(), 0);
    filt = "{\"a\":" + input + "}";
    GRD_ResultSet *reSet = nullptr;
    std::string projectInfo = R"({})";
    Query query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_Next(reSet);
    GRD_FreeRe(reSet);
    filt = "{\"a\":" + input + "}";
    reSet = nullptr;
    projectInfo = R"({})";
    query = { filt.c_str(), projectInfo.c_str() };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    GRD_Next(reSet);
    char *valueNew = nullptr;
    GRD_GetVal(reSet, &valueNew);
    GRD_FreeVal(valueNew);
    GRD_FreeRe(reSet);
    filt = "{\"person\" : {\"school\":" + input + "}" + "}";
    FindDocumentNextTwiceFuzzTest(filt, R"({})");
}
} // namespace

void FindDocumentFuzzTest(const uint8_t *data, size_t size)
{
    GRD_CreateCollect(g_datatest, COLLECT_NAME, OPTION_STRING, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson11 = "{\"field\":\"" + input + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    Query query = { inputJson11.c_str(), inputJson11.c_str() };
    GRD_ResultSet *reSet = nullptr;
    // ResultSet conflict
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_FreeRe(reSet);
    reSet = nullptr;
    GRD_FindDocument(g_datatest, input.c_str(), query, size, &reSet);
    GRD_FreeRe(reSet);
    GRD_FindDocument(nullptr, input.c_str(), query, 1, &reSet);
    query.filt = nullptr;
    GRD_FindDocument(g_datatest, input.c_str(), query, 1, &reSet);
    inputJson11 = "{\"field\": 0, \"field2\":" + input + "}";
    query.filt = "{}";
    query.project = inputJson11.c_str();
    reSet = nullptr;
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_FreeRe(reSet);
    inputJson11 = "{\"" + input + "\": 0}";
    query.project = inputJson11.c_str();
    reSet = nullptr;
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_FreeRe(reSet);
    inputJson11 = "{\"field\":[\"aaa\"," + input + "]}";
    reSet = nullptr;
    query.project = inputJson11.c_str();
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_FreeRe(reSet);
    query.filt = "{\"field\": false}";
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_FreeRe(reSet);
    FindDocumentFuzzTestPlus(input);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
}

void UpdateDocOneFuzzTest(std::string s, const std::string &input)
{
    std::string inputJson11 = "{\"field5\": \"" + s + "\"}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"name\":\"doc6\"}", inputJson11.c_str(), 1);
    inputJson11 = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"name\":\"doc7\"}", inputJson11.c_str(), 1);
    inputJson11 = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"name\":\"doc8\"}", inputJson11.c_str(), 1);

    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, inputJson11.c_str(), "{}", "{}", 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, input.c_str(), "{}", 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{}", input.c_str(), 0);
    GRD_UpDoc(nullptr, COLLECT_NAME, "{}", "{}", 0);
    inputJson11 = "{\"id\":\"2\", \"field\":" + input + "}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field\":" + input + "}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    s.clear();
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }
    inputJson11 = "{\"field5\": \"" + s + "\"}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"name\":\"doc6\"}", "{\"c0.c1\":false}", 1);
    inputJson11 = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"name\":\"doc7\"}", "{\"c0.c1\":null}", 1);
    inputJson11 = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"name\":\"doc8\"}", "{\"c0.0\":\"ac\"}", 1);
}

void UpdateDocTwoFuzzTest(const char *newCollName, std::string s, const std::string &input)
{
    std::string inputJson11 = "{\"field5\": \"" + s + "\"}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_UpDoc(g_datatest, newCollName, "{\"name\":\"doc6\"}", inputJson11.c_str(), 1);
    inputJson11 = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_UpDoc(g_datatest, newCollName, "{\"name\":\"doc7\"}", inputJson11.c_str(), 1);
    inputJson11 = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_UpDoc(g_datatest, newCollName, "{\"name\":\"doc8\"}", inputJson11.c_str(), 1);

    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, inputJson11.c_str(), "{}", "{}", 0);
    GRD_UpDoc(g_datatest, newCollName, input.c_str(), "{}", 0);
    GRD_UpDoc(g_datatest, newCollName, "{}", input.c_str(), 0);
    GRD_UpDoc(nullptr, newCollName, "{}", "{}", 0);
    inputJson11 = "{\"id\":\"2\", \"field\":" + input + "}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field\":" + input + "}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    s.clear();
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }
    inputJson11 = "{\"field5\": \"" + s + "\"}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field5\": \"" + s + s + "\"}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": [\"field2\", {\"field3\":\"" + input + "\"}]}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"name\":\"doc6\", \"c0\" : {\"c1\" : true } }";
    GRD_InsertDocument(g_datatest, newCollName, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, newCollName, "{\"name\":\"doc6\"}", "{\"c0.c1\":false}", 1);
    inputJson11 = "{\"name\":\"doc7\", \"c0\" : {\"c1\" : null } }";
    GRD_InsertDocument(g_datatest, newCollName, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, newCollName, "{\"name\":\"doc7\"}", "{\"c0.c1\":null}", 1);
    inputJson11 = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    GRD_InsertDocument(g_datatest, newCollName, inputJson11.c_str(), 0);
    GRD_UpDoc(g_datatest, newCollName, "{\"name\":\"doc8\"}", "{\"c0.0\":\"ac\"}", 1);
}

void UpdateDocFilterFuzzTest()
{
    const char *filt = "{\"id\" : \"1\"}";
    const char *updata2 = "{\"objectInfo.child.child\" : {\"child\":{\"child\":null}}}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, filt, updata2, 0);

    const char *filt1 = "{\"id\" : \"1\"}";
    const char *updata1 = "{\"id\" : \"6\"}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, filt1, updata1, 0);

    const char *filt2 = "{\"id\" : \"1\"}";
    const char *updata3 = "{\"age$\" : \"21\"}";
    const char *updata4 = "{\"bonus..traffic\" : 100}";
    const char *updata5 = "{\"0item\" : 100}";
    const char *updata6 = "{\"item\" : 1.79769313486232e308}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, filt2, updata3, 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, filt2, updata4, 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, filt2, updata5, 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, filt2, updata6, 0);

    const char *filt3 = "{\"id\" : \"1\"}";
    const char *updata7 = "{\"age\" : 21}";
    GRD_UpDoc(g_datatest, NULL, filt3, updata7, 0);
    GRD_UpDoc(g_datatest, "", filt3, updata7, 0);
    GRD_UpDoc(NULL, COLLECT_NAME, filt3, updata7, 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, NULL, updata7, 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, filt3, NULL, 0);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
}

void UpdateDocFuzzTest(const uint8_t *data, size_t size)
{
    GRD_CreateCollect(g_datatest, COLLECT_NAME, OPTION_STRING, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson11 = "{\"id\":\"2\", \"field\": \"aaa\", "
                            "\"subject\":\"aaaaaaaaaaa\", \"test1\": true, "
                            "\"test2\": null}";

    GRD_UpDoc(g_datatest, inputJson11.c_str(), "{}", "{}", 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, input.c_str(), "{}", 0);
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{}", input.c_str(), 0);
    GRD_UpDoc(nullptr, COLLECT_NAME, "{}", "{}", 0);
    inputJson11 = "{\"id\":\"2\", \"field\":" + input + "}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field\":" + input + "}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpDoc(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    std::string s;
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }
    UpdateDocOneFuzzTest(s, input);

    const char *newCollName = "./student";
    GRD_UpDoc(g_datatest, inputJson11.c_str(), "{}", "{}", 0);
    GRD_UpDoc(g_datatest, newCollName, input.c_str(), "{}", 0);
    GRD_UpDoc(g_datatest, newCollName, "{}", input.c_str(), 0);
    GRD_UpDoc(nullptr, newCollName, "{}", "{}", 0);
    inputJson11 = "{\"id\":\"2\", \"field\":" + input + "}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field\":" + input + "}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1.field2.field3.field4.field5.field6\":" + input + "}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\": {\"field2\": {\"field3\": {\"field4\": {\"field5\":" + input + "}}}}}";
    GRD_UpDoc(g_datatest, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    s.clear();
    for (int i = 0; i < ((ONE_BYTES * ONE_BYTES) - 1) - NUM_FIFTEEN; i++) {
        s += 'a';
    }

    UpdateDocTwoFuzzTest(newCollName, s, input);
    UpdateDocFilterFuzzTest();
}

void UpsertNewFuzzTest(const std::string &input, GRD_DB *db1)
{
    GRD_CreateCollect(g_datatest, "student", "", 0);
    std::string documentVirtualNew =
        "{\"name\": {\"first\":[\"XXX\", \"BBB\", \"CCC\"], \"last\":\"moray\", \"field\":" + input + "}";
    std::string updateDocNew = R""({"name.last.AA.B":"Mnado"})"";
    GRD_Upsert(g_datatest, "student", R""({"id":"10001"})"", documentVirtualNew.c_str(), 0);
    GRD_Upsert(db1, "student", R""({"id":"10001"})"", updateDocNew.c_str(), 1);
    GRD_DropCollect(g_datatest, "student", 0);

    GRD_CreateCollect(g_datatest, "student", "", 0);
    documentVirtualNew = "{\"name\":[\"Tmn\", \"BB\", \"Alice\"], \"field\":" + input + "}";
    updateDocNew = R""({"name.first":"GG"})"";
    GRD_Upsert(g_datatest, "student", R""({"id":"10002"})"", documentVirtualNew.c_str(), 0);
    GRD_Upsert(db1, "student", R""({"id":"10002"})"", updateDocNew.c_str(), 1);
}

void UpsertFuzzTest(const uint8_t *data, size_t size)
{
    GRD_CreateCollect(g_datatest, COLLECT_NAME, OPTION_STRING, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson11NoId = "{\"name\":\"doc8\", \"c0\" : [\"" + input + "\", 123]}";
    std::string inputJson11 = "{\"id\":\"1\", \"field\": " + input + "}";

    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    GRD_Upsert(g_datatest, input.c_str(), "{}", "{}", 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, inputJson11.c_str(), "{}", 0);
    inputJson11 = "{\"id\":\"1\", \"field\":" + input + "}";
    GRD_Upsert(g_datatest, COLLECT_NAME, "{\"id\":\"1\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"id\":\"1\", \"field\":\"" + input + "\"}";
    GRD_Upsert(g_datatest, COLLECT_NAME, "{\"id\":\"1\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field\":" + input + "}";
    GRD_Upsert(g_datatest, COLLECT_NAME, "{\"id\":\"1\"}", inputJson11.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    GRD_Upsert(g_datatest, COLLECT_NAME, "{\"id\":\"1\"}", inputJson11.c_str(), 1);
    GRD_Upsert(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 1);
    GRD_Upsert(nullptr, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), 1);
    GRD_Upsert(g_datatest, COLLECT_NAME, nullptr, inputJson11.c_str(), 1);
    GRD_Upsert(g_datatest, COLLECT_NAME, "{\"id\":\"2\"}", inputJson11.c_str(), BATCH_COUNT);

    const char *newCollName = "./student";
    GRD_DB *db1 = nullptr;
    GRD_CreateCollect(db1, newCollName, "", 0);
    GRD_InsertDocument(db1, newCollName, inputJson11.c_str(), 0);
    GRD_Upsert(db1, input.c_str(), "{}", "{}", 0);
    GRD_Upsert(db1, newCollName, inputJson11.c_str(), "{}", 0);
    inputJson11 = "{\"id\":\"1\", \"field\":" + input + "}";
    GRD_Upsert(db1, newCollName, "{\"id\":\"1\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"id\":\"1\", \"field\":\"" + input + "\"}";
    GRD_Upsert(db1, newCollName, "{\"id\":\"1\"}", inputJson11.c_str(), 0);
    inputJson11 = "{\"field\":" + input + "}";
    GRD_Upsert(db1, newCollName, "{\"id\":\"1\"}", inputJson11.c_str(), 0);
    GRD_Upsert(db1, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 0);
    GRD_Upsert(db1, newCollName, "{\"id\":\"1\"}", inputJson11.c_str(), 1);
    GRD_Upsert(db1, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 1);
    GRD_Upsert(db1, newCollName, "{\"id\":\"newID1999\"}", inputJson11NoId.c_str(), 0);
    GRD_Upsert(db1, newCollName, "{\"id\":\"newID1999a\"}", inputJson11NoId.c_str(), 1);
    GRD_Upsert(nullptr, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), 1);
    GRD_Upsert(db1, newCollName, nullptr, inputJson11.c_str(), 1);
    GRD_Upsert(db1, newCollName, "{\"id\":\"2\"}", inputJson11.c_str(), BATCH_COUNT);
    GRD_DropCollect(db1, newCollName, 0);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);

    UpsertNewFuzzTest(input, db1);
}

void DelDocResultFuzzTest(const std::string &input)
{
    const char *filt = "{\"age\" : 15}";
    GRD_DelDoc(g_datatest, COLLECT_NAME, filt, 0);
    const char *filt1 = "{\"id\" : \"1\", \"age\" : 15}";
    GRD_DelDoc(g_datatest, COLLECT_NAME, filt1, 0);
    std::string filt2 = "{\"id\" : \"1\", \"name\" : " + input + "}";
    GRD_DelDoc(g_datatest, COLLECT_NAME, filt2.c_str(), 1);
    GRD_DelDoc(g_datatest, NULL, filt2.c_str(), 0);
    GRD_DelDoc(g_datatest, "", filt2.c_str(), 1);
    GRD_DelDoc(g_datatest, COLLECT_NAME, filt2.c_str(), 0);

    const char *filt3 = "{\"id\" : \"1\"}";
    GRD_ResultSet *reSet = nullptr;
    Query query = { filt3, "{}" };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_DelDoc(g_datatest, COLLECT_NAME, filt3, 0);

    const char *filt4 = "{\"id\" : \"1\"}";
    std::string collectName1(LESS_HALF_BYTES, 'a');
    GRD_CreateCollect(g_datatest, collectName1.c_str(), "", 0);
    GRD_DelDoc(g_datatest, collectName1.c_str(), filt4, 0);
    GRD_DropCollect(g_datatest, collectName1.c_str(), 0);
    GRD_FreeRe(reSet);

    GRD_DelDoc(NULL, COLLECT_NAME, filt2.c_str(), 0);
    GRD_DelDoc(g_datatest, NULL, filt2.c_str(), 0);
    GRD_DelDoc(g_datatest, "", filt2.c_str(), 0);
    GRD_DelDoc(g_datatest, COLLECT_NAME, NULL, 0);
    GRD_DelDoc(g_datatest, COLLECT_NAME, "", 0);
    GRD_DelDoc(g_datatest, "notExisted", filt2.c_str(), 0);

    std::vector<std::string> filtVec = { R"({"id" : 1})", R"({"id":[1, 2]})", R"({"id" : {"t1" : 1}})",
        R"({"id":null})", R"({"id":true})", R"({"id" : 1.333})", R"({"id" : -2.0})" };
    for (const auto &item : filtVec) {
        GRD_DelDoc(g_datatest, COLLECT_NAME, item.c_str(), 0);
    }
    GRD_DelDoc(g_datatest, COLLECT_NAME, filt2.c_str(), 0);

    std::string filt5 = "{\"subject.info\" : \"" + input + "\"}";
    reSet = nullptr;
    query = { filt2.c_str(), "{}" };
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_DelDoc(g_datatest, COLLECT_NAME, filt5.c_str(), 0);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    GRD_FreeRe(reSet);
}

void DelDocFuzzTest(const uint8_t *data, size_t size)
{
    GRD_CreateCollect(g_datatest, COLLECT_NAME, OPTION_STRING, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson11 = "{\"field\":" + input + "}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    GRD_DelDoc(g_datatest, input.c_str(), "{}", 0);
    GRD_DelDoc(g_datatest, nullptr, "{}", 0);
    std::string longName = "";
    for (int i = 0; i < MORE_HALF_BYTES; i++) {
        longName += 'a';
    }
    GRD_DelDoc(g_datatest, "grd_name", "{}", 0);
    GRD_DelDoc(g_datatest, longName.c_str(), "{}", 0);
    GRD_DelDoc(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\":" + input + "}";
    GRD_DelDoc(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    inputJson11 = "{\"field\":\"" + input + "\"}";
    GRD_DelDoc(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    inputJson11 = "{\"field1\":\"" + input + "\"}";
    GRD_DelDoc(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);

    const char *documentVirtual1 = "{  \
        \"id\" : \"1\", \
        \"name\": \"xiaoming\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"math\", \"English\", \"music\", {\"info\" : \"exam\"}] \
    }";
    const char *documentVirtual2 = "{  \
        \"id\" : \"2\", \
        \"name\": \"ori\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"math\", \"English\", \"music\"] \
    }";
    const char *documentVirtual3 = "{  \
        \"id\" : \"3\", \
        \"name\": \"David\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"Sing\", \"Jump\", \"Rap\", \"BasketBall\"] \
    }";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual1, 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual2, 0);
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual3, 0);
    GRD_DelDoc(g_datatest, COLLECT_NAME, "{}", 0);

    DelDocResultFuzzTest(input);
}

namespace {
void FindAndRelease(Query query)
{
    GRD_ResultSet *reSet = nullptr;
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_Next(reSet);
    char *value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
    reSet = nullptr;
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 0, &reSet);
    GRD_Next(reSet);
    value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);

    const char *newCollName = "./student";
    reSet = nullptr;
    GRD_FindDocument(g_datatest, newCollName, query, 1, &reSet);
    GRD_Next(reSet);
    value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
    reSet = nullptr;
    GRD_FindDocument(g_datatest, newCollName, query, 0, &reSet);
    GRD_Next(reSet);
    value = nullptr;
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
}
} // namespace

void FindAndReleaseFuzzTest(std::string documentVirtual, std::string filt, Query query, const std::string &input)
{
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual.c_str(), 0);
    query.filt = filt.c_str();
    query.project = "{\"field\": 0}";
    FindAndRelease(query);
    documentVirtual = "{\"field1.field2\" [false], \"field1.field2.field3\": [3], "
               "\"field1.field4\": [null]}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual.c_str(), 0);
    query.filt = documentVirtual.c_str();
    query.project = "{\"field\": 0}";
    FindAndRelease(query);
    query.project = "{\"field\": 1}";
    FindAndRelease(query);
    query.project = "{\"field.field2\": 1}";
    FindAndRelease(query);
    documentVirtual = "{\"field1\": {\"field2\": [{\"field3\":\"" + input + "\"}]}}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, documentVirtual.c_str(), 0);
    filt = "{\"field1.field2.field3\":\"" + input + "\"}";
    query.filt = filt.c_str();
    query.project = "{\"field1.field2\": 1}";
    FindAndRelease(query);
    query.project = "{\"field1.field2\": 0}";
    FindAndRelease(query);
    query.project = "{\"field1.field3\": 1}";
    FindAndRelease(query);
    query.project = "{\"field1.field3\": 0}";
    FindAndRelease(query);
    query.project = "{\"field1.field2.field3\": 0, \"field1.field2.field3\": 0}";
    FindAndRelease(query);
    std::string project = "{\"field1\": \"" + input + "\"}";
    query.project = project.c_str();
    FindAndRelease(query);
    project = "{\"" + input + "\": true, \"" + input + "\": false}";
    query.project = project.c_str();
    FindAndRelease(query);
}

void NextFuzzTest(const uint8_t *data, size_t size)
{
    GRD_CreateCollect(g_datatest, COLLECT_NAME, OPTION_STRING, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson11 = "{" + input + "}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    Query query = { inputJson11.c_str(), "{}" };
    FindAndRelease(query);
    std::string stringJson2 = "{ \"id\":\"1\", \"field\":\"" + input + "\"}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, stringJson2.c_str(), 0);
    query.filt = "{\"id\": \"1\"}";
    FindAndRelease(query);
    std::string filt2 = "{ \"id\":\"1\", \"field\":\"" + input + " invalid\"}";
    query.filt = filt2.c_str();
    FindAndRelease(query);
    query.filt = "{\"id\": \"3\"}";
    FindAndRelease(query);
    std::string stringJson3 = "{ \"id\":\"2\", \"field\": [\"field2\", null, \"abc\", 123]}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, stringJson3.c_str(), 0);
    query.filt = "{\"field\": [\"field2\", null, \"abc\", 123]}";
    FindAndRelease(query);
    std::string documentVirtual = "{\"field\": [\"" + input + "\",\"" + input + "\",\"" + input + "\"]}";
    std::string filt = "{\"field." + std::to_string(size) + "\":\"" + input + "\"}";
    FindAndReleaseFuzzTest(documentVirtual, filt, query, input);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
}

void GetValueFuzzTest(const uint8_t *data, size_t size)
{
    GRD_CreateCollect(g_datatest, COLLECT_NAME, OPTION_STRING, 0);
    std::string input(reinterpret_cast<const char *>(data), size);
    std::string inputJson11 = "{" + input + "}";
    GRD_InsertDocument(g_datatest, COLLECT_NAME, inputJson11.c_str(), 0);
    char *value = nullptr;
    Query query = { inputJson11.c_str(), "{}" };
    GRD_ResultSet *reSet = nullptr;
    GRD_FindDocument(g_datatest, COLLECT_NAME, query, 1, &reSet);
    GRD_GetVal(reSet, &value);
    GRD_Next(reSet);
    GRD_GetVal(reSet, &value);
    GRD_FreeVal(value);
    GRD_FreeRe(reSet);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
}

void FreeReFuzzTest()
{
    GRD_ResultSet *reSet = nullptr;
    GRD_FreeRe(reSet);
    reSet = new GRD_ResultSet;
    reSet->reSet_ = DocumentDB::ResultSet();
    GRD_FreeRe(reSet);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
}

void DbOpenCloseFuzzTest(const char *dbFileVal, const char *configStr, GRD_DB *dbVal)
{
    int ret = GRD_DBOpen(dbFileVal, configStr, GRD_OPEN_CREATE, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBClose(dbVal, GRD_CLOSE);
    }
}

void DbOpenOneFuzzTest(GRD_DB *dbVal)
{
    std::string path = "./documentVirtualFuzzTest.db";
    int ret = GRD_DBOpen(path.c_str(), "", GRD_DB_OPEN_ONLY, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBClose(dbVal, GRD_CLOSE);
    }
    (void)remove(path.c_str());

    path = "./documentVirtual.db";
    DbOpenCloseFuzzTest(path.c_str(), R""({"pageSize":64, "redopubbufsize":4033})"", dbVal);
    DbOpenCloseFuzzTest(path.c_str(), R""({"pageSize":64, "redopubbufsize":4032})"", dbVal);
    DbOpenCloseFuzzTest(path.c_str(), R""({"redopubbufsize":255})"", dbVal);
    DbOpenCloseFuzzTest(path.c_str(), R""({"redopubbufsize":16385})"", dbVal);
}

void DbOpenFuzzTestTest(const uint8_t *data, size_t size)
{
    std::string dbFileData(reinterpret_cast<const char *>(data), size);
    std::string realDbFileData = DB_DIR_PATH + dbFileData;
    const char *dbFileVal = realDbFileData.data();
    std::string configStrData(reinterpret_cast<const char *>(data), size);
    const char *configStrVal = configStrData.data();
    GRD_DB *dbVal = nullptr;

    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    std::string stringMax = getMaxString();
    const char *configStrMaxLen = stringMax.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrMaxLen, dbVal);

    std::string fieldStringValue = "{\"bufferPoolSize\": \"1024.5\"}";
    const char *configStrStringValue = fieldStringValue.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrStringValue, dbVal);

    std::string fieldBoolValue = "{\"bufferPoolSize\":}";
    const char *configStrBool = fieldBoolValue.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrBool, dbVal);

    std::string fieldStringValueAppend = "{\"bufferPoolSize\":\"8192\"}";
    const char *configStrStr = fieldStringValueAppend.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrStr, dbVal);

    std::string fieldStringValueFlush = "{\"bufferPoolSize\":\"8192\",\"redoFlushBtTrx\":\"1\"}";
    const char *configStrFlush = fieldStringValueFlush.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrFlush, dbVal);

    std::string fieldStringValueRedoBufsize = "{\"bufferPoolSize\":\"8192\",\"redoBufSize\":\"16384\"}";
    const char *configStrBs = fieldStringValueRedoBufsize.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrBs, dbVal);

    std::string fieldStringValueMaxConnNum = "{\"bufferPoolSize\":\"8192\",\"maxConnNum\":\"1024\"}";
    const char *configStrMcn = fieldStringValueMaxConnNum.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrMcn, dbVal);

    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    std::string fieldStringValueAll = "{\"bufferPoolSize\":\"8192\",\"redoFlushBtTrx\":\"1\",\"redoBufSize\":"
                                        "\"16384\",\"maxConnNum\":\"1024\"}";
    const char *configStrAll = fieldStringValueAll.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrAll, dbVal);

    std::string fieldLowNumber = "{\"bufferPoolSize\": 102}";
    const char *configStrLowNumber = fieldLowNumber.data();
    DbOpenCloseFuzzTest(dbFileVal, configStrLowNumber, dbVal);

    int ret = GRD_DBOpen(nullptr, configStrVal, GRD_OPEN_CREATE, &dbVal);
    if (ret == GRD_OK) {
        GRD_DBClose(nullptr, GRD_CLOSE);
    }
    DbOpenCloseFuzzTest(dbFileVal, configStrVal, dbVal);

    DbOpenOneFuzzTest(dbVal);
}

void DbCloseFuzzTest(const uint8_t *data, size_t size)
{
    std::string dbFileData(reinterpret_cast<const char *>(data), size);
    std::string realDbFileData = DB_DIR_PATH + dbFileData;
    const char *dbFileVal = realDbFileData.data();
    std::string configStrData(reinterpret_cast<const char *>(data), size);
    const char *configStrVal = configStrData.data();
    GRD_DB *dbVal = nullptr;
    DbOpenCloseFuzzTest(dbFileVal, configStrVal, dbVal);
}

void DbCloseResultSetFuzzTest()
{
    GRD_DB *db = nullptr;
    GRD_DB *db2 = nullptr;
    int ret = GRD_DBOpen(TEST_FILE, CONFIG_STRING, GRD_OPEN_CREATE, &db);
    int errCode = GRD_DBOpen(TEST_FILE, CONFIG_STRING, GRD_OPEN_CREATE, &db2);
    if (ret == GRD_OK) {
        GRD_CreateCollect(db, "collection1", "{\"maxdoc\" : 5}", 0);

        GRD_ResultSet *reSet = nullptr;
        Query query = { "{}", "{}" };
        GRD_FindDocument(db, "collection1", query, 0, &reSet);

        GRD_FreeRe(reSet);

        GRD_DBClose(db, GRD_CLOSE);
    }

    if (errCode == GRD_OK) {
        GRD_DBClose(db2, GRD_CLOSE);
    }
}

void CreateCollectionFuzzTest(const uint8_t *data, size_t size)
{
    std::string collectNameData(reinterpret_cast<const char *>(data), size);
    const char *collectNameVal = collectNameData.data();
    std::string optionStrData(reinterpret_cast<const char *>(data), size);
    const char *optionStrVal = optionStrData.data();
    GRD_CreateCollect(nullptr, collectNameVal, optionStrVal, 0);
    GRD_CreateCollect(g_datatest, collectNameVal, optionStrVal, 0);
    const char *optionStr = nullptr;
    GRD_CreateCollect(g_datatest, COLLECT_NAME, optionStr, 0);
    GRD_CreateCollect(g_datatest, COLLECT_NAME, "{\"maxdoc\":5, \"unexpected_max_doc\":32}", 0);
    GRD_CreateCollect(g_datatest, COLLECT_NAME, "{}", 0);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    std::string optStr = "{\"maxdoc\":" + optionStrData + "}";
    GRD_CreateCollect(g_datatest, COLLECT_NAME, optStr.c_str(), 0);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    GRD_CreateCollect(g_datatest, COLLECT_NAME, optStr.c_str(), MAX_SIZE_NUM_TEST);
    optStr = "{\"maxdoc\": 5}";
    GRD_CreateCollect(g_datatest, COLLECT_NAME, optStr.c_str(), 0);
    GRD_CreateCollect(g_datatest, COLLECT_NAME, optStr.c_str(), 1);
    GRD_DropCollect(g_datatest, COLLECT_NAME, 0);
    GRD_DropCollect(g_datatest, COLLECT_NAME, MAX_SIZE_NUM_TEST);
    GRD_DropCollect(g_datatest, collectNameVal, 0);
}

void DropCollectFuzzTest(const uint8_t *data, size_t size)
{
    std::string collectNameData(reinterpret_cast<const char *>(data), size);
    const char *collectNameVal = collectNameData.data();
    std::string optionStrData(reinterpret_cast<const char *>(data), size);
    const char *optionStrVal = optionStrData.data();
    GRD_CreateCollect(g_datatest, collectNameVal, optionStrVal, 0);
    GRD_DropCollect(nullptr, collectNameVal, 0);
    GRD_DropCollect(g_datatest, collectNameVal, 0);
}

void DbFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return;
    }
    GRD_DB *db = nullptr;
    GRD_DB *db2 = nullptr;
    const uint32_t flags = *data;
    int ret = GRD_DBOpen(TEST_FILE, CONFIG_STRING, GRD_OPEN_CREATE, &db);
    if (ret == GRD_OK) {
        GRD_Flush(db, flags);
        GRD_DBClose(db, GRD_CLOSE);
    }
    GRD_Flush(db2, flags);
}

void TestGrdDbApGrdGetItem002FuzzTest()
{
    const char *config = CONFIG_STRING;
    GRD_DB *db = nullptr;
    DbOpenCloseFuzzTest(TEST_FILE, config, db);
    GRD_Preload(nullptr, COLLECT_NAME);
    GRD_Preload(nullptr, "invalid_name");
    GRD_Preload(g_datatest, COLLECT_NAME);
    std::string smallPrefix = std::string(SMALL_PREFIX_LENGTH, 'a');
    for (uint32_t i = 0; i < NUM_NINETY_EIGHT; ++i) {
        std::string v = smallPrefix + std::to_string(i);
        GRD_KVItem key = { &i, sizeof(uint32_t) };
        GRD_KVItem value = { reinterpret_cast<void *>(v.data()), static_cast<uint32_t>(v.size()) + 1 };
        GRD_KVPut(g_datatest, COLLECT_NAME, &key, &value);

        GRD_KVItem getValue = { nullptr, 0 };
        GRD_KVGet(g_datatest, COLLECT_NAME, &key, &getValue);
        GRD_KVFreeItem(&getValue);

        GRD_KVDel(g_datatest, COLLECT_NAME, &key);
    }
    GRD_Flush(g_datatest, 1);

    uint32_t begin = 0, end = MAX_SIZE_NUM_TEST;
    GRD_FilterOptionT option = {};
    option.mode = KV_SCAN_RANGE;
    option.begin = { &begin, sizeof(uint32_t) };
    option.end = { &end, sizeof(uint32_t) };
    GRD_ResultSet *reSet = nullptr;
    GRD_KVFilter(g_datatest, COLLECT_NAME, &option, &reSet);

    uint32_t keySize, valueSize;
    GRD_KVGetSize(reSet, &keySize, &valueSize);

    reSet = nullptr;
    uint32_t i = 32;
    GRD_KVItem key = { &i, sizeof(uint32_t) };
    GRD_KVScan(g_datatest, COLLECT_NAME, &key, KV_SCAN_EQUAL_OR_LESS_KEY, &reSet);

    GRD_Prev(reSet);
    GRD_Next(reSet);

    GRD_FreeRe(reSet);
}

void TestGrdKvBatchCoupling003FuzzTest()
{
    const char *config = CONFIG_STRING;
    GRD_DB *db = nullptr;
    DbOpenCloseFuzzTest(TEST_FILE, config, db);

    GRD_ResultSet *reSet = nullptr;
    GRD_KVScan(g_datatest, COLLECT_NAME, nullptr, KV_SCAN_PREFIX, &reSet);

    for (uint32_t i = 0; i < CURSOR_COUNT_TEST; i++) {
        GRD_Next(reSet);
    }

    GRD_KVBatchT *batchDel = nullptr;
    std::vector<std::string> keySet;
    std::vector<std::string> valueSet;
    for (uint32_t i = 0; i < VECTOR_SIZE; i++) {
        std::string key(MAX_SIZE_NUM_TEST, 'a');
        key += std::to_string(i);
        keySet.emplace_back(key);
        std::string value = std::to_string(i);
        valueSet.emplace_back(value);
    }

    for (int j = 0; j < BATCH_COUNT; j++) {
        GRD_KVBatchPrepare(BATCH_COUNT, &batchDel);
        GRD_KVBatchPut(g_datatest, COLLECT_NAME, batchDel);
        for (uint16_t i = 0; i < BATCH_COUNT; i++) {
            char *batchKey = const_cast<char *>(keySet[CURSOR_COUNT_TEST + j * BATCH_COUNT + i].c_str());
            char *batchValue = const_cast<char *>(valueSet[CURSOR_COUNT_TEST + j * BATCH_COUNT + i].c_str());
            GRD_KVBatchPushback(static_cast<void *>(batchKey),
                static_cast<uint32_t>(keySet[CURSOR_COUNT_TEST + j * BATCH_COUNT + i].length()) + 1,
                static_cast<void *>(batchValue),
                static_cast<uint32_t>(valueSet[CURSOR_COUNT_TEST + j * BATCH_COUNT + i].length()) + 1,
                batchDel);
        }

        GRD_KVBatchDel(g_datatest, COLLECT_NAME, batchDel);

        GRD_KVBatchDestroy(batchDel);
    }

    GRD_Next(reSet);
    GRD_KVItem keyItem = { nullptr, 0 };
    GRD_KVItem valueItem = { nullptr, 0 };
    GRD_Fetch(reSet, &keyItem, &valueItem);
    GRD_KVFreeItem(&keyItem);
    GRD_KVFreeItem(&valueItem);
    GRD_FreeRe(reSet);
}
} // namespace OHOS

/* FuzzTester entry point */
extern "C" {
int LLVMFuzzTesterTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetUpTestCase();

    OHOS::DbOpenFuzzTestTest(data, size);
    OHOS::CreateCollectionFuzzTest(data, size);
    OHOS::DropCollectFuzzTest(data, size);
    OHOS::InsertDocFuzzTest(data, size);
    OHOS::FindDocumentFuzzTest(data, size);
    OHOS::UpdateDocFuzzTest(data, size);
    OHOS::UpsertFuzzTest(data, size);
    OHOS::DelDocFuzzTest(data, size);
    OHOS::NextFuzzTest(data, size);
    OHOS::GetValueFuzzTest(data, size);
    OHOS::FreeReFuzzTest();
    OHOS::TestGrdDbApGrdGetItem002FuzzTest();
    OHOS::TestGrdKvBatchCoupling003FuzzTest();
    OHOS::DbCloseFuzzTest(data, size);
    OHOS::DbCloseResultSetFuzzTest();
    OHOS::DbFlushFuzzTest(data, size);

    OHOS::TearDownTestCase();
    return 0;
}
}
