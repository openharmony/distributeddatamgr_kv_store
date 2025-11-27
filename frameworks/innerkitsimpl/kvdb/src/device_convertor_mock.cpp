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

#include "device_convertor.h"

namespace OHOS::DistributedKv {
std::vector<uint8_t> DeviceConvertor::ToLocalDBKey(const Key &key) const
{
    return GetPrefix(key);
}

std::vector<uint8_t> DeviceConvertor::ToWholeDBKey(const Key &key) const
{
    return GetPrefix(key);
}

Key DeviceConvertor::ToKey(DBKey &&key, std::string &deviceId) const
{
    (void)deviceId;
    return std::move(key);
}

std::vector<uint8_t> DeviceConvertor::GetPrefix(const Key &prefix) const
{
    std::vector<uint8_t> dbKey = TrimKey(prefix);
    if (dbKey.size() > MAX_KEY_LENGTH) {
        dbKey.clear();
    }
    return dbKey;
}

std::vector<uint8_t> DeviceConvertor::GetPrefix(const DataQuery &query) const
{
    return GetPrefix(Key(query.prefix_));
}

std::string DeviceConvertor::GetRealKey(const std::string &key, const DataQuery &query) const
{
    (void)query;
    return key;
}

std::vector<uint8_t> DeviceConvertor::ConvertNetwork(const Key &in, bool withLen) const
{
    (void)in;
    (void)withLen;
    return {};
}

std::vector<uint8_t> DeviceConvertor::ToLocal(const Key &in, bool withLen) const
{
    (void)in;
    (void)withLen;
    return {};
}
}