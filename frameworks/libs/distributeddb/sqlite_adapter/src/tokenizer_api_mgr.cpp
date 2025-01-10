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
#include <string>
#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifndef _WIN32
static void *g_library = nullptr;
#endif

namespace CNTokenizer {

void LoadTokenizerApi(GRD_CNTokenizerApiT &tokenizerApi)
{
#ifndef _WIN32
    tokenizerApi.tokenizerInit = (TokenizerInit)dlsym(g_library, "GRD_TokenizerInit");
    tokenizerApi.tokenizerCut = (TokenizerCut)dlsym(g_library, "GRD_TokenizerCut");
    tokenizerApi.tokenizerExtract = (TokenizerExtract)dlsym(g_library, "GRD_TokenizerExtract");
    tokenizerApi.tokenizerNext = (TokenizerNext)dlsym(g_library, "GRD_TokenizerNext");
    tokenizerApi.tokenizerFreeWordEntryList =
        (TokenizerFreeWordEntryList)dlsym(g_library, "GRD_TokenizerFreeWordEntryList");
    tokenizerApi.tokenizerDestroy = (TokenizerDestroy)dlsym(g_library, "GRD_TokenizerDestroy");
#endif
}

GRD_CNTokenizerApiT GetTokenizerApi()
{
    GRD_CNTokenizerApiT tokenizerApi = {};
#ifdef _WIN32
    return tokenizerApi;
#endif
    std::string libPath = "libarkdata_tokenizer.z.so";
#ifndef HARMONY_OS
    libPath = "libgrd_store.so";
#endif
    g_library = dlopen(libPath.c_str(), RTLD_LAZY);
    if (g_library) {
        LoadTokenizerApi(tokenizerApi);
    }
    return tokenizerApi;
}
}  // namespace CNTokenizer
