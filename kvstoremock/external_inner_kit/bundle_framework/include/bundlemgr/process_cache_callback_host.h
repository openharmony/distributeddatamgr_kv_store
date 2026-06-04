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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_HOST_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_HOST_H

#include <future>
#include <mutex>

#include "iremote_stub.h"
#include "nocopyable.h"

#include "process_cache_callback_interface.h"

namespace OHOS {
namespace AppExecFwk {
class ProcessCacheCallbackHost : public IRemoteStub<IProcessCacheCallback> {
public:
    ProcessCacheCallbackHost();
    virtual ~ProcessCacheCallbackHost() override;

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnGetAllBundleCacheFinished(uint64_t cacheStat) override;
    uint64_t GetCacheStat() override;
    void OnCleanAllBundleCacheFinished(int32_t result) override;
    int32_t GetCleanRet();
    void setAllComplete(bool getAllcomplete)
    {
        std::lock_guard<std::mutex> lock(getAllMutex_);
        getAllcomplete_ = getAllcomplete;
    }
private:
    uint64_t cacheSize_ = 0;
    std::mutex getAllMutex_;
    bool getAllcomplete_ = false;
    std::promise<uint64_t> getAllPromise_;
    std::future<uint64_t> getAllFuture_ = getAllPromise_.get_future();

    std::mutex cleanAllMutex_;
    bool cleanAllcomplete_ = false;
    std::promise<int32_t> cleanAllPromise_;
    std::future<int32_t> cleanAllFuture_ = cleanAllPromise_.get_future();

    DISALLOW_COPY_AND_MOVE(ProcessCacheCallbackHost);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_HOST_H
