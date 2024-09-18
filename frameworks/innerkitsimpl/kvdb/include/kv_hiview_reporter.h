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

#include <string>
#include "types.h"

namespace OHOS::DistributedKv {
constexpr const char* OPEN_STORE = "OPEN_STORE";
constexpr const char* PUT = "PUT";
constexpr const char* PUTBATCH = "PUTBATCH";
constexpr const char* DEL = "DELETE";
constexpr const char* DELETEBATCH = "DELETEBATCH";
constexpr const char* STARTTRANSACTION = "STARTTRANSACTION";
constexpr const char* COMMIT = "COMMIT";
constexpr const char* ROLLBACK = "ROLLBACK";
constexpr const char* GET = "GET";
constexpr const char* GETDEVICEENTRIES = "GETDEVICEENTRIES";
constexpr const char* GETCOUNT = "GETCOUNT";
constexpr const char* GETSECURITYLEVEL = "GETSECURITYLEVEL";
constexpr const char* REMOVEDEVICEDATA = "REMOVEDEVICEDATA";
constexpr const char* GETENTRIES = "GETENTRIES";

struct KVDBCorruptedEvent;
class KVDBFaultHiViewReporter {
public:
    static void ReportKVDBCorruptedFault(
        const Options &options, uint32_t errorCode, int32_t systemErrorNo,
        const KvStoreTuple &storeTuple, const std::string &path);
    
    static void DeleteCorruptedFlag(const std::string &dbPath, const std::string &storeId);

    static std::string GetDBPath(const std::string &path, const std::string &storeId);

private:
    static void ReportCommonFault(const KVDBCorruptedEvent &eventInfo);

    static std::string GetCurrentMicrosecondTimeFormat();

    static bool IsReportCorruptedFault(const std::string &dbPath, const std::string &storeId);

    static void CreateCorruptedFlag(const std::string &dbPath, const std::string &storeId);
};
} // namespace OHOS::DistributedKv
#endif //KV_HIVIEW_REPORTER_H