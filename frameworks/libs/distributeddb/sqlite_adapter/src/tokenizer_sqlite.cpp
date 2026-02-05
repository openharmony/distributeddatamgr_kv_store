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

#include "tokenizer_sqlite.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "tokenizer_api.h"
#include "tokenizer_export_type.h"
#include "securec.h"

SQLITE_EXTENSION_INIT1

using namespace CNTokenizer;

typedef struct Fts5TokenizerParam {
    uint32_t magicCode = 0;
    GRD_CutScene cutScene = DEFAULT;
    bool caseSensitive = true;
} Fts5TokenizerParamT;

static std::mutex g_mtx;
static uint32_t g_refCount = 0;
constexpr int FTS5_MAX_VERSION = 2;
constexpr int MAGIC_CODE = 0x12345678;
constexpr const char *CUT_SCENE_PARAM_NAME = "cut_mode";
constexpr const char *CUT_CASE_SENSITIVE = "case_sensitive";
static std::unordered_map<std::string, GRD_CutScene> g_cutModeMap = {
    {"default", DEFAULT},
    {"short_words", SEARCH},
    {"multi_words", MULTI_WORDS}
};

int AnalyzeCutMode(std::string &value, Fts5TokenizerParamT *para)
{
    if (g_cutModeMap.find(value) == g_cutModeMap.end()) {
        sqlite3_log(SQLITE_ERROR, "invalid arg value of cut scene");
        return SQLITE_ERROR;
    }
    para->cutScene = g_cutModeMap[value];
    return SQLITE_OK;
}

int AnalyzeCaseSensitive(std::string &value, Fts5TokenizerParamT *para)
{
    if (value == "1") {
        para->caseSensitive = true;
    } else if (value == "0") {
        para->caseSensitive = false;
    } else {
        sqlite3_log(SQLITE_ERROR, "invalid arg value of case sensitive");
        return SQLITE_ERROR;
    }
    return SQLITE_OK;
}

int ParseArgs(const char **azArg, int nArg, Fts5TokenizerParamT *para)
{
    if (nArg == 0) {
        return SQLITE_OK;
    }
    // 检查参数个数是否为偶数
    if (nArg % 2 != 0) { // 通过模2校验是否为偶数
        sqlite3_log(SQLITE_ERROR, "|Parse Args| invalid args num %d", nArg);
        return SQLITE_ERROR; // 参数数量不匹配
    }
    int ret = SQLITE_OK;
    for (int i = 0; i < nArg - 1; i += 2) { // kv对 一次解析2个
        if (azArg[i] == nullptr || azArg[i + 1] == nullptr) {
            sqlite3_log(SQLITE_ERROR, "|Parse Args| azArg[i] null");
            return SQLITE_ERROR;
        }
        std::string key = std::string(azArg[i]);
        std::string value = std::string(azArg[i + 1]);
        if (key == CUT_SCENE_PARAM_NAME) {
            ret = AnalyzeCutMode(value, para);
        } else if (key == CUT_CASE_SENSITIVE) {
            ret = AnalyzeCaseSensitive(value, para);
        } else {
            sqlite3_log(SQLITE_ERROR, "invalid key");
            ret = SQLITE_ERROR;
        }
        if (ret != SQLITE_OK) {
            return ret;
        }
    }
    return SQLITE_OK;
}

int fts5_customtokenizer_xCreate(void *sqlite3, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
{
    (void)sqlite3;
    std::lock_guard<std::mutex> lock(g_mtx);
    auto *pFts5TokenizerParam = new(std::nothrow) Fts5TokenizerParamT;
    if (pFts5TokenizerParam == nullptr) {
        return SQLITE_ERROR;
    }
    pFts5TokenizerParam->magicCode = MAGIC_CODE;
    int ret = ParseArgs(azArg, nArg, pFts5TokenizerParam);
    if (ret != SQLITE_OK) {
        sqlite3_log(ret, "Parse Args wrong");
        delete pFts5TokenizerParam;
        return ret;
    }
    g_refCount++;
    if (g_refCount != 1) {  // 说明已经初始化过了，直接返回
        *ppOut = (Fts5Tokenizer *)pFts5TokenizerParam;
        return SQLITE_OK;
    }

    GRD_TokenizerParamT param = {CUT_MMSEG, EXTRACT_TF_IDF};
    ret = GRD_TokenizerInit(NULL, NULL, param);
    if (ret != GRD_OK) {
        sqlite3_log(ret, "GRD_TokenizerInit wrong");
        delete pFts5TokenizerParam;
        return ret;
    }
    *ppOut = (Fts5Tokenizer *)pFts5TokenizerParam;  // 需要保证*ppOut不为NULL，否则会使用默认分词器而不是自定的
    return SQLITE_OK;
}

static char *CpyStr(const char *pText, int nText)
{
    if (nText < 0) {
        return nullptr;
    }
    char *ptr = (char *)malloc(nText + 1);
    if (ptr == nullptr) {
        return nullptr;
    }
    errno_t err = memcpy_s(ptr, nText + 1, pText, nText);
    if (err != EOK) {
        free(ptr);
        return nullptr;
    }
    ptr[nText] = '\0';
    return ptr;
}

static int IterateCutResults(void *pCtx, int nText, XTokenFn xToken, GRD_CutScene cutScene, GRD_WordEntryListT *list)
{
    GRD_WordEntryT entry;
    int start = 0;  // word's start pos in sentence
    int end = 0;    // word's end pos in sentence
    int startBefore = -1;
    int ret = 0;
    while ((ret = GRD_TokenizerNext(list, &entry)) == GRD_OK) {
        start = static_cast<int>(entry.offset);
        end = start + static_cast<int>(entry.length);
        if (end > nText || start < 0) {
            sqlite3_log(SQLITE_ERROR, "|fts5 custom tokenizer xTokenize| offset wrong");
            return SQLITE_ERROR;
        }
        int tokenFlag = 0;
        if (start == startBefore && cutScene == MULTI_WORDS) {
            tokenFlag = FTS5_TOKEN_COLOCATED;
        }
        startBefore = start;
        ret = xToken(pCtx, tokenFlag, entry.word, entry.length, start, end);
        if (ret != SQLITE_OK) {
            sqlite3_log(ret, "xToken wrong");
            return ret;
        }
    }
    return ret;
}

int fts5_customtokenizer_xTokenize(
    Fts5Tokenizer *tokenizer_ptr, void *pCtx, int flags, const char *pText, int nText, XTokenFn xToken)
{
    Fts5TokenizerParamT *pFts5TokenizerParam = (Fts5TokenizerParamT *)tokenizer_ptr;
    if (pFts5TokenizerParam == nullptr || pFts5TokenizerParam->magicCode != MAGIC_CODE) {
        sqlite3_log(GRD_INVALID_ARGS, "The tokenizer is not initialized");
        return GRD_INVALID_ARGS;
    }
    if (nText == 0) {
        return SQLITE_OK;
    }
    char *ptr = CpyStr(pText, nText);
    if (ptr == nullptr) {
        sqlite3_log(GRD_FAILED_MEMORY_ALLOCATE, "CpyStr wrong");
        return GRD_FAILED_MEMORY_ALLOCATE;
    }
    GRD_CutOptionT option = {false, pFts5TokenizerParam->cutScene, !pFts5TokenizerParam->caseSensitive};
    GRD_WordEntryListT *entryList = nullptr;
    int ret = GRD_TokenizerCut(ptr, option, &entryList);
    if (ret != GRD_OK) {
        sqlite3_log(ret, "GRD_TokenizerCut wrong");
        free(ptr);
        return ret;
    }
    ret = IterateCutResults(pCtx, nText, xToken, pFts5TokenizerParam->cutScene, entryList);
    GRD_TokenizerFreeWordEntryList(entryList);
    free(ptr);
    if (ret != GRD_OK && ret != GRD_NO_DATA) {
        sqlite3_log(ret, "Iterate Cut Results wrong");
        return ret;
    }
    return SQLITE_OK;
}

void fts5_customtokenizer_xDelete(Fts5Tokenizer *tokenizer_ptr)
{
    std::lock_guard<std::mutex> lock(g_mtx);
    Fts5TokenizerParamT *pFts5TokenizerParam = (Fts5TokenizerParamT *)tokenizer_ptr;
    if (pFts5TokenizerParam != nullptr) {
        delete pFts5TokenizerParam;
        pFts5TokenizerParam = nullptr;
    }
    g_refCount--;
    if (g_refCount != 0) {  // 说明还有其他的地方在使用，不能释放资源
        return;
    }
    (void)GRD_TokenizerDestroy(nullptr);
}

/*
** Return a pointer to the fts5_api pointer for database connection db.
** If an error occurs, return NULL and leave an error in the database
** handle (accessible using sqlite3_errcode()/errmsg()).
*/
static int get_fts5_api(sqlite3 *db, fts5_api **api)
{
    sqlite3_stmt *stmt = nullptr;
    *api = nullptr;
    int ret = sqlite3_prepare(db, "SELECT fts5(?1)", -1, &stmt, 0);
    if (ret != SQLITE_OK) {
        sqlite3_log(ret, "sqlite3_prepare wrong");
        return ret;
    }
    sqlite3_bind_pointer(stmt, 1, reinterpret_cast<void *>(api), "fts5_api_ptr", 0);
    (void)sqlite3_step(stmt);
    return sqlite3_finalize(stmt);
}

int sqlite3_customtokenizer_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    (void)pzErrMsg;
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi)

    fts5_tokenizer tokenizer = {
        fts5_customtokenizer_xCreate, fts5_customtokenizer_xDelete, fts5_customtokenizer_xTokenize};
    fts5_api *fts5api = nullptr;
    rc = get_fts5_api(db, &fts5api);
    if (rc != SQLITE_OK) {
        sqlite3_log(rc, "get_fts5_api wrong");
        return rc;
    }
    if (fts5api == nullptr || fts5api->iVersion < FTS5_MAX_VERSION) {
        sqlite3_log(SQLITE_ERROR, "sqlite3_customtokenizer_init wrong");
        return SQLITE_ERROR;
    }
    return fts5api->xCreateTokenizer(fts5api, "customtokenizer", reinterpret_cast<void *>(fts5api), &tokenizer, NULL);
}
