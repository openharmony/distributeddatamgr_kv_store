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

#ifndef ICLOUD_DATA_TRANSLATE_H
#define ICLOUD_DATA_TRANSLATE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "cloud/cloud_store_types.h"

namespace DistributedDB {
class ICloudDataTranslate {
public:
    virtual ~ICloudDataTranslate() = default;
    virtual std::vector<uint8_t> AssetToBlob(const Asset &asset) = 0;
    virtual std::vector<uint8_t> AssetsToBlob(const Assets &assets) = 0;
    virtual Asset BlobToAsset(const std::vector<uint8_t> &blob) = 0;
    virtual Assets BlobToAssets(const std::vector<uint8_t> &blob) = 0;
};
} // namespace DistributedDB

#endif // ICLOUD_DATA_TRANSLATE_H
