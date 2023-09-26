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
#include "grd_base/grd_db_api.h"

#include <dlfcn.h>
#include <shared_mutex>

#include "check_common.h"
#include "doc_errno.h"
#include "document_store_manager.h"
#include "grd_base/grd_error.h"
#include "grd_base/grd_resultset_api.h"
#include "grd_document/grd_document_api.h"
#include "grd_kv/grd_kv_api.h"
#include "grd_shared_obj/grd_sequence_api.h"
#include "grd_type_inner.h"
#include "log_print.h"

using namespace DocumentDB;
namespace DocumentDB {
static void *library = nullptr;
static GRD_APIInfo GRD_DBApiInfo = GetApiInfoInstance();
int32_t GRD_DBOpenInner(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db)
{
    if (db == nullptr) {
        return GRD_INVALID_ARGS;
    }
    std::string path = (dbPath == nullptr ? "" : dbPath);
    std::string config = (configStr == nullptr ? "" : configStr);
    DocumentStore *store = nullptr;
    int ret = DocumentStoreManager::GetDocumentStore(path, config, flags, store);
    if (ret != E_OK || store == nullptr) {
        return TransferDocErr(ret);
    }

    *db = new (std::nothrow) GRD_DB();
    if (*db == nullptr) {
        (void)DocumentStoreManager::CloseDocumentStore(store, GRD_DB_CLOSE_IGNORE_ERROR);
        store = nullptr;
        return GRD_FAILED_MEMORY_ALLOCATE;
    }

    (*db)->store_ = store;
    return TransferDocErr(ret);
}

int32_t GRD_DBCloseInner(GRD_DB *db, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr) {
        return GRD_INVALID_ARGS;
    }

    int ret = DocumentStoreManager::CloseDocumentStore(db->store_, flags);
    if (ret != E_OK) {
        return TransferDocErr(ret);
    }

    db->store_ = nullptr;
    delete db;
    return GRD_OK;
}

int32_t GRD_FlushInner(GRD_DB *db, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr) {
        return GRD_INVALID_ARGS;
    }
    if (flags != GRD_DB_FLUSH_ASYNC && flags != GRD_DB_FLUSH_SYNC) {
        return GRD_INVALID_ARGS;
    }
    return GRD_OK;
}

int32_t GRD_IndexPreloadInner(GRD_DB *db, const char *collectionName)
{
    return GRD_OK; // No support;
}

void GRD_DBApiInitCommon(GRD_APIInfo &GRD_DBApiInfo)
{
    GRD_DBApiInfo.GRD_DBOpenApi = GRD_DBOpenInner;
    GRD_DBApiInfo.GRD_DBCloseApi = GRD_DBCloseInner;
    GRD_DBApiInfo.GRD_FlushApi = GRD_FlushInner;
    GRD_DBApiInfo.GRD_IndexPreloadApi = GRD_IndexPreloadInner;
    GRD_DBApiInfo.GRD_CreateCollectionApi = GRD_CreateCollectionInner;
    GRD_DBApiInfo.GRD_DropCollectionApi = GRD_DropCollectionInner;
    GRD_DBApiInfo.GRD_InsertDocApi = GRD_InsertDocInner;
    GRD_DBApiInfo.GRD_FindDocApi = GRD_FindDocInner;
    GRD_DBApiInfo.GRD_UpdateDocApi = GRD_UpdateDocInner;
    GRD_DBApiInfo.GRD_UpsertDocApi = GRD_UpsertDocInner;
    GRD_DBApiInfo.GRD_DeleteDocApi = GRD_DeleteDocInner;
    GRD_DBApiInfo.GRD_NextApi = GRD_NextInner;
    GRD_DBApiInfo.GRD_PrevApi = GRD_PrevInner;
    GRD_DBApiInfo.GRD_GetValueApi = GRD_GetValueInner;
    GRD_DBApiInfo.GRD_FetchApi = GRD_FetchInner;
    GRD_DBApiInfo.GRD_FreeValueApi = GRD_FreeValueInner;
    GRD_DBApiInfo.GRD_FreeResultSetApi = GRD_FreeResultSetInner;
    GRD_DBApiInfo.GRD_KVPutApi = GRD_KVPutInner;
    GRD_DBApiInfo.GRD_KVGetApi = GRD_KVGetInner;
    GRD_DBApiInfo.GRD_KVDelApi = GRD_KVDelInner;
    GRD_DBApiInfo.GRD_KVScanApi = GRD_KVScanInner;
    GRD_DBApiInfo.GRD_KVFreeItemApi = GRD_KVFreeItemInner;
    GRD_DBApiInfo.GRD_KVBatchPrepareApi = GRD_KVBatchPrepareInner;
    GRD_DBApiInfo.GRD_KVBatchPushbackApi = GRD_KVBatchPushbackInner;
    GRD_DBApiInfo.GRD_KVBatchDelApi = GRD_KVBatchDelInner;
    GRD_DBApiInfo.GRD_KVBatchDestoryApi = GRD_KVBatchDestoryInner;
    GRD_DBApiInfo.GRD_CreateSeqApi = GRD_CreateSeqInner;
    GRD_DBApiInfo.GRD_DropSeqApi = GRD_DropSeqInner;
}

void GRD_DBApiInitEnhance(GRD_APIInfo &GRD_DBApiInfo)
{
    GRD_DBApiInfo.GRD_DBOpenApi = (open_ptr)dlsym(library, "GRD_DBOpen");
    GRD_DBApiInfo.GRD_DBCloseApi = (close_ptr)dlsym(library, "GRD_DBClose");
    GRD_DBApiInfo.GRD_FlushApi = (flush_ptr)dlsym(library, "GRD_Flush");
    GRD_DBApiInfo.GRD_IndexPreloadApi = (index_preload_ptr)dlsym(library, "GRD_IndexPreload");
    GRD_DBApiInfo.GRD_CreateCollectionApi = (create_collection_ptr)dlsym(library, "GRD_CreateCollection");
    GRD_DBApiInfo.GRD_DropCollectionApi = (drop_collection_ptr)dlsym(library, "GRD_DropCollection");
    GRD_DBApiInfo.GRD_InsertDocApi = (insert_doc_ptr)dlsym(library, "GRD_InsertDoc");
    GRD_DBApiInfo.GRD_FindDocApi = (find_doc_ptr)dlsym(library, "GRD_FindDoc");
    GRD_DBApiInfo.GRD_UpdateDocApi = (update_doc_ptr)dlsym(library, "GRD_UpdateDoc");
    GRD_DBApiInfo.GRD_UpsertDocApi = (upsert_doc_ptr)dlsym(library, "GRD_UpsertDoc");
    GRD_DBApiInfo.GRD_DeleteDocApi = (delete_doc_ptr)dlsym(library, "GRD_DeleteDoc");
    GRD_DBApiInfo.GRD_NextApi = (next_ptr)dlsym(library, "GRD_Next");
    GRD_DBApiInfo.GRD_PrevApi = (prev_ptr)dlsym(library, "GRD_Prev");
    GRD_DBApiInfo.GRD_GetValueApi = (get_value_ptr)dlsym(library, "GRD_GetValue");
    GRD_DBApiInfo.GRD_FetchApi = (fetch_ptr)dlsym(library, "GRD_Fetch");
    GRD_DBApiInfo.GRD_FreeValueApi = (free_value_ptr)dlsym(library, "GRD_FreeValue");
    GRD_DBApiInfo.GRD_FreeResultSetApi = (free_resultSet_ptr)dlsym(library, "GRD_FreeResultSet");
    GRD_DBApiInfo.GRD_KVPutApi = (kv_put_ptr)dlsym(library, "GRD_KVPut");
    GRD_DBApiInfo.GRD_KVGetApi = (kv_get_ptr)dlsym(library, "GRD_KVGet");
    GRD_DBApiInfo.GRD_KVDelApi = (kv_del_ptr)dlsym(library, "GRD_KVDel");
    GRD_DBApiInfo.GRD_KVScanApi = (kv_scan_ptr)dlsym(library, "GRD_KVScan");
    GRD_DBApiInfo.GRD_KVFreeItemApi = (kv_freeItem_ptr)dlsym(library, "GRD_KVFreeItem");
    GRD_DBApiInfo.GRD_KVBatchPrepareApi = (kv_batchPrepare_ptr)dlsym(library, "GRD_KVBatchPrepare");
    GRD_DBApiInfo.GRD_KVBatchPushbackApi = (kv_batchPushback_ptr)dlsym(library, "GRD_KVBatchPushback");
    GRD_DBApiInfo.GRD_KVBatchDelApi = (kv_batchDel_ptr)dlsym(library, "GRD_KVBatchDel");
    GRD_DBApiInfo.GRD_KVBatchDestoryApi = (kv_batchDestory_ptr)dlsym(library, "GRD_KVBatchDestory");
    GRD_DBApiInfo.GRD_CreateSeqApi = (create_seq_ptr)dlsym(library, "GRD_CreateSeq");
    GRD_DBApiInfo.GRD_DropSeqApi = (drop_seq_ptr)dlsym(library, "GRD_DropSeq");
}

GRD_APIInfo GetApiInfoInstance()
{
    GRD_APIInfo GRD_TempApiStruct;
    library = dlopen("/system/lib64/libgaussdb_rd.z.so", RTLD_LAZY);
    if (!library) {
        GRD_DBApiInitCommon(GRD_TempApiStruct); // When calling specific function, read whether init is successful.
    } else {
        GRD_DBApiInitEnhance(GRD_TempApiStruct);
    }
    return GRD_TempApiStruct;
}
} // namespace DocumentDB

GRD_API int32_t GRD_DBOpen(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db)
{
    if (GRD_DBApiInfo.GRD_DBOpenApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.GRD_DBOpenApi(dbPath, configStr, flags, db);
}

GRD_API int32_t GRD_DBClose(GRD_DB *db, uint32_t flags)
{
    if (GRD_DBApiInfo.GRD_DBCloseApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.GRD_DBCloseApi(db, flags);
}

GRD_API int32_t GRD_Flush(GRD_DB *db, uint32_t flags)
{
    if (GRD_DBApiInfo.GRD_FlushApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.GRD_FlushApi(db, flags);
}

GRD_API int32_t GRD_IndexPreload(GRD_DB *db, const char *collectionName)
{
    if (GRD_DBApiInfo.GRD_IndexPreloadApi == nullptr) {
        return GRD_INNER_ERR;
    }
    return GRD_DBApiInfo.GRD_IndexPreloadApi(db, collectionName);
}