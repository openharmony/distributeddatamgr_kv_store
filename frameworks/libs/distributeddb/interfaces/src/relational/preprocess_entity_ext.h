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
#ifndef PREPROCESS_ENTITY_EXT_H
#define PREPROCESS_ENTITY_EXT_H

#include <string>
#include <unordered_map>
#include <vector>
// using the "sqlite3sym.h" in OHOS
#ifndef USE_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif

namespace DistributedDB {

enum class PreprocessEntityFieldType {
    PREPROCESS_ENTITY_FIELD_INT = 0,
    PREPROCESS_ENTITY_FIELD_TEXT,
    PREPROCESS_ENTITY_FIELD_BUTT
};

struct PreprocessEntityField {
    std::string name;
    PreprocessEntityFieldType type;
    PreprocessEntityField(const std::string &n, PreprocessEntityFieldType t) noexcept
        : name(n), type(t) {}
};

struct PreprocessEntitySchema {
    int entityType;
    std::string name;
    std::vector<PreprocessEntityField> requiredFields;
    std::vector<PreprocessEntityField> optionalFields;

    PreprocessEntitySchema(int type, const std::string &n,
                          const std::vector<PreprocessEntityField> &required,
                          const std::vector<PreprocessEntityField> &optional) noexcept
        : entityType(type), name(n), requiredFields(required), optionalFields(optional) {}
};

// 解析后的实体字段值
struct PreprocessEntityFieldValue {
    PreprocessEntityFieldType type;
    std::string textValue;
    int64_t intValue;
    bool hasValue;
    bool isRequired;
};
// 解析后的实体
struct ParsedEntity {
    int entityType;
    std::unordered_map<std::string, PreprocessEntityFieldValue> fields;
};

void IsEntityDuplicateImpl(sqlite3_context *ctx, int argc, sqlite3_value **argv);

} // namespace DistributedDB

#endif // PREPROCESS_ENTITY_EXT_H
