/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#ifndef OHOS_RESOURCE_MANAGER_RESOURCEMANAGER_H
#define OHOS_RESOURCE_MANAGER_RESOURCEMANAGER_H

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include "res_config.h"

namespace OHOS {
namespace Global {
namespace Resource {
enum FunctionType {
    SYNC = 0,
    ASYNC = 1
};
class ResourceManager {
public:
    std::pair<std::string, std::string> bundleInfo;

    int32_t userId = 0;

    typedef struct {
        /** the raw file fd */
        int fd;

        /** the offset from where the raw file starts in the HAP */
        int64_t offset;

        /** the length of the raw file in the HAP. */
        int64_t length;
    } RawFileDescriptor;

    struct Resource {
        /** the hap bundle name */
        std::string bundleName;

        /** the hap module name */
        std::string moduleName;

        /** the resource id in hap */
        uint32_t id;
    };

    struct ResData {
        /** the resource type */
        ResType resType;

        /** the resource value */
        std::string value;
    };

    struct Quantity {
        /** the indication of whether quantity is an integer. */
        bool isInteger = false;

        /** the integer representation of the quantity value. */
        int intValue = 0;

        /** the double representation of the quantity value. */
        double doubleValue = 0.0;
    };

    enum class NapiValueType {
        NAPI_NUMBER = 0,
        NAPI_STRING = 1
    };

    virtual ~ResourceManager() = 0;

    /**
     * Add resource of hap.
     *
     * @param path The path of hap.
     * @param selectedTypes If this param is setted, will only add resource of types specified by this param,
     *     it will be faster than add all resource, you can set this param by combining flags
     *     named SELECT_XXX defined in res_common.h. What's more, if you call UpdateResConfig()
     *     before calling this method, will only add resource that matching the config, for example,
     *     only add resource of current language, which means it will be further faster.
     * @return true if init success, else false
     */
    virtual bool AddResource(const char *path, const uint32_t &selectedTypes = SELECT_ALL,
        bool forceReload = false) = 0;

    virtual RState UpdateResConfig(ResConfig &resConfig, bool isUpdateTheme = false) = 0;

    virtual void GetResConfig(ResConfig &resConfig) = 0;

    virtual RState GetResConfigById(uint32_t resId, ResConfig &resConfig, uint32_t density = 0) = 0;

    virtual RState GetResConfigByName(const std::string &name, const ResType type,
        ResConfig &resConfig, uint32_t density = 0) = 0;

    virtual RState GetStringById(uint32_t id, std::string &outValue) = 0;

    virtual RState GetStringByName(const char *name, std::string &outValue) = 0;

    virtual RState GetStringFormatById(std::string &outValue, uint32_t id, ...) = 0;

    virtual RState GetStringFormatByName(std::string &outValue, const char *name, ...) = 0;

    virtual RState GetStringArrayById(uint32_t id, std::vector<std::string> &outValue) = 0;

    virtual RState GetStringArrayByName(const char *name, std::vector<std::string> &outValue) = 0;

    virtual RState GetPatternById(uint32_t id, std::map<std::string, std::string> &outValue) = 0;

    virtual RState GetPatternByName(const char *name, std::map<std::string, std::string> &outValue) = 0;

    virtual RState GetPluralStringById(uint32_t id, int quantity, std::string &outValue) = 0;

    virtual RState GetPluralStringByName(const char *name, int quantity, std::string &outValue) = 0;

    virtual RState GetPluralStringByIdFormat(std::string &outValue, uint32_t id, int quantity, ...) = 0;

    virtual RState GetPluralStringByNameFormat(std::string &outValue, const char *name, int quantity, ...) = 0;

    virtual RState GetThemeById(uint32_t id, std::map<std::string, std::string> &outValue) = 0;

    virtual RState GetThemeByName(const char *name, std::map<std::string, std::string> &outValue) = 0;

    virtual RState GetBooleanById(uint32_t id, bool &outValue) = 0;

    virtual RState GetBooleanByName(const char *name, bool &outValue) = 0;

    virtual RState GetIntegerById(uint32_t id, int &outValue) = 0;

    virtual RState GetIntegerByName(const char *name, int &outValue) = 0;

    virtual RState GetFloatById(uint32_t id, float &outValue) = 0;

    virtual RState GetFloatById(uint32_t id, float &outValue, std::string &unit) = 0;

    virtual RState GetFloatByName(const char *name, float &outValue) = 0;

    virtual RState GetFloatByName(const char *name, float &outValue, std::string &unit) = 0;

    virtual RState GetIntArrayById(uint32_t id, std::vector<int> &outValue) = 0;

    virtual RState GetIntArrayByName(const char *name, std::vector<int> &outValue) = 0;

    virtual RState GetColorById(uint32_t id, uint32_t &outValue) = 0;

    virtual RState GetColorByName(const char *name, uint32_t &outValue) = 0;

    virtual RState GetProfileById(uint32_t id, std::string &outValue) = 0;

    virtual RState GetProfileByName(const char *name, std::string &outValue) = 0;

    virtual RState GetMediaById(uint32_t id, std::string &outValue, uint32_t density = 0) = 0;

    virtual RState GetMediaByName(const char *name, std::string &outValue, uint32_t density = 0) = 0;

    virtual RState GetRawFilePathByName(const std::string &name, std::string &outValue) = 0;

    virtual RState GetRawFileDescriptor(const std::string &name, RawFileDescriptor &descriptor) = 0;

    virtual RState CloseRawFileDescriptor(const std::string &name) = 0;

    virtual RState GetMediaDataById(uint32_t id, size_t& len, std::unique_ptr<uint8_t[]> &outValue,
        uint32_t density = 0) = 0;

    virtual RState GetMediaDataByName(const char *name, size_t& len, std::unique_ptr<uint8_t[]> &outValue,
        uint32_t density = 0) = 0;

    virtual RState GetMediaBase64DataById(uint32_t id,  std::string &outValue, uint32_t density = 0) = 0;

    virtual RState GetMediaBase64DataByName(const char *name,  std::string &outValue, uint32_t density = 0) = 0;

    virtual RState GetProfileDataById(uint32_t id, size_t &len, std::unique_ptr<uint8_t[]> &outValue) = 0;

    virtual RState GetProfileDataByName(const char *name, size_t &len, std::unique_ptr<uint8_t[]> &outValue) = 0;

    virtual RState GetRawFileFromHap(const std::string &rawFileName, size_t &len,
        std::unique_ptr<uint8_t[]> &outValue) = 0;

    virtual RState GetRawFileDescriptorFromHap(const std::string &rawFileName, RawFileDescriptor &descriptor) = 0;

    virtual RState IsLoadHap(std::string &hapPath) = 0;

    virtual RState GetRawFileList(const std::string &rawDirPath, std::vector<std::string> &rawfileList) = 0;

    virtual RState GetDrawableInfoById(uint32_t id, std::string &type, size_t &len,
        std::unique_ptr<uint8_t[]> &outValue, uint32_t density = 0) = 0;

    virtual RState GetDrawableInfoByName(const char *name, std::string &type, size_t &len,
        std::unique_ptr<uint8_t[]> &outValue, uint32_t density = 0) = 0;

    virtual bool AddResource(const std::string &path, const std::vector<std::string> &overlayPaths) = 0;

    virtual bool RemoveResource(const std::string &path, const std::vector<std::string> &overlayPaths) = 0;

    virtual RState GetStringFormatById(uint32_t id, std::string &outValue,
        std::vector<std::tuple<NapiValueType, std::string>> &jsParams) = 0;

    virtual RState GetStringFormatByName(const char *name, std::string &outValue,
        std::vector<std::tuple<NapiValueType, std::string>> &jsParams) = 0;

    virtual uint32_t GetResourceLimitKeys() = 0;

    virtual bool AddAppOverlay(const std::string &path) = 0;

    virtual bool RemoveAppOverlay(const std::string &path) = 0;

    virtual RState GetRawFdNdkFromHap(const std::string &rawFileName, RawFileDescriptor &descriptor) = 0;

    virtual RState GetResId(const std::string &resTypeName, uint32_t &resId) = 0;

    virtual void GetLocales(std::vector<std::string> &outValue, bool includeSystem = false) = 0;

    virtual RState GetDrawableInfoById(uint32_t id,
        std::tuple<std::string, size_t, std::string> &drawableInfo,
        std::unique_ptr<uint8_t[]> &outValue, uint32_t iconType, uint32_t density = 0) = 0;

    virtual RState GetDrawableInfoByName(const char *name,
        std::tuple<std::string, size_t, std::string> &drawableInfo,
        std::unique_ptr<uint8_t[]> &outValue, uint32_t iconType, uint32_t density = 0) = 0;

    virtual RState GetSymbolById(uint32_t id, uint32_t &outValue) = 0;

    virtual RState GetSymbolByName(const char *name, uint32_t &outValue) = 0;

    virtual RState GetThemeIcons(uint32_t resId, std::pair<std::unique_ptr<uint8_t[]>, size_t> &foregroundInfo,
        std::pair<std::unique_ptr<uint8_t[]>, size_t> &backgroundInfo, uint32_t density = 0,
        const std::string &abilityName = "") = 0;

    virtual std::string GetThemeMask() = 0;

    virtual bool HasIconInTheme(const std::string &bundleName) = 0;

    virtual RState GetOtherIconsInfo(const std::string &iconName,
        std::unique_ptr<uint8_t[]> &outValue, size_t &len, bool isGlobalMask) = 0;

    virtual RState IsRawDirFromHap(const std::string &pathName, bool &outValue) = 0;

    virtual std::shared_ptr<ResourceManager> GetOverrideResourceManager(
        std::shared_ptr<ResConfig> overrideResConfig) = 0;

    virtual RState UpdateOverrideResConfig(ResConfig &resConfig) = 0;

    virtual void GetOverrideResConfig(ResConfig &resConfig) = 0;

    virtual RState GetDynamicIcon(const std::string &resName, std::pair<std::unique_ptr<uint8_t[]>, size_t> &iconInfo,
        uint32_t density = 0) = 0;

    virtual RState GetStringFormatById(std::string &outValue, uint32_t id, va_list args) = 0;

    virtual RState GetStringFormatByName(std::string &outValue, const char *name, va_list args) = 0;

    virtual RState GetFormatPluralStringById(std::string &outValue, uint32_t id, int quantity,
        std::vector<std::tuple<ResourceManager::NapiValueType, std::string>> &jsParams) = 0;

    virtual RState GetFormatPluralStringByName(std::string &outValue, const char *name, int quantity,
        std::vector<std::tuple<ResourceManager::NapiValueType, std::string>> &jsParams) = 0;
    
    virtual bool AddPatchResource(const char *path, const char *patchPath) = 0;

    virtual RState GetThemeDataByName(const char *name, std::map<std::string, ResData> &outValue) = 0;

    virtual RState GetThemeDataById(uint32_t id, std::map<std::string, ResData> &outValue) = 0;

    virtual RState GetPatternDataById(uint32_t id, std::map<std::string, ResData> &outValue) = 0;

    virtual RState GetPatternDataByName(const char *name, std::map<std::string, ResData> &outValue) = 0;

    virtual RState GetFormatPluralStringById(std::string &outValue, uint32_t id, Quantity quantity,
        va_list args) = 0;

    virtual RState GetFormatPluralStringById(std::string &outValue, uint32_t id, Quantity quantity,
        std::vector<std::tuple<ResourceManager::NapiValueType, std::string>> &jsParams) = 0;

    virtual RState GetFormatPluralStringByName(std::string &outValue, const char *name, Quantity quantity,
        va_list args) = 0;

    virtual RState GetFormatPluralStringByName(std::string &outValue, const char *name, Quantity quantity,
        std::vector<std::tuple<ResourceManager::NapiValueType, std::string>> &jsParams) = 0;

    virtual RState GetResName(uint32_t id, std::string &outValue) = 0;
};

EXPORT_FUNC ResourceManager *CreateResourceManager(bool includeSystemRes = true);

/**
 * Get system resource manager, the added system resource is sandbox path. This method should call
 * after the sandbox mount.
 *
 * @return pointer of system resource manager
 * @deprecated since 14
 * @useinstead CreateResourceManager
 */
EXPORT_FUNC ResourceManager *GetSystemResourceManager();

/**
 * Get system resource manager, the added system resource is no sandbox path. This method should call
 * before the sandbox mount, for example appspawn.
 *
 * @return pointer of system resource manager
 * @deprecated since 14
 * @useinstead CreateResourceManager
 */
EXPORT_FUNC ResourceManager *GetSystemResourceManagerNoSandBox();

/**
 * Create app resource manager.
 *
 * @param bundleName the hap bundleName
 * @param moduleName the hap moduleName
 * @param hapPath the hap resource path
 * @param overlayPath the hap overlay resource path
 * @param resConfig the device resConfig
 * @param appType the app type
 * @return pointer of app resource manager
 */
EXPORT_FUNC std::shared_ptr<ResourceManager> CreateResourceManager(const std::string &bundleName,
    const std::string &moduleName, const std::string &hapPath, const std::vector<std::string> &overlayPath,
    ResConfig &resConfig, int32_t appType = 0, int32_t userId = -1);

/**
 * Release system resource manager. This object may be held by multiple objects, and once released, the
 * ResourceManagers of all holders are released. When using this interface, pay attention to the impact on all holders.
 */
EXPORT_FUNC void ReleaseSystemResourceManager();
} // namespace Resource
} // namespace Global
} // namespace OHOS
#endif