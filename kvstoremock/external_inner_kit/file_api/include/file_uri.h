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

#ifndef APP_FILE_SERVICE_FILE_URI_FILE_URI_H
#define APP_FILE_SERVICE_FILE_URI_FILE_URI_H

#include <string>

#include "uri/uri.h"
namespace OHOS {
namespace AppFileService {
namespace ModuleFileUri {
class FileUri {
public:
    std::string GetName();
    std::string GetPath();
    std::string GetRealPath();
    std::string GetRealPathBySA(const std::string &targetBundleName = "");
    std::string ToString();
    std::string GetFullDirectoryUri();
    bool IsRemoteUri();

    bool CheckUriFormat(const std::string &uri);
    explicit FileUri(const std::string &uriOrPath);
    ~FileUri() = default;

    Uri uri_;
};
}  // ModuleFileUri
}  // namespace AppFileService
}  // namespace OHOS
#endif  // APP_FILE_SERVICE_FILE_URI_FILE_URI_H