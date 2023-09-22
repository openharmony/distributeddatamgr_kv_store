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
#include "grd_shared_obj/grd_sequence_api.h"

#include "check_common.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "grd_type_inner.h"
#include "log_print.h"
using namespace DocumentDB;
namespace DocumentDB {
static GRD_APIStruct GRD_SeqApiStruct = GetApiStructInstance();
int32_t GRD_CreateSeqInner(GRD_DB *db, const char *sequenceName, uint32_t flags)
{
    return GRD_OK; // No support.
}

int32_t GRD_DropSeqInner(GRD_DB *db, const char *sequenceName, uint32_t flags)
{
    return GRD_OK; // No support.
}
} // namespace DocumentDB

GRD_API int32_t GRD_CreateSeq(GRD_DB *db, const char *sequenceName, uint32_t flags)
{
    if (GRD_SeqApiStruct.GRD_CreateSeqApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_SeqApiStruct.GRD_CreateSeqApi(db, sequenceName, flags);
}

GRD_API int32_t GRD_DropSeq(GRD_DB *db, const char *sequenceName, uint32_t flags)
{
    if (GRD_SeqApiStruct.GRD_DropSeqApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_SeqApiStruct.GRD_DropSeqApi(db, sequenceName, flags);
}
