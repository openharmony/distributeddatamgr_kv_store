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
typedef int32_t (*open_ptr)(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db);
typedef int32_t (*close_ptr)(GRD_DB *db, uint32_t flags);
typedef int32_t (*flush_ptr)(GRD_DB *db, uint32_t flags);
typedef int32_t (*index_preload_ptr)(GRD_DB *db, const char *collectionName);
typedef int32_t (*create_collection_ptr)(GRD_DB *db, const char *collectionName, const char *optionStr,
    uint32_t flags);
typedef int32_t (*drop_collection_ptr)(GRD_DB *db, const char *collectionName, uint32_t flags);
typedef int32_t (*insert_doc_ptr)(GRD_DB *db, const char *collectionName, const char *document, uint32_t flags);
typedef int32_t (*find_doc_ptr)(GRD_DB *db, const char *collectionName, Query query, uint32_t flags,
    GRD_ResultSet **resultSet);
typedef int32_t (*update_doc_ptr)(GRD_DB *db, const char *collectionName, const char *filter,
    const char *update, uint32_t flags);
typedef int32_t (*upsert_doc_ptr)(GRD_DB *db, const char *collectionName, const char *filter,
    const char *document, uint32_t flags);
typedef int32_t (*delete_doc_ptr)(GRD_DB *db, const char *collectionName, const char *filter, uint32_t flags);
typedef int32_t (*next_ptr)(GRD_ResultSet *resultSet);
typedef int32_t (*prev_ptr)(GRD_ResultSet *resultSet);
typedef int32_t (*get_value_ptr)(GRD_ResultSet *resultSet, char **value);
typedef int32_t (*fetch_ptr)(GRD_ResultSet *resultSet, GRD_KVItemT *key, GRD_KVItemT *value);
typedef int32_t (*free_value_ptr)(char *value);
typedef int32_t (*free_resultSet_ptr)(GRD_ResultSet *resultSet);
typedef int32_t (*kv_put_ptr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key,
    const GRD_KVItemT *value);
typedef int32_t (*kv_get_ptr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key,
    const GRD_KVItemT *value);
typedef int32_t (*kv_del_ptr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key);
typedef int32_t (*kv_scan_ptr)(GRD_DB *db, const char *collectionName, const GRD_KVItemT *key,
    KvScanModeE mode, GRD_ResultSet **resultSet);
typedef int32_t (*kv_freeItem_ptr)(GRD_KVItemT *item);
typedef int32_t (*kv_batchPrepare_ptr)(uint16_t itemNum, GRD_KVBatchT **batch);
typedef int32_t (*kv_batchPushback_ptr)(const void *key, uint32_t keyLen, const void *data,
    uint32_t dataLen, GRD_KVBatchT *batch);
typedef int32_t (*kv_batchDel_ptr)(GRD_DB *db, const char *collectionName, GRD_KVBatchT *batch);
typedef int32_t (*kv_batchDestory_ptr)(GRD_KVBatchT *batch);
typedef int32_t (*create_seq_ptr)(GRD_DB *db, const char *sequenceName, uint32_t flags);
typedef int32_t (*drop_seq_ptr)(GRD_DB *db, const char *sequenceName, uint32_t flags);
struct GRD_APIStruct {
    open_ptr GRD_DBOpenApi = nullptr;
    close_ptr GRD_DBCloseApi = nullptr;
    flush_ptr GRD_FlushApi = nullptr;
    index_preload_ptr GRD_IndexPreloadApi = nullptr;
    create_collection_ptr GRD_CreateCollectionApi = nullptr;
    drop_collection_ptr GRD_DropCollectionApi = nullptr;
    insert_doc_ptr GRD_InsertDocApi = nullptr;
    find_doc_ptr GRD_FindDocApi = nullptr;
    update_doc_ptr GRD_UpdateDocApi = nullptr;
    upsert_doc_ptr GRD_UpsertDocApi = nullptr;
    delete_doc_ptr GRD_DeleteDocApi = nullptr;
    next_ptr GRD_NextApi = nullptr;
    prev_ptr GRD_PrevApi = nullptr;
    get_value_ptr GRD_GetValueApi = nullptr;
    fetch_ptr GRD_FetchApi = nullptr;
    free_value_ptr GRD_FreeValueApi = nullptr;
    free_resultSet_ptr GRD_FreeResultSetApi = nullptr;
    kv_put_ptr GRD_KVPutApi = nullptr;
    kv_get_ptr GRD_KVGetApi = nullptr;
    kv_del_ptr GRD_KVDelApi = nullptr;
    kv_scan_ptr GRD_KVScanApi = nullptr;
    kv_freeItem_ptr GRD_KVFreeItemApi = nullptr;
    kv_batchPrepare_ptr GRD_KVBatchPrepareApi = nullptr;
    kv_batchPushback_ptr GRD_KVBatchPushbackApi = nullptr;
    kv_batchDel_ptr GRD_KVBatchDelApi = nullptr;
    kv_batchDestory_ptr GRD_KVBatchDestoryApi = nullptr;
    create_seq_ptr GRD_CreateSeqApi = nullptr;
    drop_seq_ptr GRD_DropSeqApi = nullptr;
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