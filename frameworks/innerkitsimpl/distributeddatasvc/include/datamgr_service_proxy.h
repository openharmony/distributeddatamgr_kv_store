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
#ifndef KVSTORE_DATA_SERVICE_PROXY_H
#define KVSTORE_DATA_SERVICE_PROXY_H
#include "ikvstore_data_service.h"
namespace OHOS::DistributedKv {
class DataMgrServiceProxy : public IRemoteProxy<IKvStoreDataService> {
public:
    explicit API_EXPORT DataMgrServiceProxy(const sptr<IRemoteObject> &impl);
    ~DataMgrServiceProxy() = default;
    sptr<IRemoteObject> GetFeatureInterface(const std::string &name) override;

    Status RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer) override;

    int32_t ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex, int32_t tokenId) override;

private:
    static inline BrokerDelegator<DataMgrServiceProxy> delegator_;
};
}
#endif //KVSTORE_DATA_SERVICE_PROXY_H
