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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_JSON_SERIALIZER_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_JSON_SERIALIZER_H

#include "bundle_info.h"
#include "bundle_pack_info.h"
#include "bundle_user_info.h"
#include "code_protect_bundle_info.h"
#include "common_event_info.h"
#include "data_group_info.h"
#include "distributed_ability_info.h"
#include "distributed_module_info.h"
#include "form_info.h"
#include "distributed_manager/rpc_id_result.h"
#include "free_install/dispatch_info.h"
#include "free_install/install_result.h"
#include "free_install/target_ability_info.h"
#include "nlohmann/json.hpp"
#include "plugin/plugin_bundle_info.h"
#include "plugin/plugin_module_info.h"
#include "quick_fix/app_quick_fix.h"
#include "shared/shared_bundle_info.h"
#include "shared/shared_module_info.h"
#include "shortcut_info.h"

namespace OHOS {
namespace AppExecFwk {

/*
 * form_json and to_json is global static overload method, which need callback by json library,
 * and can not rename this function, so don't named according UpperCamelCase style
 */
void to_json(nlohmann::json &jsonObject, const CustomizeData &customizeData);
void from_json(const nlohmann::json &jsonObject, CustomizeData &customizeData);
void to_json(nlohmann::json &jsonObject, const MetaData &metaData);
void from_json(const nlohmann::json &jsonObject, MetaData &metaData);
void to_json(nlohmann::json &jsonObject, const HnpPackage &hnpPackage);
void from_json(const nlohmann::json &jsonObject, HnpPackage &hnpPackage);
void to_json(nlohmann::json &jsonObject, const Metadata &metadata);
void from_json(const nlohmann::json &jsonObject, Metadata &metadata);
void to_json(nlohmann::json &jsonObject, const AbilityInfo &abilityInfo);
void from_json(const nlohmann::json &jsonObject, AbilityInfo &abilityInfo);
void to_json(nlohmann::json &jsonObject, const ExtensionAbilityInfo &extensionInfo);
void from_json(const nlohmann::json &jsonObject, ExtensionAbilityInfo &extensionInfo);
void to_json(nlohmann::json &jsonObject, const MultiAppModeData &multiAppMode);
void from_json(const nlohmann::json &jsonObject, MultiAppModeData &multiAppMode);
void to_json(nlohmann::json &jsonObject, const ApplicationInfo &applicationInfo);
void from_json(const nlohmann::json &jsonObject, ApplicationInfo &applicationInfo);
void to_json(nlohmann::json &jsonObject, const BundleInfo &bundleInfo);
void from_json(const nlohmann::json &jsonObject, BundleInfo &bundleInfo);
void to_json(nlohmann::json &jsonObject, const HapHashAndDeveloperCert &hapHashAndDeveloperCert);
void from_json(const nlohmann::json &jsonObject, HapHashAndDeveloperCert &hapHashAndDeveloperCert);
void to_json(nlohmann::json &jsonObject, const BundleInfoForException &bundleInfoForException);
void from_json(const nlohmann::json &jsonObject, BundleInfoForException &bundleInfoForException);
void to_json(nlohmann::json &jsonObject, const ModuleInfo &moduleInfo);
void from_json(const nlohmann::json &jsonObject, ModuleInfo &moduleInfo);
void to_json(nlohmann::json &jsonObject, const FormInfo &formInfo);
void from_json(const nlohmann::json &jsonObject, FormInfo &formInfo);
void to_json(nlohmann::json &jsonObject, const ShortcutInfo &shortcutInfo);
void from_json(const nlohmann::json &jsonObject, ShortcutInfo &shortcutInfo);
void to_json(nlohmann::json &jsonObject, const CommonEventInfo &commonEvent);
void from_json(const nlohmann::json &jsonObject, CommonEventInfo &commonEvent);
void to_json(nlohmann::json &jsonObject, const HapModuleInfo &hapModuleInfo);
void from_json(const nlohmann::json &jsonObject, HapModuleInfo &hapModuleInfo);
void to_json(nlohmann::json& jsonObject, const BundleUserInfo& bundleUserInfo);
void from_json(const nlohmann::json& jsonObject, BundleUserInfo& bundleUserInfo);
void to_json(nlohmann::json &jsonObject, const RequestPermissionUsedScene &usedScene);
void from_json(const nlohmann::json &jsonObject, RequestPermissionUsedScene &usedScene);
void to_json(nlohmann::json &jsonObject, const RequestPermission &requestPermission);
void from_json(const nlohmann::json &jsonObject, RequestPermission &requestPermission);
void to_json(nlohmann::json &jsonObject, const SimpleAppInfo &simpleAppInfo);
void to_json(nlohmann::json& jsonObject, const DistributedAbilityInfo& distributedAbilityInfo);
void from_json(const nlohmann::json& jsonObject, DistributedAbilityInfo& distributedAbilityInfo);
void to_json(nlohmann::json& jsonObject, const DistributedModuleInfo& distributedModuleInfo);
void from_json(const nlohmann::json& jsonObject, DistributedModuleInfo& distributedModuleInfo);
void to_json(nlohmann::json &jsonObject, const TargetInfo &targetInfo);
void from_json(const nlohmann::json &jsonObject, TargetInfo &targetInfo);
void to_json(nlohmann::json &jsonObject, const TargetExtSetting &targetExtSetting);
void from_json(const nlohmann::json &jsonObject, TargetExtSetting &targetExtSetting);
void to_json(nlohmann::json &jsonObject, const Result &result);
void from_json(const nlohmann::json &jsonObject, Result &result);
void to_json(nlohmann::json &jsonObject, const Progress &progress);
void from_json(const nlohmann::json &jsonObject, Progress &progress);
void to_json(nlohmann::json &jsonObject, const TargetAbilityInfo &targetAbilityInfo);
void from_json(const nlohmann::json &jsonObject, TargetAbilityInfo &targetAbilityInfo);
void to_json(nlohmann::json &jsonObject, const InstallResult &installResult);
void from_json(const nlohmann::json &jsonObject, InstallResult &installResult);
void to_json(nlohmann::json &jsonObject, const DispatcherInfo &dispatcherInfo);
void from_json(const nlohmann::json &jsonObject, DispatcherInfo &dispatcherInfo);
void to_json(nlohmann::json &jsonObject, const Version &version);
void from_json(const nlohmann::json &jsonObject, Version &version);
void to_json(nlohmann::json &jsonObject, const PackageApp &app);
void from_json(const nlohmann::json &jsonObject, PackageApp &app);
void to_json(nlohmann::json &jsonObject, const ModuleAbilityInfo &abilityinfo);
void from_json(const nlohmann::json &jsonObject, ModuleAbilityInfo &abilityinfo);
void to_json(nlohmann::json &jsonObject, const ExtensionAbilities &extensionAbilities);
void from_json(const nlohmann::json &jsonObject, ExtensionAbilities &extensionAbilities);
void to_json(nlohmann::json &jsonObject, const AbilityFormInfo &abilityinfo);
void from_json(const nlohmann::json &jsonObject, AbilityFormInfo &abilityinfo);
void to_json(nlohmann::json &jsonObject, const ModuleDistro &distro);
void from_json(const nlohmann::json &jsonObject, ModuleDistro &distro);
void to_json(nlohmann::json &jsonObject, const ApiVersion &apiVersion);
void from_json(const nlohmann::json &jsonObject, ApiVersion &apiVersion);
void to_json(nlohmann::json &jsonObject, const PackageModule &packageModule);
void from_json(const nlohmann::json &jsonObject, PackageModule &packageModule);
void to_json(nlohmann::json &jsonObject, const Summary &summary);
void from_json(const nlohmann::json &jsonObject, Summary &summary);
void to_json(nlohmann::json &jsonObject, const Packages &packages);
void from_json(const nlohmann::json &jsonObject, Packages &packages);
void to_json(nlohmann::json &jsonObject, const BundlePackInfo &bundlePackInfo);
void from_json(const nlohmann::json &jsonObject, BundlePackInfo &bundlePackInfo);
void to_json(nlohmann::json &jsonObject, const Resource &resource);
void from_json(const nlohmann::json &jsonObject, Resource &resource);
void to_json(nlohmann::json &jsonObject, const SummaryAbilityInfo &summaryAbilityInfo);
void from_json(const nlohmann::json &jsonObject, SummaryAbilityInfo &summaryAbilityInfo);
void to_json(nlohmann::json &jsonObject, const RpcIdResult &rpcIdResult);
void from_json(const nlohmann::json &jsonObject, RpcIdResult &rpcIdResult);
void to_json(nlohmann::json &jsonObject, const AppQuickFix &appQuickFix);
void from_json(const nlohmann::json &jsonObject, AppQuickFix &appQuickFix);
void to_json(nlohmann::json &jsonObject, const AppqfInfo &appqfInfo);
void from_json(const nlohmann::json &jsonObject, AppqfInfo &appqfInfo);
void to_json(nlohmann::json &jsonObject, const HqfInfo &hqfInfo);
void from_json(const nlohmann::json &jsonObject, HqfInfo &hqfInfo);
void to_json(nlohmann::json &jsonObject, const ShortcutIntent &shortcutIntent);
void from_json(const nlohmann::json &jsonObject, ShortcutIntent &shortcutIntent);
void to_json(nlohmann::json &jsonObject, const ShortcutInfo &shortcutInfo);
void from_json(const nlohmann::json &jsonObject, ShortcutInfo &shortcutInfo);
void from_json(const nlohmann::json &jsonObject, ShortcutWant &shortcutWant);
void from_json(const nlohmann::json &jsonObject, Shortcut &shortcut);
void from_json(const nlohmann::json &jsonObject, ShortcutJson &shortcutJson);
void to_json(nlohmann::json &jsonObject, const OverlayBundleInfo &overlayBundleInfo);
void from_json(const nlohmann::json &jsonObject, OverlayBundleInfo &overlayBundleInfo);
void to_json(nlohmann::json &jsonObject, const OverlayModuleInfo &overlayModuleInfo);
void from_json(const nlohmann::json &jsonObject, OverlayModuleInfo &overlayModuleInfo);
void to_json(nlohmann::json &jsonObject, const Dependency &dependency);
void from_json(const nlohmann::json &jsonObject, Dependency &dependency);
void to_json(nlohmann::json &jsonObject, const SharedBundleInfo &sharedBundleInfo);
void from_json(const nlohmann::json &jsonObject, SharedBundleInfo &sharedBundleInfo);
void to_json(nlohmann::json &jsonObject, const SharedModuleInfo &sharedModuleInfo);
void from_json(const nlohmann::json &jsonObject, SharedModuleInfo &sharedModuleInfo);
void to_json(nlohmann::json &jsonObject, const ProxyData &proxyData);
void from_json(const nlohmann::json &jsonObject, ProxyData &proxyData);
void to_json(nlohmann::json &jsonObject, const DataGroupInfo &dataGroupInfo);
void from_json(const nlohmann::json &jsonObject, DataGroupInfo &dataGroupInfo);
void to_json(nlohmann::json &jsonObject, const RouterItem &routerItem);
void from_json(const nlohmann::json &jsonObject, RouterItem &routerItem);
void to_json(nlohmann::json &jsonObject, const AppEnvironment &appEnvironment);
void from_json(const nlohmann::json &jsonObject, AppEnvironment &appEnvironment);
void to_json(nlohmann::json &jsonObject, const ExecutableBinaryPath &executableBinaryPath);
void from_json(const nlohmann::json &jsonObject, ExecutableBinaryPath &executableBinaryPath);
void to_json(nlohmann::json &jsonObject, const ApplicationEnvironment &applicationEnvironment);
void from_json(const nlohmann::json &jsonObject, ApplicationEnvironment &applicationEnvironment);
void to_json(nlohmann::json &jsonObject, const Skill &skill);
void from_json(const nlohmann::json &jsonObject, Skill &skill);
void to_json(nlohmann::json &jsonObject, const CodeProtectBundleInfo &codeProtectBundleInfo);
void from_json(const nlohmann::json &jsonObject, CodeProtectBundleInfo &codeProtectBundleInfo);
void to_json(nlohmann::json &jsonObject, const PluginBundleInfo &pluginBundleInfo);
void from_json(const nlohmann::json &jsonObject, PluginBundleInfo &pluginBundleInfo);
void to_json(nlohmann::json &jsonObject, const PluginModuleInfo &pluginModuleInfo);
void from_json(const nlohmann::json &jsonObject, PluginModuleInfo &pluginModuleInfo);
void to_json(nlohmann::json &jsonObject, const StartWindowResource &startWindowResource);
void from_json(const nlohmann::json &jsonObject, StartWindowResource &startWindowResource);
void to_json(nlohmann::json &jsonObject, const AssetGroupInfo &assetGroupInfo);
void from_json(const nlohmann::json &jsonObject, AssetGroupInfo &assetGroupInfo);
void to_json(nlohmann::json &jsonObject, const AlternateIcon &alternateIcon);
void from_json(const nlohmann::json &jsonObject, AlternateIcon &alternateIcon);
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_JSON_SERIALIZER_H
