/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
 * @file perm_state_change_callback_customize.h
 *
 * @brief Declares permission state change callback class.
 *
 * @since 8.0
 * @version 8.0
 */

#ifndef INTERFACES_INNER_KITS_PERM_STATE_CALLBACK_CUSTOMIZE_H
#define INTERFACES_INNER_KITS_PERM_STATE_CALLBACK_CUSTOMIZE_H

#include "permission_state_change_info.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
/**
 * @brief Declares permission state change callback customize class
 */
class PermStateChangeCallbackCustomize {
public:
    /**
     * @brief Constructor without any param.
     */
    PermStateChangeCallbackCustomize();
    /**
     * @brief Constructor with param.
     * @param scopeInfo struct PermStateChangeScope quote
     */
    explicit PermStateChangeCallbackCustomize(const PermStateChangeScope &scopeInfo);
    /**
     * @brief Destructor without any param.
     */
    virtual ~PermStateChangeCallbackCustomize();

    /**
     * @brief Pure virtual function for callback.
     * @param result PermStateChangeInfo quote
     */
    virtual void PermStateChangeCallback(PermStateChangeInfo& result) = 0;

    /**
     * @brief Get private variable scopeInfo_.
     * @param scopeInfo struct PermStateChangeScope quote as return value
     */
    void GetScope(PermStateChangeScope &scopeInfo) const;

private:
    /** private variable struct PermStateChangeScope */
    PermStateChangeScope scopeInfo_;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS

#endif  // INTERFACES_INNER_KITS_PERM_STATE_CALLBACK_CUSTOMIZE_H