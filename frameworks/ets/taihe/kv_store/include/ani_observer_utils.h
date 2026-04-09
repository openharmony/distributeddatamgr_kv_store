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
#ifndef OHOS_ANI_OBSERVER_UTILS_H
#define OHOS_ANI_OBSERVER_UTILS_H
#include <string>
#include <optional>
#include <variant>

#include "taihe/runtime.hpp"
#include "ohos.data.distributedkvstore.proj.hpp"
#include "ohos.data.distributedkvstore.impl.hpp"
#include "event_handler.h"
#include "event_runner.h"
#include "single_kvstore.h"
#include "types.h"
#include "kvstore_observer.h"
#include "kvstore_sync_callback.h"
#include "kvstore_death_recipient.h"

namespace AniObserverUtils {

using namespace OHOS;

using JsDataChangeCallbackType = ::taihe::callback<void(::ohos::data::distributedkvstore::ChangeNotification const&)>;
using JsSyncCompleteCallbackType = ::taihe::callback<void(::taihe::array_view<uintptr_t>)>;
using JsServiceDeathType = ::taihe::callback<void(::ohos::data::distributedkvstore::OneUndef const&)>;

template<typename T>
bool IsSameTaiheCallback(T oldOptCallback, T newOptCallback)
{
    if (!oldOptCallback.has_value() && !newOptCallback.has_value()) {
        return true;
    }
    if (oldOptCallback.has_value() && newOptCallback.has_value()) {
        auto const& oldCallback = oldOptCallback.value();
        auto const& newCallback = newOptCallback.value();
        return oldCallback == newCallback;
    }
    return false;
}

class AniDataChangeObserver : public DistributedKv::KvStoreObserver {
public:
    AniDataChangeObserver(JsDataChangeCallbackType callback);
    std::optional<JsDataChangeCallbackType>& GetJsCallback() { return jsCallback_; }
    void Release();
    void SetIsSchemaStore(bool isSchemaStore);
    void OnChange(const DistributedKv::ChangeNotification& info) override;

protected:
    std::recursive_mutex mutex_;
    std::optional<JsDataChangeCallbackType> jsCallback_;
    bool isSchemaStore_ = false;
};

class AniSyncCallback : public DistributedKv::KvStoreSyncCallback {
public:
    AniSyncCallback(JsSyncCompleteCallbackType callback);
    void GetVm();
    std::optional<JsSyncCompleteCallbackType>& GetJsCallback() { return jsCallback_; }
    void Release();
    void SyncCompleted(const std::map<std::string, DistributedKv::Status>& results) override;

protected:
    std::recursive_mutex mutex_;
    std::optional<JsSyncCompleteCallbackType> jsCallback_;
    ani_vm* vm_ = nullptr;
};

class AniServiceDeathObserver : public DistributedKv::KvStoreDeathRecipient {
public:
    AniServiceDeathObserver(JsServiceDeathType callback);
    std::optional<JsServiceDeathType>& GetJsCallback() { return jsCallback_; }
    void Release();
    void OnRemoteDied() override;

private:
    std::recursive_mutex mutex_;
    std::optional<JsServiceDeathType> jsCallback_;
};

}  // namespace AniObserverUtils
#endif  // OHOS_ANI_OBSERVER_UTILS_H
