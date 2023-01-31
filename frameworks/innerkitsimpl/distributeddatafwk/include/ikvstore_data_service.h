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

#ifndef I_KV_STORE_DATA_SERVICE_H
#define I_KV_STORE_DATA_SERVICE_H

#include "iremote_broker.h"
#include "ikvstore_client_death_observer.h"
#include "ikvstore_observer.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "message_parcel.h"
#include "types.h"

namespace OHOS::DistributedKv {
class IKvStoreDataService : public IRemoteBroker {
public:
    enum {
        GET_FEATURE_INTERFACE,
        REGISTERCLIENTDEATHOBSERVER,
        CLOSEKVSTORE,
        CLOSEALLKVSTORE,
        DELETEKVSTORE,
        DELETEALLKVSTORE,
        GETSINGLEKVSTORE,
        GETLOCALDEVICE,
        GETREMOTEDEVICES,
        STARTWATCHDEVICECHANGE,
        STOPWATCHDEVICECHANGE,
        SERVICE_CMD_LAST,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedKv.IKvStoreDataService");

    virtual sptr<IRemoteObject> GetFeatureInterface(const std::string &name) = 0;

    virtual Status RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer) = 0;

protected:
    static constexpr size_t MAX_IPC_CAPACITY = 800 * 1024;
};

class KvStoreDataServiceStub : public IRemoteStub<IKvStoreDataService> {
public:
    int API_EXPORT OnRemoteRequest(uint32_t code, MessageParcel &data,
                        MessageParcel &reply, MessageOption &option) override;

private:
    int32_t NoSupport(MessageParcel &data, MessageParcel &reply);
    int32_t GetFeatureInterfaceOnRemote(MessageParcel &data, MessageParcel &reply);
    int32_t RegisterClientDeathObserverOnRemote(MessageParcel &data, MessageParcel &reply);

    using RequestHandler = int32_t(KvStoreDataServiceStub::*)(MessageParcel&, MessageParcel&);
    static constexpr RequestHandler HANDLERS[SERVICE_CMD_LAST] = {
        [GET_FEATURE_INTERFACE] = &KvStoreDataServiceStub::GetFeatureInterfaceOnRemote,
        [REGISTERCLIENTDEATHOBSERVER] = &KvStoreDataServiceStub::RegisterClientDeathObserverOnRemote,
        [CLOSEKVSTORE] = &KvStoreDataServiceStub::NoSupport,
        [CLOSEALLKVSTORE] = &KvStoreDataServiceStub::NoSupport,
        [DELETEKVSTORE] = &KvStoreDataServiceStub::NoSupport,
        [DELETEALLKVSTORE] = &KvStoreDataServiceStub::NoSupport,
        [GETSINGLEKVSTORE] = &KvStoreDataServiceStub::NoSupport,
        [GETLOCALDEVICE] = &KvStoreDataServiceStub::NoSupport,
        [GETREMOTEDEVICES] = &KvStoreDataServiceStub::NoSupport,
        [STARTWATCHDEVICECHANGE] = &KvStoreDataServiceStub::NoSupport,
        [STOPWATCHDEVICECHANGE] = &KvStoreDataServiceStub::NoSupport,
    };
};

class API_EXPORT KvStoreDataServiceProxy : public IRemoteProxy<IKvStoreDataService> {
public:
    explicit KvStoreDataServiceProxy(const sptr<IRemoteObject> &impl);
    ~KvStoreDataServiceProxy() = default;
    sptr<IRemoteObject> GetFeatureInterface(const std::string &name) override;

    Status RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer) override;

private:
    static inline BrokerDelegator<KvStoreDataServiceProxy> delegator_;
};
} // namespace OHOS::DistributedKv

#endif  // I_KV_STORE_DATA_SERVICE_H
