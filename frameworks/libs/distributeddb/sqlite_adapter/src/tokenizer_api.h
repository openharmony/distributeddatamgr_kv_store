/*

Copyright (c) 2025 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef GRD_TOKENIZER_API_H
#define GRD_TOKENIZER_API_H

#include <cstdint>
#include "tokenizer_export_type.h"

namespace CNTokenizer {

int32_t GRD_TokenizerInit(const char *userDictPath, const char *userStopWordPath, GRD_TokenizerParamT param);

int32_t GRD_TokenizerCut(const char *sentence, GRD_CutOptionT option, GRD_WordEntryListT **entryList);

int32_t GRD_TokenizerExtract(const char *sentence, GRD_ExtractOptionT option, GRD_WordEntryListT **entryList);

int32_t GRD_TokenizerNext(GRD_WordEntryListT *list, GRD_WordEntryT *entry);

int32_t GRD_TokenizerFreeWordEntryList(GRD_WordEntryListT *list);

int32_t GRD_TokenizerDestroy(void *ptr);

}

#endif // GRD_TOKENIZER_API_H
