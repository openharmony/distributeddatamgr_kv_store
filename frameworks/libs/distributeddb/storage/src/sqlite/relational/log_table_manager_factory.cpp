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

#include "log_table_manager_factory.h"
#include "cloud_sync_log_table_manager.h"
#include "collaboration_log_table_manager.h"
#include "device_tracker_log_table_manager.h"
#include "split_device_log_table_manager.h"

namespace DistributedDB {
std::unique_ptr<SqliteLogTableManager> LogTableManagerFactory::GetTableManager(const TableInfo &tableInfo,
    DistributedTableMode mode, TableSyncType syncType)
{
    if (syncType == CLOUD_COOPERATION) {
        return std::make_unique<CloudSyncLogTableManager>();
    }
    if (!tableInfo.GetTrackerTable().IsEmpty()) {
        return std::make_unique<DeviceTrackerLogTableManager>();
    }
    if (mode == DistributedTableMode::SPLIT_BY_DEVICE) {
        return std::make_unique<SplitDeviceLogTableManager>();
    }
    return std::make_unique<CollaborationLogTableManager>();
}
}