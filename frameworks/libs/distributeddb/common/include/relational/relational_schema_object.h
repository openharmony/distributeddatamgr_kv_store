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
#ifndef RELATIONAL_SCHEMA_OBJECT_H
#define RELATIONAL_SCHEMA_OBJECT_H
#ifdef RELATIONAL_STORE
#include <map>
#include "data_value.h"
#include "ischema.h"
#include "json_object.h"
#include "parcel.h"
#include "schema_constant.h"
#include "table_info.h"

namespace DistributedDB {
using TableInfoMap = std::map<std::string, TableInfo, CaseInsensitiveComparator>;
class RelationalSchemaObject : public ISchema {
public:
    RelationalSchemaObject() = default;
    ~RelationalSchemaObject() override = default;

    bool IsSchemaValid() const override;

    SchemaType GetSchemaType() const override;

    std::string ToSchemaString() const override;

    // Should be called on an invalid SchemaObject, create new SchemaObject if need to reparse
    int ParseFromSchemaString(const std::string &inSchemaString) override;

    void AddRelationalTable(const TableInfo &table);

    void RemoveRelationalTable(const std::string &tableName);

    const TableInfoMap &GetTables() const;

    std::vector<std::string> GetTableNames() const;

    TableInfo GetTable(const std::string& tableName) const;

    std::string GetSchemaVersion() const;

    DistributedTableMode GetTableMode() const;
    void SetTableMode(DistributedTableMode mode);

    void InsertTrackerSchema(const TrackerSchema &schema);
    void RemoveTrackerSchema(const TrackerSchema &schema);
    TrackerTable GetTrackerTable(const std::string &tableName) const;
    int ParseFromTrackerSchemaString(const std::string &inSchemaString);
    const TableInfoMap &GetTrackerTables() const;

    void SetReferenceProperty(const std::vector<TableReferenceProperty> &referenceProperty);
    const std::vector<TableReferenceProperty> &GetReferenceProperty() const;
    std::set<std::string> GetSharedTableForChangeTable(std::set<std::string> &changeTables) const;
    std::set<std::string> CompareReferenceProperty(const std::vector<TableReferenceProperty> &others) const;
    std::map<std::string, std::map<std::string, bool>> GetReachableRef();
    std::map<std::string, int> GetTableWeight();
private:
    int CompareAgainstSchemaObject(const std::string &inSchemaString, std::map<std::string, int> &cmpRst) const;

    int CompareAgainstSchemaObject(const RelationalSchemaObject &inSchemaObject,
        std::map<std::string, int> &cmpRst) const;

    int ParseRelationalSchema(const JsonObject &inJsonObject);
    int ParseCheckSchemaType(const JsonObject &inJsonObject);
    int ParseCheckTableMode(const JsonObject &inJsonObject);
    int ParseCheckSchemaVersion(const JsonObject &inJsonObject);
    int ParseCheckSchemaTableDefine(const JsonObject &inJsonObject);
    int ParseCheckTableInfo(const JsonObject &inJsonObject);
    int ParseCheckTableName(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckOriginTableName(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableDefine(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableFieldInfo(const JsonObject &inJsonObject, const FieldPath &path, FieldInfo &table);
    int ParseCheckTableAutoInc(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckSharedTableMark(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableSyncType(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableIndex(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableUnique(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTablePrimaryKey(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckReferenceProperty(const JsonObject &inJsonObject); // parse all reference
    int ParseCheckReference(const JsonObject &inJsonObject); // parse one reference
    // parse reference columns
    int ParseCheckReferenceColumns(const JsonObject &inJsonObject, TableReferenceProperty &tableReferenceProperty);
    // parse one reference column pair
    int ParseCheckReferenceColumn(const JsonObject &inJsonObject, TableReferenceProperty &tableReferenceProperty);

    void GenerateSchemaString();
    void GenerateTrackerSchemaString();
    std::string GetReferencePropertyString();
    std::string GetOneReferenceString(const TableReferenceProperty &reference);
    int ParseTrackerSchema(const JsonObject &inJsonObject);
    int ParseCheckTrackerTable(const JsonObject &inJsonObject);
    int ParseCheckTrackerTableName(const JsonObject &inJsonObject, TrackerTable &resultTable);
    int ParseCheckTrackerExtendName(const JsonObject &inJsonObject, TrackerTable &resultTable);
    int ParseCheckTrackerName(const JsonObject &inJsonObject, TrackerTable &resultTable);
    void GenerateReachableRef();
    void GenerateTableInfoReferenced();
    void RefreshReachableRef(const TableReferenceProperty &referenceProperty);
    void CalculateTableWeight(const std::set<std::string> &startNodes,
        const std::map<std::string, std::set<std::string>> &nextNodes);

    bool isValid_ = false; // set to true after parse success from string or add at least one relational table
    SchemaType schemaType_ = SchemaType::RELATIVE; // Default RELATIVE
    std::string schemaString_; // The minified and valid schemaString
    std::string schemaVersion_ = SchemaConstant::SCHEMA_SUPPORT_VERSION_V2; // Default version 2.0
    TableInfoMap tables_;
    TableInfoMap trackerTables_;
    std::vector<TableReferenceProperty> referenceProperty_;
    std::map<std::string, std::map<std::string, bool>> reachableReference_;
    std::map<std::string, int> tableWeight_;

    DistributedTableMode tableMode_ = DistributedTableMode::SPLIT_BY_DEVICE;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE
#endif // RELATIONAL_SCHEMA_OBJECT_H