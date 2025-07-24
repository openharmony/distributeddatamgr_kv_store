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

#include "schema_constant.h"

namespace DistributedDB {
const uint32_t SchemaConstant::SCHEMA_META_FEILD_COUNT_MAX = 5;
const uint32_t SchemaConstant::SCHEMA_META_FEILD_COUNT_MIN = 3;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_LENGTH_MAX = 64;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_LENGTH_MIN = 1;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_COUNT_MAX = 256;
const uint32_t SchemaConstant::SCHEMA_FEILD_NAME_COUNT_MIN = 1;
const uint32_t SchemaConstant::SCHEMA_FEILD_PATH_DEPTH_MAX = 4;
const uint32_t SchemaConstant::SCHEMA_INDEX_COUNT_MAX = 32;
const uint32_t SchemaConstant::SCHEMA_STRING_SIZE_LIMIT = 524288; // 512K
const uint32_t SchemaConstant::SCHEMA_DEFAULT_STRING_SIZE_LIMIT = 4096; // 4K
const uint32_t SchemaConstant::SCHEMA_SKIPSIZE_MAX = 4194302; // 4M - 2 Bytes

const uint32_t SchemaConstant::SECURE_BYTE_ALIGN = 8; // 8 bytes align
} // namespace DistributedDB
