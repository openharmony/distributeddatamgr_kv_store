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

#include "query_utils.h"

namespace DistributedDB {
void QueryUtils::FillQueryIn(const std::string &col, const std::vector<Type> &data, size_t valueType,
    Query &query)
{
    switch (valueType) {
        case TYPE_INDEX<int64_t>: {
            std::vector<int64_t> pkList;
            for (const auto &pk : data) {
                pkList.push_back(std::get<int64_t>(pk));
            }
            query.In(col, pkList);
            break;
        }
        case TYPE_INDEX<std::string>: {
            std::vector<std::string> pkList;
            for (const auto &pk : data) {
                pkList.push_back(std::get<std::string>(pk));
            }
            query.In(col, pkList);
            break;
        }
        case TYPE_INDEX<double>: {
            std::vector<double> pkList;
            for (const auto &pk : data) {
                pkList.push_back(std::get<double>(pk));
            }
            query.In(col, pkList);
            break;
        }
        case TYPE_INDEX<bool>: {
            std::vector<bool> pkList;
            for (const auto &pk : data) {
                pkList.push_back(std::get<bool>(pk));
            }
            query.In(col, pkList);
            break;
        }
        default:
            break;
    }
}

void QueryUtils::FillQueryInKeys(const std::map<std::string, std::vector<Type>> &syncPk,
    std::map<std::string, size_t> dataIndex, Query &query)
{
    std::set<Key> keys;
    for (const auto &[col, pkList] : syncPk) {
        switch (dataIndex[col]) {
            case TYPE_INDEX<std::string>:
                FillStringQueryKeys(keys, pkList);
                break;
            case TYPE_INDEX<Bytes>:
                FillByteQueryKeys(keys, pkList);
                break;
            default:
                break;
        }
    }
    if (!keys.empty()) {
        query.InKeys(keys);
    }
}

void QueryUtils::FillStringQueryKeys(std::set<Key> &keys, const std::vector<Type> &pkList)
{
    for (const auto &pk : pkList) {
        const std::string *keyStrPtr = std::get_if<std::string>(&pk);
        if (keyStrPtr == nullptr) {
            continue;
        }
        keys.insert(Key((*keyStrPtr).begin(), (*keyStrPtr).end()));
    }
}

void QueryUtils::FillByteQueryKeys(std::set<Key> &keys, const std::vector<Type> &pkList)
{
    for (const auto &pk : pkList) {
        if (std::get_if<Bytes>(&pk) == nullptr) {
            continue;
        }
        keys.insert(std::get<Bytes>(pk));
    }
}
} // DistributedDB