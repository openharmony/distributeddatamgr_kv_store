/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "platform_specific.h"

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <climits>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#if defined OS_TYPE_WINDOWS
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#endif

#include "db_errno.h"
#include "log_print.h"
#include "securec.h"

namespace DistributedDB {
namespace OS {
/*
 * Common part that is the same between each os
 */
#if defined OS_TYPE_WINDOWS
namespace {
    const int ACCESS_MODE_EXISTENCE = 0;
}
bool CheckPathExistence(const std::string &filePath)
{
    int ret = _access(filePath.c_str(), ACCESS_MODE_EXISTENCE);
    LOGI("CheckPathExistence %s error:%d", filePath.c_str(), ret);
    return (ret == 0);
}

int RenameFilePath(const std::string &oldFilePath, const std::string &newFilePath)
{
    int errCode = rename(oldFilePath.c_str(), newFilePath.c_str());
    if (errCode < 0) {
        LOGE("[Rename] Rename file fail. err = %d", errno);
        return -E_SYSTEM_API_FAIL;
    }
    LOGI("Rename file path successfully!");
    return E_OK;
}

int RemoveFile(const std::string &filePath)
{
    int errCode = remove(filePath.c_str());
    if (errCode < 0) {
        LOGE("[RemoveFile] Remove file fail %s %d err = %d", filePath.c_str(), errCode, errno);
        return -E_SYSTEM_API_FAIL;
    }
    LOGI("Remove file successfully!");
    return E_OK;
}

int CalFileSize(const std::string &fileUrl, uint64_t &size)
{
    struct _stat64 fileStat;
    if (fileUrl.empty() || _stat64(fileUrl.c_str(), &fileStat) < 0 || fileStat.st_size < 0) {
        int errCode = (errno == ENOENT) ? -E_NOT_FOUND : -E_INVALID_DB;
        LOGD("Get file[%zu] size failed, errno [%d].", fileUrl.size(), errno);
        return errCode;
    }

    size = static_cast<uint64_t>(fileStat.st_size);
    return E_OK;
}

void SplitFilePath(const std::string &filePath, std::string &fileDir, std::string &fileName)
{
    if (filePath.empty()) {
        return;
    }

    auto slashPos = filePath.find_last_of('/');
    if (slashPos == std::string::npos) {
        fileName = filePath;
        fileDir = "";
        return;
    }

    fileDir = filePath.substr(0, slashPos);
    fileName = filePath.substr(slashPos + 1);
    return;
}

int MakeDBDirectory(const std::string &directory)
{
    int errCode = mkdir(directory.c_str());
    if (errCode < 0) {
        LOGE("[MakeDir] Make directory fail:%d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    return E_OK;
}

int RemoveDBDirectory(const std::string &directory)
{
    int ret = rmdir(directory.c_str());
    LOGI("CheckPathExistence %s ret:%d error %d", directory.c_str(), ret, errno);
    return ret;
}

int CreateFileByFileName(const std::string &fileName)
{
    int fp = _open(fileName.c_str(), (O_WRONLY | O_CREAT), _S_IREAD | _S_IWRITE);
    if (fp < 0) {
        LOGE("[CreateFile] Create file fail:%d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    close(fp);
    return E_OK;
}

int GetRealPath(const std::string &inOriPath, std::string &outRealPath)
{
    const unsigned int MAX_PATH_LENGTH = PATH_MAX;
    if (inOriPath.length() > MAX_PATH_LENGTH || MAX_PATH_LENGTH > 0x10000) { // max limit is 64K(0x10000).
        LOGE("[RealPath] OriPath too long.");
        return -E_INVALID_ARGS;
    }

    char *realPath = new (std::nothrow) char[MAX_PATH_LENGTH + 1];
    if (realPath == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    if (memset_s(realPath, MAX_PATH_LENGTH + 1, 0, MAX_PATH_LENGTH + 1) != EOK) {
        delete [] realPath;
        return -E_SECUREC_ERROR;
    }

    if (_fullpath(realPath, inOriPath.c_str(), MAX_PATH_LENGTH) == nullptr) {
        LOGE("[OS] Realpath error:%d.", errno);
        delete [] realPath;
        return -E_SYSTEM_API_FAIL;
    }
    outRealPath = std::string(realPath);
    delete [] realPath;
    return E_OK;
}
namespace {
    const uint64_t MULTIPLES_BETWEEN_MICROSECONDS_AND_NANOSECONDS = 1000;
    const uint64_t MULTIPLES_BETWEEN_MICROSECONDS_AND_100_NANOSECONDS = 10;
    const uint64_t MICROSECONDS_OFFSET = 11644473600000000;  // microseconds from January 1, 1601 to January 1, 1970
    const uint64_t UINT32_BIT_LENGTH = 32;
}

int GetCurrentSysTimeInMicrosecond(uint64_t &outTime)
{
    FILETIME rawTime;
    GetSystemTimeAsFileTime(&rawTime);
    outTime = ((static_cast<uint64_t>(rawTime.dwHighDateTime) << UINT32_BIT_LENGTH) +
        (static_cast<uint64_t>(rawTime.dwLowDateTime))) / MULTIPLES_BETWEEN_MICROSECONDS_AND_100_NANOSECONDS -
        MICROSECONDS_OFFSET;
    return E_OK;
}

int GetMonotonicRelativeTimeInMicrosecond(uint64_t &outTime)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER rawTime;
    if (!QueryPerformanceFrequency(&frequency)) {
        LOGE("query performance frequency fail %d", errno);
        return -E_SYSTEM_API_FAIL;
    }

    if (frequency.QuadPart == 0) {
        LOGE("frequency quadpart zero!");
        return -E_SYSTEM_API_FAIL;
    }

    if (!QueryPerformanceCounter(&rawTime)) {
        LOGE("query frequency counter fail %d!", errno);
        return -E_SYSTEM_API_FAIL;
    }

    outTime = (static_cast<uint64_t>(rawTime.QuadPart) * MULTIPLES_BETWEEN_MICROSECONDS_AND_NANOSECONDS) /
        static_cast<uint64_t>(frequency.QuadPart);
    return E_OK;
}

static void InitFileType(const _finddata_t &file, FileAttr& fileAttr)
{
    switch (file.attrib) {
        case _A_NORMAL:
        case _A_ARCH:
            fileAttr.fileType = FILE;
            return;
        case _A_SUBDIR:
            fileAttr.fileType = PATH;
            return;
        default:
            fileAttr.fileType = OTHER;
            return;
    }
}

static int GetFilePathAttr(const std::string &topPath, const std::string &relativePath,
    std::list<FileAttr> &files, bool isNeedAllPath)
{
    _finddata_t file;
    std::string findPath = std::string(topPath) + "\\*.*";
    intptr_t handle = _findfirst(findPath.c_str(), &file);
    if (handle < 0) {
        LOGE("Open dir error:%s %d.", topPath.c_str(), errno);
        return -E_INVALID_PATH;
    }
    int errCode = E_OK;
    std::string fileAbsName;
    struct _stat64 fileStat;
    LOGE("find first file %s  %s relativePath %s", topPath.c_str(), file.name, relativePath.c_str());

    FileAttr fileAttr;
    do {
        InitFileType(file, fileAttr);

        if (strlen(file.name) == 0 || strcmp(file.name, ".") == 0 ||
            strcmp(file.name, "..") == 0) {
            continue;
        }

        fileAttr.fileName = relativePath + file.name;
        fileAbsName = topPath + "/" + file.name;
        errCode = _stat64(fileAbsName.c_str(), &fileStat);
        if (errCode != 0) {
            LOGE("[GetFileAttr]Get file stat failed, %s error = %d.", fileAbsName.c_str(), errno);
            errCode = -E_INVALID_PATH;
            break;
        }
        if (isNeedAllPath) {
            fileAttr.fileName = fileAbsName;
        }
        fileAttr.fileLen = static_cast<uint64_t>(fileStat.st_size);
        files.push_back(fileAttr);
        LOGE("find fileAbsName file %s %s %d relativePath %s", topPath.c_str(), file.name,
             file.attrib, relativePath.c_str());
        if (fileAttr.fileType == PATH) {
            errCode = GetFilePathAttr(fileAbsName, relativePath + file.name + "/", files, isNeedAllPath);
            if (errCode != E_OK) {
                LOGE("[GetFileAttr]GetFilePathAttr finish, %s error = %d.", topPath.c_str(), errCode);
                printf("GetFilePathAttr the finish %s error:%d\n", topPath.c_str(), errCode);
                break;
            }
        }
    } while (_findnext(handle, &file) == 0);
    _findclose(handle);
    LOGE("[GetFileAttr]GetFilePathAttr finish, %s error = %d.", topPath.c_str(), errCode);
    printf("GetFilePathAttr the finish end %s error:%d\n", topPath.c_str(), errCode);
    return errCode;
}

int GetFileAttrFromPath(const std::string &filePath, std::list<FileAttr> &files, bool isNeedAllPath)
{
    return GetFilePathAttr(filePath, std::string(), files, isNeedAllPath);
}

int GetFilePermissions(const std::string &fileName, uint32_t &permissions)
{
    struct _stat64 fileStat;
    int errCode = _stat64(fileName.c_str(), &fileStat);
    if (errCode != E_OK) {
        permissions = _S_IREAD | _S_IWRITE;
        LOGE("Get file stat failed, error = %d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    permissions = fileStat.st_mode & (_S_IRWXU | _S_IFMT | _S_IFDIR | _S_IFCHR | _S_IFIFO | _S_IFREG);
    return E_OK;
}

int SetFilePermissions(const std::string &fileName, uint32_t permissions)
{
    if (permissions > (S_IRWXU | S_IRWXG | S_IRWXO)) {
        return -E_INVALID_ARGS;
    }
    int errCode = _chmod(fileName.c_str(), _S_IREAD | _S_IWRITE);
    if (errCode != E_OK) {
        LOGE("Set file permissions failed, error = %d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    return E_OK;
}

int OpenFile(const std::string &fileName, FileHandle &handle)
{
    handle.handle = _open(fileName.c_str(), (O_WRONLY | O_CREAT), _S_IREAD | _S_IWRITE);
    if (handle.handle < 0) {
        LOGE("[FileLock] can not open file when lock it:[%d]", errno);
        return -E_SYSTEM_API_FAIL;
    }
    return E_OK;
}

int CloseFile(FileHandle &handle)
{
    if (close(handle.handle) != 0) {
        LOGE("close file failed, errno:%d", errno);
        return -E_SYSTEM_API_FAIL;
    }
    handle.handle = -1;
    return E_OK;
}

int FileLock(const FileHandle &handle, bool isBlock)
{
    if (handle.handle < 0) {
        LOGE("[FileLock] can not open file when lock it:[%d]", errno);
        return -E_SYSTEM_API_FAIL;
    }
    // windows下的LockFile与Linux差异较大，先不替换
    return E_OK;
}

int FileUnlock(FileHandle &handle)
{
    if (handle.handle == -1) {
        LOGI("[FileUnlock] file handle is invalid!");
        return E_OK;
    }

    return CloseFile(handle);
}
#else
namespace {
    const int ACCESS_MODE_EXISTENCE = 0;
    const uint64_t MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS = 1000000;
}
bool CheckPathExistence(const std::string &filePath)
{
    return (access(filePath.c_str(), ACCESS_MODE_EXISTENCE) == 0);
}

int RenameFilePath(const std::string &oldFilePath, const std::string &newFilePath)
{
    int errCode = rename(oldFilePath.c_str(), newFilePath.c_str());
    if (errCode < 0) {
        LOGE("[Rename] Rename file fail. err = %d", errno);
        return -E_SYSTEM_API_FAIL;
    }
    LOGI("Rename file path successfully!");
    return E_OK;
}

int RemoveFile(const std::string &filePath)
{
    int errCode = remove(filePath.c_str());
    if (errCode < 0) {
        LOGE("[RemoveFile] Remove file fail. err = %d", errno);
        return -E_SYSTEM_API_FAIL;
    }
    LOGI("Remove file successfully!");
    return E_OK;
}

int CalFileSize(const std::string &fileUrl, uint64_t &size)
{
    struct stat fileStat;
    if (fileUrl.empty() || stat(fileUrl.c_str(), &fileStat) < 0 || fileStat.st_size < 0) {
        int errCode = (errno == ENOENT) ? -E_NOT_FOUND : -E_INVALID_DB;
        LOGD("Get file[%zu] size failed, errno [%d].", fileUrl.size(), errno);
        return errCode;
    }

    size = static_cast<uint64_t>(fileStat.st_size);
    return E_OK;
}

void SplitFilePath(const std::string &filePath, std::string &fileDir, std::string &fileName)
{
    if (filePath.empty()) {
        return;
    }

    auto slashPos = filePath.find_last_of('/');
    if (slashPos == std::string::npos) {
        fileName = filePath;
        fileDir = "";
        return;
    }

    fileDir = filePath.substr(0, slashPos);
    fileName = filePath.substr(slashPos + 1);
    return;
}

int MakeDBDirectory(const std::string &directory)
{
    int errCode = mkdir(directory.c_str(), (S_IRWXU | S_IRWXG)); // The permission is 770 for linux based os
    if (errCode < 0) {
        LOGE("[MakeDir] Make directory fail:%d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    return E_OK;
}

int RemoveDBDirectory(const std::string &directory)
{
    return remove(directory.c_str());
}

int CreateFileByFileName(const std::string &fileName)
{
    int fp = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
    if (fp < 0) {
        LOGE("[CreateFile] Create file fail:%d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    close(fp);
    return E_OK;
}

int GetRealPath(const std::string &inOriPath, std::string &outRealPath)
{
    const unsigned int MAX_PATH_LENGTH = PATH_MAX;
    if (inOriPath.length() > MAX_PATH_LENGTH || MAX_PATH_LENGTH > 0x10000) { // max limit is 64K(0x10000).
        LOGE("[RealPath] OriPath too long.");
        return -E_INVALID_ARGS;
    }

    char *realPath = new (std::nothrow) char[MAX_PATH_LENGTH + 1];
    if (realPath == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    if (memset_s(realPath, MAX_PATH_LENGTH + 1, 0, MAX_PATH_LENGTH + 1) != EOK) {
        delete []realPath;
        return -E_SECUREC_ERROR;
    }

    if (realpath(inOriPath.c_str(), realPath) == nullptr) {
        LOGE("[OS] Realpath error:%d.", errno);
        delete []realPath;
        return -E_SYSTEM_API_FAIL;
    }
    outRealPath = std::string(realPath);
    delete []realPath;
    return E_OK;
}

#ifndef RUNNING_ON_SIMULATED_ENV
int GetCurrentSysTimeInMicrosecond(uint64_t &outTime)
{
    struct timeval rawTime;
    int errCode = gettimeofday(&rawTime, nullptr);
    if (errCode < 0) {
        LOGE("[GetSysTime] Fail:%d.", errCode);
        return -E_SYSTEM_API_FAIL;
    }
    outTime = static_cast<uint64_t>(rawTime.tv_sec) * MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS +
              static_cast<uint64_t>(rawTime.tv_usec);
    return E_OK;
}
#endif // RUNNING_ON_SIMULATED_ENV

namespace {
    const uint64_t MULTIPLES_BETWEEN_MICROSECONDS_AND_NANOSECONDS = 1000;
}

int GetMonotonicRelativeTimeInMicrosecond(uint64_t &outTime)
{
    struct timespec rawTime;
    clockid_t clockId = CLOCK_REALTIME;
#ifdef OS_TYPE_WINDOWS
    clockId = CLOCK_BOOTTIME;
#endif
#ifdef OS_TYPE_MAC
    clockId = CLOCK_UPTIME_RAW;
#endif
    int errCode = clock_gettime(clockId, &rawTime);
    if (errCode < 0) {
        LOGE("[GetMonoTime] Fail.");
        return -E_SYSTEM_API_FAIL;
    }

    outTime = static_cast<uint64_t>(rawTime.tv_sec) * MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS +
              static_cast<uint64_t>(rawTime.tv_nsec) / MULTIPLES_BETWEEN_MICROSECONDS_AND_NANOSECONDS;
    return E_OK;
}

static int GetFilePathAttr(const std::string &topPath, const std::string &relativePath,
                           std::list<FileAttr> &files, bool isNeedAllPath)
{
    DIR *dir = opendir(topPath.c_str());
    if (dir == nullptr) {
        LOGE("Open dir error:%d.", errno);
        return -E_INVALID_PATH;
    }
    struct stat fileStat;
    std::string fileAbsName;
    int errCode = E_OK;
    FileAttr file;
    for (struct dirent *fileDirInfo = readdir(dir); fileDirInfo != nullptr; fileDirInfo = readdir(dir)) {
        switch (fileDirInfo->d_type) {
            case DT_REG:
                file.fileType = FILE;
                break;
            case DT_DIR:
                file.fileType = PATH;
                break;
            default:
                file.fileType = OTHER;
        }
        if (strlen(fileDirInfo->d_name) == 0 || strcmp(fileDirInfo->d_name, ".") == 0 ||
            strcmp(fileDirInfo->d_name, "..") == 0) {
            continue;
        }
        file.fileName = relativePath + fileDirInfo->d_name;
        fileAbsName = topPath + "/" + fileDirInfo->d_name;
        errCode = stat(fileAbsName.c_str(), &fileStat);
        if (errCode != 0) {
            LOGE("[GetFileAttr]Get file stat failed, error = %d.", errno);
            errCode = -E_INVALID_PATH;
            break;
        }
        if (isNeedAllPath) {
            file.fileName = fileAbsName;
        }
        file.fileLen = static_cast<uint64_t>(fileStat.st_size);
        files.push_back(file);
        if (file.fileType == PATH) {
            errCode = GetFilePathAttr(fileAbsName, relativePath + fileDirInfo->d_name + "/", files, isNeedAllPath);
            if (errCode != E_OK) {
                break;
            }
        }
    }

    closedir(dir);
    return errCode;
}

int GetFileAttrFromPath(const std::string &filePath, std::list<FileAttr> &files, bool isNeedAllPath)
{
    return GetFilePathAttr(filePath, std::string(), files, isNeedAllPath);
}

int GetFilePermissions(const std::string &fileName, uint32_t &permissions)
{
    struct stat fileStat;
    int errCode = stat(fileName.c_str(), &fileStat);
    if (errCode != E_OK) {
        permissions = S_IRUSR | S_IWUSR;
        LOGE("Get file stat failed, error = %d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    permissions = fileStat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    return E_OK;
}

int SetFilePermissions(const std::string &fileName, uint32_t permissions)
{
    if (permissions > (S_IRWXU | S_IRWXG | S_IRWXO)) {
        return -E_INVALID_ARGS;
    }
    int errCode = chmod(fileName.c_str(), permissions);
    if (errCode != E_OK) {
        LOGE("Set file permissions failed, error = %d.", errno);
        return -E_SYSTEM_API_FAIL;
    }
    return E_OK;
}

int OpenFile(const std::string &fileName, FileHandle &handle)
{
    handle.handle = open(fileName.c_str(), (O_WRONLY | O_CREAT), (S_IRUSR | S_IWUSR | S_IRGRP));
    if (handle.handle < 0) {
        LOGE("[FileLock] can not open file when lock it:[%d]", errno);
        return -E_SYSTEM_API_FAIL;
    }
    return E_OK;
}

int CloseFile(FileHandle &handle)
{
    if (close(handle.handle) != 0) {
        LOGE("close file failed, errno:%d", errno);
        return -E_SYSTEM_API_FAIL;
    }
    handle.handle = -1;
    return E_OK;
}

int FileLock(const FileHandle &handle, bool isBlock)
{
    if (handle.handle < 0) {
        LOGE("[FileLock] can not open file when lock it:[%d]", errno);
        return -E_SYSTEM_API_FAIL;
    }

    struct flock fileLockInfo;
    (void)memset_s(&fileLockInfo, sizeof(fileLockInfo), 0, sizeof(fileLockInfo));
    fileLockInfo.l_type = F_WRLCK;
    fileLockInfo.l_whence = SEEK_SET;
    fileLockInfo.l_start = 0;
    fileLockInfo.l_len = 0;
    LOGD("Lock file isBlock[%d]", isBlock);
    if (fcntl(handle.handle, isBlock ? F_SETLKW : F_SETLK, &fileLockInfo) == -1 && !isBlock) {
        LOGD("Lock file is Blocked, please retry!");
        return -E_BUSY;
    }
    LOGI("file locked! errno:%d", errno);
    return E_OK;
}

int FileUnlock(FileHandle &handle)
{
    if (handle.handle == -1) {
        LOGI("[FileUnlock] file handle is invalid!");
        return E_OK;
    }

    struct flock fileLockInfo;
    (void)memset_s(&fileLockInfo, sizeof(fileLockInfo), 0, sizeof(fileLockInfo));
    fileLockInfo.l_type = F_UNLCK;
    fileLockInfo.l_whence = SEEK_SET;
    fileLockInfo.l_start = 0;
    fileLockInfo.l_len = 0;
    if (fcntl(handle.handle, F_SETLK, &fileLockInfo) == -1) {
        LOGE("Unlock file failed. errno:%d", errno);
        return -E_SYSTEM_API_FAIL;
    }
    return CloseFile(handle);
}
#endif
} // namespace OS
} // namespace DistributedDB
