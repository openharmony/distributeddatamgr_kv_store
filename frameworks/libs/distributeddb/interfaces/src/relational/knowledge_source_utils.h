/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef KNOWLEDGE_SOURCE_UTILS_H
#define KNOWLEDGE_SOURCE_UTILS_H

#include <cstdlib>
#include <string>
#include <utility>

#include "relational_schema_object.h"
#include "relational_store_client.h"

namespace DistributedDB {
class KnowledgeSourceUtils {
public:
    static int SetKnowledgeSourceSchema(sqlite3 *db, const KnowledgeSourceSchema &schema);

    static int RemoveKnowledgeTableSchema(sqlite3 *db, const std::string &tableName);

    static int CleanDeletedData(sqlite3 *db, const std::string &tableName, uint64_t cursor);
protected:
    static int CheckProcessSequence(sqlite3 *db, const std::string &tableName, const std::string &columnName);

    static int CheckColumnExists(sqlite3 *db, const std::string &tableName, const std::set<std::string> &columnNames);

    static int CheckSchemaFields(sqlite3 *db, const KnowledgeSourceSchema &schema);

    static int SetKnowledgeSourceSchemaInner(sqlite3 *db, const KnowledgeSourceSchema &schema);

    static int InitMeta(sqlite3 *db, const std::string &table);

    static std::pair<int, RelationalSchemaObject> GetKnowledgeSourceSchema(sqlite3 *db);

    static std::pair<int, RelationalSchemaObject> GetRDBSchema(sqlite3 *db, bool isTracker);

    static int SaveKnowledgeSourceSchema(sqlite3 *db, const RelationalSchemaObject &schema);

    static std::pair<int, bool> CheckSchemaValidAndChangeStatus(sqlite3 *db,
        const RelationalSchemaObject &knowledgeSchema,
        const KnowledgeSourceSchema &schema, const TableInfo &tableInfo);

    static bool IsSchemaValid(const KnowledgeSourceSchema &schema, const TableInfo &tableInfo);

    static bool IsTableInRDBSchema(const std::string &table, const RelationalSchemaObject &rdbSchema, bool isTracker);

    static bool IsSchemaChange(const RelationalSchemaObject &dbSchema, const KnowledgeSourceSchema &schema);

    static int InitLogTable(sqlite3 *db, const KnowledgeSourceSchema &schema, const TableInfo &tableInfo);

    static int UpdateFlagAndTriggerIfNeeded(sqlite3 *db, const TableInfo &table);

    static int CheckIfTriggerNeedsUpdate(sqlite3 *db, const std::string &triggerName, const std::string &tableName,
        bool &needUpdate);

    static int GetKnowledgeCursor(sqlite3 *db, const TableInfo &tableInfo, int64_t &cursor);

    static int UpdateKnowledgeFlag(sqlite3 *db, const TableInfo &tableInfo);
};
}

#endif // KNOWLEDGE_SOURCE_UTILS_H
