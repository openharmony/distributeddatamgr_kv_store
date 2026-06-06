/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef OHOS_ABILITY_RUNTIME_DATA_ABILITY_OBSERVER_STUB_H
#define OHOS_ABILITY_RUNTIME_DATA_ABILITY_OBSERVER_STUB_H

#include <memory>
#include <map>

#include "data_ability_observer_interface.h"
#include "nocopyable.h"

#include <iremote_object.h>
#include <iremote_stub.h>

namespace OHOS {
namespace AAFwk {
class DataAbilityObserverStub : public IRemoteStub<IDataAbilityObserver> {
public:
    DataAbilityObserverStub();
    virtual ~DataAbilityObserverStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    /**
     * @brief Called back to notify that the data being observed has changed.
     *
     */
    int32_t OnChangeInner(MessageParcel &data, MessageParcel &reply);

    /**
     * @brief Called back to notify that the data being observed has changed.
     *
     */
    int32_t OnChangeExtInner(MessageParcel &data, MessageParcel &reply);

    /**
     * @brief Called back to notify that the data being observed has changed.
     *
     */
    int32_t OnChangePreferencesInner(MessageParcel &data, MessageParcel &reply);

    using RequestFuncType = int (DataAbilityObserverStub::*)(MessageParcel &data, MessageParcel &reply);
    static const RequestFuncType HANDLES[TRANS_BUTT];

    DISALLOW_COPY_AND_MOVE(DataAbilityObserverStub);
};

/**
 * @class DataObsCallbackRecipient
 * DataObsCallbackRecipient notices IRemoteBroker died.
 */
class DataObsCallbackRecipient : public IRemoteObject::DeathRecipient {
public:
    using RemoteDiedHandler = std::function<void(const wptr<IRemoteObject> &)>;
    explicit DataObsCallbackRecipient(RemoteDiedHandler handler);
    virtual ~DataObsCallbackRecipient();
    virtual void OnRemoteDied(const wptr<IRemoteObject> &remote);

private:
    RemoteDiedHandler handler_;
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_DATA_ABILITY_OBSERVER_STUB_H
