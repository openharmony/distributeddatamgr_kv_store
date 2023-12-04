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
#include "grd_base/grd_resultset_api.h"

#include "doc_errno.h"
#include "grd_api_manager.h"
#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "rd_log_print.h"

using namespace DocumentDB;
static GRD_APIInfo GRD_ResultSetApiInfo;

GRD_API int32_t GRD_Next(GRD_ResultSet *resultSet)
{
    if (GRD_ResultSetApiInfo.NextApi == nullptr) {
        GRD_ResultSetApiInfo = GetApiInfoInstance();
    }
    if (GRD_ResultSetApiInfo.NextApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.NextApi(resultSet);
}

GRD_API int32_t GRD_GetValue(GRD_ResultSet *resultSet, char **value)
{
    if (GRD_ResultSetApiInfo.GetValueApi == nullptr) {
        GRD_ResultSetApiInfo = GetApiInfoInstance();
    }
    if (GRD_ResultSetApiInfo.GetValueApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.GetValueApi(resultSet, value);
}

GRD_API int32_t GRD_FreeValue(char *value)
{
    if (GRD_ResultSetApiInfo.FreeValueApi == nullptr) {
        GRD_ResultSetApiInfo = GetApiInfoInstance();
    }
    if (GRD_ResultSetApiInfo.FreeValueApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.FreeValueApi(value);
}

GRD_API int32_t GRD_FreeResultSet(GRD_ResultSet *resultSet)
{
    if (GRD_ResultSetApiInfo.FreeResultSetApi == nullptr) {
        GRD_ResultSetApiInfo = GetApiInfoInstance();
    }
    if (GRD_ResultSetApiInfo.FreeResultSetApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.FreeResultSetApi(resultSet);
}

GRD_API int32_t GRD_Prev(GRD_ResultSet *resultSet)
{
    if (GRD_ResultSetApiInfo.PrevApi == nullptr) {
        GRD_ResultSetApiInfo = GetApiInfoInstance();
    }
    if (GRD_ResultSetApiInfo.PrevApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.PrevApi(resultSet);
}

GRD_API int32_t GRD_Fetch(GRD_ResultSet *resultSet, GRD_KVItemT *key, GRD_KVItemT *value)
{
    if (GRD_ResultSetApiInfo.FetchApi == nullptr) {
        GRD_ResultSetApiInfo = GetApiInfoInstance();
    }
    if (GRD_ResultSetApiInfo.FetchApi == nullptr) {
        GLOGE("Fail to dlysm RD api symbol");
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.FetchApi(resultSet, key, value);
}
