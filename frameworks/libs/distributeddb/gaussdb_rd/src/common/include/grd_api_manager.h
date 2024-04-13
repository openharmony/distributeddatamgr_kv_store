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

#ifndef GRD_API_MANAGER_H
#define GRD_API_MANAGER_H

#include "grd_base/grd_type_export.h"

namespace DocumentDB {
typedef int32_t (*DBOpen)(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db);
typedef int32_t (*DBClose)(GRD_DB *db, uint32_t flags);
typedef int32_t (*DBFlush)(GRD_DB *db, uint32_t flags);
typedef int32_t (*IndexPreload)(GRD_DB *db, const char *collectionName);
typedef int32_t (*CreateCollection)(GRD_DB *db, const char *collectionName, const char *optionStr, uint32_t flags);
typedef int32_t (*DropCollection)(GRD_DB *db, const char *collectionName, uint32_t flags);
typedef int32_t (*InsertDoc)(GRD_DB *db, const char *collectionName, const char *document, uint32_t flags);
typedef int32_t (*FindDoc)(GRD_DB *db, const char *collectionName, Query query,
    uint32_t flags, GRD_ResultSet **resultSet);
typedef int32_t (*UpdateDoc)(GRD_DB *db, const char *collectionName, const char *filter,
    const char *update, uint32_t flags);
typedef int32_t (*UpsertDoc)(GRD_DB *db, const char *collectionName, const char *filter,
    const char *document, uint32_t flags);
typedef int32_t (*DeleteDoc)(GRD_DB *db, const char *collectionName, const char *filter, uint32_t flags);
typedef int32_t (*ResultNext)(GRD_ResultSet *resultSet);
typedef int32_t (*ResultPrev)(GRD_ResultSet *resultSet);
typedef int32_t (*GetValue)(GRD_ResultSet *resultSet, char **value);
typedef int32_t (*Fetch)(GRD_ResultSet *resultSet, GRD_KVItemT *key, GRD_KVItemT *value);
typedef int32_t (*FreeValue)(char *value);
typedef int32_t (*FreeResultSet)(GRD_ResultSet *resultSet);
typedef int32_t (*KVPut)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value);
typedef int32_t (*KVGet)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, const GRD_KVItemT *value);
typedef int32_t (*KVDel)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key);
typedef int32_t (*KVScan)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key, GRD_KvScanModeE mode,
    GRD_ResultSet **resultSet);
typedef int32_t (*KVFilter)(GRD_DB *db, const char *collectionName, const GRD_FilterOptionT *scanParams,
    GRD_ResultSet **resultSet);
typedef int32_t (*KVGetSize)(GRD_ResultSet *resultSet, uint32_t *keyLen, uint32_t *valueLen);
typedef int32_t (*GetItem)(GRD_ResultSet *resultSet, void *key, void *value);
typedef int32_t (*KVFreeItem)(GRD_KVItemT *item);
typedef int32_t (*KVBatchPrepare)(uint16_t itemNum, GRD_KVBatchT **batch);
typedef int32_t (*KVBatchPushback)(const void *key, uint32_t keyLen, const void *data, uint32_t dataLen,
    GRD_KVBatchT *batch);
typedef int32_t (*KVBatchPut)(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);
typedef int32_t (*KVBatchDel)(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);
typedef int32_t (*KVBatchDestory)(GRD_KVBatchT *batch);
struct GRD_APIInfo {
    DBOpen DBOpenApi = nullptr;
    DBClose DBCloseApi = nullptr;
    DBFlush FlushApi = nullptr;
    IndexPreload IndexPreloadApi = nullptr;
    CreateCollection CreateCollectionApi = nullptr;
    DropCollection DropCollectionApi = nullptr;
    InsertDoc InsertDocApi = nullptr;
    FindDoc FindDocApi = nullptr;
    UpdateDoc UpdateDocApi = nullptr;
    UpsertDoc UpsertDocApi = nullptr;
    DeleteDoc DeleteDocApi = nullptr;
    ResultNext NextApi = nullptr;
    ResultPrev PrevApi = nullptr;
    GetValue GetValueApi = nullptr;
    Fetch FetchApi = nullptr;
    FreeValue FreeValueApi = nullptr;
    FreeResultSet FreeResultSetApi = nullptr;
    KVPut KVPutApi = nullptr;
    KVGet KVGetApi = nullptr;
    KVDel KVDelApi = nullptr;
    KVScan KVScanApi = nullptr;
    KVFilter KVFilterApi = nullptr;
    KVGetSize KVGetSizeApi = nullptr;
    GetItem GetItemApi = nullptr;
    KVFreeItem KVFreeItemApi = nullptr;
    KVBatchPrepare KVBatchPrepareApi = nullptr;
    KVBatchPushback KVBatchPushbackApi = nullptr;
    KVBatchDel KVBatchPutApi = nullptr;
    KVBatchDel KVBatchDelApi = nullptr;
    KVBatchDestory KVBatchDestoryApi = nullptr;
};
GRD_APIInfo GetApiInfoInstance();
void SetLibType(bool isHash);
} // namespace DocumentDB
#endif // __cplusplus
