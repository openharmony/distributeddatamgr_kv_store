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

#include "schema_utils.h"
namespace DistributedDB {
void SchemaUtils::TrimFiled(std::string &inString)
{
    inString.erase(0, inString.find_first_not_of("\r\t "));
    size_t temp = inString.find_last_not_of("\r\t ");
    if (temp < inString.size()) {
        inString.erase(temp + 1);
    }
}

std::string SchemaUtils::Strip(const std::string &inString)
{
    std::string stripRes = inString;
    TrimFiled(stripRes);
    return stripRes;
}

std::string SchemaUtils::FieldTypeString(FieldType inType)
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

int SchemaUtils::ExtractJsonObj(const JsonObject &inJsonObject, const std::string &field,
    JsonObject &out)
{
    FieldType fieldType;
    auto fieldPath = FieldPath {field};
    int errCode = inJsonObject.GetFieldTypeByFieldPath(fieldPath, fieldType);
    if (errCode != E_OK) {
        LOGE("[SchemaUtils][ExtractJsonObj] Get schema %s fieldType failed: %d.", field.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::INTERNAL_FIELD_OBJECT != fieldType) {
        LOGE("[SchemaUtils][ExtractJsonObj] Expect %s Object but %s.", field.c_str(),
            SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    errCode = inJsonObject.GetObjectByFieldPath(fieldPath, out);
    if (errCode != E_OK) {
        LOGE("[SchemaUtils][ExtractJsonObj] Get schema %s value failed: %d.", field.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    return E_OK;
}

int SchemaUtils::ExtractJsonObjArray(const JsonObject &inJsonObject, const std::string &field,
    std::vector<JsonObject> &out)
{
    FieldType fieldType;
    auto fieldPath = FieldPath {field};
    int errCode = inJsonObject.GetFieldTypeByFieldPath(fieldPath, fieldType);
    if (errCode != E_OK) {
        LOGE("[SchemaUtils][ExtractJsonObj] Get schema %s fieldType failed: %d.", field.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::LEAF_FIELD_ARRAY != fieldType) {
        LOGE("[SchemaUtils][ExtractJsonObj] Expect %s Object but %s.", field.c_str(),
            SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    errCode = inJsonObject.GetObjectArrayByFieldPath(fieldPath, out);
    if (errCode != E_OK) {
        LOGE("[SchemaUtils][ExtractJsonObj] Get schema %s value failed: %d.", field.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    return E_OK;
}
} // namespace DistributedDB
