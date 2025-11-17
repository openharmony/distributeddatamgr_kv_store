/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ICLOUD_CONFLICT_HANDLER_H
#define ICLOUD_CONFLICT_HANDLER_H

#include "cloud_store_types.h"

namespace DistributedDB {
enum class ConflictRet : uint32_t {
    UPSERT = 0,
    DELETE,
    NOT_HANDLE,
    BUTT
};

class ICloudConflictHandler {
public:
    ICloudConflictHandler() = default;
    virtual ~ICloudConflictHandler() = default;
    virtual ConflictRet HandleConflict(const std::string &table, const VBucket &oldData, const VBucket &newData,
        VBucket &upsert) = 0;
};
}
#endif // ICLOUD_CONFLICT_HANDLER_H