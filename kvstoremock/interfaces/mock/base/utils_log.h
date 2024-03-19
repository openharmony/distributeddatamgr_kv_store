/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef UTILS_BASE_LOG_H
#define UTILS_BASE_LOG_H

#ifdef CONFIG_HILOG
#include "hilog/log.h"
static constexpr OHOS::HiviewDFX::HiLogLabel label = { LOG_CORE, 0xD003D00, "utils_base" };
#define UTILS_LOGF(...) printf("fatal: " fmt "\n",  __VA_ARGS__)
#define UTILS_LOGE(...) printf("error: " fmt "\n",  __VA_ARGS__)
#define UTILS_LOGW(...) printf("warn: " fmt "\n", __VA_ARGS__)
#define UTILS_LOGI(...) printf("info: " fmt "\n", __VA_ARGS__)
#define UTILS_LOGD(...) printf("debug: " fmt "\n",  __VA_ARGS__)
#else
#define UTILS_LOGF(...)
#define UTILS_LOGE(...)
#define UTILS_LOGW(...)
#define UTILS_LOGI(...)
#define UTILS_LOGD(...)
#endif  // CONFIG_HILOG

#if (defined CONFIG_HILOG) && (defined CONFIG_PARCEL_DEBUG)
static constexpr OHOS::HiviewDFX::HiLogLabel parcel = { LOG_CORE, 0xD003D01, "parcel" };
#define PARCEL_LOGF(...) printf("fatal: " fmt "\n", __VA_ARGS__)
#define PARCEL_LOGE(...) printf("error: " fmt "\n", __VA_ARGS__)
#define PARCEL_LOGW(...) printf("warn: " fmt "\n",  __VA_ARGS__)
#define PARCEL_LOGI(...) printf("info: " fmt "\n", __VA_ARGS__)
#define PARCEL_LOGD(...) printf("debug: " fmt "\n",  __VA_ARGS__)
#else
#define PARCEL_LOGF(...)
#define PARCEL_LOGE(...)
#define PARCEL_LOGW(...)
#define PARCEL_LOGI(...)
#define PARCEL_LOGD(...)
#endif  // (defined CONFIG_HILOG) && (defined CONFIG_PARCEL_DEBUG)

#endif  // UTILS_BASE_LOG_H
