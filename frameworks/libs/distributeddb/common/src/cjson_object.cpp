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
#include "cjson_object.h"
#include <cmath>

#include "schema_utils.h"

namespace DistributedDB {
namespace {
bool IsEqual(double a, double b)
{
    return std::abs(a - b) < std::numeric_limits<double>::epsilon();
}
}

CJsonObject::~CJsonObject()
{
    Free();
}

void CJsonObject::Free()
{
    if (isOwner_ && cjson_ != nullptr) {
        cJSON_Delete(cjson_);
        cjson_ = nullptr;
    }
}

CJsonObject::CJsonObject(cJSON *other, bool isOwner)
{
    if (isOwner && other != nullptr) {
        cjson_ = cJSON_Duplicate(other, true);
    } else if (!isOwner) {
        cjson_ = other;
    }
    isOwner_ = isOwner;
}

CJsonObject::CJsonObject(const CJsonObject &other)
    : CJsonObject(other.cjson_, other.isOwner_)
{
}

CJsonObject &CJsonObject::operator=(const CJsonObject &other)
{
    if (&other == this) {
        return *this;
    }
    if (other.cjson_ == nullptr) {
        cjson_ = nullptr;
        return *this;
    }
    if (other.isOwner_) {
        cjson_ = cJSON_Duplicate(other.cjson_, true);
    } else {
        cjson_ = other.cjson_;
    }
    isOwner_ = other.isOwner_;
    return *this;
}

CJsonObject &CJsonObject::operator=(CJsonObject &&other) noexcept
{
    Free();
    cjson_ = other.cjson_;
    isOwner_ = other.isOwner_;
    other.cjson_ = nullptr;
    other.isOwner_ = false;
    return *this;
}

CJsonObject CJsonObject::operator[](const std::string &key) const
{
    if (cjson_ == nullptr) {
        return *this;
    }
    if (!IsMember(key)) {
        auto obj = CreateObjCJsonObject();
        (void)cJSON_AddItemToObject(cjson_, key.c_str(), obj.cjson_);
        obj.isOwner_ = false;
        return obj;
    }
    // default cjson get first item when match
    // we should keep it same as json cpp which is getting last item
    auto rawCJsonPtr = GetObjectItem(key);
    return CJsonObject(rawCJsonPtr, false);
}

cJSON *CJsonObject::GetObjectItem(const std::string &key) const
{
    cJSON *res = nullptr;
    auto current = cjson_->child;
    while (current != nullptr) {
        if (current->string != nullptr && strcmp(current->string, key.c_str()) == 0) {
            res = current;
        }
        current = current->next;
    }
    return res;
}

CJsonObject CJsonObject::operator[](int index) const
{
    if (cjson_ == nullptr || !cJSON_IsArray(cjson_)) {
        return *this;
    }
    return CJsonObject(cJSON_GetArrayItem(cjson_, index), false);
}

bool CJsonObject::operator==(const CJsonObject &other) const
{
    return cJSON_Compare(cjson_, other.cjson_, 1) != 0; // 1 is case-sensitive
}

bool CJsonObject::operator!=(const DistributedDB::CJsonObject &other) const
{
    return !(*this == other);
}

int CJsonObject::GetType() const
{
    if (cjson_ == nullptr) {
        return cJSON_NULL;
    }
    return cjson_->type;
}

size_t CJsonObject::GetArraySize() const
{
    if (cjson_ == nullptr || !cJSON_IsArray(cjson_)) {
        return 0u;
    }
    return static_cast<size_t>(cJSON_GetArraySize(cjson_));
}

std::string CJsonObject::GetStringValue() const
{
    if (!IsString()) {
        return "";
    }
    return cJSON_GetStringValue(cjson_);
}

bool CJsonObject::GetBoolValue() const
{
    if (!IsBool()) {
        return false;
    }
    return cJSON_IsTrue(cjson_);
}

int32_t CJsonObject::GetInt32Value() const
{
    if (!IsInt32()) {
        return 0;
    }
    double val = cJSON_GetNumberValue(cjson_);
    return static_cast<int32_t>(val);
}

int32_t CJsonObject::GetInt64Value() const
{
    if (!IsInt64()) {
        return 0;
    }
    double val = cJSON_GetNumberValue(cjson_);
    return static_cast<int64_t>(val);
}

double CJsonObject::GetDoubleValue() const
{
    if (IsInfiniteDouble()) {
        return 0.0;
    }
    return cJSON_GetNumberValue(cjson_);
}

std::vector<std::string> CJsonObject::GetMemberNames() const
{
    std::vector<std::string> res;
    if (!IsObject()) {
        return res;
    }
    auto *child = cjson_->child;
    while (child != nullptr) {
        if (child->string != nullptr) {
            res.emplace_back(child->string);
        }
        child = child->next;
    }
    return res;
}

bool CJsonObject::IsMember(const std::string &key) const
{
    if (cjson_ == nullptr) {
        return false;
    }
    return cJSON_HasObjectItem(cjson_, key.c_str());
}

bool CJsonObject::IsString() const
{
    return cJSON_IsString(cjson_);
}

bool CJsonObject::IsArray() const
{
    return cJSON_IsArray(cjson_);
}

bool CJsonObject::IsObject() const
{
    return cJSON_IsObject(cjson_);
}

bool CJsonObject::IsNumber() const
{
    return cJSON_IsNumber(cjson_);
}

bool CJsonObject::IsInt64() const
{
    if (!IsNumber()) {
        return false;
    }
    double val = cJSON_GetNumberValue(cjson_);
    auto intVal = static_cast<int64_t>(val);
    return IsEqual(val, static_cast<double>(intVal));
}

bool CJsonObject::IsInt32() const
{
    if (!IsNumber()) {
        return false;
    }
    double val = cJSON_GetNumberValue(cjson_);
    auto intVal = static_cast<int32_t>(val);
    return IsEqual(val, static_cast<double>(intVal));
}

bool CJsonObject::IsInfiniteDouble() const
{
    if (!IsNumber()) {
        return false;
    }
    double val = cJSON_GetNumberValue(cjson_);
    return !std::isfinite(val);
}

bool CJsonObject::IsNull() const
{
    return cjson_ == nullptr;
}

bool CJsonObject::IsValid() const
{
    return !cJSON_IsInvalid(cjson_);
}

bool CJsonObject::IsBool() const
{
    return cJSON_IsBool(cjson_);
}

std::string CJsonObject::ToString() const
{
    if (!IsValid()) {
        return "";
    }
    auto str = cJSON_PrintUnformatted(cjson_);
    std::string res(str);
    cJSON_free(str);
    return res;
}

void CJsonObject::Append(const CJsonObject &object)
{
    if (!IsArray()) {
        return;
    }
    if (isOwner_) {
        CJsonObject cpyObject = object;
        cJSON_AddItemToArray(cjson_, cpyObject.cjson_);
    } else {
        cJSON_AddItemToArray(cjson_, object.cjson_);
    }
}

void CJsonObject::RemoveMember(const std::string &path)
{
    if (!IsObject()) {
        return;
    }
    cJSON_DeleteItemFromObject(cjson_, path.c_str());
}

void CJsonObject::InsertOrReplaceField(const std::string &field, const CJsonObject &value)
{
    if (IsNull() || value.IsNull() || !IsObject()) {
        return;
    }
    CJsonObject copy(value.cjson_, true);
    if (IsMember(field)) {
        cJSON_ReplaceItemInObject(cjson_, field.c_str(), copy.cjson_);
    } else {
        cJSON_AddItemToObject(cjson_, field.c_str(), copy.cjson_);
    }
    copy.isOwner_ = false;
}

CJsonObject CJsonObject::CreateTempCJsonObject(const CJsonObject &object)
{
    return CJsonObject(object.cjson_, false);
}

CJsonObject CJsonObject::CreateNullCJsonObject()
{
    return {};
}

CJsonObject CJsonObject::CreateObjCJsonObject()
{
    return CJsonObject::CreateCJsonObject(cJSON_CreateObject());
}

CJsonObject CJsonObject::CreateBoolCJsonObject(bool val)
{
    return CJsonObject::CreateCJsonObject(cJSON_CreateBool(val ? 1 : 0));
}

CJsonObject CJsonObject::CreateInt32CJsonObject(int32_t val)
{
    return CJsonObject::CreateCJsonObject(cJSON_CreateNumber(val * 1.0));
}

CJsonObject CJsonObject::CreateInt64CJsonObject(int64_t val)
{
    return CJsonObject::CreateCJsonObject(cJSON_CreateNumber(val * 1.0));
}

CJsonObject CJsonObject::CreateDoubleCJsonObject(double val)
{
    return CJsonObject::CreateCJsonObject(cJSON_CreateNumber(val));
}

CJsonObject CJsonObject::CreateStrCJsonObject(const std::string &val)
{
    return CJsonObject::CreateCJsonObject(cJSON_CreateString(val.data()));
}

CJsonObject CJsonObject::CreateCJsonObject(cJSON *other)
{
    CJsonObject obj;
    obj.cjson_ = other;
    obj.isOwner_ = true;
    return obj;
}

CJsonObject CJsonObject::Parse(const uint8_t *dataBegin, const uint8_t *dataEnd)
{
    if (dataBegin == nullptr || dataEnd == nullptr) {
        return {};
    }
    size_t len = dataEnd - dataBegin;
    auto begin = reinterpret_cast<const char *>(dataBegin);
    CJsonObject obj;
    obj.cjson_ = cJSON_ParseWithLength(begin, len);
    obj.isOwner_ = true;
    return obj;
}

CJsonObject CJsonObject::Parse(const std::string &json)
{
    CJsonObject obj;
    obj.cjson_ = cJSON_Parse(json.c_str());
    obj.isOwner_ = true;
    return obj;
}
}