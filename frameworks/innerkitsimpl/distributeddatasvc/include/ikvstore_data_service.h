/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "message_parcel.h"
#include "types.h"
#include "distributeddata_ipc_interface_code.h"

namespace OHOS::DistributedKv {
class IKvStoreDataService : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedKv.IKvStoreDataService");

    virtual sptr<IRemoteObject> GetFeatureInterface(const std::string &name) = 0;

    virtual Status RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer) = 0;

    virtual int32_t ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex,
        int32_t tokenId) = 0;

protected:
    static constexpr size_t MAX_IPC_CAPACITY = 800 * 1024;
};

} // namespace OHOS::DistributedKv

#endif  // I_KV_STORE_DATA_SERVICE_H
