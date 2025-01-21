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
static constexpr const char *CORRUPTED_EVENT_NAME = "DATABASE_CORRUPTED";
static constexpr const char *FAULT_EVENT_NAME = "DISTRIBUTED_DATA_KV_FAULT";
static constexpr const char *DISTRIBUTED_DATAMGR = "DISTDATAMGR";
constexpr const char *DB_CORRUPTED_POSTFIX = ".corruptedflg";
static constexpr const char *DEFAULTPATH = "single_ver/main/gen_natural_store.db";
constexpr const char* DATABASE_REBUILD = "RestoreType:Rebuild";
static constexpr const char* FUNCTION = "FunctionName ";
static constexpr const char* DBPATH = "dbPath";
static constexpr const char* FILEINFO = "fileInfo";
std::set<std::string> KVDBFaultHiViewReporter::storeFaults_ = {};

static constexpr Suffix FILE_SUFFIXES[] = {
    {"", "DB"},
    {"-shm", "SHM"},
    {"-wal", "WAL"},
};

static std::map<BusineseType, std::string> busineseTypeMap = {
    {BusineseType::SQLITE, "sqlite"},
    {BusineseType::GAUSSPD, "gausspd"}
};

struct KVDBFaultEvent {
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
    std::string faultType = "common";
    std::string busineseType;
    std::string functionName;

    explicit KVDBFaultEvent(const Options &options) : storeType("KVDB")
    {
        moduleName = options.hapName;
        securityLevel = static_cast<uint32_t>(options.securityLevel);
        pathArea = static_cast<uint32_t>(options.area);
        encryptStatus = static_cast<uint32_t>(options.encrypt);
    }
};

void KVDBFaultHiViewReporter::ReportKVFaultEvent(const ReportInfo &reportInfo, unsigned int mode)
{
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.errorCode = reportInfo.errorCode;
    eventInfo.appendix = reportInfo.appendix;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.systemErrorNo = reportInfo.systemErrorNo;
    eventInfo.errorOccurTime = GetCurrentMicrosecondTimeFormat();
    if (mode & DFXEvent::FAULT) {
        if (!IsReportedFault(eventInfo)) {
            ReportFaultEvent(eventInfo);
        }
    }
    if (mode & DFXEvent::CORRUPTED) {
        ReportCurruptedEvent(eventInfo);
    }
    if (mode & DFXEvent::REBUILD) {
        ReportRebuildEvent(eventInfo);
    }
}

void KVDBFaultHiViewReporter::ReportFaultEvent(KVDBFaultEvent eventInfo)
{
    eventInfo.busineseType = busineseTypeMap[BusineseType::SQLITE];
    eventInfo.appendix = GenerateAppendix(eventInfo);
    char *faultTime = const_cast<char *>(eventInfo.errorOccurTime.c_str());
    char *faultType = const_cast<char *>(eventInfo.faultType.c_str());
    char *bundleName = const_cast<char *>(eventInfo.bundleName.c_str());
    char *moduleName = const_cast<char *>(eventInfo.moduleName.c_str());
    char *storeName = const_cast<char *>(eventInfo.storeName.c_str());
    char *busineseType = const_cast<char *>(eventInfo.busineseType.c_str());
    char *appendix = const_cast<char *>(eventInfo.appendix.c_str());
    HiSysEventParam params[] = {
        { .name = "FAULT_TIME", .t = HISYSEVENT_STRING, .v = { .s = faultTime }, .arraySize = 0 },
        { .name = "FAULT_TYPE", .t = HISYSEVENT_STRING, .v = { .s = faultType }, .arraySize = 0 },
        { .name = "BUNDLE_NAME", .t = HISYSEVENT_STRING, .v = { .s = bundleName }, .arraySize = 0 },
        { .name = "MODULE_NAME", .t = HISYSEVENT_STRING, .v = { .s = moduleName }, .arraySize = 0 },
        { .name = "STORE_NAME", .t = HISYSEVENT_STRING, .v = { .s = storeName }, .arraySize = 0 },
        { .name = "BUSINESE_TYPE", .t = HISYSEVENT_STRING, .v = { .s = busineseType }, .arraySize = 0 },
        { .name = "ERROR_CODE", .t = HISYSEVENT_UINT32, .v = { .ui32 = eventInfo.errorCode }, .arraySize = 0 },
        { .name = "APPENDIX", .t = HISYSEVENT_STRING, .v = { .s = appendix }, .arraySize = 0 },
    };
    OH_HiSysEvent_Write(DISTRIBUTED_DATAMGR, FAULT_EVENT_NAME,
                        HISYSEVENT_FAULT, params, sizeof(params) / sizeof(params[0]));
}

void KVDBFaultHiViewReporter::ReportCurruptedEvent(KVDBFaultEvent eventInfo)
{
    if (IsReportedCorruptedFault(eventInfo.appendix, eventInfo.storeName)) {
        return;
    }
    CreateCorruptedFlag(eventInfo.appendix, eventInfo.storeName);
    eventInfo.appendix = GenerateAppendix(eventInfo);
    ZLOGI("Db corrupted report:storeId:%{public}s", StoreUtil::Anonymous(eventInfo.storeName).c_str());
    ReportCommonFault(eventInfo);
}

void KVDBFaultHiViewReporter::ReportRebuildEvent(KVDBFaultEvent eventInfo)
{
    if (eventInfo.errorCode == 0) {
        ZLOGI("Db rebuild report:storeId:%{public}s", StoreUtil::Anonymous(eventInfo.storeName).c_str());
        DeleteCorruptedFlag(eventInfo.appendix, eventInfo.storeName);
        eventInfo.appendix = GenerateAppendix(eventInfo);
        eventInfo.appendix += "\n" + std::string(DATABASE_REBUILD);
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
        auto file = dbPath + DEFAULTPATH + suffix.suffix_;
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

void KVDBFaultHiViewReporter::ReportCommonFault(const KVDBFaultEvent &eventInfo)
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

    OH_HiSysEvent_Write(DISTRIBUTED_DATAMGR, CORRUPTED_EVENT_NAME,
        HISYSEVENT_FAULT, params, sizeof(params) / sizeof(params[0]));
}

bool KVDBFaultHiViewReporter::IsReportedFault(const KVDBFaultEvent& eventInfo)
{
    std::stringstream oss;
    oss << eventInfo.bundleName << eventInfo.storeName << eventInfo.functionName << eventInfo.errorCode;
    std::string faultFlag = oss.str();
    if (storeFaults_.find(faultFlag) != storeFaults_.end()) {
        return true;
    }
    storeFaults_.insert(faultFlag);
    return false;
}

bool KVDBFaultHiViewReporter::IsReportedCorruptedFault(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty() || storeId.empty()) {
        ZLOGW("The dbPath or storeId is empty, dbPath:%{public}s, storeId:%{public}s", dbPath.c_str(),
            StoreUtil::Anonymous(storeId).c_str());
        return true;
    }

    std::string flagFilename = dbPath + storeId + DB_CORRUPTED_POSTFIX;
    if (access(flagFilename.c_str(), F_OK) == 0) {
        ZLOGW("Corrupted flag already exit");
        return true;
    }
    return false;
}

void KVDBFaultHiViewReporter::CreateCorruptedFlag(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty() || storeId.empty()) {
        ZLOGW("The dbPath or storeId is empty, dbPath:%{public}s, storeId:%{public}s", dbPath.c_str(),
            StoreUtil::Anonymous(storeId).c_str());
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
    if (dbPath.empty() || storeId.empty()) {
        ZLOGW("The dbPath or storeId is empty, dbPath:%{public}s, storeId:%{public}s", dbPath.c_str(),
            StoreUtil::Anonymous(storeId).c_str());
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

std::string KVDBFaultHiViewReporter::GenerateAppendix(const KVDBFaultEvent &eventInfo)
{
    std::string fileStatInfo = GetFileStatInfo(eventInfo.appendix);
    std::string appenDix = "";
    appenDix = FUNCTION + eventInfo.functionName + "\n" +
               DBPATH + eventInfo.appendix + "\n" +
               FILEINFO + fileStatInfo;
    return appenDix;
}
} // namespace OHOS::DistributedKv