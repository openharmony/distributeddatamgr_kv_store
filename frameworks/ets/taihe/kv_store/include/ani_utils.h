/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_ANI_UTILS_H
#define OHOS_ANI_UTILS_H

#include <ani.h>
#include <cstdarg>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <iostream>
#include "taihe/runtime.hpp"

namespace ani_utils {
int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, std::string &result,
    bool optional = false);
int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, bool &result,
    bool optional = false);
int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, int32_t &result,
    bool optional = false);
int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, uint32_t &result,
    bool optional = false);
int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, ani_object &result,
    bool optional = false);

class AniObjectUtils {
public:
    template<typename T>
    static ani_status Wrap(ani_env *env, ani_object object, T *nativePtr, const char *propName = "nativePtr")
    {
        return env->Object_SetFieldByName_Long(object, propName, reinterpret_cast<ani_long>(nativePtr));
    }

    template<typename T>
    static T* Unwrap(ani_env *env, ani_object object, const char *propName = "nativePtr")
    {
        ani_long nativePtr;
        if (ANI_OK != env->Object_GetFieldByName_Long(object, propName, &nativePtr)) {
            return nullptr;
        }
        return reinterpret_cast<T*>(nativePtr);
    }
};

class AniStringUtils {
public:
    static std::string ToStd(ani_env *env, ani_string ani_str);
    static ani_string ToAni(ani_env *env, const std::string& str);
};

ani_status AniCreateInt(ani_env* env, int32_t value, ani_object& result);
bool AniCreateTuple(ani_env* env, ani_ref item1, ani_ref item2, ani_tuple_value &tuple);

bool AniIsInstanceOf(ani_env* aniEnv, ani_ref aniRef, const std::string& cls_name);

} //namespace ani_utils
#endif

