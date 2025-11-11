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

#ifndef KNOWLEDGE_LOG_TABLE_MANAGER_H
#define KNOWLEDGE_LOG_TABLE_MANAGER_H

#include "simple_tracker_log_table_manager.h"

namespace DistributedDB {
class KnowledgeLogTableManager : public SimpleTrackerLogTableManager {
public:
    KnowledgeLogTableManager() = default;
    ~KnowledgeLogTableManager() override = default;

private:
    // inherit insert and delete trigger from tracker table, only update trigger is different
    std::string GetUpdateTrigger(const TableInfo &table, const std::string &identity) override;

    std::string GetDiffIncSql(const TableInfo &tableInfo);
};
} // DistributedDB

#endif // KNOWLEDGE_LOG_TABLE_MANAGER_H
