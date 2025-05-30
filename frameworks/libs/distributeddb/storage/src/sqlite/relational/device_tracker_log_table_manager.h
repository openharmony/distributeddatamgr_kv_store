/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef DEVICE_TRACKER_LOG_TABLE_MANAGER_H
#define DEVICE_TRACKER_LOG_TABLE_MANAGER_H

#include "sqlite_log_table_manager.h"

namespace DistributedDB {
class DeviceTrackerLogTableManager : public SqliteLogTableManager {
public:
    DeviceTrackerLogTableManager() = default;
    ~DeviceTrackerLogTableManager() override = default;

    // The parameter "references" is "", "NEW." or "OLD.". "identity" is a hash string that identifies a device.
    std::string CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
        const std::string &identity) override;

private:
    void GetIndexSql(const TableInfo &table, std::vector<std::string> &schema) override;
    std::string GetPrimaryKeySql(const TableInfo &table) override;

    // The parameter "identity" is a hash string that identifies a device. The same for the next two functions.
    std::string GetInsertTrigger(const TableInfo &table, const std::string &identity) override;
    std::string GetUpdateTrigger(const TableInfo &table, const std::string &identity) override;
    std::string GetDeleteTrigger(const TableInfo &table, const std::string &identity) override;
    std::vector<std::string> GetDropTriggers(const TableInfo &table) override;
    static std::string GetChangeDataStatus(const TableInfo &table);
};

} // DistributedDB

#endif // DEVICE_TRACKER_LOG_TABLE_MANAGER_H
