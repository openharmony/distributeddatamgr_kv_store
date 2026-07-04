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

#ifndef MATRIX_FILE_H
#define MATRIX_FILE_H

#include <cstdint>
#include <string>
#include <vector>

namespace DistributedDB {

class MatrixFile {
public:
    MatrixFile();
    ~MatrixFile();

    int AcquireWithRetry(const std::string &path);

    int MapMatrixFile();

    int WriteMatrixFile(const std::vector<uint64_t> &indexes) const;

    uint64_t GetValueByIndex(uint64_t index) const;

    static constexpr size_t MAX_SLOT_NUM = 100;
    static constexpr size_t MATRIX_FILE_SLOT_SIZE = sizeof(uint64_t);
    static constexpr size_t MATRIX_FILE_SIZE = MAX_SLOT_NUM * MATRIX_FILE_SLOT_SIZE;

private:
    int GetAndCheckRealPath(const std::string &src, std::string &out) const;

    int fd_ = -1;
    uint64_t *filePtr_ = nullptr;
};

}   // DistributedDB
#endif  // MATRIX_FILE_H