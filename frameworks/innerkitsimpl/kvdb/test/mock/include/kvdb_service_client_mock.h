/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_KVDB_SERVICE_CLIENT_MOCK_H
#define OHOS_DISTRIBUTED_DATA_KVDB_SERVICE_CLIENT_MOCK_H

#include "kvdb_service_client.h"
#include <gmock/gmock.h>

namespace OHOS::DistributedKv {
class BKVDBServiceClient {
public:
    virtual std::shared_ptr<KVDBServiceClient> GetInstance() = 0;
    virtual sptr<KVDBNotifierClient> GetServiceAgent(const AppId &) = 0;
    BKVDBServiceClient() = default;
    virtual ~BKVDBServiceClient() = default;

public:
    static inline std::shared_ptr<BKVDBServiceClient> kVDBServiceClient = nullptr;
};

class KVDBServiceClientMock : public BKVDBServiceClient {
public:
    MOCK_METHOD(std::shared_ptr<KVDBServiceClient>, GetInstance, ());
    MOCK_METHOD(sptr<KVDBNotifierClient>, GetServiceAgent, (const AppId &));
    static inline Status status = ERROR;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_KVDB_SERVICE_CLIENT_MOCK_H