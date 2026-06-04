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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_PROXY_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_PROXY_H

#include "iremote_proxy.h"

#include "process_cache_callback_interface.h"

namespace OHOS {
namespace AppExecFwk {
class ProcessCacheCallbackProxy : public IRemoteProxy<IProcessCacheCallback> {
public:
    explicit ProcessCacheCallbackProxy(const sptr<IRemoteObject> &object);
    virtual ~ProcessCacheCallbackProxy() override;
    /**
     * @brief Called when process cache files progress finished through the proxy object.
     * @param cacheStat Indicates the result of the get cache files progress.
     */
    virtual void OnGetAllBundleCacheFinished(uint64_t cacheStat) override;

    /**
     * @brief Called when process cache files progress finished through the proxy object.
     * @param result Indicates the result of the delete cache files progress.
     */
    virtual void OnCleanAllBundleCacheFinished(int32_t resulted) override;
    
    /**
     * @brief get all bundle cache stat.
     */
    virtual uint64_t GetCacheStat() override;

private:
    static inline BrokerDelegator<ProcessCacheCallbackProxy> delegator_;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_PROXY_H
