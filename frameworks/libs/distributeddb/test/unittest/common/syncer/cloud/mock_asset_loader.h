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

#ifndef MOCK_ASSET_LOADER_H
#define MOCK_ASSET_LOADER_H
#include <gmock/gmock.h>
#include "cloud/iAssetLoader.h"

namespace DistributedDB {
class MockAssetLoader : public IAssetLoader {
public:
    MOCK_METHOD4(Download, DBStatus(const std::string &, const std::string &, const Type &,
        std::map<std::string, Assets> &));
    MOCK_METHOD1(RemoveLocalAssets, DBStatus(const std::vector<Asset> &));
};
}
#endif // MOCK_ASSET_LOADER_H
