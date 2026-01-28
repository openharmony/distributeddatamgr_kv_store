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

class JsBaseObserver {
public:
    JsBaseObserver(VarCallbackType cb);
    virtual ~JsBaseObserver() {}
    VarCallbackType& GetJsCallback();
    static bool IsSameTaiheCallback(VarCallbackType p1, VarCallbackType p2);
    bool SendEventToMainThread(const std::function<void()> func);
    void Release();

protected:
    std::recursive_mutex mutex_;
    VarCallbackType jsCallback_;
    static std::shared_ptr<OHOS::AppExecFwk::EventHandler> mainHandler_;
};

class DataObserver : public JsBaseObserver,
    public DistributedKv::KvStoreObserver, public DistributedKv::KvStoreSyncCallback,
    public std::enable_shared_from_this<DataObserver> {
public:
    DataObserver(VarCallbackType callback);
    ~DataObserver();
    void GetVm();
    void SetIsSchemaStore(bool isSchemaStore);
    void OnChange(const DistributedKv::ChangeNotification& info) override;
    void SyncCompleted(const std::map<std::string, DistributedKv::Status>& results) override;

protected:
    bool isSchemaStore_ = false;
    ani_vm* vm_ = nullptr;
};

class DataObserverMgr {
public:
    DataObserverMgr(std::shared_ptr<DistributedKv::SingleKvStore> kvstore, bool isSchemaStore);
    ~DataObserverMgr();

    void RegisterListener(std::string const& event,
        DistributedKv::SubscribeType type, ani_observerutils::VarCallbackType &callbackParam);
    void UnregisterListener(std::string const& event,
        ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag);
    DistributedKv::Status RegisterDataChangeObserver(DistributedKv::SubscribeType type,
        ani_observerutils::VarCallbackType &cb);
    DistributedKv::Status RegisterSyncCompleteObserver(ani_observerutils::VarCallbackType &cb);
    DistributedKv::Status UnRegisterObserver(std::string const& event, DistributedKv::SubscribeType type,
        ::taihe::optional<ani_observerutils::VarCallbackType> optCallback, bool &isUpdateFlag);
    DistributedKv::Status UnRegisterObserver(std::string const& event, DistributedKv::SubscribeType type,
        ani_observerutils::VarCallbackType callbackParam, bool &isUpdateFlag);
    void UnRegisterAll();

private:
    bool isSchemaStore_ = false;
    std::recursive_mutex cbMapMutex_;
    std::map<std::string, std::vector<std::shared_ptr<ani_observerutils::DataObserver>>> jsCbMap_;
    std::shared_ptr<DistributedKv::SingleKvStore> nativeStore_;
};

class ManagerObserver : public JsBaseObserver, public DistributedKv::KvStoreDeathRecipient,
    public std::enable_shared_from_this<ManagerObserver> {
public:
    ManagerObserver(VarCallbackType cb);
    ~ManagerObserver();
    void OnRemoteDied() override;
};

}  // namespace ani_observerutils
#endif  // OHOS_ANI_OBSERVER_UTILS_H
