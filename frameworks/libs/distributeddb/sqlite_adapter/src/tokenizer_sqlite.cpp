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
#include "tokenizer_api.h"
#include "tokenizer_export_type.h"
#include "securec.h"

#include <mutex>

SQLITE_EXTENSION_INIT1

using namespace CNTokenizer;

typedef struct Fts5TokenizerParam {
    uint32_t magicCode = 0;
    GRD_CutScene cutScene = DEFAULT;
} Fts5TokenizerParamT;

static std::mutex g_mtx;
static uint32_t g_refCount = 0;
constexpr int FTS5_MAX_VERSION = 2;
constexpr int CUSTOM_TOKENIZER_PARAM_NUM = 2;
constexpr int MAGIC_CODE = 0x12345678;
constexpr const char *CUT_SCENE_PARAM_NAME = "cut_mode";
constexpr const char *CUT_SCENE_SHORT_WORDS = "short_words";
constexpr const char *CUT_SCENE_DEFAULT = "default";

int fts5_customtokenizer_xCreate(void *sqlite3, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
{
    (void)sqlite3;
    std::lock_guard<std::mutex> lock(g_mtx);
    auto *pFts5TokenizerParam = new Fts5TokenizerParamT;
    pFts5TokenizerParam->magicCode = MAGIC_CODE;
    if (nArg != 0 && nArg != CUSTOM_TOKENIZER_PARAM_NUM) {
        sqlite3_log(SQLITE_ERROR, "invalid args num");
        return SQLITE_ERROR;
    }
    if (nArg == CUSTOM_TOKENIZER_PARAM_NUM) {
        std::string paramKey = std::string(azArg[0]);
        std::string paramValue = std::string(azArg[1]);
        if (paramKey != CUT_SCENE_PARAM_NAME) {
            sqlite3_log(SQLITE_ERROR, "invalid arg name");
            return SQLITE_ERROR;
        }
        if (paramValue == CUT_SCENE_SHORT_WORDS) {
            pFts5TokenizerParam->cutScene = SEARCH;
        } else if (paramValue == CUT_SCENE_DEFAULT) {
            pFts5TokenizerParam->cutScene = DEFAULT;
        } else {
            sqlite3_log(SQLITE_ERROR, "invalid arg value of cut scene");
            return SQLITE_ERROR;
        }
    }
    g_refCount++;
    if (g_refCount != 1) {  // 说明已经初始化过了，直接返回
        *ppOut = (Fts5Tokenizer *)pFts5TokenizerParam;
        return SQLITE_OK;
    }

    GRD_TokenizerParamT param = {CUT_MMSEG, EXTRACT_TF_IDF};
    int ret = GRD_TokenizerInit(NULL, NULL, param);
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
    GRD_CutOptionT option = {false, pFts5TokenizerParam->cutScene};
    GRD_WordEntryListT *entryList = nullptr;
    int ret = GRD_TokenizerCut(ptr, option, &entryList);
    if (ret != GRD_OK) {
        sqlite3_log(ret, "GRD_TokenizerCut wrong");
        free(ptr);
        return ret;
    }
    GRD_WordEntryT entry;
    int start = 0;  // 词在句子中的起始位置
    int end = 0;    // 词在句子中的结束位置
    while ((ret = GRD_TokenizerNext(entryList, &entry)) == GRD_OK) {
        start = entry.word - ptr;
        end = start + static_cast<int>(entry.length);
        ret = xToken(pCtx, 0, entry.word, entry.length, start, end);
        if (ret != SQLITE_OK) {
            sqlite3_log(ret, "xToken wrong");
            break;
        }
    }
    GRD_TokenizerFreeWordEntryList(entryList);
    free(ptr);
    if (ret != GRD_OK && ret != GRD_NO_DATA) {
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
    }
    g_refCount--;
    if (g_refCount != 0) {  // 说明还有其他的地方在使用，不能释放资源
        return;
    }
    (void)GRD_TokenizerDestroy(tokenizer_ptr);
}

/*
** Return a pointer to the fts5_api pointer for database connection db.
** If an error occurs, return NULL and leave an error in the database
** handle (accessible using sqlite3_errcode()/errmsg()).
*/
static int fts5_api_from_db(sqlite3 *db, fts5_api **ppApi)
{
    sqlite3_stmt *pStmt = 0;
    int rc;

    *ppApi = 0;
    rc = sqlite3_prepare(db, "SELECT fts5(?1)", -1, &pStmt, 0);
    if (rc == SQLITE_OK) {
        sqlite3_bind_pointer(pStmt, 1, reinterpret_cast<void *>(ppApi), "fts5_api_ptr", 0);
        (void)sqlite3_step(pStmt);
        rc = sqlite3_finalize(pStmt);
    }

    return rc;
}

int sqlite3_customtokenizer_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    (void)pzErrMsg;
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi)

    fts5_tokenizer tokenizer = {
        fts5_customtokenizer_xCreate, fts5_customtokenizer_xDelete, fts5_customtokenizer_xTokenize};
    fts5_api *fts5api;
    rc = fts5_api_from_db(db, &fts5api);
    if (rc != SQLITE_OK) {
        sqlite3_log(rc, "fts5_api_from_db wrong");
        return rc;
    }
    if (fts5api == 0 || fts5api->iVersion < FTS5_MAX_VERSION) {
        sqlite3_log(SQLITE_ERROR, "sqlite3_customtokenizer_init wrong");
        return SQLITE_ERROR;
    }
    return fts5api->xCreateTokenizer(fts5api, "customtokenizer", reinterpret_cast<void *>(fts5api), &tokenizer, NULL);
}
