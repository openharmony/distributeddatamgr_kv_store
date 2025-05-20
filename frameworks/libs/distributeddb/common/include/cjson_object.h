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

#ifndef CJSON_OBJECT_H
#define CJSON_OBJECT_H

#include <memory>
#ifndef OMIT_JSON
#include "cJSON.h"
#endif
#include "db_types.h"

namespace DistributedDB {
class CJsonObject final {
public:
    CJsonObject() = default;
    ~CJsonObject();
    void Free();
    explicit CJsonObject(cJSON *other, bool isOwner = true);
    CJsonObject(const CJsonObject &other);
    // copy cjson
    CJsonObject &operator=(const CJsonObject &other);
    CJsonObject &operator=(CJsonObject &&other) noexcept;
    CJsonObject operator[](const std::string &key) const;
    CJsonObject operator[](int index) const;
    bool operator==(const CJsonObject &other) const;
    bool operator!=(const CJsonObject &other) const;

    [[nodiscard]] int GetType() const;
    [[nodiscard]] size_t GetArraySize() const;
    [[nodiscard]] std::string GetStringValue() const;
    [[nodiscard]] bool GetBoolValue() const;
    [[nodiscard]] int32_t GetInt32Value() const;
    [[nodiscard]] int64_t GetInt64Value() const;
    [[nodiscard]] double GetDoubleValue() const;
    [[nodiscard]] std::vector<std::string> GetMemberNames() const;

    [[nodiscard]] bool IsMember(const std::string &key) const;
    [[nodiscard]] bool IsString() const;
    [[nodiscard]] bool IsArray() const;
    [[nodiscard]] bool IsObject() const;
    [[nodiscard]] bool IsNumber() const;
    [[nodiscard]] bool IsInt64() const;
    [[nodiscard]] bool IsInt32() const;
    [[nodiscard]] bool IsInfiniteDouble() const;
    [[nodiscard]] bool IsNull() const;
    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] bool IsBool() const;

    [[nodiscard]] std::string ToString() const;
    void Append(const CJsonObject &object);
    void RemoveMember(const std::string &path);
    void InsertOrReplaceField(const std::string &field, const CJsonObject &value);
    cJSON *GetObjectItem(const std::string &key) const;

    // copy cjson ptr only, not copy content
    static CJsonObject CreateTempCJsonObject(const CJsonObject &object);
    static CJsonObject CreateNullCJsonObject();
    static CJsonObject CreateObjCJsonObject();
    static CJsonObject CreateBoolCJsonObject(bool val);
    static CJsonObject CreateInt32CJsonObject(int32_t val);
    static CJsonObject CreateInt64CJsonObject(int64_t val);
    static CJsonObject CreateDoubleCJsonObject(double val);
    static CJsonObject CreateStrCJsonObject(const std::string &val);
    static CJsonObject CreateCJsonObject(cJSON *other);
    static CJsonObject Parse(const uint8_t *dataBegin, const uint8_t *dataEnd);
    static CJsonObject Parse(const std::string &json);
private:
#ifndef OMIT_JSON
    cJSON *cjson_ = nullptr;
    bool isOwner_ = true;
#endif
};
} // namespace DistributedDB
#endif // CJSON_OBJECT_H
