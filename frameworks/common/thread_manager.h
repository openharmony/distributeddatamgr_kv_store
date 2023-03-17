/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_THREAD_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_THREAD_MANAGER_H
#include "kv_thread_pool.h"

namespace OHOS {
class ThreadManager {
public:
    ThreadManager GetInstance()
    {
        static ThreadManager instance;
        return instance;
    }
    std::shared_ptr<KVThreadPool> GetThreadPool()
    {
        return threadPool_;
    }

private:
    static constexpr size_t maxThreads = 12;
    static constexpr size_t minThreads = 5;
	
    ThreadManager()
    {
        threadPool_ = std::make_shared<KVThreadPool>(12, 5);
    };

    ~ThreadManager();
    std::shared_ptr<KVThreadPool> threadPool_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_THREAD_MANAGER_H
