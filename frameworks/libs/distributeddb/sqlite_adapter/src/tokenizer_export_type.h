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
#define GRD_INVALID_ARGS (-3000)
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

#define GRD_TOKENIZER_POS_LEN 4

typedef enum GRD_CutMode {
    CUT_MMSEG = 0,
    CUT_BUTT  // INVALID TokenizeMode
} GRD_CutModeE;

typedef enum GRD_CutScene {
    DEFAULT = 0,
    SEARCH = 1,
    SCENE_BUTT  // INVALID mode
} GRD_CutSceneE;

typedef enum GRD_ExtractMode {
    EXTRACT_TF_IDF = 0,
    EXTRACT_BUTT  // INVALID ExtractMode
} GRD_ExtractModeE;

#define POS_WEIGHT_LEN 58

typedef struct GRD_ExtractWeight {
    bool setWeight;
    uint8_t *posWeight;
    uint32_t posWeightLen;
    float shortWordWeight;
} GRD_ExtractWeightT;

typedef struct GRD_TokenizerParam {
    GRD_CutModeE cutMode;
    GRD_ExtractModeE extractMode;
    GRD_ExtractWeightT extractWeight;
} GRD_TokenizerParamT;

typedef struct GRD_CutOption {
    bool needPreProcess;
    GRD_CutSceneE cutScene;
    bool toLowerCase;
} GRD_CutOptionT;

#define EXTRACT_USE_POS_WEIGHT 1
#define EXTRACT_USE_SHORT_WORD_WEIGHT 2
#define EXTRACT_SKIP_PREFIX_WORD 4

typedef struct GRD_ExtractOption {
    uint32_t topN;  // used for extract
    uint32_t extractFlag;
} GRD_ExtractOptionT;

typedef struct GRD_WordEntryList GRD_WordEntryListT;

typedef struct GRD_WordEntry {
    const char *word;
    uint32_t length;
    char partOfSpeech[GRD_TOKENIZER_POS_LEN + 1];
    double idf;
    uint32_t offset;
} GRD_WordEntryT;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // GRD_TOKENNIZER_TYPE_H
