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

std::shared_ptr<OHOS::AppExecFwk::EventHandler> JsBaseObserver::mainHandler_;
static std::once_flag g_handlerOnceFlag;

JsBaseObserver::JsBaseObserver(VarCallbackType cb)
    : jsCallback_(cb)
{
}

VarCallbackType& JsBaseObserver::GetJsCallback()
{
    return jsCallback_;
}

bool JsBaseObserver::IsSameTaiheCallback(VarCallbackType p1, VarCallbackType p2)
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

bool JsBaseObserver::SendEventToMainThread(const std::function<void()> func)
{
    if (func == nullptr) {
        return false;
    }
    std::call_once(g_handlerOnceFlag, [] {
        auto runner = OHOS::AppExecFwk::EventRunner::GetMainEventRunner();
        if (runner) {
            mainHandler_ = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
        }
    });
    if (!mainHandler_) {
        ZLOGE("Failed to initialize event handler");
        return false;
    }
    mainHandler_->PostTask(func, "", 0, OHOS::AppExecFwk::EventQueue::Priority::IMMEDIATE, {});
    return true;
}

void JsBaseObserver::Release()
{
    ZLOGI("DataObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_ = std::monostate();
}

DataObserver::DataObserver(VarCallbackType callback)
    : JsBaseObserver(callback)
{
    ZLOGI("DataObserver");
    GetVm();
}

DataObserver::~DataObserver()
{
    ZLOGI("~DataObserver");
}

void DataObserver::GetVm()
{
    ani_env* env = taihe::get_env();
    if (env != nullptr) {
        env->GetVM(&vm_);
    }
    if (vm_ == nullptr) {
        ZLOGE("GetVm failed");
    }
}

void DataObserver::SetIsSchemaStore(bool isSchemaStore)
{
    isSchemaStore_ = isSchemaStore;
}

void DataObserver::OnChange(const DistributedKv::ChangeNotification& info)
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

void DataObserver::SyncCompleted(const std::map<std::string, DistributedKv::Status>& results)
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

void DataObserverMgr::RegisterListener(std::string const& event, DistributedKv::SubscribeType type,
    ani_observerutils::VarCallbackType &callbackParam)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
    auto &cbVec = jsCbMap_[event];
    bool isDuplicate = std::any_of(cbVec.begin(), cbVec.end(),
        [&callbackParam](std::shared_ptr<ani_observerutils::DataObserver> const& item) {
            if (item == nullptr) {
                return false;
            }
            return JsBaseObserver::IsSameTaiheCallback(callbackParam, item->GetJsCallback());
        });
    if (isDuplicate) {
        ZLOGW("Failed to register duplicated, %{public}s", event.c_str());
        return;
    }
    DistributedKv::Status status = DistributedKv::Status::SUCCESS;
    if (event == DATA_CHANGE_EVENT) {
        status = RegisterDataChangeObserver(type, callbackParam);
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
        ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
    const auto iter = jsCbMap_.find(event);
    if (iter == jsCbMap_.end()) {
        ZLOGE("%{public}s is not registered", event.c_str());
        return;
    }
    if (event == SYNC_COMPLETE_EVENT) {
        DistributedKv::SubscribeType kvtype = DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL;
        Status status = UnRegisterObserver(event, kvtype, optCallback, isUpdateFlag);
        if (status != Status::SUCCESS) {
            ZLOGE("UnregisterListener syncComplete failed, status %{public}d", status);
            ThrowAniError(status, "");
        }
        return;
    }
    for (uint8_t type = ani_kvstoreutils::SUBSCRIBE_LOCAL; type < ani_kvstoreutils::SUBSCRIBE_COUNT; type++) {
        bool updated = false;
        DistributedKv::SubscribeType kvtype = ani_kvstoreutils::SubscribeTypeToNative(type);
        DistributedKv::Status status = UnRegisterObserver(event, kvtype, optCallback, updated);
        isUpdateFlag |= updated;
        if (status != DistributedKv::Status::SUCCESS) {
            ZLOGE("UnregisterListener failed, type = %{public}d, status %{public}d", type, status);
            ThrowAniError(status, "");
            return;
        }
    }
    ZLOGI("UnregisterListener success type:%{public}s", event.c_str());
}

DistributedKv::Status DataObserverMgr::RegisterDataChangeObserver(DistributedKv::SubscribeType type,
    ani_observerutils::VarCallbackType &callbackParam)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }

    std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
    auto &cbVec = jsCbMap_[DATA_CHANGE_EVENT];
    auto observer = std::make_shared<ani_observerutils::DataObserver>(callbackParam);
    observer->SetIsSchemaStore(isSchemaStore_);
    DistributedKv::Status status = nativeStore_->SubscribeKvStore(type, observer);
    if (status != DistributedKv::Status::SUCCESS) {
        ZLOGE("RegisterDataChangeObserver, SubscribeKvStore failed, %{public}d", status);
        observer->Release();
        return status;
    }
    cbVec.emplace_back(std::move(observer));
    return Status::SUCCESS;
}

DistributedKv::Status DataObserverMgr::RegisterSyncCompleteObserver(ani_observerutils::VarCallbackType &callbackParam)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }
    std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
    auto &cbVec = jsCbMap_[SYNC_COMPLETE_EVENT];
    auto observer = std::make_shared<ani_observerutils::DataObserver>(callbackParam);
    observer->SetIsSchemaStore(isSchemaStore_);
    DistributedKv::Status status = nativeStore_->RegisterSyncCallback(observer);
    if (status != DistributedKv::Status::SUCCESS) {
        ZLOGE("RegisterSyncCompleteObserver, RegisterSyncCallback failed, %{public}d", status);
        observer->Release();
        return status;
    }
    cbVec.emplace_back(std::move(observer));
    return Status::SUCCESS;
}

DistributedKv::Status DataObserverMgr::UnRegisterObserver(std::string const& event, DistributedKv::SubscribeType type,
    ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }
    std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
    Status result = Status::SUCCESS;
    auto &callbackList = jsCbMap_[event];
    if (!optCallback.has_value()) {
        for (auto iter = callbackList.begin(); iter != callbackList.end();) {
            result = DistributedKv::Status::SUCCESS;
            if (event == DATA_CHANGE_EVENT) {
                result = nativeStore_->UnSubscribeKvStore(type, *iter);
            }
            if (result == DistributedKv::Status::SUCCESS || result == DistributedKv::Status::ALREADY_CLOSED) {
                isUpdateFlag = true;
                (*iter)->Release();
                iter = callbackList.erase(iter);
            } else {
                ZLOGE("DataObserverMgr UnRegisterObserver failed, status %{public}d", result);
                break;
            }
        }
        if (callbackList.empty()) {
            if (event == SYNC_COMPLETE_EVENT) {
                result = nativeStore_->UnRegisterSyncCallback();
                ZLOGI("UnRegisterObserver syncComplete, unregister native, %{public}d", result);
            }
            jsCbMap_.erase(event);
        }
        return result;
    }
    return UnRegisterObserver(event, type, optCallback.value(), isUpdateFlag);
}
 
DistributedKv::Status DataObserverMgr::UnRegisterObserver(std::string const& event, DistributedKv::SubscribeType type,
    ani_observerutils::VarCallbackType callbackParam, bool &isUpdateFlag)
{
    if (nativeStore_ == nullptr) {
        ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
        return Status::SUCCESS;
    }
    std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
    auto &callbackList = jsCbMap_[event];
    const auto pred = [&callbackParam](std::shared_ptr<ani_observerutils::DataObserver> &item) {
        if (item == nullptr) {
            return false;
        }
        return JsBaseObserver::IsSameTaiheCallback(callbackParam, item->GetJsCallback());
    };
    const auto it = std::find_if(callbackList.begin(), callbackList.end(), pred);
    DistributedKv::Status result = DistributedKv::Status::SUCCESS;
    if (it != callbackList.end()) {
        if (event == DATA_CHANGE_EVENT) {
            result = nativeStore_->UnSubscribeKvStore(type, *it);
        }
        if (result == DistributedKv::Status::SUCCESS || result == DistributedKv::Status::ALREADY_CLOSED) {
            isUpdateFlag = true;
            (*it)->Release();
            callbackList.erase(it);
        } else {
            return result;
        }
    }
    if (callbackList.empty()) {
        if (event == SYNC_COMPLETE_EVENT) {
            result = nativeStore_->UnRegisterSyncCallback();
            ZLOGI("UnRegisterObserver syncComplete, unregister native, %{public}d", result);
        }
        jsCbMap_.erase(event);
    }
    return result;
}

void DataObserverMgr::UnRegisterAll()
{
    ZLOGI("DataObserverMgr UnRegisterAll");
    std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
    bool isUpdated = false;
    ::taihe::optional<ani_observerutils::VarCallbackType> empty;
    for (uint8_t type = ani_kvstoreutils::SUBSCRIBE_LOCAL; type < ani_kvstoreutils::SUBSCRIBE_COUNT; type++) {
        DistributedKv::SubscribeType kvtype = ani_kvstoreutils::SubscribeTypeToNative(type);
        UnRegisterObserver(DATA_CHANGE_EVENT, kvtype, empty, isUpdated);
    }
    UnRegisterObserver(SYNC_COMPLETE_EVENT, DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL, empty, isUpdated);
}

ManagerObserver::ManagerObserver(VarCallbackType cb)
    : JsBaseObserver(cb)
{
    ZLOGI("ManagerObserver");
}

ManagerObserver::~ManagerObserver()
{
    ZLOGI("~ManagerObserver");
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