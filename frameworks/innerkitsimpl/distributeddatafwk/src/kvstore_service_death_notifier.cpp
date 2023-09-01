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

#define LOG_TAG "KvStoreServiceDeathNotifier"

#include "kvstore_service_death_notifier.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "kvstore_client_death_observer.h"
#include "datamgr_service_proxy.h"
#include "log_print.h"
#include "refbase.h"
#include "system_ability_definition.h"
#include "task_executor.h"

namespace OHOS {
namespace DistributedKv {

std::mutex KvStoreServiceDeathNotifier::instanceMutex_;
KvStoreServiceDeathNotifier* KvStoreServiceDeathNotifier::instance_ = nullptr;

KvStoreServiceDeathNotifier* KvStoreServiceDeathNotifier::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        if (instance_ == nullptr) {
            instance_ = new (std::nothrow) KvStoreServiceDeathNotifier();
            if (instance_ == nullptr) {
                ZLOGE("KvStoreServiceDeathNotifier nullptr");
            }
            return instance_;
        }
    }
    return instance_;
}

void KvStoreServiceDeathNotifier::SetAppId(const AppId &appId)
{
    auto *instance = GetInstance();
    if (instance == nullptr) {
        return;
    }
    std::lock_guard<decltype(mutex_)> lg(instance->mutex_);
    instance->appId_ = appId;
}

AppId KvStoreServiceDeathNotifier::GetAppId()
{
    auto *instance = GetInstance();
    if (instance == nullptr) {
        return {};
    }
    std::lock_guard<decltype(mutex_)> lg(instance->mutex_);
    return instance->appId_;
}

sptr<IKvStoreDataService> KvStoreServiceDeathNotifier::GetDistributedKvDataService()
{
    ZLOGD("begin.");
    auto *instance = GetInstance();
    if (instance == nullptr) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lg(instance->watchMutex_);
    if (instance->kvDataServiceProxy_ != nullptr) {
        return instance->kvDataServiceProxy_;
    }

    ZLOGI("create remote proxy.");
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        ZLOGE("get samgr fail.");
        return nullptr;
    }

    auto remote = samgr->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    instance->kvDataServiceProxy_ = iface_cast<DataMgrServiceProxy>(remote);
    if (instance->kvDataServiceProxy_ == nullptr) {
        ZLOGE("initialize proxy failed.");
        return nullptr;
    }

    if (instance->deathRecipientPtr_ == nullptr) {
        instance->deathRecipientPtr_ = new (std::nothrow) ServiceDeathRecipient();
        if (instance->deathRecipientPtr_ == nullptr) {
            ZLOGW("new KvStoreDeathRecipient failed");
            return nullptr;
        }
    }
    if ((remote->IsProxyObject()) && (!remote->AddDeathRecipient(instance->deathRecipientPtr_))) {
        ZLOGE("failed to add death recipient.");
    }

    instance->RegisterClientDeathObserver();

    return instance->kvDataServiceProxy_;
}

void KvStoreServiceDeathNotifier::RegisterClientDeathObserver()
{
    if (kvDataServiceProxy_ == nullptr) {
        return;
    }
    if (clientDeathObserverPtr_ == nullptr) {
        clientDeathObserverPtr_ = new (std::nothrow) KvStoreClientDeathObserver();
    }
    if (clientDeathObserverPtr_ == nullptr) {
        ZLOGW("new KvStoreClientDeathObserver failed");
        return;
    }
    kvDataServiceProxy_->RegisterClientDeathObserver(GetAppId(), clientDeathObserverPtr_);
}

void KvStoreServiceDeathNotifier::AddServiceDeathWatcher(std::shared_ptr<KvStoreDeathRecipient> watcher)
{
    auto *instance = GetInstance();
    if (instance == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lg(instance->watchMutex_);
    auto ret = instance->serviceDeathWatchers_.insert(std::move(watcher));
    if (ret.second) {
        ZLOGI("success set size: %zu", instance->serviceDeathWatchers_.size());
    } else {
        ZLOGE("failed set size: %zu", instance->serviceDeathWatchers_.size());
    }
}

void KvStoreServiceDeathNotifier::RemoveServiceDeathWatcher(std::shared_ptr<KvStoreDeathRecipient> watcher)
{
    auto *instance = GetInstance();
    if (instance == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lg(instance->watchMutex_);
    auto it = instance->serviceDeathWatchers_.find(std::move(watcher));
    if (it != instance->serviceDeathWatchers_.end()) {
        instance->serviceDeathWatchers_.erase(it);
        ZLOGI("find & erase set size: %zu", instance->serviceDeathWatchers_.size());
    } else {
        ZLOGE("no found set size: %zu", instance->serviceDeathWatchers_.size());
    }
}

void KvStoreServiceDeathNotifier::ServiceDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    ZLOGW("DistributedDataMgrService died.");
    auto *instance = GetInstance();
    if (instance == nullptr) {
        return;
    }
    // Need to do this with the lock held
    std::lock_guard<std::mutex> lg(instance->watchMutex_);
    instance->kvDataServiceProxy_ = nullptr;
    ZLOGI("watcher set size: %zu", instance->serviceDeathWatchers_.size());
    for (const auto &watcher : instance->serviceDeathWatchers_) {
        if (watcher == nullptr) {
            ZLOGI("watcher is nullptr");
            continue;
        }
        TaskExecutor::GetInstance().Execute([watcher] {
            watcher->OnRemoteDied();
        });
    }
}

KvStoreServiceDeathNotifier::ServiceDeathRecipient::ServiceDeathRecipient()
{
    ZLOGI("constructor.");
}

KvStoreServiceDeathNotifier::ServiceDeathRecipient::~ServiceDeathRecipient()
{
    ZLOGI("destructor.");
}
} // namespace DistributedKv
} // namespace OHOS
