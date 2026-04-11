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

#ifndef CLOUD_SYNC_LOG_TABLE_MANAGER_H
#define CLOUD_SYNC_LOG_TABLE_MANAGER_H

#include "sqlite_log_table_manager.h"

namespace DistributedDB {
class CloudSyncLogTableManager : public SqliteLogTableManager {
public:
    CloudSyncLogTableManager() = default;
    ~CloudSyncLogTableManager() override = default;

    // The parameter "references" is "", "NEW." or "OLD.". "identity" is a hash string that identifies a device.
    std::string CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
        const std::string &identity) override;
    std::string GetConflictPkSql(const TableInfo &table) override;

protected:
    std::string GetUpdatePkTrigger(const TableInfo &table, const std::string &identity) override;

private:
    void GetIndexSql(const TableInfo &table, std::vector<std::string> &schema) override;
    std::string GetPrimaryKeySql(const TableInfo &table) override;

    // The parameter "identity" is a hash string that identifies a device. The same for the next two functions.
    std::string GetInsertTrigger(const TableInfo &table, const std::string &identity) override;
    std::string GetUpdateTrigger(const TableInfo &table, const std::string &identity) override;
    std::string GetDeleteTrigger(const TableInfo &table, const std::string &identity) override;
    std::vector<std::string> GetDropTriggers(const TableInfo &table) override;
    std::string CalcPrimaryKeyHashInner(const std::string &references, const std::vector<std::string> &sourceFields,
        const FieldInfoMap &fieldInfos) const;
    std::string GetInsertLogSQL(const TableInfo &table, const std::string &identity, bool isReplace);
    std::string GetUpdateLog(const TableInfo &table, const std::string &identity) const;
    std::string GetUpdateConflictLog(const TableInfo &table, const std::string &identity);
    std::string GetInsertConflictSql(const TableInfo &table, const std::string &identity);
    static std::string GetUpdateCondition(const TableInfo &table);
    static std::string GetUpdateConflictCondition(const TableInfo &table);
    static std::string GetDupNotConflictUpdateCondition(const TableInfo &table);
    static std::string GetInsertCondition(const TableInfo &table);
    static std::string GetUpdateConflictKey(const TableInfo &table);
    static std::string GetOldPkNullCondition(const std::vector<std::string> &pk, bool isNull, bool isAnd);
};

} // DistributedDB

#endif // DISTRIBUTED_UT_CLOUD_SYNC_LOG_TABLE_MANAGER_H
