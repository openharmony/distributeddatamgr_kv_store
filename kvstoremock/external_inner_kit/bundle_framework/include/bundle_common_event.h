/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_COMMON_EVENT_H
#define FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_COMMON_EVENT_H

namespace OHOS {
namespace AppExecFwk {
constexpr const char* COMMON_EVENT_SANDBOX_PACKAGE_ADDED = "usual.event.SANDBOX_PACKAGE_ADDED";
constexpr const char* COMMON_EVENT_SANDBOX_PACKAGE_REMOVED = "usual.event.SANDBOX_PACKAGE_REMOVED";
constexpr const char* COMMON_EVENT_BUNDLE_SCAN_FINISHED = "usual.event.BUNDLE_SCAN_FINISHED";
constexpr const char* OVERLAY_ADD_ACTION = "usual.event.OVERLAY_PACKAGE_ADDED";
constexpr const char* OVERLAY_CHANGED_ACTION = "usual.event.OVERLAY_PACKAGE_CHANGED";
constexpr const char* OVERLAY_STATE_CHANGED = "usual.event.OVERLAY_STATE_CHANGED";
constexpr const char* DISPOSED_RULE_ADDED = "usual.event.DISPOSED_RULE_ADDED";
constexpr const char* DISPOSED_RULE_DELETED = "usual.event.DISPOSED_RULE_DELETED";
constexpr const char* DYNAMIC_ICON_CHANGED = "usual.event.DYNAMIC_ICON_CHANGED";
constexpr const char* DEFAULT_APPLICATION_CHANGED = "usual.event.DEFAULT_APPLICATION_CHANGED";
constexpr const char* PLUGIN_PACKAGE_CHANGED = "usual.event.PLUGIN_PACKAGE_CHANGED";
constexpr const char* PLUGIN_PACKAGE_ADDED = "usual.event.PLUGIN_PACKAGE_ADDED";
constexpr const char* PLUGIN_PACKAGE_REMOVED = "usual.event.PLUGIN_PACKAGE_REMOVED";
constexpr const char* SKILL_CHANGED = "usual.event.SKILL_CHANGED";
} // AppExecFwk
} // OHOS
#endif // FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_COMMON_EVENT_H
