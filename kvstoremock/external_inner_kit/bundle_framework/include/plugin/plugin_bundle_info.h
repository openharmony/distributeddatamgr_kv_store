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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_PLUGIN_BUNDLE_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_PLUGIN_BUNDLE_INFO_H

#include <string>
#include <unordered_map>
#include <vector>

#include "ability_info.h"
#include "hap_module_info.h"
#include "parcel.h"
#include "plugin_module_info.h"

namespace OHOS {
namespace AppExecFwk {
struct PluginBundleInfo : public Parcelable {
    uint32_t iconId = 0;
    uint32_t labelId = 0;
    uint32_t versionCode = 0;

    std::string pluginBundleName;
    std::string label;
    std::string icon;
    std::string versionName;
    std::string appIdentifier;
    std::string appId;
    std::string codePath;
    std::string nativeLibraryPath;
    std::vector<PluginModuleInfo> pluginModuleInfos;

    std::unordered_map<std::string, AbilityInfo> abilityInfos;
    std::unordered_map<std::string, ExtensionAbilityInfo> extensionInfos;
    ApplicationInfo appInfo;

    bool GetAbilityInfoByName(const std::string &abilityName, const std::string &moduleName, AbilityInfo &info);
    bool GetHapModuleInfo(const std::string &moduleName, HapModuleInfo &hapInfo);
    bool GetExtensionInfoByName(const std::string &extensionName,
        const std::string &moduleName, ExtensionAbilityInfo &info);

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static PluginBundleInfo *Unmarshalling(Parcel &parcel);
    bool ReadExtensionInfos(Parcel &parcel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_PLUGIN_BUNDLE_INFO_H
