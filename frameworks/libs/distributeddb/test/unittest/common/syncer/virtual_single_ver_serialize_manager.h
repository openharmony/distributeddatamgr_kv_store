/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef VIRTUAL_SINGLE_VER_SERIALIZE_MANAGER_H
#define VIRTUAL_SINGLE_VER_SERIALIZE_MANAGER_H

#include "single_ver_serialize_manager.h"

namespace DistributedDB {
class VirtualSingleVerSerializeManager : public SingleVerSerializeManager {
public:
    static int CallDataSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
    {
        return SingleVerSerializeManager::DataSerialization(buffer, length, inMsg);
    }

    static int CallControlSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
    {
        return SingleVerSerializeManager::ControlSerialization(buffer, length, inMsg);
    }

    static int CallDataDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
    {
        return SingleVerSerializeManager::DataDeSerialization(buffer, length, inMsg);
    }

    static int CallControlDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
    {
        return SingleVerSerializeManager::ControlDeSerialization(buffer, length, inMsg);
    }

    static int CallDataPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
    {
        return SingleVerSerializeManager::DataPacketSerialization(buffer, length, inMsg);
    }

    static int CallDataPacketCalculateLen(const Message *inMsg, uint32_t &len)
    {
        return SingleVerSerializeManager::DataPacketCalculateLen(inMsg, len);
    }

    static int CallAckPacketCalculateLen(const Message *inMsg, uint32_t &len)
    {
        return SingleVerSerializeManager::AckPacketCalculateLen(inMsg, len);
    }
};
} // namespace DistributedDB
#endif  // #define VIRTUAL_SINGLE_VER_SERIALIZE_MANAGER_H