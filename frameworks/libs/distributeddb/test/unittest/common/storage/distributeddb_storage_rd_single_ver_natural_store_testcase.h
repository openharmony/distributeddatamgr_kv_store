/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifdef USE_RD_KERNEL
#ifndef DISTRIBUTEDDB_STORAGE_SINGLE_VER_NATURAL_STORE_TESTCASE_H
#define DISTRIBUTEDDB_STORAGE_SINGLE_VER_NATURAL_STORE_TESTCASE_H

#include <gtest/gtest.h>
#include "db_errno.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "multi_ver_natural_store.h"
#include "sqlite_local_kvdb.h"
#include "rd_single_ver_natural_store.h"
#include "rd_single_ver_natural_store_connection.h"
#include "sqlite_utils.h"

struct SyncData {
    std::vector<uint8_t> hashKey;
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
    uint64_t timestamp;
    uint64_t flag;
    std::string deviceInfo;
};

class DistributedDBStorageRdSingleVerNaturalStoreTestCase final {
public:
    DistributedDBStorageRdSingleVerNaturalStoreTestCase() {};
    ~DistributedDBStorageRdSingleVerNaturalStoreTestCase() {};

    static void SyncDatabaseOperate001(DistributedDB::RdSingleVerNaturalStore *&store,
        DistributedDB::RdSingleVerNaturalStoreConnection *&connection);

    static void SyncDatabaseOperate003(DistributedDB::RdSingleVerNaturalStore *&store,
        DistributedDB::RdSingleVerNaturalStoreConnection *&connection);

    static void SyncDatabaseOperate005(DistributedDB::RdSingleVerNaturalStore *&store,
        DistributedDB::RdSingleVerNaturalStoreConnection *&connection);

    static void SyncDatabaseOperate006(DistributedDB::RdSingleVerNaturalStore *&store,
        DistributedDB::RdSingleVerNaturalStoreConnection *&connection);

private:
    static bool IsSqlinteExistKey(const std::vector<SyncData> &vecSyncData, const std::vector<uint8_t> &key);

    static void TestMetaDataPutAndGet(DistributedDB::RdSingleVerNaturalStore *&store,
    DistributedDB::RdSingleVerNaturalStoreConnection *&connection);

    static void TestMetaDataDeleteByPrefixKey(DistributedDB::RdSingleVerNaturalStore *&store,
        DistributedDB::RdSingleVerNaturalStoreConnection *&connection);

    static void DataBaseCommonPutOperate(DistributedDB::RdSingleVerNaturalStore *&store,
     DistributedDB::RdSingleVerNaturalStoreConnection *&connection, DistributedDB::IOption option);

    static void DataBaseCommonDeleteOperate(DistributedDB::RdSingleVerNaturalStore *&store,
    DistributedDB::RdSingleVerNaturalStoreConnection *&connection, DistributedDB::IOption option);

    static void DataBaseCommonGetOperate(DistributedDB::RdSingleVerNaturalStore *&store,
    DistributedDB::RdSingleVerNaturalStoreConnection *&connection, DistributedDB::IOption option);
};
#endif
#endif // USE_RD_KERNEL