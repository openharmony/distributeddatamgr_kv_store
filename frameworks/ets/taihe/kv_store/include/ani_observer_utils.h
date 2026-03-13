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
#include "taihe/runtime.hpp"
#include "ohos.data.distributedkvstore.proj.hpp"
#include "ohos.data.distributedkvstore.impl.hpp"
#include <string>
#include <optional>
#include <variant>

#include "event_handler.h"
#include "event_runner.h"
#include "single_kvstore.h"
#include "types.h"
#include "kvstore_observer.h"
#include "kvstore_sync_callback.h"
#include "kvstore_death_recipient.h"

static constexpr const char* DATA_CHANGE_EVENT = "dataChange";
static constexpr const char* SYNC_COMPLETE_EVENT = "syncComplete";

namespace AniObserverUtils {

using namespace OHOS;

using JsDataChangeCallbackType = ::taihe::callback<void(::ohos::data::distributedkvstore::ChangeNotification const&)>;
using JsSyncCompleteCallbackType = ::taihe::callback<void(::taihe::array_view<uintptr_t>)>;
using JsServiceDeathType = ::taihe::callback<void(::ohos::data::distributedkvstore::OneUndef const&)>;
using TaiheVariantCallbackType = std::variant<std::monostate,
    JsServiceDeathType, JsSyncCompleteCallbackType, JsDataChangeCallbackType>;

bool IsSameTaiheCallback(TaiheVariantCallbackType callbackType1, TaiheVariantCallbackType callbackType2);

class AniDataChangeObserver : public DistributedKv::KvStoreObserver {
public:
    AniDataChangeObserver(TaiheVariantCallbackType callback);
    ~AniDataChangeObserver();
    TaiheVariantCallbackType& GetJsCallback() { return jsCallback_; }
    void Release();
    void SetIsSchemaStore(bool isSchemaStore);
    void OnChange(const DistributedKv::ChangeNotification& info) override;

protected:
    std::recursive_mutex mutex_;
    TaiheVariantCallbackType jsCallback_;
    bool isSchemaStore_ = false;
};

class AniSyncCallback : public DistributedKv::KvStoreSyncCallback {
public:
    AniSyncCallback(TaiheVariantCallbackType callback);
    ~AniSyncCallback();
    void GetVm();
    TaiheVariantCallbackType& GetJsCallback() { return jsCallback_; }
    void Release();
    void SyncCompleted(const std::map<std::string, DistributedKv::Status>& results) override;

protected:
    std::recursive_mutex mutex_;
    TaiheVariantCallbackType jsCallback_;
    ani_vm* vm_ = nullptr;
};

class AniServiceDeathObserver : public DistributedKv::KvStoreDeathRecipient {
public:
    AniServiceDeathObserver(TaiheVariantCallbackType cb);
    ~AniServiceDeathObserver();
    TaiheVariantCallbackType& GetJsCallback() { return jsCallback_; }
    void Release();
    void OnRemoteDied() override;

private:
    std::recursive_mutex mutex_;
    TaiheVariantCallbackType jsCallback_;
};

}  // namespace AniObserverUtils
#endif  // OHOS_ANI_OBSERVER_UTILS_H
