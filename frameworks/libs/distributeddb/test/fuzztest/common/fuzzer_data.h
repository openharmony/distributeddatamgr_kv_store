/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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


#ifndef FUZZER_DATA_H
#define FUZZER_DATA_H

#include <cstdint>
#include <vector>
#include <set>
#include <string>

namespace DistributedDBTest {

class FuzzerData final {
public:
    FuzzerData(const uint8_t *data, size_t size);
    ~FuzzerData();

    int GetInt();
    uint32_t GetUInt32();
    uint64_t GetUInt64();
    std::vector<uint8_t> GetSequence(size_t size, uint32_t mod = MOD);
    std::string GetString(size_t len);
    std::vector<std::string> GetStringVector(size_t size);
    std::set<std::string> GetStringSet(size_t size);
    std::vector<std::u16string> GetU16StringVector(size_t size);
private:
    static const uint32_t MOD = 1024; // MOD length

    const uint8_t *data_;
    const size_t size_;
    const uint8_t *curr_;
};
} // DistributedDBTest
#endif // FUZZER_DATA_H