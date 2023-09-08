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

constexpr int GID_INDEX = 0;
constexpr int PREFIX_INDEX = 1;
constexpr int STRATEGY_INDEX = 2;
constexpr int ASSETS_INDEX = 3;
constexpr int HASH_KEY_INDEX = 4;
constexpr int PRIMARY_KEY_INDEX = 5;

int GetCloudPkVals(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey,
    std::vector<Type> &cloudPkVals);

ChangeType OpTypeToChangeType(OpType strategy);

bool IsSinglePrimaryKey(const std::vector<std::string> &pKColNames);

void RemoveDataExceptExtendInfo(VBucket &datum, const std::vector<std::string> &pkColNames);

AssetOpType StatusToFlag(AssetStatus status);

void StatusToFlagForAsset(Asset &asset);

void StatusToFlagForAssets(Assets &assets);

void StatusToFlagForAssetsInRecord(const std::vector<Field> &fields, VBucket &record);

bool IsChngDataEmpty(const ChangedData &changedData);

bool EqualInMsLevel(const Timestamp cmp, const Timestamp beCmp);

bool NeedSaveData(const LogInfo &localLogInfo, const LogInfo &cloudLogInfo);
}
#endif // CLOUD_SYNC_UTILS_H