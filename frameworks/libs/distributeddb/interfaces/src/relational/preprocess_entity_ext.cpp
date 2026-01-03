/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Create Date: 2026
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
#include "preprocess_entity_ext.h"

#ifndef OMIT_JSON
#include <json/json.h>
#endif
#include "log_print.h"

namespace DistributedDB {

namespace {
constexpr int E_OK = 0;
constexpr int E_ERROR = 1;
}

constexpr int MAX_ENTITY_LEN = 1000 * 1024;  // 1024000字节
constexpr int UNRECOGNIZED_ENTITY_TYPE = 0;
// 创建实体字段
PreprocessEntityField CreateEntityField(const std::string &name, PreprocessEntityFieldType type) noexcept
{
    return PreprocessEntityField(name, type);
}

// 创建作业待办必选字段
std::vector<PreprocessEntityField> CreateHomeworkRequiredFields()
{
    return { CreateEntityField("subject", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("assignment_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("assignment_date", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("completion_date", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("assignment_description", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("teacher_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("issue_date", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT) };
}

// 创建作业待办可选字段
std::vector<PreprocessEntityField> CreateHomeworkOptionalFields()
{
    return { CreateEntityField("attachments", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("difficulty", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("score", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("status", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) };
}

// 创建活动任务待办必选字段
std::vector<PreprocessEntityField> CreateActivityRequiredFields()
{
    return { CreateEntityField("activity_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("activity_date", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("location", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("teacher_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("activity_description", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("issue_date", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT) };
}

// 创建活动任务待办可选字段
std::vector<PreprocessEntityField> CreateActivityOptionalFields()
{
    return { CreateEntityField("activity_type", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) };
}

// 创建取餐码必选字段
std::vector<PreprocessEntityField> CreateMealPickupRequiredFields()
{
    return { CreateEntityField("restaurant_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("meal_code", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("meal_description", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("order_id", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("estimated_time", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("reserved_time", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("meal_link", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) };
}

// 创建取餐码可选字段
std::vector<PreprocessEntityField> CreateMealPickupOptionalFields()
{
    return { CreateEntityField("expected_time", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("meal_status", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("pickup_location", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("qr_code_url", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) };
}

// 创建医院预约必选字段
std::vector<PreprocessEntityField> CreateHospitalRequiredFields()
{
    return { CreateEntityField("hospital_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("department_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("doctor_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("visit_date", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT),
        CreateEntityField("visit_time", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("visit_number", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("patient_name", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) };
}

// 创建医院预约可选字段
std::vector<PreprocessEntityField> CreateHospitalOptionalFields()
{
    return { CreateEntityField("clinic_type", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("visit_location", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("status", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT),
        CreateEntityField("hospital_address", PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) };
}

const std::vector<PreprocessEntitySchema> &GetPreprocessEntitySchemas()
{
    static const std::vector<PreprocessEntitySchema> SCHEMAS = []() {
        std::vector<PreprocessEntitySchema> temp;
        temp.reserve(5);
        std::vector<PreprocessEntityField> emptyFields;

        // 未识别实体类型
        temp.emplace_back(0, "未识别", emptyFields, emptyFields);

        // 作业待办实体类型
        temp.emplace_back(1, "作业待办", CreateHomeworkRequiredFields(), CreateHomeworkOptionalFields());

        // 活动任务待办实体类型
        temp.emplace_back(2, "活动任务待办", CreateActivityRequiredFields(), CreateActivityOptionalFields());

        // 取餐码实体类型
        temp.emplace_back(3, "取餐码", CreateMealPickupRequiredFields(), CreateMealPickupOptionalFields());

        // 医院预约实体类型
        temp.emplace_back(4, "医院预约", CreateHospitalRequiredFields(), CreateHospitalOptionalFields());

        return temp;
    }();

    return SCHEMAS;
}

/**
 * @brief 推断实体类型
 * @param jsonObj 解析后的JSON对象
 * @param entityType 输出的实体类型
 * @return 错误码
 */
static int InferEntityType(const Json::Value& jsonObj, int *entityType)
{
    if (!entityType) {
        LOGE("inferEntityType: entityType is null");
        return -E_ERROR;
    }
    bool foundRequiredFields = false;
    const std::vector<PreprocessEntitySchema> entitySchemas = GetPreprocessEntitySchemas();
    for (const auto& schema : entitySchemas) {
        // 跳过未识别类型
        if (schema.entityType == UNRECOGNIZED_ENTITY_TYPE) {
            continue;
        }
        bool allFieldsPresent = true;
        for (const auto& field : schema.requiredFields) {
            if (!jsonObj.isMember(field.name)) {
                allFieldsPresent = false;
                foundRequiredFields = true;
                break;
            }
            
            if (field.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT &&
                !jsonObj[field.name].isInt()) {
                allFieldsPresent = false;
                foundRequiredFields = true;
                break;
            }
            if (field.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT &&
                !jsonObj[field.name].isString()) {
                allFieldsPresent = false;
                foundRequiredFields = true;
                break;
            }
        }
        if (allFieldsPresent) {
            *entityType = schema.entityType;
            return E_OK;
        }
    }
    if (foundRequiredFields) {
        LOGE("inferEntityType: missing requiredFields");
        return -E_ERROR;
    }
    LOGW("inferEntityType: unrecognized entity type");
    return E_OK;
}

static int ParseEntityRequiredFields(const Json::Value &root, const PreprocessEntitySchema *schema,
    ParsedEntity &entity)
{
    // 验证和提取必选字段
    for (const auto& field : schema->requiredFields) {
        if (!root.isMember(field.name)) {
            LOGE("ParseEntityRequiredFields: missing required field: %s", field.name.c_str());
            return -E_ERROR;
        }
        
        PreprocessEntityFieldValue fieldValue;
        fieldValue.type = field.type;
        fieldValue.hasValue = true;
        fieldValue.isRequired = true;
        
        if (field.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT) {
            if (!root[field.name].isInt()) {
                LOGE("ParseEntityRequiredFields: field %s should be integer", field.name.c_str());
                return -E_ERROR;
            }
            fieldValue.intValue = root[field.name].asInt64();
        } else if (field.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) {
            if (!root[field.name].isString()) {
                LOGE("ParseEntityRequiredFields: field %s should be string", field.name.c_str());
                return -E_ERROR;
            }
            fieldValue.textValue = root[field.name].asString();
        } else {
            LOGE("ParseEntityRequiredFields: unknown field type for %s", field.name.c_str());
            return -E_ERROR;
        }
        
        entity.fields[field.name] = fieldValue;
    }

    return E_OK;
}

static int ParseOptionalFields(const Json::Value& root, const PreprocessEntitySchema *schema, ParsedEntity &entity)
{
    // 提取可选字段
    for (const auto& field : schema->optionalFields) {
        if (root.isMember(field.name)) {
            PreprocessEntityFieldValue fieldValue;
            fieldValue.type = field.type;
            fieldValue.hasValue = true;
            fieldValue.isRequired = false;
            
            if (field.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT &&
                root[field.name].isInt()) {
                fieldValue.intValue = root[field.name].asInt64();
                entity.fields[field.name] = fieldValue;
            } else if (field.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT &&
                root[field.name].isString()) {
                fieldValue.textValue = root[field.name].asString();
                entity.fields[field.name] = fieldValue;
            }
        }
    }
    
    return E_OK;
}

static int ParseEntityJsonInner(const Json::Value& root, ParsedEntity &entity)
{
    // 推断实体类型
    int entityType = 0;
    if (InferEntityType(root, &entityType) != E_OK) {
        LOGE("ParseEntityJsonInner: failed to infer entity type");
        return -E_ERROR;
    }

    const PreprocessEntitySchema* schema = nullptr;
    const std::vector<PreprocessEntitySchema> entitySchemas = GetPreprocessEntitySchemas();
    for (const auto& s : entitySchemas) {
        if (s.entityType == entityType) {
            schema = &s;
            break;
        }
    }

    if (!schema) {
        LOGE("ParseEntityJsonInner: no schema for entity type %d", entityType);
        return -E_ERROR;
    }

    entity.entityType = entityType;
    entity.fields.clear();

    if (ParseEntityRequiredFields(root, schema, entity) != E_OK) {
        LOGE("ParseEntityJsonInner: failed to parse required fields");
        return -E_ERROR;
    }

    if (ParseOptionalFields(root, schema, entity) != E_OK) {
        LOGE("ParseEntityJsonInner: failed to parse optional fields");
        return -E_ERROR;
    }

    return E_OK;
}

/**
 * @brief parse entity json to string
 * @param jsonStr json string
 * @param entity parsed entity
 * @return error code
 */
static int ParseEntityJson(const std::string& jsonStr, ParsedEntity &entity)
{
    if (jsonStr.empty()) {
        LOGE("parseEntityJson: empty JSON string");
        return -E_ERROR;
    }

    if (jsonStr.length() > MAX_ENTITY_LEN) {
        LOGE("parseEntityJson: JSON too long (%zu bytes)", jsonStr.length());
        return -E_ERROR;
    }

    // 解析JSON
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    std::string errs;
    
    if (!reader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.length(), &root, &errs)) {
        LOGE("parseEntityJson: JSON parse error: %s", errs.c_str());
        return -E_ERROR;
    }

    if (!root.isObject()) {
        LOGE("parseEntityJson: JSON root must be an object");
        return -E_ERROR;
    }

    return ParseEntityJsonInner(root, entity);
}

static int CompareEntitiesInner(const PreprocessEntitySchema *schema, const ParsedEntity &entity1,
    const ParsedEntity &entity2, bool &isDuplicate)
{
    // 比较所有必选字段
    for (const auto& field : schema->requiredFields) {
        auto it1 = entity1.fields.find(field.name);
        auto it2 = entity2.fields.find(field.name);
        if (it1 == entity1.fields.end() || it2 == entity2.fields.end()) {
            LOGE("compareEntities: required field %s not found", field.name.c_str());
            return E_ERROR;
        }

        const PreprocessEntityFieldValue& val1 = it1->second;
        const PreprocessEntityFieldValue& val2 = it2->second;
        
        if (val1.type != val2.type) {
            LOGE("compareEntities: field %s type mismatch", field.name.c_str());
            return E_ERROR;
        }
        
        if (val1.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_INT) {
            if (val1.intValue != val2.intValue) {
                return E_OK; // 值不同，不是重复实体
            }
        } else if (val1.type == PreprocessEntityFieldType::PREPROCESS_ENTITY_FIELD_TEXT) {
            if (val1.textValue != val2.textValue) {
                return E_OK; // 值不同，不是重复实体
            }
        }
    }
    
    isDuplicate = true;
    return E_OK;
}

/**
 * @brief 比较两个实体是否重复(必选字段完全相同)
 * @param entity1 实体1
 * @param entity2 实体2
 * @param isDuplicate 输出的比较结果
 * @return 错误码
 */
static int CompareEntities(const ParsedEntity& entity1, const ParsedEntity& entity2, bool &isDuplicate)
{
    isDuplicate = false;
    // 实体类型必须相同
    if (entity1.entityType != entity2.entityType) {
        return E_OK; // 类型不同，不是重复实体
    }
    if (entity1.entityType == UNRECOGNIZED_ENTITY_TYPE || entity2.entityType == UNRECOGNIZED_ENTITY_TYPE) {
        LOGW("compareEntities: unrecognized entity type");
        return E_OK;
    }

    // 获取对应的schema
    const PreprocessEntitySchema* schema1 = nullptr;
    const std::vector<PreprocessEntitySchema> entitySchemas = GetPreprocessEntitySchemas();
    for (const auto& s : entitySchemas) {
        if (s.entityType == entity1.entityType) {
            schema1 = &s;
            break;
        }
    }
    
    if (!schema1) {
        LOGE("compareEntities: no schema for entity type %d", entity1.entityType);
        return E_ERROR;
    }

    return CompareEntitiesInner(schema1, entity1, entity2, isDuplicate);
}

/**
 * @brief is_entity_duplicate user defined function
 */
void IsEntityDuplicateImpl(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr) {
        return;
    }

    if (argc != 2 || argv == nullptr || argv[0] == nullptr || argv[1] == nullptr) { // 2 params count
        std::string errorMsg = "is_entity_duplicate expects 2 arguments: entity_json, input_entity_json";
        sqlite3_result_error(ctx, errorMsg.c_str(), -1);
        LOGE("%s", errorMsg.c_str());
        return;
    }

    const char* dbEntityJson = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    const char* inputEntityJson = reinterpret_cast<const char*>(sqlite3_value_text(argv[1]));
    if (!dbEntityJson || !inputEntityJson) {
        std::string errorMsg = "is_entity_duplicate expects not null value";
        return;
    }

    // 解析数据库实体
    ParsedEntity dbEntity = {0};
    if (ParseEntityJson(dbEntityJson, dbEntity) != E_OK) {
        sqlite3_result_error(ctx, "Failed to parse database entity", -1);
        return;
    }
    
    // 解析输入实体
    ParsedEntity inputEntity = {0};
    if (ParseEntityJson(inputEntityJson, inputEntity) != E_OK) {
        sqlite3_result_error(ctx, "Failed to parse input entity", -1);
        return;
    }
    
    // 比较实体是否重复
    bool isDuplicate = false;
    if (CompareEntities(dbEntity, inputEntity, isDuplicate) != E_OK) {
        sqlite3_result_error(ctx, "Failed to compare entities", -1);
        return;
    }
    
    // 返回结果
    sqlite3_result_int(ctx, isDuplicate ? 1 : 0);
}

} // namespace DistributedDB
