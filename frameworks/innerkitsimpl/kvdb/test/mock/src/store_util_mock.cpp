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

#include "include/store_util_mock.h"
#include "types.h"

namespace OHOS::DistributedKv {

bool StoreUtil::IsFileExist(const std::string &name)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return false;
    }
    return BStoreUtil::storeUtil->IsFileExist(name);
}

bool StoreUtil::Remove(const std::string &path)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return false;
    }
    return BStoreUtil::storeUtil->Remove(path);
}

Status StoreUtil::ConvertStatus(StoreUtil::DBStatus status)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return Status::ERROR;
    }
    return BStoreUtil::storeUtil->ConvertStatus(status);
}

StoreUtil::DBSecurity StoreUtil::GetDBSecurity(int32_t secLevel)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return { DistributedDB::NOT_SET, DistributedDB::ECE };
    }
    return BStoreUtil::storeUtil->GetDBSecurity(secLevel);
}

bool StoreUtil::InitPath(const std::string &path)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return false;
    }
    return BStoreUtil::storeUtil->InitPath(path);
}

StoreUtil::DBIndexType StoreUtil::GetDBIndexType(IndexType type)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return DistributedDB::HASH;
    }
    return BStoreUtil::storeUtil->GetDBIndexType(type);
}

std::vector<std::string> StoreUtil::GetSubPath(const std::string &path)
{
    if (BStoreUtil::storeUtil == nullptr) {
        std::vector<std::string> vec;
        return vec;
    }
    return BStoreUtil::storeUtil->GetSubPath(path);
}

std::vector<StoreUtil::FileInfo> StoreUtil::GetFiles(const std::string &path)
{
    if (BStoreUtil::storeUtil == nullptr) {
        std::vector<StoreUtil::FileInfo> fileInfos;
        return fileInfos;
    }
    return BStoreUtil::storeUtil->GetFiles(path);
}

bool StoreUtil::Rename(const std::string &oldName, const std::string &newName)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return false;
    }
    return BStoreUtil::storeUtil->Rename(oldName, newName);
}

std::string StoreUtil::Anonymous(const std::string &name)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return "";
    }
    return BStoreUtil::storeUtil->Anonymous(name);
}

uint32_t StoreUtil::Anonymous(const void *ptr)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return 0;
    }
    return BStoreUtil::storeUtil->Anonymous(ptr);
}

bool StoreUtil::RemoveRWXForOthers(const std::string &path)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return false;
    }
    return BStoreUtil::storeUtil->RemoveRWXForOthers(path);
}

bool StoreUtil::CreateFile(const std::string &name)
{
    if (BStoreUtil::storeUtil == nullptr) {
        return false;
    }
    return BStoreUtil::storeUtil->CreateFile(name);
}

void StoreUtil::Flush()
{
}

int32_t StoreUtil::GetSecLevel(StoreUtil::DBSecurity dbSec)
{
    return NO_LABEL;
}

uint64_t StoreUtil::GenSequenceId()
{
    return 0;
}

StoreUtil::DBMode StoreUtil::GetDBMode(SyncMode syncMode)
{
    DBMode dbMode = DBMode::SYNC_MODE_PULL_ONLY;
    return dbMode;
}
} // namespace OHOS::DistributedKv