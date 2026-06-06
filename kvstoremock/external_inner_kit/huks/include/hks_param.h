/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

/**
 * @file hks_param.h
 *
 * @brief Declares operate params interface.
 *
 * @since 8
 */

#ifndef HKS_PARAM_H
#define HKS_PARAM_H

#include "hks_type.h"

#define HKS_PARAM_SET_MAX_SIZE (4 * 1024 * 1024)
#define HKS_DEFAULT_PARAM_SET_SIZE 1024
#define HKS_DEFAULT_PARAM_CNT ((uint32_t)((HKS_DEFAULT_PARAM_SET_SIZE - sizeof(struct HksParamSet)) / \
    sizeof(struct HksParam)))
#define HKS_TAG_TYPE_MASK (0xF << 28)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init parameter set
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksInitParamSet(struct HksParamSet **paramSet);

/**
 * @brief Add parameter set
 * @param paramSet required parameter set
 * @param params params need to add
 *
 * @param paramCnt numbers of params
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksAddParams(struct HksParamSet *paramSet,
    const struct HksParam *params, uint32_t paramCnt);

/**
 * @brief Build parameter set
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksBuildParamSet(struct HksParamSet **paramSet);

/**
 * @brief Free parameter set
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT void HksFreeParamSet(struct HksParamSet **paramSet);

/**
 * @brief Get parameter
 * @param paramSet required parameter set
 * @param tag param's tag
 * @param param output param
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksGetParam(const struct HksParamSet *paramSet, uint32_t tag, struct HksParam **param);

#ifndef HKS_CHIPSET_API
/**
 * @brief Add parameter set and filter in black list way
 * @param paramSet required parameter set
 * @param params params need to add
 *
 * @param paramCnt numbers of params
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksAddParamsWithFilter(struct HksParamSet *paramSet,
    const struct HksParam *params, uint32_t paramCnt);

/**
 * @brief Free alias set
 * @param aliasSet required alias set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT void HksFreeKeyAliasSet(struct HksKeyAliasSet *aliasSet);

/**
 * @brief Free CertInfo set
 * @param CertInfoSet required CertInfo set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT void HksFreeExtCertSet(struct HksExtCertInfoSet *certInfoSet);

/**
 * @brief Get parameter set
 * @param inParamSet required parameter set
 * @param inParamSetSize input patamSet size
 * @param outParamSet output parameter set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksGetParamSet(const struct HksParamSet *inParamSet, uint32_t inParamSetSize,
    struct HksParamSet **outParamSet);

/**
 * @brief Fresh parameter set
 * @param paramSet required parameter set
 * @param isCopy is copy or not
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksFreshParamSet(struct HksParamSet *paramSet, bool isCopy);

/**
 * @brief Check param set tag
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksCheckParamSetTag(const struct HksParamSet *paramSet);

/**
 * @brief Check param set
 * @param paramSet required parameter set
 * @param size paramset size
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksCheckParamSet(const struct HksParamSet *paramSet, uint32_t size);

/**
 * @brief Check param whether match or not
 * @param baseParam one param
 * @param param another param
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksCheckParamMatch(const struct HksParam *baseParam, const struct HksParam *param);

/**
 * @brief Check param set tag
 * @param paramSet required parameter set
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksCheckParamSetTag(const struct HksParamSet *paramSet);

/**
 * @brief Check whether the tag exists
 * @param params required parameter
 * @param paramsCnt paramter size
 * @param targetParamSet target paramset
 * @return error code, see hks_type.h
 */
HKS_API_EXPORT int32_t HksCheckIsTagAlreadyExist(const struct HksParam *params, uint32_t paramsCnt,
    const struct HksParamSet *targetParamSet);

/**
 * @brief Get tag type
 * @param tag the tag
 * @return tag type, see hks_type.h
 */
HKS_API_EXPORT enum HksTagType GetTagType(enum HksTag tag);

HKS_API_EXPORT int32_t HksDeleteTagsFromParamSet(const uint32_t *tag, uint32_t tagCount,
    const struct HksParamSet *paramSet, struct HksParamSet **outParamSet);
#endif

#ifdef __cplusplus
}
#endif

#endif /* HKS_PARAM_H */
