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
#include "log_print.h"

using namespace ::ohos::data::distributedkvstore;
using namespace OHOS::DistributedKVStore;

namespace AniObserverUtils {

AniDataChangeObserver::AniDataChangeObserver(JsDataChangeCallbackType callback)
{
    ZLOGI("AniDataChangeObserver");
    jsCallback_ = callback;
}

void AniDataChangeObserver::Release()
{
    ZLOGI("AniDataChangeObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_.reset();
}

void AniDataChangeObserver::SetIsSchemaStore(bool isSchemaStore)
{
    isSchemaStore_ = isSchemaStore;
}

void AniDataChangeObserver::OnChange(const DistributedKv::ChangeNotification& info)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!jsCallback_.has_value()) {
        ZLOGE("jsCallback_ == nullptr");
        return;
    }
    auto &taiheCallback = jsCallback_.value();
    auto jspara = ani_kvstoreutils::KvChangeNotificationToTaihe(info, isSchemaStore_);
    taiheCallback(jspara);
}

AniSyncCallback::AniSyncCallback(JsSyncCompleteCallbackType callback)
{
    ZLOGI("AniSyncCallback");
    jsCallback_ = callback;
    GetVm();
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
    jsCallback_.reset();
}

void AniSyncCallback::SyncCompleted(const std::map<std::string, DistributedKv::Status>& results)
{
    if (vm_ == nullptr) {
        ZLOGW("SyncCompleted, vm_ nullptr");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!jsCallback_.has_value()) {
        ZLOGE("jsCallback_ == nullptr");
        return;
    }
    auto &taiheCallback = jsCallback_.value();
    ani_utils::AniExecuteFunc(vm_, [taiheCallback, &results](ani_env* aniEnv) {
        auto jspara = ani_kvstoreutils::KvStatusMapToTaiheArray(aniEnv, results);
        taiheCallback(jspara);
    });
}

AniServiceDeathObserver::AniServiceDeathObserver(JsServiceDeathType callback)
{
    ZLOGI("AniServiceDeathObserver");
    jsCallback_ = callback;
}

void AniServiceDeathObserver::Release()
{
    ZLOGI("AniServiceDeathObserver::Release");
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    jsCallback_.reset();
}

void AniServiceDeathObserver::OnRemoteDied()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!jsCallback_.has_value()) {
        ZLOGE("jsCallback_ == nullptr");
        return;
    }
    auto &taiheCallback = jsCallback_.value();
    auto undefined = ::ohos::data::distributedkvstore::OneUndef::make_UNDEFINED();
    taiheCallback(undefined);
}

} // namespace AniObserverUtils