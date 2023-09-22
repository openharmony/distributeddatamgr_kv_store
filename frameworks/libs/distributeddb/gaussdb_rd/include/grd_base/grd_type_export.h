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

#ifndef GRD_TYPE_EXPORT_H
#define GRD_TYPE_EXPORT_H
#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef _WIN32
#define GRD_API __attribute__((visibility("default")))
#endif

typedef struct GRD_DB GRD_DB;

/**
 * @brief Open database config
 */
#define GRD_DB_OPEN_ONLY 0x00
#define GRD_DB_OPEN_CREATE 0x01
// check data in database if close abnormally last time, if data is corrupted, rebuild the database
#define GRD_DB_OPEN_CHECK_FOR_ABNORMAL 0x02
// check data in database when open database, if data is corrupted, rebuild the database.
#define GRD_DB_OPEN_CHECK 0x04

/**
 * @brief Close database config
 */
#define GRD_DB_CLOSE 0x00
#define GRD_DB_CLOSE_IGNORE_ERROR 0x01

/**
 * @brief flush database config
 */
#define GRD_DB_FLUSH_ASYNC 0x00
#define GRD_DB_FLUSH_SYNC 0x01

#define GRD_DOC_ID_DISPLAY 0x01
typedef struct Query {
    const char *filter;
    const char *projection;
} Query;

typedef struct GRD_KVItem {
    void *data;
    uint32_t dataLen;
} GRD_KVItemT;

typedef enum KvScanMode {
    KV_SCAN_PREFIX = 0,
    KV_SCAN_EQUAL_OR_LESS_KEY = 1,
    KV_SCAN_EQUAL_OR_GREATER_KEY = 2,
    KV_SCAN_BUTT
} KvScanModeE;

typedef struct GRD_ResultSet GRD_ResultSet;
typedef struct GRD_DB GRD_DB;
typedef struct GRD_KVBatch GRD_KVBatchT;
typedef int32_t (*OpenPtr)(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db);
typedef int32_t (*ClosePtr)(GRD_DB *db, uint32_t flags);
typedef int32_t (*FlushPtr)(GRD_DB *db, uint32_t flags);
typedef int32_t (*IndexPreloadPtr)(GRD_DB *db, const char *collectionName);
typedef int32_t (*CreateCollectionPtr)(GRD_DB *db, const char *collectionName, const char *optionStr,
    uint32_t flags);
typedef int32_t (*DropCollectionPtr)(GRD_DB *db, const char *collectionName, uint32_t flags);
typedef int32_t (*InsertDocPtr)(GRD_DB *db, const char *collectionName, const char *document, uint32_t flags);
typedef int32_t (*FindDocPtr)(GRD_DB *db, const char *collectionName, Query query, uint32_t flags,
    GRD_ResultSet **resultSet);
typedef int32_t (*UpdateDocPtr)(GRD_DB *db, const char *collectionName, const char *filter,
    const char *update, uint32_t flags);
typedef int32_t (*UpsertDocPtr)(GRD_DB *db, const char *collectionName, const char *filter,
    const char *document, uint32_t flags);
typedef int32_t (*DeleteDocPtr)(GRD_DB *db, const char *collectionName, const char *filter, uint32_t flags);
typedef int32_t (*NextPtr)(GRD_ResultSet *resultSet);
typedef int32_t (*PrevPtr)(GRD_ResultSet *resultSet);
typedef int32_t (*GetValuePtr)(GRD_ResultSet *resultSet, char **value);
typedef int32_t (*FetchPtr)(GRD_ResultSet *resultSet, GRD_KVItemT *key, GRD_KVItemT *value);
typedef int32_t (*FreeValuePtr)(char *value);
typedef int32_t (*FreeResultSetPtr)(GRD_ResultSet *resultSet);
typedef int32_t (*KVBatchDestoryPtr)(GRD_KVBatchT *batch);
typedef int32_t (*KVPutPtr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key,
    const GRD_KVItemT *value);
typedef int32_t (*KVGetPtr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key,
    const GRD_KVItemT *value);
typedef int32_t (*KVDelPtr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key);
typedef int32_t (*KVScanPtr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key,
    KvScanModeE mode, GRD_ResultSet **resultSet);
typedef int32_t (*KVFreeItemPtr)(GRD_KVItemT *item);
typedef int32_t (*KVBatchPreparePtr)(uint16_t itemNum, GRD_KVBatchT **batch);
typedef int32_t (*KVBatchPushbackPtr)(const void *key, uint32_t keyLen, const void *data,
    uint32_t dataLen, GRD_KVBatchT *batch);
typedef int32_t (*KVBatchDelPtr)(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);
typedef int32_t (*CreateSeqPtr)(GRD_DB *db, const char *sequenceName, uint32_t flags);
typedef int32_t (*DropSeqPtr)(GRD_DB *db, const char *sequenceName, uint32_t flags);
struct GRD_APIStruct {
    OpenPtr GRD_DBOpenApi = nullptr;
    ClosePtr GRD_DBCloseApi = nullptr;
    FlushPtr GRD_FlushApi = nullptr;
    IndexPreloadPtr GRD_IndexPreloadApi = nullptr;
    CreateCollectionPtr GRD_CreateCollectionApi = nullptr;
    DropCollectionPtr GRD_DropCollectionApi = nullptr;
    InsertDocPtr GRD_InsertDocApi = nullptr;
    FindDocPtr GRD_FindDocApi = nullptr;
    UpdateDocPtr GRD_UpdateDocApi = nullptr;
    UpsertDocPtr GRD_UpsertDocApi = nullptr;
    DeleteDocPtr GRD_DeleteDocApi = nullptr;
    NextPtr GRD_NextApi = nullptr;
    PrevPtr GRD_PrevApi = nullptr;
    GetValuePtr GRD_GetValueApi = nullptr;
    FetchPtr GRD_FetchApi = nullptr;
    FreeValuePtr GRD_FreeValueApi = nullptr;
    FreeResultSetPtr GRD_FreeResultSetApi = nullptr;
    KVPutPtr GRD_KVPutApi = nullptr;
    KVGetPtr GRD_KVGetApi = nullptr;
    KVDelPtr GRD_KVDelApi = nullptr;
    KVScanPtr GRD_KVScanApi = nullptr;
    KVFreeItemPtr GRD_KVFreeItemApi = nullptr;
    KVBatchPreparePtr GRD_KVBatchPrepareApi = nullptr;
    KVBatchPushbackPtr GRD_KVBatchPushbackApi = nullptr;
    KVBatchDelPtr GRD_KVBatchDelApi = nullptr;
    KVBatchDestoryPtr GRD_KVBatchDestoryApi = nullptr;
    CreateSeqPtr GRD_CreateSeqApi = nullptr;
    DropSeqPtr GRD_DropSeqApi = nullptr;
};

/**
 * @brief Flags for create and drop collection
 */
#define CHK_EXIST_COLLECTION 1
#define CHK_NON_EXIST_COLLECTION 1

#define GRD_DOC_APPEND 0
#define GRD_DOC_REPLACE 1

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // GRD_TYPE_EXPORT_H