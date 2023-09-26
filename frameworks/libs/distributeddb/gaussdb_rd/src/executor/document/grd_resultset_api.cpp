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

#include <mutex>

#include "doc_errno.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "log_print.h"

using namespace DocumentDB;
namespace DocumentDB {
static GRD_APIInfo GRD_ResultSetApiInfo = GetApiInfoInstance();
int32_t GRD_NextInner(GRD_ResultSet *resultSet)
{
    if (resultSet == nullptr) {
        GLOGE("resultSet is nullptr");
        return GRD_INVALID_ARGS;
    };
    int ret = resultSet->resultSet_.GetNext(true, true);
    return TransferDocErr(ret);
}

int32_t GRD_GetValueInner(GRD_ResultSet *resultSet, char **value)
{
    if (resultSet == nullptr || value == nullptr) {
        GLOGE("resultSet is nullptr,cant get value from it");
        return GRD_INVALID_ARGS;
    };
    char *val = nullptr;
    int ret = resultSet->resultSet_.GetValue(&val);
    if (val == nullptr) {
        GLOGE("Value that get from resultSet is nullptr");
        return GRD_NOT_AVAILABLE;
    }
    *value = val;
    return TransferDocErr(ret);
}

int32_t GRD_FreeValueInner(char *value)
{
    if (value == nullptr) {
        return GRD_INVALID_ARGS;
    }
    delete[] value;
    return GRD_OK;
}

int32_t GRD_FreeResultSetInner(GRD_ResultSet *resultSet)
{
    if (resultSet == nullptr) {
        return GRD_INVALID_ARGS;
    }
    resultSet->resultSet_.EraseCollection();
    delete resultSet;
    return GRD_OK;
}

int32_t GRD_PrevInner(GRD_ResultSet *resultSet)
{
    return GRD_OK; // No support;
}

int32_t GRD_FetchInner(GRD_ResultSet *resultSet, GRD_KVItemT *key, GRD_KVItemT *value)
{
    return GRD_OK; // No support;
}
} // namespace DocumentDB
GRD_API int32_t GRD_Next(GRD_ResultSet *resultSet)
{
    if (GRD_ResultSetApiInfo.GRD_NextApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.GRD_NextApi(resultSet);
}

GRD_API int32_t GRD_GetValue(GRD_ResultSet *resultSet, char **value)
{
    if (GRD_ResultSetApiInfo.GRD_GetValueApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.GRD_GetValueApi(resultSet, value);
}

GRD_API int32_t GRD_FreeValue(char *value)
{
    if (GRD_ResultSetApiInfo.GRD_FreeValueApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.GRD_FreeValueApi(value);
}

GRD_API int32_t GRD_FreeResultSet(GRD_ResultSet *resultSet)
{
    if (GRD_ResultSetApiInfo.GRD_FreeResultSetApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.GRD_FreeResultSetApi(resultSet);
}

GRD_API int32_t GRD_Prev(GRD_ResultSet *resultSet)
{
    if (GRD_ResultSetApiInfo.GRD_PrevApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.GRD_PrevApi(resultSet);
}

GRD_API int32_t GRD_Fetch(GRD_ResultSet *resultSet, GRD_KVItemT *key, GRD_KVItemT *value)
{
    if (GRD_ResultSetApiInfo.GRD_FetchApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_ResultSetApiInfo.GRD_FetchApi(resultSet, key, value);
}
