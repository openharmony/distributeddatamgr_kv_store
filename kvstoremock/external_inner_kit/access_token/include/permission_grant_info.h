/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @file permission_grant_info.h
 *
 * @brief Declares permission grant info class.
 *
 * @since 11.0
 * @version 11.0
 */

#ifndef INTERFACES_INNER_KITS_ACCESSTOKEN_PERMISSION_GRANT_INFO_H
#define INTERFACES_INNER_KITS_ACCESSTOKEN_PERMISSION_GRANT_INFO_H

#include <string>

namespace OHOS {
namespace Security {
namespace AccessToken {
/**
 * @brief Declares permission dlp mode class
 */
class PermissionGrantInfo final {
public:
    /**
     * permission manager application bundle name
     */
    std::string grantBundleName;
    /**
     * permission grant ability name
     */
    std::string grantAbilityName;
    /**
     * permission grant service ability name
     */
    std::string grantServiceAbilityName;
    /**
     * permission state sheet ability name
     */
    std::string permStateAbilityName;
    /**
     * global switch sheet ability name
     */
    std::string globalSwitchAbilityName;
    /**
     * open setting ability name
     */
    std::string openSettingAbilityName;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS

#endif // INTERFACES_INNER_KITS_ACCESSTOKEN_PERMISSION_GRANT_INFO_H
