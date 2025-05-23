/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "db_common.h"
#include "db_properties.h"

namespace DistributedDB {
std::string DBProperties::GetStringProp(const std::string &name, const std::string &defaultValue) const
{
    auto iter = stringProperties_.find(name);
    if (iter != stringProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetStringProp(const std::string &name, const std::string &value)
{
    stringProperties_[name] = value;
}

bool DBProperties::GetBoolProp(const std::string &name, bool defaultValue) const
{
    auto iter = boolProperties_.find(name);
    if (iter != boolProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetBoolProp(const std::string &name, bool value)
{
    boolProperties_[name] = value;
}

int DBProperties::GetIntProp(const std::string &name, int defaultValue) const
{
    auto iter = intProperties_.find(name);
    if (iter != intProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetIntProp(const std::string &name, int value)
{
    intProperties_[name] = value;
}

uint32_t DBProperties::GetUIntProp(const std::string &name, uint32_t defaultValue) const
{
    auto iter = uintProperties_.find(name);
    if (iter != uintProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetUIntProp(const std::string &name, uint32_t value)
{
    uintProperties_[name] = value;
}

void DBProperties::SetIdentifier(const std::string &userId, const std::string &appId, const std::string &storeId,
    const std::string &subUser, int32_t instanceId)
{
    SetStringProp(DBProperties::APP_ID, appId);
    SetStringProp(DBProperties::USER_ID, userId);
    SetStringProp(DBProperties::STORE_ID, storeId);
    SetStringProp(DBProperties::SUB_USER, subUser);
    SetIntProp(DBProperties::INSTANCE_ID, instanceId);
    std::string hashIdentifier = DBCommon::TransferHashString(
        DBCommon::GenerateIdentifierId(storeId, appId, userId, subUser, instanceId));
    SetStringProp(DBProperties::IDENTIFIER_DATA, hashIdentifier);
    std::string dualIdentifier = DBCommon::TransferHashString(DBCommon::GenerateDualTupleIdentifierId(storeId, appId));
    SetStringProp(DBProperties::DUAL_TUPLE_IDENTIFIER_DATA, dualIdentifier);
}
}