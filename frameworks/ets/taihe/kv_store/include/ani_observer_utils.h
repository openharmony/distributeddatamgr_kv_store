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

namespace ani_observerutils {

using namespace OHOS;

using JsDataChangeCallbackType = ::taihe::callback<void(::ohos::data::distributedkvstore::ChangeNotification const&)>;
using JsSyncCompleteCallbackType = ::taihe::callback<void(::taihe::array_view<uintptr_t>)>;
using JsServiceDeathType = ::taihe::callback<void(::ohos::data::distributedkvstore::OneUndef const&)>;
using VarCallbackType = std::variant<
    std::monostate,
    ::taihe::callback<void(::ohos::data::distributedkvstore::OneUndef const&)>,
    ::taihe::callback<void(::taihe::array_view<uintptr_t>)>,
    ::taihe::callback<void(::ohos::data::distributedkvstore::ChangeNotification const&)>>;

bool IsSameTaiheCallback(VarCallbackType p1, VarCallbackType p2);

class ChangeObserver : public DistributedKv::KvStoreObserver {
public:
    ChangeObserver(VarCallbackType callback);
    ~ChangeObserver();
    VarCallbackType& GetJsCallback() { return jsCallback_; }
    void Release();
    void SetIsSchemaStore(bool isSchemaStore);
    void OnChange(const DistributedKv::ChangeNotification& info) override;

protected:
    std::recursive_mutex mutex_;
    VarCallbackType jsCallback_;
    bool isSchemaStore_ = false;
};

class SyncObserver : public DistributedKv::KvStoreSyncCallback {
public:
    SyncObserver(VarCallbackType callback);
    ~SyncObserver();
    void GetVm();
    VarCallbackType& GetJsCallback() { return jsCallback_; }
    void Release();
    void SyncCompleted(const std::map<std::string, DistributedKv::Status>& results) override;

protected:
    std::recursive_mutex mutex_;
    VarCallbackType jsCallback_;
    ani_vm* vm_ = nullptr;
};

class DataObserverMgr {
public:
    DataObserverMgr(std::shared_ptr<DistributedKv::SingleKvStore> kvstore, bool isSchemaStore);
    ~DataObserverMgr();

    bool IsCallbackDuplicated(std::string const& event, ani_observerutils::VarCallbackType const& callback);
    void RegisterListener(std::string const& event,
        DistributedKv::SubscribeType type, ani_observerutils::VarCallbackType &callbackParam);
    void UnregisterListener(std::string const& event,
        ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag);
    DistributedKv::Status RegisterChangeObserver(DistributedKv::SubscribeType type,
        ani_observerutils::VarCallbackType &cb);
    DistributedKv::Status RegisterSyncCompleteObserver(ani_observerutils::VarCallbackType &cb);
    DistributedKv::Status UnRegisterChangeObserver(DistributedKv::SubscribeType type,
        ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag);
    DistributedKv::Status UnRegisterSyncCompleteObserver(DistributedKv::SubscribeType type,
        ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag);
    void UnRegisterAll();

private:
    bool isSchemaStore_ = false;
    std::shared_ptr<DistributedKv::SingleKvStore> nativeStore_;
    std::recursive_mutex changeMapMutex_;
    std::vector<std::shared_ptr<ani_observerutils::ChangeObserver>> jsChangeMap_;
    std::recursive_mutex syncMapMutex_;
    std::vector<std::shared_ptr<ani_observerutils::SyncObserver>> jsSyncMap_;
};

class ManagerObserver : public DistributedKv::KvStoreDeathRecipient {
public:
    ManagerObserver(VarCallbackType cb);
    ~ManagerObserver();
    VarCallbackType& GetJsCallback() { return jsCallback_; }
    void Release();
    void OnRemoteDied() override;

private:
    std::recursive_mutex mutex_;
    VarCallbackType jsCallback_;
};

}  // namespace ani_observerutils
#endif  // OHOS_ANI_OBSERVER_UTILS_H
