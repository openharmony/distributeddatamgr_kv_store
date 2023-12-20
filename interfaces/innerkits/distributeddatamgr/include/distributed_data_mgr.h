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

#ifndef DISTRIBUTED_DATA_MGR_H
#define DISTRIBUTED_DATA_MGR_H
#include <cstdio>
#include <string>
#include "visibility.h"
namespace OHOS::DistributedKv {
class DistributedDataMgr {
public:
    /**
     * @brief Constructor.
     */
    API_EXPORT DistributedDataMgr();

    /**
     * @brief Destructor.
     */
    API_EXPORT ~DistributedDataMgr();
    /**
     * @brief Clear the data of given bundleName, userID and appIndex in datamgr_service.
     * @param bundleName The bundle name.
     * @param userId The user ID.
     * @param appIndex The app index in sandbox.
     * @param tokenId The token id to clear.
    */
    API_EXPORT int32_t ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex,
        int32_t tokenId);
};
}  // namespace OHOS
#endif // DISTRIBUTED_DATA_MGR_H
