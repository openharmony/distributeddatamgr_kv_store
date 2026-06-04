/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_HAP_MODULE_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_HAP_MODULE_INFO_H

#include <string>

#include "ability_info.h"
#include "extension_ability_info.h"
#include "overlay/overlay_module_info.h"
#include "parcel.h"
#include "quick_fix/hqf_info.h"

namespace OHOS {
namespace AppExecFwk {
enum class ModuleColorMode {
    AUTO = -1,
    DARK,
    LIGHT,
};

enum class ModuleType {
    UNKNOWN = 0,
    ENTRY = 1,
    FEATURE = 2,
    SHARED = 3,
    SKILL = 4,
};

enum class AtomicServiceModuleType {
    NORMAL = 0,
    MAIN = 1,
};

enum class IsolationMode {
    NONISOLATION_FIRST = 0,
    ISOLATION_FIRST = 1,
    ISOLATION_ONLY = 2,
    NONISOLATION_ONLY = 3,
};

enum class AOTCompileStatus {
    NOT_COMPILED = 0,
    IDLE_COMPILE_SUCCESS = 1,
    COMPILE_FAILED = 2,
    COMPILE_CRASH = 3,
    COMPILE_CANCELLED = 4,
    INSTALL_COMPILE_SUCCESS = 5,
};

struct PreloadItem : public Parcelable {
    std::string moduleName;

    PreloadItem() = default;
    explicit PreloadItem(std::string name) : moduleName(name) {}
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static PreloadItem *Unmarshalling(Parcel &parcel);
};

struct Dependency : public Parcelable {
    uint32_t versionCode = 0;
    std::string bundleName;
    std::string moduleName;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static Dependency *Unmarshalling(Parcel &parcel);
};

struct ProxyData : public Parcelable {
    std::string uri;
    std::string requiredReadPermission;
    std::string requiredWritePermission;
    Metadata metadata;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static ProxyData *Unmarshalling(Parcel &parcel);
};

struct RouterItem : public Parcelable {
    std::string name;
    std::string pageSourceFile;
    std::string buildFunction;
    std::string customData;
    std::string ohmurl;
    std::string bundleName;
    std::string moduleName;
    std::map<std::string, std::string> data;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static RouterItem *Unmarshalling(Parcel &parcel);
};

struct AppEnvironment : public Parcelable {
    std::string name;
    std::string value;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AppEnvironment *Unmarshalling(Parcel &parcel);
};

struct ExecutableBinaryPath : public Parcelable {
    std::string path;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static ExecutableBinaryPath *Unmarshalling(Parcel &parcel);
};

// configuration information about an module
struct HapModuleInfo : public Parcelable {
    bool compressNativeLibs = true;
    bool isLibIsolated = false;
    bool deliveryWithInstall = false;
    bool installationFree = false;
    bool isModuleJson = false;
    bool isStageBasedModel = false;
    bool hasIntent = false;
    bool resizeable = false;
    bool deduplicateHar = false;
    uint32_t descriptionId = 0;
    uint32_t iconId = 0;
    uint32_t labelId = 0;
    uint32_t versionCode = 0;
    int32_t upgradeFlag = 0;
    int supportedModes = 0;
    ModuleColorMode colorMode = ModuleColorMode::AUTO;
    ModuleType moduleType = ModuleType::UNKNOWN;
    CompileMode compileMode = CompileMode::JS_BUNDLE;
    AOTCompileStatus aotCompileStatus = AOTCompileStatus::NOT_COMPILED;
    IsolationMode isolationMode = IsolationMode::NONISOLATION_FIRST;
    std::string name;        // module.name in config.json
    std::string package;
    std::string moduleName;  // module.distro.moduleName in config.json
    std::string description;
    std::string iconPath;
    std::string label;
    std::string backgroundImg;
    std::string mainAbility;
    std::string srcPath;
    std::string hashValue;
    std::string hapPath;
    std::string nativeLibraryPath;
    std::string cpuAbi;
    // new version fields
    std::string bundleName;
    std::string mainElementName;
    std::string pages;
    std::string systemTheme;
    std::string process;
    std::string resourcePath;
    std::string srcEntrance;
    std::string uiSyntax;
    std::string virtualMachine;
    std::string moduleSourceDir;
    std::string buildHash;
    std::string fileContextMenu;
    std::string easyGo;
    std::string shareFiles;
    std::string routerMap;
    std::string packageName;
    std::string crossAppSharedConfig;
    std::string abilitySrcEntryDelegator;
    std::string abilityStageSrcEntryDelegator;
    std::string appStartup;
    std::string moduleArkTSMode = Constants::ARKTS_MODE_DYNAMIC;
    std::string arkTSMode = Constants::ARKTS_MODE_DYNAMIC;
    std::string formExtensionModule;
    std::string formWidgetModule;

    // quick fix hqf info
    HqfInfo hqfInfo;
    std::vector<std::string> nativeLibraryFileNames;

    // overlay module info
    std::vector<OverlayModuleInfo> overlayModuleInfos;

    std::vector<std::string> reqCapabilities;
    std::vector<std::string> deviceTypes;
    std::map<std::string, std::vector<std::string>> requiredDeviceFeatures;
    std::vector<std::string> librarySupportDirectory;
    std::vector<Dependency> dependencies;
    std::vector<AbilityInfo> abilityInfos;
    std::vector<ExtensionAbilityInfo> extensionInfos;
    std::vector<Metadata> metadata;
    std::vector<ProxyData> proxyDatas;
    std::vector<PreloadItem> preloads;
    std::vector<RouterItem> routerArray;
    std::vector<AppEnvironment> appEnvironments;
    std::map<std::string, bool> isRemovable;
    std::set<std::string> isRemovableSet;
    std::vector<ExecutableBinaryPath> executableBinaryPaths;
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static HapModuleInfo *Unmarshalling(Parcel &parcel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_HAP_MODULE_INFO_H
