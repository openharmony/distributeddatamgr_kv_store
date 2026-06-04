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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_INTERFACE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_INTERFACE_H

#include "iremote_broker.h"

namespace OHOS {
namespace AppExecFwk {
class IProcessCacheCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.ProcessCacheCallback");

    /**
     * @brief Called when get all bundle cache files progress finished.
     * @param cacheStat Indicates the result of progress.
     */
    virtual void OnGetAllBundleCacheFinished(uint64_t cacheStat) = 0;

    /**
     * @brief Called when del all bundle cache files progress finished.
     * @param result Indicates the result of progress.
     */
    virtual void OnCleanAllBundleCacheFinished(int32_t resulted) = 0;

    /**
     * @brief get all bundle cache stat.
     */
    virtual uint64_t GetCacheStat() = 0;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_PROCESS_CACHE_CALLBACK_INTERFACE_H
