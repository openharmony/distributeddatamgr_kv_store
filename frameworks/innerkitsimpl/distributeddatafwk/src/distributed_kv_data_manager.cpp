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

#include "dds_trace.h"
#include "dev_manager.h"
#include "device_status_change_listener_client.h"
#include "ikvstore_data_service.h"
#include "kvstore_service_death_notifier.h"
#include "log_print.h"
#include "refbase.h"
#include "store_manager.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::DistributedDataDfx;
DistributedKvDataManager::DistributedKvDataManager()
{}

DistributedKvDataManager::~DistributedKvDataManager()
{}

Status DistributedKvDataManager::GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
                                                  std::shared_ptr<SingleKvStore> &singleKvStore)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    singleKvStore = nullptr;
    if (!storeId.IsValid()) {
        ZLOGE("invalid storeId.");
        return Status::INVALID_ARGUMENT;
    }
    if (options.baseDir.empty()) {
        ZLOGE("base dir empty.");
        return Status::INVALID_ARGUMENT;
    }
    KvStoreServiceDeathNotifier::SetAppId(appId);

    Status status = Status::INVALID_ARGUMENT;
    singleKvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    return status;
}

Status DistributedKvDataManager::GetAllKvStoreId(const AppId &appId, std::vector<StoreId> &storeIds)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    KvStoreServiceDeathNotifier::SetAppId(appId);
    return StoreManager::GetInstance().GetStoreIds(appId, storeIds);
}

Status DistributedKvDataManager::CloseKvStore(const AppId &appId, const StoreId &storeId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    KvStoreServiceDeathNotifier::SetAppId(appId);
    if (!storeId.IsValid()) {
        ZLOGE("invalid storeId.");
        return Status::INVALID_ARGUMENT;
    }

    return StoreManager::GetInstance().CloseKVStore(appId, storeId);
}

Status DistributedKvDataManager::CloseKvStore(const AppId &appId, std::shared_ptr<SingleKvStore> &kvStorePtr)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    if (kvStorePtr == nullptr) {
        ZLOGE("kvStorePtr is nullptr.");
        return Status::INVALID_ARGUMENT;
    }
    KvStoreServiceDeathNotifier::SetAppId(appId);
    StoreId storeId = kvStorePtr->GetStoreId();
    kvStorePtr = nullptr;

    return StoreManager::GetInstance().CloseKVStore(appId, storeId);
}

Status DistributedKvDataManager::CloseAllKvStore(const AppId &appId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    KvStoreServiceDeathNotifier::SetAppId(appId);
    return StoreManager::GetInstance().CloseAllKVStore(appId);
}

Status DistributedKvDataManager::DeleteKvStore(const AppId &appId, const StoreId &storeId, const std::string &path)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    if (!storeId.IsValid()) {
        ZLOGE("invalid storeId.");
        return Status::INVALID_ARGUMENT;
    }
    if (path.empty()) {
        ZLOGE("path empty");
        return Status::INVALID_ARGUMENT;
    }
    KvStoreServiceDeathNotifier::SetAppId(appId);

    return StoreManager::GetInstance().Delete(appId, storeId, path);
}

Status DistributedKvDataManager::DeleteAllKvStore(const AppId &appId, const std::string &path)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);
    if (path.empty()) {
        ZLOGE("path empty");
        return Status::INVALID_ARGUMENT;
    }
    KvStoreServiceDeathNotifier::SetAppId(appId);

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

void DistributedKvDataManager::RegisterKvStoreServiceDeathRecipient(
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient)
{
    ZLOGD("begin");
    if (kvStoreDeathRecipient == nullptr) {
        ZLOGW("Register KvStoreService Death Recipient input is null.");
        return;
    }
    KvStoreServiceDeathNotifier::AddServiceDeathWatcher(kvStoreDeathRecipient);
}

void DistributedKvDataManager::UnRegisterKvStoreServiceDeathRecipient(
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient)
{
    ZLOGD("begin");
    if (kvStoreDeathRecipient == nullptr) {
        ZLOGW("UnRegister KvStoreService Death Recipient input is null.");
        return;
    }
    KvStoreServiceDeathNotifier::RemoveServiceDeathWatcher(kvStoreDeathRecipient);
}

Status DistributedKvDataManager::GetLocalDevice(DeviceInfo &localDevice)
{
    auto dvInfo = DevManager::GetInstance().GetLocalDevice();
    if (dvInfo.networkId.empty()) {
        ZLOGE("deviceId empty!");
        return Status::ERROR;
    }
    localDevice.deviceId = dvInfo.networkId;
    return Status::SUCCESS;
}

Status DistributedKvDataManager::GetDeviceList(std::vector<DeviceInfo> &deviceInfoList, DeviceFilterStrategy strategy)
{
    auto dvInfos = DevManager::GetInstance().GetRemoteDevices();
    if (dvInfos.empty()) {
        ZLOGD("no remote device!");
        return Status::ERROR;
    }
    for (const auto &info : dvInfos) {
        if (info.networkId.empty()) {
            ZLOGW("deviceId empty!");
            continue;
        }
        DeviceInfo devInfo = {
            .deviceId = info.networkId,
        };
        deviceInfoList.emplace_back(devInfo);
    }
    ZLOGI("strategy is:%{public}d", strategy);
    return Status::SUCCESS;
}

static std::map<DeviceStatusChangeListener *, DeviceStatusChangeListenerClient *> deviceObservers_;
static std::mutex deviceObserversMapMutex_;
Status DistributedKvDataManager::StartWatchDeviceChange(std::shared_ptr<DeviceStatusChangeListener> observer)
{
    DeviceStatusChangeListenerClient *observerClient = new(std::nothrow) DeviceStatusChangeListenerClient(observer);
    if (observerClient == nullptr) {
        ZLOGW("new DeviceStatusChangeListenerClient failed");
        return Status::ERROR;
    }

    DevManager::GetInstance().Register(observerClient);
    {
        std::lock_guard<std::mutex> lck(deviceObserversMapMutex_);
        deviceObservers_.insert({ observer.get(), observerClient });
    }
    return Status::SUCCESS;
}

Status DistributedKvDataManager::StopWatchDeviceChange(std::shared_ptr<DeviceStatusChangeListener> observer)
{
    std::lock_guard<std::mutex> lck(deviceObserversMapMutex_);
    auto it = deviceObservers_.find(observer.get());
    if (it == deviceObservers_.end()) {
        ZLOGW(" not start watch device change.");
        return Status::ERROR;
    }

    DevManager::GetInstance().Unregister(it->second);
    deviceObservers_.erase(it->first);
    return Status::SUCCESS;
}
}  // namespace DistributedKv
}  // namespace OHOS
