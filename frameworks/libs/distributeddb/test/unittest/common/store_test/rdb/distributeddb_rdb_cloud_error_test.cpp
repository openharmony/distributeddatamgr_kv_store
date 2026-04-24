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

#include "cloud/cloud_db_constant.h"
#include "rdb_general_ut.h"

#ifdef USE_DISTRIBUTEDDB_CLOUD
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBRDBCloudErrorTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    static constexpr const char *CLOUD_TABLE = "cloud_skip_download_table";
    static constexpr int DATA_COUNT = 50;
    static constexpr const int64_t BASE_MODIFY_TIME = 12345678L;
    static constexpr const int64_t BASE_CREATE_TIME = 12345679L;
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    void InitTableAndSchema();
    void InsertCloudRecordBatch(const std::string &tableName, int64_t begin, int64_t count,
        int64_t baseModifyTime = BASE_MODIFY_TIME, int64_t baseCreateTime = BASE_CREATE_TIME);
    CloudSyncOption BuildCloudSyncOption();
    void SyncAndCheckAction(RelationalStoreDelegate *delegate, CloudSyncOption option);
};

void DistributedDBRDBCloudErrorTest::SetUp()
{
    RDBGeneralUt::SetUp();
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1_, "dev1"), E_OK);
    InitTableAndSchema();
}

void DistributedDBRDBCloudErrorTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBCloudErrorTest::InitTableAndSchema()
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + std::string(CLOUD_TABLE) + "("
        "id INTEGER PRIMARY KEY,"
        "name TEXT,"
        "height REAL)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    const std::vector<UtFieldInfo> fieldInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, true}, false},
        {{"name", TYPE_INDEX<std::string>, false, true}, false},
        {{"height", TYPE_INDEX<double>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = CLOUD_TABLE, .fieldInfo = fieldInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info1_, schemaInfo);
    RDBGeneralUt::SetCloudDbConfig(info1_);
    ASSERT_EQ(SetDistributedTables(info1_, {CLOUD_TABLE}, TableSyncType::CLOUD_COOPERATION), E_OK);
}

void DistributedDBRDBCloudErrorTest::InsertCloudRecordBatch(const std::string &tableName, int64_t begin, int64_t count,
    int64_t baseModifyTime, int64_t baseCreateTime)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::vector<VBucket> records;
    std::vector<VBucket> extends;
    for (int64_t i = begin; i < begin + count; ++i) {
        VBucket record;
        record["id"] = i;
        record["name"] = "cloud_insert_" + std::to_string(i);
        record["height"] = static_cast<double>(i);
        records.push_back(std::move(record));

        VBucket extend;
        extend[CloudDbConstant::GID_FIELD] = std::to_string(i);
        extend[CloudDbConstant::CREATE_FIELD] = baseCreateTime;
        extend[CloudDbConstant::MODIFY_FIELD] = baseModifyTime;
        extend[CloudDbConstant::DELETE_FIELD] = false;
        extends.push_back(std::move(extend));
    }
    EXPECT_EQ(cloudDB->BatchInsertWithGid(tableName, std::move(records), extends), DBStatus::OK);
}

/**
  * @tc.name: RdbCloudSkipDownloadErrCode001
  * @tc.desc: Test IsDownloadUpdate returns true, cloud data should be downloaded normally
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xiefengzhu
  */
HWTEST_F(DistributedDBRDBCloudErrorTest, RdbCloudSkipDownloadErrCode001, TestSize.Level1)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    cloudDB->SetHasCloudUpdate(true);
    ASSERT_NO_FATAL_FAILURE(InsertCloudRecordBatch(CLOUD_TABLE, 1, DATA_COUNT));
    ASSERT_NO_FATAL_FAILURE(InsertLocalRecordBatch(info1_, CLOUD_TABLE, DATA_COUNT + 1, DATA_COUNT));

    Query query = Query::Select().FromTable({CLOUD_TABLE});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));

    EXPECT_GT(cloudDB->GetQueryTimes(CLOUD_TABLE), 0u);
    EXPECT_EQ(CountTableData(info1_, CLOUD_TABLE), DATA_COUNT * 2);
}

/**
  * @tc.name: RdbCloudSkipDownloadErrCode002
  * @tc.desc: Test IsDownloadUpdate returns false, cloud data download should be skipped
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xiefengzhu
  */
HWTEST_F(DistributedDBRDBCloudErrorTest, RdbCloudSkipDownloadErrCode002, TestSize.Level1)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    cloudDB->SetHasCloudUpdate(false);
    ASSERT_NO_FATAL_FAILURE(InsertCloudRecordBatch(CLOUD_TABLE, 1, DATA_COUNT));
    ASSERT_NO_FATAL_FAILURE(InsertLocalRecordBatch(info1_, CLOUD_TABLE, DATA_COUNT + 1, DATA_COUNT));

    Query query = Query::Select().FromTable({CLOUD_TABLE});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));

    EXPECT_EQ(cloudDB->GetQueryTimes(CLOUD_TABLE), 0u);
    EXPECT_EQ(CountTableData(info1_, CLOUD_TABLE), DATA_COUNT);
}

CloudSyncOption DistributedDBRDBCloudErrorTest::BuildCloudSyncOption()
{
    CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.syncFlowType = SyncFlowType::NORMAL;
    option.query = Query::Select().FromTable({CLOUD_TABLE});
    option.priorityTask = true;
    option.compensatedSyncOnly = false;
    option.waitTime = DBConstant::MAX_TIMEOUT;
    return option;
}

void DistributedDBRDBCloudErrorTest::SyncAndCheckAction(RelationalStoreDelegate *delegate, CloudSyncOption option)
{
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finished = false;
    CloudErrorAction capturedAction = CloudErrorAction::ACTION_DEFAULT;

    auto callback = [&](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item : process) {
            std::lock_guard<std::mutex> autoLock(dataMutex);
            if (item.second.process != DistributedDB::FINISHED) {
                capturedAction = item.second.cloudErrorInfo.cloudAction;
                EXPECT_EQ(capturedAction, CloudErrorAction::ACTION_RETRY_SYNC_TASK);
            }
            if (item.second.process == DistributedDB::FINISHED) {
                finished = true;
                cv.notify_one();
            }
        }
    };

    EXPECT_EQ(delegate->Sync(option, callback), DBStatus::OK);

    std::unique_lock<std::mutex> uniqueLock(dataMutex);
    cv.wait(uniqueLock, [&finished]() { return finished; });
}

/**
  * @tc.name: RdbCloudErrorActionTest001
  * @tc.desc: do query with error_action=ACTION_RETRY_SYNC_TASK
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xiefengzhu
  */
HWTEST_F(DistributedDBRDBCloudErrorTest, RdbCloudErrorActionTest001, TestSize.Level0)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    ASSERT_NO_FATAL_FAILURE(InsertCloudRecordBatch(CLOUD_TABLE, 1, DATA_COUNT));

    cloudDB->ForkAfterQueryResult([](VBucket &extend, std::vector<VBucket> &data) -> DBStatus {
        for (auto &record : data) {
            record[CloudDbConstant::CLOUD_ERROR_ACTION_FIELD] =
                static_cast<int64_t>(CloudErrorAction::ACTION_RETRY_SYNC_TASK);
        }
        return DBStatus::QUERY_END;
    });

    auto delegate = GetDelegate(info1_);
    ASSERT_NE(delegate, nullptr);
    CloudSyncOption option = DistributedDBRDBCloudErrorTest::BuildCloudSyncOption();
    ASSERT_NO_FATAL_FAILURE(SyncAndCheckAction(delegate, option));
    cloudDB->ForkAfterQueryResult(nullptr);
}

/**
  * @tc.name: RdbCloudErrorActionTest002
  * @tc.desc: do batchInsert or batchUpdate or batchUpdate with error_action=ACTION_RETRY_SYNC_TASK
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xiefengzhu
  */
HWTEST_F(DistributedDBRDBCloudErrorTest, RdbCloudErrorActionTest002, TestSize.Level0)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    ASSERT_NO_FATAL_FAILURE(InsertCloudRecordBatch(CLOUD_TABLE, 1, DATA_COUNT));
    ASSERT_NO_FATAL_FAILURE(InsertLocalRecordBatch(info1_, CLOUD_TABLE, DATA_COUNT + 1, DATA_COUNT));
    cloudDB->ForkAfterQueryResult([](VBucket &extend, std::vector<VBucket> &data) -> DBStatus {
        for (auto &record : data) {
            record[CloudDbConstant::CLOUD_ERROR_ACTION_FIELD] =
                static_cast<int64_t>(CloudErrorAction::ACTION_RETRY_SYNC_TASK);
        }
        return DBStatus::QUERY_END;
    });
    cloudDB->ForkUpload([](const std::string &tableName, VBucket &extend) {
        extend[CloudDbConstant::CLOUD_ERROR_ACTION_FIELD] =
            static_cast<int64_t>(CloudErrorAction::ACTION_RETRY_SYNC_TASK);
    });

    auto delegate = GetDelegate(info1_);
    ASSERT_NE(delegate, nullptr);
    CloudSyncOption option = DistributedDBRDBCloudErrorTest::BuildCloudSyncOption();
    ASSERT_NO_FATAL_FAILURE(SyncAndCheckAction(delegate, option));
    cloudDB->ForkAfterQueryResult(nullptr);
    cloudDB->ForkUpload(nullptr);
}

/**
  * @tc.name: RdbCloudErrorActionTest003
  * @tc.desc: do batchDelete with error_action=ACTION_RETRY_SYNC_TASK
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xiefengzhu
  */
HWTEST_F(DistributedDBRDBCloudErrorTest, RdbCloudErrorActionTest003, TestSize.Level0)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    ASSERT_NO_FATAL_FAILURE(InsertCloudRecordBatch(CLOUD_TABLE, 1, DATA_COUNT));
    Query query = Query::Select().FromTable({CLOUD_TABLE});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    std::string sql = "DELETE FROM cloud_skip_download_table";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);

    ASSERT_NO_FATAL_FAILURE(InsertCloudRecordBatch(CLOUD_TABLE, DATA_COUNT, DATA_COUNT));
    cloudDB->ForkAfterQueryResult([](VBucket &extend, std::vector<VBucket> &data) -> DBStatus {
        for (auto &record : data) {
            record[CloudDbConstant::CLOUD_ERROR_ACTION_FIELD] =
                static_cast<int64_t>(CloudErrorAction::ACTION_RETRY_SYNC_TASK);
        }
        return DBStatus::QUERY_END;
    });
    cloudDB->ForkBeforeBatchUpdate([](const std::string &tableName, std::vector<VBucket> &record,
        std::vector<VBucket> &extend, bool isDelete) {
        if (isDelete) {
            for (auto &ext : extend) {
                ext[CloudDbConstant::CLOUD_ERROR_ACTION_FIELD] =
                    static_cast<int64_t>(CloudErrorAction::ACTION_RETRY_SYNC_TASK);
            }
        }
    });

    auto delegate = GetDelegate(info1_);
    ASSERT_NE(delegate, nullptr);
    CloudSyncOption option = DistributedDBRDBCloudErrorTest::BuildCloudSyncOption();
    ASSERT_NO_FATAL_FAILURE(SyncAndCheckAction(delegate, option));
    cloudDB->ForkAfterQueryResult(nullptr);
    cloudDB->ForkBeforeBatchUpdate(nullptr);
}
}
#endif
