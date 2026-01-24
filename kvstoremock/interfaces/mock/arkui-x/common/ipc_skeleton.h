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

#ifndef DISTRIBUTED_KVSTORE_IPC_SKELETON_H
#define DISTRIBUTED_KVSTORE_IPC_SKELETON_H

#include <cstdint>

namespace OHOS {
class IPCSkeleton {
public:
    IPCSkeleton() = default;
    ~IPCSkeleton() = default;

    static uint32_t GetCallingTokenID()
    {
        return 0;
    }

    static int32_t GetCallingPid()
    {
        return 0;
    }

    static uint64_t GetSelfTokenID()
    {
        return 0;
    }
};
}
#endif // DISTRIBUTED_KVSTORE_IPC_SKELETON_H
