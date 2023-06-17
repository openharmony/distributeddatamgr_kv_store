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
#ifndef IASSET_LOADER_H
#define IASSET_LOADER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "cloud/cloud_store_types.h"

namespace DistributedDB {
class IAssetLoader {
public:
    virtual ~IAssetLoader() = default;

    // for single primary key data, prefix is the key; otherwise, prefix is default value of std::std::monostate
    virtual DBStatus Download(const std::string &tableName, const std::string &gid, const Type &prefix,
        std::map<std::string, Assets> &assets)
    {
        return DBStatus::OK;
    }

    virtual DBStatus RemoveLocalAssets(const std::vector<Asset> &assets)
    {
        return DBStatus::OK;
    }
};
} // namespace DistributedDB

#endif // IASSET_LOADER_H