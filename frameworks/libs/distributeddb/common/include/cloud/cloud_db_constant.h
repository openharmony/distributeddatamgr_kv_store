/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef CLOUD_DB_CONSTANT_H
#define CLOUD_DB_CONSTANT_H

#include "cloud/cloud_store_types.h"
#include <string>

namespace DistributedDB {
class CloudDbConstant {
public:
    static constexpr const char *CLOUD_META_TABLE_PREFIX = "naturalbase_cloud_meta_";
    static constexpr const char *GID_FIELD = "#_gid";
    static constexpr const char *CREATE_FIELD = "#_createTime";
    static constexpr const char *MODIFY_FIELD = "#_modifyTime";
    static constexpr const char *DELETE_FIELD = "#_deleted";
    static constexpr const char *CURSOR_FIELD = "#_cursor";
    static constexpr const char *ROW_ID_FIELD_NAME = "rowid";
    static constexpr uint32_t MAX_UPLOAD_SIZE = 1024 * 512 * 3; // 1.5M
    static constexpr uint32_t MAX_DOWNLOAD_RETRY_TIME = 50;
};
} // namespace DistributedDB
#endif // CLOUD_DB_CONSTANT_H