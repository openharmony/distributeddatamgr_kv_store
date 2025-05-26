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

#include "param_check_utils.h"

#include "db_common.h"

namespace DistributedDB {
bool ParamCheckUtils::CheckRelationalTableName(const std::string &tableName)
{
    if (!DBCommon::CheckIsAlnumOrUnderscore(tableName)) {
        return false;
    }
    return tableName.compare(0, DBConstant::SYSTEM_TABLE_PREFIX.size(), DBConstant::SYSTEM_TABLE_PREFIX) != 0;
}
} // namespace DistributedDB