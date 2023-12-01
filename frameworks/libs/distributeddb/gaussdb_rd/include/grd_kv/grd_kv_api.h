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

#ifndef GRD_KV_API_H
#define GRD_KV_API_H

#include <cstdint>

#include "grd_base/grd_resultset_api.h"
#include "grd_base/grd_type_export.h"

#ifdef __cplusplus
extern "C" {
#endif
GRD_API int32_t GRD_KVPut(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value);

GRD_API int32_t GRD_KVGet(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value);

GRD_API int32_t GRD_KVDel(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key);

GRD_API int32_t GRD_KVScan(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, GRD_KvScanModeE mode,
    GRD_ResultSet **resultSet);

GRD_API int32_t GRD_KVFilter(GRD_DB *db, const char *collectionName, const GRD_FilterOptionT *scanParams,
    GRD_ResultSet **resultSet);

GRD_API int32_t GRD_KVGetSize(GRD_ResultSet *resultSet, uint32_t *keyLen, uint32_t *valueLen);

GRD_API int32_t GRD_GetItem(GRD_ResultSet *resultSet, void *key, void *value);

GRD_API int32_t GRD_KVFreeItem(GRD_KVItemT *item);

GRD_API int32_t GRD_KVBatchPrepare(uint16_t itemNum, GRD_KVBatchT **batch);

GRD_API int32_t GRD_KVBatchPushback(const void *key, uint32_t keyLen, const void *data, uint32_t dataLen,
    GRD_KVBatchT *batch);

GRD_API int32_t GRD_KVBatchPut(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);

GRD_API int32_t GRD_KVBatchDel(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);

GRD_API int32_t GRD_KVBatchDestroy(GRD_KVBatchT *batch);
#ifdef __cplusplus
}
#endif
#endif // GRD_DOCUMENT_API_H