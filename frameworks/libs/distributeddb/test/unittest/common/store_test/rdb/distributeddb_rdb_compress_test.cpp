/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifdef USE_DISTRIBUTEDDB_DEVICE
#include "rdb_general_ut.h"
#include "relational_store_delegate_impl.h"
#include "single_ver_relational_syncer.h"
#include "sqlite_relational_store.h"
#include "sqlite_relational_store_connection.h"
#include "sync_able_engine.h"
#include "mock_sync_engine.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBRDBCompressTest : public RDBGeneralUt {
public:
    void SetUp() override;
protected:
    void InitOption(DistributedTableMode tableMode, bool isNeedCompressOnSync, uint8_t compressRate);
    void InitInfo(const StoreInfo &info, const char *dev, DistributedTableMode tableMode,
        bool isNeedCompressOnSync, uint8_t compressRate);
    void SimpleSyncTest(bool store1Compress, bool store2Compress, uint8_t store1Rate, uint8_t store2Rate,
        SyncMode mode);
    void CompressTest(bool store1Compress, bool store2Compress, uint8_t store1Rate,
        uint8_t store2Rate, SyncMode mode);
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table);
    static std::string GetInsertSQL();
    static std::string GetConditionSQL();
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    const StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    const StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
    std::atomic<DistributedTableMode> mode_ = DistributedTableMode::COLLABORATION;
};

void DistributedDBRDBCompressTest::SetUp()
{
    mode_ = DistributedTableMode::COLLABORATION;
    RDBGeneralUt::SetUp();
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
}

void DistributedDBRDBCompressTest::InitOption(DistributedTableMode tableMode, bool isNeedCompressOnSync,
    uint8_t compressRate)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = tableMode;
    option.isNeedCompressOnSync = isNeedCompressOnSync;
    option.compressionRate = compressRate;
    SetOption(option);
}

void DistributedDBRDBCompressTest::InitInfo(const StoreInfo &info, const char *dev, DistributedTableMode tableMode,
    bool isNeedCompressOnSync, uint8_t compressRate)
{
    InitOption(tableMode, isNeedCompressOnSync, compressRate);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, dev), E_OK);
    ASSERT_EQ(SetDistributedTables(info, {DEVICE_SYNC_TABLE}), E_OK);
}

UtDateBaseSchemaInfo DistributedDBRDBCompressTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBRDBCompressTest::GetTableSchema(const std::string &table)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo field;
    field.field.colName = "id";
    field.field.type = TYPE_INDEX<int64_t>;
    field.field.primary = true;
    tableSchema.fieldInfo.push_back(field);
    field.field.colName = "val";
    field.field.type = TYPE_INDEX<std::string>;
    field.field.primary = false;
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

std::string DistributedDBRDBCompressTest::GetInsertSQL()
{
    std::string sql = "INSERT OR REPLACE INTO ";
    sql.append(DEVICE_SYNC_TABLE).append("(id, val) VALUES(1, '");
    for (size_t i = 0; i < 1000; ++i) { // 1000 is data len
        sql.append("v");
    }
    sql.append("')");
    return sql;
}

std::string DistributedDBRDBCompressTest::GetConditionSQL()
{
    std::string condition = " id=1 AND val='";
    for (size_t i = 0; i < 1000; ++i) { // 1000 is data len
        condition.append("v");
    }
    condition.append("' ");
    return condition;
}

void DistributedDBRDBCompressTest::SimpleSyncTest(bool store1Compress, bool store2Compress, uint8_t store1Rate,
    uint8_t store2Rate, SyncMode mode)
{
    LOGI("SimpleSyncTest begin store1Compress %d store2Compress %d store1Rate %" PRIu8 " store2Rate %" PRIu8,
        store1Compress, store2Compress, store1Rate, store2Rate);
    RDBGeneralUt::CloseAllDelegate();
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
    /**
     * @tc.steps: step1. Init store1 and store2.
     * @tc.expected: step1. Ok
     */
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, store1Compress,
        store1Rate));
    ASSERT_NO_FATAL_FAILURE(InitInfo(info2_, DEVICE_B, mode_, store2Compress,
        store2Rate));
    /**
     * @tc.steps: step2. Store1 insert local data.
     * @tc.expected: step2. Ok
     */
    ExecuteSQL(GetInsertSQL(), info1_);
    /**
     * @tc.steps: step3. DEV_A sync to DEV_B.
     * @tc.expected: step3. Ok
     */
    if (mode == SYNC_MODE_PUSH_ONLY) {
        ASSERT_NO_FATAL_FAILURE(BlockPush(info1_, info2_, DEVICE_SYNC_TABLE));
    } else if (mode == SYNC_MODE_PULL_ONLY) {
        ASSERT_NO_FATAL_FAILURE(BlockPull(info2_, info1_, DEVICE_SYNC_TABLE));
    }
    /**
     * @tc.steps: step4. Check store2's data.
     * @tc.expected: step4. Exist 1 row
     */
    if (mode_ == DistributedTableMode::COLLABORATION) {
        EXPECT_EQ(CountTableData(info2_, DEVICE_SYNC_TABLE, GetConditionSQL()), 1);
    } else if (mode_ == DistributedTableMode::SPLIT_BY_DEVICE) {
        EXPECT_EQ(CountTableData(info2_,
            DBCommon::GetDistributedTableName(DEVICE_A, DEVICE_SYNC_TABLE), GetConditionSQL()), 1);
    }
}

void DistributedDBRDBCompressTest::CompressTest(bool store1Compress, bool store2Compress, uint8_t store1Rate,
    uint8_t store2Rate, SyncMode mode)
{
    /**
     * @tc.steps: step1. Store1 sync to store2 with compress.
     * @tc.expected: step1. Sync ok
     */
    auto size1 = GetAllSendMsgSize();
    SimpleSyncTest(store1Compress, store2Compress, store1Rate, store2Rate, mode); // both compress rate is 100
    auto size2 = GetAllSendMsgSize();
    /**
     * @tc.steps: step2. Store1 sync to store2 without compress.
     * @tc.expected: step2. Sync ok and
     */
    auto size3 = GetAllSendMsgSize();
    SimpleSyncTest(store1Compress, store2Compress, store1Rate, store2Rate, mode); // both compress rate is 100
    auto size4 = GetAllSendMsgSize();
    EXPECT_LE(size2 - size1, size4 - size3);
}

/**
 * @tc.name: RDBCompressSync001
 * @tc.desc: Test collaboration table push sync with compress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBCompressSync001, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(CompressTest(true, true, 100, 100, SyncMode::SYNC_MODE_PUSH_ONLY));
}

/**
 * @tc.name: RDBCompressSync002
 * @tc.desc: Test collaboration table pull sync with compress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBCompressSync002, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(CompressTest(true, true, 100, 100, SyncMode::SYNC_MODE_PULL_ONLY));
}

/**
 * @tc.name: RDBCompressSync003
 * @tc.desc: Test split table push sync with compress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBCompressSync003, TestSize.Level1)
{
    mode_ = DistributedTableMode::SPLIT_BY_DEVICE;
    ASSERT_NO_FATAL_FAILURE(CompressTest(true, true, 100, 100, SyncMode::SYNC_MODE_PUSH_ONLY));
}

/**
 * @tc.name: RDBCompressSync004
 * @tc.desc: Test split table pull sync with compress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBCompressSync004, TestSize.Level1)
{
    mode_ = DistributedTableMode::SPLIT_BY_DEVICE;
    ASSERT_NO_FATAL_FAILURE(CompressTest(true, true, 100, 100, SyncMode::SYNC_MODE_PULL_ONLY));
}

/**
 * @tc.name: RDBCompressSync005
 * @tc.desc: Test collaboration table push sync with compress but remote is not compress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBCompressSync005, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompressTest(true, false, 100, 100, SyncMode::SYNC_MODE_PUSH_ONLY));
}

/**
 * @tc.name: RDBCompressSync006
 * @tc.desc: Test collaboration table pull sync with compress but remote is not compress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBCompressSync006, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompressTest(true, false, 100, 100, SyncMode::SYNC_MODE_PULL_ONLY));
}

/**
 * @tc.name: RDBInvalidCompress001
 * @tc.desc: Test open store with invalid compress option.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBInvalidCompress001, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, true, 0));
    RDBGeneralUt::CloseAllDelegate();
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, true, 101)); // compress rate is 101
    RDBGeneralUt::CloseAllDelegate();
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, false, 0));
    RDBGeneralUt::CloseAllDelegate();
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, false, 101)); // compress rate is 101
}

/**
 * @tc.name: RDBInvalidCompress001
 * @tc.desc: Test open store with invalid compress option.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBInvalidCompress002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Open store1 with compress and rate1.
     * @tc.expected: step1. Open ok
     */
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, true, 1));
    /**
     * @tc.steps: step2. Open store1 with compress and rate2.
     * @tc.expected: step2. Open failed by conflict with cache
     */
    InitOption(mode_, true, 2); // compress rate is 2
    auto [errCode, delegate] = OpenRDBStore(info1_);
    EXPECT_EQ(errCode, INVALID_ARGS);
    EXPECT_EQ(delegate, nullptr);
    /**
     * @tc.steps: step3. Open store1 without compress and rate1.
     * @tc.expected: step3. Open failed by conflict with cache
     */
    InitOption(mode_, false, 1); // compress rate is 1
    std::tie(errCode, delegate) = OpenRDBStore(info1_);
    EXPECT_EQ(errCode, INVALID_ARGS);
    EXPECT_EQ(delegate, nullptr);
    InitOption(mode_, true, 100); // compress rate is 100
    std::tie(errCode, delegate) = OpenRDBStore(info1_);
    EXPECT_EQ(errCode, INVALID_ARGS);
    EXPECT_EQ(delegate, nullptr);
    /**
     * @tc.steps: step4. Open store1 without compress and rate100 after close store1.
     * @tc.expected: step4. Open ok
     */
    RDBGeneralUt::CloseAllDelegate();
    /**
     * @tc.steps: step5. Open store1 without compress and rate100.
     * @tc.expected: step5. Open ok
     */
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, false, 100)); // compress rate is 100
    InitOption(mode_, false, 2); // compress rate is 2
    std::tie(errCode, delegate) = OpenRDBStore(info1_);
    EXPECT_EQ(errCode, OK);
    ASSERT_NE(delegate, nullptr);
    RelationalStoreManager mgr(info1_.appId, info1_.userId);
    mgr.CloseStore(delegate);
}

/**
 * @tc.name: RDBGetDevTaskCount001
 * @tc.desc: Test get dev task count when sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, RDBGetDevTaskCount001, TestSize.Level0)
{
    std::atomic<bool> checkFlag = false;
    RegBeforeDispatch([this, &checkFlag](const std::string &dev, const Message *inMsg) {
        if (dev != DEVICE_B || inMsg->GetMessageId() != QUERY_SYNC_MESSAGE) {
            return;
        }
        if (checkFlag) {
            return;
        }
        checkFlag = true;
        auto store1 = GetDelegate(info1_);
        ASSERT_NE(store1, nullptr);
        EXPECT_GT(store1->GetDeviceSyncTaskCount(), 0);
        auto store2 = GetDelegate(info2_);
        ASSERT_NE(store2, nullptr);
        EXPECT_GT(store2->GetDeviceSyncTaskCount(), 0);
    });
    CompressTest(true, true, 100, 100, SyncMode::SYNC_MODE_PULL_ONLY);
    RegBeforeDispatch(nullptr);
}

/**
 * @tc.name: VerifyDeviceSyncTaskCountAfterRemoteQuery001
 * @tc.desc: Test get device task count when remote query.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, VerifyDeviceSyncTaskCountAfterRemoteQuery001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init store1 and store2.
     * @tc.expected: step1. Ok
     */
    RDBGeneralUt::CloseAllDelegate();
    SetSchemaInfo(info1_, GetDefaultSchema());
    SetSchemaInfo(info2_, GetDefaultSchema());
    ASSERT_NO_FATAL_FAILURE(InitInfo(info1_, DEVICE_A, mode_, true, 100)); // compress rate is 100
    ASSERT_NO_FATAL_FAILURE(InitInfo(info2_, DEVICE_B, mode_, true, 100)); // compress rate is 100
    std::atomic<int> msgCount = 0;
    RegBeforeDispatch([this, &msgCount](const std::string &dev, const Message *inMsg) {
        if (dev != DEVICE_A || inMsg->GetMessageId() != REMOTE_EXECUTE_MESSAGE) {
            return;
        }
        auto store1 = GetDelegate(info1_);
        ASSERT_NE(store1, nullptr);
        EXPECT_GT(store1->GetDeviceSyncTaskCount(), 0);
        msgCount++;
    });
    RemoteQuery(info1_, info2_, "SELECT * FROM DEVICE_SYNC_TABLE", OK);
    RegBeforeDispatch(nullptr);
    EXPECT_GT(msgCount, 0);
}

/**
 * @tc.name: VerifyInvalidGetDeviceSyncTaskCount001
 * @tc.desc: Test abnormal get device task count.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBCompressTest, VerifyInvalidGetDeviceSyncTaskCount001, TestSize.Level0)
{
    RelationalStoreDelegateImpl delegate;
    EXPECT_EQ(delegate.GetDeviceSyncTaskCount(), 0);
    SyncAbleEngine engine(nullptr);
    EXPECT_EQ(engine.GetDeviceSyncTaskCount(), 0);
    SQLiteRelationalStore store;
    EXPECT_EQ(store.GetDeviceSyncTaskCount(), 0);
    SQLiteRelationalStoreConnection connection(nullptr);
    EXPECT_EQ(connection.GetDeviceSyncTaskCount(), 0);
    MockSyncEngine syncEngine;
    EXPECT_EQ(syncEngine.GetRemoteQueryTaskCount(), 0);
    SingleVerRelationalSyncer syncer;
    EXPECT_EQ(syncer.GetTaskCount(), 0);
}
}
#endif