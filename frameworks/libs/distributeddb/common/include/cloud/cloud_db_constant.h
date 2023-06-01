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
    static const std::string CLOUD_META_TABLE_PREFIX;
    static const std::string GID_FIELD;
    static const std::string CREATE_FIELD;
    static const std::string MODIFY_FIELD;
    static const std::string DELETE_FIELD;
    static const std::string CURSOR_FIELD;
    static const uint32_t MAX_UPLOAD_SIZE;
    static const std::string CLOUD_DEVICE_NAME;
    static const std::string ROW_ID_FIELD_NAME;
    static const uint32_t MAX_DOWNLOAD_RETRY_TIME;
};
} // namespace DistributedDB
#endif // CLOUD_DB_CONSTANT_H
