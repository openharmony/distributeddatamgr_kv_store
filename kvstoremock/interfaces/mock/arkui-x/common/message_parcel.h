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

#ifndef DISTRIBUTED_KVSTORE_MOCK_MESSAGE_PARCEL_H
#define DISTRIBUTED_KVSTORE_MOCK_MESSAGE_PARCEL_H

#include <string>
#include <vector>

#include "ashmem.h"
#include "parcel.h"
#include "refbase.h"

namespace OHOS {
class IRemoteObject;
class MessageParcel : public Parcel {
public:
    MessageParcel() = default;
    ~MessageParcel() = default;

    bool WriteRemoteObject(const sptr<IRemoteObject>& object)
    {
        return false;
    }

    sptr<IRemoteObject> ReadRemoteObject()
    {
        return sptr<IRemoteObject>(nullptr);
    }

    bool WriteFileDescriptor(int fd)
    {
        return false;
    }

    int ReadFileDescriptor()
    {
        return 0;
    }

    bool ContainFileDescriptors() const
    {
        return 0;
    }

    bool WriteInterfaceToken(std::u16string name)
    {
        if (!name.empty()) {
            interfaceToken_ = name;
            return false;
        }
        return false;
    }

    std::u16string ReadInterfaceToken()
    {
        return interfaceToken_;
    }

    bool WriteRawData(const void* data, size_t size)
    {
        return false;
    }

    const void* ReadRawData(size_t size)
    {
        return nullptr;
    }

    bool RestoreRawData(std::shared_ptr<char> rawData, size_t size)
    {
        return false;
    }

    const void* GetRawData() const
    {
        return nullptr;
    }

    size_t GetRawDataSize() const
    {
        return 0;
    }

    size_t GetRawDataCapacity() const
    {
        return 0;
    }

    void WriteNoException() {}

    int32_t ReadException()
    {
        return 0;
    }

    bool WriteAshmem(sptr<Ashmem> ashmem)
    {
        return 0;
    }

    sptr<Ashmem> ReadAshmem()
    {
        return nullptr;
    }

    void ClearFileDescriptor() {}

    void SetClearFdFlag()
    {
        needCloseFd_ = true;
    }

    bool Append(MessageParcel& data)
    {
        return false;
    }

    void PrintBuffer(const char* funcName, const size_t lineNum) {}

    std::u16string GetInterfaceToken() const
    {
        return std::u16string(u"mockToken");
    }

    bool IsOwner()
    {
        return false;
    }
    
    bool ReadInt16(int16_t &value)
    {
        return false;
    }

    bool WriteInt16(int16_t value)
    {
        return false;
    }

    bool ReadUint16(uint16_t &value)
    {
        return false;
    }

    bool WriteUint16(uint16_t value)
    {
        return false;
    }

    bool ReadInt32(int32_t &value)
    {
        return false;
    }

    int32_t ReadInt32()
    {
        return 0;
    }

    bool WriteInt32(int32_t value)
    {
        return false;
    }

    bool ReadUint32(uint32_t &value)
    {
        return false;
    }

    int32_t ReadUint32()
    {
        return 0;
    }

    bool WriteUint32(uint32_t value)
    {
        return false;
    }

    bool ReadInt64(int64_t &value)
    {
        return false;
    }

    bool WriteInt64(int64_t value)
    {
        return false;
    }

    bool ReadUint64(uint64_t &value)
    {
        return false;
    }

    bool WriteUint64(uint64_t value)
    {
        return false;
    }

    bool ReadDouble(double &value)
    {
        return false;
    }

    bool WriteDouble(double value)
    {
        return false;
    }

    bool ReadBool(bool &value)
    {
        return false;
    }

    bool WriteBool(bool value)
    {
        return false;
    }

    bool ReadString(std::string &value)
    {
        return false;
    }

    bool WriteString(const std::string &value)
    {
        return false;
    }

    bool ReadString16(std::u16string &value)
    {
        return false;
    }

    bool WriteString16(const std::u16string &value)
    {
        return false;
    }

    bool ReadFloatVector(std::vector<float> *value)
    {
        return false;
    }

    bool WriteFloatVector(const std::vector<float> &value)
    {
        return false;
    }

    bool ReadUInt8Vector(std::vector<uint8_t> *value)
    {
        return false;
    }

    bool WriteUInt8Vector(const std::vector<uint8_t> &value)
    {
        return false;
    }

    size_t GetReadableBytes()
    {
        return 0;
    }

    bool needCloseFd_ = false;
    std::u16string interfaceToken_;
};
} // namespace OHOS
#endif // DISTRIBUTED_KVSTORE_MOCK_MESSAGE_PARCEL_H
