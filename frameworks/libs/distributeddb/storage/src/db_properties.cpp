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
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    auto iter = stringProperties_.find(name);
    if (iter != stringProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetStringProp(const std::string &name, const std::string &value)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    stringProperties_[name] = value;
}

bool DBProperties::GetBoolProp(const std::string &name, bool defaultValue) const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    auto iter = boolProperties_.find(name);
    if (iter != boolProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetBoolProp(const std::string &name, bool value)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    boolProperties_[name] = value;
}

int DBProperties::GetIntProp(const std::string &name, int defaultValue) const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    auto iter = intProperties_.find(name);
    if (iter != intProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetIntProp(const std::string &name, int value)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    intProperties_[name] = value;
}

uint32_t DBProperties::GetUIntProp(const std::string &name, uint32_t defaultValue) const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    auto iter = uintProperties_.find(name);
    if (iter != uintProperties_.end()) {
        return iter->second;
    }
    return defaultValue;
}

void DBProperties::SetUIntProp(const std::string &name, uint32_t value)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
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

DBProperties::DBProperties(const DBProperties &other)
{
    CopyProperties(other);
}

DBProperties &DBProperties::operator=(const DBProperties &other)
{
    CopyProperties(other);
    return *this;
}

void DBProperties::CopyProperties(const DBProperties &other)
{
    if (&other == this) {
        return;
    }
    std::scoped_lock<std::mutex, std::mutex> scopedLock(dataMutex_, other.dataMutex_);
    stringProperties_ = other.stringProperties_;
    boolProperties_ = other.boolProperties_;
    intProperties_ = other.intProperties_;
    uintProperties_ = other.uintProperties_;
}
}