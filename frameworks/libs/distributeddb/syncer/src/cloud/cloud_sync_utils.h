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
#ifndef CLOUD_SYNC_UTILS_H
#define CLOUD_SYNC_UTILS_H

#include <cstdint>
#include "cloud/cloud_store_types.h"
#include "icloud_sync_storage_interface.h"

namespace DistributedDB {
int GetCloudPkVals(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey,
    std::vector<Type> &cloudPkVals);

ChangeType OpTypeToChangeType(OpType strategy);

bool IsCompositeKey(const std::vector<std::string> &pKColNames);

bool IsNoPrimaryKey(const std::vector<std::string> &pKColNames);

bool IsSinglePrimaryKey(const std::vector<std::string> &pKColNames);

int GetSinglePk(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey, Type &pkVal);

void RemoveDataExceptExtendInfo(VBucket &datum, const std::vector<std::string> &pkColNames);
}
#endif // CLOUD_SYNC_UTILS_H