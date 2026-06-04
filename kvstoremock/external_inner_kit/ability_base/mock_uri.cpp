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

#include "uri.h"

namespace OHOS {
Uri::Uri(const std::string &uriString)
    : uriString_(uriString), port_(-1), cachedSsi_(std::string::npos), cachedFsi_(std::string::npos)
{
}

Uri::~Uri() {}

std::string Uri::GetScheme()
{
    return scheme_;
}

std::string Uri::GetSchemeSpecificPart()
{
    return ssp_;
}

std::string Uri::GetAuthority()
{
    return authority_;
}

std::string Uri::GetHost()
{
    return host_;
}

int Uri::GetPort()
{
    return port_;
}

std::string Uri::GetUserInfo()
{
    return userInfo_;
}

std::string Uri::GetQuery()
{
    return query_;
}

std::string Uri::GetPath()
{
    return path_;
}

void Uri::GetPathSegments(std::vector<std::string> &segments)
{
    segments.clear();
}

std::string Uri::GetFragment()
{
    return fragment_;
}

bool Uri::IsHierarchical()
{
    return false;
}

bool Uri::IsAbsolute()
{
    return !scheme_.empty();
}

bool Uri::IsRelative()
{
    return scheme_.empty();
}

bool Uri::Equals(const Uri &other) const
{
    return uriString_ == other.uriString_;
}

int Uri::CompareTo(const Uri &other) const
{
    return uriString_.compare(other.uriString_);
}

std::string Uri::ToString() const
{
    return uriString_;
}

bool Uri::operator==(const Uri &other) const
{
    return Equals(other);
}

bool Uri::Marshalling(Parcel &parcel) const
{
    return parcel.WriteString(uriString_);
}

Uri *Uri::Unmarshalling(Parcel &parcel)
{
    std::string uriString;
    if (!parcel.ReadString(uriString)) {
        return nullptr;
    }
    return new Uri(uriString);
}

bool Uri::CheckScheme()
{
    return true;
}

std::string Uri::ParseScheme()
{
    return "";
}

std::string Uri::ParseSsp()
{
    return "";
}

std::string Uri::ParseAuthority()
{
    return "";
}

std::string Uri::ParseUserInfo()
{
    return "";
}

std::string Uri::ParseHost()
{
    return "";
}

int Uri::ParsePort()
{
    return -1;
}

std::string Uri::ParsePath(size_t ssi)
{
    return "";
}

std::string Uri::ParsePath()
{
    return "";
}

std::string Uri::ParseQuery()
{
    return "";
}

std::string Uri::ParseFragment()
{
    return "";
}

size_t Uri::FindSchemeSeparator()
{
    if (cachedSsi_ != std::string::npos) {
        return cachedSsi_;
    }
    return std::string::npos;
}

size_t Uri::FindFragmentSeparator()
{
    if (cachedFsi_ != std::string::npos) {
        return cachedFsi_;
    }
    return std::string::npos;
}
}