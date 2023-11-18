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

#ifndef DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ITYPES_H
#define DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ITYPES_H

#include <cstdint>
#include <string>
#include <type_traits>

namespace OHOS {
namespace CommonTypes {
struct TsString {
    std::string value;
    TsString() = default;
    explicit TsString(const std::string &str) : value(str) {}
    operator std::string() const
    {
        return value;
    }
    bool operator==(const TsString &data)
    {
        return value == data.value;
    }
};

template<typename T>
struct Result {
    int32_t errCode;
    TsString description;
    using value_type = typename std::conditional<std::is_void<T>::value, std::nullptr_t, T>::type;
    value_type value;
};

} // namespace CommonTypes
} // namespace OHOS
#endif // DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ITYPES_H
