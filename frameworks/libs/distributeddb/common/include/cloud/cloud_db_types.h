/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef CLOUD_DB_TYPES_H
#define CLOUD_DB_TYPES_H

#include "cloud/cloud_store_types.h"
#include <string>

namespace DistributedDB {
enum class CloudWaterType {
    INSERT,
    UPDATE,
    DELETE
};

struct CloudSyncBatch {
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    std::vector<int64_t> rowid;
    std::vector<int64_t> timestamp;
    std::vector<VBucket> assets;
    std::vector<Bytes> hashKey;
};

struct CloudSyncData {
    std::string tableName;
    CloudSyncBatch insData;
    CloudSyncBatch updData;
    CloudSyncBatch delData;
    CloudSyncBatch lockData;
    bool isCloudForcePushStrategy = false;
    bool isCompensatedTask = false;
    bool isShared = false;
    int ignoredCount = 0;
    CloudWaterType mode;
    bool isCloudVersionRecord = false;
    CloudSyncData() = default;
    CloudSyncData(const std::string &_tableName) : tableName(_tableName) {};
    CloudSyncData(const std::string &_tableName, CloudWaterType _mode) : tableName(_tableName), mode(_mode) {};
};

struct CloudTaskConfig {
    bool allowLogicDelete = false;
};

template<typename Tp, typename... Types>
struct index_of : std::integral_constant<size_t, 0> {};

template<typename Tp, typename... Types>
inline static constexpr size_t index_of_v = index_of<Tp, Types...>::value;

template<typename Tp, typename First, typename... Rest>
struct index_of<Tp, First, Rest...>
    : std::integral_constant<size_t, std::is_same_v<Tp, First> ? 0 : index_of_v<Tp, Rest...> + 1> {};

template<typename... Types>
struct variant_size_of {
    static constexpr size_t value = sizeof...(Types);
};

template<typename T, typename... Types>
struct variant_index_of {
    static constexpr size_t value = index_of_v<T, Types...>;
};

template<typename... Types>
static variant_size_of<Types...> variant_size_test(const std::variant<Types...> &);

template<typename T, typename... Types>
static variant_index_of<T, Types...> variant_index_test(const T &, const std::variant<Types...> &);

template<typename T>
inline constexpr static int32_t TYPE_INDEX =
    decltype(variant_index_test(std::declval<T>(), std::declval<Type>()))::value;

inline constexpr static int32_t TYPE_MAX = decltype(variant_size_test(std::declval<Type>()))::value;

} // namespace DistributedDB
#endif // CLOUD_DB_TYPES_H
