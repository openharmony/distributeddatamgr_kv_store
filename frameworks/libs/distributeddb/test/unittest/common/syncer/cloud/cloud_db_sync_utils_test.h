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

#ifndef CLOUD_DB_SYNC_UTILS_TEST_H
#define CLOUD_DB_SYNC_UTILS_TEST_H

#include <gtest/gtest.h>
#include <iostream>
#include "cloud/cloud_storage_utils.h"
#include "cloud/cloud_db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "process_system_api_adapter_impl.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "sqlite_relational_store.h"
#include "sqlite_relational_utils.h"
#include "store_observer.h"
#include "time_helper.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"

namespace DistributedDB {
    using namespace DistributedDBUnitTest;
    using CloudSyncStatusCallback = std::function<void(const std::map<std::string, SyncProcess> &onProcess)>;

class CloudDBSyncUtilsTest {
public:
    static void SetStorePath(const std::string &path);

    static void InitSyncUtils(const std::vector<Field> &cloudField, RelationalStoreObserverUnitTest *&observer,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb, std::shared_ptr<VirtualAssetLoader> &virtualAssetLoader,
        RelationalStoreDelegate *&delegate);

    static void CreateUserDBAndTable(sqlite3 *&db, std::string sql);

    static void InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb);

    static void UpdateCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb);

    static void DeleteCloudTableRecordByGid(int64_t begin, int64_t count,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb);

    static void DeleteUserTableRecord(sqlite3 *&db, int64_t begin, int64_t count);

    static void GetCallback(SyncProcess &syncProcess, CloudSyncStatusCallback &callback,
        std::vector<SyncProcess> &expectProcess);

    static void CheckCloudTotalCount(std::vector<int64_t> expectCounts,
        const std::shared_ptr<VirtualCloudDb> &virtualCloudDb);

    static void WaitForSyncFinish(SyncProcess &syncProcess, const int64_t &waitTime);

    static void callSync(const std::vector<std::string> &tableNames, SyncMode mode, DBStatus dbStatus,
        RelationalStoreDelegate *&delegate);

    static void CloseDb(RelationalStoreObserverUnitTest *&observer,
        std::shared_ptr<VirtualCloudDb> &virtualCloudDb, RelationalStoreDelegate *&delegate);

    static int QueryCountCallback(void *data, int count, char **colValue, char **colName);

    static void CheckDownloadResult(sqlite3 *&db, std::vector<int64_t> expectCounts, const std::string &keyStr);

    static void CheckLocalRecordNum(sqlite3 *&db, const std::string &tableName, int count);

    static void GetCloudDbSchema(const std::string &tableName, const std::vector<Field> &cloudField,
        DataBaseSchema &dataBaseSchema);

    static void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
        const std::string &storeId, RelationalDBProperties &properties);

    static void CheckCount(sqlite3 *db, const std::string &sql, int64_t count);

    static void GetHashKey(const std::string &tableName, const std::string &condition, sqlite3 *db,
        std::vector<std::vector<uint8_t>> &hashKey);
};
}

#endif // CLOUD_DB_SYNC_UTILS_TEST_H
