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

#ifndef KVSTORE_SYNC_CALLBACK_H
#define KVSTORE_SYNC_CALLBACK_H

#include <map>
#include "types.h"

namespace OHOS {
namespace DistributedKv {
// client implement this class to watch kvstore change.
class KvStoreSyncCallback {
public:
    /**
     * @brief Constructor.
     */
    API_EXPORT KvStoreSyncCallback() = default;

    /**
     * @brief Destructor.
     */
    API_EXPORT virtual ~KvStoreSyncCallback()  {}

    /**
     * @brief This virtual function will be called on sync callback.
     *
     * client should override this function to receive sync results.
     *
     * @param results sync results for devices set in sync function.
    */
    API_EXPORT virtual void SyncCompleted(const std::map<std::string, Status> &results) = 0;

    /**
     * @brief This virtual function will be called on sync callback.
     *
     * @param results sync results for devices set in sync function.
     * @param sequenceId sequence Id of current synchronization.
    */
    API_EXPORT virtual void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) {}
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // KVSTORE_SYNC_CALLBACK_H
