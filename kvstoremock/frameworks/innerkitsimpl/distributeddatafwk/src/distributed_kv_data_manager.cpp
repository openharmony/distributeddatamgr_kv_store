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
#define LOG_TAG "DistributedKvDataManager"
#include "distributed_kv_data_manager.h"

#include "constant.h"
#include "log_print.h"
#include "store_manager.h"

namespace OHOS {
namespace DistributedKv {
DistributedKvDataManager::DistributedKvDataManager()
{}

DistributedKvDataManager::~DistributedKvDataManager()
{}

Status DistributedKvDataManager::GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
                                                  std::shared_ptr<SingleKvStore> &singleKvStore)
{
    singleKvStore = nullptr;

    if (!options.baseDir.empty()) {
        Status status = Status::INVALID_ARGUMENT;
        singleKvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
        return status;
    }

    return SUCCESS;
}

Status DistributedKvDataManager::GetAllKvStoreId(const AppId &appId, std::vector<StoreId> &storeIds)
{
    auto status = StoreManager::GetInstance().GetStoreIds(appId, storeIds);
    if (status == Status::SUCCESS) {
        return status;
    }
    return SUCCESS;
}

Status DistributedKvDataManager::CloseKvStore(const AppId &appId, const StoreId &storeId)
{
    auto status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    if (status == SUCCESS) {
        return status;
    }

    return SUCCESS;
}

Status DistributedKvDataManager::CloseKvStore(const AppId &appId, std::shared_ptr<SingleKvStore> &kvStorePtr)
{
    if (kvStorePtr == nullptr) {
        ZLOGE("kvStorePtr is nullptr.");
        return Status::INVALID_ARGUMENT;
    }
    StoreId storeId = kvStorePtr->GetStoreId();
    kvStorePtr = nullptr;

    auto status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    if (status == SUCCESS) {
        return status;
    }

    return SUCCESS;
}

Status DistributedKvDataManager::CloseAllKvStore(const AppId &appId)
{
    auto status = StoreManager::GetInstance().CloseAllKVStore(appId);

    return status;
}

Status DistributedKvDataManager::DeleteKvStore(const AppId &appId, const StoreId &storeId, const std::string &path)
{
    if (!path.empty()) {
        return StoreManager::GetInstance().Delete(appId, storeId, path);
    }

    return SUCCESS;
}

Status DistributedKvDataManager::DeleteAllKvStore(const AppId &appId, const std::string &path)
{
    if (!path.empty()) {
        std::vector<StoreId> storeIds;
        Status status = GetAllKvStoreId(appId, storeIds);
        if (status != SUCCESS) {
            return status;
        }
        for (auto &storeId : storeIds) {
            status = StoreManager::GetInstance().Delete(appId, storeId, path);
            if (status != SUCCESS) {
                return status;
            }
        }
        return SUCCESS;
    }

    return SUCCESS;
}

void DistributedKvDataManager::RegisterKvStoreServiceDeathRecipient(
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient)
{
    ZLOGD("begin");
    if (kvStoreDeathRecipient == nullptr) {
        ZLOGW("Register KvStoreService Death Recipient input is null.");
        return;
    }
}

void DistributedKvDataManager::UnRegisterKvStoreServiceDeathRecipient(
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient)
{
    ZLOGD("begin");
    if (kvStoreDeathRecipient == nullptr) {
        ZLOGW("UnRegister KvStoreService Death Recipient input is null.");
        return;
    }
}

Status DistributedKvDataManager::GetLocalDevice(DeviceInfo &localDevice)
{
    return SUCCESS;
}

Status DistributedKvDataManager::GetDeviceList(std::vector<DeviceInfo> &deviceInfoList, DeviceFilterStrategy strategy)
{
    return SUCCESS;
}

Status DistributedKvDataManager::StartWatchDeviceChange(std::shared_ptr<DeviceStatusChangeListener> observer)
{
    return Status::SUCCESS;
}

Status DistributedKvDataManager::StopWatchDeviceChange(std::shared_ptr<DeviceStatusChangeListener> observer)
{
    return SUCCESS;
}
}  // namespace DistributedKv
}  // namespace OHOS
