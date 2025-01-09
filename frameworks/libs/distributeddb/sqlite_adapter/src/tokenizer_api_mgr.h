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

#ifndef TOKENIZER_API_MGR_H
#define TOKENIZER_API_MGR_H

#include "tokenizer_export_type.h"

namespace CNTokenizer {
typedef int32_t (*TokenizerInit)(const char *userDictPath, const char *userStopWordPath, GRD_TokenizerParamT param);

typedef int32_t (*TokenizerCut)(const char *sentence, GRD_CutOptionT option, GRD_WordEntryListT **entryList);

typedef int32_t (*TokenizerExtract)(const char *sentence, GRD_ExtractOptionT option, GRD_WordEntryListT **entryList);

typedef int32_t (*TokenizerNext)(GRD_WordEntryListT *list, GRD_WordEntryT *entry);

typedef int32_t (*TokenizerFreeWordEntryList)(GRD_WordEntryListT *list);

typedef void (*TokenizerDestroy)(void);

typedef struct GRD_CNTokenizerApi {
    TokenizerCut tokenizerCut = nullptr;
    TokenizerExtract tokenizerExtract = nullptr;
    TokenizerNext tokenizerNext = nullptr;
    TokenizerFreeWordEntryList tokenizerFreeWordEntryList = nullptr;
    TokenizerInit tokenizerInit = nullptr;
    TokenizerDestroy tokenizerDestroy = nullptr;
} GRD_CNTokenizerApiT;

GRD_CNTokenizerApiT GetTokenizerApi();

void InitApiInfo(const char *configStr);
}  // namespace CNTokenizer
#endif  // TOKENIZER_API_MGR_H
