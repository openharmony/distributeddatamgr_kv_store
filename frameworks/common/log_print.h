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

namespace CloudData {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001613, "CLOUD" };
}
} // end namespace CloudData

namespace UDMF {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001614, "UDMF" };
}
} // end namespace UDMF

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

namespace DistributedObject {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001654, "DOBJECT" };
}
} // end namespace DistributedObject

namespace DataSearchable {
static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
{
    return { LOG_CORE, 0xD001656, "DSRCH" };
}
} // end namespace DataSearchable

namespace ConnectInner {
    static inline OHOS::HiviewDFX::HiLogLabel LogLabel()
    {
        return { LOG_CORE, 0xD001610, "Connect" };
    }
} // end namespace ConnectInner
} // end namespace OHOS

#define ZLOGD(fmt, ...)                                                   \
    do {                                                                  \
        auto lable = LogLabel();                                          \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_DEBUG)) {       \
            break;                                                        \
        }                                                                 \
        ((void)HILOG_IMPL(lable.type, LOG_DEBUG, lable.domain, lable.tag, \
            LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
    } while (0)

#define ZLOGI(fmt, ...)                                                   \
    do {                                                                  \
        auto lable = LogLabel();                                          \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_INFO)) {        \
            break;                                                        \
        }                                                                 \
        ((void)HILOG_IMPL(lable.type, LOG_INFO, lable.domain, lable.tag,  \
            LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
    } while (0)

#define ZLOGW(fmt, ...)                                                   \
    do {                                                                  \
        auto lable = LogLabel();                                          \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_WARN)) {        \
            break;                                                        \
        }                                                                 \
        ((void)HILOG_IMPL(lable.type, LOG_WARN, lable.domain, lable.tag,  \
            LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
    } while (0)

#define ZLOGE(fmt, ...)                                                   \
    do {                                                                  \
        auto lable = LogLabel();                                          \
        if (!HiLogIsLoggable(lable.domain, lable.tag, LOG_ERROR)) {       \
            break;                                                        \
        }                                                                 \
        ((void)HILOG_IMPL(lable.type, LOG_ERROR, lable.domain, lable.tag, \
            LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
    } while (0)                                   \

#else
    #error // unknown system
#endif

#endif // DISTRIBUTEDDATA_LOG_PRINT_H