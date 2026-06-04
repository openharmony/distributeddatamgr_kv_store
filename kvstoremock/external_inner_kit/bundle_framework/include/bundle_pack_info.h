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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_PACK_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_PACK_INFO_H

#include <string>
#include <vector>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {

enum BundlePackFlag {
    GET_PACK_INFO_ALL = 0x00000000,
    GET_PACKAGES = 0x00000001,
    GET_BUNDLE_SUMMARY = 0x00000002,
    GET_MODULE_SUMMARY = 0x00000004,
};

struct Version {
    uint32_t code = 0;
    uint32_t minCompatibleVersionCode = 0;
    std::string name;
};

struct PackageApp {
    std::string bundleName;
    Version version;
};

struct AbilityFormInfo {
    bool updateEnabled = false;
    uint32_t updateDuration = 0;
    std::string name;
    std::string type;
    std::string scheduledUpdateTime;
    std::string multiScheduledUpdateTime;
    std::string defaultDimension;
    std::vector<std::string> supportDimensions;
};

struct ModuleAbilityInfo {
    bool visible = false;
    std::string name;
    std::string label;
    std::vector<AbilityFormInfo> forms;
};

struct ModuleDistro {
    bool installationFree = false;
    bool deliveryWithInstall = false;
    std::string moduleType;
    std::string moduleName;
};

struct ApiVersion {
    uint32_t compatible = 0;
    uint32_t target = 0;
    std::string releaseType;
};

struct ExtensionAbilities {
    std::string name;
    std::vector<AbilityFormInfo> forms;
};

struct PackageModule {
    std::string mainAbility;
    std::vector<std::string> deviceType;
    std::vector<ModuleAbilityInfo> abilities;
    std::vector<ExtensionAbilities> extensionAbilities;
    ModuleDistro distro;
    ApiVersion apiVersion;
};

struct Summary {
    std::vector<PackageModule> modules;
    PackageApp app;
};

struct Packages {
    bool deliveryWithInstall = false;
    std::string moduleType;
    std::string name;
    std::vector<std::string> deviceType;
};

struct BundlePackInfo : public Parcelable {
    std::vector<Packages> packages;
    Summary summary;

    bool GetValid() const
    {
        return isValid_;
    }
    void SetValid(bool isValid)
    {
        isValid_ = isValid;
    }
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static BundlePackInfo *Unmarshalling(Parcel &parcel);
private:
    bool isValid_ = false;
};
} // AppExecFwk
} // OHOS
#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_PACK_INFO_H
