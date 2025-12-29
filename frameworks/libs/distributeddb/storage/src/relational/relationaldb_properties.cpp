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
#ifdef RELATIONAL_STORE
#include "relationaldb_properties.h"

namespace DistributedDB {
const std::string RelationalDBProperties::DISTRIBUTED_TABLE_MODE = "distributed_table_mode";

RelationalDBProperties::RelationalDBProperties()
    : schema_(),
      isEncrypted_(false),
      cipherType_(),
      passwd_(),
      iterTimes_(0)
{}

void RelationalDBProperties::SetSchema(const RelationalSchemaObject &schema)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    schema_ = schema;
}

RelationalSchemaObject RelationalDBProperties::GetSchema() const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    return schema_;
}

void RelationalDBProperties::SetCipherArgs(CipherType cipherType, const CipherPassword &passwd, uint32_t iterTimes)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    isEncrypted_ = true;
    cipherType_ = cipherType;
    passwd_ = passwd;
    iterTimes_ = iterTimes;
}

bool RelationalDBProperties::IsEncrypted() const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    return isEncrypted_;
}

CipherType RelationalDBProperties::GetCipherType() const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    return cipherType_;
}

const CipherPassword &RelationalDBProperties::GetPasswd() const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    return passwd_;
}

uint32_t RelationalDBProperties::GetIterTimes() const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    return iterTimes_;
}

DistributedTableMode RelationalDBProperties::GetDistributedTableMode() const
{
    auto defaultMode = static_cast<int>(DistributedTableMode::SPLIT_BY_DEVICE);
    return static_cast<DistributedTableMode>(GetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE, defaultMode));
}


RelationalDBProperties::RelationalDBProperties(const RelationalDBProperties &other)
    : DBProperties(other)
{
    CopyRDBProperties(other);
}

RelationalDBProperties &RelationalDBProperties::operator=(const RelationalDBProperties &other)
{
    DBProperties::operator=(other);
    CopyRDBProperties(other);
    return *this;
}

void RelationalDBProperties::CopyRDBProperties(const RelationalDBProperties &other)
{
    if (&other == this) {
        return;
    }
    std::scoped_lock<std::mutex, std::mutex> scopedLock(dataMutex_, other.dataMutex_);
    schema_ = other.schema_;
    isEncrypted_ = other.isEncrypted_;
    cipherType_ = other.cipherType_;
    passwd_ = other.passwd_;
    iterTimes_ = other.iterTimes_;
}
}
#endif