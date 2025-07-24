/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef SCHEMA_CONSTANT_H
#define SCHEMA_CONSTANT_H

#include <cstdint>

// This header is supposed to be included only in source files. Do not include it in any header files.
namespace DistributedDB {
class SchemaConstant final {
public:
    static constexpr const char *KEYWORD_SCHEMA_VERSION = "SCHEMA_VERSION";
    static constexpr const char *KEYWORD_SCHEMA_MODE = "SCHEMA_MODE";
    static constexpr const char *KEYWORD_SCHEMA_DEFINE = "SCHEMA_DEFINE";
    static constexpr const char *KEYWORD_SCHEMA_INDEXES = "SCHEMA_INDEXES";
    static constexpr const char *KEYWORD_SCHEMA_SKIPSIZE = "SCHEMA_SKIPSIZE";
    static constexpr const char *KEYWORD_SCHEMA_TYPE = "SCHEMA_TYPE";
    static constexpr const char *KEYWORD_SCHEMA_TABLE = "TABLES";
    static constexpr const char *KEYWORD_INDEX = "INDEX"; // For FlatBuffer-Schema
    static constexpr const char *KEYWORD_TABLE_MODE = "TABLE_MODE";

    static constexpr const char *KEYWORD_MODE_STRICT = "STRICT";
    static constexpr const char *KEYWORD_MODE_COMPATIBLE = "COMPATIBLE";

    static constexpr const char *KEYWORD_TYPE_BOOL = "BOOL";
    static constexpr const char *KEYWORD_TYPE_INTEGER = "INTEGER";
    static constexpr const char *KEYWORD_TYPE_LONG = "LONG";
    static constexpr const char *KEYWORD_TYPE_DOUBLE = "DOUBLE";
    static constexpr const char *KEYWORD_TYPE_STRING = "STRING";
    static constexpr const char *KEYWORD_TYPE_BOOLEAN = "BOOLEAN";

    static constexpr const char *KEYWORD_ATTR_NOT_NULL = "NOT NULL";
    static constexpr const char *KEYWORD_ATTR_DEFAULT = "DEFAULT";
    static constexpr const char *KEYWORD_ATTR_VALUE_NULL = "null";
    static constexpr const char *KEYWORD_ATTR_VALUE_TRUE = "true";
    static constexpr const char *KEYWORD_ATTR_VALUE_FALSE = "false";

    static constexpr const char *KEYWORD_TABLE_SPLIT_DEVICE = "SPLIT_BY_DEVICE";
    static constexpr const char *KEYWORD_TABLE_COLLABORATION = "COLLABORATION";

    static constexpr const char *KEYWORD_TYPE_RELATIVE = "RELATIVE";
    static constexpr const char *SCHEMA_SUPPORT_VERSION = "1.0";
    static constexpr const char *SCHEMA_SUPPORT_VERSION_V2 = "2.0";
    static constexpr const char *SCHEMA_SUPPORT_VERSION_V2_1 = "2.1";
    static constexpr const char *SCHEMA_CURRENT_VERSION = SCHEMA_SUPPORT_VERSION_V2_1;

    static constexpr const char *REFERENCE_PROPERTY = "REFERENCE_PROPERTY";
    static constexpr const char *SOURCE_TABLE_NAME = "SOURCE_TABLE_NAME";
    static constexpr const char *TARGET_TABLE_NAME = "TARGET_TABLE_NAME";
    static constexpr const char *COLUMNS = "COLUMNS";
    static constexpr const char *SOURCE_COL = "SOURCE_COL";
    static constexpr const char *TARGET_COL = "TARGET_COL";
    static constexpr const char *KEYWORD_DISTRIBUTED_SCHEMA = "DISTRIBUTED_SCHEMA";
    static constexpr const char *KEYWORD_DISTRIBUTED_VERSION = "VERSION";
    static constexpr const char *KEYWORD_DISTRIBUTED_TABLE = "DISTRIBUTED_TABLE";
    static constexpr const char *KEYWORD_DISTRIBUTED_TABLE_NAME = "TABLE_NAME";
    static constexpr const char *KEYWORD_DISTRIBUTED_FIELD = "DISTRIBUTED_FIELD";
    static constexpr const char *KEYWORD_DISTRIBUTED_COL_NAME = "COL_NAME";
    static constexpr const char *KEYWORD_DISTRIBUTED_IS_P2P_SYNC = "IS_P2P_SYNC";
    static constexpr const char *KEYWORD_DISTRIBUTED_IS_SPECIFIED = "IS_SPECIFIED";

    static const uint32_t SCHEMA_META_FEILD_COUNT_MAX;
    static const uint32_t SCHEMA_META_FEILD_COUNT_MIN;
    static const uint32_t SCHEMA_FEILD_NAME_LENGTH_MAX;
    static const uint32_t SCHEMA_FEILD_NAME_LENGTH_MIN;
    static const uint32_t SCHEMA_FEILD_NAME_COUNT_MAX;
    static const uint32_t SCHEMA_FEILD_NAME_COUNT_MIN;
    static const uint32_t SCHEMA_FEILD_PATH_DEPTH_MAX;
    static const uint32_t SCHEMA_INDEX_COUNT_MAX;
    static const uint32_t SCHEMA_STRING_SIZE_LIMIT;
    static const uint32_t SCHEMA_DEFAULT_STRING_SIZE_LIMIT;
    static const uint32_t SCHEMA_SKIPSIZE_MAX;

    static const uint32_t SECURE_BYTE_ALIGN;
};
} // namespace DistributedDB
#endif // SCHEMA_CONSTANT_H