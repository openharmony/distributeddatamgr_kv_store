/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H
#include <limits>
#include <memory>
#include "ikvstore_observer.h"
#include "ikvdb_notifier.h"
#include "iremote_broker.h"
#include "kvstore_death_recipient.h"
#include "single_kvstore.h"
#include "types.h"
namespace OHOS::DistributedKv {
class KVDBService {
public:
    using SingleKVStore = SingleKvStore;
    struct SyncInfo {
        uint64_t seqId = std::numeric_limits<uint64_t>::max();
        int32_t mode = PUSH_PULL;
        uint32_t delay = 0;
        std::vector<std::string> devices;
        std::string query;
        uint64_t syncId = 0;
        int32_t triggerMode = 0;
    };
    enum PasswordType {
        BACKUP_SECRET_KEY = 0,
        SECRET_KEY = 1,
        BUTTON,
    };
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedKv.KVFeature");

    API_EXPORT KVDBService() = default;

    API_EXPORT virtual ~KVDBService() = default;

    virtual Status GetStoreIds(const AppId &appId, int32_t subUser, std::vector<StoreId> &storeIds) = 0;
    virtual Status BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options) = 0;
    virtual Status AfterCreate(
        const AppId &appId, const StoreId &storeId, const Options &options, const std::vector<uint8_t> &password) = 0;
    virtual Status Delete(const AppId &appId, const StoreId &storeId, int32_t subUser) = 0;
    virtual Status Close(const AppId &appId, const StoreId &storeId, int32_t subUser) = 0;
    virtual Status Sync(const AppId &appId, const StoreId &storeId, int32_t subUser, SyncInfo &syncInfo) = 0;
    virtual Status RegServiceNotifier(const AppId &appId, sptr<IKVDBNotifier> notifier) = 0;
    virtual Status UnregServiceNotifier(const AppId &appId) = 0;
    virtual Status SetSyncParam(const AppId &appId, const StoreId &storeId, int32_t subUser,
        const KvSyncParam &syncParam) = 0;
    virtual Status GetSyncParam(const AppId &appId, const StoreId &storeId, int32_t subUser,
        KvSyncParam &syncParam) = 0;
    virtual Status EnableCapability(const AppId &appId, const StoreId &storeId, int32_t subUser) = 0;
    virtual Status DisableCapability(const AppId &appId, const StoreId &storeId, int32_t subUser) = 0;
    virtual Status SetCapability(const AppId &appId, const StoreId &storeId, int32_t subUser,
        const std::vector<std::string> &local, const std::vector<std::string> &remote) = 0;
    virtual Status AddSubscribeInfo(const AppId &appId, const StoreId &storeId, int32_t subUser,
        const SyncInfo &syncInfo) = 0;
    virtual Status RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, int32_t subUser,
        const SyncInfo &syncInfo) = 0;
    virtual Status Subscribe(const AppId &appId, const StoreId &storeId, int32_t subUser,
        sptr<IKvStoreObserver> observer) = 0;
    virtual Status Unsubscribe(const AppId &appId, const StoreId &storeId, int32_t subUser,
        sptr<IKvStoreObserver> observer) = 0;
    virtual Status GetBackupPassword(const AppId &appId, const StoreId &storeId, int32_t subUser,
        std::vector<std::vector<uint8_t>> &passwords, int32_t passwordType) = 0;
    virtual Status CloudSync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) = 0;
    virtual Status NotifyDataChange(const AppId &appId, const StoreId &storeId, uint64_t delay) = 0;
    virtual Status PutSwitch(const AppId &appId, const SwitchData &data) = 0;
    virtual Status GetSwitch(const AppId &appId, const std::string &networkId, SwitchData &data) = 0;
    virtual Status SubscribeSwitchData(const AppId &appId) = 0;
    virtual Status UnsubscribeSwitchData(const AppId &appId) = 0;
    virtual Status SetConfig(const AppId &appId, const StoreId &storeId, const StoreConfig &storeConfig) = 0;
    virtual Status RemoveDeviceData(const AppId &appId, const StoreId &storeId, int32_t subUser,
        const std::string &device) = 0;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H