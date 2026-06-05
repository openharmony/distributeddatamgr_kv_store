/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "common_func.h"
#include "file_uri.h"
namespace OHOS::AppFileService {
std::string CommonFunc::GetSelfBundleName()
{
    return std::string();
}
std::string CommonFunc::GetUriFromPath(const std::string& path)
{
    return std::string();
}
bool CommonFunc::GetDirByBundleNameAndAppIndex(const std::string &bundleName, int32_t appIndex, std::string &dirName)
{
    (void)bundleName;
    (void)appIndex;
    (void)dirName;
    return false;
}
bool CommonFunc::EndsWith(const std::string &str, const std::string &suffix)
{
    (void)str;
    (void)suffix;
    return false;
}
namespace ModuleFileUri {
std::string FileUri::GetName()
{
    return "";
}

std::string FileUri::GetPath()
{
    return "";
}

std::string FileUri::ToString()
{
    return "";
}

FileUri::FileUri(const std::string& uriOrPath)
    : uri_(uriOrPath)
{
}

std::string FileUri::GetRealPath()
{
    return "";
}
}
}