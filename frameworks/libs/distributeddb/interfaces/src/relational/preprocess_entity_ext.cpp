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
#include "preprocess_entity_ext.h"

#ifndef OMIT_JSON
#include <json/json.h>
#endif
#include "log_print.h"

namespace DistributedDB {

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
    if (!inputEntityJson) {
        LOGE("is_entity_duplicate expects not null value input");
        sqlite3_result_error(ctx, "is_entity_duplicate expects not null value input", -1);
        return;
    }
    if (!dbEntityJson) {
        // db entity invalid, return false
        sqlite3_result_int(ctx, 0);
        return;
    }

    // 比较实体是否重复
    bool isDuplicate = false;
    sqlite3_result_int(ctx, isDuplicate ? 1 : 0);
}

} // namespace DistributedDB
