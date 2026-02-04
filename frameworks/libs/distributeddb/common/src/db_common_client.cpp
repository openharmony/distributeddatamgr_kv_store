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

#include "db_common.h"
#include "value_hash_calc.h"

namespace DistributedDB {
namespace {
    constexpr const int32_t HEAD_SIZE = 3;
    constexpr const int32_t END_SIZE = 3;
    constexpr const int32_t MIN_SIZE = HEAD_SIZE + END_SIZE + 3;
    constexpr const char *REPLACE_CHAIN = "***";
    constexpr const char *DEFAULT_ANONYMOUS = "******";

    const std::string HEX_CHAR_MAP = "0123456789abcdef";
}

void DBCommon::StringToVector(const std::string &src, std::vector<uint8_t> &dst)
{
    dst.resize(src.size());
    dst.assign(src.begin(), src.end());
}

std::string DBCommon::StringMiddleMasking(const std::string &name)
{
    if (name.length() <= HEAD_SIZE) {
        return DEFAULT_ANONYMOUS;
    }

    if (name.length() < MIN_SIZE) {
        return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN);
    }

    return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN + name.substr(name.length() - END_SIZE, END_SIZE));
}

std::string DBCommon::StringMiddleMaskingWithLen(const std::string &name)
{
    return StringMiddleMasking(name).append(", len[").append(std::to_string(name.length())).append("]");
}

void DBCommon::RTrim(std::string &oriString)
{
    if (oriString.empty()) {
        return;
    }
    oriString.erase(oriString.find_last_not_of(" ") + 1);
}

bool DBCommon::CheckIsAlnumOrUnderscore(const std::string &text)
{
    auto iter = std::find_if_not(text.begin(), text.end(), [](char c) {
        return (std::isalnum(c) || c == '_');
    });
    return iter == text.end();
}

std::string DBCommon::ToLowerCase(const std::string &str)
{
    std::string res(str.length(), ' ');
    std::transform(str.begin(), str.end(), res.begin(), ::tolower);
    return res;
}

std::string DBCommon::ToUpperCase(const std::string &str)
{
    std::string res(str.length(), ' ');
    std::transform(str.begin(), str.end(), res.begin(), ::toupper);
    return res;
}

int DBCommon::CalcValueHash(const std::vector<uint8_t> &value, std::vector<uint8_t> &hashValue)
{
    ValueHashCalc hashCalc;
    int errCode = hashCalc.Initialize();
    if (errCode != E_OK) {
        return -E_INTERNAL_ERROR;
    }

    errCode = hashCalc.Update(value);
    if (errCode != E_OK) {
        return -E_INTERNAL_ERROR;
    }

    errCode = hashCalc.GetResult(hashValue);
    if (errCode != E_OK) {
        return -E_INTERNAL_ERROR;
    }

    return E_OK;
}


namespace {
bool CharIn(char c, const std::string &pattern)
{
    return std::any_of(pattern.begin(), pattern.end(), [c] (char p) {
        return c == p;
    });
}
}
bool DBCommon::HasConstraint(const std::string &sql, const std::string &keyWord, const std::string &prePattern,
    const std::string &nextPattern)
{
    size_t pos = 0;
    while ((pos = sql.find(keyWord, pos)) != std::string::npos) {
        if (pos >= 1 && CharIn(sql[pos - 1], prePattern) && ((pos + keyWord.length() == sql.length()) ||
            ((pos + keyWord.length() < sql.length()) && CharIn(sql[pos + keyWord.length()], nextPattern)))) {
            return true;
        }
        pos++;
    }
    return false;
}

std::string DBCommon::TransferStringToHex(const std::string &origStr)
{
    if (origStr.empty()) {
        return "";
    }

    std::string tmp;
    for (auto item : origStr) {
        unsigned char currentByte = static_cast<unsigned char>(item);
        tmp.push_back(HEX_CHAR_MAP[currentByte >> 4]); // high 4 bits to one hex.
        tmp.push_back(HEX_CHAR_MAP[currentByte & 0x0F]); // low 4 bits to one hex.
    }
    return tmp;
}

std::string DBCommon::GetCursorKey(const std::string &tableName)
{
    return std::string(DBConstant::RELATIONAL_PREFIX) + "cursor_" + ToLowerCase(tableName);
}

std::string DBCommon::TrimSpace(const std::string &input)
{
    std::string res;
    res.reserve(input.length());
    bool isPreSpace = true;
    for (char c : input) {
        if (std::isspace(c)) {
            isPreSpace = true;
        } else {
            if (!res.empty() && isPreSpace) {
                res += ' ';
            }
            res += c;
            isPreSpace = false;
        }
    }
    res.shrink_to_fit();
    return res;
}

std::string DBCommon::TransferHashString(const std::string &devName)
{
    if (devName.empty()) {
        return "";
    }
    std::vector<uint8_t> devVect(devName.begin(), devName.end());
    std::vector<uint8_t> hashVect;
    int errCode = CalcValueHash(devVect, hashVect);
    if (errCode != E_OK) {
        return "";
    }

    return std::string(hashVect.begin(), hashVect.end());
}

bool DBCommon::CaseInsensitiveCompare(const std::string &first, const std::string &second)
{
    return (strcasecmp(first.c_str(), second.c_str()) == 0);
}
} // namespace DistributedDB
