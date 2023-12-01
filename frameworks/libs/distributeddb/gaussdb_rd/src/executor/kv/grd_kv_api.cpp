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
#include "grd_kv/grd_kv_api.h"

#include "grd_api_manager.h"
#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "grd_type_inner.h"
#include "rd_log_print.h"
using namespace DocumentDB;

static GRD_APIInfo GRD_KVApiInfo;

GRD_API int32_t GRD_KVPut(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value)
{
    if (GRD_KVApiInfo.KVPutApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVPutApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVPutApi(db, collectionName, key, value);
}

GRD_API int32_t GRD_KVGet(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value)
{
    if (GRD_KVApiInfo.KVGetApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVGetApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVGetApi(db, collectionName, key, value);
}

GRD_API int32_t GRD_KVDel(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key)
{
    if (GRD_KVApiInfo.KVDelApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVDelApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVDelApi(db, collectionName, key);
}

GRD_API int32_t GRD_KVScan(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, GRD_KvScanModeE mode,
    GRD_ResultSet **resultSet)
{
    if (GRD_KVApiInfo.KVScanApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVScanApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVScanApi(db, collectionName, key, mode, resultSet);
}

GRD_API int32_t GRD_KVFilter(GRD_DB *db, const char *collectionName, const GRD_FilterOptionT *scanParams,
    GRD_ResultSet **resultSet)
{
    if (GRD_KVApiInfo.KVFilterApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVFilterApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVFilterApi(db, collectionName, scanParams, resultSet);
}

GRD_API int32_t GRD_KVGetSize(GRD_ResultSet *resultSet, uint32_t *keyLen, uint32_t *valueLen)
{
    if (GRD_KVApiInfo.KVGetSizeApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVGetSizeApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVGetSizeApi(resultSet, keyLen, valueLen);
}

GRD_API int32_t GRD_GetItem(GRD_ResultSet *resultSet, void *key, void *value)
{
    if (GRD_KVApiInfo.GetItemApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.GetItemApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GetItemApi(resultSet, key, value);
}

GRD_API int32_t GRD_KVFreeItem(GRD_KVItemT *item)
{
    if (GRD_KVApiInfo.KVFreeItemApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVFreeItemApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVFreeItemApi(item);
}

GRD_API int32_t GRD_KVBatchPrepare(uint16_t itemNum, GRD_KVBatchT **batch)
{
    if (GRD_KVApiInfo.KVBatchPrepareApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVBatchPrepareApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVBatchPrepareApi(itemNum, batch);
}

GRD_API int32_t GRD_KVBatchPushback(const void *key, uint32_t keyLen, const void *data, uint32_t dataLen,
    GRD_KVBatchT *batch)
{
    if (GRD_KVApiInfo.KVBatchPushbackApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVBatchPushbackApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVBatchPushbackApi(key, keyLen, data, dataLen, batch);
}

GRD_API int32_t GRD_KVBatchPut(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch)
{
    if (GRD_KVApiInfo.KVBatchPutApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVBatchPutApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVBatchPutApi(db, collectionName, batch);
}

GRD_API int32_t GRD_KVBatchDel(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch)
{
    if (GRD_KVApiInfo.KVBatchDelApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVBatchDelApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVBatchDelApi(db, collectionName, batch);
}

GRD_API int32_t GRD_KVBatchDestroy(GRD_KVBatchT *batch)
{
    if (GRD_KVApiInfo.KVBatchDestoryApi == nullptr) {
        GRD_KVApiInfo = GetApiInfoInstance();
    }
    if (GRD_KVApiInfo.KVBatchDestoryApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.KVBatchDestoryApi(batch);
}
