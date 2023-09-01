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
#ifndef KVSTORE_DATA_SERVICE_MGR_H
#define KVSTORE_DATA_SERVICE_MGR_H
#include <mutex>
#include <string>

#include "ikvstore_data_service.h"
#include "iremote_object.h"

namespace OHOS::DistributedKv {
class KvStoreDataServiceMgr {
public:
    static int32_t ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex, int32_t tokenId);
};
}
#endif //KVSTORE_DATA_SERVICE_MGR_H
