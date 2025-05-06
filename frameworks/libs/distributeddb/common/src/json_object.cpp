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

#include "json_object.h"

#include <algorithm>
#include <cmath>
#include <queue>

#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
#ifndef OMIT_JSON
namespace {
    const uint32_t MAX_NEST_DEPTH = 100;
#ifdef JSONCPP_USE_BUILDER
    const int JSON_VALUE_PRECISION = 16;
    const std::string JSON_CONFIG_INDENTATION = "indentation";
    const std::string JSON_CONFIG_COLLECT_COMMENTS = "collectComments";
    const std::string JSON_CONFIG_PRECISION = "precision";
    const std::string JSON_CONFIG_REJECT_DUP_KEYS = "rejectDupKeys";
#endif
}
uint32_t JsonObject::maxNestDepth_ = MAX_NEST_DEPTH;

uint32_t JsonObject::SetMaxNestDepth(uint32_t nestDepth)
{
    uint32_t preValue = maxNestDepth_;
    // No need to check the reasonability, only test code will use this method
    maxNestDepth_ = nestDepth;
    return preValue;
}

uint32_t JsonObject::CalculateNestDepth(const std::string &inString, int &errCode)
{
    std::vector<uint8_t> bytes;
    for (auto it = inString.begin(); it != inString.end(); ++it) {
        bytes.push_back(static_cast<uint8_t>(*it));
    }
    const uint8_t *begin = bytes.data();
    auto end = begin + inString.size();
    return CalculateNestDepth(begin, end, errCode);
}

uint32_t JsonObject::CalculateNestDepth(const uint8_t *dataBegin, const uint8_t *dataEnd, int &errCode)
{
    if (dataBegin == nullptr || dataEnd == nullptr || dataBegin >= dataEnd) {
        errCode = -E_INVALID_ARGS;
        return maxNestDepth_ + 1; // return a invalid depth
    }
    bool isInString = false;
    uint32_t maxDepth = 0;
    uint32_t objectDepth = 0;
    uint32_t arrayDepth = 0;
    uint32_t numOfEscape = 0;

    for (auto ptr = dataBegin; ptr < dataEnd; ptr++) {
        if (*ptr == '"' && numOfEscape % 2 == 0) { // 2 used to detect parity
            isInString = !isInString;
            continue;
        }
        if (!isInString) {
            if (*ptr == '{') {
                objectDepth++;
                maxDepth = std::max(maxDepth, objectDepth + arrayDepth);
            }
            if (*ptr == '}') {
                objectDepth = ((objectDepth > 0) ? (objectDepth - 1) : 0);
            }
            if (*ptr == '[') {
                arrayDepth++;
                maxDepth = std::max(maxDepth, objectDepth + arrayDepth);
            }
            if (*ptr == ']') {
                arrayDepth = ((arrayDepth > 0) ? (arrayDepth - 1) : 0);
            }
        }
        numOfEscape = ((*ptr == '\\') ? (numOfEscape + 1) : 0);
    }
    return maxDepth;
}

JsonObject::JsonObject(const JsonObject &other)
{
    isValid_ = other.isValid_;
    cjson_ = other.cjson_;
}

JsonObject& JsonObject::operator=(const JsonObject &other)
{
    if (&other != this) {
        isValid_ = other.isValid_;
        cjson_ = other.cjson_;
    }
    return *this;
}

JsonObject::JsonObject(const CJsonObject &value) : isValid_(true), cjson_(value)
{
}

int JsonObject::Parse(const std::string &inString)
{
    // The jsoncpp lib parser in strict mode will still regard root type jsonarray as valid, but we require jsonobject
    if (isValid_) {
        LOGE("[Json][Parse] Already Valid.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    uint32_t nestDepth = CalculateNestDepth(inString, errCode);
    if (errCode != E_OK || nestDepth > maxNestDepth_) {
        LOGE("[Json][Parse] Json calculate nest depth failed %d, depth=%" PRIu32 " exceed max allowed:%" PRIu32,
            errCode, nestDepth, maxNestDepth_);
        return -E_JSON_PARSE_FAIL;
    }
    cjson_ = CJsonObject::Parse(inString);
    // The jsoncpp lib parser in strict mode will still regard root type jsonarray as valid, but we require jsonobject
    if (!cjson_.IsObject()) {
        cjson_ = CJsonObject::CreateNullCJsonObject();
        LOGE("[Json][Parse] Not an object at root.");
        return -E_JSON_PARSE_FAIL;
    }
    isValid_ = true;
    return E_OK;
}

int JsonObject::Parse(const std::vector<uint8_t> &inData)
{
    if (inData.empty()) {
        return -E_INVALID_ARGS;
    }
    return Parse(inData.data(), inData.data() + inData.size());
}

int JsonObject::Parse(const uint8_t *dataBegin, const uint8_t *dataEnd)
{
    if (isValid_) {
        LOGE("[Json][Parse] Already Valid.");
        return -E_NOT_PERMIT;
    }
    if (dataBegin == nullptr || dataEnd == nullptr || dataBegin >= dataEnd) {
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    uint32_t nestDepth = CalculateNestDepth(dataBegin, dataEnd, errCode);
    if (errCode != E_OK || nestDepth > maxNestDepth_) {
        LOGE("[Json][Parse] Json calculate nest depth failed %d, depth:%" PRIu32 " exceed max allowed:%" PRIu32,
            errCode, nestDepth, maxNestDepth_);
        return -E_JSON_PARSE_FAIL;
    }
    cjson_ = CJsonObject::Parse(dataBegin, dataEnd);
    if (!cjson_.IsObject()) {
        cjson_ = CJsonObject::CreateNullCJsonObject();
        LOGE("[Json][Parse] Not an object at root.");
        return -E_JSON_PARSE_FAIL;
    }
    isValid_ = true;
    return E_OK;
}

bool JsonObject::IsValid() const
{
    return isValid_;
}

std::string JsonObject::ToString() const
{
    if (!isValid_) {
        LOGE("[Json][ToString] Not Valid Yet.");
        return {};
    }
    return cjson_.ToString();
}

bool JsonObject::IsFieldPathExist(const FieldPath &inPath) const
{
    if (!isValid_) {
        LOGE("[Json][isExisted] Not Valid Yet.");
        return false;
    }
    int errCode = E_OK;
    (void)GetCJsonValueByFieldPath(inPath, errCode); // Ignore return const reference
    return (errCode == E_OK);
}

int JsonObject::GetFieldTypeByFieldPath(const FieldPath &inPath, FieldType &outType) const
{
    if (!isValid_) {
        LOGE("[Json][GetType] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    return GetFieldTypeByCJsonValue(valueNode, outType);
}

int JsonObject::GetFieldValueByFieldPath(const FieldPath &inPath, FieldValue &outValue) const
{
    if (!isValid_) {
        LOGE("[Json][GetValue] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    FieldType valueType;
    errCode = GetFieldTypeByCJsonValue(valueNode, valueType);
    if (errCode != E_OK) {
        return errCode;
    }
    switch (valueType) {
        case FieldType::LEAF_FIELD_BOOL:
            outValue.boolValue = valueNode.GetBoolValue();
            break;
        case FieldType::LEAF_FIELD_INTEGER:
            outValue.integerValue = valueNode.GetInt32Value();
            break;
        case FieldType::LEAF_FIELD_LONG:
            outValue.longValue = valueNode.GetInt64Value();
            break;
        case FieldType::LEAF_FIELD_DOUBLE:
            outValue.doubleValue = valueNode.GetDoubleValue();
            break;
        case FieldType::LEAF_FIELD_STRING:
            outValue.stringValue = valueNode.GetStringValue();
            break;
        default:
            return -E_NOT_SUPPORT;
    }
    return E_OK;
}

int JsonObject::GetSubFieldPath(const FieldPath &inPath, std::set<FieldPath> &outSubPath) const
{
    if (!isValid_) {
        LOGE("[Json][GetSubPath] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!valueNode.IsObject()) {
        return -E_NOT_SUPPORT;
    }
    // Note: the subFields JsonCpp returnout will be different from each other
    std::vector<std::string> subFields = valueNode.GetMemberNames();
    for (const auto &eachSubField : subFields) {
        FieldPath eachSubPath = inPath;
        eachSubPath.emplace_back(eachSubField);
        outSubPath.insert(eachSubPath);
    }
    return E_OK;
}

int JsonObject::GetSubFieldPath(const std::set<FieldPath> &inPath, std::set<FieldPath> &outSubPath) const
{
    for (const auto &eachPath : inPath) {
        int errCode = GetSubFieldPath(eachPath, outSubPath);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int JsonObject::GetSubFieldPathAndType(const FieldPath &inPath, std::map<FieldPath, FieldType> &outSubPathType) const
{
    if (!isValid_) {
        LOGE("[Json][GetSubPathType] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!valueNode.IsObject()) {
        return -E_NOT_SUPPORT;
    }
    // Note: the subFields JsonCpp returnout will be different from each other
    std::vector<std::string> subFields = valueNode.GetMemberNames();
    for (const auto &eachSubField : subFields) {
        FieldPath eachSubPath = inPath;
        eachSubPath.push_back(eachSubField);
        FieldType eachSubType;
        errCode = GetFieldTypeByCJsonValue(valueNode[eachSubField], eachSubType);
        if (errCode != E_OK) {
            return errCode;
        }
        outSubPathType[eachSubPath] = eachSubType;
    }
    return E_OK;
}

int JsonObject::GetSubFieldPathAndType(const std::set<FieldPath> &inPath,
    std::map<FieldPath, FieldType> &outSubPathType) const
{
    for (const auto &eachPath : inPath) {
        int errCode = GetSubFieldPathAndType(eachPath, outSubPathType);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int JsonObject::GetArraySize(const FieldPath &inPath, uint32_t &outSize) const
{
    if (!isValid_) {
        LOGE("[Json][GetArraySize] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!valueNode.IsArray()) {
        return -E_NOT_SUPPORT;
    }
    outSize = valueNode.GetArraySize();
    return E_OK;
}

int JsonObject::GetArrayContentOfStringOrStringArray(const FieldPath &inPath,
    std::vector<std::vector<std::string>> &outContent) const
{
    if (!isValid_) {
        LOGE("[Json][GetArrayContent] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        LOGW("[Json][GetArrayContent] Get JsonValue Fail=%d.", errCode);
        return errCode;
    }
    if (!valueNode.IsArray()) {
        LOGE("[Json][GetArrayContent] Not an array.");
        return -E_NOT_SUPPORT;
    }
    if (valueNode.GetArraySize() > DBConstant::MAX_SET_VALUE_SIZE) {
        LOGE("[Json][GetArrayContent] Exceeds max value size.");
        return -E_NOT_SUPPORT;
    }
    for (uint32_t index = 0; index < valueNode.GetArraySize(); index++) {
        const auto eachArrayItem = valueNode[index];
        if (eachArrayItem.IsString()) {
            outContent.emplace_back(std::vector<std::string>({eachArrayItem.GetStringValue()}));
            continue;
        }
        if (eachArrayItem.IsArray()) {
            if (eachArrayItem.GetArraySize() == 0) {
                continue; // Ignore empty array-type member
            }
            outContent.emplace_back(std::vector<std::string>());
            errCode = GetStringArrayContentByCJsonValue(eachArrayItem, outContent.back());
            if (errCode == E_OK) {
                continue; // Everything ok
            }
        }
        // If reach here, then something is not ok
        outContent.clear();
        LOGE("[Json][GetArrayContent] Not string or array fail=%d at index:%" PRIu32, errCode, index);
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

namespace {
bool InsertFieldCheckParameter(const FieldPath &inPath, FieldType inType, const FieldValue &inValue,
    uint32_t maxNestDepth)
{
    if (inPath.empty() || inPath.size() > maxNestDepth || inType == FieldType::LEAF_FIELD_ARRAY ||
        inType == FieldType::INTERNAL_FIELD_OBJECT) {
        return false;
    }
    // Infinite double not support
    return !(inType == FieldType::LEAF_FIELD_DOUBLE && !std::isfinite(inValue.doubleValue));
}

void LeafJsonNodeAppendValue(CJsonObject &leafNode, FieldType inType, const FieldValue &inValue)
{
    if (inType == FieldType::LEAF_FIELD_STRING) {
        leafNode.Append(CJsonObject::CreateStrCJsonObject(inValue.stringValue));
    }
}

// Function design for InsertField call on a null-type
void LeafCJsonNodeAssignValue(CJsonObject &parentNode, FieldType inType,
    const std::string &fieldName, const FieldValue &inValue)
{
    switch (inType) {
        case FieldType::LEAF_FIELD_BOOL:
            parentNode.InsertOrReplaceField(fieldName, CJsonObject::CreateBoolCJsonObject(inValue.boolValue));
            break;
        case FieldType::LEAF_FIELD_INTEGER:
            // Cast to Json::Int to avoid "ambiguous call of overloaded function"
            parentNode.InsertOrReplaceField(fieldName, CJsonObject::CreateInt32CJsonObject(inValue.integerValue));
            break;
        case FieldType::LEAF_FIELD_LONG:
            // Cast to Json::Int64 to avoid "ambiguous call of overloaded function"
            parentNode.InsertOrReplaceField(fieldName, CJsonObject::CreateInt64CJsonObject(inValue.longValue));
            break;
        case FieldType::LEAF_FIELD_DOUBLE:
            parentNode.InsertOrReplaceField(fieldName, CJsonObject::CreateDoubleCJsonObject(inValue.doubleValue));
            break;
        case FieldType::LEAF_FIELD_STRING:
            parentNode.InsertOrReplaceField(fieldName, CJsonObject::CreateStrCJsonObject(inValue.stringValue));
            break;
        case FieldType::LEAF_FIELD_OBJECT:
            parentNode.InsertOrReplaceField(fieldName, CJsonObject::CreateObjCJsonObject());
            break;
        default:
            // For LEAF_FIELD_NULL, Do nothing.
            // For LEAF_FIELD_ARRAY and INTERNAL_FIELD_OBJECT, Not Support, had been excluded by InsertField
            return;
    }
}
}

int JsonObject::MoveToPath(const FieldPath &inPath, CJsonObject &exact, CJsonObject &nearest)
{
    uint32_t nearDepth = 0;
    int errCode = LocateCJsonValueByFieldPath(inPath, exact, nearest, nearDepth);
    if (errCode != -E_NOT_FOUND) { // Path already exist and it's not an array object
        return -E_JSON_INSERT_PATH_EXIST;
    }
    // nearDepth 0 represent for root value. nearDepth equal to inPath.size indicate an exact path match
    if (nearest.IsNull() || nearDepth >= inPath.size()) { // Impossible
        return -E_INTERNAL_ERROR;
    }
    if (!nearest.IsObject()) { // path ends with type not object
        return -E_JSON_INSERT_PATH_CONFLICT;
    }
    // Use nearDepth as startIndex pointing to the first field that lacked
    exact = nearest;
    for (uint32_t lackFieldIndex = nearDepth; lackFieldIndex < inPath.size(); lackFieldIndex++) {
        // The new JsonValue is null-type, we can safely add members to an null-type JsonValue which will turn into
        // object-type after member adding. Then move "nearest" to point to the new JsonValue.
        nearest = exact;
        exact = (nearest)[inPath[lackFieldIndex]];
    }
    return E_OK;
}

int JsonObject::InsertField(const FieldPath &inPath, FieldType inType, const FieldValue &inValue, bool isAppend)
{
    if (!InsertFieldCheckParameter(inPath, inType, inValue, maxNestDepth_)) {
        return -E_INVALID_ARGS;
    }
    if (!isValid_) {
        // Insert on invalid object never fail after parameter check ok, so here no need concern rollback.
        cjson_ = CJsonObject::CreateObjCJsonObject();
        isValid_ = true;
    }
    CJsonObject exact;
    CJsonObject nearest;
    int errCode = MoveToPath(inPath, exact, nearest);
    if (errCode != E_OK) {
        return errCode;
    }
    // Here "nearest" points to the JsonValue(null-type now) corresponding to the last field
    if (isAppend || exact.IsArray()) {
        LeafJsonNodeAppendValue(exact, inType, inValue);
    } else {
        LeafCJsonNodeAssignValue(nearest, inType, inPath[inPath.size() - 1], inValue);
    }
    return E_OK;
}

int JsonObject::DeleteField(const FieldPath &inPath)
{
    if (!isValid_) {
        LOGE("[Json][DeleteField] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    if (inPath.empty()) {
        return -E_INVALID_ARGS;
    }
    CJsonObject exact;
    CJsonObject nearest;
    uint32_t nearDepth = 0;
    int errCode = LocateCJsonValueByFieldPath(inPath, exact, nearest, nearDepth);
    if (errCode != E_OK) { // Path not exist
        return -E_JSON_DELETE_PATH_NOT_FOUND;
    }
    // nearDepth should be equal to inPath.size() - 1, because nearest is at the parent path of inPath
    if (nearest.IsNull() || !nearest.IsObject() || nearDepth != inPath.size() - 1) {
        return -E_INTERNAL_ERROR; // Impossible
    }
    // Remove member from nearest, ignore returned removed Value, use nearDepth as index pointing to last field of path.
    nearest.RemoveMember(inPath[nearDepth]);
    return E_OK;
}

int JsonObject::GetStringArrayContentByCJsonValue(const CJsonObject &value,
    std::vector<std::string> &outStringArray) const
{
    if (!value.IsArray()) {
        LOGE("[Json][GetStringArrayByValue] Not an array.");
        return -E_NOT_SUPPORT;
    }
    if (value.GetArraySize() > DBConstant::MAX_SET_VALUE_SIZE) {
        LOGE("[Json][GetStringArrayByValue] Exceeds max value size.");
        return -E_NOT_SUPPORT;
    }
    for (uint32_t index = 0; index < value.GetArraySize(); index++) {
        CJsonObject eachArrayItem = value[index];
        if (!eachArrayItem.IsString()) {
            LOGE("[Json][GetStringArrayByValue] Index=%u in Array is not string.", index);
            outStringArray.clear();
            return -E_NOT_SUPPORT;
        }
        outStringArray.push_back(eachArrayItem.GetStringValue());
    }
    return E_OK;
}

int JsonObject::GetFieldTypeByCJsonValue(const CJsonObject &value, FieldType &outType) const
{
    switch (value.GetType()) {
        case cJSON_NULL:
            outType = FieldType::LEAF_FIELD_NULL;
            break;
        case cJSON_False:
        case cJSON_True:
            outType = FieldType::LEAF_FIELD_BOOL;
            break;
            // The case intValue and uintValue cover from INT64_MIN to UINT64_MAX. Inside this range, isInt() take range
            // from INT32_MIN to INT32_MAX, which should be regard as LEAF_FIELD_INTEGER; isInt64() take range from
            // INT64_MIN to INT64_MAX, which should be regard as LEAF_FIELD_LONG if it is not LEAF_FIELD_INTEGER;
            // INT64_MAX + 1 to UINT64_MAX will be regard as LEAF_FIELD_DOUBLE, therefore lose its precision
            // when read out as double value.
        case cJSON_Number:
            if (value.IsInt32()) {
                outType = FieldType::LEAF_FIELD_INTEGER;
            } else if (value.IsInt64()) {
                outType = FieldType::LEAF_FIELD_LONG;
            } else {
                outType = FieldType::LEAF_FIELD_DOUBLE; // The isDouble() judge is always true in this case.
            }
            // The isDouble() judge is always true in this case. A value exceed double range is not support.
            if (value.IsInfiniteDouble()) {
                LOGE("[Json][GetTypeByJson] Infinite double not support.");
                return -E_NOT_SUPPORT;
            }
            break;
            // Integral value beyond range INT64_MIN to UINT64_MAX will be recognized as realValue
            // and lose its precision.
            // Value in scientific notation or has decimal point will be recognized as realValue without exception,
            // no matter whether the value is large or small, no matter with or without non-zero decimal part.
            // In a word, when regard as DOUBLE type, a value can not guarantee its presision
        case cJSON_String:
            outType = FieldType::LEAF_FIELD_STRING;
            break;
        case cJSON_Array:
            outType = FieldType::LEAF_FIELD_ARRAY;
            break;
        case cJSON_Object:
            if (value.GetMemberNames().empty()) {
                outType = FieldType::LEAF_FIELD_OBJECT;
                break;
            }
            outType = FieldType::INTERNAL_FIELD_OBJECT;
            break;
        default:
            LOGE("[Json][GetTypeByJson] no such type.");
            return -E_NOT_SUPPORT;
    }
    return E_OK;
}

CJsonObject JsonObject::GetCJsonValueByFieldPath(const FieldPath &inPath, int &errCode) const
{
    // Root path always exist
    if (inPath.empty()) {
        errCode = E_OK;
        return CJsonObject::CreateTempCJsonObject(cjson_);
    }
    CJsonObject valueNode = CJsonObject::CreateTempCJsonObject(cjson_);
    for (const auto &eachPathSegment : inPath) {
        if (!valueNode.IsObject() || (!valueNode.IsMember(eachPathSegment))) {
            // Current JsonValue is not an object, or no such member field
            errCode = -E_INVALID_PATH;
            return CJsonObject::CreateTempCJsonObject(cjson_);
        }
        valueNode = (valueNode)[eachPathSegment];
    }
    errCode = E_OK;
    return valueNode;
}

int JsonObject::LocateCJsonValueByFieldPath(const FieldPath &inPath, CJsonObject &exact, CJsonObject &nearest,
    uint32_t &nearDepth)
{
    if (!isValid_) {
        return -E_NOT_PERMIT;
    }
    exact = CJsonObject::CreateTempCJsonObject(cjson_);
    nearest = CJsonObject::CreateTempCJsonObject(cjson_);
    nearDepth = 0;
    if (inPath.empty()) {
        return E_OK;
    }
    for (const auto &eachPathSegment : inPath) {
        nearest = exact; // Let "nearest" trace "exact" before "exact" go deeper
        if (nearest != cjson_) {
            nearDepth++; // For each "nearest" trace up "exact", increase nearDepth to indicate where it is.
        }
        if (!exact.IsObject() || (!exact.IsMember(eachPathSegment))) {
            // "exact" is not an object, or no such member field
            exact = CJsonObject::CreateNullCJsonObject(); // Set "exact" to nullptr indicate exact path not exist
            return -E_NOT_FOUND;
        }
        exact = exact[eachPathSegment]; // "exact" go deeper
    }
    if (exact.IsArray()) {
        return -E_NOT_FOUND; // could append value if path is an array field.
    }
    // Here, JsonValue exist at exact path, "nearest" is "exact" parent.
    return E_OK;
}

int JsonObject::GetObjectArrayByFieldPath(const FieldPath &inPath, std::vector<JsonObject> &outArray) const
{
    if (!isValid_) {
        LOGE("[Json][GetValue] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        LOGE("[Json][GetValue] Get json value failed. %d", errCode);
        return errCode;
    }

    if (!valueNode.IsArray()) {
        LOGE("[Json][GetValue] Not Array type.");
        return -E_NOT_PERMIT;
    }
    if (valueNode.GetArraySize() > DBConstant::MAX_SET_VALUE_SIZE) {
        LOGE("[Json][GetValue] Exceeds max value size.");
        return -E_NOT_PERMIT;
    }
    for (size_t i = 0; i < valueNode.GetArraySize(); ++i) {
        outArray.emplace_back(JsonObject(valueNode[i]));
    }
    return E_OK;
}

int JsonObject::GetObjectByFieldPath(const FieldPath &inPath, JsonObject &outObj) const
{
    if (!isValid_) {
        LOGE("[Json][GetValue] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        LOGE("[Json][GetValue] Get json value failed. %d", errCode);
        return errCode;
    }

    if (!valueNode.IsObject()) {
        LOGE("[Json][GetValue] Not Object type.");
        return -E_NOT_PERMIT;
    }
    outObj = JsonObject(valueNode);
    return E_OK;
}

int JsonObject::GetStringArrayByFieldPath(const FieldPath &inPath, std::vector<std::string> &outArray) const
{
    if (!isValid_) {
        LOGE("[Json][GetValue] Not Valid Yet.");
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    const auto valueNode = GetCJsonValueByFieldPath(inPath, errCode);
    if (errCode != E_OK) {
        LOGE("[Json][GetValue] Get json value failed. %d", errCode);
        return errCode;
    }
    return GetStringArrayContentByCJsonValue(valueNode, outArray);
}

#else // OMIT_JSON
uint32_t JsonObject::SetMaxNestDepth(uint32_t nestDepth)
{
    (void)nestDepth;
    return 0;
}

uint32_t JsonObject::CalculateNestDepth(const std::string &inString, int &errCode)
{
    (void)inString;
    (void)errCode;
    return 0;
}

uint32_t JsonObject::CalculateNestDepth(const uint8_t *dataBegin, const uint8_t *dataEnd, int &errCode)
{
    (void)dataBegin;
    (void)dataEnd;
    (void)errCode;
    return 0;
}

JsonObject::JsonObject(const JsonObject &other) = default;

JsonObject& JsonObject::operator=(const JsonObject &other) = default;

int JsonObject::Parse(const std::string &inString)
{
    (void)inString;
    LOGW("[Json][Parse] Json Omit From Compile.");
    return -E_NOT_PERMIT;
}

int JsonObject::Parse(const std::vector<uint8_t> &inData)
{
    (void)inData;
    LOGW("[Json][Parse] Json Omit From Compile.");
    return -E_NOT_PERMIT;
}

int JsonObject::Parse(const uint8_t *dataBegin, const uint8_t *dataEnd)
{
    (void)dataBegin;
    (void)dataEnd;
    LOGW("[Json][Parse] Json Omit From Compile.");
    return -E_NOT_PERMIT;
}

bool JsonObject::IsValid() const
{
    return false;
}

std::string JsonObject::ToString() const
{
    return std::string();
}

bool JsonObject::IsFieldPathExist(const FieldPath &inPath) const
{
    (void)inPath;
    return false;
}

int JsonObject::GetFieldTypeByFieldPath(const FieldPath &inPath, FieldType &outType) const
{
    (void)inPath;
    (void)outType;
    return -E_NOT_PERMIT;
}

int JsonObject::GetFieldValueByFieldPath(const FieldPath &inPath, FieldValue &outValue) const
{
    (void)inPath;
    (void)outValue;
    return -E_NOT_PERMIT;
}

int JsonObject::GetSubFieldPath(const FieldPath &inPath, std::set<FieldPath> &outSubPath) const
{
    (void)inPath;
    (void)outSubPath;
    return -E_NOT_PERMIT;
}

int JsonObject::GetSubFieldPath(const std::set<FieldPath> &inPath, std::set<FieldPath> &outSubPath) const
{
    (void)inPath;
    (void)outSubPath;
    return -E_NOT_PERMIT;
}

int JsonObject::GetSubFieldPathAndType(const FieldPath &inPath, std::map<FieldPath, FieldType> &outSubPathType) const
{
    (void)inPath;
    (void)outSubPathType;
    return -E_NOT_PERMIT;
}

int JsonObject::GetSubFieldPathAndType(const std::set<FieldPath> &inPath,
    std::map<FieldPath, FieldType> &outSubPathType) const
{
    (void)inPath;
    (void)outSubPathType;
    return -E_NOT_PERMIT;
}

int JsonObject::GetArraySize(const FieldPath &inPath, uint32_t &outSize) const
{
    (void)inPath;
    (void)outSize;
    return -E_NOT_PERMIT;
}

int JsonObject::GetArrayContentOfStringOrStringArray(const FieldPath &inPath,
    std::vector<std::vector<std::string>> &outContent) const
{
    (void)inPath;
    (void)outContent;
    return -E_NOT_PERMIT;
}

int JsonObject::InsertField(const FieldPath &inPath, FieldType inType, const FieldValue &inValue)
{
    (void)inPath;
    (void)inType;
    (void)inValue;
    return -E_NOT_PERMIT;
}

int JsonObject::InsertField(const FieldPath &inPath, const JsonObject &inValue, bool isAppend = false)
{
    (void)inPath;
    (void)inValue;
    (void)isAppend;
    return -E_NOT_PERMIT;
}

int JsonObject::DeleteField(const FieldPath &inPath)
{
    (void)inPath;
    return -E_NOT_PERMIT;
}

int JsonObject::GetArrayValueByFieldPath(const FieldPath &inPath, JsonObject &outArray) const
{
    (void)inPath;
    (void)outArray;
    return -E_NOT_PERMIT;
}
#endif // OMIT_JSON
} // namespace DistributedDB
