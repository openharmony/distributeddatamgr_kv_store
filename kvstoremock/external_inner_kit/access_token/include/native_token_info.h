/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

/**
 * @addtogroup AccessToken
 * @{
 *
 * @brief Provides permission management interfaces.
 *
 * Provides tokenID-based application permission verification mechanism.
 * When an application accesses sensitive data or APIs, this module can check
 * whether the application has the corresponding permission. Allows applications
 * to query their access token information or APL levcels based on token IDs.
 *
 * @since 7.0
 * @version 7.0
 */

/**
 * @file native_token_info.h
 *
 * @brief Declares native token infos.
 *
 * @since 7.0
 * @version 7.0
 */

#ifndef ACCESSTOKEN_NATIVE_TOKEN_INFO_H
#define ACCESSTOKEN_NATIVE_TOKEN_INFO_H

#include "access_token.h"
#include <string>
#include <vector>
#include "permission_state_full.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
/**
 * @brief Declares native token info class
 */
class NativeTokenInfo final {
public:
    /**
     * apl level, for details about the valid values,
     * see the definition of ATokenAplEnum in the access_token.h file.
     */
    ATokenAplEnum apl;
    /** native process name */
    std::string processName;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // ACCESSTOKEN_NATIVE_TOKEN_INFO_H
