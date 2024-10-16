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
constexpr const char* DATABASE_REBUILD = "RestoreType:Rebuild";
struct Suffix {
    const char *suffix_ = nullptr;
    const char *name_ = nullptr;
};
static constexpr Suffix FILE_SUFFIXES[] = {
    {"", "DB"},
    {"-shm", "SHM"},
    {"-wal", "WAL"},
};
static constexpr const char *defaultPath = "single_ver/main/gen_natural_store.db";
struct KVDBCorruptedEvent;
class KVDBFaultHiViewReporter {
public:
    static void ReportKVDBCorruptedFault(
        const Options &options, uint32_t errorCode, int32_t systemErrorNo,
        const KvStoreTuple &storeTuple, const std::string &appendix);

    static void ReportKVDBRebuild(
        const Options &options, uint32_t errorCode, int32_t systemErrorNo,
        const KvStoreTuple &storeTuple, const std::string &appendix);

    static std::string GetDBPath(const std::string &path, const std::string &storeId);

    static void DeleteCorruptedFlag(const std::string &dbPath, const std::string &storeId);
private:
    static void ReportCommonFault(const KVDBCorruptedEvent &eventInfo);

    static std::string GetCurrentMicrosecondTimeFormat();

    static bool IsReportCorruptedFault(const std::string &dbPath, const std::string &storeId);

    static void CreateCorruptedFlag(const std::string &dbPath, const std::string &storeId);

    static std::string GetFileStatInfo(const std::string &dbPath);

    static std::string GetTimeWithMilliseconds(time_t sec, int64_t nsec);
};
} // namespace OHOS::DistributedKv
#endif //KV_HIVIEW_REPORTER_H