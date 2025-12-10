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
#include "ani_observer_utils.h"
#include "ani_kvstore_utils.h"
#include <algorithm>
#include <endian.h>

#include "log_print.h"
#include "cJSON.h"

using namespace ::ohos::data::distributedkvstore;
using namespace OHOS::DistributedKVStore;

namespace ani_observerutils {

std::shared_ptr<OHOS::AppExecFwk::EventHandler> JsBaseObserver::mainHandler_;
static std::once_flag g_handlerOnceFlag;

JsBaseObserver::JsBaseObserver(VarCallbackType cb, ani_ref jsCallbackRef)
    : jsCallback_(cb), jsCallbackRef_(jsCallbackRef)
{
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
    taihe::env_guard guard;
    if (auto *env = guard.get_env()) {
        env->GlobalReference_Delete(jsCallbackRef_);
        jsCallbackRef_ = nullptr;
    }
    jsCallback_ = std::monostate();
}

DataObserver::DataObserver(VarCallbackType cb, ani_ref jsCallbackRef)
    : JsBaseObserver(cb, jsCallbackRef)
{
    ZLOGI("DataObserver");
}

DataObserver::~DataObserver()
{
    ZLOGI("~DataObserver");
}

void DataObserver::SetIsSchemaStore(bool isSchemaStore)
{
    isSchemaStore_ = isSchemaStore;
}

void DataObserver::OnChange(const DistributedKv::ChangeNotification& info)
{
    auto sharedThis = shared_from_this();
    SendEventToMainThread([info, sharedThis] {
        sharedThis->OnChangeInMainThread(info);
    });
}

void DataObserver::OnChangeInMainThread(const DistributedKv::ChangeNotification& info)
{
    std::optional<ani_observerutils::JsDataChangeCallbackType> callbackTemp;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (jsCallbackRef_ == nullptr || std::holds_alternative<std::monostate>(jsCallback_)) {
            return;
        }
        auto pTaiheCallback = std::get_if<ani_observerutils::JsDataChangeCallbackType>(&jsCallback_);
        if (pTaiheCallback == nullptr) {
            return;
        }
        callbackTemp = *pTaiheCallback;
    }
    auto jspara = ani_kvstoreutils::KvChangeNotificationToTaihe(info, isSchemaStore_);
    if (callbackTemp.has_value()) {
        callbackTemp.value()(jspara);
    }
}

void DataObserver::SyncCompleted(const std::map<std::string, DistributedKv::Status>& results)
{
    auto sharedThis = shared_from_this();
    SendEventToMainThread([results, sharedThis] {
        sharedThis->SyncCompletedInMainThread(results);
    });
}

void DataObserver::SyncCompletedInMainThread(const std::map<std::string, DistributedKv::Status>& results)
{
    ani_env *env = taihe::get_env();
    if (env == nullptr) {
        return;
    }
    std::optional<ani_observerutils::JsSyncCompleteCallbackType> callbackTemp;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (jsCallbackRef_ == nullptr || std::holds_alternative<std::monostate>(jsCallback_)) {
            return;
        }
        auto pTaiheCallback = std::get_if<ani_observerutils::JsSyncCompleteCallbackType>(&jsCallback_);
        if (pTaiheCallback == nullptr) {
            return;
        }
        callbackTemp = *pTaiheCallback;
    }
    auto jspara = ani_kvstoreutils::KvStatusMapToTaiheArray(env, results);
    if (callbackTemp.has_value()) {
        callbackTemp.value()(jspara);
    }
}

ManagerObserver::ManagerObserver(VarCallbackType cb, ani_ref jsCallbackRef)
    : JsBaseObserver(cb, jsCallbackRef)
{
    ZLOGI("ManagerObserver");
}

ManagerObserver::~ManagerObserver()
{
    ZLOGI("~ManagerObserver");
}

void ManagerObserver::OnRemoteDied()
{
    auto sharedThis = shared_from_this();
    SendEventToMainThread([sharedThis] {
        sharedThis->OnRemoteDiedInMainThread();
    });
}

void ManagerObserver::OnRemoteDiedInMainThread()
{
    std::optional<ani_observerutils::JsServiceDeathType> callbackTemp;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (jsCallbackRef_ == nullptr || std::holds_alternative<std::monostate>(jsCallback_)) {
            return;
        }
        auto pTaiheCallback = std::get_if<ani_observerutils::JsServiceDeathType>(&jsCallback_);
        if (pTaiheCallback == nullptr) {
            return;
        }
        callbackTemp = *pTaiheCallback;
    }
    auto undefined = ::ohos::data::distributedkvstore::OneUndef::make_UNDEFINED();
    if (callbackTemp.has_value()) {
        callbackTemp.value()(undefined);
    }
}

} // namespace ani_observerutils