/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <algorithm>
#include "cloud/cloud_storage_utils.h"
#include "db_errno.h"
#include "log_print.h"
#include "data_donation_sql_generator.h"

namespace DistributedDB {

int DataDonationSqlGenerator::GenerateQuerySql(const DataDonationSchema::DdRelationsPath &path,
    int64_t cursor, std::string &sql) const
{
    int errCode = ValidatePath(path);
    if (errCode != E_OK) {
        LOGE("[DataDonationSqlGenerator] ValidatePath failed: %d", errCode);
        return errCode;
    }

    std::set<std::string> requiredTables = AnalyzeRequiredTables(path);

    sql = BuildSelectClause(path);
    sql += " " + BuildFromClause(path);
    sql += BuildJoinClauses(path, requiredTables);
    sql += " " + BuildPaginationClause(cursor);

    return E_OK;
}

int DataDonationSqlGenerator::ValidatePath(const DataDonationSchema::DdRelationsPath &path) const
{
    if (path.table.empty()) {
        LOGE("[DataDonationSqlGenerator] Main table name is empty");
        return -E_INVALID_ARGS;
    }

    if (path.relations.empty()) {
        LOGE("[DataDonationSqlGenerator] Relations path is empty");
        return -E_INVALID_ARGS;
    }

    for (const auto &relation : path.relations) {
        if (relation.key.localField.table.empty() || relation.key.localField.field.empty()) {
            LOGE("[DataDonationSqlGenerator] Local field in relation key is invalid");
            return -E_INVALID_ARGS;
        }
        if (relation.key.foreignField.table.empty() || relation.key.foreignField.field.empty()) {
            LOGE("[DataDonationSqlGenerator] Foreign field in relation key is invalid");
            return -E_INVALID_ARGS;
        }
    }

    bool hasQueryField = false;
    for (const auto &relation : path.relations) {
        if (!relation.localField.table.empty() || !relation.foreignField.table.empty()) {
            hasQueryField = true;
            break;
        }
    }
    if (!hasQueryField) {
        LOGE("[DataDonationSqlGenerator] No query field specified in relations");
        return -E_INVALID_ARGS;
    }

    return E_OK;
}

std::set<std::string> DataDonationSqlGenerator::AnalyzeRequiredTables(
    const DataDonationSchema::DdRelationsPath &path) const
{
    std::set<std::string> requiredTables;

    for (const auto &relation : path.relations) {
        if (!relation.localField.table.empty()) {
            requiredTables.insert(relation.localField.table);
        }
        if (!relation.foreignField.table.empty()) {
            requiredTables.insert(relation.foreignField.table);
        }
    }

    return requiredTables;
}

std::string DataDonationSqlGenerator::BuildSelectClause(const DataDonationSchema::DdRelationsPath &path) const
{
    std::string selectClause = "SELECT ";
    std::vector<std::string> fields;

    for (const auto &relation : path.relations) {
        if (!relation.localField.table.empty()) {
            std::string fieldRef = FormatFieldRef(relation.localField.table, relation.localField.field);
            if (std::find(fields.begin(), fields.end(), fieldRef) == fields.end()) {
                fields.push_back(fieldRef);
            }
        }
        if (!relation.foreignField.table.empty()) {
            std::string fieldRef = FormatFieldRef(relation.foreignField.table, relation.foreignField.field);
            if (std::find(fields.begin(), fields.end(), fieldRef) == fields.end()) {
                fields.push_back(fieldRef);
            }
        }
    }

    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) {
            selectClause += ", ";
        }
        selectClause += fields[i] + " AS [" + fields[i] + "]";
    }

    return selectClause;
}

std::string DataDonationSqlGenerator::BuildFromClause(const DataDonationSchema::DdRelationsPath &path) const
{
    if (path.relations.empty()) {
        return "FROM " + path.table;
    }
    return "FROM " + path.relations[0].key.localField.table;
}

std::string DataDonationSqlGenerator::BuildJoinClauses(const DataDonationSchema::DdRelationsPath &path,
    const std::set<std::string> &requiredTables) const
{
    std::string joinClause;

    int maxJoinIndex = -1;
    for (int i = 0; i < static_cast<int>(path.relations.size()); ++i) {
        const auto &relation = path.relations[i];
        const std::string &joinedTable = relation.key.foreignField.table;
        if (requiredTables.count(joinedTable) != 0) {
            maxJoinIndex = i;
        }
    }

    for (int i = 0; i <= maxJoinIndex; ++i) {
        const auto &relation = path.relations[i];
        const auto &foreignKey = relation.key;

        joinClause += " LEFT JOIN " + foreignKey.foreignField.table;
        joinClause += " ON " + FormatFieldRef(foreignKey.localField.table, foreignKey.localField.field);
        joinClause += " = " + FormatFieldRef(foreignKey.foreignField.table, foreignKey.foreignField.field);
    }

    return joinClause;
}

std::string DataDonationSqlGenerator::BuildPaginationClause(int64_t cursor) const
{
    return "LIMIT " + std::to_string(CloudDbConstant::SUBSCRIBE_QUERY_LIMIT) + " OFFSET " + std::to_string(cursor);
}

std::string DataDonationSqlGenerator::FormatFieldRef(const std::string &table, const std::string &field) const
{
    return table + "." + field;
}

} // namespace DistributedDB
