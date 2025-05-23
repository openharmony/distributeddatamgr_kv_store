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

#ifndef GRD_KV_API_INNER_H
#define GRD_KV_API_INNER_H

#include <cstdint>

#include "grd_base/grd_resultset_api.h"
#include "grd_base/grd_type_export.h"

namespace DocumentDB {
int32_t GRD_KVPutInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value);

int32_t GRD_KVGetInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value);

int32_t GRD_KVDelInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key);

int32_t GRD_KVScanInner(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, GRD_KvScanModeE mode,
    GRD_ResultSet **resultSet);

int32_t GRD_KVFilterInner(GRD_DB *db, const char *collectionName, const GRD_FilterOptionT *scanParams,
    GRD_ResultSet **resultSet);

int32_t GRD_KVGetSizeInner(GRD_ResultSet *resultSet, uint32_t *keyLen, uint32_t *valueLen);

int32_t GRD_GetItemInner(GRD_ResultSet *resultSet, void *key, void *value);

int32_t GRD_KVFreeItemInner(GRD_KVItemT *item);

int32_t GRD_KVBatchPrepareInner(uint16_t itemNum, GRD_KVBatchT **batch);

int32_t GRD_KVBatchPushbackInner(const void *key, uint32_t keyLen, const void *data, uint32_t dataLen,
    GRD_KVBatchT *batch);

int32_t GRD_KVBatchPutInner(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);

int32_t GRD_KVBatchDelInner(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);

int32_t GRD_KVBatchDestroyInner(GRD_KVBatchT *batch);
} // namespace DocumentDB
#endif // GRD_KV_API_INNER_H