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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APPLICATION_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APPLICATION_INFO_H

#include <map>
#include <string>
#include <vector>

#include "bundle_constants.h"
#include "module_info.h"
#include "parcel.h"
#include "quick_fix/app_quick_fix.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
    constexpr const char* AVAILABLELEVEL_NORMAL = "normal";
    constexpr const char* DEFAULT_ENTITY_TYPE = "unspecified";
    constexpr const char* DEFAULT_COMPILE_SDK_TYPE = "OpenHarmony";
}
enum ApplicationFlag {
    GET_BASIC_APPLICATION_INFO = 0x00000000,
    GET_APPLICATION_INFO_WITH_PERMISSION = 0x00000008,
    GET_APPLICATION_INFO_WITH_METADATA = 0x00000040,
    GET_APPLICATION_INFO_WITH_DISABLE = 0x00000200,
    GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT = 0x00000400,
    GET_ALL_APPLICATION_INFO = 0xFFFF0000,
};

enum class GetApplicationFlag {
    GET_APPLICATION_INFO_DEFAULT = 0x00000000,
    GET_APPLICATION_INFO_WITH_PERMISSION = 0x00000001,
    GET_APPLICATION_INFO_WITH_METADATA = 0x00000002,
    GET_APPLICATION_INFO_WITH_DISABLE = 0x00000004,
};

enum class GetDependentBundleInfoFlag {
    GET_APP_CROSS_HSP_BUNDLE_INFO = 0x00000000,
    GET_APP_SERVICE_HSP_BUNDLE_INFO = 0x00000001,
    GET_ALL_DEPENDENT_BUNDLE_INFO = 0x00000002,
};

enum class BundleType {
    APP = 0,
    ATOMIC_SERVICE = 1,
    SHARED = 2,
    APP_SERVICE_FWK = 3,
    APP_PLUGIN = 4,
    SKILL = 5,
};

enum class CompatiblePolicy {
    NORMAL = 0,
    BACKWARD_COMPATIBILITY = 1,
};

// Each bit of this ApplicationReservedFlag value identifies relevant information
enum class ApplicationReservedFlag {
    ENCRYPTED_APPLICATION = 0x00000001,
    ENCRYPTED_KEY_EXISTED = 0x00000002,
};

enum class MultiAppModeType : uint8_t {
    UNSPECIFIED = 0,
    MULTI_INSTANCE = 1,
    APP_CLONE = 2,
};

enum class ApplicationInfoFlag {
    FLAG_INSTALLED = 0x00000001,
    /**
     * Indicates the installation source of pre-installed applications
     * App upgrades will not change installation source
     * FLAG_BOOT_INSTALLED App installed during first boot
     * FLAG_OTA_INSTALLED App installed during OTA
     * FLAG_RECOVER_INSTALLED App recover
     */
    FLAG_BOOT_INSTALLED = 0x00000002,
    FLAG_OTA_INSTALLED = 0x00000004,
    FLAG_RECOVER_INSTALLED = 0x00000008,
    FLAG_OTHER_INSTALLED = 0x00000010,
    FLAG_PREINSTALLED_APP = 0x00000020,
    FLAG_PREINSTALLED_APP_UPDATE = 0x00000040,
};

enum class StartMode : uint8_t {
    MAIN_TASK = 1,
    RECENT_TASK = 2
};

enum class AppPreloadPhase : uint8_t {
    DEFAULT = 0,
    PROCESS_CREATED = 1,
    ABILITY_STAGE_CREATED = 2,
    WINDOW_STAGE_CREATED = 3
};

struct MultiAppModeData : public Parcelable {
    MultiAppModeType multiAppModeType = MultiAppModeType::UNSPECIFIED;
    int32_t maxCount = 0;
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static MultiAppModeData *Unmarshalling(Parcel &parcel);
};

struct Metadata : public Parcelable {
    uint32_t valueId = 0;
    std::string name;
    std::string value;
    std::string resource;
    Metadata() = default;
    Metadata(const std::string &paramName, const std::string &paramValue, const std::string &paramResource);
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static Metadata *Unmarshalling(Parcel &parcel);
};

struct HnpPackage : public Parcelable {
    std::string package;
    std::string type;
    bool independentSign = false;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static HnpPackage *Unmarshalling(Parcel &parcel);
};

struct CustomizeData : public Parcelable {
    std::string name;
    std::string value;
    std::string extra;
    CustomizeData() = default;
    CustomizeData(std::string paramName, std::string paramValue, std::string paramExtra);
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static CustomizeData *Unmarshalling(Parcel &parcel);
};

struct MetaData {
    std::vector<CustomizeData> customizeData;
};

struct Resource : public Parcelable {
    /** the resource id in hap */
    uint32_t id = 0;

    /** the hap bundle name */
    std::string bundleName;

    /** the hap module name */
    std::string moduleName;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static Resource *Unmarshalling(Parcel &parcel);
};

struct ApplicationEnvironment : public Parcelable {
    std::string name;
    std::string value;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static ApplicationEnvironment *Unmarshalling(Parcel &parcel);
};

struct AlternateIcon : public Parcelable {
    std::string name;
    std::string icon;
    uint32_t iconId = 0;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AlternateIcon *Unmarshalling(Parcel &parcel);
};

struct ApplicationInfo;

struct CompatibleApplicationInfo : public Parcelable {
    bool isCompressNativeLibs = true;

    bool systemApp = false;
    bool enabled = true;
    bool debug = false;

    uint32_t iconId = 0;
    uint32_t labelId = 0;
    uint32_t descriptionId = 0;

    int32_t supportedModes = 0; // supported modes.
    // items set when installing.
    std::string name; // application name.
    std::string icon; // application icon resource index.
    std::string label; // application name displayed to the user.
    std::string description; // description of application.
    std::string cpuAbi; // current device cpu abi.
    std::string process;

    std::vector<std::string> permissions;
    std::vector<ModuleInfo> moduleInfos;

    bool ReadFromParcel(Parcel& parcel);
    virtual bool Marshalling(Parcel& parcel) const override;
    static CompatibleApplicationInfo* Unmarshalling(Parcel& parcel);
    void ConvertToApplicationInfo(ApplicationInfo& applicationInfo) const;
};

// configuration information about an application
struct ApplicationInfo : public Parcelable {
    bool keepAlive = false;
    bool removable = true;
    bool singleton = false;
    bool userDataClearable = true;
    bool allowAppRunWhenDeviceFirstLocked = false;
    bool accessible = false;
    bool runningResourcesApply = false;
    bool associatedWakeUp = false;
    bool hideDesktopIcon = false;
    bool formVisibleNotify = false;
    bool isSystemApp = false;
    bool isLauncherApp = false;
    bool isFreeInstallApp = false;
    bool asanEnabled = false;
    bool debug = false;
    bool distributedNotificationEnabled = true;
    bool allowEnableNotification = false;
    bool allowArkTsLargeHeap = false;
    bool allowMultiProcess = false;
    bool enabled = false;
    bool isBundleFirstLaunched = false;
    bool tsanEnabled = false;
    bool gwpAsanEnabled = false;
    bool hasPlugin = false;
    // switch
    bool multiProjects = false;
    bool isCompressNativeLibs = true;
    // app detail ability
    bool needAppDetail = false;
    bool cloudFileSyncEnabled = false;
    bool cloudStructuredDataSyncEnabled = false;

    bool hwasanEnabled = false;
    bool ubsanEnabled = false;
    bool installedForAllUser = false;
    bool skillEnabled = false;

    // forceRotate only for broker
    bool isForceRotate = false;
    bool profileable = false;

    uint32_t versionCode = 0;

    uint32_t apiCompatibleVersion = 0;
    uint32_t compatibleMinorVersion = 0;
    uint32_t compatiblePatchVersion = 0;
    uint32_t iconId = 0;
    uint32_t labelId = 0;
    uint32_t descriptionId = 0;

    // user related fields, assign when calling the get interface
    uint32_t accessTokenId = 0;
    uint32_t applicationReservedFlag = 0;
    int32_t minCompatibleVersionCode = 0;
    int32_t apiTargetVersion = 0;
    int32_t targetMinorApiVersion = 0;
    int32_t targetPatchApiVersion = 0;

    int32_t supportedModes = 0;  // returns 0 if the application does not support the driving mode
    int32_t appIndex = 0;
    int32_t uid = -1;

    int32_t flags = 0;
    int32_t targetPriority = 0;
    int32_t overlayState = 0;
    int32_t maxChildProcess = 0;

    int32_t applicationFlags = static_cast<uint32_t>(ApplicationInfoFlag::FLAG_INSTALLED);

    BundleType bundleType = BundleType::APP;

    MultiAppModeData multiAppMode;

    StartMode startMode = StartMode::MAIN_TASK;
    int64_t crowdtestDeadline = Constants::INVALID_CROWDTEST_DEADLINE;
    uint64_t accessTokenIdEx = 0;
    std::string name;  // application name is same to bundleName
    std::string bundleName;
    std::string versionName;

    std::string iconPath;

    std::string label;

    std::string description;

    std::string appDetailAbilityLibraryPath;

    std::string asanLogPath;
    std::string codePath;
    std::string dataDir;
    std::string dataBaseDir;
    std::string cacheDir;
    std::string entryDir;
    std::string apiReleaseType;
    std::string deviceId;
    std::string entityType = DEFAULT_ENTITY_TYPE;
    std::string process;
    std::string vendor;

    // apl
    std::string appPrivilegeLevel = AVAILABLELEVEL_NORMAL;
    // provision
    std::string appDistributionType = Constants::APP_DISTRIBUTION_TYPE_NONE;
    std::string appProvisionType = Constants::APP_PROVISION_TYPE_RELEASE;

    // native so
    std::string nativeLibraryPath;
    std::string cpuAbi;
    std::string arkNativeFilePath;
    std::string arkNativeFileAbi;

    std::string fingerprint;
    std::string icon;
    std::string entryModuleName;
    std::string signatureKey;

    // overlay installation
    std::string targetBundleName;

    std::string compileSdkVersion;
    std::string compileSdkType = DEFAULT_COMPILE_SDK_TYPE;
    std::string organization;

    std::string installSource;
    std::string configuration;
    std::string arkTSMode;
    std::string curAlternateIconModuleName;
    Resource iconResource;
    Resource labelResource;
    Resource descriptionResource;
    std::vector<int32_t> resourcesApply;
    std::vector<std::string> allowCommonEvent;

    // assign when calling the get interface
    std::vector<std::string> permissions;
    std::vector<std::string> moduleSourceDirs;
    std::vector<ModuleInfo> moduleInfos;
    // Installation-free
    std::vector<std::string> targetBundleList;
    std::vector<ApplicationEnvironment> appEnvironments;
    std::vector<AlternateIcon> alternateIcons;
    std::vector<std::string> assetAccessGroups;
    std::map<std::string, std::vector<HnpPackage>> hnpPackages;
    std::map<std::string, std::vector<CustomizeData>> metaData;
    std::map<std::string, std::vector<Metadata>> metadata;
    // quick fix info
    AppQuickFix appQuickFix;
    AppPreloadPhase appPreloadPhase = AppPreloadPhase::DEFAULT;
    std::string appSignType = Constants::APP_SIGN_TYPE_NONE;
    std::vector<std::string> allowListenBundleChangedEvent;

    bool ReadFromParcel(Parcel &parcel);
    bool ReadMetaDataFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static ApplicationInfo *Unmarshalling(Parcel &parcel);
    void Dump(const std::string &prefix, int fd);
    void ConvertToCompatibleApplicationInfo(CompatibleApplicationInfo& compatibleApplicationInfo) const;
    bool CheckNeedPreload(const std::string &moduleName) const;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APPLICATION_INFO_H
