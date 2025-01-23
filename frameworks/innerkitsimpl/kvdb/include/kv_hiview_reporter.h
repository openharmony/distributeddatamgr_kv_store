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

#ifndef KV_HIVIEW_REPORTER_H
#define KV_HIVIEW_REPORTER_H

#include <map>
#include <set>
#include <string>
#include "types.h"

namespace OHOS::DistributedKv {
struct Suffix {
    const char *suffix_ = nullptr;
    const char *name_ = nullptr;
};

enum BusineseType {
    SQLITE,
    GAUSSPD,
};

enum DFXEvent {
    FAULT = 0x1,       // 001
    CORRUPTED = 0x2,   // 010
    REBUILD = 0x4,     // 100
};

struct ReportInfo {
    Options options;
    uint32_t errorCode;
    int32_t systemErrorNo;
    std::string appId;
    std::string storeId;
    std::string functionName;
};

struct KVDBFaultEvent;
class KVDBFaultHiViewReporter {
public:
    static void ReportKVFaultEvent(const ReportInfo &reportInfo);

    static void ReportKVRebuildEvent(const ReportInfo &reportInfo);
private:
    static void ReportFaultEvent(KVDBFaultEvent eventInfo);

    static void ReportCurruptedEvent(KVDBFaultEvent eventInfo);

    static void ReportCommonFault(const KVDBFaultEvent &eventInfo);

    static std::string GetCurrentMicrosecondTimeFormat();

    static bool IsReportedCorruptedFault(const std::string &dbPath, const std::string &storeId);

    static void CreateCorruptedFlag(const std::string &dbPath, const std::string &storeId);

    static std::string GetDBPath(const std::string& path, const std::string& storeId);

    static void DeleteCorruptedFlag(const std::string& dbPath, const std::string& storeId);

    static std::string GetFileStatInfo(const std::string &dbPath);

    static std::string GetTimeWithMilliseconds(time_t sec, int64_t nsec);

    static std::string GenerateAppendix(const KVDBFaultEvent &eventInfo);

    static bool IsReportedFault(const KVDBFaultEvent& eventInfo);
    
    static std::set<std::string> storeFaults_;
};
} // namespace OHOS::DistributedKv
#endif //KV_HIVIEW_REPORTER_H