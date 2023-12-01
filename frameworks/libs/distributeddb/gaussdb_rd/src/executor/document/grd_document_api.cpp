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

#include "grd_document/grd_document_api.h"

#include "check_common.h"
#include "grd_api_manager.h"
#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "grd_type_inner.h"
#include "rd_log_print.h"
using namespace DocumentDB;

static GRD_APIInfo GRD_DocApiInfo;

GRD_API int32_t GRD_CreateCollection(GRD_DB *db, const char *collectionName, const char *optionStr, uint32_t flags)
{
    if (GRD_DocApiInfo.CreateCollectionApi == nullptr) {
        GRD_DocApiInfo = GetApiInfoInstance();
    }
    if (GRD_DocApiInfo.CreateCollectionApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DocApiInfo.CreateCollectionApi(db, collectionName, optionStr, flags);
}

GRD_API int32_t GRD_DropCollection(GRD_DB *db, const char *collectionName, uint32_t flags)
{
    if (GRD_DocApiInfo.DropCollectionApi == nullptr) {
        GRD_DocApiInfo = GetApiInfoInstance();
    }
    if (GRD_DocApiInfo.DropCollectionApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DocApiInfo.DropCollectionApi(db, collectionName, flags);
}

GRD_API int32_t GRD_UpdateDoc(GRD_DB *db, const char *collectionName, const char *filter, const char *update,
    uint32_t flags)
{
    if (GRD_DocApiInfo.UpdateDocApi == nullptr) {
        GRD_DocApiInfo = GetApiInfoInstance();
    }
    if (GRD_DocApiInfo.UpdateDocApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DocApiInfo.UpdateDocApi(db, collectionName, filter, update, flags);
}

GRD_API int32_t GRD_UpsertDoc(GRD_DB *db, const char *collectionName, const char *filter, const char *document,
    uint32_t flags)
{
    if (GRD_DocApiInfo.UpsertDocApi == nullptr) {
        GRD_DocApiInfo = GetApiInfoInstance();
    }
    if (GRD_DocApiInfo.UpsertDocApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DocApiInfo.UpsertDocApi(db, collectionName, filter, document, flags);
}

GRD_API int32_t GRD_InsertDoc(GRD_DB *db, const char *collectionName, const char *document, uint32_t flags)
{
    if (GRD_DocApiInfo.InsertDocApi == nullptr) {
        GRD_DocApiInfo = GetApiInfoInstance();
    }
    if (GRD_DocApiInfo.InsertDocApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DocApiInfo.InsertDocApi(db, collectionName, document, flags);
}

GRD_API int32_t GRD_DeleteDoc(GRD_DB *db, const char *collectionName, const char *filter, uint32_t flags)
{
    if (GRD_DocApiInfo.DeleteDocApi == nullptr) {
        GRD_DocApiInfo = GetApiInfoInstance();
    }
    if (GRD_DocApiInfo.DeleteDocApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DocApiInfo.DeleteDocApi(db, collectionName, filter, flags);
}

GRD_API int32_t GRD_FindDoc(GRD_DB *db, const char *collectionName, Query query, uint32_t flags,
    GRD_ResultSet **resultSet)
{
    if (GRD_DocApiInfo.FindDocApi == nullptr) {
        GRD_DocApiInfo = GetApiInfoInstance();
    }
    if (GRD_DocApiInfo.FindDocApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_DocApiInfo.FindDocApi(db, collectionName, query, flags, resultSet);
}
