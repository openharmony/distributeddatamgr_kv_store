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

#ifndef DATA_DONATION_SQL_GENERATOR_H
#define DATA_DONATION_SQL_GENERATOR_H

#include <set>
#include <string>
#include "data_donation_schema.h"
#include "db_types.h"

namespace DistributedDB {

class DataDonationSqlGenerator {
public:
    DataDonationSqlGenerator() = default;
    ~DataDonationSqlGenerator() = default;

    int GenerateQuerySql(const DataDonationSchema::DdRelationsPath &path,
        const std::vector<std::pair<std::string, int64_t>> &cursorValues,
        const std::vector<std::pair<std::string, int64_t>> &maxRowids, std::string &sql) const;
    static std::string BuildFromTableName(const DataDonationSchema::DdRelationsPath &path);
    static std::vector<std::string> GetJoinedTableNames(const DataDonationSchema::DdRelationsPath &path);

private:
    std::string BuildSelectClause(const DataDonationSchema::DdRelationsPath &path) const;
    std::string BuildRowidSelectClause(const std::vector<std::string> &tableNames) const;
    std::string BuildFromClause(const DataDonationSchema::DdRelationsPath &path) const;
    std::string BuildJoinClauses(const DataDonationSchema::DdRelationsPath &path,
        const std::set<std::string> &requiredTables) const;
    std::string BuildRowidClause(const std::vector<std::pair<std::string, int64_t>> &cursorValues,
        const std::vector<std::pair<std::string, int64_t>> &maxRowids, const std::string &mainTable) const;
    std::string BuildLexicographicWhere(const std::vector<std::pair<std::string, int64_t>> &cursorValues) const;
    std::string BuildMaxRowidConstraints(const std::vector<std::pair<std::string, int64_t>> &maxRowids,
        const std::string &mainTable) const;
    std::string BuildOrderByClause(const std::vector<std::string> &tableNames) const;
    std::set<std::string> AnalyzeRequiredTables(const DataDonationSchema::DdRelationsPath &path) const;
    std::string FormatFieldRef(const std::string &table, const std::string &field) const;
    int ValidatePath(const DataDonationSchema::DdRelationsPath &path) const;
};

} // namespace DistributedDB

#endif // DATA_DONATION_SQL_GENERATOR_H
