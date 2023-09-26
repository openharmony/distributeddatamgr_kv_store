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

#include "check_common.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "grd_type_inner.h"
#include "log_print.h"
using namespace DocumentDB;

namespace DocumentDB {
static GRD_APIInfo GRD_KVApiInfo = GetApiInfoInstance();
int32_t GRD_KVPutInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVGetInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVDelInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVScanInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, KvScanModeE mode,
    GRD_ResultSet **resultSet)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVFreeItemInner(GRD_KVItemT *item)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVBatchPrepareInner(uint16_t itemNum, GRD_KVBatchT **batch)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVBatchPushbackInner(const void *key, uint32_t keyLen, const void *data, uint32_t dataLen,
    GRD_KVBatchT *batch)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVBatchDelInner(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch)
{
    return GRD_OK; // No support.
}

int32_t GRD_KVBatchDestoryInner(GRD_KVBatchT *batch)
{
    return GRD_OK; // No support.
}
} // namespace DocumentDB

GRD_API int32_t GRD_KVPut(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value)
{
    if (GRD_KVApiInfo.GRD_KVPutApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVPutApi(db, collectionName, key, value);
}

GRD_API int32_t GRD_KVGet(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value)
{
    if (GRD_KVApiInfo.GRD_KVGetApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVGetApi(db, collectionName, key, value);
}

GRD_API int32_t GRD_KVDel(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key)
{
    if (GRD_KVApiInfo.GRD_KVDelApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVDelApi(db, collectionName, key);
}

GRD_API int32_t GRD_KVScan(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, KvScanModeE mode,
    GRD_ResultSet **resultSet)
{
    if (GRD_KVApiInfo.GRD_KVScanApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVScanApi(db, collectionName, key, mode, resultSet);
}

GRD_API int32_t GRD_KVFreeItem(GRD_KVItemT *item)
{
    if (GRD_KVApiInfo.GRD_KVFreeItemApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVFreeItemApi(item);
}

GRD_API int32_t GRD_KVBatchPrepare(uint16_t itemNum, GRD_KVBatchT **batch)
{
    if (GRD_KVApiInfo.GRD_KVBatchPrepareApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVBatchPrepareApi(itemNum, batch);
}

GRD_API int32_t GRD_KVBatchPushback(const void *key, uint32_t keyLen, const void *data, uint32_t dataLen,
    GRD_KVBatchT *batch)
{
    if (GRD_KVApiInfo.GRD_KVBatchPushbackApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVBatchPushbackApi(key, keyLen, data, dataLen, batch);
}

GRD_API int32_t GRD_KVBatchDel(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch)
{
    if (GRD_KVApiInfo.GRD_KVBatchDelApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVBatchDelApi(db, collectionName, batch);
}

GRD_API int32_t GRD_KVBatchDestory(GRD_KVBatchT *batch)
{
    if (GRD_KVApiInfo.GRD_KVBatchDestoryApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_KVApiInfo.GRD_KVBatchDestoryApi(batch);
}
