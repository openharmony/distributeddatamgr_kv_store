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

namespace AniObserverUtils {

bool IsSameTaiheCallback(TaiheVariantCallbackType callbackType1, TaiheVariantCallbackType callbackType2)
{
    if (std::holds_alternative<std::monostate>(callbackType1) && std::holds_alternative<std::monostate>(callbackType2)) {
        return true;
    }
    if (std::holds_alternative<JsDataChangeCallbackType>(callbackType1) &&
        std::holds_alternative<JsDataChangeCallbackType>(callbackType2)) {
        JsDataChangeCallbackType *callback1 = std::get_if<JsDataChangeCallbackType>(&callbackType1);
        JsDataChangeCallbackType *callback2 = std::get_if<JsDataChangeCallbackType>(&callbackType2);
        if (callback1 != nullptr && callback2 != nullptr) {
            return (*callback1 == *callback2);
        } else {
            return false;
        }
    }
    if (std::holds_alternative<JsSyncCompleteCallbackType>(callbackType1) &&
        std::holds_alternative<JsSyncCompleteCallbackType>(callbackType2)) {
        JsSyncCompleteCallbackType *callback1 = std::get_if<JsSyncCompleteCallbackType>(&callbackType1);    
        JsSyncCompleteCallbackType *callback2 = std::get_if<JsSyncCompleteCallbackType>(&callbackType2);
        if (callback1 != nullptr && callback2 != nullptr) {
            return (*callback1 == *callback2);
        } else {
            return false;
        }
    }
    if (std::holds_alternative<JsServiceDeathType>(callbackType1) &&
        std::holds_alternative<JsServiceDeathType>(callbackType2)) {
        JsServiceDeathType *callback1 = std::get_if<JsServiceDeathType>(&callbackType1);
        JsServiceDeathType *callback2 = std::get_if<JsServiceDeathType>(&callbackType2);
        if (callback1 != nullptr && callback2 != nullptr) {
            return (*callback1 == *callback2);
        } else {
            return false;
        }
    }
    return false;
}

AniDataChangeObserver::AniDataChangeObserver(TaiheVariantCallbackType callback)
{
    ZLOGI("AniDataChangeObserver");
    jsCallback_ = callback;
}

AniDataChangeObserver::~AniDataChangeObserver()
{
    ZLOGI("~AniDataChangeObserver");
}

void AniDataChangeObserver::Release()
{
    ZLOGI("AniDataChangeObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_ = std::monostate();
}

void AniDataChangeObserver::SetIsSchemaStore(bool isSchemaStore)
{
    isSchemaStore_ = isSchemaStore;
}

void AniDataChangeObserver::OnChange(const DistributedKv::ChangeNotification& info)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto pTaiheCallback = std::get_if<AniObserverUtils::JsDataChangeCallbackType>(&jsCallback_);
    if (pTaiheCallback == nullptr) {
        ZLOGE("pTaiheCallback == nullptr");
        return;
    }
    auto jspara = ani_kvstoreutils::KvChangeNotificationToTaihe(info, isSchemaStore_);
    (*pTaiheCallback)(jspara);
}

AniSyncCallback::AniSyncCallback(TaiheVariantCallbackType callback)
{
    ZLOGI("AniSyncCallback");
    jsCallback_ = callback;
    GetVm();
}

AniSyncCallback::~AniSyncCallback()
{
    ZLOGI("~AniSyncCallback");
}

void AniSyncCallback::GetVm()
{
    ani_env* env = taihe::get_env();
    if (env != nullptr) {
        env->GetVM(&vm_);
    }
    if (vm_ == nullptr) {
        ZLOGE("GetVm failed");
    }
}

void AniSyncCallback::Release()
{
    ZLOGI("AniSyncCallback::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_ = std::monostate();
}

void AniSyncCallback::SyncCompleted(const std::map<std::string, DistributedKv::Status>& results)
{
    if (vm_ == nullptr) {
        ZLOGW("SyncCompleted, vm_ nullptr");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto pTaiheCallback = std::get_if<AniObserverUtils::JsSyncCompleteCallbackType>(&jsCallback_);
    if (pTaiheCallback == nullptr) {
        ZLOGE("pTaiheCallback == nullptr");
        return;
    }
    ani_utils::AniExecuteFunc(vm_, [pTaiheCallback, &results](ani_env* aniEnv) {
        auto jspara = ani_kvstoreutils::KvStatusMapToTaiheArray(aniEnv, results);
        (*pTaiheCallback)(jspara);
    });
}

AniServiceDeathObserver::AniServiceDeathObserver(TaiheVariantCallbackType cb)
{
    ZLOGI("AniServiceDeathObserver");
    jsCallback_ = cb;
}

AniServiceDeathObserver::~AniServiceDeathObserver()
{
    ZLOGI("~AniServiceDeathObserver");
}

void AniServiceDeathObserver::Release()
{
    ZLOGI("AniServiceDeathObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_ = std::monostate();
}

void AniServiceDeathObserver::OnRemoteDied()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto pTaiheCallback = std::get_if<AniObserverUtils::JsServiceDeathType>(&jsCallback_);
    if (pTaiheCallback == nullptr) {
        ZLOGE("pTaiheCallback == nullptr");
        return;
    }
    auto undefined = ::ohos::data::distributedkvstore::OneUndef::make_UNDEFINED();
    (*pTaiheCallback)(undefined);
}

} // namespace AniObserverUtils