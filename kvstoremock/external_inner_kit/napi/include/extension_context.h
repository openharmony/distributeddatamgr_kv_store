/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef NAPI_INCLUDE_EXTENSION_CONTEXT_H
#define NAPI_INCLUDE_EXTENSION_CONTEXT_H

#include <memory>
#include <string>
#include <vector>

#include "ability_info.h"
#include "application_info.h"
#include "hap_module_info.h"

namespace OHOS {
namespace AbilityRuntime {

class Context {
public:
    virtual ~Context() = default;
    virtual std::string GetDatabaseDir() { return ""; }
    virtual std::string GetBundleName() { return ""; }
    virtual int GetArea() { return 0; }
    virtual std::shared_ptr<AppExecFwk::HapModuleInfo> GetHapModuleInfo() { return nullptr; }
    virtual std::shared_ptr<AppExecFwk::ApplicationInfo> GetApplicationInfo() { return nullptr; }
    virtual int GetSystemDatabaseDir(const std::string &dataGroupId, bool hasDataGroupId, std::string &databaseDir) { return 0; }
    
    template<typename T>
    static std::shared_ptr<T> ConvertTo(std::shared_ptr<Context> context) {
        if (context == nullptr) {
            return nullptr;
        }
        return std::static_pointer_cast<T>(context);
    }
};

class ContextImpl : public Context {
public:
    ContextImpl() = default;
    virtual ~ContextImpl() = default;
};

class ExtensionContext : public ContextImpl {
public:
    ExtensionContext() = default;
    virtual ~ExtensionContext() = default;

    std::shared_ptr<AppExecFwk::AbilityInfo> GetAbilityInfo() const {
        return abilityInfo_;
    }

    void SetAbilityInfo(const std::shared_ptr<AppExecFwk::AbilityInfo> &abilityInfo) {
        abilityInfo_ = abilityInfo;
    }

    using SelfType = ExtensionContext;
    static const size_t CONTEXT_TYPE_ID;

protected:
    std::shared_ptr<AppExecFwk::AbilityInfo> abilityInfo_;
};

class AbilityContext : public Context {
public:
    AbilityContext() = default;
    virtual ~AbilityContext() = default;

    std::shared_ptr<AppExecFwk::AbilityInfo> GetAbilityInfo() const {
        return abilityInfo_;
    }

protected:
    std::shared_ptr<AppExecFwk::AbilityInfo> abilityInfo_;
};

} // namespace AbilityRuntime
} // namespace OHOS

#endif // NAPI_INCLUDE_EXTENSION_CONTEXT_H