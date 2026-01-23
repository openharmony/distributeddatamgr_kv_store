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
#include "nlohmann/json.hpp"

#include <cmath>
#include <cstddef>
#include <cstdio>
#import <Foundation/Foundation.h>
#include <iomanip>
#include <sstream>

namespace nlohmann {
static constexpr unsigned char JSON_CONTROL_CHAR_MAX = 0x20;
static constexpr int JSON_NUMBER_PRECISION = 15;
} // namespace nlohmann

namespace {
std::string EscapeJsonChar(unsigned char c)
{
    switch (c) {
        case '\\':
            return "\\\\";
        case '"':
            return "\\\"";
        case '\b':
            return "\\b";
        case '\f':
            return "\\f";
        case '\n':
            return "\\n";
        case '\r':
            return "\\r";
        case '\t':
            return "\\t";
        default:
            if (c < nlohmann::JSON_CONTROL_CHAR_MAX) {
                NSString* escaped = [NSString stringWithFormat:@"\\u%04X", c];
                const char* utf8 = [escaped UTF8String];
                return (utf8 != nullptr) ? std::string(utf8) : std::string();
            }
            return std::string(1, static_cast<char>(c));
    }
}

std::string EscapeJsonString(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    for (unsigned char c : input) {
        out += EscapeJsonChar(c);
    }
    return out;
}

std::string DumpNumber(double value)
{
    if (!std::isfinite(value)) {
        return "null";
    }
    std::ostringstream oss;
    oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
    oss << std::setprecision(nlohmann::JSON_NUMBER_PRECISION) << value;
    return oss.str();
}
} // namespace

namespace nlohmann {
json::json() = default;

json::json(const char* value)
    : type_(Type::STRING_TYPE),
      string_(value != nullptr ? value : "")
{
}

json::json(const std::string& value)
    : type_(Type::STRING_TYPE),
      string_(value)
{
}

json::json(bool value)
    : type_(Type::BOOLEAN_TYPE),
      boolean_(value)
{
}

json::json(int32_t value)
    : type_(Type::NUMBER_TYPE),
      number_(static_cast<double>(value))
{
}

json::json(uint32_t value)
    : type_(Type::NUMBER_TYPE),
      number_(static_cast<double>(value))
{
}

json::json(std::initializer_list<object_init> init)
    : type_(Type::OBJECT_TYPE)
{
    for (const auto& item : init) {
        object_[item.first] = item.second;
    }
}

json json::array()
{
    json value;
    value.type_ = Type::ARRAY_TYPE;
    return value;
}

json& json::operator[](const std::string& key)
{
    EnsureObject();
    return object_[key];
}

void json::push_back(const std::string& value)
{
    EnsureArray();
    array_.push_back(json(value));
}

std::string json::dump() const
{
    std::string out;
    NSMutableString* buffer = [NSMutableString string];
    auto appendStd = [&](const std::string& value) {
        if (value.empty()) {
            return;
        }
        NSString* str = [[NSString alloc] initWithBytes:value.data()
            length:value.size()
            encoding:NSUTF8StringEncoding];
        if (str != nil) {
            [buffer appendString:str];
        }
    };

    auto appendArray = [&](const std::vector<json>& values, const auto& appendRef) {
        [buffer appendString:@"["];
        bool first = true;
        for (const auto& item : values) {
            [buffer appendString:(first ? @"" : @",")];
            first = false;
            appendRef(item, appendRef);
        }
        [buffer appendString:@"]"];
    };

    auto appendObject = [&](const std::map<std::string, json>& values, const auto& appendRef) {
        [buffer appendString:@"{"];
        bool first = true;
        for (const auto& item : values) {
            [buffer appendString:(first ? @"" : @",")];
            first = false;
            [buffer appendString:@"\""];
            appendStd(EscapeJsonString(item.first));
            [buffer appendString:@"\":"];
            appendRef(item.second, appendRef);
        }
        [buffer appendString:@"}"];
    };

    auto appendValue = [&](const json& value, const auto& appendRef) -> void {
        if (value.type_ == Type::NULL_TYPE) {
            [buffer appendString:@"null"];
            return;
        }
        if (value.type_ == Type::BOOLEAN_TYPE) {
            [buffer appendString:(value.boolean_ ? @"true" : @"false")];
            return;
        }
        if (value.type_ == Type::NUMBER_TYPE) {
            appendStd(DumpNumber(value.number_));
            return;
        }
        if (value.type_ == Type::STRING_TYPE) {
            [buffer appendString:@"\""];
            appendStd(EscapeJsonString(value.string_));
            [buffer appendString:@"\""];
            return;
        }
        if (value.type_ == Type::ARRAY_TYPE) {
            appendArray(value.array_, appendRef);
            return;
        }
        appendObject(value.object_, appendRef);
    };

    appendValue(*this, appendValue);
    const char* utf8 = [buffer UTF8String];
    if (utf8 != nullptr) {
        out = utf8;
    }
    return out;
}

void json::EnsureObject()
{
    if (type_ == Type::OBJECT_TYPE) {
        return;
    }
    type_ = Type::OBJECT_TYPE;
    object_.clear();
    array_.clear();
    string_.clear();
    number_ = 0.0;
    boolean_ = false;
}

void json::EnsureArray()
{
    if (type_ == Type::ARRAY_TYPE) {
        return;
    }
    type_ = Type::ARRAY_TYPE;
    array_.clear();
    object_.clear();
    string_.clear();
    number_ = 0.0;
    boolean_ = false;
}
} // namespace nlohmann
