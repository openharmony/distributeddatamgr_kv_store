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

#include "matrix_file.h"

#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "platform_specific.h"

namespace DistributedDB {

MatrixFile::MatrixFile()
{}

MatrixFile::~MatrixFile()
{
    if (fd_ >= 0) {
        close(fd_); // close file to trigger event
        fd_ = -1;
    }

    if (filePtr_ != nullptr) {
#ifndef _WIN32
        munmap(filePtr_, MATRIX_FILE_SIZE);
#endif
        filePtr_ = nullptr;
    }
}

int MatrixFile::AcquireWithRetry(const std::string &path)
{
    std::string realPath;
    int errCode = GetAndCheckRealPath(path, realPath);
    if (errCode != E_OK) {
        LOGE("[AcquireWithRetry] Get real path err: %d, %s", errCode,
            DBCommon::StringMiddleMaskingWithLen(path).c_str());
        return errCode;
    }

    int fd = open(realPath.c_str(), O_RDWR);
    if (fd < 0) {
        LOGE("[AcquireWithRetry] Open matrix file err: %d, %s", errno,
            DBCommon::StringMiddleMaskingWithLen(path).c_str());
        return -E_INVALID_FILE;
    }
    fd_ = fd;
    return E_OK;
}

int MatrixFile::WriteMatrixFile(const std::vector<uint64_t> &indexes) const
{
    if (filePtr_ == nullptr) {
        return -E_INVALID_FILE;
    }

    for (const uint64_t index : indexes) {
        if (index < MAX_SLOT_NUM) {
            filePtr_[index] += 1;
        }
    }

#ifndef _WIN32
    if (msync(filePtr_, MATRIX_FILE_SIZE, MS_SYNC) != 0) {
        LOGE("[WriteMatrixFile] msync err: %d", errno);
        return -E_SYSTEM_API_FAIL;
    }
#endif
    return E_OK;
}

int MatrixFile::MapMatrixFile()
{
    if (filePtr_ != nullptr) {
        return E_OK;
    }
    if (fd_ < 0) {
        return -E_INVALID_FILE;
    }
#ifndef _WIN32
    void *statusData = mmap(nullptr, MATRIX_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (statusData == MAP_FAILED) {
        LOGE("[MapMatrixFile] mmap err: %d, size: %zu", errno, MATRIX_FILE_SIZE);
        return -E_SYSTEM_API_FAIL;
    }

    int errCode = madvise(statusData, MATRIX_FILE_SIZE, MADV_RANDOM);
    if (errCode != 0) {
        LOGE("[MapMatrixFile] madvise err: %d", errno);
        munmap(statusData, MATRIX_FILE_SIZE);
        return -E_SYSTEM_API_FAIL;
    }

    filePtr_ = static_cast<uint64_t *>(statusData);
    return E_OK;
#else
    LOGE("[MapMatrixFile] mmap is not supported on windows");
    return -E_NOT_SUPPORT;
#endif
}

uint64_t MatrixFile::GetValueByIndex(uint64_t index) const
{
    if (filePtr_ == nullptr || index >= MAX_SLOT_NUM) {
        return 0u;
    }
    return filePtr_[index];
}

int MatrixFile::GetAndCheckRealPath(const std::string &src, std::string &out) const
{
    int errCode = OS::GetRealPath(src, out);
    if (errCode != E_OK) {
        LOGE("[GetAndCheckRealPath] Get real path err: %d", errCode);
        return -E_INVALID_FILE;
    }

    struct stat st;
    if (stat(out.c_str(), &st) == -1) {
        LOGE("[GetAndCheckRealPath] Check file stat err: %d", errno);
        return -E_INVALID_FILE;
    }

    if (st.st_size != static_cast<int64_t>(MATRIX_FILE_SIZE)) {
        LOGE("[GetAndCheckRealPath] File size err, expected: %zu, actual: %ld", MATRIX_FILE_SIZE, st.st_size);
        return -E_INVALID_FILE;
    }
    return E_OK;
}
}   // DistributedDB