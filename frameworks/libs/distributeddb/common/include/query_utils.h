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

#ifndef QUERY_UTILS_H
#define QUERY_UTILS_H

#include "cloud/cloud_db_types.h"
#include "query.h"

namespace DistributedDB {
class QueryUtils {
public:
    static void FillQueryIn(const std::string &col, const std::vector<Type> &data, size_t valueType, Query &query);

    static void FillQueryInKeys(const std::map<std::string, std::vector<Type>> &syncPk,
        std::map<std::string, size_t> dataIndex, Query &query);
private:
    static void FillStringQueryKeys(std::set<Key> &keys, const std::vector<Type> &pkList);

    static void FillByteQueryKeys(std::set<Key> &keys, const std::vector<Type> &pkList);
};
} // DistributedDB
#endif // QUERY_UTILS_H
