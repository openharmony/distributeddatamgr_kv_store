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

#include "fuzzer_data.h"


namespace DistributedDBTest {
FuzzerData::FuzzerData(const uint8_t *data, size_t size) : data_(data), size_(size), curr_(data)
{
}

FuzzerData::~FuzzerData() = default;

int FuzzerData::GetInt()
{
    if (curr_ + sizeof(int) > data_ + size_) {
        return 0;
    }
    auto *r = reinterpret_cast<const int *>(curr_);
    curr_ += sizeof(int);
    return *r;
}

uint32_t FuzzerData::GetUInt32()
{
    if (curr_ + sizeof(uint32_t) > data_ + size_) {
        return 0;
    }
    auto *r = reinterpret_cast<const uint32_t *>(curr_);
    curr_ += sizeof(uint32_t);
    return *r;
}

uint64_t FuzzerData::GetUInt64()
{
    if (curr_ + sizeof(uint64_t) > data_ + size_) {
        return 0;
    }
    auto *r = reinterpret_cast<const uint64_t *>(curr_);
    curr_ += sizeof(uint64_t);
    return *r;
}

std::vector<uint8_t> FuzzerData::GetSequence(size_t size)
{
    if (curr_ + size > data_ + size_) {
        return {};
    }
    std::vector<uint8_t> r(curr_, curr_ + size);
    curr_ += size;
    return r;
}
} // DistributedDBTest
