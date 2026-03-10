/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "AniObserverUtils"
#include "ani_error_utils.h"
#include "ani_observer_utils.h"
#include "ani_kvstore_utils.h"
#include "ani_utils.h"
#include <algorithm>
#include <endian.h>

#include "log_print.h"

using namespace ::ohos::data::distributedkvstore;
using namespace OHOS::DistributedKVStore;

namespace ani_observerutils {

bool IsSameTaiheCallback(VarCallbackType p1, VarCallbackType p2)
{
    if (std::holds_alternative<std::monostate>(p1) && std::holds_alternative<std::monostate>(p2)) {
        return true;
    }
    if (std::holds_alternative<JsDataChangeCallbackType>(p1) &&
        std::holds_alternative<JsDataChangeCallbackType>(p2)) {
        JsDataChangeCallbackType *p1val = std::get_if<JsDataChangeCallbackType>(&p1);
        JsDataChangeCallbackType *p2val = std::get_if<JsDataChangeCallbackType>(&p2);
        if (p1val != nullptr && p2val != nullptr) {
            return (*p1val == *p2val);
        } else {
            return false;
        }
    }
    if (std::holds_alternative<JsSyncCompleteCallbackType>(p1) &&
        std::holds_alternative<JsSyncCompleteCallbackType>(p2)) {
        JsSyncCompleteCallbackType *p1val = std::get_if<JsSyncCompleteCallbackType>(&p1);
        JsSyncCompleteCallbackType *p2val = std::get_if<JsSyncCompleteCallbackType>(&p2);
        if (p1val != nullptr && p2val != nullptr) {
            return (*p1val == *p2val);
        } else {
            return false;
        }
    }
    if (std::holds_alternative<JsServiceDeathType>(p1) &&
        std::holds_alternative<JsServiceDeathType>(p2)) {
        JsServiceDeathType *p1val = std::get_if<JsServiceDeathType>(&p1);
        JsServiceDeathType *p2val = std::get_if<JsServiceDeathType>(&p2);
        if (p1val != nullptr && p2val != nullptr) {
            return (*p1val == *p2val);
        } else {
            return false;
        }
    }
    return false;
}

ChangeObserver::ChangeObserver(VarCallbackType callback)
{
    ZLOGI("ChangeObserver");
    jsCallback_ = callback;
}

ChangeObserver::~ChangeObserver()
{
    ZLOGI("~ChangeObserver");
}

void ChangeObserver::Release()
{
    ZLOGI("ChangeObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_ = std::monostate();
}

void ChangeObserver::SetIsSchemaStore(bool isSchemaStore)
{
    isSchemaStore_ = isSchemaStore;
}

void ChangeObserver::OnChange(const DistributedKv::ChangeNotification& info)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto pTaiheCallback = std::get_if<ani_observerutils::JsDataChangeCallbackType>(&jsCallback_);
    if (pTaiheCallback == nullptr) {
        ZLOGE("pTaiheCallback == nullptr");
        return;
    }
    auto jspara = ani_kvstoreutils::KvChangeNotificationToTaihe(info, isSchemaStore_);
    (*pTaiheCallback)(jspara);
}

SyncObserver::SyncObserver(VarCallbackType callback)
{
    ZLOGI("SyncObserver");
    jsCallback_ = callback;
    GetVm();
}

SyncObserver::~SyncObserver()
{
    ZLOGI("~SyncObserver");
}

void SyncObserver::GetVm()
{
    ani_env* env = taihe::get_env();
    if (env != nullptr) {
        env->GetVM(&vm_);
    }
    if (vm_ == nullptr) {
        ZLOGE("GetVm failed");
    }
}

void SyncObserver::Release()
{
    ZLOGI("SyncObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_ = std::monostate();
}

void SyncObserver::SyncCompleted(const std::map<std::string, DistributedKv::Status>& results)
{
    if (vm_ == nullptr) {
        ZLOGW("SyncCompleted, vm_ nullptr");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto pTaiheCallback = std::get_if<ani_observerutils::JsSyncCompleteCallbackType>(&jsCallback_);
    if (pTaiheCallback == nullptr) {
        ZLOGE("pTaiheCallback == nullptr");
        return;
    }
    ani_utils::AniExecuteFunc(vm_, [pTaiheCallback, &results](ani_env* aniEnv) {
        auto jspara = ani_kvstoreutils::KvStatusMapToTaiheArray(aniEnv, results);
        (*pTaiheCallback)(jspara);
    });
}

DataObserverMgr::DataObserverMgr(std::shared_ptr<OHOS::DistributedKv::SingleKvStore> kvstore, bool isSchemaStore)
{
    nativeStore_ = kvstore;
    isSchemaStore_ = isSchemaStore;
}

DataObserverMgr::~DataObserverMgr()
{
}

bool DataObserverMgr::IsCallbackDuplicated(std::string const& event, ani_observerutils::VarCallbackType const& callback)
{
    bool isDuplicate = false;
    if (event == DATA_CHANGE_EVENT) {
        std::lock_guard<std::recursive_mutex> lock(changeMapMutex_);
        isDuplicate = std::any_of(jsChangeMap_.begin(), jsChangeMap_.end(),
            [&callback](std::shared_ptr<ani_observerutils::ChangeObserver> const& item) {
                if (item == nullptr) {
                    return false;
                }
                return IsSameTaiheCallback(callback, item->GetJsCallback());
            });
    } else if (event == SYNC_COMPLETE_EVENT) {
        std::lock_guard<std::recursive_mutex> lock(syncMapMutex_);
        isDuplicate = std::any_of(jsSyncMap_.begin(), jsSyncMap_.end(),
            [&callback](std::shared_ptr<ani_observerutils::SyncObserver> const& item) {
                if (item == nullptr) {
                    return false;
                }
                return IsSameTaiheCallback(callback, item->GetJsCallback());
            });
    }
    return isDuplicate;
}

void DataObserverMgr::RegisterListener(std::string const& event, DistributedKv::SubscribeType type,
    ani_observerutils::VarCallbackType &callbackParam)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("RegisterListener, nativeStore_ is nullptr");
        return;
    }
    bool isDuplicate = IsCallbackDuplicated(event, callbackParam);
    if (isDuplicate) {
        ZLOGW("Failed to register, duplicated, %{public}s", event.c_str());
        return;
    }
    DistributedKv::Status status = DistributedKv::Status::SUCCESS;
    if (event == DATA_CHANGE_EVENT) {
        status = RegisterChangeObserver(type, callbackParam);
    } else if (event == SYNC_COMPLETE_EVENT) {
        status = RegisterSyncCompleteObserver(callbackParam);
    }
    if (status != DistributedKv::Status::SUCCESS) {
        ThrowAniError(status, "");
        return;
    }
    ZLOGI("RegisterListener success type: %{public}s", event.c_str());
}

void DataObserverMgr::UnregisterListener(std::string const& event,
    ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnregisterListener, nativeStore_ is nullptr");
        return;
    }
    if (event == SYNC_COMPLETE_EVENT) {
        DistributedKv::SubscribeType kvtype = DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL;
        Status status = UnRegisterSyncCompleteObserver(kvtype, optCallback, isUpdateFlag);
        if (status != Status::SUCCESS) {
            ZLOGE("UnregisterListener syncComplete failed, status %{public}d", status);
            ThrowAniError(status, "");
        }
        return;
    }
    for (uint8_t type = ani_kvstoreutils::SUBSCRIBE_LOCAL; type < ani_kvstoreutils::SUBSCRIBE_COUNT; type++) {
        bool updated = false;
        DistributedKv::SubscribeType kvtype = ani_kvstoreutils::SubscribeTypeToNative(type);
        DistributedKv::Status status = UnRegisterChangeObserver(kvtype, optCallback, updated);
        isUpdateFlag |= updated;
        if (status != DistributedKv::Status::SUCCESS) {
            ZLOGE("UnregisterListener failed, type = %{public}d, status %{public}d", type, status);
            ThrowAniError(status, "");
            return;
        }
    }
    ZLOGI("UnregisterListener success type:%{public}s", event.c_str());
}

DistributedKv::Status DataObserverMgr::RegisterChangeObserver(DistributedKv::SubscribeType type,
    ani_observerutils::VarCallbackType &callbackParam)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("RegisterDataChangeObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }

    std::lock_guard<std::recursive_mutex> lock(changeMapMutex_);
    auto observer = std::make_shared<ani_observerutils::ChangeObserver>(callbackParam);
    observer->SetIsSchemaStore(isSchemaStore_);
    DistributedKv::Status status = nativeStore_->SubscribeKvStore(type, observer);
    if (status != DistributedKv::Status::SUCCESS) {
        ZLOGE("RegisterDataChangeObserver, SubscribeKvStore failed, %{public}d", status);
        observer->Release();
        return status;
    }
    jsChangeMap_.emplace_back(std::move(observer));
    return Status::SUCCESS;
}

DistributedKv::Status DataObserverMgr::RegisterSyncCompleteObserver(ani_observerutils::VarCallbackType &callbackParam)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("RegisterSyncCompleteObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }
    std::lock_guard<std::recursive_mutex> lock(syncMapMutex_);
    auto observer = std::make_shared<ani_observerutils::SyncObserver>(callbackParam);
    DistributedKv::Status status = nativeStore_->RegisterSyncCallback(observer);
    if (status != DistributedKv::Status::SUCCESS) {
        ZLOGE("RegisterSyncCompleteObserver, RegisterSyncCallback failed, %{public}d", status);
        observer->Release();
        return status;
    }
    jsSyncMap_.emplace_back(std::move(observer));
    return Status::SUCCESS;
}

DistributedKv::Status DataObserverMgr::UnRegisterChangeObserver(DistributedKv::SubscribeType type,
    ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnRegisterChangeObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }
    std::lock_guard<std::recursive_mutex> lock(changeMapMutex_);
    if (jsChangeMap_.empty()) {
        return Status::SUCCESS;
    }
    Status result = Status::SUCCESS;
    for (auto iter = jsChangeMap_.begin(); iter != jsChangeMap_.end();) {
        auto item = *iter;
        if (item == nullptr) {
            iter++;
            continue;
        }
        if (optCallback.has_value() && !IsSameTaiheCallback(optCallback.value(), item->GetJsCallback())) {
            iter++;
            continue;
        }
        result = nativeStore_->UnSubscribeKvStore(type, item);
        if (result == DistributedKv::Status::SUCCESS || result == DistributedKv::Status::ALREADY_CLOSED) {
            isUpdateFlag = true;
            (*iter)->Release();
            iter = jsChangeMap_.erase(iter);
        } else {
            ZLOGE("DataObserverMgr UnRegisterChangeObserver failed, status %{public}d", result);
            break;
        }
    }
    return result;
}

DistributedKv::Status DataObserverMgr::UnRegisterSyncCompleteObserver(DistributedKv::SubscribeType type,
    ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnRegisterSyncCompleteObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }
    std::lock_guard<std::recursive_mutex> lock(syncMapMutex_);
    if (jsSyncMap_.empty()) {
        return Status::SUCCESS;
    }
    Status result = Status::SUCCESS;
    for (auto iter = jsSyncMap_.begin(); iter != jsSyncMap_.end();) {
        auto item = *iter;
        if (item == nullptr) {
            iter++;
            continue;
        }
        if (optCallback.has_value() && !IsSameTaiheCallback(optCallback.value(), item->GetJsCallback())) {
            iter++;
            continue;
        }
        isUpdateFlag = true;
        (*iter)->Release();
        iter = jsSyncMap_.erase(iter);
    }
    if (jsSyncMap_.empty()) {
        result = nativeStore_->UnRegisterSyncCallback();
        ZLOGI("UnRegisterSyncCompleteObserver, unregister native, %{public}d", result);
    }
    return result;
}

void DataObserverMgr::UnRegisterAll()
{
    ZLOGI("DataObserverMgr UnRegisterAll");
    ::taihe::optional<ani_observerutils::VarCallbackType> empty;
    bool isUpdated = false;
    {
        std::lock_guard<std::recursive_mutex> lock(changeMapMutex_);
        for (uint8_t type = ani_kvstoreutils::SUBSCRIBE_LOCAL; type < ani_kvstoreutils::SUBSCRIBE_COUNT; type++) {
            DistributedKv::SubscribeType kvtype = ani_kvstoreutils::SubscribeTypeToNative(type);
            UnRegisterChangeObserver(kvtype, empty, isUpdated);
        }
    }
    std::lock_guard<std::recursive_mutex> lock(syncMapMutex_);
    UnRegisterSyncCompleteObserver(DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL, empty, isUpdated);
}

ManagerObserver::ManagerObserver(VarCallbackType cb)
{
    ZLOGI("ManagerObserver");
    jsCallback_ = cb;
}

ManagerObserver::~ManagerObserver()
{
    ZLOGI("~ManagerObserver");
}

void ManagerObserver::Release()
{
    ZLOGI("ManagerObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_ = std::monostate();
}

void ManagerObserver::OnRemoteDied()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto pTaiheCallback = std::get_if<ani_observerutils::JsServiceDeathType>(&jsCallback_);
    if (pTaiheCallback == nullptr) {
        ZLOGE("pTaiheCallback == nullptr");
        return;
    }
    auto undefined = ::ohos::data::distributedkvstore::OneUndef::make_UNDEFINED();
    (*pTaiheCallback)(undefined);
}

} // namespace ani_observerutils