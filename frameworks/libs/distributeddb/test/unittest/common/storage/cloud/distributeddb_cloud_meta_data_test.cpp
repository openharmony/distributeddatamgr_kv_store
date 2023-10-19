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
#include "cloud/cloud_meta_data.h"

#include <gtest/gtest.h>

#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"
#include "distributeddb_data_generate_unit_test.h"
#include "relational_sync_able_storage.h"
#include "relational_store_instance.h"
#include "sqlite_relational_store.h"
#include "log_table_manager_factory.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr auto TABLE_NAME_1 = "tableName1";
    constexpr auto TABLE_NAME_2 = "tableName2";
    const string STORE_ID = "Relational_Store_ID";
    const string TABLE_NAME = "cloudData";
    string TEST_DIR;
    string g_storePath;
    string g_dbDir;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;
    IRelationalStore *g_store = nullptr;
    std::shared_ptr<StorageProxy> g_storageProxy = nullptr;

    void CreateDB()
    {
        sqlite3 *db = nullptr;
        int errCode = sqlite3_open(g_storePath.c_str(), &db);
        if (errCode != SQLITE_OK) {
            LOGE("open db failed:%d", errCode);
            sqlite3_close(db);
            return;
        }

        const string sql =
            "PRAGMA journal_mode=WAL;";
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql.c_str()), E_OK);
        sqlite3_close(db);
    }

    void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
        RelationalDBProperties &properties)
    {
        properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
        properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
        properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
        properties.SetStringProp(RelationalDBProperties::STORE_ID, STORE_ID);
        std::string identifier = userId + "-" + appId + "-" + STORE_ID;
        std::string hashIdentifier = DBCommon::TransferHashString(identifier);
        properties.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
    }

    const RelationalSyncAbleStorage *GetRelationalStore()
    {
        RelationalDBProperties properties;
        InitStoreProp(g_storePath, APP_ID, USER_ID, properties);
        int errCode = E_OK;
        g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
        if (g_store == nullptr) {
            LOGE("Get db failed:%d", errCode);
            return nullptr;
        }
        return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
    }

    std::shared_ptr<StorageProxy> GetStorageProxy(ICloudSyncStorageInterface *store)
    {
        return StorageProxy::GetCloudDb(store);
    }

    void SetAndGetWaterMark(TableName tableName, Timestamp mark)
    {
        Timestamp retMark;
        EXPECT_EQ(g_storageProxy->PutLocalWaterMark(tableName, mark), E_OK);
        EXPECT_EQ(g_storageProxy->GetLocalWaterMark(tableName, retMark), E_OK);
        EXPECT_EQ(retMark, mark);
    }

    void SetAndGetWaterMark(TableName tableName, std::string mark)
    {
        std::string retMark;
        EXPECT_EQ(g_storageProxy->SetCloudWaterMark(tableName, mark), E_OK);
        EXPECT_EQ(g_storageProxy->GetCloudWaterMark(tableName, retMark), E_OK);
        EXPECT_EQ(retMark, mark);
    }

    class DistributedDBCloudMetaDataTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    };

    void DistributedDBCloudMetaDataTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(TEST_DIR);
        LOGD("test dir is %s", TEST_DIR.c_str());
        g_dbDir = TEST_DIR + "/";
        g_storePath =  g_dbDir + STORE_ID + ".db";
        DistributedDBToolsUnitTest::RemoveTestDbFiles(TEST_DIR);
    }

    void DistributedDBCloudMetaDataTest::TearDownTestCase(void)
    {
    }

    void DistributedDBCloudMetaDataTest::SetUp(void)
    {
        DistributedDBToolsUnitTest::PrintTestCaseInfo();
        LOGD("Test dir is %s", TEST_DIR.c_str());
        CreateDB();
        ASSERT_EQ(g_mgr.OpenStore(g_storePath, STORE_ID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
        ASSERT_NE(g_delegate, nullptr);
        g_storageProxy = GetStorageProxy((ICloudSyncStorageInterface *) GetRelationalStore());
    }

    void DistributedDBCloudMetaDataTest::TearDown(void)
    {
        RefObject::DecObjRef(g_store);
        if (g_delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
            g_delegate = nullptr;
            g_storageProxy = nullptr;
        }
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(TEST_DIR) != 0) {
            LOGE("rm test db files error.");
        }
    }

    /**
     * @tc.name: CloudMetaDataTest001
     * @tc.desc: Set and get local water mark with various value
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, CloudMetaDataTest001, TestSize.Level0)
    {
        SetAndGetWaterMark(TABLE_NAME_1, 123); // 123 is a random normal value, not magic number
        SetAndGetWaterMark(TABLE_NAME_1, 0); // 0 is used for test, not magic number
        SetAndGetWaterMark(TABLE_NAME_1, -1); // -1 is used for test, not magic number
        SetAndGetWaterMark(TABLE_NAME_1, UINT64_MAX);
        SetAndGetWaterMark(TABLE_NAME_1, UINT64_MAX + 1);
    }

    /**
     * @tc.name: CloudMetaDataTest002
     * @tc.desc: Set and get cloud water mark with various value
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, CloudMetaDataTest002, TestSize.Level0)
    {
        SetAndGetWaterMark(TABLE_NAME_1, "");
        SetAndGetWaterMark(TABLE_NAME_1, "123");
        SetAndGetWaterMark(TABLE_NAME_1, "1234567891012112345678910121");
        SetAndGetWaterMark(TABLE_NAME_1, "ABCDEFGABCDEFGABCDEFGABCDEFG");
        SetAndGetWaterMark(TABLE_NAME_1, "abcdefgabcdefgabcdefgabcdefg");
        SetAndGetWaterMark(TABLE_NAME_1, "ABCDEFGABCDEFGabcdefgabcdefg");
        SetAndGetWaterMark(TABLE_NAME_1, "123456_GABEFGab@中文字符cdefg");
    }

    /**
     * @tc.name: CloudMetaDataTest003
     * @tc.desc:
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: wanyi
     */
    HWTEST_F(DistributedDBCloudMetaDataTest, CloudMetaDataTest003, TestSize.Level0)
    {
        std::string retMark;
        EXPECT_EQ(g_storageProxy->GetCloudWaterMark(TABLE_NAME_2, retMark), E_OK);
        EXPECT_EQ(retMark, "");

        Timestamp retLocalMark;
        EXPECT_EQ(g_storageProxy->GetLocalWaterMark(TABLE_NAME_2, retLocalMark), E_OK);
        EXPECT_EQ(retLocalMark, 0u);
    }
}
