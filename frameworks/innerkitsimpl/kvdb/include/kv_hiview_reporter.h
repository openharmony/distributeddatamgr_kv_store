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
struct KVDBCorruptedEvent;
enum DBErrorCode {
    CORRUPTED = 0,
};

class KVDBFaultHiViewReporter {
public:
    static void ReportKVDBCorruptedFault(
        const Options &options, uint32_t errorCode, uint32_t systemErrorNo,
        const KvStoreTuple &storeTuple, std::string &appendix);

private:
    static void ReportCommonFault(const KVDBCorruptedEvent &eventInfo);

    static std::string getCurrentMicrosecondTimeFormat();
};
} // namespace OHOS::DistributedKv
#endif //KV_HIVIEW_REPORTER_H