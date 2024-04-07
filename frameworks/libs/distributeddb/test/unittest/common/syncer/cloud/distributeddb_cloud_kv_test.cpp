/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "platform_specific.h"
#include "virtual_cloud_db.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
KvStoreConfig g_config;
DistributedDBToolsUnitTest g_tool;
DBStatus g_kvDelegateStatus = INVALID_ARGS;
KvStoreNbDelegate* g_kvDelegatePtr = nullptr;
class DistributedDBCloudKvTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
protected:
    void BlockSync();
    DataBaseSchema GetDataBaseSchema();
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
};

void DistributedDBCloudKvTest::SetUpTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }
}

void DistributedDBCloudKvTest::TearDownTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBCloudKvTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B and C, and get a KvStoreNbDelegate as deviceA
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore(STORE_ID_1, option, [](DBStatus status, KvStoreNbDelegate *delegate) {
        g_kvDelegateStatus = status;
        g_kvDelegatePtr = delegate;
    });
    EXPECT_EQ(g_kvDelegateStatus, OK);
    EXPECT_NE(g_kvDelegatePtr, nullptr);

    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
    cloudDbs[USER_ID] = virtualCloudDb_;
    g_kvDelegatePtr->SetCloudDB(cloudDbs);
    std::map<std::string, DataBaseSchema> schemas;
    schemas[USER_ID] = GetDataBaseSchema();
    g_kvDelegatePtr->SetCloudDbSchema(schemas);
}

void DistributedDBCloudKvTest::TearDown()
{
    if (g_kvDelegatePtr != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
        g_kvDelegatePtr = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(STORE_ID_1);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
    virtualCloudDb_ = nullptr;
}

void DistributedDBCloudKvTest::BlockSync()
{
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    auto callback = [&cv, &dataMutex, &finish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    finish = true;
                }
                cv.notify_one();
            }
        }
    };
    CloudSyncOption option;
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    ASSERT_EQ(g_kvDelegatePtr->Sync(option, callback), OK);
    std::unique_lock<std::mutex> uniqueLock(dataMutex);
    cv.wait(uniqueLock, [&finish]() {
        return finish;
    });
}

DataBaseSchema DistributedDBCloudKvTest::GetDataBaseSchema()
{
    DataBaseSchema schema;
    TableSchema tableSchema;
    tableSchema.name = "HiCloudSystemTableForKVDB";
    Field field;
    field.colName = "key";
    field.type = TYPE_INDEX<std::string>;
    field.primary = true;
    tableSchema.fields.push_back(field);
    field.colName = "device";
    field.primary = false;
    tableSchema.fields.push_back(field);
    field.colName = "oridevice";
    tableSchema.fields.push_back(field);
    field.colName = "value";
    tableSchema.fields.push_back(field);
    field.colName = "device_create_time";
    field.type = TYPE_INDEX<int64_t>;
    tableSchema.fields.push_back(field);
    schema.tables.push_back(tableSchema);
    return schema;
}

/**
 * @tc.name: NormalSync001
 * @tc.desc: Test normal push sync for add data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync001, TestSize.Level0)
{
    g_kvDelegatePtr->Put({'k'}, {'v'});
    BlockSync();
}
}