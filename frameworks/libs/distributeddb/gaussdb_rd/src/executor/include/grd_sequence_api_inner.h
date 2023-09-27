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

#ifndef GRD_SEQUENCE_API_INNER_H
#define GRD_SEQUENCE_API_INNER_H

#include <cstdint>

#include "grd_base/grd_resultset_api.h"
#include "grd_base/grd_type_export.h"

namespace DocumentDB {
int32_t GRD_CreateSeqInner(GRD_DB *db, const char *sequenceName, uint32_t flags);

int32_t GRD_DropSeqInner(GRD_DB *db, const char *sequenceName, uint32_t flags);
} // namespace DocumentDB
#endif // GRD_SEQUENCE_API_INNER_H