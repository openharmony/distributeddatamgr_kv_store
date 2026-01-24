/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_KVSTORE_MOCK_IPC_OBJECT_STUB_H
#define DISTRIBUTED_KVSTORE_MOCK_IPC_OBJECT_STUB_H

#include <list>

#include "iremote_object.h"

namespace OHOS {
struct RefCountNode {
    int remotePid;
    std::string deviceId;
};

class IPCObjectStub : public IRemoteObject {
public:
    enum {
        OBJECT_TYPE_NATIVE,
        OBJECT_TYPE_JAVA,
        OBJECT_TYPE_JAVASCRIPT,
    };

    explicit IPCObjectStub(std::u16string descriptor = std::u16string()) : IRemoteObject(descriptor) {}
    ~IPCObjectStub() override {}

    bool IsProxyObject() const override
    {
        return false;
    }

    int32_t GetObjectRefCount() override
    {
        return 0;
    }

    int Dump(int fd, const std::vector<std::u16string>& args) override
    {
        return -1;
    }

    virtual int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
    {
        return -1;
    }

    int SendRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override
    {
        return -1;
    }

    void OnFirstStrongRef(const void* objectId) override {}

    void OnLastStrongRef(const void* objectId) override {}

    bool AddDeathRecipient(const sptr<DeathRecipient>& recipient) override
    {
        return false;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient>& recipient) override
    {
        return false;
    }

    int GetCallingPid()
    {
        return 0;
    }

    int GetCallingUid()
    {
        return 0;
    }

    uint32_t GetCallingTokenID()
    {
        return 0;
    }

    uint64_t GetCallingFullTokenID()
    {
        return 0;
    }

    uint32_t GetFirstTokenID()
    {
        return 0;
    }

    uint64_t GetFirstFullTokenID()
    {
        return 0;
    }

    virtual int OnRemoteDump(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
    {
        return -1;
    }

    virtual int32_t ProcessProto(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
    {
        return -1;
    }

    virtual int GetObjectType() const
    {
        return -1;
    }

private:
    bool IsDeviceIdIllegal(const std::string& deviceID)
    {
        return false;
    }
};
} // namespace OHOS
#endif // DISTRIBUTED_KVSTORE_MOCK_IPC_OBJECT_STUB_H
