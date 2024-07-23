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

#include "db_constant.h"

namespace DistributedDB {
const std::string DBConstant::MULTI_SUB_DIR = "multi_ver";
const std::string DBConstant::SINGLE_SUB_DIR = "single_ver";
const std::string DBConstant::LOCAL_SUB_DIR = "local";
const std::string DBConstant::MAINDB_DIR = "main";
const std::string DBConstant::METADB_DIR = "meta";
const std::string DBConstant::CACHEDB_DIR = "cache";
const std::string DBConstant::SYSTEM_TABLE_PREFIX = "naturalbase_rdb_";
const std::string DBConstant::RELATIONAL_PREFIX = "naturalbase_rdb_aux_";
const std::string DBConstant::RELATIONAL_SCHEMA_KEY = "relational_schema";
const std::string DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY = "relational_tracker_schema";
const std::string DBConstant::LOG_TABLE_VERSION_KEY = "log_table_version";
const std::string DBConstant::TIMESTAMP_ALIAS = "naturalbase_rdb_aux_timestamp";
const std::string DBConstant::UPDATE_META_FUNC = "update_meta_within_trigger";
const std::string DBConstant::SINGLE_VER_DATA_STORE = "gen_natural_store";
const std::string DBConstant::SINGLE_VER_META_STORE = "meta";
const std::string DBConstant::SINGLE_VER_CACHE_STORE = "cache";

}
