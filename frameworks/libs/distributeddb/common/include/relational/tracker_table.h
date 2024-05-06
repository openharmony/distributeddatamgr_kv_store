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
#ifndef TRACKER_TABLE_H
#define TRACKER_TABLE_H
#include <set>
#include "store_types.h"
#include "db_types.h"
#include "db_errno.h"
#include "json_object.h"

#ifdef RELATIONAL_STORE
namespace DistributedDB {
class TrackerTable {
public:
    TrackerTable() = default;
    virtual ~TrackerTable() {};

    std::string GetTableName() const;
    const std::set<std::string> &GetTrackerColNames() const;
    void Init(const TrackerSchema &schema);
    const std::string GetAssignValSql(bool isDelete = false) const;
    const std::string GetExtendAssignValSql(bool isDelete = false) const;
    const std::string GetDiffTrackerValSql() const;
    const std::string GetDiffIncCursorSql(const std::string &tableName) const;
    const std::string GetExtendName() const;
    std::string ToString() const;
    const std::vector<std::string> GetDropTempTriggerSql() const;
    const std::string GetTempInsertTriggerSql() const;
    const std::string GetTempUpdateTriggerSql() const;
    const std::string GetTempDeleteTriggerSql() const;
    const std::string GetUpgradedExtendValSql() const;
    void SetTableName(const std::string &tableName);
    void SetExtendName(const std::string &colName);
    void SetTrackerNames(const std::set<std::string> &trackerNames);
    bool IsEmpty() const;
    bool IsChanging(const TrackerSchema &schema);

private:
    std::string tableName_;
    std::string extendColName_;
    std::set<std::string> trackerColNames_;
};

} // namespace DistributedDB
#endif // RELATIONAL_STORE
#endif // TRACKER_TABLE_H