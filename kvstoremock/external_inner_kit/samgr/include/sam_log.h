/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SAMGR_PROXY_INCLUDE_SAM_LOG_H
#define SAMGR_PROXY_INCLUDE_SAM_LOG_H

#include "hilog/log.h"

namespace OHOS {

constexpr OHOS::HiviewDFX::HiLogLabel SYSTEM_ABLILITY_LABEL = {
    LOG_KMSG,
    0xD001800,
    "SAMGR"
};

#undef LOG_DOMAIN
#ifdef SAMGR_PROXY
#define LOG_DOMAIN 0xD001810
#else
#define LOG_DOMAIN 0xD001800
#endif

#undef LOG_TAG
#ifdef SAMGR_PROXY
#define LOG_TAG "SA_CLIENT"
#else
#define LOG_TAG "SAMGR"
#endif

#ifdef HILOGF
#undef HILOGF
#endif

#ifdef HILOGE
#undef HILOGE
#endif

#ifdef HILOGW
#undef HILOGW
#endif

#ifdef HILOGI
#undef HILOGI
#endif

#ifdef HILOGD
#undef HILOGD
#endif

#ifdef KHILOGI
#undef KHILOGI
#endif

#ifdef KHILOGE
#undef KHILOGE
#endif

#ifdef KHILOGW
#undef KHILOGW
#endif

#ifdef KHILOGD
#undef KHILOGD
#endif

#ifdef KHILOGF
#undef KHILOGF
#endif

#define KHILOGI(...) \
    do { \
        (void)OHOS::HiviewDFX::HiLog::Info(SYSTEM_ABLILITY_LABEL, __VA_ARGS__); \
        HILOG_INFO(LOG_CORE, __VA_ARGS__); \
    } while (0)

#define KHILOGE(...) \
    do { \
        (void)OHOS::HiviewDFX::HiLog::Error(SYSTEM_ABLILITY_LABEL, __VA_ARGS__); \
        HILOG_ERROR(LOG_CORE, __VA_ARGS__); \
    } while (0)

#define KHILOGW(...) \
    do { \
        (void)OHOS::HiviewDFX::HiLog::Warn(SYSTEM_ABLILITY_LABEL, __VA_ARGS__); \
        HILOG_WARN(LOG_CORE, __VA_ARGS__); \
    } while (0)

#define KHILOGD(...) \
    do { \
        (void)OHOS::HiviewDFX::HiLog::Debug(SYSTEM_ABLILITY_LABEL, __VA_ARGS__); \
        HILOG_DEBUG(LOG_CORE, __VA_ARGS__); \
    } while (0)

#define KHILOGF(...) \
    do { \
        (void)OHOS::HiviewDFX::HiLog::Fatal(SYSTEM_ABLILITY_LABEL, __VA_ARGS__); \
        HILOG_FATAL(LOG_CORE, __VA_ARGS__); \
    } while (0)

#define HILOGF(...) HILOG_FATAL(LOG_CORE, __VA_ARGS__)
#define HILOGE(...) HILOG_ERROR(LOG_CORE, __VA_ARGS__)
#define HILOGW(...) HILOG_WARN(LOG_CORE, __VA_ARGS__)
#define HILOGI(...) HILOG_INFO(LOG_CORE, __VA_ARGS__)
#define HILOGD(...) HILOG_DEBUG(LOG_CORE, __VA_ARGS__)
} // namespace OHOS

#endif // #ifndef SAMGR_PROXY_INCLUDE_SAM_LOG_H
