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

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreOnDistributedDataServiceDie(int64_t id, void (*callbackId)());

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreOffDistributedDataServiceDie(int64_t id, void (*callbackId)());

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreOffAllDistributedDataServiceDie(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStorePut(int64_t id, const char* key, ValueType value);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStorePutBatch(int64_t id, const CArrEntry cArrEntry);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreDelete(int64_t id, const char* key);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreDeleteBatch(int64_t id, const CArrStr cArrStr);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreRemoveDeviceData(int64_t id, const char* deviceId);

    FFI_EXPORT ValueType FfiOHOSDistributedKVStoreSingleKVStoreGet(int64_t id, const char* key, int32_t* errCode);

    FFI_EXPORT CArrEntry FfiOHOSDistributedKVStoreSingleKVStoreGetEntriesByQuery(int64_t id,
        int64_t queryId, int32_t* errCode);

    FFI_EXPORT CArrEntry FfiOHOSDistributedKVStoreSingleKVStoreGetEntriesByString(int64_t id,
        const char* prefix, int32_t* errCode);

    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreSingleKVStoreGetResultSetByString(int64_t id,
        const char* keyPrefix, int32_t* errCode);

    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreSingleKVStoreGetResultSetByQuery(int64_t id,
        int64_t queryId, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreCloseResultSet(int64_t id, int64_t resultSetId);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreGetResultSize(int64_t id,
        int64_t queryId, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreBackup(int64_t id, const char* file);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreRestore(int64_t id, const char* file);

    FFI_EXPORT CStringNum FfiOHOSDistributedKVStoreSingleKVStoreDeleteBackup(int64_t id,
        const CArrStr cArrStr, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreStartTransaction(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreCommit(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreRollback(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreEnableSync(int64_t id, bool enabled);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreSetSyncRange(int64_t id, const CArrStr localLabels,
        const CArrStr remoteSupportLabels);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreSetSyncParam(int64_t id, uint32_t defaultAllowedDelayMs);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreSync(int64_t id, const CArrStr deviceIds, uint8_t mode,
        uint32_t delayMs);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreSyncByQuery(int64_t id, const CArrStr deviceIds,
        int64_t queryId, uint8_t mode, uint32_t delayMs);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreOnDataChange(int64_t id, uint8_t type,
        void (*callbackId)(const CChangeNotification valueRef));

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreOffDataChange(int64_t id,
        void (*callbackId)(const CChangeNotification valueRef));

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreOffAllDataChange(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreOnSyncComplete(int64_t id,
        void (*callbackId)(const CStringNum valueRef));

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreOffAllSyncComplete(int64_t id);

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreSingleKVStoreGetSecurityLevel(int64_t id, int32_t* errCode);

    FFI_EXPORT int64_t FfiOHOSDistributedKVStoreQueryConstructor();

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryReset(int64_t id);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryEqualTo(int64_t id, const char* field, ValueType value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryNotEqualTo(int64_t id, const char* field, ValueType value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryGreaterThan(int64_t id, const char* field, ValueType value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryLessThan(int64_t id, const char* field, ValueType value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryGreaterThanOrEqualTo(int64_t id, const char* field, ValueType value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryLessThanOrEqualTo(int64_t id, const char* field, ValueType value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryIsNull(int64_t id, const char* field);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryInNumber(int64_t id, const char* field, const CArrNumber valueList);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryInString(int64_t id, const char* field, const CArrStr valueList);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryNotInNumber(int64_t id, const char* field,
        const CArrNumber valueList);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryNotInString(int64_t id, const char* field, const CArrStr valueList);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryLike(int64_t id, const char* field, const char* value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryUnlike(int64_t id, const char* field, const char* value);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryAnd(int64_t id);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryOr(int64_t id);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryOrderByAsc(int64_t id, const char* field);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryOrderByDesc(int64_t id, const char* field);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryLimit(int64_t id, int32_t total, int32_t offset);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryIsNotNull(int64_t id, const char* field);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryBeginGroup(int64_t id);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryEndGroup(int64_t id);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryPrefixKey(int64_t id, const char* prefix);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQuerySetSuggestIndex(int64_t id, const char* index);

    FFI_EXPORT void FfiOHOSDistributedKVStoreQueryDeviceId(int64_t id, const char* deviceId);

    FFI_EXPORT const char* FfiOHOSDistributedKVStoreQueryGetSqlLike(int64_t id);

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

    FFI_EXPORT int32_t FfiOHOSDistributedKVStoreKVStoreResultSetGetPosition(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToFirst(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToLast(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToNext(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToPrevious(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetMove(int64_t id, int32_t offset);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToPosition(int64_t id, int32_t position);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetIsFirst(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetIsLast(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetIsBeforeFirst(int64_t id);

    FFI_EXPORT bool FfiOHOSDistributedKVStoreKVStoreResultSetIsAfterLast(int64_t id);

    FFI_EXPORT CEntry FfiOHOSDistributedKVStoreKVStoreResultSetGetEntry(int64_t id);
}
}
}

#endif