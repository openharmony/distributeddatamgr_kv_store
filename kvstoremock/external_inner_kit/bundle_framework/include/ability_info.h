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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_ABILITY_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_ABILITY_INFO_H

#include <string>
#include <unordered_set>

#include "parcel.h"
#include "application_info.h"
#include "extension_ability_info.h"

namespace OHOS {
namespace AppExecFwk {
enum AbilityInfoFlag {
    GET_ABILITY_INFO_DEFAULT = 0x00000000,
    GET_ABILITY_INFO_WITH_PERMISSION = 0x00000002,
    GET_ABILITY_INFO_WITH_APPLICATION = 0x00000004,
    GET_ABILITY_INFO_WITH_METADATA = 0x00000020,
    GET_ABILITY_INFO_SYSTEMAPP_ONLY = 0x00000080,
    GET_ABILITY_INFO_WITH_DISABLE = 0x00000100,
    GET_ABILITY_INFO_WITH_SKILL_URI = 0x00000200,
    GET_ABILITY_INFO_WITH_SKILL = 0x00000400,
    // the flag only for C++ interface passing
    GET_ABILITY_INFO_EXCLUDE_EXT = 0x00020000,
};

enum class GetAbilityInfoFlag {
    GET_ABILITY_INFO_DEFAULT = 0x00000000,
    GET_ABILITY_INFO_WITH_PERMISSION = 0x00000001,
    GET_ABILITY_INFO_WITH_APPLICATION = 0x00000002,
    GET_ABILITY_INFO_WITH_METADATA = 0x00000004,
    GET_ABILITY_INFO_WITH_DISABLE = 0x00000008,
    GET_ABILITY_INFO_ONLY_SYSTEM_APP = 0x00000010,
    GET_ABILITY_INFO_WITH_SKILL_URI = 0x00000020,
    GET_ABILITY_INFO_WITH_APP_LINKING = 0x00000040,
    GET_ABILITY_INFO_WITH_SKILL = 0x00000080,
    // the flag only for C++ interface passing
    GET_ABILITY_INFO_EXCLUDE_EXT = 0x00020000,
};

enum class AbilityType {
    UNKNOWN = 0,
    PAGE,
    SERVICE,
    DATA,
    FORM,
    EXTENSION,
};

enum class BackgroundMode {
    DEFAULT = 0,
    DATA_TRANSFER = 1 << 0,
    AUDIO_PLAYBACK = 1 << 1,
    AUDIO_RECORDING = 1 << 2,
    LOCATION = 1 << 3,
    BLUETOOTH_INTERACTION = 1 << 4,
    MULTI_DEVICE_CONNECTION = 1 << 5,
    WIFI_INTERACTION = 1 << 6,
    VOIP = 1 << 7,
    TASK_KEEPING = 1 << 8,
};

enum class AbilitySubType {
    UNSPECIFIED = 0,
    CA,
};

enum class DisplayOrientation {
    UNSPECIFIED = 0,
    LANDSCAPE,
    PORTRAIT,
    FOLLOWRECENT,
    LANDSCAPE_INVERTED,
    PORTRAIT_INVERTED,
    AUTO_ROTATION,
    AUTO_ROTATION_LANDSCAPE,
    AUTO_ROTATION_PORTRAIT,
    AUTO_ROTATION_RESTRICTED,
    AUTO_ROTATION_LANDSCAPE_RESTRICTED,
    AUTO_ROTATION_PORTRAIT_RESTRICTED,
    LOCKED,
    AUTO_ROTATION_UNSPECIFIED,
    FOLLOW_DESKTOP,
};

enum class LaunchMode {
    SINGLETON = 0,
    STANDARD,  // support more than one instance
    SPECIFIED,
};

enum class SupportWindowMode {
    FULLSCREEN = 0,
    SPLIT,
    FLOATING,
};

enum class LinkType : uint8_t {
    DEEP_LINK = 0,
    APP_LINK,
    DEFAULT_APP,
};

struct AbilityInfo;

struct StartWindowResource : public Parcelable {
    uint32_t startWindowAppIconId = 0;
    uint32_t startWindowIllustrationId = 0;
    uint32_t startWindowBrandingImageId = 0;
    uint32_t startWindowBackgroundColorId = 0;
    uint32_t startWindowBackgroundImageId = 0;
    std::string startWindowBackgroundImageFit = "Cover";

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static StartWindowResource *Unmarshalling(Parcel &parcel);
};

/*
* According to Ability profile 1.0
*/
struct CompatibleAbilityInfo : public Parcelable {
    bool visible = false;
    bool formEnabled = false;
    bool multiUserShared = false;
    bool supportPipMode = false;
    bool grantPermission = false;
    bool directLaunch = true;
    bool enabled = true;
    uint32_t backgroundModes = 0;
    uint32_t packageSize = 0; // The size of the package that AbilityInfo.uri points to.

    // form widget info
    uint32_t formEntity = 1; // where form can be displayed

    uint32_t iconId = 0;
    uint32_t labelId = 0;
    uint32_t descriptionId = 0;
    int32_t minFormHeight = 0; // minimum height of ability.
    int32_t defaultFormHeight = 0; // default height of ability.
    int32_t minFormWidth = 0; // minimum width of ability.
    int32_t defaultFormWidth = 0; // default width of ability.
    // deprecated: remove this field in new package format.
    AbilityType type = AbilityType::UNKNOWN;
    AbilitySubType subType = AbilitySubType::UNSPECIFIED;
    DisplayOrientation orientation = DisplayOrientation::UNSPECIFIED;
    LaunchMode launchMode = LaunchMode::SINGLETON;
    // deprecated: ability code class simple name, use 'className' instead.
    std::string package;
    std::string name;
    std::string label; // display name on screen.
    std::string description;
    std::string iconPath; // used as icon data (base64) for WEB Ability.
    std::string uri; // uri of ability.
    std::string moduleName; // indicates the name of the .hap package to which the capability belongs.
    std::string process;
    std::string targetAbility;
    std::string appName;
    std::string privacyUrl;
    std::string privacyName;
    std::string downloadUrl;
    std::string versionName;
    std::string readPermission;
    std::string writePermission;
    std::string uriPermissionMode;
    std::string uriPermissionPath;

    // set when install
    std::string bundleName; // bundle name which has this ability.
    std::string className;  // the ability full class name.
    std::string originalClassName; // the original ability full class name
    std::string deviceId; // device UDID information.
    std::vector<std::string> permissions;
    std::vector<std::string> deviceTypes;
    std::vector<std::string> deviceCapabilities;
    CompatibleApplicationInfo applicationInfo;

    bool ReadFromParcel(Parcel& parcel);
    virtual bool Marshalling(Parcel& parcel) const override;
    static CompatibleAbilityInfo* Unmarshalling(Parcel& parcel);

    void ConvertToAbilityInfo(AbilityInfo& abilityInfo) const;
};

// configuration information about an ability
struct AbilityInfo : public Parcelable {
    bool visible = false;
    bool isLauncherAbility = false;
    bool isNativeAbility = false;
    bool enabled = false;
    bool supportPipMode = false;
    bool formEnabled = false;
    bool removeMissionAfterTerminate = false;
    bool allowSelfRedirect  = true;
    bool isModuleJson = false;
    bool isStageBasedModel = false;
    bool continuable = false;
    // whether to display in the missions list
    bool excludeFromMissions = false;
    bool unclearableMission = false;
    bool excludeFromDock = false;
    // whether to support recover UI interface
    bool recoverable = false;
    bool isolationProcess = false;
    bool multiUserShared = false;
    bool grantPermission = false;
    bool directLaunch = true;
    LinkType linkType = LinkType::DEEP_LINK;
    uint32_t labelId = 0;
    uint32_t descriptionId = 0;
    uint32_t iconId = 0;
    uint32_t orientationId = 0;
    uint32_t formEntity = 0;
    uint32_t startWindowId = 0;
    uint32_t backgroundModes = 0;
    uint32_t startWindowIconId = 0;
    uint32_t startWindowBackgroundId = 0;
    uint32_t maxWindowWidth = 0;
    uint32_t minWindowWidth = 0;
    uint32_t maxWindowHeight = 0;
    uint32_t minWindowHeight = 0;
    uint32_t packageSize = 0;
    int32_t minFormHeight = 0;
    int32_t defaultFormHeight = 0;
    int32_t minFormWidth = 0;
    int32_t defaultFormWidth = 0;
    int32_t priority = 0;
    int32_t appIndex = 0;
    // for NAPI, save self query cache
    int32_t uid = -1;
    AbilityType type = AbilityType::UNKNOWN;
    ExtensionAbilityType extensionAbilityType = ExtensionAbilityType::UNSPECIFIED;
    DisplayOrientation orientation = DisplayOrientation::UNSPECIFIED;
    LaunchMode launchMode = LaunchMode::SINGLETON;
    CompileMode compileMode = CompileMode::JS_BUNDLE;
    AbilitySubType subType = AbilitySubType::UNSPECIFIED;
    int64_t installTime = 0;
    double maxWindowRatio = 0;
    double minWindowRatio = 0;
    std::string name;  // ability name, only the main class name
    std::string label;
    std::string description;
    std::string iconPath;
    std::string theme;
    std::string kind;  // ability category
    std::string extensionTypeName;
    std::string srcPath;
    std::string srcLanguage = Constants::SRC_LANGUAGE_JS;
    std::string arkTSMode = Constants::ARKTS_MODE_DYNAMIC;

    std::string process;
    std::string uri;
    std::string targetAbility;
    std::string readPermission;
    std::string writePermission;

    // set when install
    std::string package;  // the "module.package" in config.json
    std::string bundleName;
    std::string moduleName;       // the "module.name" in config.json
    std::string applicationName;  // the "bundlename" in config.json

    std::string codePath;         // ability main code path with name
    std::string resourcePath;     // resource path for resource init
    std::string hapPath;

    std::string srcEntrance;

    // configuration fields on startup page
    std::string startWindow;
    std::string startWindowIcon;
    std::string startWindowBackground;
    std::string preferMultiWindowOrientation = Constants::PREFER_MULTI_WINDOW_ORIENTATION_DEFAULT;

    std::string originalBundleName;
    std::string appName;
    std::string privacyUrl;
    std::string privacyName;
    std::string downloadUrl;
    std::string versionName;
    std::string className;
    std::string originalClassName;
    std::string uriPermissionMode;
    std::string uriPermissionPath;
    std::string libPath;
    std::string deviceId;
    std::vector<std::string> permissions;
    std::vector<std::string> deviceTypes;
    std::vector<std::string> deviceCapabilities;
    std::vector<std::string> configChanges;

    // for Check flags, add to abilityInfo and extensionAbilityInfo
    std::vector<SkillUriForAbilityAndExtension> skillUri;
    std::vector<Skill> skills;
    std::vector<Metadata> metadata;

    // support windows mode
    std::vector<SupportWindowMode> windowModes;
    std::vector<std::string> supportExtNames;
    std::vector<std::string> supportMimeTypes;
    std::vector<std::string> continueType;
    MetaData metaData;
    std::unordered_set<std::string> continueBundleNames;
    ApplicationInfo applicationInfo;
    StartWindowResource startWindowResource;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AbilityInfo *Unmarshalling(Parcel &parcel);
    void Dump(std::string prefix, int fd);
    void ConvertToCompatiableAbilityInfo(CompatibleAbilityInfo& compatibleAbilityInfo) const;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_ABILITY_INFO_H
