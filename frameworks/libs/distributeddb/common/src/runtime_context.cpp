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

#include "log_print.h"
#include "runtime_context_impl.h"
#include "version.h"

namespace DistributedDB {
std::atomic<RuntimeContext *> RuntimeContext::instance_{nullptr};
std::mutex RuntimeContext::instanceLock_;

RuntimeContext *RuntimeContext::GetInstance()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    if (instance_.load(std::memory_order_acquire) == nullptr) {
        if (instance_ == nullptr) {
            instance_.store(new RuntimeContextImpl(), std::memory_order_release);
            LOGI("DistributedDB Version : %s", SOFTWARE_VERSION_STRING);
        }
    }
    return instance_.load(std::memory_order_acquire);
}

void RuntimeContext::DeleteInstance()
{
    RuntimeContext *inst = nullptr;
    {
        std::lock_guard<std::mutex> lock(instanceLock_);
        inst = instance_.load(std::memory_order_acquire);
        instance_.store(nullptr, std::memory_order_release);
    }
    if (inst != nullptr) {
        delete inst;
    }
}
} // namespace DistributedDB
