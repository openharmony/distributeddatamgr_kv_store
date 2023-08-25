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

#ifndef DISTRIBUTED_DATA_HELPER_H
#define DISTRIBUTED_DATA_HELPER_H
#include <cstdio>
#include <string>
#include "visibility.h"
namespace OHOS::DistributedKv {
class API_EXPORT DistributedDataHelper {
public:
    /**
     * @brief Clear the data of given bundleName, userID and appIndex in datamgr_service.
     * @param bundleName The bundle name.
     * @param userId The user ID.
     * @param appIndex The app index in sandbox.
    */
    static API_EXPORT int32_t ClearData(const std::string &bundleName, int32_t userId, int32_t appIndex);
};
}  // namespace OHOS
#endif //DISTRIBUTED_DATA_HELPER_H
