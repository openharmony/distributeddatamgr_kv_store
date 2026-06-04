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

#ifndef HUKS_API_H
#define HUKS_API_H

#include <cstdint>
#include <vector>

namespace OHOS {
namespace Security {
namespace Huks {

enum HuksResultCode {
    HUKS_SUCCESS = 0,
    HUKS_FAILURE = -1,
};

struct HuksBlob {
    uint32_t size = 0;
    uint8_t* data = nullptr;
};

struct HuksParam {
    uint32_t tag = 0;
    HuksBlob blob;
};

struct HuksParamSet {
    uint32_t paramCnt = 0;
    HuksParam* params = nullptr;
};

int32_t HuksGenerateKey(const HuksBlob* keyAlias, const HuksParamSet* paramSetIn,
                        HuksParamSet* paramSetOut) {
    return HUKS_SUCCESS;
}

int32_t HuksInit(const HuksBlob* keyAlias, const HuksParamSet* paramSetIn,
                 HuksBlob* handle) {
    return HUKS_SUCCESS;
}

int32_t HuksUpdate(const HuksBlob* handle, const HuksParamSet* paramSetIn,
                   const HuksBlob* inData, HuksBlob* outData) {
    return HUKS_SUCCESS;
}

int32_t HuksFinish(const HuksBlob* handle, const HuksParamSet* paramSetIn,
                   const HuksBlob* inData, HuksBlob* outData) {
    return HUKS_SUCCESS;
}

int32_t HuksAbort(const HuksBlob* handle, const HuksParamSet* paramSetIn) {
    return HUKS_SUCCESS;
}

int32_t HuksDeleteKey(const HuksBlob* keyAlias, const HuksParamSet* paramSetIn) {
    return HUKS_SUCCESS;
}

int32_t HuksGetKeyParamSet(const HuksBlob* keyAlias, const HuksParamSet* paramSetIn,
                           HuksParamSet* paramSetOut) {
    return HUKS_SUCCESS;
}

int32_t HuksExportPublicKey(const HuksBlob* keyAlias, const HuksParamSet* paramSetIn,
                             HuksBlob* publicKey) {
    return HUKS_SUCCESS;
}

int32_t HuksImportKey(const HuksBlob* keyAlias, const HuksParamSet* paramSetIn,
                      const HuksBlob* key) {
    return HUKS_SUCCESS;
}

}
}
}

#endif // HUKS_API_H