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

#include "tokenizer_api_mgr.h"
#include "tokenizer_api.h"

namespace CNTokenizer {

static GRD_CNTokenizerApiT g_tokenizerApi = {};

// LCOV_EXCL_START
GRD_API int32_t GRD_TokenizerInit(const char *userDictPath, const char *userStopWordPath, GRD_TokenizerParamT param)
{
    if (g_tokenizerApi.tokenizerInit == nullptr) {
        g_tokenizerApi = GetTokenizerApi();
    }
    if (g_tokenizerApi.tokenizerInit == nullptr) {
        return GRD_API_NOT_SUPPORT;
    }
    return g_tokenizerApi.tokenizerInit(userDictPath, userStopWordPath, param);
}

GRD_API int32_t GRD_TokenizerCut(const char *sentence, GRD_CutOptionT option, GRD_WordEntryListT **entryList)
{
    if (g_tokenizerApi.tokenizerCut == nullptr) {
        g_tokenizerApi = GetTokenizerApi();
    }
    if (g_tokenizerApi.tokenizerCut == nullptr) {
        return GRD_API_NOT_SUPPORT;
    }
    return g_tokenizerApi.tokenizerCut(sentence, option, entryList);
}

GRD_API int32_t GRD_TokenizerExtract(const char *sentence, GRD_ExtractOptionT option, GRD_WordEntryListT **entryList)
{
    if (g_tokenizerApi.tokenizerExtract == nullptr) {
        g_tokenizerApi = GetTokenizerApi();
    }
    if (g_tokenizerApi.tokenizerExtract == nullptr) {
        return GRD_API_NOT_SUPPORT;
    }
    return g_tokenizerApi.tokenizerExtract(sentence, option, entryList);
}

GRD_API int32_t GRD_TokenizerNext(GRD_WordEntryListT *list, GRD_WordEntryT *entry)
{
    if (g_tokenizerApi.tokenizerNext == nullptr) {
        g_tokenizerApi = GetTokenizerApi();
    }
    if (g_tokenizerApi.tokenizerNext == nullptr) {
        return GRD_API_NOT_SUPPORT;
    }
    return g_tokenizerApi.tokenizerNext(list, entry);
}

GRD_API int32_t GRD_TokenizerFreeWordEntryList(GRD_WordEntryListT *list)
{
    if (g_tokenizerApi.tokenizerFreeWordEntryList == nullptr) {
        g_tokenizerApi = GetTokenizerApi();
    }
    if (g_tokenizerApi.tokenizerFreeWordEntryList == nullptr) {
        return GRD_API_NOT_SUPPORT;
    }
    return g_tokenizerApi.tokenizerFreeWordEntryList(list);
}

GRD_API int32_t GRD_TokenizerDestroy(void *ptr)
{
    (void)ptr;
    if (g_tokenizerApi.tokenizerDestroy == nullptr) {
        g_tokenizerApi = GetTokenizerApi();
    }
    if (g_tokenizerApi.tokenizerDestroy == nullptr) {
        return GRD_API_NOT_SUPPORT;
    }
    g_tokenizerApi.tokenizerDestroy();
    return GRD_OK;
}
// LCOV_EXCL_STOP
}  // namespace CNTokenizer
