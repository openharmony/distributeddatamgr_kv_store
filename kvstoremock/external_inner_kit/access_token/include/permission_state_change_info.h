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
 * @file permission_state_change_info.h
 *
 * @brief Declares PermStateChangeInfo and PermStateChangeScope struct.
 *
 * @since 7.0
 * @version 7.0
 */

#ifndef INTERFACES_INNER_KITS_PERMISSION_STATE_CHANGE_INFO_H
#define INTERFACES_INNER_KITS_PERMISSION_STATE_CHANGE_INFO_H

#include <string>
#include <vector>

#include "access_token.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
/** define tokenID list max size */
#define TOKENIDS_LIST_SIZE_MAX 1024
/** define permission list max size */
#define PERMS_LIST_SIZE_MAX 1024

/**
 * @brief Declares permission state change type
 */
enum PermStateChangeType {
    STATE_CHANGE_REVOKED = 0,
    STATE_CHANGE_GRANTED = 1,
};

/**
 * @brief Declares permission state change info struct
 */
struct PermStateChangeInfo {
    /**
     * permission state change type, for details about the valid values,
     * see the definition of PermStateChangeType.
     */
    int32_t permStateChangeType;
    AccessTokenID tokenID;
    std::string permissionName;
};

/**
 * @brief Declares permission state change scope struct
 */
struct PermStateChangeScope {
    /**
     * indicates which tokenID to listen the permission state change,
     * empty means listen all tokenIDs in the device
     */
    std::vector<AccessTokenID> tokenIDs;
    /**
     * indicates which permission to listen the state change,
     * empty means listen all permission state changes in the device
     */
    std::vector<std::string> permList;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS

#endif // INTERFACES_INNER_KITS_PERMISSION_STATE_CHANGE_INFO_H
