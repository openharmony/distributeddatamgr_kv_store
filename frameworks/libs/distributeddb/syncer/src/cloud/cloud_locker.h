/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef CLOUD_LOCKER_H
#define CLOUD_LOCKER_H
#include <memory>
#include "icloud_syncer.h"
namespace DistributedDB {
using AfterBuildAction = std::function<int()>;
using BeforeFinalize = std::function<void()>;
class CloudLocker final {
public:
    static int BuildCloudLock(const AfterBuildAction &buildAction, const BeforeFinalize &finalize,
        std::shared_ptr<CloudLocker> &locker);
    CloudLocker() = default;
    ~CloudLocker();
private:
    BeforeFinalize finalize_;
};
}
#endif // CLOUD_LOCKER_H