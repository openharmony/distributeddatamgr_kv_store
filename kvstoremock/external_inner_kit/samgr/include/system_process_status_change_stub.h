/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SAMGR_INTERFACES_INNERKITS_SAMGR_PROXY_INCLUDE_SYSTEM_PROCESS_STATUS_CHANGE_STUB_H
#define SAMGR_INTERFACES_INNERKITS_SAMGR_PROXY_INCLUDE_SYSTEM_PROCESS_STATUS_CHANGE_STUB_H

#include <map>
#include "iremote_stub.h"
#include "isystem_process_status_change.h"

namespace OHOS {
class SystemProcessStatusChangeStub : public IRemoteStub<ISystemProcessStatusChange> {
public:
    SystemProcessStatusChangeStub();
    ~SystemProcessStatusChangeStub() = default;
    int32_t OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
private:
    // The use of gnu extensions is for stabilizing compiler behaviour.
    // Thus to keep ABI compatibilities as long as there are no real interface changes.
    static int32_t LocalSystemProcessStarted(SystemProcessStatusChangeStub *stub,
        MessageParcel& data, MessageParcel& reply)  __attribute__((always_inline))
    {
        return stub->OnSystemProcessStartedInner(data, reply);
    }
    static int32_t LocalSystemProcessStopped(SystemProcessStatusChangeStub *stub,
        MessageParcel& data, MessageParcel& reply) __attribute__((always_inline))
    {
        return stub->OnSystemProcessStoppedInner(data, reply);
    }
    static int32_t LocalSystemProcessActivated(SystemProcessStatusChangeStub *stub,
        MessageParcel& data, MessageParcel& reply) __attribute__((always_inline))
    {
        return stub->OnSystemProcessActivatedInner(data, reply);
    }
    static int32_t LocalSystemProcessIdled(SystemProcessStatusChangeStub *stub,
        MessageParcel& data, MessageParcel& reply) __attribute__((always_inline))
    {
        return stub->OnSystemProcessIdledInner(data, reply);
    }
    int32_t OnSystemProcessStartedInner(MessageParcel& data, MessageParcel& reply) __attribute__((noinline));
    int32_t OnSystemProcessStoppedInner(MessageParcel& data, MessageParcel& reply) __attribute__((noinline));
    int32_t OnSystemProcessActivatedInner(MessageParcel& data, MessageParcel& reply) __attribute__((noinline));
    int32_t OnSystemProcessIdledInner(MessageParcel& data, MessageParcel& reply) __attribute__((noinline));
    static bool EnforceInterceToken(MessageParcel& data);

    using SystemProcessStatusChangeStubFunc =
        int32_t (*)(SystemProcessStatusChangeStub* stub, MessageParcel& data, MessageParcel& reply);
    std::map<uint32_t, SystemProcessStatusChangeStubFunc> memberFuncMap_;
};
}
#endif /* SAMGR_INTERFACES_INNERKITS_SAMGR_PROXY_INCLUDE_SYSTEM_PROCESS_STATUS_CHANGE_STUB_H */
