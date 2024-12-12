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

#include "cj_ffi/cj_common_ffi.h"

extern "C" {
    FFI_EXPORT int FfiOHOSDistributedKVStoreCreateKVManager = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreGetKVStore = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreCloseKVStore = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreDeleteKVStore = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreGetAllKVStoreId = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStorePut = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStorePutBatch = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreDelete = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreDeleteBatch = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreGet = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreBackup = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreRestore = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreStartTransaction = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreCommit = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreRollback = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreEnableSync = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreSingleKVStoreSetSyncParam = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreQueryConstructor = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreQueryReset = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreQueryEqualTo = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreDeviceKVStoreGet = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreDeviceKVStoreGetEntries = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreDeviceKVStoreGetEntriesQuery = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSet = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSetQuery = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSize = 0;

    FFI_EXPORT int FfiOHOSDistributedKVStoreKVStoreResultSetGetCount = 0;
}