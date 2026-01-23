/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DISTRIBUTED_KVSTORE_MOCK_HKS_PARAM_H
#define DISTRIBUTED_KVSTORE_MOCK_HKS_PARAM_H

#include "hks_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init parameter set
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
static int32_t HksInitParamSet(struct HksParamSet **paramSet)
{
    return HKS_SUCCESS;
}

/**
 * @brief Add parameter set
 * @param paramSet required parameter set
 * @param params params need to add
 *
 * @param paramCnt numbers of params
 * @return error code, see hks_type.h
 */
static int32_t HksAddParams(struct HksParamSet *paramSet, const struct HksParam *params, uint32_t paramCnt)
{
    return HKS_SUCCESS;
}

/**
 * @brief Build parameter set
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
static int32_t HksBuildParamSet(struct HksParamSet **paramSet)
{
    return HKS_SUCCESS;
}

/**
 * @brief Free parameter set
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
static void HksFreeParamSet(struct HksParamSet **paramSet)
{
    return;
}

#ifdef __cplusplus
}
#endif
#endif // DISTRIBUTED_KVSTORE_MOCK_HKS_PARAM_H
