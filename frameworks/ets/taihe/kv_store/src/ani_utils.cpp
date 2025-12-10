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
#define LOG_TAG "AniUtils"
#include "ani_utils.h"
#include "log_print.h"

namespace ani_utils {
using namespace OHOS;
using namespace OHOS::DistributedKVStore;

int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, std::string &result, bool optional)
{
    if (env == nullptr || ani_obj == nullptr || property == nullptr) {
        return ANI_INVALID_ARGS;
    }
    ani_object object = nullptr;
    int32_t status = AniGetProperty(env, ani_obj, property, object, optional);
    if (status == ANI_OK && object != nullptr) {
        result = AniStringUtils::ToStd(env, reinterpret_cast<ani_string>(object));
    }
    return status;
}

int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, bool &result, bool optional)
{
    if (env == nullptr || ani_obj == nullptr || property == nullptr) {
        return ANI_INVALID_ARGS;
    }
    ani_boolean ani_field_value;
    ani_status status = env->Object_GetPropertyByName_Boolean(
        ani_obj, property, reinterpret_cast<ani_boolean*>(&ani_field_value));
    if (status != ANI_OK) {
        if (optional) {
            status = ANI_OK;
        }
        return status;
    }
    result = (bool)ani_field_value;
    return ANI_OK;
}

int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, int32_t &result, bool optional)
{
    if (env == nullptr || ani_obj == nullptr || property == nullptr) {
        return ANI_INVALID_ARGS;
    }
    ani_int ani_field_value;
    ani_status status = env->Object_GetPropertyByName_Int(
        ani_obj, property, reinterpret_cast<ani_int*>(&ani_field_value));
    if (status != ANI_OK) {
        if (optional) {
            status = ANI_OK;
        }
        return status;
    }
    result = (int32_t)ani_field_value;
    return ANI_OK;
}

int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, uint32_t &result, bool optional)
{
    if (env == nullptr || ani_obj == nullptr || property == nullptr) {
        return ANI_INVALID_ARGS;
    }
    ani_int ani_field_value;
    ani_status status = env->Object_GetPropertyByName_Int(
        ani_obj, property, reinterpret_cast<ani_int*>(&ani_field_value));
    if (status != ANI_OK) {
        if (optional) {
            status = ANI_OK;
        }
        return status;
    }
    result = (uint32_t)ani_field_value;
    return ANI_OK;
}

int32_t AniGetProperty(ani_env *env, ani_object ani_obj, const char *property, ani_object &result, bool optional)
{
    if (env == nullptr || ani_obj == nullptr || property == nullptr) {
        return ANI_INVALID_ARGS;
    }
    ani_status status = env->Object_GetPropertyByName_Ref(ani_obj, property, reinterpret_cast<ani_ref*>(&result));
    if (status != ANI_OK) {
        if (optional) {
            status = ANI_OK;
        }
        return status;
    }
    return ANI_OK;
}

std::string AniStringUtils::ToStd(ani_env *env, ani_string ani_str)
{
    if (env == nullptr) {
        return std::string();
    }
    ani_size strSize = 0;
    auto status = env->String_GetUTF8Size(ani_str, &strSize);
    if (ANI_OK != status) {
        ZLOGI("String_GetUTF8Size failed");
        return std::string();
    }

    std::vector<char> buffer(strSize + 1);
    char *utf8Buffer = buffer.data();

    ani_size bytesWritten = 0;
    status = env->String_GetUTF8(ani_str, utf8Buffer, strSize + 1, &bytesWritten);
    if (ANI_OK != status) {
        ZLOGI("String_GetUTF8Size failed");
        return std::string();
    }

    utf8Buffer[bytesWritten] = '\0';
    std::string content = std::string(utf8Buffer);
    return content;
}

ani_string AniStringUtils::ToAni(ani_env *env, const std::string& str)
{
    if (env == nullptr) {
        return nullptr;
    }
    ani_string aniStr = nullptr;
    if (ANI_OK != env->String_NewUTF8(str.data(), str.size(), &aniStr)) {
        ZLOGI("Unsupported ANI_VERSION_1");
        return nullptr;
    }
    return aniStr;
}

ani_status AniCreateInt(ani_env* env, int32_t value, ani_object& result)
{
    ani_status state;
    ani_class intClass;
    if ((state = env->FindClass("std.core.Int", &intClass)) != ANI_OK) {
        ZLOGE("FindClass std/core/Int failed, %{public}d", state);
        return state;
    }
    ani_method intClassCtor;
    if ((state = env->Class_FindMethod(intClass, "<ctor>", "i:", &intClassCtor)) != ANI_OK) {
        ZLOGE("Class_FindMethod Int ctor failed, %{public}d", state);
        return state;
    }
    ani_int aniValue = value;
    if ((state = env->Object_New(intClass, intClassCtor, &result, aniValue)) != ANI_OK) {
        ZLOGE("New Int object failed, %{public}d", state);
    }
    if (state != ANI_OK) {
        result = nullptr;
    }
    return state;
}

bool AniCreateTuple(ani_env* env, ani_ref item1, ani_ref item2, ani_tuple_value &tuple)
{
    if (env == nullptr) {
        return false;
    }
    ani_class tupleCls {};
    ani_method tupleCtorMethod {};
    ani_object tupleObj {};
    if (ANI_OK != env->FindClass("std.core.Tuple2", &tupleCls)) {
        ZLOGE("FindClass std.core.Tuple2 failed");
        return false;
    }
    if (ANI_OK != env->Class_FindMethod(tupleCls, "<ctor>", nullptr, &tupleCtorMethod)) {
        ZLOGE("Class_FindMethod ctor failed");
        return false;
    }
    if (ANI_OK != env->Object_New(tupleCls, tupleCtorMethod, &tupleObj, item1, item2)) {
         ZLOGE("Object_New tupleCls failed");
        return false;
    }
    tuple = static_cast<ani_tuple_value>(tupleObj);
    return true;
}

bool AniIsInstanceOf(ani_env* aniEnv, ani_ref aniRef, const std::string& cls_name)
{
    if (aniEnv == nullptr || aniRef == nullptr) {
        return false;
    }
    ani_boolean isNull = false;
    ani_boolean isUndefined = false;
    aniEnv->Reference_IsNull(aniRef, &isNull);
    aniEnv->Reference_IsUndefined(aniRef, &isUndefined);
    if (isNull || isUndefined) {
        return false;
    }
    ani_class cls;
    if (ANI_OK != aniEnv->FindClass(cls_name.c_str(), &cls) || cls == nullptr) {
        return false;
    }
    ani_boolean ret;
    ani_object aniObj = reinterpret_cast<ani_object>(aniRef);
    if (ANI_OK != aniEnv->Object_InstanceOf(aniObj, cls, &ret)) {
        return false;
    }
    return ret;
}

} //namespace ani_utils