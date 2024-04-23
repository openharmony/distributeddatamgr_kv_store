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

#ifndef RD_UTILS_H
#define RD_UTILS_H
#include <vector>
#include "db_errno.h"
#include "grd_db_api.h"
#include "grd_error.h"
#include "grd_kv_api.h"
#include "grd_type_export.h"
#include "kv_store_nb_delegate.h"
#include "sqlite_single_ver_storage_executor.h"
#include "grd_document_api.h"

namespace DistributedDB {

const std::string SYNC_COLLECTION_NAME = "naturalbase_kv_sync_data";

std::string InitRdConfig();

int TransferGrdErrno(int err);

std::vector<uint8_t> KvItemToBlob(GRD_KVItemT &item);

int GetCollNameFromType(SingleVerDataType type, std::string &collName);

int RdKVPut(GRD_DB *db, const char *collectionName, const Key &key, const Value &value);

int RdKVGet(GRD_DB *db, const char *collectionName, const Key &key, Value &value);

int RdKVDel(GRD_DB *db, const char *collectionName, const Key &key);

int RdKVScan(GRD_DB *db, const char *collectionName, const Key &key, GRD_KvScanModeE mode,
    GRD_ResultSet **resultSet);

int RdKVRangeScan(GRD_DB *db, const char *collectionName, const Key &beginKey, const Key &endKey,
    GRD_ResultSet **resultSet);

int RdKvFetch(GRD_ResultSet *resultSet, Key &key, Value &value);

int RdFreeResultSet(GRD_ResultSet *resultSet);

int RdDBClose(GRD_DB *db, uint32_t flags);

int RdKVBatchPrepare(uint16_t itemNum, GRD_KVBatchT **batch);

int RdKVBatchPushback(GRD_KVBatchT *batch, const Key &key, const Value &value);

int RdKVBatchPut(GRD_DB *db, const char *kvTableName, GRD_KVBatchT *batch);

int RdFlush(GRD_DB *db, uint32_t flags);

int RdKVBatchDel(GRD_DB *db, const char *kvTableName, GRD_KVBatchT *batch);

int RdKVBatchDestroy(GRD_KVBatchT *batch);

int RdDbOpen(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB *&db);

int RdIndexPreload(GRD_DB *&db, const char *collectionName);

bool CheckRdOption(const KvStoreNbDelegate::Option &option,
    const std::function<void(DBStatus, KvStoreNbDelegate *)> &callback);
} // namespace DistributedDB
#endif // RD_UTILS_H