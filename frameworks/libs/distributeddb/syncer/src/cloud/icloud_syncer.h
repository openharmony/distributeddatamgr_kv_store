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
#ifndef I_CLOUD_SYNCER_H
#define I_CLOUD_SYNCER_H
#include "cloud/cloud_store_types.h"
#include "ref_object.h"
namespace DistributedDB {
class ICloudSyncer : public virtual RefObject {
public:
    using TaskId = uint64_t;
    struct CloudTaskInfo {
        SyncMode mode = SyncMode::SYNC_MODE_PUSH_ONLY;
        ProcessStatus status = ProcessStatus::PREPARED;
        int errCode = 0;
        TaskId taskId = 0u;
        std::vector<std::string> table;
        SyncProcessCallback callback;
        int64_t timeout = 0;
        std::vector<std::string> devices;
    };

    struct InnerProcessInfo {
        std::string tableName;
        ProcessStatus tableStatus = ProcessStatus::PREPARED;
        Info downLoadInfo;
        Info upLoadInfo;
    };

    virtual void IncSyncCallbackTaskCount() = 0;

    virtual void DecSyncCallbackTaskCount() = 0;

    virtual std::string GetIdentify() const = 0;
};
}
#endif // I_CLOUD_SYNCER_H
