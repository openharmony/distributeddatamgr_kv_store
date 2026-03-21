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

#ifndef DISTRIBUTED_KVSTORE_MOCK_FILE_EX_H
#define DISTRIBUTED_KVSTORE_MOCK_FILE_EX_H

#include <fcntl.h>
#include <string>
#include <sys/file.h>
#include <unistd.h>
#include <vector>

static bool SaveBufferToFile(const std::string &filePath, const std::vector<char> &content, bool truncated = true)
{
    return true;
}

static bool LoadBufferFromFile(const std::string &filePath, std::vector<char> &content)
{
    return true;
}

static bool FileExists(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

static bool SaveStringToFd(int fd, const std::string& content)
{
    if (fd <= 0) {
        return false;
    }

    if (content.empty()) {
        return true;
    }

    const ssize_t len = write(fd, content.c_str(), content.length());
    if (len < 0) {
        return false;
    }

    if (static_cast<unsigned long>(len) != content.length()) {
        return false;
    }

    return true;
}
#endif // DISTRIBUTED_KVSTORE_MOCK_FILE_EX_H
