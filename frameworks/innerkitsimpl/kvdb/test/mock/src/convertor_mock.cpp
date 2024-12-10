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

#include "include/convertor_mock.h"

namespace OHOS::DistributedKv {
std::vector<uint8_t> Convertor::ToLocalDBKey(const Key &key) const
{
    if (BConvertor::convertor == nullptr) {
        std::vector<uint8_t> vec;
        return vec;
    }
    return BConvertor::convertor->ToLocalDBKey(key);
}

std::vector<uint8_t> Convertor::ToWholeDBKey(const Key &key) const
{
    if (BConvertor::convertor == nullptr) {
        std::vector<uint8_t> vc;
        return vc;
    }
    return BConvertor::convertor->ToWholeDBKey(key);
}

Key Convertor::ToKey(DBKey &&key, std::string &deviceId) const
{
    return std::move(key);
}

std::vector<uint8_t> Convertor::GetPrefix(const Key &prefix) const
{
    if (BConvertor::convertor == nullptr) {
        std::vector<uint8_t> dbKey;
        return dbKey;
    }
    return BConvertor::convertor->GetPrefix(prefix);
}

std::vector<uint8_t> Convertor::GetPrefix(const DataQuery &query) const
{
    if (BConvertor::convertor == nullptr) {
        std::vector<uint8_t> vec;
        return vec;
    }
    return BConvertor::convertor->GetPrefix(query);
}

Convertor::DBQuery Convertor::GetDBQuery(const DataQuery &query) const
{
    if (BConvertor::convertor == nullptr) {
        DBQuery dbQuery = *(query.query_);
        return dbQuery;
    }
    return BConvertor::convertor->GetDBQuery(query);
}

std::string Convertor::GetRealKey(const std::string &key, const DataQuery &query) const
{
    if (BConvertor::convertor == nullptr) {
        return "";
    }
    return BConvertor::convertor->GetRealKey(key, query);
}

std::vector<uint8_t> Convertor::TrimKey(const Key &prefix) const
{
    if (BConvertor::convertor == nullptr) {
        std::vector<uint8_t> vc;
        return vc;
    }
    return BConvertor::convertor->TrimKey(prefix);
}
} // namespace OHOS::DistributedKv