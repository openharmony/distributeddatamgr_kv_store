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
#include "relational_schema_object.h"

#include <algorithm>

#include "db_common.h"
#include "json_object.h"
#include "schema_constant.h"
#include "schema_utils.h"

namespace DistributedDB {
bool RelationalSchemaObject::IsSchemaValid() const
{
    return isValid_;
}

SchemaType RelationalSchemaObject::GetSchemaType() const
{
    return schemaType_;
}

std::string RelationalSchemaObject::ToSchemaString() const
{
    return schemaString_;
}

int RelationalSchemaObject::ParseFromSchemaString(const std::string &inSchemaString)
{
    if (isValid_) {
        return -E_NOT_PERMIT;
    }

    if (inSchemaString.empty() || inSchemaString.size() > SchemaConstant::SCHEMA_STRING_SIZE_LIMIT) {
        LOGE("[RelationalSchema][Parse] SchemaSize=%zu is invalid.", inSchemaString.size());
        return -E_INVALID_ARGS;
    }
    JsonObject schemaObj;
    int errCode = schemaObj.Parse(inSchemaString);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Schema json string parse failed: %d.", errCode);
        return errCode;
    }

    errCode = ParseRelationalSchema(schemaObj);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Parse to relational schema failed: %d.", errCode);
        return errCode;
    }

    schemaType_ = SchemaType::RELATIVE;
    schemaString_ = schemaObj.ToString();
    isValid_ = true;
    return E_OK;
}

void RelationalSchemaObject::GenerateSchemaString()
{
    schemaString_ = {};
    schemaString_ += "{";
    schemaString_ += R"("SCHEMA_VERSION":")" + schemaVersion_ + R"(",)";
    schemaString_ += R"("SCHEMA_TYPE":"RELATIVE",)";
    if (schemaVersion_ == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1) {
        std::string modeString = tableMode_ == DistributedTableMode::COLLABORATION ?
            SchemaConstant::KEYWORD_TABLE_COLLABORATION : SchemaConstant::KEYWORD_TABLE_SPLIT_DEVICE;
        schemaString_ += R"("TABLE_MODE":")" + modeString + R"(",)";
    }
    schemaString_ += R"("TABLES":[)";
    for (auto it = tables_.begin(); it != tables_.end(); it++) {
        if (it != tables_.begin()) {
            schemaString_ += ",";
        }
        schemaString_ += it->second.ToTableInfoString(schemaVersion_);
    }
    schemaString_ += R"(])";
    schemaString_ += GetReferencePropertyString();
    schemaString_ += "}";
}

void RelationalSchemaObject::AddRelationalTable(const TableInfo &table)
{
    tables_[table.GetTableName()] = table;
    isValid_ = true;
    if (table.GetPrimaryKey().size() > 1 && table.GetTableSyncType() != CLOUD_COOPERATION) {
        // Table with composite primary keys
        // Composite primary keys are supported since version 2.1
        schemaVersion_ = SchemaConstant::SCHEMA_CURRENT_VERSION;
    }
    GenerateSchemaString();
}

void RelationalSchemaObject::RemoveRelationalTable(const std::string &tableName)
{
    tables_.erase(tableName);
    GenerateSchemaString();
}

const TableInfoMap &RelationalSchemaObject::GetTables() const
{
    return tables_;
}

std::vector<std::string> RelationalSchemaObject::GetTableNames() const
{
    std::vector<std::string> tableNames;
    for (const auto &it : tables_) {
        tableNames.emplace_back(it.first);
    }
    return tableNames;
}

TableInfo RelationalSchemaObject::GetTable(const std::string &tableName) const
{
    auto it = tables_.find(tableName);
    if (it != tables_.end()) {
        return it->second;
    }
    return {};
}

std::string RelationalSchemaObject::GetSchemaVersion() const
{
    return schemaVersion_;
}

DistributedTableMode RelationalSchemaObject::GetTableMode() const
{
    return tableMode_;
}

void RelationalSchemaObject::SetTableMode(DistributedTableMode mode)
{
    tableMode_ = mode;
    if (tableMode_ == DistributedTableMode::COLLABORATION) {
        schemaVersion_ = SchemaConstant::SCHEMA_CURRENT_VERSION;
    }
    GenerateSchemaString();
}

void RelationalSchemaObject::InsertTrackerSchema(const TrackerSchema &schema)
{
    TrackerTable table;
    table.Init(schema);
    trackerTables_[schema.tableName].SetTrackerTable(table);
    GenerateTrackerSchemaString();
}

void RelationalSchemaObject::RemoveTrackerSchema(const TrackerSchema &schema)
{
    trackerTables_.erase(schema.tableName);
    GenerateTrackerSchemaString();
}

void RelationalSchemaObject::GenerateTrackerSchemaString()
{
    schemaString_ = {};
    schemaString_ += "{";
    schemaString_ += R"("SCHEMA_TYPE":"TRACKER",)";
    schemaString_ += R"("TABLES":[)";
    for (auto it = trackerTables_.begin(); it != trackerTables_.end(); it++) {
        if (it != trackerTables_.begin()) {
            schemaString_ += ",";
        }
        schemaString_ += it->second.GetTrackerTable().ToString();
    }
    schemaString_ += R"(])";
    schemaString_ += "}";
}

std::string RelationalSchemaObject::GetReferencePropertyString()
{
    std::string res;
    if (!referenceProperty_.empty()) {
        res += R"(,"REFERENCE_PROPERTY":[)";
        for (const auto &reference : referenceProperty_) {
            res += GetOneReferenceString(reference) + ",";
        }
        res.pop_back();
        res += R"(])";
    }
    return res;
}

std::string RelationalSchemaObject::GetOneReferenceString(const TableReferenceProperty &reference)
{
    std::string res = R"({"SOURCE_TABLE_NAME":")";
    res += reference.sourceTableName;
    res += R"(","TARGET_TABLE_NAME":")";
    res += reference.targetTableName;
    res += R"(","COLUMNS":[)";
    for (const auto &item : reference.columns) {
        res += R"({"SOURCE_COL":")";
        res += item.first;
        res += R"(","TARGET_COL":")";
        res += item.second;
        res += R"("},)";
    }
    res.pop_back();
    res += R"(]})";
    return res;
}

static bool ColumnsCompare(const std::map<std::string, std::string> &left,
    const std::map<std::string, std::string> &right)
{
    if (left.size() != right.size()) {
        LOGE("[ColumnsCompare] column size not equal");
        return false;
    }
    std::map<std::string, std::string>::const_iterator leftIt = left.begin();
    std::map<std::string, std::string>::const_iterator rightIt = right.begin();
    for (; leftIt != left.end() && rightIt != right.end(); leftIt++, rightIt++) {
        if (strcasecmp(leftIt->first.c_str(), rightIt->first.c_str()) != 0 ||
            strcasecmp(leftIt->second.c_str(), rightIt->second.c_str()) != 0) {
            LOGE("[ColumnsCompare] column not equal");
            return false;
        }
    }
    return true;
}

static bool ReferenceCompare(const TableReferenceProperty &left, const TableReferenceProperty &right)
{
    if (strcasecmp(left.sourceTableName.c_str(), right.sourceTableName.c_str()) == 0 &&
        strcasecmp(left.targetTableName.c_str(), right.targetTableName.c_str()) == 0 &&
        ColumnsCompare(left.columns, right.columns)) {
        return true;
    }
    return false;
}

static void PropertyCompare(const std::vector<TableReferenceProperty> &left,
    const std::vector<TableReferenceProperty> &right, std::set<std::string> &changeTables)
{
    for (const auto &reference : left) {
        bool found = false;
        for (const auto &otherRef : right) {
            if (ReferenceCompare(reference, otherRef)) {
                found = true;
                break;
            }
        }
        if (!found) {
            changeTables.insert(reference.sourceTableName);
            changeTables.insert(reference.targetTableName);
        }
    }
}

std::set<std::string> RelationalSchemaObject::GetSharedTableForChangeTable(std::set<std::string> &changeTables) const
{
    std::set<std::string> res;
    TableInfoMap tableInfos = GetTables();
    for (const auto &changeName : changeTables) {
        for (const auto &item : tableInfos) {
            if (item.second.GetSharedTableMark() &&
                (strcasecmp(item.second.GetOriginTableName().c_str(), changeName.c_str()) == 0)) {
                res.insert(item.second.GetTableName()); // get shared table name
            }
        }
    }
    return res;
}

std::set<std::string> RelationalSchemaObject::CompareReferenceProperty(
    const std::vector<TableReferenceProperty> &others) const
{
    std::set<std::string> changeTables;
    PropertyCompare(referenceProperty_, others, changeTables);
    PropertyCompare(others, referenceProperty_, changeTables);
    if (!changeTables.empty()) { // get shared tables
        std::set<std::string> sharedTables = GetSharedTableForChangeTable(changeTables);
        changeTables.insert(sharedTables.begin(), sharedTables.end());
    }
    LOGI("[CompareReferenceProperty] changeTables size = %zu", changeTables.size());
    return changeTables;
}

std::map<std::string, std::map<std::string, bool>> RelationalSchemaObject::GetReachableRef()
{
    return reachableReference_;
}

std::map<std::string, int> RelationalSchemaObject::GetTableWeight()
{
    return tableWeight_;
}

TrackerTable RelationalSchemaObject::GetTrackerTable(const std::string &tableName) const
{
    auto it = trackerTables_.find(tableName);
    if (it != trackerTables_.end()) {
        return it->second.GetTrackerTable();
    }
    return {};
}

int RelationalSchemaObject::ParseFromTrackerSchemaString(const std::string &inSchemaString)
{
    JsonObject schemaObj;
    int errCode = schemaObj.Parse(inSchemaString);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Schema json string parse failed: %d.", errCode);
        return errCode;
    }

    errCode = ParseTrackerSchema(schemaObj);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Parse to tracker schema failed: %d.", errCode);
        return errCode;
    }

    schemaString_ = schemaObj.ToString();
    return E_OK;
}

const TableInfoMap &RelationalSchemaObject::GetTrackerTables() const
{
    return trackerTables_;
}

void RelationalSchemaObject::SetReferenceProperty(const std::vector<TableReferenceProperty> &referenceProperty)
{
    referenceProperty_ = referenceProperty;
    GenerateSchemaString();
    GenerateReachableRef();
    GenerateTableInfoReferenced();
}

const std::vector<TableReferenceProperty> &RelationalSchemaObject::GetReferenceProperty() const
{
    return referenceProperty_;
}

int RelationalSchemaObject::CompareAgainstSchemaObject(const std::string &inSchemaString,
    std::map<std::string, int> &cmpRst) const
{
    return E_OK;
}

int RelationalSchemaObject::CompareAgainstSchemaObject(const RelationalSchemaObject &inSchemaObject,
    std::map<std::string, int> &cmpRst) const
{
    return E_OK;
}

namespace {
int GetMemberFromJsonObject(const JsonObject &inJsonObject, const std::string &fieldName, FieldType expectType,
    bool isNecessary, FieldValue &fieldValue)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {fieldName})) {
        if (isNecessary) {
            LOGE("[RelationalSchema][Parse] Get schema %s not exist. isNecessary: %d", fieldName.c_str(), isNecessary);
            return -E_SCHEMA_PARSE_FAIL;
        }
        return -E_NOT_FOUND;
    }

    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {fieldName}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema %s fieldType failed: %d.", fieldName.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }

    if (fieldType != expectType) {
        LOGE("[RelationalSchema][Parse] Expect %s fieldType %d but: %d.", fieldName.c_str(),
            static_cast<int>(expectType), static_cast<int>(fieldType));
        return -E_SCHEMA_PARSE_FAIL;
    }

    errCode = inJsonObject.GetFieldValueByFieldPath(FieldPath {fieldName}, fieldValue);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema %s value failed: %d.", fieldName.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    return E_OK;
}
}

int RelationalSchemaObject::ParseTrackerSchema(const JsonObject &inJsonObject)
{
    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {SchemaConstant::KEYWORD_SCHEMA_TABLE}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get tracker schema TABLES fieldType failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::LEAF_FIELD_ARRAY != fieldType) {
        LOGE("[RelationalSchema][Parse] Expect tracker TABLES fieldType ARRAY but %s.",
             SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::vector<JsonObject> tables;
    errCode = inJsonObject.GetObjectArrayByFieldPath(FieldPath {SchemaConstant::KEYWORD_SCHEMA_TABLE}, tables);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get tracker schema TABLES value failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    for (const JsonObject &table : tables) {
        errCode = ParseCheckTrackerTable(table);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse schema TABLES failed: %d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTrackerTable(const JsonObject &inJsonObject)
{
    TrackerTable table;
    int errCode = ParseCheckTrackerTableName(inJsonObject, table);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTrackerExtendName(inJsonObject, table);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTrackerName(inJsonObject, table);
    if (errCode != E_OK) {
        return errCode;
    }
    trackerTables_[table.GetTableName()].SetTrackerTable(table);
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTrackerTableName(const JsonObject &inJsonObject, TrackerTable &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "NAME", FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode == E_OK) {
        if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
            LOGE("[RelationalSchema][Parse] Invalid characters in table name, err=%d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.SetTableName(fieldValue.stringValue);
    }
    return errCode;
}

int RelationalSchemaObject::ParseCheckTrackerExtendName(const JsonObject &inJsonObject, TrackerTable &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "EXTEND_NAME", FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode == E_OK) {
        if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
            LOGE("[RelationalSchema][Parse] Invalid characters in extend name, err=%d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.SetExtendName(fieldValue.stringValue);
    }
    return errCode;
}

int RelationalSchemaObject::ParseCheckTrackerName(const JsonObject &inJsonObject, TrackerTable &resultTable)
{
    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {"TRACKER_NAMES"}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get tracker col names fieldType failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::LEAF_FIELD_ARRAY != fieldType) {
        LOGE("[RelationalSchema][Parse] Expect tracker TABLES fieldType ARRAY but %s.",
            SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::vector<JsonObject> fieldValues;
    errCode = inJsonObject.GetObjectArrayByFieldPath(FieldPath{"TRACKER_NAMES"}, fieldValues);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get tracker col names value failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::set<std::string> colNames;
    for (const JsonObject &value : fieldValues) {
        FieldValue fieldValue;
        errCode = value.GetFieldValueByFieldPath(FieldPath {}, fieldValue);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse tracker col name failed: %d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        colNames.insert(fieldValue.stringValue);
    }
    resultTable.SetTrackerNames(colNames);
    return errCode;
}

int RelationalSchemaObject::ParseRelationalSchema(const JsonObject &inJsonObject)
{
    int errCode = ParseCheckSchemaVersion(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckSchemaType(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableMode(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckSchemaTableDefine(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    return ParseCheckReferenceProperty(inJsonObject);
}

namespace {
inline bool IsSchemaVersionValid(const std::string &version)
{
    std::string stripedVersion = SchemaUtils::Strip(version);
    return stripedVersion == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2 ||
        stripedVersion == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1;
}
}

int RelationalSchemaObject::ParseCheckSchemaVersion(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::KEYWORD_SCHEMA_VERSION,
        FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (IsSchemaVersionValid(fieldValue.stringValue)) {
        schemaVersion_ = fieldValue.stringValue;
        return E_OK;
    }

    LOGE("[RelationalSchema][Parse] Unexpected SCHEMA_VERSION=%s.", fieldValue.stringValue.c_str());
    return -E_SCHEMA_PARSE_FAIL;
}

int RelationalSchemaObject::ParseCheckSchemaType(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::KEYWORD_SCHEMA_TYPE,
        FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (SchemaUtils::Strip(fieldValue.stringValue) != SchemaConstant::KEYWORD_TYPE_RELATIVE) {
        LOGE("[RelationalSchema][Parse] Unexpected SCHEMA_TYPE=%s.", fieldValue.stringValue.c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    schemaType_ = SchemaType::RELATIVE;
    return E_OK;
}

namespace {
inline bool IsTableModeValid(const std::string &mode)
{
    std::string stripedMode = SchemaUtils::Strip(mode);
    return stripedMode == SchemaConstant::KEYWORD_TABLE_SPLIT_DEVICE ||
        stripedMode == SchemaConstant::KEYWORD_TABLE_COLLABORATION;
}
}

int RelationalSchemaObject::ParseCheckTableMode(const JsonObject &inJsonObject)
{
    if (schemaVersion_ == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2) {
        return E_OK; // version 2 has no table mode, no parsing required
    }

    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::KEYWORD_TABLE_MODE,
        FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (!IsTableModeValid(fieldValue.stringValue)) {
        LOGE("[RelationalSchema][Parse] Unexpected TABLE_MODE=%s.", fieldValue.stringValue.c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }

    tableMode_ = SchemaUtils::Strip(fieldValue.stringValue) == SchemaConstant::KEYWORD_TABLE_SPLIT_DEVICE ?
        DistributedDB::SPLIT_BY_DEVICE : DistributedTableMode::COLLABORATION;
    return E_OK;
}

int RelationalSchemaObject::ParseCheckSchemaTableDefine(const JsonObject &inJsonObject)
{
    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {SchemaConstant::KEYWORD_SCHEMA_TABLE}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES fieldType failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::LEAF_FIELD_ARRAY != fieldType) {
        LOGE("[RelationalSchema][Parse] Expect TABLES fieldType ARRAY but %s.",
            SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::vector<JsonObject> tables;
    errCode = inJsonObject.GetObjectArrayByFieldPath(FieldPath {SchemaConstant::KEYWORD_SCHEMA_TABLE}, tables);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES value failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    for (const JsonObject &table : tables) {
        errCode = ParseCheckTableInfo(table);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse schema TABLES failed: %d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableInfo(const JsonObject &inJsonObject)
{
    TableInfo resultTable;
    int errCode = ParseCheckTableName(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableDefine(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckOriginTableName(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableAutoInc(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckSharedTableMark(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTablePrimaryKey(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = ParseCheckTableSyncType(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableIndex(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableUnique(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    tables_[resultTable.GetTableName()] = resultTable;
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableName(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "NAME", FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode == E_OK) {
        if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
            LOGE("[RelationalSchema][Parse] Invalid characters in table name, err=%d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.SetTableName(fieldValue.stringValue);
    }
    return errCode;
}

int RelationalSchemaObject::ParseCheckTableDefine(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    std::map<FieldPath, FieldType> tableFields;
    int errCode = inJsonObject.GetSubFieldPathAndType(FieldPath {"DEFINE"}, tableFields);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES DEFINE failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }

    for (const auto &field : tableFields) {
        if (field.second != FieldType::INTERNAL_FIELD_OBJECT) {
            LOGE("[RelationalSchema][Parse] Expect schema TABLES DEFINE fieldType INTERNAL OBJECT but : %s.",
                SchemaUtils::FieldTypeString(field.second).c_str());
            return -E_SCHEMA_PARSE_FAIL;
        }

        JsonObject fieldObj;
        errCode = inJsonObject.GetObjectByFieldPath(field.first, fieldObj);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Get table field object failed. %d", errCode);
            return errCode;
        }

        if (!DBCommon::CheckIsAlnumOrUnderscore(field.first[1])) {
            LOGE("[RelationalSchema][Parse] Invalid characters in field name, err=%d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }

        FieldInfo fieldInfo;
        fieldInfo.SetFieldName(field.first[1]); // 1 : table name element in path
        errCode = ParseCheckTableFieldInfo(fieldObj, field.first, fieldInfo);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse table field info failed. %d", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.AddField(fieldInfo);
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableFieldInfo(const JsonObject &inJsonObject, const FieldPath &path,
    FieldInfo &field)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "COLUMN_ID", FieldType::LEAF_FIELD_INTEGER, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    field.SetColumnId(fieldValue.integerValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "TYPE", FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    field.SetDataType(fieldValue.stringValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "NOT_NULL", FieldType::LEAF_FIELD_BOOL, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    field.SetNotNull(fieldValue.boolValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "DEFAULT", FieldType::LEAF_FIELD_STRING, false, fieldValue);
    if (errCode == E_OK) {
        field.SetDefaultValue(fieldValue.stringValue);
    } else if (errCode != -E_NOT_FOUND) {
        return errCode;
    }

    return E_OK;
}

int RelationalSchemaObject::ParseCheckOriginTableName(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "ORIGINTABLENAME", FieldType::LEAF_FIELD_STRING,
        false, fieldValue);
    if (errCode == E_OK) {
        if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
            LOGE("[RelationalSchema][Parse] Invalid characters in origin table name, err=%d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.SetOriginTableName(fieldValue.stringValue);
    } else if (errCode != -E_NOT_FOUND) {
        LOGE("[RelationalSchema][Parse] Get schema orgin table name failed: %d", errCode);
        return errCode;
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableAutoInc(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "AUTOINCREMENT", FieldType::LEAF_FIELD_BOOL, false, fieldValue);
    if (errCode == E_OK) {
        resultTable.SetAutoIncrement(fieldValue.boolValue);
    } else if (errCode != -E_NOT_FOUND) {
        return errCode;
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckSharedTableMark(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "SHAREDTABLEMARK", FieldType::LEAF_FIELD_BOOL, false,
        fieldValue);
    if (errCode == E_OK) {
        resultTable.SetSharedTableMark(fieldValue.boolValue);
    } else if (errCode != -E_NOT_FOUND) {
        return errCode;
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTablePrimaryKey(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {"PRIMARY_KEY"})) {
        return E_OK;
    }

    FieldType type;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {"PRIMARY_KEY"}, type);
    if (errCode != E_OK) {
        return errCode;
    }

    if (type == FieldType::LEAF_FIELD_STRING) { // Compatible with schema 2.0
        FieldValue fieldValue;
        errCode = GetMemberFromJsonObject(inJsonObject, "PRIMARY_KEY", FieldType::LEAF_FIELD_STRING, false, fieldValue);
        if (errCode == E_OK) {
            resultTable.SetPrimaryKey(fieldValue.stringValue, 1);
        }
    } else if (type == FieldType::LEAF_FIELD_ARRAY) {
        CompositeFields multiPrimaryKey;
        errCode = inJsonObject.GetStringArrayByFieldPath(FieldPath {"PRIMARY_KEY"}, multiPrimaryKey);
        if (errCode == E_OK) {
            int index = 1; // primary key index
            for (const auto &item : multiPrimaryKey) {
                resultTable.SetPrimaryKey(item, index++);
            }
        }
    } else {
        errCode = -E_SCHEMA_PARSE_FAIL;
    }
    return errCode;
}

int RelationalSchemaObject::ParseCheckReferenceProperty(const JsonObject &inJsonObject)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {SchemaConstant::REFERENCE_PROPERTY})) {
        return E_OK;
    }

    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {SchemaConstant::REFERENCE_PROPERTY}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema REFERENCE_PROPERTY fieldType failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::LEAF_FIELD_ARRAY != fieldType) {
        LOGE("[RelationalSchema][Parse] Expect TABLES REFERENCE_PROPERTY ARRAY but %s.",
             SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::vector<JsonObject> references;
    errCode = inJsonObject.GetObjectArrayByFieldPath(FieldPath{SchemaConstant::REFERENCE_PROPERTY}, references);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema REFERENCE_PROPERTY value failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    for (const JsonObject &reference : references) {
        errCode = ParseCheckReference(reference);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse schema reference failed: %d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckReference(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::SOURCE_TABLE_NAME, FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get source table name failed, errCode = %d", errCode);
        return errCode;
    }
    if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
        LOGE("[RelationalSchema][Parse] Invalid characters in source table name.");
        return -E_SCHEMA_PARSE_FAIL;
    }

    TableReferenceProperty referenceProperty;
    referenceProperty.sourceTableName = fieldValue.stringValue;
    errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::TARGET_TABLE_NAME, FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get target table name failed, errCode = %d", errCode);
        return errCode;
    }
    if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
        LOGE("[RelationalSchema][Parse] Invalid characters in target table name.");
        return -E_SCHEMA_PARSE_FAIL;
    }

    referenceProperty.targetTableName = fieldValue.stringValue;
    errCode = ParseCheckReferenceColumns(inJsonObject, referenceProperty);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Parse reference columns failed, errCode = %d", errCode);
        return errCode;
    }
    referenceProperty_.emplace_back(referenceProperty);
    tables_[referenceProperty.targetTableName].AddTableReferenceProperty(referenceProperty);
    return E_OK;
}

int RelationalSchemaObject::ParseCheckReferenceColumns(const JsonObject &inJsonObject,
    TableReferenceProperty &tableReferenceProperty)
{
    // parse columns
    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {SchemaConstant::COLUMNS}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema reference COLUMNS fieldType failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::LEAF_FIELD_ARRAY != fieldType) {
        LOGE("[RelationalSchema][Parse] Expect reference COLUMNS ARRAY but %s.",
             SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::vector<JsonObject> columns;
    errCode = inJsonObject.GetObjectArrayByFieldPath(FieldPath{SchemaConstant::COLUMNS}, columns);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema reference COLUMNS value failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }

    for (const JsonObject &column : columns) {
        errCode = ParseCheckReferenceColumn(column, tableReferenceProperty);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse reference one COLUMN failed: %d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckReferenceColumn(const JsonObject &inJsonObject,
    TableReferenceProperty &tableReferenceProperty)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::SOURCE_COL, FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get source col failed, errCode = %d", errCode);
        return errCode;
    }
    if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
        LOGE("[RelationalSchema][Parse] Invalid characters in source col name.");
        return -E_SCHEMA_PARSE_FAIL;
    }

    std::string sourceCol = fieldValue.stringValue;
    errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::TARGET_COL, FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get target col failed, errCode = %d", errCode);
        return errCode;
    }
    if (!DBCommon::CheckIsAlnumOrUnderscore(fieldValue.stringValue)) {
        LOGE("[RelationalSchema][Parse] Invalid characters in target col name.");
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::string targetCol = fieldValue.stringValue;
    tableReferenceProperty.columns[sourceCol] = targetCol;
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableSyncType(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "TABLE_SYNC_TYPE", FieldType::LEAF_FIELD_INTEGER,
        false, fieldValue);
    if (errCode == E_OK) {
        resultTable.SetTableSyncType(static_cast<TableSyncType>(fieldValue.integerValue));
    } else if (errCode != -E_NOT_FOUND) {
        return errCode;
    }
    return E_OK; // if there is no "TABLE_SYNC_TYPE" filed, the table_sync_type is DEVICE_COOPERATION
}

int RelationalSchemaObject::ParseCheckTableIndex(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {"INDEX"})) { // INDEX is not necessary
        return E_OK;
    }
    std::map<FieldPath, FieldType> tableFields;
    int errCode = inJsonObject.GetSubFieldPathAndType(FieldPath {"INDEX"}, tableFields);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES INDEX failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }

    for (const auto &field : tableFields) {
        if (field.second != FieldType::LEAF_FIELD_ARRAY) {
            LOGE("[RelationalSchema][Parse] Expect schema TABLES INDEX fieldType ARRAY but : %s.",
                SchemaUtils::FieldTypeString(field.second).c_str());
            return -E_SCHEMA_PARSE_FAIL;
        }
        CompositeFields indexDefine;
        errCode = inJsonObject.GetStringArrayByFieldPath(field.first, indexDefine);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Get schema TABLES INDEX field value failed: %d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.AddIndexDefine(field.first[1], indexDefine); // 1 : second element in path
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableUnique(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {"UNIQUE"})) { // UNIQUE is not necessary
        return E_OK;
    }

    std::vector<CompositeFields> uniques;
    int errCode = inJsonObject.GetArrayContentOfStringOrStringArray(FieldPath {"UNIQUE"}, uniques);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES UNIQUE failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    resultTable.SetUniqueDefine(uniques);
    return E_OK;
}

void RelationalSchemaObject::GenerateReachableRef()
{
    reachableReference_.clear();
    tableWeight_.clear();
    std::set<std::string> startNodes; // such as {a->b->c,d->e}, record {a,d}
    std::map<std::string, std::set<std::string>> nextNodes; // such as {a->b->c}, record {{a,{b}}, {b, {c}}}
    // we need to record all table reachable reference here
    for (const auto &tableRef : referenceProperty_) {
        // they also can reach target
        RefreshReachableRef(tableRef);
        startNodes.insert(tableRef.sourceTableName);
        startNodes.erase(tableRef.targetTableName);
        nextNodes[tableRef.sourceTableName].insert(tableRef.targetTableName);
    }
    CalculateTableWeight(startNodes, nextNodes);
}

void RelationalSchemaObject::GenerateTableInfoReferenced()
{
    for (auto &table : tables_) {
        table.second.SetSourceTableReference({});
    }
    for (const auto &reference : referenceProperty_) {
        tables_[reference.targetTableName].AddTableReferenceProperty(reference);
    }
}

void RelationalSchemaObject::RefreshReachableRef(const TableReferenceProperty &referenceProperty)
{
    // such as source:A target:B
    std::set<std::string> recordSources;
    // find all node which can reach source as collection recordSources
    for (const auto &[start, end] : reachableReference_) {
        auto node = end.find(referenceProperty.sourceTableName);
        // find the node and it can reach
        if (node != end.end() && node->second) {
            recordSources.insert(start);
        }
    }
    recordSources.insert(referenceProperty.sourceTableName);
    // find all node which start with target as collection recordTargets
    std::set<std::string> recordTargets;
    for (auto &[entry, reach] : reachableReference_[referenceProperty.targetTableName]) {
        if (reach) {
            recordTargets.insert(entry);
        }
    }
    recordTargets.insert(referenceProperty.targetTableName);
    for (const auto &source : recordSources) {
        for (const auto &target : recordTargets) {
            reachableReference_[source][target] = true;
        }
    }
}

void RelationalSchemaObject::CalculateTableWeight(const std::set<std::string> &startNodes,
    const std::map<std::string, std::set<std::string>> &nextNodes)
{
    // record the max long path as table weight
    for (const auto &start : startNodes) {
        std::map<std::string, int> tmpTableWeight;
        tmpTableWeight[start] = 1;
        if (nextNodes.find(start) == nextNodes.end()) {
            continue;
        }
        std::list<std::string> queue;
        for (const auto &target : nextNodes.at(start)) {
            queue.push_back(target);
            tmpTableWeight[target] = 2; // this path contain 2 nodes
        }
        // bfs all the path which start from startNodes
        while (!queue.empty()) {
            auto node = queue.front();
            queue.pop_front();
            if (nextNodes.find(node) == nextNodes.end()) {
                continue;
            }
            for (const auto &item : nextNodes.at(node)) {
                queue.push_back(item);
                tmpTableWeight[item] = std::max(tmpTableWeight[item], tmpTableWeight[node] + 1);
            }
        }
        for (const auto &[table, weight] : tmpTableWeight) {
            tableWeight_[table] = std::max(tableWeight_[table], weight);
        }
    }
}
}
#endif