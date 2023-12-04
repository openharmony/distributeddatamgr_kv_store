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
#else
#define GRD_API
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
#define GRD_DB_OPEN_SHARED_READ_ONLY 0x08

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
    KV_SCAN_RANGE = 3,
    KV_SCAN_BUTT
} GRD_KvScanModeE;

typedef struct GRD_FilterOption {
    GRD_KvScanModeE mode;
    GRD_KVItem begin;
    GRD_KVItem end;
} GRD_FilterOptionT;

typedef struct GRD_ResultSet GRD_ResultSet;
typedef struct GRD_DB GRD_DB;
typedef struct GRD_KVBatch GRD_KVBatchT;

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