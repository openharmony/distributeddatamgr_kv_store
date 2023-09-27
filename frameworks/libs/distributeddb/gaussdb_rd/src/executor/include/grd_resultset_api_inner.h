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

#ifndef GRD_RESULTSET_API_INNER_H
#define GRD_RESULTSET_API_INNER_H

#include <cstdint>

#include "grd_base/grd_type_export.h"

namespace DocumentDB {
typedef struct GRD_ResultSet GRD_ResultSet;
int32_t GRD_NextInner(GRD_ResultSet *resultSet);

int32_t GRD_PrevInner(GRD_ResultSet *resultSet);

int32_t GRD_GetValueInner(GRD_ResultSet *resultSet, char **value);

int32_t GRD_FetchInner(GRD_ResultSet *resultSet, GRD_KVItemT *key, GRD_KVItemT *value);

int32_t GRD_FreeValueInner(char *value);

int32_t GRD_FreeResultSetInner(GRD_ResultSet *resultSet);
} // namespace DocumentDB
#endif // GRD_RESULTSET_API_INNER_H