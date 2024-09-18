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
#include "hisysevent_c.h"
#include "log_print.h"
#include "types.h"
#include "store_util.h"

namespace OHOS::DistributedKv {

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
    const KvStoreTuple &storeTuple, const std::string &path)
{
    KVDBCorruptedEvent eventInfo(options);
    eventInfo.errorCode = errorCode;
    eventInfo.systemErrorNo = systemErrorNo;
    eventInfo.appendix = path;
    eventInfo.storeName = storeTuple.storeId;
    eventInfo.bundleName = storeTuple.appId;
    eventInfo.errorOccurTime = GetCurrentMicrosecondTimeFormat();
    if (IsReportCorruptedFault(eventInfo.appendix, storeTuple.storeId)) {
        ReportCommonFault(eventInfo);
        CreateCorruptedFlag(eventInfo.appendix, storeTuple.storeId);
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

bool KVDBFaultHiViewReporter::IsReportCorruptedFault(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty()) {
        ZLOGW("dbPath path is empty");
        return false;
    }

    std::string flagFilename = dbPath + storeId + DB_CORRUPTED_POSTFIX;
    if (access(flagFilename.c_str(), F_OK) == 0) {
        ZLOGW("corrupted flag already exit");
        return false;
    }
    return true;
}

void KVDBFaultHiViewReporter::CreateCorruptedFlag(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty()) {
        ZLOGW("dbPath path is empty");
        return;
    }
    std::string flagFilename = dbPath + storeId + DB_CORRUPTED_POSTFIX;
    int fd = creat(flagFilename.c_str(), S_IRWXU | S_IRWXG);
    if (fd == -1) {
        ZLOGW("creat corrupted flg fail, flgname=%{public}s, errno=%{public}d",
            StoreUtil::Anonymous(flagFilename).c_str(), errno);
        return;

    }
    close(fd);
}

void KVDBFaultHiViewReporter::DeleteCorruptedFlag(const std::string &dbPath, const std::string &storeId)
{
    if (dbPath.empty()) {
        ZLOGW("dbPath path is empty");
        return;
    }
    std::string flagFilename = dbPath + storeId + DB_CORRUPTED_POSTFIX;
    int result = remove(flagFilename.c_str());
    if (result != 0) {
        ZLOGW("remove corrupted flg fail, flgname=%{public}s, errno=%{public}d",
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
} // namespace OHOS::DistributedKv