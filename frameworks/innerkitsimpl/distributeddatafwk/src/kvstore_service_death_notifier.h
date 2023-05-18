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

#ifndef KVSTORE_SERVICE_DEATH_NOTIFIER_H
#define KVSTORE_SERVICE_DEATH_NOTIFIER_H

#include <memory>
#include <set>
#include <thread>
#include "ikvstore_data_service.h"
#include "iremote_object.h"
#include "kvstore_death_recipient.h"
#include "refbase.h"

namespace OHOS {
namespace DistributedKv {
class KvStoreServiceDeathNotifier final {
public:
    // get DistributedKvDataService proxy object.
    static sptr<IKvStoreDataService> GetDistributedKvDataService();
    // temporarily used, should get in service side from binder.
    static void SetAppId(const AppId &appId);
    static AppId GetAppId();
    // add watcher for server die msg.
    static void AddServiceDeathWatcher(std::shared_ptr<KvStoreDeathRecipient> watcher);
    // remove watcher for server die msg.
    static void RemoveServiceDeathWatcher(std::shared_ptr<KvStoreDeathRecipient> watcher);

private:
    KvStoreServiceDeathNotifier() = default;
    ~KvStoreServiceDeathNotifier() = default;
    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        ServiceDeathRecipient();

        virtual ~ServiceDeathRecipient();

        void OnRemoteDied(const wptr<IRemoteObject> &remote) override;
    };
    static KvStoreServiceDeathNotifier* GetInstance();

    static std::mutex instanceMutex_;
    static KvStoreServiceDeathNotifier *instance_;

    // add watcher for server die msg.
    void RegisterClientDeathObserver();
    AppId appId_;
    // lock for kvDataServiceProxy_ and serviceDeathWatchers_.
    std::mutex watchMutex_;
    std::mutex mutex_;
    sptr<IKvStoreDataService> kvDataServiceProxy_;
    sptr<ServiceDeathRecipient> deathRecipientPtr_;
    sptr<IRemoteObject> clientDeathObserverPtr_;
    // set of watchers for server die msg.
    std::set<std::shared_ptr<KvStoreDeathRecipient>> serviceDeathWatchers_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // KVSTORE_SERVICE_DEATH_NOTIFIER_H
