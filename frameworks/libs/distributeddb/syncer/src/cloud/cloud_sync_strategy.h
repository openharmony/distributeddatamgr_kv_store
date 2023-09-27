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

#ifndef CLOUD_SYNC_STRATEGY_H
#define CLOUD_SYNC_STRATEGY_H

#include "data_transformer.h"
#include "db_errno.h"
#include "db_types.h"
#include "icloud_sync_storage_interface.h"

namespace DistributedDB {
class CloudSyncStrategy {
public:
    CloudSyncStrategy() = default;
    virtual ~CloudSyncStrategy() = default;

    virtual OpType TagSyncDataStatus(bool existInLocal, const LogInfo &localInfo, const LogInfo &cloudInfo)
    {
        (void)existInLocal;
        (void)localInfo;
        (void)cloudInfo;
        return OpType::NOT_HANDLE;
    }

    virtual bool JudgeUpdateCursor()
    {
        return false;
    }

    virtual bool JudgeUpload()
    {
        return false;
    }

    static bool IsDelete(const LogInfo &info)
    {
        return (info.flag & 0x1) == 1;
    }
};
}
#endif // CLOUD_SYNC_STRATEGY_H
