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

#include "grd_api_manager.h"

#ifndef _WIN32
#include <dlfcn.h>
#endif

#include "check_common.h"
#include "doc_errno.h"
#include "document_store_manager.h"
#include "grd_base/grd_error.h"
#include "grd_db_api_inner.h"
#include "grd_document_api_inner.h"
#include "grd_kv_api_inner.h"
#include "grd_resultset_api_inner.h"
#include "grd_sequence_api_inner.h"
#include "grd_type_inner.h"
#include "rd_log_print.h"

#ifndef _WIN32
static void *g_library = nullptr;
#endif

static bool g_isHashLib = false;

namespace DocumentDB {
void GRD_DBApiInitCommon(GRD_APIInfo &GRD_DBApiInfo)
{
    GRD_DBApiInfo.DBOpenApi = GRD_DBOpenInner;
    GRD_DBApiInfo.DBCloseApi = GRD_DBCloseInner;
    GRD_DBApiInfo.FlushApi = GRD_FlushInner;
    GRD_DBApiInfo.IndexPreloadApi = GRD_IndexPreloadInner;
    GRD_DBApiInfo.CreateCollectionApi = GRD_CreateCollectionInner;
    GRD_DBApiInfo.DropCollectionApi = GRD_DropCollectionInner;
    GRD_DBApiInfo.InsertDocApi = GRD_InsertDocInner;
    GRD_DBApiInfo.FindDocApi = GRD_FindDocInner;
    GRD_DBApiInfo.UpdateDocApi = GRD_UpdateDocInner;
    GRD_DBApiInfo.UpsertDocApi = GRD_UpsertDocInner;
    GRD_DBApiInfo.DeleteDocApi = GRD_DeleteDocInner;
    GRD_DBApiInfo.NextApi = GRD_NextInner;
    GRD_DBApiInfo.PrevApi = GRD_PrevInner;
    GRD_DBApiInfo.GetValueApi = GRD_GetValueInner;
    GRD_DBApiInfo.FetchApi = GRD_FetchInner;
    GRD_DBApiInfo.FreeValueApi = GRD_FreeValueInner;
    GRD_DBApiInfo.FreeResultSetApi = GRD_FreeResultSetInner;
    GRD_DBApiInfo.KVPutApi = GRD_KVPutInner;
    GRD_DBApiInfo.KVGetApi = GRD_KVGetInner;
    GRD_DBApiInfo.KVDelApi = GRD_KVDelInner;
    GRD_DBApiInfo.KVScanApi = GRD_KVScanInner;
    GRD_DBApiInfo.KVFilterApi = GRD_KVFilterInner;
    GRD_DBApiInfo.KVGetSizeApi = GRD_KVGetSizeInner;
    GRD_DBApiInfo.GetItemApi = GRD_GetItemInner;
    GRD_DBApiInfo.KVFreeItemApi = GRD_KVFreeItemInner;
    GRD_DBApiInfo.KVBatchPrepareApi = GRD_KVBatchPrepareInner;
    GRD_DBApiInfo.KVBatchPushbackApi = GRD_KVBatchPushbackInner;
    GRD_DBApiInfo.KVBatchPutApi = GRD_KVBatchPutInner;
    GRD_DBApiInfo.KVBatchDelApi = GRD_KVBatchDelInner;
    GRD_DBApiInfo.KVBatchDestoryApi = GRD_KVBatchDestroyInner;
}

void GRD_DBApiInitEnhance(GRD_APIInfo &GRD_DBApiInfo)
{
#ifndef _WIN32
    GRD_DBApiInfo.DBOpenApi = (DBOpen)dlsym(g_library, "GRD_DBOpen");
    GRD_DBApiInfo.DBCloseApi = (DBClose)dlsym(g_library, "GRD_DBClose");
    GRD_DBApiInfo.FlushApi = (DBFlush)dlsym(g_library, "GRD_Flush");
    GRD_DBApiInfo.IndexPreloadApi = (IndexPreload)dlsym(g_library, "GRD_IndexPreload");
    GRD_DBApiInfo.CreateCollectionApi = (CreateCollection)dlsym(g_library, "GRD_CreateCollection");
    GRD_DBApiInfo.DropCollectionApi = (DropCollection)dlsym(g_library, "GRD_DropCollection");
    GRD_DBApiInfo.InsertDocApi = (InsertDoc)dlsym(g_library, "GRD_InsertDoc");
    GRD_DBApiInfo.FindDocApi = (FindDoc)dlsym(g_library, "GRD_FindDoc");
    GRD_DBApiInfo.UpdateDocApi = (UpdateDoc)dlsym(g_library, "GRD_UpdateDoc");
    GRD_DBApiInfo.UpsertDocApi = (UpsertDoc)dlsym(g_library, "GRD_UpsertDoc");
    GRD_DBApiInfo.DeleteDocApi = (DeleteDoc)dlsym(g_library, "GRD_DeleteDoc");
    GRD_DBApiInfo.NextApi = (ResultNext)dlsym(g_library, "GRD_Next");
    GRD_DBApiInfo.PrevApi = (ResultPrev)dlsym(g_library, "GRD_Prev");
    GRD_DBApiInfo.GetValueApi = (GetValue)dlsym(g_library, "GRD_GetValue");
    GRD_DBApiInfo.FetchApi = (Fetch)dlsym(g_library, "GRD_Fetch");
    GRD_DBApiInfo.FreeValueApi = (FreeValue)dlsym(g_library, "GRD_FreeValue");
    GRD_DBApiInfo.FreeResultSetApi = (FreeResultSet)dlsym(g_library, "GRD_FreeResultSet");
    GRD_DBApiInfo.KVPutApi = (KVPut)dlsym(g_library, "GRD_KVPut");
    GRD_DBApiInfo.KVGetApi = (KVGet)dlsym(g_library, "GRD_KVGet");
    GRD_DBApiInfo.KVDelApi = (KVDel)dlsym(g_library, "GRD_KVDel");
    GRD_DBApiInfo.KVScanApi = (KVScan)dlsym(g_library, "GRD_KVScan");
    GRD_DBApiInfo.KVFilterApi = (KVFilter)dlsym(g_library, "GRD_KVFilter");
    GRD_DBApiInfo.KVGetSizeApi = (KVGetSize)dlsym(g_library, "GRD_KVGetSize");
    GRD_DBApiInfo.GetItemApi = (GetItem)dlsym(g_library, "GRD_GetItem");
    GRD_DBApiInfo.KVFreeItemApi = (KVFreeItem)dlsym(g_library, "GRD_KVFreeItem");
    GRD_DBApiInfo.KVBatchPrepareApi = (KVBatchPrepare)dlsym(g_library, "GRD_KVBatchPrepare");
    GRD_DBApiInfo.KVBatchPushbackApi = (KVBatchPushback)dlsym(g_library, "GRD_KVBatchPushback");
    GRD_DBApiInfo.KVBatchPutApi = (KVBatchPut)dlsym(g_library, "GRD_KVBatchPut");
    GRD_DBApiInfo.KVBatchDelApi = (KVBatchDel)dlsym(g_library, "GRD_KVBatchDel");
    GRD_DBApiInfo.KVBatchDestoryApi = (KVBatchDestory)dlsym(g_library, "GRD_KVBatchDestroy");
#endif
}

void SetLibType(bool isHash)
{
    g_isHashLib = isHash;
}

GRD_APIInfo GetApiInfoInstance()
{
    GRD_APIInfo GRD_TempApiStruct;
#ifndef _WIN32
    std::string libPath = g_isHashLib ? "libgaussdb_rd_vector.z.so" : "libgaussdb_rd.z.so";
    g_library = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!g_library) {
        GRD_DBApiInitCommon(GRD_TempApiStruct); // When calling specific function, read whether init is successful.
    } else {
        GRD_DBApiInitEnhance(GRD_TempApiStruct);
    }
#endif
    return GRD_TempApiStruct;
}
} // namespace DocumentDB
