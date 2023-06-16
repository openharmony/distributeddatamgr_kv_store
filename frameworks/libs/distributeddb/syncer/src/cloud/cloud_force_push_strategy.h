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
#ifdef MANNUAL_SYNC_AND_CLEAN_CLOUD_DATA
#ifndef LOCAL_COVER_CLOUD_H
#define LOCAL_COVER_CLOUD_H
#include "cloud_sync_strategy.h"

namespace DistributedDB {
class CloudForcePushStrategy : public CloudSyncStrategy {
public:
    OpType TagSyncDataStatus(bool existInLocal, LogInfo &localInfo, LogInfo &cloudInfo) override;

    bool JudgeUpdateCursor() override;

    bool JudgeUpload() override;
};
}
#endif // LOCAL_COVER_CLOUD_H
#endif // MANNUAL_SYNC_AND_CLEAN_CLOUD_DATA