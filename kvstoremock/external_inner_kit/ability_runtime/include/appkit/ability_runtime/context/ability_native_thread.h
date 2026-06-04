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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_NATIVE_THREAD_H
#define OHOS_ABILITY_RUNTIME_ABILITY_NATIVE_THREAD_H

#include <functional>
#include <memory>
#include <string>
#include <thread>

typedef struct napi_env__* napi_env;
typedef struct napi_value__* napi_value;
typedef class __ani_object* ani_object;

class NativeReference;

#ifdef __cplusplus
extern "C" {
#endif
struct AbilityRuntime_NativeAbilityWrapper {
    std::string instanceId;
    std::string abilityName;
    napi_env env = nullptr;
    std::shared_ptr<NativeReference> jsAbilityObj;
    ani_object etsAbilityObj = nullptr;
};

#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace AAFwk {
struct NativeAbilityMetaData;
}  // namespace AAFwk

namespace AppExecFwk {
class AbilityNativeThread {
public:
    AbilityNativeThread() = default;
    ~AbilityNativeThread();

    // Disable copy and move
    AbilityNativeThread(AbilityNativeThread &) = delete;
    AbilityNativeThread& operator=(AbilityNativeThread &) = delete;
    AbilityNativeThread(AbilityNativeThread &&) = delete;
    AbilityNativeThread& operator=(AbilityNativeThread &&) = delete;

    /**
     * @brief Load native module from the specified metadata.
     * @param metaData The native module metadata containing library path and function names.
     * @param bundleName Indicates native library path bundleName.
     * @param moduleName Indicates native library path moduleName.
     * @return true if loading succeeded, false otherwise.
     */
    bool LoadNativeModule(const AAFwk::NativeAbilityMetaData& metaData, const std::string &bundleName,
        const std::string &moduleName);

    /**
     * @brief Run the main function in a new native thread.
     */
    void RunMain();

    /**
     * @brief Post the ability wrapper to the native thread.
     * @param nativeAbilityWrapper The pointer to the native ability wrapper.
     */
    void PostAbility(const AbilityRuntime_NativeAbilityWrapper* nativeAbilityWrapper);

    /**
     * @brief Notify the native module that the ability is being destroyed.
     * @param nativeAbilityWrapper The pointer to the native ability wrapper.
     */
    void DestroyAbility(const AbilityRuntime_NativeAbilityWrapper* nativeAbilityWrapper);

    /**
     * @brief Notify the native module that the process is exiting.
     */
    void NotifyProcessExit();

    static void* OpenNativeLibrary(const std::string& bundleModuleName, const std::string& fileName);
private:
    void *moduleHandle_ = nullptr;
    std::function<void()> ohMainFun_;
    std::function<void(const AbilityRuntime_NativeAbilityWrapper*)> postAbilityFunc_;
    std::function<void(const AbilityRuntime_NativeAbilityWrapper*)> destroyAbilityFunc_;
    std::function<void()> notifyProcessExitFunc_;
    std::thread nativeThread_;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_ABILITY_NATIVE_THREAD_H