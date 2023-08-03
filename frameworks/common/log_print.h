/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATA_LOG_PRINT_H
#define DISTRIBUTEDDATA_LOG_PRINT_H

#define OS_OHOS
#if defined OS_OHOS // log for OHOS

#include "hilog/log.h"
namespace OHOS {
namespace DistributedKv {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001610, "ZDDS" };
}
} // end namespace DistributesdKv

namespace DistributedData {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001611, "ZDD" };
}
} // end namespace DistributedData

namespace DistributedKVStore {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001612, "ZDD" };
}
} // end namespace DistributedKVStore

namespace AppDistributedKv {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001620, "ZDDC" };
}
} // namespace AppDistributedKv

namespace DistributedRdb {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001655, "DRDB" };
}
} // end namespace DistributedRdb

namespace DataShare {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001651, "DataShare" };
}
} // end namespace DataShare
} // end namespace OHOS

#define ZLOGD(fmt, ...)                                                                 \
    do {                                                                                \
        using HiLog = OHOS::HiviewDFX::HiLog;                                           \
        auto lable = LogLabel();                                                        \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_DEBUG)) {                     \
            break;                                                                      \
        }                                                                               \
        HiLog::Debug(lable, LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)

#define ZLOGI(fmt, ...)                                                                \
    do {                                                                               \
        using HiLog = OHOS::HiviewDFX::HiLog;                                          \
        auto lable = LogLabel();                                                       \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_INFO)) {                     \
            break;                                                                     \
        }                                                                              \
        HiLog::Info(lable, LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)

#define ZLOGW(fmt, ...)                                                                \
    do {                                                                               \
        using HiLog = OHOS::HiviewDFX::HiLog;                                          \
        auto lable = LogLabel();                                                       \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_WARN)) {                     \
            break;                                                                     \
        }                                                                              \
        HiLog::Warn(lable, LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)

#define ZLOGE(fmt, ...)                                                                 \
    do {                                                                                \
        using HiLog = OHOS::HiviewDFX::HiLog;                                           \
        auto lable = LogLabel();                                                        \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_ERROR)) {                     \
            break;                                                                      \
        }                                                                               \
        HiLog::Error(lable, LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)

#else
    #error // unknown system
#endif

#endif // DISTRIBUTEDDATA_LOG_PRINT_H
