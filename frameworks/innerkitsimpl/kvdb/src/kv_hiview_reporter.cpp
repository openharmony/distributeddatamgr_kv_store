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

#define LOG_TAG "KVDBFaultHiViewReporter"

#include "kv_hiview_reporter.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "hisysevent_c.h"
#include "log_print.h"
#include "types.h"
#include "store_util.h"

namespace OHOS::DistributedKv {
static constexpr int MAX_TIME_BUF_LEN = 32;
static constexpr int MILLISECONDS_LEN = 3;
static constexpr int NANO_TO_MILLI = 1000000;
static constexpr int MILLI_PRE_SEC = 1000;
static constexpr const char *EVENT_NAME = "DATABASE_CORRUPTED";
static constexpr const char *DISTRIBUTED_DATAMGR = "DISTDATAMGR";
constexpr const char *DB_CORRUPTED_POSTFIX = ".corruptedflg";
struct KVDBCorruptedEvent {
    std::string bundleName;
    std::string moduleName;
    std::string storeType;
    std::string storeName;
    uint32_t securityLevel;
    uint32_t pathArea;
    uint32_t encryptStatus;
    uint32_t integrityCheck = 0;
    uint32_t errorCode = 0;
    int32_t systemErrorNo = 0;
    std::string appendix;
    std::string errorOccurTime;

    explicit KVDBCorruptedEvent(const Options &options) : storeType("KVDB")
    {
        moduleName = options.hapName;
        securityLevel = static_cast<uint32_t>(options.securityLevel);
        pathArea = static_cast<uint32_t>(options.area);
        encryptStatus = static_cast<uint32_t>(options.encrypt);
    }
};

void KVDBFaultHiViewReporter::ReportKVDBCorruptedFault(
    const Options &options, uint32_t errorCode, int32_t systemErrorNo,
    const KvStoreTuple &storeTuple, const std::string &appendix)
{
    KVDBCorruptedEvent eventInfo(options);
    eventInfo.errorCode = errorCode;
    eventInfo.systemErrorNo = systemErrorNo;
    eventInfo.appendix = appendix;
    eventInfo.storeName = storeTuple.storeId;
    eventInfo.bundleName = storeTuple.appId;
    eventInfo.errorOccurTime = GetCurrentMicrosecondTimeFormat();
    if (IsReportCorruptedFault(eventInfo.appendix, storeTuple.storeId)) {
        CreateCorruptedFlag(eventInfo.appendix, storeTuple.storeId);
        auto corruptedTime = GetFileStatInfo(eventInfo.appendix);
        eventInfo.appendix = corruptedTime;
        ZLOGI("Db corrupted report:storeId:%{public}s", StoreUtil::Anonymous(storeTuple.storeId).c_str());
        ReportCommonFault(eventInfo);
    }
}

void KVDBFaultHiViewReporter::ReportKVDBRebuild(
    const Options &options, uint32_t errorCode, int32_t systemErrorNo,
    const KvStoreTuple &storeTuple, const std::string &appendix)
{
    KVDBCorruptedEvent eventInfo(options);
    eventInfo.errorCode = errorCode;
    eventInfo.systemErrorNo = systemErrorNo;
    eventInfo.appendix = appendix;
    eventInfo.storeName = storeTuple.storeId;
    eventInfo.bundleName = storeTuple.appId;
    eventInfo.errorOccurTime = GetCurrentMicrosecondTimeFormat();
    if (errorCode == 0) {
        ZLOGI("Db rebuild report:storeId:%{public}s", StoreUtil::Anonymous(storeTuple.storeId).c_str());
        DeleteCorruptedFlag(eventInfo.appendix, storeTuple.storeId);
        auto corruptedTime = GetFileStatInfo(eventInfo.appendix);
        corruptedTime += "\n" + std::string(DATABASE_REBUILD);
        eventInfo.appendix = corruptedTime;
        ReportCommonFault(eventInfo);
    }
}

std::string KVDBFaultHiViewReporter::GetCurrentMicrosecondTimeFormat()
{
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::microseconds>(epoch);
    auto timestamp = value.count();

    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm *tm = std::localtime(&tt);
    if (tm == nullptr) {
        ZLOGE("Failed localtime is nullptr");
        return "";
    }

    const int offset = 1000;
    const int width = 3;
    std::stringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(width)
        << ((timestamp / offset) % offset) << "." << std::setfill('0') << std::setw(width) << (timestamp % offset);
    return oss.str();
}

std::string KVDBFaultHiViewReporter::GetFileStatInfo(const std::string &dbPath)
{
    std::string fileTimeInfo;
    const uint32_t permission = 0777;
    for (auto &suffix : FILE_SUFFIXES) {
        if (suffix.name_ == nullptr) {
            continue;
        }
        auto file = dbPath + defaultPath + suffix.suffix_;
        struct stat fileStat;
        if (stat(file.c_str(), &fileStat) != 0) {
            continue;
        }
        std::stringstream oss;
        oss << " dev:0x" << std::hex << fileStat.st_dev << " ino:0x" << std::hex << fileStat.st_ino;
        oss << " mode:0" << std::oct << (fileStat.st_mode & permission) << " size:" << std::dec << fileStat.st_size
            << " atime:" << GetTimeWithMilliseconds(fileStat.st_atime, fileStat.st_atim.tv_nsec)
            << " mtime:" << GetTimeWithMilliseconds(fileStat.st_mtime, fileStat.st_mtim.tv_nsec)
            << " ctime:" << GetTimeWithMilliseconds(fileStat.st_ctime, fileStat.st_ctim.tv_nsec);
        fileTimeInfo += "\n" + std::string(suffix.name_) + " :" + oss.str();
    }
    return fileTimeInfo;
}

std::string KVDBFaultHiViewReporter::GetTimeWithMilliseconds(time_t sec, int64_t nsec)
{
    std::stringstream oss;
    char buffer[MAX_TIME_BUF_LEN] = { 0 };
    std::tm local_time;
    localtime_r(&sec, &local_time);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);
    oss << buffer << "." << std::setfill('0') << std::setw(MILLISECONDS_LEN) << (nsec / NANO_TO_MILLI) % MILLI_PRE_SEC;
    return oss.str();
}

void KVDBFaultHiViewReporter::ReportCommonFault(const KVDBCorruptedEvent &eventInfo)
{
    char *bundleName = const_cast<char *>(eventInfo.bundleName.c_str());
    char *moduleName = const_cast<char *>(eventInfo.moduleName.c_str());
    char *storeType = const_cast<char *>(eventInfo.storeType.c_str());
    char *storeName = const_cast<char *>(eventInfo.storeName.c_str());
    uint32_t checkType = eventInfo.integrityCheck;
    char *appendix = const_cast<char *>(eventInfo.appendix.c_str());
    char *errorOccurTime = const_cast<char *>(eventInfo.errorOccurTime.c_str());
    HiSysEventParam params[] = {
        { .name = "BUNDLE_NAME", .t = HISYSEVENT_STRING, .v = { .s = bundleName }, .arraySize = 0 },
        { .name = "MODULE_NAME", .t = HISYSEVENT_STRING, .v = { .s = moduleName }, .arraySize = 0 },
        { .name = "STORE_TYPE", .t = HISYSEVENT_STRING, .v = { .s = storeType }, .arraySize = 0 },
        { .name = "STORE_NAME", .t = HISYSEVENT_STRING, .v = { .s = storeName }, .arraySize = 0 },
        { .name = "SECURITY_LEVEL", .t = HISYSEVENT_UINT32, .v = { .ui32 = eventInfo.securityLevel }, .arraySize = 0 },
        { .name = "PATH_AREA", .t = HISYSEVENT_UINT32, .v = { .ui32 = eventInfo.pathArea }, .arraySize = 0 },
        { .name = "ENCRYPT_STATUS", .t = HISYSEVENT_UINT32, .v = { .ui32 = eventInfo.encryptStatus }, .arraySize = 0 },
        { .name = "INTEGRITY_CHECK", .t = HISYSEVENT_UINT32, .v = { .ui32 = checkType }, .arraySize = 0 },
        { .name = "ERROR_CODE", .t = HISYSEVENT_UINT32, .v = { .ui32 = eventInfo.errorCode }, .arraySize = 0 },
        { .name = "ERRNO", .t = HISYSEVENT_INT32, .v = { .i32 = eventInfo.systemErrorNo }, .arraySize = 0 },
        { .name = "APPENDIX", .t = HISYSEVENT_STRING, .v = { .s = appendix }, .arraySize = 0 },
        { .name = "ERROR_TIME", .t = HISYSEVENT_STRING, .v = { .s = errorOccurTime }, .arraySize = 0 },
    };

    OH_HiSysEvent_Write(DISTRIBUTED_DATAMGR, EVENT_NAME, HISYSEVENT_FAULT, params, sizeof(params) / sizeof(params[0]));
}

bool KVDBFaultHiViewReporter::IsReportCorruptedFault(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty()) {
        ZLOGW("This dbPath path is empty");
        return false;
    }

    std::string flagFilename = dbPath + storeId + DB_CORRUPTED_POSTFIX;
    if (access(flagFilename.c_str(), F_OK) == 0) {
        ZLOGW("Corrupted flag already exit");
        return false;
    }
    return true;
}

void KVDBFaultHiViewReporter::CreateCorruptedFlag(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty()) {
        ZLOGW("This dbPath path is empty");
        return;
    }
    std::string flagFilename = dbPath + storeId + DB_CORRUPTED_POSTFIX;
    int fd = creat(flagFilename.c_str(), S_IRWXU | S_IRWXG);
    if (fd == -1) {
        ZLOGW("Creat corrupted flg fail, flgname=%{public}s, errno=%{public}d",
            StoreUtil::Anonymous(flagFilename).c_str(), errno);
        return;

    }
    close(fd);
}

void KVDBFaultHiViewReporter::DeleteCorruptedFlag(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty()) {
        ZLOGW("This dbPath path is empty");
        return;
    }
    std::string flagFilename = dbPath + storeId + DB_CORRUPTED_POSTFIX;
    int result = remove(flagFilename.c_str());
    if (result != 0) {
        ZLOGW("Remove corrupted flg fail, flgname=%{public}s, errno=%{public}d",
            StoreUtil::Anonymous(flagFilename).c_str(), errno);
    }
}

std::string KVDBFaultHiViewReporter::GetDBPath(const std::string &path, const std::string &storeId)
{
    std::string reporterDir = "";
    DistributedDB::KvStoreDelegateManager::GetDatabaseDir(storeId, reporterDir);
    reporterDir = path + "/kvdb/" + reporterDir + "/";
    return reporterDir;
}
} // namespace OHOS::DistributedKv