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

#include "cloud/cloud_db_constant.h"

namespace DistributedDB {
    const std::string CloudDbConstant::CLOUD_META_TABLE_PREFIX = "naturalbase_cloud_meta_";
    const std::string CloudDbConstant::GID_FIELD = "#_gid";
    const std::string CloudDbConstant::CREATE_FIELD = "#_createTime";
    const std::string CloudDbConstant::MODIFY_FIELD = "#_modifyTime";
    const std::string CloudDbConstant::DELETE_FIELD = "#_deleted";
    const std::string CloudDbConstant::CURSOR_FIELD = "#_cursor";
    const uint32_t CloudDbConstant::MAX_UPLOAD_SIZE = 1024 * 1024 * 8;
    const std::string CloudDbConstant::ROW_ID_FIELD_NAME = "rowid";
    const uint32_t CloudDbConstant::MAX_DOWNLOAD_RETRY_TIME = 50;
}
