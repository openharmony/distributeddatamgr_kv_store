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

#ifndef FILEMANAGEMENT_DFS_DEVICE_INFO_H
#define FILEMANAGEMENT_DFS_DEVICE_INFO_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
struct DfsDeviceInfo {
    std::string networkId_;
    std::string path_;
    DfsDeviceInfo() = default;
    DfsDeviceInfo(const std::string &networkId, const std::string &path): networkId_(networkId), path_(path) {}
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS
#endif // FILEMANAGEMENT_DFS_DEVICE_INFO_H
