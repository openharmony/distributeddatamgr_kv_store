/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HKS_LOG_H
#define HKS_LOG_H

#include <time.h>
#include <unistd.h>

#include "hks_type.h"

#ifdef L2_STANDARD
    #include "hks_error_msg.h"
#endif

#ifdef HKS_CONFIG_FILE
    #include HKS_CONFIG_FILE
#else
    #include "hks_config.h"
#endif

// On non-L0 devices, log will be enabled if _HUKS_LOG_ENABLE_ is defined.
// On L0 devices, log will be disabled if HUKS_DISABLE_LOG_AT_FILE_TO_REDUCE_ROM_SIZE is defined,
// even if _HUKS_LOG_ENABLE_ is defined.
#if (!defined(__LITEOS_M__) && defined(_HUKS_LOG_ENABLE_)) || \
    (defined(__LITEOS_M__) && defined(_HUKS_LOG_ENABLE_) && !defined(HUKS_DISABLE_LOG_AT_FILE_TO_REDUCE_ROM_SIZE))
    #ifdef HKS_ENABLE_LOG_PUBLIC
        #define LOG_PUBLIC "{public}"
    #else
        #define LOG_PUBLIC
    #endif

    #undef LOG_TAG
    #define LOG_TAG "HUKS"
    #undef LOG_DOMAIN
    #define LOG_DOMAIN 0xD002F06 /* Security subsystem huks domain id */

    #ifdef HKS_LOG_ENGINE_LOG_CORE
        #include "hilog/log.h"
        #define LOG_ENGINE LOG_CORE
    #else
        #ifdef HKS_LOG_ENGINE_HILOG_MODULE_SCY
            #include "log.h"
            #define LOG_ENGINE HILOG_MODULE_SCY
        #endif
    #endif

    #ifdef L2_STANDARD
        enum HksLogLevel {
            HKS_LOG_LEVEL_I,
            HKS_LOG_LEVEL_E,
            HKS_LOG_LEVEL_E_IMPORTANT,
            HKS_LOG_LEVEL_W,
            HKS_LOG_LEVEL_D,
        };
        #define HKS_LOG_I(fmt, arg...) \
            HILOG_INFO(LOG_ENGINE, "%" LOG_PUBLIC "s: " fmt "\n", __func__, ##arg)
        #define HKS_LOG_W(fmt, arg...) \
            HILOG_WARN(LOG_ENGINE, "%" LOG_PUBLIC "s: " fmt "\n", __func__, ##arg)
        #define HKS_LOG_E(fmt, arg...) \
            do { \
                HILOG_ERROR(LOG_ENGINE, "%" LOG_PUBLIC "s: " fmt "\n", __func__, ##arg); \
                HksLog(HKS_LOG_LEVEL_E, "%" LOG_PUBLIC "s: " fmt, __func__, ##arg); \
            } while (0)
        #define HKS_LOG_E_IMPORTANT(fmt, arg...) \
            do { \
                HILOG_ERROR(LOG_ENGINE, "%" LOG_PUBLIC "s: " fmt "\n", __func__, ##arg); \
                HksLog(HKS_LOG_LEVEL_E_IMPORTANT, "%" LOG_PUBLIC "s: " fmt, __func__, ##arg); \
            } while (0)
        #define HKS_LOG_D(fmt, arg...) \
            HILOG_DEBUG(LOG_ENGINE, "%" LOG_PUBLIC "s: " fmt "\n", __func__, ##arg)
    #else // L2_STANDARD
        #define HKS_LOG_I(fmt, arg...) do {                                                                         \
            struct timespec _ts;                                                                                    \
            clock_gettime(CLOCK_REALTIME, &_ts);                                                                    \
            uint32_t _ms = (uint32_t)((long long)_ts.tv_sec * 1000LL + _ts.tv_nsec / 1000000LL);                    \
            HILOG_INFO(LOG_ENGINE, "HUKS_I_%" LOG_PUBLIC "u:" fmt "\n", _ms, ##arg);     \
        } while (0)
        #define HKS_LOG_W(fmt, arg...) do {                                                                         \
            struct timespec _ts;                                                                                    \
            clock_gettime(CLOCK_REALTIME, &_ts);                                                                    \
            uint32_t _ms = (uint32_t)((long long)_ts.tv_sec * 1000LL + _ts.tv_nsec / 1000000LL);                    \
            HILOG_WARN(LOG_ENGINE, "HUKS_W_%" LOG_PUBLIC "u:" fmt "\n", _ms, ##arg);     \
        } while (0)
        #define HKS_LOG_E(fmt, arg...) do {                                                                         \
            struct timespec _ts;                                                                                    \
            clock_gettime(CLOCK_REALTIME, &_ts);                                                                    \
            uint32_t _ms = (uint32_t)((long long)_ts.tv_sec * 1000LL + _ts.tv_nsec / 1000000LL);                    \
            HILOG_ERROR(LOG_ENGINE, "HUKS_E_%" LOG_PUBLIC "u:" fmt "\n", _ms, ##arg);    \
        } while (0)
        #define HKS_LOG_E_IMPORTANT(fmt, arg...) do {                                                               \
            struct timespec _ts;                                                                                    \
            clock_gettime(CLOCK_REALTIME, &_ts);                                                                    \
            uint32_t _ms = (uint32_t)((long long)_ts.tv_sec * 1000LL + _ts.tv_nsec / 1000000LL);                    \
            HILOG_ERROR(LOG_ENGINE, "HUKS_E_%" LOG_PUBLIC "u:" fmt "\n", _ms, ##arg);    \
        } while (0)
        #define HKS_LOG_D(fmt, arg...) do {                                                                         \
            struct timespec _ts;                                                                                    \
            clock_gettime(CLOCK_REALTIME, &_ts);                                                                    \
            uint32_t _ms = (uint32_t)((long long)_ts.tv_sec * 1000LL + _ts.tv_nsec / 1000000LL);                    \
            HILOG_DEBUG(LOG_ENGINE, "HUKS_D_%" LOG_PUBLIC "u:" fmt "\n", _ms, ##arg);    \
        } while (0)
    #endif // L2_STANDARD

#else // _HUKS_LOG_ENABLE_
    #define HKS_LOG_I(...)
    #define HKS_LOG_W(...)
    #define HKS_LOG_E(...)
    #define HKS_LOG_E_IMPORTANT(...)
    #define HKS_LOG_D(...)
#endif //_HUKS_LOG_ENABLE_

#endif /* HKS_LOG_H */
