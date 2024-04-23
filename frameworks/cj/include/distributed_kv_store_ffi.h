/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DISTRIBUTED_KV_STORE_FFI_H
#define DISTRIBUTED_KV_STORE_FFI_H

#include <cstdint>
#include "napi_base_context.h"
#include "ffi_remote_data.h"
#include "cj_common_ffi.h"
#include "data_query.h"

#include "distributed_kv_store_log.h"
#include "distributed_kv_store_impl.h"

namespace OHOS {
namespace DistributedKVStore {
    
extern "C" {
    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreCreateKVManager(const char* boudleName,
        OHOS::AbilityRuntime::Context* context);

    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreGetKVStore(int64_t id, const char* storeId,
        CJOptions options, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreCloseKVStore(int64_t id, const char* appId, const char* storeId);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreDeleteKVStore(int64_t id, const char* appId, const char* storeId);

    FFI_EXPORT CArrStr FfiOHOSDistributedKVStoreGetAllKVStoreId(int64_t id, const char* appId, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStorePut(int64_t id, const char* key, ValueType value);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStorePutBatch(int64_t id, const CArrEntry cArrEntry);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreDelete(int64_t id, const char* key);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreDeleteBatch(int64_t id, const CArrStr cArrStr);

    FFI_EXPORT ValueType FfiOHOSDistributedKVStoreSingleKVStoreGet(int64_t id, const char* key, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreBackup(int64_t id, const char* file);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreRestore(int64_t id, const char* file);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreStartTransaction(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreCommit(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreRollback(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreEnableSync(int64_t id, bool enabled);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreSetSyncParam(int64_t id, uint32_t defaultAllowedDelayMs);

    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreQueryConstructor();

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryReset(int64_t id);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryEqualTo(int64_t id, const char* field, ValueType value);

    FFI_EXPORT ValueType FfiOHOSDistributedKVStoreDeviceKVStoreGet(int64_t id, const char* deviceId, const char* key,
        int32_t* errCode);

    FFI_EXPORT CArrEntry FfiOHOSDistributedKVStoreDeviceKVStoreGetEntries(int64_t id, const char* deviceId,
        const char* keyPrefix, int32_t* errCode);

    FFI_EXPORT CArrEntry FfiOHOSDistributedKVStoreDeviceKVStoreGetEntriesQuery(int64_t id, const char* deviceId,
        int64_t queryId, int32_t* errCode);

    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSet(int64_t id, const char* deviceId,
        const char* keyPrefix, int32_t* errCode);

    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSetQuery(int64_t id, const char* deviceId,
        int64_t queryId, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSize(int64_t id, const char* deviceId,
        int64_t queryId, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreKVStoreResultSetGetCount(int64_t id);
}

}
}

#endif