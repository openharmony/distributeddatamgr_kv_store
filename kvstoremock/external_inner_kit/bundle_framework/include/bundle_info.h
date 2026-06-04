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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_INFO_H

#include <string>
#include <vector>

#include "parcel.h"

#include "ability_info.h"
#include "application_info.h"
#include "extension_ability_info.h"
#include "hap_module_info.h"
#include "message_parcel.h"
#include "overlay/overlay_bundle_info.h"

namespace OHOS {
namespace AppExecFwk {
enum BundleFlag {
    // get bundle info except abilityInfos
    GET_BUNDLE_DEFAULT = 0x00000000,
    // get bundle info include abilityInfos
    GET_BUNDLE_WITH_ABILITIES = 0x00000001,
    // get bundle info include request permissions
    GET_BUNDLE_WITH_REQUESTED_PERMISSION = 0x00000010,
    // get bundle info include extension info
    GET_BUNDLE_WITH_EXTENSION_INFO = 0x00000020,
    // get bundle info include hash value
    GET_BUNDLE_WITH_HASH_VALUE = 0x00000030,
    // get bundle info inlcude menu, only for dump usage
    GET_BUNDLE_WITH_MENU = 0x00000040,
    // get bundle info inlcude router map, only for dump usage
    GET_BUNDLE_WITH_ROUTER_MAP = 0x00000080,
    // get bundle info include skill info
    GET_BUNDLE_WITH_SKILL = 0x00000800,
    // the flag only for C++ interface passing
    GET_BUNDLE_INFO_EXCLUDE_EXT = 0x00020000,
    // get bundle info include request permissions with no detailed info
    GET_BUNDLE_WITH_REQUESTED_PERMISSION_NO_DETAILED = 0x80000000,
};

enum class GetBundleInfoFlag {
    GET_BUNDLE_INFO_DEFAULT = 0x00000000,
    GET_BUNDLE_INFO_WITH_APPLICATION = 0x00000001,
    GET_BUNDLE_INFO_WITH_HAP_MODULE = 0x00000002,
    GET_BUNDLE_INFO_WITH_ABILITY = 0x00000004,
    GET_BUNDLE_INFO_WITH_EXTENSION_ABILITY = 0x00000008,
    GET_BUNDLE_INFO_WITH_REQUESTED_PERMISSION = 0x00000010,
    GET_BUNDLE_INFO_WITH_METADATA = 0x00000020,
    GET_BUNDLE_INFO_WITH_DISABLE = 0x00000040,
    GET_BUNDLE_INFO_WITH_SIGNATURE_INFO = 0x00000080,
    GET_BUNDLE_INFO_WITH_MENU = 0x00000100,
    GET_BUNDLE_INFO_WITH_ROUTER_MAP = 0x00000200,
    GET_BUNDLE_INFO_WITH_SKILL = 0x00000800,
    GET_BUNDLE_INFO_ONLY_WITH_LAUNCHER_ABILITY = 0x00001000,
    GET_BUNDLE_INFO_OF_ANY_USER = 0x00002000,
    GET_BUNDLE_INFO_EXCLUDE_CLONE = 0x00004000,
    GET_BUNDLE_INFO_WITH_CLOUD_KIT = 0x00008000,
    GET_BUNDLE_INFO_WITH_ENTRY_MODULE = 0x00010000,
    // the flag only for C++ interface passing
    GET_BUNDLE_INFO_EXCLUDE_EXT = 0x00020000,
};

struct RequestPermissionUsedScene : public Parcelable {
    std::string when;
    std::vector<std::string> abilities;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static RequestPermissionUsedScene *Unmarshalling(Parcel &parcel);
};

struct RequestPermission : public Parcelable {
    uint32_t reasonId = 0;
    std::string name;
    std::string moduleName;
    std::string reason;
    std::string requiredFeature;
    RequestPermissionUsedScene usedScene;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static RequestPermission *Unmarshalling(Parcel &parcel);
};

struct SignatureInfo : public Parcelable {
    std::string appId;
    std::string fingerprint;
    std::string appIdentifier;
    std::string certificate;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static SignatureInfo *Unmarshalling(Parcel &parcel);
};

enum class BundleInstallStatus : uint8_t {
    UNKNOWN_STATUS = 0,
    BUNDLE_NOT_EXIST = 1,
    BUNDLE_INSTALLING = 2,
    BUNDLE_INSTALLED = 3,
};

struct SimpleAppInfo : public Parcelable {
    int32_t uid = -1;
    std::string bundleName;
    int32_t appIndex = -1;
    ErrCode ret = ERR_OK;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static SimpleAppInfo *Unmarshalling(Parcel &parcel);
    std::string ToString() const;
};

struct HapHashAndDeveloperCert : public Parcelable {
    std::string path;
    std::string hash;
    std::string developCert;
 
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static HapHashAndDeveloperCert *Unmarshalling(Parcel &parcel);
};
 
// configuration information about a bundle
struct BundleInfoForException : public Parcelable {
    std::vector<std::string> allowedAcls;
    std::vector<std::string> abilityNames;
    std::vector<HapHashAndDeveloperCert> hapHashValueAndDevelopCerts;
    // key: soname, value:hash
    std::map<std::string, std::string> soHash;
 
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static BundleInfoForException *Unmarshalling(Parcel &parcel);
};

// configuration information about a bundle
struct BundleInfo : public Parcelable {
    bool isNewVersion = false;
    bool isKeepAlive = false;
    bool singleton = false;
    bool isPreInstallApp = false;

    bool isNativeApp = false;

    bool entryInstallationFree = false; // application : false; atomic service : true
    bool isDifferentName = false;

    uint32_t versionCode = 0;
    uint32_t minCompatibleVersionCode = 0;

    uint32_t compatibleVersion = 0;
    uint32_t compatibleMinorVersion = 0;
    uint32_t compatiblePatchVersion = 0;
    uint32_t targetVersion = 0;
    int32_t targetMinorApiVersion = 0;
    int32_t targetPatchApiVersion = 0;
    int32_t appIndex = 0; // index for sandbox app
    int32_t minSdkVersion = -1;
    int32_t maxSdkVersion = -1;
    int32_t overlayType = NON_OVERLAY_TYPE;

    // user related fields, assign when calling the get interface
    int uid = -1;
    int gid = -1;
    int64_t installTime = 0;
    int64_t updateTime = 0;
    int64_t firstInstallTime = 0;
    std::string name;
    std::string versionName;
    std::string buildVersion;

    std::string vendor;
    std::string releaseType;
    std::string mainEntry; // modulePackage
    std::string entryModuleName; // moduleName
    std::string appId;

    std::string cpuAbi;
    std::string seInfo;
    std::string label;
    std::string description;
    std::string jointUserId;
    SignatureInfo signatureInfo;
    std::vector<std::string> oldAppIds; // used for appId changed

    std::vector<AbilityInfo> abilityInfos;
    std::vector<ExtensionAbilityInfo> extensionInfos;
    std::vector<HapModuleInfo> hapModuleInfos;
    std::vector<std::string> hapModuleNames;    // the "module.package" in each config.json
    std::vector<std::string> moduleNames;       // the "module.name" in each config.json
    std::vector<std::string> modulePublicDirs;  // the public paths of all modules of the application.
    std::vector<std::string> moduleDirs;        // the paths of all modules of the application.
    std::vector<std::string> moduleResPaths;    // the paths of all resources paths.

    std::vector<std::string> reqPermissions;
    std::vector<std::string> defPermissions;
    std::vector<int32_t> reqPermissionStates;
    std::vector<RequestPermission> reqPermissionDetails;
    std::vector<OverlayBundleInfo> overlayBundleInfos;

    std::vector<RouterItem> routerArray;
    ApplicationInfo applicationInfo;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static BundleInfo *Unmarshalling(Parcel &parcel);
};

struct AssetGroupInfo : public Parcelable {
    int32_t appIndex = 0;
    std::string bundleName;
    std::string appId;
    std::string appIdentifier;
    std::string developerId;
    std::vector<std::string> assetAccessGroups;
 
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AssetGroupInfo *Unmarshalling(Parcel &parcel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_INFO_H
