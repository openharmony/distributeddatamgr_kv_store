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

#ifndef GRD_TOKENNIZER_TYPE_H
#define GRD_TOKENNIZER_TYPE_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#define GRD_OK 0
#define GRD_API_NOT_SUPPORT (-8888)
#define GRD_NO_DATA (-11000)
#define GRD_FAILED_MEMORY_ALLOCATE (-13000)

#ifndef GRD_API
#ifndef _WIN32
#define GRD_API __attribute__((visibility("default")))
#else
#define GRD_API
#endif
#endif  // GRD_API

typedef enum GRD_CutMode {
    CUT_MMSEG = 0,
    CUT_BUTT  // INVALID TokenizeMode
} GRD_CutModeE;

typedef enum GRD_ExtractMode {
    Extract_TF_IDF = 0,
    Extract_BUTT  // INVALID ExtractMode
} GRD_ExtractModeE;

typedef struct GRD_TokenizerParam {
    GRD_CutModeE cutMode;
    GRD_ExtractModeE extractMode;
} GRD_TokenizerParamT;

typedef struct GRD_CutOption {
    bool needPreProcess;
} GRD_CutOptionT;

typedef struct GRD_ExtractOption {
    uint32_t topN;  // used for extract
} GRD_ExtractOptionT;

typedef struct GRD_WordEntryList GRD_WordEntryListT;

typedef struct GRD_WordEntry {
    const char *word;
    uint32_t length;
} GRD_WordEntryT;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // GRD_TOKENNIZER_TYPE_H
