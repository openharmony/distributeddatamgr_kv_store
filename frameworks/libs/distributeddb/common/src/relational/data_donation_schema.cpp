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
#ifdef RELATIONAL_STORE
#include "log_print.h"
#include "json_object.h"
#include "db_common.h"
#include "storage_engine.h"
#include "sqlite_single_ver_relational_storage_executor.h"
#include "data_donation_schema.h"

namespace DistributedDB {

int DataDonationSchema::Init(const std::string &schema)
{
    Clear();
    int ret = Decode(schema);
    if (ret != E_OK) {
        Clear();
    }
    return ret;
}

void DataDonationSchema::Clear()
{
    str.clear();
    triggers.clear();
    foreignKeys.clear();
    keysOut.clear();
    ddRelations.clear();
}

std::string DataDonationSchema::FieldTypeString(FieldType inType) const
{
    static std::map<FieldType, std::string> fieldTypeMapString = {
        {FieldType::LEAF_FIELD_NULL, "NULL"},
        {FieldType::LEAF_FIELD_BOOL, "BOOL"},
        {FieldType::LEAF_FIELD_INTEGER, "INTEGER"},
        {FieldType::LEAF_FIELD_LONG, "LONG"},
        {FieldType::LEAF_FIELD_DOUBLE, "DOUBLE"},
        {FieldType::LEAF_FIELD_STRING, "STRING"},
        {FieldType::LEAF_FIELD_ARRAY, "ARRAY"},
        {FieldType::LEAF_FIELD_OBJECT, "LEAF_OBJECT"},
        {FieldType::INTERNAL_FIELD_OBJECT, "INTERNAL_OBJECT"},
    };
    return fieldTypeMapString[inType];
}

int DataDonationSchema::DecodeWhereConditions(const JsonObject &mapping, DdKeyOut &keyOut) const
{
    std::vector<JsonObject> wheres;
    int errCode = DataDonationUtils::ExtractJsonObjArray(mapping, "where", wheres);
    if (errCode != E_OK) {
        return errCode;
    }
    for (auto where : wheres) {
        JsonObject equalTo;
        errCode = DataDonationUtils::ExtractJsonObj(where, "where", equalTo);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue conditionTable;
        errCode = where.GetFieldValueByFieldPath(FieldPath {"tableName"}, conditionTable);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue conditionColumn;
        errCode = where.GetFieldValueByFieldPath(FieldPath {"columnName"}, conditionColumn);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue conditionValue;
        errCode = where.GetFieldValueByFieldPath(FieldPath {"value"}, conditionValue);
        if (errCode != E_OK) {
            continue;
        }
        keyOut.condition.enable = true;
        keyOut.condition.field = {conditionTable.stringValue, conditionColumn.stringValue};
        keyOut.condition.value = conditionValue.integerValue;
    }
    return E_OK;
}

int DataDonationSchema::DecodePrimaryKeyMapping(const JsonObject &mapping, DdKeyOut &keyOut) const
{
    FieldValue primaryKey;
    int errCode = mapping.GetFieldValueByFieldPath(FieldPath {"primaryKey"}, primaryKey);
    if (errCode != E_OK || !primaryKey.boolValue) {
        return -E_INVALID_ARGS;
    }
    JsonObject value;
    errCode = DataDonationUtils::ExtractJsonObj(mapping, "value", value);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }
    FieldValue tableName;
    errCode = value.GetFieldValueByFieldPath(FieldPath {"tableName"}, tableName);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }
    FieldValue columnName;
    errCode = value.GetFieldValueByFieldPath(FieldPath {"columnName"}, columnName);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }
    keyOut.item = DdField {tableName.stringValue, columnName.stringValue};
    DecodeWhereConditions(mapping, keyOut);
    return E_OK;
}

int DataDonationSchema::DecodeKeysOut(const JsonObject &src)
{
    int errCode = E_OK;
    JsonObject searchConfig;
    errCode = DataDonationUtils::ExtractJsonObj(src, "searchConfig", searchConfig);
    if (errCode != E_OK) {
        LOGE("Plz check searchConfig. %d", errCode);
        return errCode;
    }

    std::vector<JsonObject> utdMappings;
    errCode = DataDonationUtils::ExtractJsonObjArray(searchConfig, "UTDMapping", utdMappings);
    if (errCode != E_OK) {
        LOGE("Plz check UTDMapping. %d", errCode);
        return errCode;
    }

    for (const auto &utdMapping : utdMappings) {
        std::vector<JsonObject> parts;
        errCode = DataDonationUtils::ExtractJsonObjArray(utdMapping, "parts", parts);
        if (errCode != E_OK) {
            LOGE("Plz check parts. %d", errCode);
            return errCode;
        }
        for (const auto &part : parts) {
            std::vector<JsonObject> mappings;
            errCode = DataDonationUtils::ExtractJsonObjArray(part, "mappings", mappings);
            if (errCode != E_OK) {
                continue;
            }
            DecodeMappings4KeyOut(mappings);
        }
    }
    return keysOut.empty() ? -E_INVALID_ARGS : E_OK;
}

void DataDonationSchema::DecodeMappings4KeyOut(const std::vector<JsonObject> &mappings)
{
    for (const auto &mapping : mappings) {
        FieldValue primaryKey;
        int errCode = mapping.GetFieldValueByFieldPath(FieldPath {"primaryKey"}, primaryKey);
        if (errCode != E_OK || !primaryKey.boolValue) {
            continue;
        }
        JsonObject value;
        errCode = DataDonationUtils::ExtractJsonObj(mapping, "value", value);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue tableName;
        errCode = value.GetFieldValueByFieldPath(FieldPath {"tableName"}, tableName);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue columnName;
        errCode = value.GetFieldValueByFieldPath(FieldPath {"columnName"}, columnName);
        if (errCode != E_OK) {
            continue;
        }
        DdKeyOut keyOut;
        keyOut.item = DdField {tableName.stringValue, columnName.stringValue};
        std::vector<JsonObject> wheres;
        errCode = DataDonationUtils::ExtractJsonObjArray(mapping, "where", wheres);
        if (errCode == E_OK) {
            DecodeWheres4KeyOut(wheres, keyOut);
        }
        keysOut.push_back(keyOut);
    }
}

void DataDonationSchema::DecodeWheres4KeyOut(const std::vector<JsonObject> &wheres, DdKeyOut &keyOut) const
{
    for (auto where : wheres) {
        JsonObject equalTo;
        int errCode = DataDonationUtils::ExtractJsonObj(where, "where", equalTo);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue conditionTable;
        errCode = where.GetFieldValueByFieldPath(FieldPath {"tableName"}, conditionTable);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue conditionColumn;
        errCode = where.GetFieldValueByFieldPath(FieldPath {"columnName"}, conditionColumn);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue conditionValue;
        errCode = where.GetFieldValueByFieldPath(FieldPath {"value"}, conditionValue);
        if (errCode != E_OK) {
            continue;
        }
        keyOut.condition.enable = true;
        keyOut.condition.field = {conditionTable.stringValue, conditionColumn.stringValue};
        keyOut.condition.value = conditionValue.integerValue;
    }
}

int DataDonationSchema::DecodePrimaryKeyFromField(const std::string &tableName, const JsonObject &field)
{
    FieldValue primaryKey;
    int errCode = field.GetFieldValueByFieldPath(FieldPath {"primaryKey"}, primaryKey);
    if (errCode != E_OK || !primaryKey.boolValue) {
        return -E_INVALID_ARGS;
    }
    FieldValue columnName;
    errCode = field.GetFieldValueByFieldPath(FieldPath {"columnName"}, columnName);
    if (errCode != E_OK) {
        LOGE("[DecodePrimaryKeyFromField] table %s parse primaryKey failed errcode %d",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
        return -E_INVALID_ARGS;
    }
    primaryKeys.insert({tableName, columnName.stringValue});
    return E_OK;
}

int DataDonationSchema::DecodeForeignKeyFromField(const std::string &tableName, const JsonObject &field)
{
    std::vector<JsonObject> foreignKeyArray;
    int errCode = DataDonationUtils::ExtractJsonObjArray(field, "foreignKey", foreignKeyArray);
    if (errCode != E_OK) {
        return errCode;
    }
    for (auto &foreignKey : foreignKeyArray) {
        FieldValue localColumnValue;
        errCode = field.GetFieldValueByFieldPath(FieldPath {"columnName"}, localColumnValue);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue foreignTableValue;
        errCode = foreignKey.GetFieldValueByFieldPath(FieldPath {"tableName"}, foreignTableValue);
        if (errCode != E_OK) {
            continue;
        }
        FieldValue foreignColumnValue;
        errCode = foreignKey.GetFieldValueByFieldPath(FieldPath {"columnName"}, foreignColumnValue);
        if (errCode != E_OK) {
            continue;
        }
        DdForeignKey ddForeignKey = {
            {tableName, localColumnValue.stringValue},
            {foreignTableValue.stringValue, foreignColumnValue.stringValue},
        };
        foreignKeys.insert({tableName, ddForeignKey});
    }
    return E_OK;
}

int DataDonationSchema::DecodeForeignKeys(const JsonObject &src)
{
    std::vector<JsonObject> dbSchemas;
    int errCode = DataDonationUtils::ExtractJsonObjArray(src, "dbSchema", dbSchemas);
    if (errCode != E_OK) {
        LOGE("Plz check dbSchema. %d", errCode);
        return errCode;
    }
    for (const auto &dbSchema : dbSchemas) {
        std::vector<JsonObject> tables;
        errCode = DataDonationUtils::ExtractJsonObjArray(dbSchema, "tables", tables);
        if (errCode != E_OK) {
            LOGE("Plz check dbSchema tables. %d", errCode);
            return errCode;
        }
        for (const auto &table : tables) {
            FieldValue localTableValue;
            errCode = table.GetFieldValueByFieldPath(FieldPath {"tableName"}, localTableValue);
            if (errCode != E_OK) {
                continue;
            }
            std::vector<JsonObject> fields;
            errCode = DataDonationUtils::ExtractJsonObjArray(table, "fields", fields);
            if (errCode != E_OK) {
                continue;
            }
            for (const auto &field : fields) {
                DecodeForeignKeyFromField(localTableValue.stringValue, field);
                DecodePrimaryKeyFromField(localTableValue.stringValue, field);
            }
        }
    }
    if (foreignKeys.empty()) {
        LOGE("dbSchema foreignKeys not found. %d", -E_INVALID_ARGS);
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int DataDonationSchema::MergeFields4Triggers(const JsonObject &arg, DdCondition &condition)
{
    FieldValue tableValue;
    int errCode = arg.GetFieldValueByFieldPath(FieldPath {"tableName"}, tableValue);
    if (errCode != E_OK) {
        return errCode;
    }
    FieldValue columnValue;
    errCode = arg.GetFieldValueByFieldPath(FieldPath {"columnName"}, columnValue);
    if (errCode != E_OK) {
        return errCode;
    }
    auto tableIter = triggers.find(tableValue.stringValue);
    if (tableIter == triggers.end()) {
        DdTrigger trigger;
        trigger.table = tableValue.stringValue;
        trigger.fields.insert({columnValue.stringValue, condition});
        triggers.insert({tableValue.stringValue, trigger});
    } else {
        auto fieldIter = tableIter->second.fields.find(columnValue.stringValue);
        if (fieldIter == tableIter->second.fields.end()) {
            tableIter->second.fields.insert({columnValue.stringValue, condition});
        }
    }
    return E_OK;
}

int DataDonationSchema::DecodeCondition4Triggers(const JsonObject &parent, DdCondition &condition) const
{
    std::vector<JsonObject> wheres;
    int errCode = DataDonationUtils::ExtractJsonObjArray(parent, "where", wheres);
    if (errCode != E_OK) {
        return errCode;
    }

    for (const auto &where : wheres) {
        JsonObject equalTo;
        errCode = DataDonationUtils::ExtractJsonObj(where, "equalto", equalTo);
        if (errCode != E_OK) {
            return errCode;
        }
        FieldValue tableValue;
        errCode = equalTo.GetFieldValueByFieldPath(FieldPath {"tableName"}, tableValue);
        if (errCode != E_OK) {
            return errCode;
        }
        FieldValue columnValue;
        errCode = equalTo.GetFieldValueByFieldPath(FieldPath {"columnName"}, columnValue);
        if (errCode != E_OK) {
            return errCode;
        }
        FieldValue valueValue;
        errCode = equalTo.GetFieldValueByFieldPath(FieldPath {"value"}, valueValue);
        if (errCode != E_OK) {
            return errCode;
        }
        condition = DdCondition {true, {tableValue.stringValue, columnValue.stringValue}, valueValue.integerValue};
    }

    return E_OK;
}

int DataDonationSchema::DecodeFunction4Triggers(const JsonObject &mapping, const JsonObject &function)
{
    std::vector<JsonObject> argList;
    int errCode = DataDonationUtils::ExtractJsonObjArray(function, "argList", argList);
    if (errCode != E_OK) {
        return errCode;
    }
    DdCondition condition;
    DecodeCondition4Triggers(mapping, condition);
    for (const auto &arg : argList) {
        MergeFields4Triggers(arg, condition);
    }
    return E_OK;
}

int DataDonationSchema::DecodeOthers4Triggers(const JsonObject &mapping)
{
    DdCondition condition;
    DecodeCondition4Triggers(mapping, condition);
    JsonObject value;
    int errCode = DataDonationUtils::ExtractJsonObj(mapping, "value", value);
    if (errCode == E_OK) {
        MergeFields4Triggers(value, condition);
        return E_OK;
    }
    std::vector<JsonObject> values;
    errCode = DataDonationUtils::ExtractJsonObjArray(mapping, "values", values);
    if (errCode == E_OK) {
        for (const auto &eachValue : values) {
            MergeFields4Triggers(eachValue, condition);
        }
    }
    return errCode;
}

int DataDonationSchema::DecodeTriggers(const JsonObject &src)
{
    JsonObject searchConfig;
    int errCode = DataDonationUtils::ExtractJsonObj(src, "searchConfig", searchConfig);
    if (errCode != E_OK) {
        LOGE("Plz check searchConfig. %d", errCode);
        return errCode;
    }
    std::vector<JsonObject> utdMappings;
    errCode = DataDonationUtils::ExtractJsonObjArray(searchConfig, "UTDMapping", utdMappings);
    if (errCode != E_OK) {
        LOGE("Plz check UTDMapping. %d", errCode);
        return errCode;
    }

    for (const auto &utdMapping : utdMappings) {
        std::vector<JsonObject> parts;
        errCode = DataDonationUtils::ExtractJsonObjArray(utdMapping, "parts", parts);
        if (errCode != E_OK) {
            continue;
        }
        for (const auto &part : parts) {
            std::vector<JsonObject> mappings;
            errCode = DataDonationUtils::ExtractJsonObjArray(part, "mappings", mappings);
            if (errCode != E_OK) {
                continue;
            }
            DecodeMappings4Trigger(mappings);
        }
    }
    if (triggers.empty()) {
        LOGE("Plz check searchConfig, mappings' sub item not found. %d", -E_INVALID_ARGS);
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

void DataDonationSchema::DecodeMappings4Trigger(const std::vector<JsonObject> &mappings)
{
    for (const auto &mapping : mappings) {
        JsonObject function;
        if (DataDonationUtils::ExtractJsonObj(mapping, "function", function) == E_OK) {
            DecodeFunction4Triggers(mapping, function);
        } else {
            DecodeOthers4Triggers(mapping);
        }
    }
}

// Purpose 1: check if the path is complete
// Purpose 2: insert the corresponding keyOut field
bool DataDonationSchema::InsertKeyOutAndCheckCompleted(DdRelationsPath &path)
{
    vector<DdKeyOut> unusedKeysOut = keysOut;
    for (auto &relation : path.relations) {
        for (auto keyOut = unusedKeysOut.begin(); keyOut != unusedKeysOut.end(); keyOut++) {
            if (keyOut->item.table == relation.key.localField.table) {
                relation.localField = keyOut->item;
            }

            if (keyOut->item.table == relation.key.foreignField.table) {
                relation.foreignField = keyOut->item;
            }
        }
    }

    for (auto &relation : path.relations) {
        for (auto keyOut = unusedKeysOut.begin(); keyOut != unusedKeysOut.end();) {
            if (keyOut->item.table == relation.key.localField.table) {
                keyOut = unusedKeysOut.erase(keyOut);
            } else {
                keyOut++;
            }
        }
    }

    auto relation = path.relations.back();
    for (auto keyOut = unusedKeysOut.begin(); keyOut != unusedKeysOut.end();) {
        if (keyOut->item.table == relation.key.foreignField.table) {
            keyOut = unusedKeysOut.erase(keyOut);
        } else {
            keyOut++;
        }
    }

    return unusedKeysOut.empty();
}

int DataDonationSchema::DecodeRelationsMaps()
{
    // build path from triggers
    for (auto trigger = triggers.begin(); trigger != triggers.end(); trigger++) {
        MergeRelationsMaps(trigger->second);
    }
    // filter incomplete paths
    for (auto path = ddRelations.begin(); path !=ddRelations.end();) {
        if (InsertKeyOutAndCheckCompleted(path->second)) {
            path++;
            continue;
        }
        path = ddRelations.erase(path);
    }

    if (ddRelations.empty()) {
        LOGE("Plz check dbSchema, relations to keysOut not found. %d", -E_INVALID_ARGS);
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

void DataDonationSchema::MergeRelationsMaps(DdTrigger &trigger)
{
    auto foreignKey = foreignKeys.find(trigger.table);
    if (foreignKey == foreignKeys.end()) {
        return;
    }
    auto path = ddRelations.find(trigger.table);
    if (path == ddRelations.end()) {
        vector<DdRelation> relations;
        DdRelation rel = {foreignKey->second, DdField{"", ""}, DdField{"", ""}};
        relations.push_back(rel);
        auto [newIt, success] = ddRelations.insert({trigger.table, {trigger.table, relations}});
        InsertKeyOutAndCheckCompleted(newIt->second);
        path = ddRelations.find(trigger.table); // update path after insert for later merge
    } else if (InsertKeyOutAndCheckCompleted(path->second)) {
        return;
    }
    // path cannot be empty
    size_t tryTimes = 10;
    while (!InsertKeyOutAndCheckCompleted(path->second) && (tryTimes-- > 0)) {
        auto subKey = path->second.relations.back();
        auto nextKey = foreignKeys.find(subKey.key.foreignField.table);
        if (nextKey == foreignKeys.end()) {
            ddRelations.erase(path);
            return;
        }
        DdRelation rel = {nextKey->second, DdField{"", ""}, DdField{"", ""}};
        path->second.relations.push_back(rel);
    }
}

int DataDonationSchema::Decode(const std::string &schema)
{
    int errCode = E_OK;
    if (schema.empty()) {
        LOGE("Data donation schema not found.");
        return -E_INVALID_ARGS;
    }

    JsonObject src;
    errCode = src.Parse(schema);
    if (errCode != E_OK || !src.IsValid()) {
        LOGE("Data donation schema is invalid json. %d", errCode);
        if (errCode == -E_JSON_PARSE_FAIL) {
            return -E_INVALID_ARGS;
        }
        return errCode;
    }

    errCode = DecodeKeysOut(src);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = DecodeForeignKeys(src);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = DecodeTriggers(src);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = DecodeRelationsMaps();
    if (errCode != E_OK) {
        return errCode;
    }

    // 4. for dfx
    str = schema;
    return E_OK;
}

bool DataDonationSchema::NeedWakeup(DdTrigger &trigger)
{
    auto path = GetRelationPath(trigger.table);
    if (path.relations.empty()) {
        return false;
    }

    auto triggerCfg = triggers.find(trigger.table);
    if (triggerCfg == triggers.end()) {
        return false;
    }

    for (auto &fieldIn : std::as_const(trigger.fields)) {
        auto subPath = GetRelationPath(fieldIn.second.field.table);
        auto range = triggerCfg->second.fields.equal_range(fieldIn.first);
        for (auto field = range.first; field != range.second; field++) {
            if (!field->second.enable) {
                return true;
            }
            if (subPath.relations.empty()) {
                continue;
            }
            if (field->second.field.table == fieldIn.second.field.table &&
                field->second.field.field == fieldIn.second.field.field &&
                field->second.value == fieldIn.second.value) {
                return true;
            }
        }
    }
    return false;
}

DataDonationSchema::DdRelationsPath& DataDonationSchema::GetRelationPath(const std::string &table)
{
    auto it = ddRelations.find(table);
    if (it != ddRelations.end()) {
        return it->second;
    }

    static DdRelationsPath emptyPath;
    return emptyPath;
}

std::string DataDonationSchema::GetPrimaryKey(const std::string &table) const
{
    auto it = primaryKeys.find(table);
    if (it != primaryKeys.end()) {
        return it->second;
    }

    return "";
}

DataDonationSchema::DdRelationsPath& DataDonationSchema::GetRelationPath()
{
    for (const auto &key : keysOut) {
        if (!key.condition.enable) {
            return GetRelationPath(key.item.table);
        }
    }
    LOGE("Data donation schema find default relation path failed. %d", E_NOT_FOUND);
    static DdRelationsPath emptyPath;
    return emptyPath;
}

std::vector<DataDonationSchema::DdKeyOut> DataDonationSchema::GetKeyOut()
{
    return keysOut;
}

}  // namespace DistributedDB

#endif
