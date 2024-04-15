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
#include "grd_base/grd_db_api.h"

#include "check_common.h"
#include "doc_errno.h"
#include "document_store_manager.h"
#include "grd_api_manager.h"
#include "grd_base/grd_error.h"
#include "grd_type_inner.h"
#include "rd_log_print.h"

using namespace DocumentDB;
static GRD_APIInfo GRD_DBApiInfo;

GRD_API int32_t GRD_DBOpen(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db)
{
    if (GRD_DBApiInfo.DBOpenApi == nullptr) {
        GRD_DBApiInfo = GetApiInfoInstance();
    }
    if (GRD_DBApiInfo.DBOpenApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.DBOpenApi(dbPath, configStr, flags, db);
}

GRD_API int32_t GRD_DBClose(GRD_DB *db, uint32_t flags)
{
    if (GRD_DBApiInfo.DBCloseApi == nullptr) {
        GRD_DBApiInfo = GetApiInfoInstance();
    }
    if (GRD_DBApiInfo.DBCloseApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.DBCloseApi(db, flags);
}

GRD_API int32_t GRD_Flush(GRD_DB *db, uint32_t flags)
{
    if (GRD_DBApiInfo.FlushApi == nullptr) {
        GRD_DBApiInfo = GetApiInfoInstance();
    }
    if (GRD_DBApiInfo.FlushApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.FlushApi(db, flags);
}

GRD_API int32_t GRD_IndexPreload(GRD_DB *db, const char *collectionName)
{
    if (GRD_DBApiInfo.IndexPreloadApi == nullptr) {
        GRD_DBApiInfo = GetApiInfoInstance();
    }
    if (GRD_DBApiInfo.IndexPreloadApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.IndexPreloadApi(db, collectionName);
}

void GRD_SetLibType(bool isHash)
{
    SetLibType(isHash);
}