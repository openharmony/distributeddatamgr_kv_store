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

#ifndef DISTRIBUTED_KVSTORE_MOCK_REMOTE_OBJECT_H
#define DISTRIBUTED_KVSTORE_MOCK_REMOTE_OBJECT_H

#include <codecvt>
#include <locale>
#include <string>

#include "ipc_types.h"
#include "message_option.h"
#include "message_parcel.h"

namespace OHOS {
class IRemoteBroker;
inline std::u16string to_utf16(const std::string &str)
{
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(str);
}

class IRemoteObject : public virtual Parcelable, public virtual RefBase {
public:
    enum {
        IF_PROT_DEFAULT, /* Invoker family. */
        IF_PROT_BINDER = IF_PROT_DEFAULT,
        IF_PROT_DATABUS,
        IF_PROT_ERROR,
    };
    enum {
        DATABUS_TYPE,
    };
    class DeathRecipient : public RefBase {
    public:
        enum {
            ADD_DEATH_RECIPIENT,
            REMOVE_DEATH_RECIPIENT,
            NOTICE_DEATH_RECIPIENT,
            TEST_SERVICE_DEATH_RECIPIENT,
            TEST_DEVICE_DEATH_RECIPIENT,
        };
        virtual void OnRemoteDied(const wptr<IRemoteObject> &object) = 0;
    };

    virtual int32_t GetObjectRefCount()
    {
        return 0;
    }

    virtual int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        return false;
    }

    virtual bool IsProxyObject() const
    {
        return true;
    }

    virtual bool CheckObjectLegality() const
    {
        return false;
    }

    virtual bool AddDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return false;
    }

    virtual bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return false;
    }

    virtual bool Marshalling(Parcel &parcel) const override
    {
        return false;
    }

    static sptr<IRemoteObject> Unmarshalling(Parcel &parcel)
    {
        return nullptr;
    }

    static bool Marshalling(Parcel &parcel, const sptr<IRemoteObject> &object)
    {
        return false;
    }

    virtual sptr<IRemoteBroker> AsInterface()
    {
        return nullptr;
    }

    virtual int Dump(int fd, const std::vector<std::u16string> &args)
    {
        return 0;
    }

    const std::u16string descriptor_;

    std::u16string GetObjectDescriptor() const
    {
        return descriptor_;
    }

protected:
    explicit IRemoteObject(std::u16string descriptor = nullptr) {}
    virtual ~IRemoteObject() {}
};
} // namespace OHOS
#endif // DISTRIBUTED_KVSTORE_MOCK_REMOTE_OBJECT_H