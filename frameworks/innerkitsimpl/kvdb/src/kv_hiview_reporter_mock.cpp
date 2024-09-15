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

#define LOG_TAG "KVDBFaultHiViewReporterMock"

#include "kv_hiview_reporter.h"

namespace OHOS::DistributedKv {
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
    ReportCommonFault(eventInfo);
}

std::string KVDBFaultHiViewReporter::GetCurrentMicrosecondTimeFormat()
{
    return "";
}

void KVDBFaultHiViewReporter::ReportCommonFault(__attribute__((unused))
    const KVDBCorruptedEvent &eventInfo)
{
    return;
}

bool KVDBFaultHiViewReporter::IsReportCorruptedFault(const std::string &dbPath, const std::string &storeId)
{
    return;
}

void KVDBFaultHiViewReporter::CreateCorruptedFlag(const std::string &dbPath, const std::string &storeId)
{
    return;
}

void KVDBFaultHiViewReporter::DeleteCorruptedFlag(const std::string &dbPath, const std::string &storeId)
{
    return;
}

std::string KVDBFaultHiViewReporter::GetDBPath(const std::string &path, const std::string &storeId)
{
    return "";
}
} // namespace OHOS::NativeRdb