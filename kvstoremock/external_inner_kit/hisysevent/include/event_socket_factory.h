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

#ifndef EVENT_SOCKET_FACTORY_H
#define EVENT_SOCKET_FACTORY_H

#include <sys/socket.h>
#include <sys/un.h>

#include "raw_data.h"

namespace OHOS {
namespace HiviewDFX {
using namespace Encoded;
using EventSocket = struct sockaddr_un;
class EventSocketFactory {
public:
    static EventSocket& GetEventSocket(RawData& data);
};
}
}

#endif // EVENT_SOCKET_FACTORY_H
