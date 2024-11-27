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
#include "include/kvdb_service_client_mock.h"

namespace OHOS::DistributedKv {

KVDBServiceClient::KVDBServiceClient(const sptr<IRemoteObject> &handle) : IRemoteProxy(handle) { }

std::shared_ptr<KVDBServiceClient> KVDBServiceClient::GetInstance()
{
    if (BKVDBServiceClient::kVDBServiceClient == nullptr) {
        return nullptr;
    }
    return BKVDBServiceClient::kVDBServiceClient->GetInstance();
}

sptr<KVDBNotifierClient> KVDBServiceClient::GetServiceAgent(const AppId &appId)
{
    if (BKVDBServiceClient::kVDBServiceClient == nullptr) {
        return nullptr;
    }
    return BKVDBServiceClient::kVDBServiceClient->GetServiceAgent(appId);
}

void KVDBServiceClient::ServiceDeath::OnRemoteDied()
{
    instance_ = nullptr;
}

Status KVDBServiceClient::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::AfterCreate(
    const AppId &appId, const StoreId &storeId, const Options &options, const std::vector<uint8_t> &password)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::Delete(const AppId &appId, const StoreId &storeId)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::Close(const AppId &appId, const StoreId &storeId)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::Sync(const AppId &appId, const StoreId &storeId, SyncInfo &syncInfo)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::CloudSync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::NotifyDataChange(const AppId &appId, const StoreId &storeId, uint64_t delay)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::RegServiceNotifier(const AppId &appId, sptr<IKVDBNotifier> notifier)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::UnregServiceNotifier(const AppId &appId)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::EnableCapability(const AppId &appId, const StoreId &storeId)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::DisableCapability(const AppId &appId, const StoreId &storeId)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::SetCapability(const AppId &appId, const StoreId &storeId,
    const std::vector<std::string> &local, const std::vector<std::string> &remote)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::GetBackupPassword(
    const AppId &appId, const StoreId &storeId, std::vector<uint8_t> &password, int32_t passwordType)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::PutSwitch(const AppId &appId, const SwitchData &data)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::GetSwitch(const AppId &appId, const std::string &networkId, SwitchData &data)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::SubscribeSwitchData(const AppId &appId)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::UnsubscribeSwitchData(const AppId &appId)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::SetConfig(const AppId &appId, const StoreId &storeId, const StoreConfig &storeConfig)
{
    return KVDBServiceClientMock::status;
}

Status KVDBServiceClient::RemoveDeviceData(const AppId &appId, const StoreId &storeId, const std::string &device)
{
    return KVDBServiceClientMock::status;
}
} // namespace OHOS::DistributedKv