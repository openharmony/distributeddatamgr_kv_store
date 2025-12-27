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

void KVDBFaultHiViewReporter::ReportKVFaultEvent(const ReportInfo &reportInfo)
{
    return;
}

void KVDBFaultHiViewReporter::ReportKVRebuildEvent(const ReportInfo &reportInfo)
{
    return;
}

std::string KVDBFaultHiViewReporter::GetCurrentMicrosecondTimeFormat()
{
    return "";
}

void KVDBFaultHiViewReporter::ReportCommonFault(__attribute__((unused))
    const KVDBFaultEvent &eventInfo)
{
    return;
}

bool KVDBFaultHiViewReporter::IsReportedCorruptedFault(const std::string &appId, const std::string &storeId,
    const std::string &dbPath)
{
    return false;
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
} // namespace OHOS::DistributedKv