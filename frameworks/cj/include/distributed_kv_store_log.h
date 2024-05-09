/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DISTRIBUTED_KV_STORE_LOG_H
#define DISTRIBUTED_KV_STORE_LOG_H

#include "hilog/log.h"

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0xD002B25
#define LOG_TAG "DistributedKVStoreCffi"

#define LOGI(...)                                           \
if (HiLogIsLoggable(LOG_DOMAIN, LOG_TAG, LOG_INFO)) {       \
    HILOG_INFO(LOG_CORE, ##__VA_ARGS__);                    \
}

#define LOGE(...)                                           \
if (HiLogIsLoggable(LOG_DOMAIN, LOG_TAG, LOG_ERROR)) {      \
    HILOG_ERROR(LOG_CORE, __VA_ARGS__);                     \
}

#endif
