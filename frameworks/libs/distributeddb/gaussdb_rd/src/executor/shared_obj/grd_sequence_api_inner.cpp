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
#include "check_common.h"
#include "grd_api_manager.h"
#include "grd_base/grd_error.h"
#include "grd_sequence_api_inner.h"
#include "grd_type_inner.h"
#include "rd_log_print.h"
namespace DocumentDB {
int32_t GRD_CreateSeqInner(GRD_DB *db, const char *sequenceName, uint32_t flags)
{
    return GRD_NOT_SUPPORT; // No support.
}

int32_t GRD_DropSeqInner(GRD_DB *db, const char *sequenceName, uint32_t flags)
{
    return GRD_NOT_SUPPORT; // No support.
}
} // namespace DocumentDB