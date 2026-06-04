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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_JSON_UTIL_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_JSON_UTIL_H

#include <string>

#include "appexecfwk_errors.h"
#include "app_log_wrapper.h"
#include "json_serializer.h"

namespace OHOS {
namespace AppExecFwk {
enum class JsonType : int8_t {
    NULLABLE,
    BOOLEAN,
    NUMBER,
    OBJECT,
    ARRAY,
    STRING,
};

enum class ArrayType : int8_t {
    NUMBER,
    OBJECT,
    STRING,
    NOT_ARRAY,
};

class BMSJsonUtil {
public:
    static void GetStrValueIfFindKey(const nlohmann::json &jsonObject,
        const nlohmann::detail::iter_impl<const nlohmann::json> &end,
        const std::string &key, std::string &data, bool isNecessary, int32_t &parseResult);
    static void GetBoolValueIfFindKey(const nlohmann::json &jsonObject,
        const nlohmann::detail::iter_impl<const nlohmann::json> &end,
        const std::string &key, bool &data, bool isNecessary, int32_t &parseResult);
    static bool CheckArrayValueType(const nlohmann::json &value, ArrayType arrayType);
    static bool CheckMapValueType(const nlohmann::json &value, JsonType valueType, ArrayType arrayType);
};

template<typename T, typename dataType>
void CheckArrayType(
    const nlohmann::detail::iter_impl<const nlohmann::json> &iter,
    dataType &data, ArrayType arrayType, int32_t &parseResult)
{
    if (iter->is_null() || !iter->is_array()) {
        return;
    }
    const auto &arrays = *iter;
    if (arrays.empty()) {
        return;
    }
    if (arrays.size() > Constants::MAX_JSON_ARRAY_LENGTH) {
        parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_SIZE_CHECK_ERROR;
        return;
    }
    switch (arrayType) {
        case ArrayType::STRING:
            for (const auto &array : arrays) {
                if (!array.is_string()) {
                    APP_LOGE("Array element not string type");
                    parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                }
            }
            if (parseResult == ERR_OK) {
                data = iter->get<T>();
            }
            break;
        case ArrayType::OBJECT:
            for (const auto &array : arrays) {
                if (!array.is_object()) {
                    APP_LOGE("Array element not object type");
                    parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                    break;
                }
            }
            if (parseResult == ERR_OK) {
                data = iter->get<T>();
            }
            break;
        case ArrayType::NUMBER:
            for (const auto &array : arrays) {
                if (!array.is_number()) {
                    APP_LOGE("Array element not number type");
                    parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                }
            }
            if (parseResult == ERR_OK) {
                data = iter->get<T>();
            }
            break;
        case ArrayType::NOT_ARRAY:
            APP_LOGE("Invalid array type");
            break;
        default:
            APP_LOGE("Unknown array type");
            break;
    }
}

template<typename T, typename dataType>
void GetValueIfFindKey(const nlohmann::json &jsonObject, const nlohmann::detail::iter_impl<const nlohmann::json> &end,
    const std::string &key, dataType &data, JsonType jsonType, bool isNecessary, int32_t &parseResult,
    ArrayType arrayType)
{
    if (parseResult) {
        return;
    }
    auto iter = jsonObject.find(key);
    if (iter != end) {
        switch (jsonType) {
            case JsonType::NUMBER:
                if (!iter->is_number()) {
                    APP_LOGE("type error %{public}s not number", key.c_str());
                    parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                    break;
                }
                data = iter->get<T>();
                break;
            case JsonType::OBJECT:
                if (!iter->is_object()) {
                    APP_LOGE("type error %{public}s not object", key.c_str());
                    parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                    break;
                }
                data = iter->get<T>();
                break;
            case JsonType::ARRAY:
                if (!iter->is_array()) {
                    APP_LOGE("type error %{public}s not array", key.c_str());
                    parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                    break;
                }
                CheckArrayType<T>(iter, data, arrayType, parseResult);
                break;
            case JsonType::NULLABLE:
                // Nullable type, accept any value or null
                APP_LOGE("type error %{public}s nullable", key.c_str());
                break;
            default:
                APP_LOGE("type error %{public}s not jsonType", key.c_str());
                parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
        }
        return;
    }

    if (isNecessary) {
        APP_LOGE("profile prop %{public}s missing", key.c_str());
        parseResult = ERR_APPEXECFWK_PARSE_PROFILE_MISSING_PROP;
    }
}

template<typename T>
const std::string GetJsonStrFromInfo(T &t)
{
    nlohmann::json json = t;
    return json.dump();
}

template<typename T>
bool ParseInfoFromJsonStr(const char *data, T &t)
{
    if (data == nullptr) {
        APP_LOGE("data is nullptr");
        return false;
    }

    nlohmann::json jsonObject = nlohmann::json::parse(data, nullptr, false, true);
    if (jsonObject.is_discarded()) {
        APP_LOGE("jsonObject discarded");
        return false;
    }

    t = jsonObject.get<T>();
    return true;
}

template<typename T, typename dataType>
void GetBigStringIfFindKey(const nlohmann::json &jsonObject,
    const nlohmann::detail::iter_impl<const nlohmann::json> &end, const std::string &key, dataType &data,
    JsonType jsonType, bool isNecessary, int32_t &parseResult, ArrayType arrayType)
{
    if (parseResult) {
        return;
    }
    // Performance optimization: Use iterator to reduce JSON lookups from 3 to 1
    auto iter = jsonObject.find(key);
    if (iter != end) {
        switch (jsonType) {
            case JsonType::STRING:
                if (!iter->is_string()) {
                    APP_LOGE("type error %{public}s not string", key.c_str());
                    parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                    break;
                }
                data = iter->get<T>();
                break;
            default:
                APP_LOGE("type error %{public}s not jsonType", key.c_str());
                parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
        }
        return;
    }

    if (isNecessary) {
        APP_LOGE("profile prop %{public}s missing", key.c_str());
        parseResult = ERR_APPEXECFWK_PARSE_PROFILE_MISSING_PROP;
    }
}

/**
 * @brief Retrieves a map value from a JSON object if the specified key exists, with type validation.
 * @param valueType The expected type of map values.
 *                  Supported types: [BOOLEAN, NUMBER, STRING, ARRAY]. Returns an error if the type is not supported.
 * @param arrayType If valueType is ARRAY, specifies the expected type of array items.
 *                  Supported types: [NUMBER, STRING]. Returns an error if the type is not supported.
 */
template<typename T, typename dataType>
void GetMapValueIfFindKey(const nlohmann::json &jsonObject,
    const nlohmann::detail::iter_impl<const nlohmann::json> &end, const std::string &key, dataType &data,
    bool isNecessary, int32_t &parseResult, JsonType valueType, ArrayType arrayType)
{
    if (parseResult != ERR_OK) {
        return;
    }
    // Performance optimization: Use iterator to reduce JSON lookups from 3 to 1
    auto iter = jsonObject.find(key);
    if (iter != end) {
        if (!iter->is_object()) {
            APP_LOGE("type error %{public}s not map object", key.c_str());
            parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
            return;
        }
        for (const auto& [mapKey, mapValue] : iter->items()) {
            if (!BMSJsonUtil::CheckMapValueType(mapValue, valueType, arrayType)) {
                APP_LOGE("type error key:%{public}s", mapKey.c_str());
                parseResult = ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR;
                return;
            }
        }
        data = iter->get<T>();
        return;
    }
    if (isNecessary) {
        APP_LOGE("profile prop %{public}s missing", key.c_str());
        parseResult = ERR_APPEXECFWK_PARSE_PROFILE_MISSING_PROP;
    }
}
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_JSON_UTIL_H
