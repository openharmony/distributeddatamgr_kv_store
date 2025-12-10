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
#include "types.h"
#include "kvstore_observer.h"
#include "kvstore_sync_callback.h"
#include "kvstore_death_recipient.h"

namespace ani_observerutils {

using namespace OHOS;

class GlobalRefGuard {
    ani_env *env_ = nullptr;
    ani_ref ref_ = nullptr;

public:
    GlobalRefGuard(ani_env *env, ani_object obj) : env_(env)
    {
        if (!env_ || !obj)
            return;
        if (ANI_OK != env_->GlobalReference_Create(obj, &ref_)) {
            ref_ = nullptr;
        }
    }
    explicit operator bool() const
    {
        return ref_ != nullptr;
    }
    ani_ref get() const
    {
        return ref_;
    }
    ~GlobalRefGuard()
    {
        if (env_ && ref_) {
            env_->GlobalReference_Delete(ref_);
        }
    }

    GlobalRefGuard(const GlobalRefGuard &) = delete;
    GlobalRefGuard &operator=(const GlobalRefGuard &) = delete;
};

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
    JsBaseObserver(VarCallbackType cb, ani_ref jsCallbackRef);
    virtual ~JsBaseObserver() {}
    bool SendEventToMainThread(const std::function<void()> func);
    void Release();

    std::recursive_mutex mutex_;
    VarCallbackType jsCallback_;
    ani_ref jsCallbackRef_ = nullptr;
    static std::shared_ptr<OHOS::AppExecFwk::EventHandler> mainHandler_;
};

template<class DerivedObserverClass>
ani_ref CreateCallbackRefIfNotDuplicate(std::vector<std::shared_ptr<DerivedObserverClass>> const& cbVec,
    ani_object callbackObj)
{
    ani_ref callbackRef = nullptr;
    ani_env *env = taihe::get_env();
    if (env == nullptr || ANI_OK != env->GlobalReference_Create(callbackObj, &callbackRef)) {
        return nullptr;
    }
    bool isDuplicate = std::any_of(cbVec.begin(), cbVec.end(),
        [env, callbackRef](std::shared_ptr<DerivedObserverClass> const& obj) {
            ani_boolean isEqual = false;
            return (ANI_OK == env->Reference_StrictEquals(callbackRef, obj->jsCallbackRef_, &isEqual)) && isEqual;
        });
    if (isDuplicate) {
        env->GlobalReference_Delete(callbackRef);
        return nullptr;
    }
    return callbackRef;
}

class DataObserver : public JsBaseObserver,
    public DistributedKv::KvStoreObserver, public DistributedKv::KvStoreSyncCallback,
    public std::enable_shared_from_this<DataObserver> {
public:
    DataObserver(VarCallbackType cb, ani_ref jsCallbackRef);
    ~DataObserver();
    void SetIsSchemaStore(bool isSchemaStore);
    void OnChange(const DistributedKv::ChangeNotification& info) override;
    void OnChangeInMainThread(const DistributedKv::ChangeNotification& info);
    void SyncCompleted(const std::map<std::string, DistributedKv::Status>& results) override;
    void SyncCompletedInMainThread(const std::map<std::string, DistributedKv::Status>& results);

protected:
    bool isSchemaStore_ = false;
};

class ManagerObserver : public JsBaseObserver, public DistributedKv::KvStoreDeathRecipient,
    public std::enable_shared_from_this<ManagerObserver> {
public:
    ManagerObserver(VarCallbackType cb, ani_ref jsCallbackRef);
    ~ManagerObserver();
    void OnRemoteDied() override;
    void OnRemoteDiedInMainThread();
};

}  // namespace ani_observerutils
#endif  // OHOS_ANI_OBSERVER_UTILS_H
