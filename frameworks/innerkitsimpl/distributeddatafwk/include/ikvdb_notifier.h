/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef I_KVDB_NOTIFIER_H
#define I_KVDB_NOTIFIER_H

#include <map>
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "kvstore_observer.h"
#include "types.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
class IKVDBNotifier : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedKv.IKvStoreSyncCallback");
    virtual void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) = 0;
    virtual void SyncCompleted(uint64_t seqNum, ProgressDetail &&detail) = 0;
    virtual void OnRemoteChange(const std::map<std::string, bool> &mask) = 0;
    virtual void OnSwitchChange(const SwitchNotification &notification) = 0;
};
} // namespace DistributedKv
} // namespace OHOS

#endif // I_KVDB_NOTIFIER_H
