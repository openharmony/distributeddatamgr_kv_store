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

#ifndef DB_BASE64_UTILS_H
#define DB_BASE64_UTILS_H

#include <string>
#include <vector>

namespace DistributedDB {
class DBBase64Utils {
public:
    static std::string Encode(const std::vector<uint8_t> &source);
    static std::vector<uint8_t> Decode(const std::string &encoded);

    static std::string Encode(const std::string &source);
    static std::string DecodeIfNeed(const std::string &source);
};
}
#endif // DB_BASE64_UTILS_H
