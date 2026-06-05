/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef REMOTE_FILE_SHARE_H
#define REMOTE_FILE_SHARE_H

#include <string>
#include <unordered_map>
#include <vector>

namespace OHOS {
namespace AppFileService {
namespace ModuleRemoteFileShare {
namespace {
    const std::string SHARE_ALL_DEVICE = "0";
}

struct HmdfsUriInfo {
    std::string uriStr;
    size_t fileSize;
};

class RemoteFileShare {
public:
    RemoteFileShare() {}
    static int32_t GetDfsUrisDirFromLocal(const std::vector<std::string> &uriList, const int32_t &userId,
                                            std::unordered_map<std::string, HmdfsUriInfo> &uriToDfsUriMaps);
    static int32_t TransRemoteUriToLocal(const std::vector<std::string> &uriList,
                                         const std::string &networkId,
                                         const std::string &deviceId,
                                         std::vector<std::string> &resultList);
    ~RemoteFileShare() {}
};
} // namespace ModuleRemoteFileShare
} // namespace AppFileService
} // namespace OHOS

#endif