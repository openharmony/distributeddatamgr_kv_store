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

#include "dfx_helper.h"

#include "log_print.h"

namespace DistributedDB {
CostTimeHelper::CostTimeHelper(const std::string &tag)
    : tag_(tag)
{
    startTime_ = std::chrono::steady_clock::now();
}

CostTimeHelper::~CostTimeHelper()
{
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
        startTime_);
    if (duration > DFXHelper::DFX_TIME_THRESHOLD) {
        LOGW("%s cost:%" PRIi64 "ms", tag_.c_str(), duration);
    }
}

std::shared_ptr<CostTimeHelper> DFXHelper::GetCostTimeHelper(const std::string &tag)
{
    return std::make_shared<CostTimeHelper>(tag);
}
} // namespace DistributedDB
