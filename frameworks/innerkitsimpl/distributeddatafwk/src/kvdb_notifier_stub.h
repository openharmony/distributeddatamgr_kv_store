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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_NOTIFIER_STUB_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_NOTIFIER_STUB_H

#include "distributeddata_kvdb_ipc_interface_code.h"
#include "ikvdb_notifier.h"
#include "iremote_broker.h"
#include "iremote_stub.h"

namespace OHOS {
namespace DistributedKv {
class KVDBNotifierStub : public IRemoteStub<IKVDBNotifier> {
public:
    int OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    using Handler = int32_t (KVDBNotifierStub::*)(MessageParcel &data, MessageParcel &reply);
    int32_t OnSyncCompleted(MessageParcel& data, MessageParcel& reply);
    int32_t OnCloudSyncCompleted(MessageParcel& data, MessageParcel& reply);
    int32_t OnOnRemoteChange(MessageParcel& data, MessageParcel& reply);
    int32_t OnOnSwitchChange(MessageParcel& data, MessageParcel& reply);
    static const Handler HANDLERS[static_cast<uint32_t>(KVDBNotifierCode::TRANS_BUTT)];
};
} // namespace DistributedKv
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_NOTIFIER_STUB_H