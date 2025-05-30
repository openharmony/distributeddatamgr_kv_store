/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "db_constant.h"
#include "db_properties.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "platform_specific.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "relational_virtual_device.h"
#include "runtime_config.h"
#include "virtual_relational_ver_sync_db_interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "rdb_stroe_sync_test";
    const string USER_ID_1 = "userId1";
    const string USER_ID_2 = "userId2";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const int WAIT_TIME = 1000; // 1000ms
    const std::string g_tableName = "TEST_TABLE";
    std::string g_identifier;

    RelationalStoreManager g_mgr1(APP_ID, USER_ID_1);
    RelationalStoreManager g_mgr2(APP_ID, USER_ID_2);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    RelationalStoreDelegate* g_rdbDelegatePtr1 = nullptr;
    RelationalStoreDelegate* g_rdbDelegatePtr2 = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    RelationalVirtualDevice *g_deviceB = nullptr;
    RelationalVirtualDevice *g_deviceC = nullptr;
    std::string g_dbDir;
    std::string g_storePath1;
    std::string g_storePath2;
    RelationalStoreObserverUnitTest *g_observer = nullptr;

    auto g_syncActivationCheckCallback1 = [] (const std::string &userId, const std::string &appId,
        const std::string &storeId)-> bool {
        if (userId == USER_ID_2) {
            LOGE("active call back1, active user2");
            return true;
        } else {
            LOGE("active call back1, no need to active user1");
            return false;
        }
        return true;
    };
    auto g_syncActivationCheckCallback2 = [] (const std::string &userId, const std::string &appId,
        const std::string &storeId)-> bool {
        if (userId == USER_ID_1) {
            LOGE("active call back2, active user1");
            return true;
        } else {
            LOGE("active call back2, no need to active user2");
            return false;
        }
        return true;
    };

    int DropTable(sqlite3 *db, const std::string &tableName)
    {
        std::string sql = "DROP TABLE IF EXISTS " + tableName + ";";
        char *errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != 0) {
            LOGE("failed to drop table: %s, errMsg: %s", tableName.c_str(), errMsg);
        }
        return rc;
    }

    int CreateTable(sqlite3 *db, const std::string &tableName)
    {
        std::string sql = "CREATE TABLE " + tableName + "(id int, name text);";
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        return rc;
    }

    int InsertValue(sqlite3 *db, const std::string &tableName)
    {
        std::string sql = "insert into " + tableName + " values(1, 'aaa');";
        return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    }

    int GetDB(sqlite3 *&db, const std::string &dbPath)
    {
        int flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        int rc = sqlite3_open_v2(dbPath.c_str(), &db, flag, nullptr);
        if (rc != SQLITE_OK) {
            return rc;
        }
        EXPECT_EQ(SQLiteUtils::RegisterCalcHash(db), E_OK);
        EXPECT_EQ(SQLiteUtils::RegisterGetSysTime(db), E_OK);
        EXPECT_EQ(sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr), SQLITE_OK);
        return rc;
    }

    void PrepareVirtualDeviceEnv(const std::string &tableName, const std::string &dbPath,
        std::vector<RelationalVirtualDevice *> &remoteDeviceVec)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, dbPath), SQLITE_OK);
        TableInfo tableInfo;
        SQLiteUtils::AnalysisSchema(db, tableName, tableInfo);
        for (auto &dev : remoteDeviceVec) {
            std::vector<FieldInfo> fieldInfoList = tableInfo.GetFieldInfos();
            dev->SetLocalFieldInfo(fieldInfoList);
            dev->SetTableInfo(tableInfo);
        }
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    void PrepareVirtualDeviceBEnv(const std::string &tableName)
    {
        std::vector<RelationalVirtualDevice *> remoteDev;
        remoteDev.push_back(g_deviceB);
        PrepareVirtualDeviceEnv(tableName, g_storePath1, remoteDev);
    }

    void PrepareData(const std::string &tableName, const std::string &dbPath)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, dbPath), SQLITE_OK);
        EXPECT_EQ(InsertValue(db, tableName), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    void OpenStore1(bool syncDualTupleMode = true)
    {
        if (g_observer == nullptr) {
            g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        }
        RelationalStoreDelegate::Option option = {g_observer};
        option.syncDualTupleMode = syncDualTupleMode;
        g_mgr1.OpenStore(g_storePath1, STORE_ID_1, option, g_rdbDelegatePtr1);
        ASSERT_TRUE(g_rdbDelegatePtr1 != nullptr);
    }

    void OpenStore2(bool syncDualTupleMode = true)
    {
        if (g_observer == nullptr) {
            g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        }
        RelationalStoreDelegate::Option option = {g_observer};
        option.syncDualTupleMode = syncDualTupleMode;
        g_mgr2.OpenStore(g_storePath2, STORE_ID_2, option, g_rdbDelegatePtr2);
        ASSERT_TRUE(g_rdbDelegatePtr2 != nullptr);
    }

    void CloseStore()
    {
        if (g_rdbDelegatePtr1 != nullptr) {
            ASSERT_EQ(g_mgr1.CloseStore(g_rdbDelegatePtr1), OK);
            g_rdbDelegatePtr1 = nullptr;
            LOGD("delete rdb store");
        }
        if (g_rdbDelegatePtr2 != nullptr) {
            ASSERT_EQ(g_mgr2.CloseStore(g_rdbDelegatePtr2), OK);
            g_rdbDelegatePtr2 = nullptr;
            LOGD("delete rdb store");
        }
    }

    void PrepareEnvironment(const std::string &tableName, const std::string &dbPath,
        RelationalStoreDelegate* rdbDelegate)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, dbPath), SQLITE_OK);
        EXPECT_EQ(DropTable(db, tableName), SQLITE_OK);
        EXPECT_EQ(CreateTable(db, tableName), SQLITE_OK);
        if (rdbDelegate != nullptr) {
            EXPECT_EQ(rdbDelegate->CreateDistributedTable(tableName), OK);
        }
        sqlite3_close(db);
    }

    void CheckSyncTest(DBStatus status1, DBStatus status2, std::vector<std::string> &devices)
    {
        std::map<std::string, std::vector<TableStatus>> statusMap;
        SyncStatusCallback callBack = [&statusMap](
            const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            statusMap = devicesMap;
        };
        Query query = Query::Select(g_tableName);
        DBStatus status = g_rdbDelegatePtr1->Sync(devices, SYNC_MODE_PUSH_ONLY, query, callBack, true);
        LOGE("expect status is: %d, actual is %d", status1, status);
        ASSERT_TRUE(status == status1);
        if (status == OK) {
            for (const auto &pair : statusMap) {
                for (const auto &tableStatus : pair.second) {
                    LOGD("dev %s, status %d", pair.first.c_str(), tableStatus.status);
                    EXPECT_TRUE(tableStatus.status == OK);
                }
            }
        }
        statusMap.clear();

        std::map<std::string, std::vector<TableStatus>> statusMap2;
        SyncStatusCallback callBack2 = [&statusMap2](
            const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            LOGE("call back devicesMap.size = %d", devicesMap.size());
            statusMap2 = devicesMap;
        };
        status = g_rdbDelegatePtr2->Sync(devices, SYNC_MODE_PUSH_ONLY, query, callBack2, true);
        LOGE("expect status2 is: %d, actual is %d, statusMap2.size = %d", status2, status, statusMap2.size());
        ASSERT_TRUE(status == status2);
        if (status == OK) {
            for (const auto &pair : statusMap2) {
                for (const auto &tableStatus : pair.second) {
                    LOGE("*********** rdb dev %s, status %d", pair.first.c_str(), tableStatus.status);
                    EXPECT_TRUE(tableStatus.status == OK);
                }
            }
        }
        statusMap.clear();
    }

    int g_currentStatus = 0;
    const AutoLaunchNotifier g_notifier = [](const std::string &userId,
        const std::string &appId, const std::string &storeId, AutoLaunchStatus status) {
            LOGD("notifier status = %d", status);
            g_currentStatus = static_cast<int>(status);
        };

    const AutoLaunchRequestCallback g_callback = [](const std::string &identifier, AutoLaunchParam &param) {
        if (g_identifier != identifier) {
            LOGD("g_identifier(%s) != identifier(%s)", g_identifier.c_str(), identifier.c_str());
            return false;
        }
        param.path    = g_testDir + "/test2.db";
        param.appId   = APP_ID;
        param.storeId = STORE_ID;
        CipherPassword passwd;
        param.option = {true, false, CipherType::DEFAULT, passwd, "", false, g_testDir, nullptr,
            0, nullptr};
        param.notifier = g_notifier;
        param.option.syncDualTupleMode = true;
        return true;
    };

    const AutoLaunchRequestCallback g_callback2 = [](const std::string &identifier, AutoLaunchParam &param) {
        if (g_identifier != identifier) {
            LOGD("g_identifier(%s) != identifier(%s)", g_identifier.c_str(), identifier.c_str());
            return false;
        }
        param.subUser = SUB_USER_1;
        param.path    = g_testDir + "/test2.db";
        param.appId   = APP_ID;
        param.storeId = STORE_ID;
        CipherPassword passwd;
        param.option = {true, false, CipherType::DEFAULT, passwd, "", false, g_testDir, nullptr,
                        0, nullptr};
        param.notifier = g_notifier;
        param.option.syncDualTupleMode = false;
        return true;
    };

    void DoRemoteQuery()
    {
        RemoteCondition condition;
        condition.sql = "SELECT * FROM " + g_tableName;
        std::shared_ptr<ResultSet> result = std::make_shared<RelationalResultSetImpl>();
        EXPECT_EQ(g_rdbDelegatePtr1->RemoteQuery(g_deviceB->GetDeviceId(), condition,
            DBConstant::MAX_TIMEOUT, result), USER_CHANGED);
        EXPECT_EQ(result, nullptr);
        g_communicatorAggregator->RegOnDispatch(nullptr);
        CloseStore();
    }

    void CheckSyncResult(bool wait, bool isRemoteQuery)
    {
        std::mutex syncLock_{};
        std::condition_variable syncCondVar_{};
        std::map<std::string, std::vector<TableStatus>> statusMap;
        SyncStatusCallback callBack = [&statusMap, &syncLock_, &syncCondVar_, wait](
            const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            statusMap = devicesMap;
            if (!wait) {
                std::unique_lock<std::mutex> innerlock(syncLock_);
                syncCondVar_.notify_one();
            }
        };
        Query query = Query::Select(g_tableName);
        std::vector<std::string> devices;
        devices.push_back(g_deviceB->GetDeviceId());
        std::vector<RelationalVirtualDevice *> remoteDev;
        remoteDev.push_back(g_deviceB);
        PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);

        DBStatus status = DB_ERROR;
        if (isRemoteQuery) {
            DoRemoteQuery();
            return;
        }

        status = g_rdbDelegatePtr1->Sync(devices, SYNC_MODE_PUSH_ONLY, query, callBack, wait);
        EXPECT_EQ(status, OK);

        if (!wait) {
            std::unique_lock<std::mutex> lock(syncLock_);
            syncCondVar_.wait(lock, [status, &statusMap]() {
                if (status != OK) {
                    return true;
                }
                return !statusMap.empty();
            });
        }

        g_communicatorAggregator->RegOnDispatch(nullptr);
        EXPECT_EQ(statusMap.size(), devices.size());
        for (const auto &pair : statusMap) {
            for (const auto &tableStatus : pair.second) {
                EXPECT_TRUE(tableStatus.status == USER_CHANGED);
            }
        }
        CloseStore();
    }

    void TestSyncWithUserChange(bool wait, bool isRemoteQuery)
    {
        /**
         * @tc.steps: step1. set SyncActivationCheckCallback and only userId1 can active
         */
        RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);
        /**
         * @tc.steps: step2. openstore1 in dual tuple sync mode and openstore2 in normal sync mode
         * @tc.expected: step2. only user2 sync mode is active
         */
        OpenStore1(true);
        OpenStore2(true);

        /**
         * @tc.steps: step3. prepare environment
         */
        PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
        PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);

        /**
         * @tc.steps: step4. prepare data
         */
        PrepareData(g_tableName, g_storePath1);
        PrepareData(g_tableName, g_storePath2);

        /**
         * @tc.steps: step5. set SyncActivationCheckCallback and only userId2 can active
         */
        RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

        /**
         * @tc.steps: step6. call NotifyUserChanged and block sync db concurrently
         * @tc.expected: step6. return OK
         */
        CipherPassword passwd;
        bool startSync = false;
        std::condition_variable cv;
        thread subThread([&]() {
            std::mutex notifyLock;
            std::unique_lock<std::mutex> lck(notifyLock);
            cv.wait(lck, [&startSync]() { return startSync; });
            EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
        });
        subThread.detach();
        g_communicatorAggregator->RegOnDispatch([&](const std::string&, Message *inMsg) {
            if (!startSync) {
                startSync = true;
                cv.notify_all();
                std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME)); // wait for notify user change
            }
        });

        /**
         * @tc.steps: step7. deviceA call sync and wait
         * @tc.expected: step7. sync should return OK.
         */
        CheckSyncResult(wait, isRemoteQuery);
    }

    int PrepareSelect(sqlite3 *db, sqlite3_stmt *&statement, const std::string &table)
    {
        std::string dis_tableName = RelationalStoreManager::GetDistributedTableName(DEVICE_B, table);
        const std::string sql = "SELECT * FROM " + dis_tableName;
        LOGD("exec sql: %s", sql.c_str());
        return sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    }

    void ClearAllDevicesData()
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, g_storePath2), SQLITE_OK);
        std::string dis_tableName = RelationalStoreManager::GetDistributedTableName(DEVICE_B, g_tableName);
        std::string sql = "DELETE FROM " + dis_tableName;
        EXPECT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    void CheckDataInRealDevice()
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, g_storePath2), SQLITE_OK);

        sqlite3_stmt *statement = nullptr;
        EXPECT_EQ(PrepareSelect(db, statement, g_tableName), SQLITE_OK);
        int rowCount = 0;
        while (true) {
            int rc = sqlite3_step(statement);
            if (rc != SQLITE_ROW) {
                LOGD("GetSyncData Exist by code[%d]", rc);
                break;
            }
            int columnCount = sqlite3_column_count(statement);
            EXPECT_EQ(columnCount, 2); // 2: result index
            rowCount++;
        }
        EXPECT_EQ(rowCount, 1);
        sqlite3_finalize(statement);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    void RegOnDispatchWithoutDataPacket(std::atomic<int> &messageCount, bool checkVirtual = true)
    {
        g_communicatorAggregator->RegOnDispatch([&messageCount, checkVirtual](const std::string &dev, Message *msg) {
            if (msg->GetMessageId() != TIME_SYNC_MESSAGE && msg->GetMessageId() != ABILITY_SYNC_MESSAGE) {
                return;
            }
            if (((checkVirtual && dev != DEVICE_B) || (!checkVirtual && dev != "real_device")) ||
                msg->GetMessageType() != TYPE_REQUEST) {
                return;
            }
            messageCount++;
        });
    }

    void InsertBatchValue(const std::string &tableName, const std::string &dbPath, int totalNum)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, dbPath), SQLITE_OK);
        for (int i = 0; i < totalNum; i++) {
            std::string sql = "insert into " + tableName
                + " values(" + std::to_string(i) + ", 'aaa');";
            EXPECT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
        }
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }
}

class DistributedDBRelationalMultiUserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalMultiUserTest::SetUpTestCase(void)
{
    /**
    * @tc.setup: Init datadir and Virtual Communicator.
    */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath1 = g_testDir + "/test1.db";
    g_storePath2 = g_testDir + "/test2.db";
    sqlite3 *db1 = nullptr;
    ASSERT_EQ(GetDB(db1, g_storePath1), SQLITE_OK);
    sqlite3_close(db1);

    sqlite3 *db2 = nullptr;
    ASSERT_EQ(GetDB(db2, g_storePath2), SQLITE_OK);
    sqlite3_close(db2);

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBRelationalMultiUserTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBRelationalMultiUserTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B
     */
    g_deviceB = new (std::nothrow) RelationalVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualRelationalVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);
    g_deviceC = new (std::nothrow) RelationalVirtualDevice(DEVICE_C);
    ASSERT_TRUE(g_deviceC != nullptr);
    VirtualRelationalVerSyncDBInterface *syncInterfaceC = new (std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceC != nullptr);
    ASSERT_EQ(g_deviceC->Initialize(g_communicatorAggregator, syncInterfaceC), E_OK);
}

void DistributedDBRelationalMultiUserTest::TearDown(void)
{
    /**
     * @tc.teardown: Release device A, B, C
     */
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    if (g_deviceC != nullptr) {
        delete g_deviceC;
        g_deviceC = nullptr;
    }
    SyncActivationCheckCallback callback = nullptr;
    RuntimeConfig::SetSyncActivationCheckCallback(callback);
    RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
}

/**
 * @tc.name: multi user 001
 * @tc.desc: Test multi user change for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step2. openstore1 and openstore2
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1();
    OpenStore2();

    /**
     * @tc.steps: step3. prepare environment
     */
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);

    /**
     * @tc.steps: step4. prepare data
     */
    PrepareData(g_tableName, g_storePath1);
    PrepareData(g_tableName, g_storePath2);

    /**
     * @tc.steps: step5. g_rdbDelegatePtr1 and g_rdbDelegatePtr2 call sync
     * @tc.expected: step5. g_rdbDelegatePtr2 call success
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    remoteDev.push_back(g_deviceC);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);
    CheckSyncTest(NOT_ACTIVE, OK, devices);

    /**
     * @tc.expected: step6. onComplete should be called, DeviceB have {k1,v1}
     */
    std::vector<VirtualRowData> targetData;
    g_deviceB->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);

    /**
     * @tc.expected: step7. user change
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);
    RuntimeConfig::NotifyUserChanged();
    /**
     * @tc.steps: step8. g_kvDelegatePtr1 and g_kvDelegatePtr2 call sync
     * @tc.expected: step8. g_kvDelegatePtr1 call success
     */
    devices.clear();
    devices.push_back(g_deviceC->GetDeviceId());
    CheckSyncTest(OK, NOT_ACTIVE, devices);
    /**
     * @tc.expected: step9. onComplete should be called, DeviceC have {k1,v1}
     */
    targetData.clear();
    g_deviceC->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);
    CloseStore();
}

/**
 * @tc.name: multi user 002
 * @tc.desc: Test multi user not change for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);
    /**
     * @tc.steps: step2. openstore1 and openstore2
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1();
    OpenStore2();

    /**
     * @tc.steps: step3. prepare environment
     */
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);

    /**
     * @tc.steps: step4. prepare data
     */
    PrepareData(g_tableName, g_storePath1);
    PrepareData(g_tableName, g_storePath2);

    /**
     * @tc.steps: step4. GetRelationalStoreIdentifier success when userId is invalid
     */
    std::string userId;
    EXPECT_TRUE(g_mgr1.GetRelationalStoreIdentifier(userId, APP_ID, USER_ID_2, true) != "");
    userId.resize(130);
    EXPECT_TRUE(g_mgr1.GetRelationalStoreIdentifier(userId, APP_ID, USER_ID_2, true) != "");

    /**
     * @tc.steps: step5. g_rdbDelegatePtr1 and g_rdbDelegatePtr2 call sync
     * @tc.expected: step5. g_rdbDelegatePtr2 call success
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    remoteDev.push_back(g_deviceC);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);
    CheckSyncTest(NOT_ACTIVE, OK, devices);

    /**
     * @tc.expected: step6. onComplete should be called, DeviceB have {k1,v1}
     */
    std::vector<VirtualRowData> targetData;
    g_deviceB->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);

    /**
     * @tc.expected: step7. user not change
     */
    RuntimeConfig::NotifyUserChanged();

    /**
     * @tc.steps: step8. g_kvDelegatePtr1 and g_kvDelegatePtr2 call sync
     * @tc.expected: step8. g_kvDelegatePtr1 call success
     */
    CheckSyncTest(NOT_ACTIVE, OK, devices);

    /**
     * @tc.expected: step9. onComplete should be called, DeviceC have {k1,v1}
     */
    targetData.clear();
    g_deviceB->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);
    CloseStore();
}

/**
 * @tc.name: multi user 003
 * @tc.desc: enhancement callback return true in multiuser mode for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser003, TestSize.Level3)
{
    /**
     * @tc.steps: step1. prepare environment
     */
    OpenStore1();
    OpenStore2();
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);
    CloseStore();

    /**
     * @tc.steps: step2. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step3. SetAutoLaunchRequestCallback
     * @tc.expected: step3. success.
     */
    g_mgr1.SetAutoLaunchRequestCallback(g_callback);

    /**
     * @tc.steps: step4. RunCommunicatorLackCallback
     * @tc.expected: step4. success.
     */
    g_identifier = g_mgr1.GetRelationalStoreIdentifier(USER_ID_2, APP_ID, STORE_ID, true);
    EXPECT_TRUE(g_identifier == g_mgr1.GetRelationalStoreIdentifier(USER_ID_1, APP_ID, STORE_ID, true));
    std::vector<uint8_t> label(g_identifier.begin(), g_identifier.end());
    g_communicatorAggregator->SetCurrentUserId(USER_ID_2);
    g_communicatorAggregator->RunCommunicatorLackCallback(label);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

    /**
     * @tc.steps: step5. device B put one data
     * @tc.expected: step5. success.
     */
    VirtualRowData virtualRowData;
    DataValue d1;
    d1 = (int64_t)1;
    virtualRowData.objectData.PutDataValue("id", d1);
    DataValue d2;
    d2.SetText("hello");
    virtualRowData.objectData.PutDataValue("name", d2);
    virtualRowData.logInfo.timestamp = 1;
    g_deviceB->PutData(g_tableName, {virtualRowData});

    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);

    /**
     * @tc.steps: step6. device B push sync to A
     * @tc.expected: step6. success.
     */
    Query query = Query::Select(g_tableName);
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);

    /**
     * @tc.steps: step7. deviceA have {k1,v1}
     * @tc.expected: step7. success.
     */
    CheckDataInRealDevice();

    RuntimeConfig::SetAutoLaunchRequestCallback(nullptr, DBType::DB_RELATION);
    RuntimeConfig::ReleaseAutoLaunch(USER_ID_2, APP_ID, STORE_ID, DBType::DB_RELATION);
    RuntimeConfig::ReleaseAutoLaunch(USER_ID_2, APP_ID, STORE_ID, DBType::DB_RELATION);
    g_currentStatus = 0;
    ClearAllDevicesData();
    CloseStore();
}

/**
 * @tc.name: multi user 004
 * @tc.desc: test NotifyUserChanged func when all db in normal sync mode for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. openstore1 and openstore2 in normal sync mode
     * @tc.expected: step1. only user2 sync mode is active
     */
    OpenStore1(false);
    OpenStore2(false);

    /**
     * @tc.steps: step2. call NotifyUserChanged
     * @tc.expected: step2. return OK
     */
    EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
    CloseStore();

     /**
     * @tc.steps: step3. openstore1 open normal sync mode and and openstore2 in dual tuple
     * @tc.expected: step3. only user2 sync mode is active
     */
    OpenStore1(false);
    OpenStore2();

    /**
     * @tc.steps: step4. call NotifyUserChanged
     * @tc.expected: step4. return OK
     */
    EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
    CloseStore();
}

/**
 * @tc.name: multi user 005
 * @tc.desc: test NotifyUserChanged and close db concurrently for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId1 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);

    /**
     * @tc.steps: step2. openstore1 in dual tuple sync mode and openstore2 in normal sync mode
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1(true);
    OpenStore2(false);

    /**
     * @tc.steps: step3. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step4. call NotifyUserChanged and close db concurrently
     * @tc.expected: step4. return OK
     */
    thread subThread([&]() {
        EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
    });
    subThread.detach();
    EXPECT_EQ(g_mgr1.CloseStore(g_rdbDelegatePtr1), OK);
    g_rdbDelegatePtr1 = nullptr;
    CloseStore();
}

/**
 * @tc.name: multi user 006
 * @tc.desc: test NotifyUserChanged and block sync concurrently for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser006, TestSize.Level1)
{
    TestSyncWithUserChange(true, false);
}

/**
 * @tc.name: multi user 007
 * @tc.desc: test NotifyUserChanged and non-block sync concurrently for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser007, TestSize.Level1)
{
    TestSyncWithUserChange(false, false);
}

/**
 * @tc.name: multi user 008
 * @tc.desc: test NotifyUserChanged and remote query concurrently for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step2. openstore1 in dual tuple sync mode and openstore2 in normal sync mode
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1(true);
    OpenStore2(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);

    /**
     * @tc.steps: step3. user1 call remote query
     * @tc.expected: step3. sync should return NOT_ACTIVE.
     */
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);
    RemoteCondition condition;
    condition.sql = "SELECT * FROM " + g_tableName;
    std::shared_ptr<ResultSet> result = std::make_shared<RelationalResultSetImpl>();
    DBStatus status = g_rdbDelegatePtr1->RemoteQuery(g_deviceB->GetDeviceId(), condition,
        DBConstant::MAX_TIMEOUT, result);
    EXPECT_EQ(status, NOT_ACTIVE);
    EXPECT_EQ(result, nullptr);
    CloseStore();
}

/**
 * @tc.name: multi user 009
 * @tc.desc: test user change and remote query concurrently for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser009, TestSize.Level1)
{
    TestSyncWithUserChange(false, true);
}

/**
 * @tc.name: multi user 010
 * @tc.desc: test check sync active twice when open store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser010, TestSize.Level1)
{
    uint32_t callCount = 0u;
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and record call count, only first call return not active
     */
    RuntimeConfig::SetSyncActivationCheckCallback([&callCount] (const std::string &userId, const std::string &appId,
        const std::string &storeId) -> bool {
        callCount++;
        return callCount != 1;
    });
    /**
     * @tc.steps: step2. openstore1 in dual tuple sync mode
     * @tc.expected: step2. it should be activated finally
     */
    OpenStore1(true);
    /**
     * @tc.steps: step3. prepare environment
     */
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    /**
     * @tc.steps: step4. call sync to DEVICES_B
     * @tc.expected: step4. should return OK, not NOT_ACTIVE
     */
    Query query = Query::Select(g_tableName);
    SyncStatusCallback callback = nullptr;
    EXPECT_EQ(g_rdbDelegatePtr1->Sync({DEVICE_B}, SYNC_MODE_PUSH_ONLY, query, callback, true), OK);
    CloseStore();
}

/**
 * @tc.name: multi user 011
 * @tc.desc: test use different option to open store for rdb
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser011, TestSize.Level1)
{
    for (int i = 0; i < 2; i++) {
        bool syncDualTupleMode = i / 2;
        OpenStore1(syncDualTupleMode);
        RelationalStoreDelegate::Option option = { g_observer };
        option.syncDualTupleMode = !syncDualTupleMode;
        RelationalStoreDelegate *rdbDeletegatePtr = nullptr;
        EXPECT_EQ(g_mgr1.OpenStore(g_storePath1, STORE_ID_1, option, rdbDeletegatePtr), MODE_MISMATCH);
        EXPECT_EQ(rdbDeletegatePtr, nullptr);
        CloseStore();
    }
}

/**
 * @tc.name: multi user 012
 * @tc.desc: test dont check sync active when open store with normal store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser012, TestSize.Level1)
{
    uint32_t callCount = 0u;
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and record call count
     */
    RuntimeConfig::SetSyncActivationCheckCallback([&callCount] (const std::string &userId, const std::string &appId,
        const std::string &storeId) -> bool {
        callCount++;
        return true;
    });
    /**
     * @tc.steps: step2. openStore in no dual tuple sync mode
     * @tc.expected: step2. it should be activated finally, and callCount should be zero
     */
    OpenStore1(false);
    EXPECT_EQ(callCount, 0u);
    CloseStore();
}

/**
 * @tc.name: multi user 013
 * @tc.desc: test dont check sync active when open store with normal store
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser013, TestSize.Level1)
{
    uint32_t callCount = 0u;
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and record call count
     */
    RuntimeConfig::SetSyncActivationCheckCallback([&callCount] (const std::string &userId, const std::string &appId,
        const std::string &storeId) -> bool {
        callCount++;
        return true;
    });
    /**
     * @tc.steps: step2. openStore in dual tuple sync mode
     * @tc.expected: step2. it should not be activated finally, and callCount should be 2
     */
    OpenStore1(true);
    EXPECT_EQ(callCount, 2u);
    callCount = 0u;
    EXPECT_EQ(g_rdbDelegatePtr1->RemoveDeviceData("DEVICE"), OK);
    EXPECT_EQ(callCount, 0u);
    CloseStore();
}

/**
 * @tc.name: multi user 014
 * @tc.desc: test remote query with multi user
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser014, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);
    /**
     * @tc.steps: step2. openStore in dual tuple sync mode and call remote query
     */
    OpenStore1(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    /**
     * @tc.steps: step3. disable communicator and call remote query
     * @tc.expected: step3. failed by communicator
     */
    g_communicatorAggregator->DisableCommunicator();
    RemoteCondition condition;
    condition.sql = "SELECT * FROM RdbMultiUser014";
    std::shared_ptr<ResultSet> resultSet;
    DBStatus status = g_rdbDelegatePtr1->RemoteQuery("DEVICE", condition, DBConstant::MAX_TIMEOUT, resultSet);
    EXPECT_EQ(status, COMM_FAILURE);
    EXPECT_EQ(resultSet, nullptr);
    CloseStore();
    g_communicatorAggregator->EnableCommunicator();
    SyncActivationCheckCallbackV2 callbackV2 = nullptr;
    RuntimeConfig::SetSyncActivationCheckCallback(callbackV2);
    OS::RemoveFile(g_storePath1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
}

/**
 * @tc.name: RDBSyncOpt001
 * @tc.desc: check time sync and ability sync once
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RDBSyncOpt001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount);
    /**
     * @tc.steps: step2. openStore in dual tuple sync mode and call remote query
     */
    OpenStore1(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareVirtualDeviceBEnv(g_tableName);
    /**
     * @tc.steps: step3. call sync to DEVICES_B
     * @tc.expected: step3. should return OK
     */
    Query query = Query::Select(g_tableName);
    SyncStatusCallback callback = nullptr;
    EXPECT_EQ(g_rdbDelegatePtr1->Sync({DEVICE_B}, SYNC_MODE_PUSH_ONLY, query, callback, true), OK);
    CloseStore();
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step4. re open store and sync again
     * @tc.expected: step4. reopen OK and sync success, no negotiation packet
     */
    OpenStore1(true);
    messageCount = 0;
    EXPECT_EQ(g_rdbDelegatePtr1->Sync({DEVICE_B}, SYNC_MODE_PUSH_ONLY, query, callback, true), OK);
    EXPECT_EQ(messageCount, 0);
    CloseStore();
    OS::RemoveFile(g_storePath1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: RDBSyncOpt002
 * @tc.desc: check re ability sync after create distributed table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RDBSyncOpt002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount);
    /**
     * @tc.steps: step2. openStore in dual tuple sync mode and call remote query
     */
    OpenStore1(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareVirtualDeviceBEnv(g_tableName);
    /**
     * @tc.steps: step3. call sync to DEVICES_B
     * @tc.expected: step3. should return OK
     */
    Query query = Query::Select(g_tableName);
    SyncStatusCallback callback = nullptr;
    EXPECT_EQ(g_rdbDelegatePtr1->Sync({DEVICE_B}, SYNC_MODE_PUSH_ONLY, query, callback, true), OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step4. create distributed table and sync again
     * @tc.expected: step4. reopen OK and sync success, only ability packet
     */
    PrepareEnvironment("table2", g_storePath1, g_rdbDelegatePtr1);
    messageCount = 0;
    EXPECT_EQ(g_rdbDelegatePtr1->Sync({DEVICE_B}, SYNC_MODE_PUSH_ONLY, query, callback, true), OK);
    EXPECT_EQ(messageCount, 1);
    CloseStore();
    OS::RemoveFile(g_storePath1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: RDBSyncOpt003
 * @tc.desc: check re ability sync after create distributed table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RDBSyncOpt003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount, false);
    /**
     * @tc.steps: step2. openStore in dual tuple sync mode and call remote query
     */
    OpenStore1(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareVirtualDeviceBEnv(g_tableName);
    /**
     * @tc.steps: step3. call sync to DEVICES_B
     * @tc.expected: step3. should return OK
     */
    Query query = Query::Select(g_tableName);
    SyncStatusCallback callback = nullptr;
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step4. create distributed table and sync again
     * @tc.expected: step4. reopen OK and sync success, only ability packet
     */
    PrepareEnvironment("table2", g_storePath1, g_rdbDelegatePtr1);
    messageCount = 0;
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    EXPECT_EQ(messageCount, 1);
    CloseStore();
    OS::RemoveFile(g_storePath1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: RDBSyncOpt004
 * @tc.desc: check ability sync once after reopen
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RDBSyncOpt004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount, false);
    /**
     * @tc.steps: step2. openStore in dual tuple sync mode and call remote query
     */
    OpenStore1(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareVirtualDeviceBEnv(g_tableName);
    /**
     * @tc.steps: step3. call sync to DEVICES_B
     * @tc.expected: step3. should return OK
     */
    Query query = Query::Select(g_tableName);
    g_deviceB->GenericVirtualDevice::Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, query, true);
    CloseStore();
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step4. re open store and sync again
     * @tc.expected: step4. reopen OK and sync success, no negotiation packet
     */
    OpenStore1(true);
    messageCount = 0;
    g_deviceB->GenericVirtualDevice::Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, query, true);
    EXPECT_EQ(messageCount, 0);
    CloseStore();
    OS::RemoveFile(g_storePath1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: RDBSyncOpt005
 * @tc.desc: check re time sync after time change
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RDBSyncOpt005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount, false);
    /**
     * @tc.steps: step2. openStore in dual tuple sync mode and call remote query
     */
    OpenStore1(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareVirtualDeviceBEnv(g_tableName);
    /**
     * @tc.steps: step3. call sync to DEVICES_B
     * @tc.expected: step3. should return OK
     */
    Query query = Query::Select(g_tableName);
    SyncStatusCallback callback = nullptr;
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step4. notify time change and sync again
     * @tc.expected: step4. sync success, only time sync packet
     */
    RuntimeContext::GetInstance()->NotifyTimestampChanged(100);
    RuntimeContext::GetInstance()->RecordAllTimeChange();
    RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
    messageCount = 0;
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    EXPECT_EQ(messageCount, 1);
    CloseStore();
    OS::RemoveFile(g_storePath1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: RDBSyncOpt006
 * @tc.desc: check sync when time change.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RDBSyncOpt006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. openStore and insert data
     * @tc.expected: step1. return ok
     */
    OpenStore1(true);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareVirtualDeviceBEnv(g_tableName);
    InsertBatchValue(g_tableName, g_storePath1, 10000);

    /**
     * @tc.steps: step2. device B set delay send time
     * * @tc.expected: step2. return ok
    */
    std::set<std::string> delayDevice = {DEVICE_B};
    g_communicatorAggregator->SetSendDelayInfo(3000u, TIME_SYNC_MESSAGE, 1u, 0u, delayDevice); // send delay 3000ms

    /**
     * @tc.steps: step3. notify time change when sync
     * @tc.expected: step3. sync success
     */
    Query query = Query::Select(g_tableName);
    SyncStatusCallback callback = nullptr;
    thread thd1 = thread([&]() {
        EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PULL_ONLY, query, true), E_OK);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    thread thd2 = thread([]() {
        RuntimeContext::GetInstance()->NotifyTimestampChanged(100);
        RuntimeContext::GetInstance()->RecordAllTimeChange();
        RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
    });
    thd1.join();
    thd2.join();

    CloseStore();
    OS::RemoveFile(g_storePath1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    g_communicatorAggregator->ResetSendDelayInfo();
}

/**
 * @tc.name: SubUserAutoLaunchTest001
 * @tc.desc: Test auto launch with sub user
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, SubUserAutoLaunchTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Prepare db1 and db2 with subUser
     * @tc.expected: step1. success.
     */
    RelationalStoreManager subUserMgr2(APP_ID, USER_ID_1, SUB_USER_1);
    RelationalStoreDelegate::Option option;
    subUserMgr2.OpenStore(g_storePath2, STORE_ID, option, g_rdbDelegatePtr2);
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);
    subUserMgr2.SetAutoLaunchRequestCallback(g_callback2);
    RelationalStoreManager subUserMgr1(APP_ID, USER_ID_1, SUB_USER_1);
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    CloseStore();

    /**
     * @tc.steps: step2. Prepare data in deviceB
     * @tc.expected: step2. success.
     */
    VirtualRowData virtualRowData;
    DataValue d1;
    d1 = (int64_t)1;
    virtualRowData.objectData.PutDataValue("id", d1);
    DataValue d2;
    d2.SetText("hello");
    virtualRowData.objectData.PutDataValue("name", d2);
    virtualRowData.logInfo.timestamp = 1;
    g_deviceB->PutData(g_tableName, {virtualRowData});

    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);

    /**
     * @tc.steps: step3. Set label in deviceB and sync
     * @tc.expected: step3. success.
     */
    Query query = Query::Select(g_tableName);
    g_identifier = subUserMgr1.GetRelationalStoreIdentifier(USER_ID_1, SUB_USER_1, APP_ID, STORE_ID);
    std::vector<uint8_t> label(g_identifier.begin(), g_identifier.end());
    g_communicatorAggregator->SetCurrentUserId(USER_ID_1);
    g_communicatorAggregator->RunCommunicatorLackCallback(label);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);

    /**
     * @tc.steps: step4. Check result
     * @tc.expected: step4. deviceA have data from deviceB.
     */
    CheckDataInRealDevice();

    RuntimeConfig::SetAutoLaunchRequestCallback(nullptr, DBType::DB_RELATION);
    RuntimeConfig::ReleaseAutoLaunch(USER_ID_1, SUB_USER_1, APP_ID, STORE_ID, DBType::DB_RELATION);
    ClearAllDevicesData();
}

/**
 * @tc.name: DropDistributedTableTest001
 * @tc.desc: Test sync after drop distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, DropDistributedTableTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Prepare db1 and db2.
     * @tc.expected: step1. success.
     */
    OpenStore1();
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    CloseStore();
    OpenStore2();
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);
    /**
     * @tc.steps: step2. Do 1st sync to create distributed table
     * @tc.expected: step2. success.
     */
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);
    Query query = Query::Select(g_tableName);
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    /**
     * @tc.steps: step3. Drop distributed table
     * @tc.expected: step3. success.
     */
    std::string distributedTableName = DBCommon::GetDistributedTableName(DEVICE_B, g_tableName);
    sqlite3 *db = nullptr;
    EXPECT_EQ(GetDB(db, g_storePath2), SQLITE_OK);
    EXPECT_EQ(DropTable(db, distributedTableName), SQLITE_OK);
    sqlite3_close(db);
    /**
     * @tc.steps: step4. Do 2nd sync and check result.
     * @tc.expected: step4. success.
     */
    VirtualRowData virtualRowData;
    DataValue d1;
    d1 = (int64_t)1;
    virtualRowData.objectData.PutDataValue("id", d1);
    DataValue d2;
    d2.SetText("hello");
    virtualRowData.objectData.PutDataValue("name", d2);
    virtualRowData.logInfo.timestamp = 1;
    g_deviceB->PutData(g_tableName, {virtualRowData});
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    CheckDataInRealDevice();
    g_currentStatus = 0;
    CloseStore();
}

/**
 * @tc.name: DeleteTest001
 * @tc.desc: Test insert update and delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, DeleteTest001, TestSize.Level1)
{
     /**
     * @tc.steps: step1. Prepare db2.
     * @tc.expected: step1. success.
     */
    OpenStore2();
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);
    /**
     * @tc.steps: step2. Do 1st sync to create distributed table
     * @tc.expected: step2. success.
     */
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath2, remoteDev);
    /**
     * @tc.steps: step3. Do sync and check result.
     * @tc.expected: step3. success.
     */
    VirtualRowData virtualRowData;
    DataValue d1;
    d1 = static_cast<int64_t>(1);
    virtualRowData.objectData.PutDataValue("id", d1);
    DataValue d2;
    d2.SetText("hello");
    virtualRowData.objectData.PutDataValue("name", d2);
    virtualRowData.logInfo.timestamp = 1;
    virtualRowData.logInfo.hashKey = {'1'};
    g_deviceB->PutData(g_tableName, {virtualRowData});
    Query query = Query::Select(g_tableName);
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    /**
     * @tc.steps: step4. Update data and sync again.
     * @tc.expected: step4. success.
     */
    virtualRowData.logInfo.timestamp++;
    g_deviceB->PutData(g_tableName, {virtualRowData});
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    /**
     * @tc.steps: step5. Delete data and sync again.
     * @tc.expected: step5. success.
     */
    virtualRowData.logInfo.timestamp++;
    virtualRowData.logInfo.flag = static_cast<uint64_t>(LogInfoFlag::FLAG_DELETE);
    g_deviceB->PutData(g_tableName, {virtualRowData});
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    CheckDataInRealDevice();
    g_currentStatus = 0;
    CloseStore();
}