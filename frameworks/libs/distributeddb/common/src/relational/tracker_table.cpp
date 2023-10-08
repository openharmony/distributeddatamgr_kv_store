/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifdef RELATIONAL_STORE
#include "tracker_table.h"
#include "schema_constant.h"

namespace DistributedDB {
void TrackerTable::Init(const TrackerSchema &schema)
{
    tableName_ = schema.tableName;
    extendColName_ = schema.extendColName;
    trackerColNames_ = schema.trackerColNames;
}

std::string TrackerTable::GetTableName() const
{
    return tableName_;
}

const std::set<std::string> &TrackerTable::GetTrackerColNames() const
{
    return trackerColNames_;
}

const std::string TrackerTable::GetAssignValSql() const
{
    if (extendColName_.empty()) {
        return "''";
    }
    std::string sql;
    sql += "NEW." + extendColName_;
    return sql;
}

const std::string TrackerTable::GetExtendAssignValSql() const
{
    if (extendColName_.empty()) {
        return "";
    }
    std::string sql;
    sql += ", extend_field = NEW." + extendColName_;
    return sql;
}

const std::string TrackerTable::GetDiffTrackerValSql() const
{
    if (trackerColNames_.empty()) {
        return "0";
    }
    std::string sql = " case when (";
    size_t index = 0;
    for (const auto &colName: trackerColNames_) {
        sql += "NEW." + colName + " <> OLD." + colName;
        if (index < trackerColNames_.size() - 1) {
            sql += " or ";
        }
        index++;
    }
    sql += ") then 1 else 0 end)";
    return sql;
}

const std::string TrackerTable::GetExtendName() const
{
    return extendColName_;
}

std::string TrackerTable::ToString() const
{
    std::string attrStr;
    attrStr += "{";
    attrStr += R"("NAME": ")" + tableName_ + "\",";
    attrStr += R"("EXTEND_NAME": ")" + extendColName_ + "\",";
    attrStr += R"("TRACKER_NAMES": [)";
    for (const auto &colName: trackerColNames_) {
        attrStr += "\"" + colName + "\",";
    }
    attrStr.pop_back();
    attrStr += "]}";
    return attrStr;
}

void TrackerTable::SetTableName(const std::string &tableName)
{
    tableName_ = tableName;
}

void TrackerTable::SetExtendName(const std::string &colName)
{
    extendColName_ = colName;
}

void TrackerTable::SetTrackerNames(const std::set<std::string> &trackerNames)
{
    trackerColNames_ = std::move(trackerNames);
}

bool TrackerTable::IsEmpty() const
{
    return trackerColNames_.empty();
}
}
#endif