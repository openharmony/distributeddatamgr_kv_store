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
#include "cloud_locker.h"

#include "db_errno.h"
namespace DistributedDB {
int CloudLocker::BuildCloudLock(const AfterBuildAction &buildAction, const BeforeFinalize &finalize,
    std::shared_ptr<CloudLocker> &locker)
{
    locker = std::make_shared<CloudLocker>();
    int errCode = buildAction();
    if (errCode != E_OK) {
        locker = nullptr;
    } else {
        locker->finalize_ = finalize;
    }
    return errCode;
}

CloudLocker::~CloudLocker()
{
    if (finalize_) {
        finalize_();
    }
}
}