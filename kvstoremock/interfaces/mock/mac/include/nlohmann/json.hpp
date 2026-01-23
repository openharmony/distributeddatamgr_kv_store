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

#ifndef OHOS_KV_STORE_IOS_NLOHMANN_JSON_HPP
#define OHOS_KV_STORE_IOS_NLOHMANN_JSON_HPP

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace nlohmann {
class json {
public:
    enum class Type {
        NULL_TYPE,
        OBJECT_TYPE,
        ARRAY_TYPE,
        STRING_TYPE,
        NUMBER_TYPE,
        BOOLEAN_TYPE,
    };

    using object_init = std::pair<std::string, json>;

    json();
    json(const char* value);
    json(const std::string& value);
    json(bool value);
    json(int32_t value);
    json(uint32_t value);
    json(std::initializer_list<object_init> init);

    json(const json& other) = default;
    json(json&& other) noexcept = default;
    json& operator=(const json& other) = default;
    json& operator=(json&& other) noexcept = default;
    ~json() = default;

    static json array();

    json& operator[](const std::string& key);

    void push_back(const std::string& value);

    std::string dump() const;

private:
    void EnsureObject();
    void EnsureArray();

    Type type_ = Type::NULL_TYPE;
    std::map<std::string, json> object_;
    std::vector<json> array_;
    std::string string_;
    double number_ = 0.0;
    bool boolean_ = false;
};
} // namespace nlohmann
#endif // OHOS_KV_STORE_IOS_NLOHMANN_JSON_HPP
