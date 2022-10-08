/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef MOCK_REMOTE_EXECUTOR_H
#define MOCK_REMOTE_EXECUTOR_H

#include <gmock/gmock.h>
#include "remote_executor.h"

namespace DistributedDB {
class MockRemoteExecutor : public RemoteExecutor {
public:
    MOCK_METHOD2(ParseOneRequestMessage, void(const std::string &, Message *));

    MOCK_METHOD1(IsPacketValid, bool(uint32_t));

    void CallResponseFailed(int errCode, uint32_t sessionId, uint32_t sequenceId, const std::string &device)
    {
        RemoteExecutor::ResponseFailed(errCode, sessionId, sequenceId, device);
    }
};
} // namespace DistributedDB
#endif  // #define MOCK_META_DATA_H