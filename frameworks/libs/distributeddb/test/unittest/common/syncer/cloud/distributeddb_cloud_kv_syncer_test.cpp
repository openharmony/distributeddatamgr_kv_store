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

#include "cloud/cloud_db_constant.h"
#include "cloud_db_sync_utils_test.h"
#include "db_base64_utils.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_virtual_device.h"
#include "kv_store_nb_delegate.h"
#include "kvdb_manager.h"
#include "platform_specific.h"
#include "process_system_api_adapter_impl.h"
#include "sqlite_cloud_kv_executor_utils.h"
#include "virtual_communicator_aggregator.h"
#include "virtual_cloud_db.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
static std::string HWM_HEAD = "naturalbase_cloud_meta_sync_data_";
string g_testDir;
KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
CloudSyncOption g_CloudSyncoption;
const std::string USER_ID_2 = "user2";
const std::string USER_ID_3 = "user3";
const int64_t WAIT_TIME = 5;
class DistributedDBCloudKvSyncerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
protected:
    DBStatus GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId, KvStoreNbDelegate::Option option,
        bool invalidSchema = false);
    void CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId);
    void BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus, CloudSyncOption option,
        DBStatus expectSyncResult = OK);
    static DataBaseSchema GetDataBaseSchema(bool invalidSchema);
    void PutKvBatchDataAndSyncCloud(CloudSyncOption &syncOption);
    void GetSingleStore();
    void ReleaseSingleStore();
    void BlockCompensatedSync(int &actSyncCnt, int expSyncCnt);
    void CheckUploadAbnormal(OpType opType, int64_t expCnt, bool isCompensated = false);
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb2_ = nullptr;
    KvStoreConfig config_;
    KvStoreNbDelegate* kvDelegatePtrS1_ = nullptr;
    KvStoreNbDelegate* kvDelegatePtrS2_ = nullptr;
    SyncProcess lastProcess_;
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
    KvVirtualDevice *deviceB_ = nullptr;
    SQLiteSingleVerNaturalStore *singleStore_ = nullptr;
    std::mutex comSyncMutex;
    std::condition_variable comSyncCv;
};

void DistributedDBCloudKvSyncerTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    g_CloudSyncoption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    g_CloudSyncoption.users.push_back(USER_ID);
    g_CloudSyncoption.devices.push_back("cloud");

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }
}

void DistributedDBCloudKvSyncerTest::TearDownTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBCloudKvSyncerTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    config_.dataDir = g_testDir;
    /**
     * @tc.setup: create virtual device B and C, and get a KvStoreNbDelegate as deviceA
     */
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    virtualCloudDb2_ = std::make_shared<VirtualCloudDb>();
    g_mgr.SetKvStoreConfig(config_);
    KvStoreNbDelegate::Option option1;
    ASSERT_EQ(GetKvStore(kvDelegatePtrS1_, STORE_ID_1, option1), OK);
    // set aggregator after get store1, only store2 can sync with p2p
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);
    KvStoreNbDelegate::Option option2;
    ASSERT_EQ(GetKvStore(kvDelegatePtrS2_, STORE_ID_2, option2), OK);

    deviceB_ = new (std::nothrow) KvVirtualDevice("DEVICE_B");
    ASSERT_TRUE(deviceB_ != nullptr);
    auto syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(deviceB_->Initialize(communicatorAggregator_, syncInterfaceB), E_OK);
    GetSingleStore();
}

void DistributedDBCloudKvSyncerTest::TearDown()
{
    ReleaseSingleStore();
    CloseKvStore(kvDelegatePtrS1_, STORE_ID_1);
    CloseKvStore(kvDelegatePtrS2_, STORE_ID_2);
    virtualCloudDb_ = nullptr;
    virtualCloudDb2_ = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }

    if (deviceB_ != nullptr) {
        delete deviceB_;
        deviceB_ = nullptr;
    }

    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    communicatorAggregator_ = nullptr;
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

void DistributedDBCloudKvSyncerTest::BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus,
    CloudSyncOption option, DBStatus expectSyncResult)
{
    if (delegate == nullptr) {
        return;
    }
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    SyncProcess last;
    auto callback = [expectDBStatus, &last, &cv, &dataMutex, &finish, &option](const std::map<std::string,
        SyncProcess> &process) {
        size_t notifyCnt = 0;
        for (const auto &item: process) {
            LOGD("user = %s, status = %d, errCode = %d", item.first.c_str(), item.second.process, item.second.errCode);
            if (item.second.process != DistributedDB::FINISHED) {
                continue;
            }
            EXPECT_EQ(item.second.errCode, expectDBStatus);
            {
                std::lock_guard<std::mutex> autoLock(dataMutex);
                notifyCnt++;
                std::set<std::string> userSet(option.users.begin(), option.users.end());
                if (notifyCnt == userSet.size()) {
                    finish = true;
                    last = item.second;
                    cv.notify_one();
                }
            }
        }
    };
    auto actualRet = delegate->Sync(option, callback);
    EXPECT_EQ(actualRet, expectSyncResult);
    if (actualRet == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
    lastProcess_ = last;
}

DataBaseSchema DistributedDBCloudKvSyncerTest::GetDataBaseSchema(bool invalidSchema)
{
    DataBaseSchema schema;
    TableSchema tableSchema;
    tableSchema.name = invalidSchema ? "invalid_schema_name" : CloudDbConstant::CLOUD_KV_TABLE_NAME;
    Field field;
    field.colName = CloudDbConstant::CLOUD_KV_FIELD_KEY;
    field.type = TYPE_INDEX<std::string>;
    field.primary = true;
    tableSchema.fields.push_back(field);
    field.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE;
    field.primary = false;
    tableSchema.fields.push_back(field);
    field.colName = CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE;
    tableSchema.fields.push_back(field);
    field.colName = CloudDbConstant::CLOUD_KV_FIELD_VALUE;
    tableSchema.fields.push_back(field);
    field.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE_CREATE_TIME;
    field.type = TYPE_INDEX<int64_t>;
    tableSchema.fields.push_back(field);
    schema.tables.push_back(tableSchema);
    return schema;
}

void DistributedDBCloudKvSyncerTest::GetSingleStore()
{
    KvDBProperties prop;
    prop.SetStringProp(KvDBProperties::USER_ID, USER_ID);
    prop.SetStringProp(KvDBProperties::APP_ID, APP_ID);
    prop.SetStringProp(KvDBProperties::STORE_ID, STORE_ID_1);

    std::string hashIdentifier = DBCommon::TransferHashString(
        DBCommon::GenerateIdentifierId(STORE_ID_1, APP_ID, USER_ID, "", 0));
    prop.SetStringProp(DBProperties::IDENTIFIER_DATA, hashIdentifier);
    prop.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
    int errCode = E_OK;
    singleStore_ = static_cast<SQLiteSingleVerNaturalStore *>(KvDBManager::OpenDatabase(prop, errCode));
    ASSERT_NE(singleStore_, nullptr);
}

void DistributedDBCloudKvSyncerTest::ReleaseSingleStore()
{
    RefObject::DecObjRef(singleStore_);
    singleStore_ = nullptr;
}

void DistributedDBCloudKvSyncerTest::BlockCompensatedSync(int &actSyncCnt, int expSyncCnt)
{
    {
        std::unique_lock<std::mutex> lock(comSyncMutex);
        bool result = comSyncCv.wait_for(lock, std::chrono::seconds(WAIT_TIME),
            [&actSyncCnt, expSyncCnt]() { return actSyncCnt == expSyncCnt; });
        ASSERT_EQ(result, true);
    }
}


DBStatus DistributedDBCloudKvSyncerTest::GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId,
    KvStoreNbDelegate::Option option, bool invalidSchema)
{
    DBStatus openRet = OK;
    g_mgr.GetKvStore(storeId, option, [&openRet, &delegate](DBStatus status, KvStoreNbDelegate *openDelegate) {
        openRet = status;
        delegate = openDelegate;
    });
    EXPECT_EQ(openRet, OK);
    EXPECT_NE(delegate, nullptr);

    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
    cloudDbs[USER_ID] = virtualCloudDb_;
    cloudDbs[USER_ID_2] = virtualCloudDb2_;
    delegate->SetCloudDB(cloudDbs);
    std::map<std::string, DataBaseSchema> schemas;
    schemas[USER_ID] = GetDataBaseSchema(invalidSchema);
    schemas[USER_ID_2] = GetDataBaseSchema(invalidSchema);
    return delegate->SetCloudDbSchema(schemas);
}

void DistributedDBCloudKvSyncerTest::CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId)
{
    if (delegate != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(delegate), OK);
        delegate = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(storeId);
        LOGD("delete kv store status %d store %s", status, storeId.c_str());
        ASSERT_EQ(status, OK);
    }
}

void DistributedDBCloudKvSyncerTest::CheckUploadAbnormal(OpType opType, int64_t expCnt, bool isCompensated)
{
    sqlite3 *db_;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    EXPECT_EQ(sqlite3_open_v2(fileUrl.c_str(), &db_, flag, nullptr), SQLITE_OK);

    std::string sql = "SELECT count(*) FROM naturalbase_kv_aux_sync_data_log WHERE ";
    switch (opType) {
        case OpType::INSERT:
            sql += isCompensated ? " cloud_gid != '' AND version !='' AND cloud_flag&0x10=0" :
                " cloud_gid != '' AND version !='' AND cloud_flag=cloud_flag|0x10";
            break;
        case OpType::UPDATE:
            sql += isCompensated ? " cloud_gid != '' AND version !='' AND cloud_flag&0x10=0" :
                " cloud_gid == '' AND version =='' AND cloud_flag=cloud_flag|0x10";
            break;
        case OpType::DELETE:
            sql += " cloud_gid == '' AND version ==''";
            break;
        default:
            break;
    }
    EXPECT_EQ(sqlite3_exec(db_, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(expCnt), nullptr), SQLITE_OK);
    sqlite3_close_v2(db_);
}

/**
 * @tc.name: UploadAbnormalSync001
 * @tc.desc: Test upload update record, cloud returned record not found.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, UploadAbnormalSync001, TestSize.Level0)
{
    auto cloudHook = (ICloudSyncStorageHook *) singleStore_->GetCloudKvStore();
    ASSERT_NE(cloudHook, nullptr);

    /**
     * @tc.steps:step1. Device A inserts data and synchronizes
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);

    /**
     * @tc.steps:step2. Device A update data and synchronizes, cloud returned record not found
     * @tc.expected: step2 OK.
     */
    Value value2 = {'x'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value2), OK);
    int upIdx = 0;
    virtualCloudDb_->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("cloud db upload index:%d", ++upIdx);
        if (upIdx == 1) { // 1 is index
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_NOT_FOUND);
        }
    });
    int syncCnt = 0;
    cloudHook->SetSyncFinishHook([&syncCnt, this] {
        LOGD("sync finish times:%d", ++syncCnt);
        if (syncCnt == 1) { // 1 is the first sync
            CheckUploadAbnormal(OpType::UPDATE, 1L); // 1 is expected count
        } else {
            CheckUploadAbnormal(OpType::UPDATE, 1L, true); // 1 is expected count
        }
        comSyncCv.notify_all();
    });
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockCompensatedSync(syncCnt, 2); // 2 is sync times
    virtualCloudDb_->ForkUpload(nullptr);
    cloudHook->SetSyncFinishHook(nullptr);
}

/**
 * @tc.name: UploadAbnormalSync002
 * @tc.desc: Test upload insert record, cloud returned record already existed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, UploadAbnormalSync002, TestSize.Level0)
{
    auto cloudHook = (ICloudSyncStorageHook *) singleStore_->GetCloudKvStore();
    ASSERT_NE(cloudHook, nullptr);

    /**
     * @tc.steps:step1. Device A inserts k-v and synchronizes
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);

    /**
     * @tc.steps:step2. Device A insert k2-v2 and synchronizes, Device B insert k2-v2 and sync before A upload
     * @tc.expected: step2 OK.
     */
    Key key2 = {'x'};
    Value value2 = {'y'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key2, value2), OK);
    int upIdx = 0;
    virtualCloudDb_->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("cloud db upload index:%d", ++upIdx);
        if (upIdx == 2) { // 2 is index
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_ALREADY_EXISTED);
        }
    });
    int doUpIdx = 0;
    cloudHook->SetDoUploadHook([&doUpIdx, key2, value2, this] {
        LOGD("begin upload index:%d", ++doUpIdx);
        ASSERT_EQ(kvDelegatePtrS2_->Put(key2, value2), OK);
        BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    });
    int syncCnt = 0;
    cloudHook->SetSyncFinishHook([&syncCnt, this] {
        LOGD("sync finish times:%d", ++syncCnt);
        if (syncCnt == 1) { // 1 is the normal sync
            CheckUploadAbnormal(OpType::INSERT, 1L); // 1 is expected count
        } else {
            CheckUploadAbnormal(OpType::INSERT, 2L, true); // 2 is expected count
        }
        comSyncCv.notify_all();
    });
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockCompensatedSync(syncCnt, 2); // 2 is sync times
    virtualCloudDb_->ForkUpload(nullptr);
    cloudHook->SetSyncFinishHook(nullptr);
    cloudHook->SetDoUploadHook(nullptr);
}

/**
 * @tc.name: UploadAbnormalSync003
 * @tc.desc: Test upload delete record, cloud returned record not found.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, UploadAbnormalSync003, TestSize.Level0)
{
    auto cloudHook = (ICloudSyncStorageHook *) singleStore_->GetCloudKvStore();
    ASSERT_NE(cloudHook, nullptr);

    /**
     * @tc.steps:step1. Device A inserts data and synchronizes
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);

    /**
     * @tc.steps:step2. Device A delete data and synchronizes, Device B delete data and sync before A upload
     * @tc.expected: step2 OK.
     */
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key), OK);
    int upIdx = 0;
    virtualCloudDb_->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("cloud db upload index:%d", ++upIdx);
        if (upIdx == 2) { // 2 is index
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_NOT_FOUND);
        }
    });
    int doUpIdx = 0;
    cloudHook->SetDoUploadHook([&doUpIdx, key, this] {
        LOGD("begin upload index:%d", ++doUpIdx);
        ASSERT_EQ(kvDelegatePtrS2_->Delete(key), OK);
        BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    });
    int syncCnt = 0;
    cloudHook->SetSyncFinishHook([&syncCnt, this] {
        LOGD("sync finish times:%d", ++syncCnt);
        if (syncCnt == 1) { // 1 is the normal sync
            CheckUploadAbnormal(OpType::DELETE, 1L); // 1 is expected count
        } else {
            CheckUploadAbnormal(OpType::DELETE, 1L, true); // 1 is expected count
        }
        comSyncCv.notify_all();
    });
    BlockSync(kvDelegatePtrS1_, CLOUD_ERROR, g_CloudSyncoption);
    BlockCompensatedSync(syncCnt, 1); // 1 is sync times
    virtualCloudDb_->ForkUpload(nullptr);
    cloudHook->SetSyncFinishHook(nullptr);
    cloudHook->SetDoUploadHook(nullptr);
}

/**
 * @tc.name: UploadAbnormalSync004
 * @tc.desc: Test sync errCode is not in [27328512, 27394048).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenghuitao
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, UploadAbnormalSync004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device A inserts data and synchronizes
     * @tc.expected: step1 errCode outside DBStatus should be kept.
     */
    int errCode = 27394048; // an error not in [27328512, 27394048)
    virtualCloudDb_->SetActionStatus(static_cast<DBStatus>(errCode));
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, static_cast<DBStatus>(errCode), g_CloudSyncoption);
    virtualCloudDb_->SetActionStatus(OK);
}

/**
 * @tc.name: QueryParsingProcessTest001
 * @tc.desc: Test Query parsing process.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: luoguo
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, QueryParsingProcessTest001, TestSize.Level0)
{
    auto cloudHook = (ICloudSyncStorageHook *)singleStore_->GetCloudKvStore();
    ASSERT_NE(cloudHook, nullptr);

    /**
     * @tc.steps:step1. Device A inserts data and synchronizes
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);

    /**
     * @tc.steps:step2. Test Query parsing Process
     * @tc.expected: step2 OK.
     */
    QuerySyncObject syncObject;
    std::vector<VBucket> syncDataPk;
    VBucket bucket;
    bucket.insert_or_assign(std::string("k"), std::string("k"));
    syncDataPk.push_back(bucket);
    std::string tableName = "sync_data";
    ASSERT_EQ(CloudStorageUtils::GetSyncQueryByPk(tableName, syncDataPk, true, syncObject), E_OK);

    Bytes bytes;
    bytes.resize(syncObject.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel parcel(bytes.data(), bytes.size());
    ASSERT_EQ(syncObject.SerializeData(parcel, SOFTWARE_VERSION_CURRENT), E_OK);

    /**
     * @tc.steps:step3. Check Node's type is QueryNodeType::IN.
     * @tc.expected: step3 OK.
     */
    std::vector<QueryNode> queryNodes;
    syncObject.ParserQueryNodes(bytes, queryNodes);
    ASSERT_EQ(queryNodes[0].type, QueryNodeType::IN);
}

/**
 * @tc.name: UploadFinished001
 * @tc.desc: Test upload update record when do update.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, UploadFinished001, TestSize.Level0)
{
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    Value newValue = {'v', '1'};
    // update [k,v] to [k,v1] when upload
    virtualCloudDb_->ForkUpload([kvDelegatePtrS1 = kvDelegatePtrS1_, key, newValue](const std::string &, VBucket &) {
        EXPECT_EQ(kvDelegatePtrS1->Put(key, newValue), OK);
    });
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    // cloud download [k,v]
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    // sync again and get [k,v1]
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, newValue);
    virtualCloudDb_->ForkUpload(nullptr);
}

/**
 * @tc.name: SyncWithMultipleUsers001.
 * @tc.desc: Test sync data with multiple users.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liufuchenxing
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, SyncWithMultipleUsers001, TestSize.Level0)
{
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    CloudSyncOption syncOption;
    syncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    syncOption.users.push_back(USER_ID);
    syncOption.users.push_back(USER_ID_2);
    syncOption.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, syncOption);
    BlockSync(kvDelegatePtrS2_, OK, syncOption);
    Value actualValue;
    // cloud download [k,v]
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, value);
}

void DistributedDBCloudKvSyncerTest::PutKvBatchDataAndSyncCloud(CloudSyncOption &syncOption)
{
    std::vector<Entry> entries;
    for (int i = 0; i < 200; i++) { // 200 is number of data
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        Entry entry;
        entry.key = key;
        entry.value = value;
        entries.push_back(entry);
    }

    ASSERT_EQ(kvDelegatePtrS1_->PutBatch(entries), OK);
    syncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    syncOption.users.push_back(USER_ID);
    syncOption.users.push_back(USER_ID_2);
    syncOption.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, syncOption);
}

/**
 * @tc.name: SyncWithMultipleUsers002.
 * @tc.desc: test whether upload to the cloud after delete local data that does not have a gid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: luoguo
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, SyncWithMultipleUsers002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. kvDelegatePtrS1_ put 200 data and sync to cloud.
     * @tc.expected: step1. return ok.
     */
    CloudSyncOption syncOption;
    PutKvBatchDataAndSyncCloud(syncOption);

    /**
     * @tc.steps: step2. kvDelegatePtrS2_ only sync user0 from cloud.
     * @tc.expected: step2. return ok.
     */
    syncOption.users.clear();
    syncOption.users.push_back(USER_ID);
    BlockSync(kvDelegatePtrS2_, OK, syncOption);

    /**
     * @tc.steps: step3. kvDelegatePtrS2_ delete 100 data.
     * @tc.expected: step3. return ok.
     */
    std::vector<Key> keys;
    for (int i = 0; i < 100; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        keys.push_back(key);
    }
    ASSERT_EQ(kvDelegatePtrS2_->DeleteBatch(keys), OK);

    /**
     * @tc.steps: step4. kvDelegatePtrS2_ sync to cloud with user0 user2.
     * @tc.expected: step4. return ok.
     */
    syncOption.users.clear();
    syncOption.users.push_back(USER_ID);
    syncOption.users.push_back(USER_ID_2);

    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    uint32_t insertCount = 0;
    auto callback = [&dataMutex, &syncOption, &finish, &insertCount, &cv](const std::map<std::string,
        SyncProcess> &process) {
        size_t notifyCnt = 0;
        for (const auto &item : process) {
            LOGD("user = %s, status = %d, errCode=%d", item.first.c_str(), item.second.process, item.second.errCode);
            if (item.second.process != DistributedDB::FINISHED) {
                continue;
            }
            EXPECT_EQ(item.second.errCode, OK);
            {
                std::lock_guard<std::mutex> autoLock(dataMutex);
                notifyCnt++;
                std::set<std::string> userSet(syncOption.users.begin(), syncOption.users.end());
                if (notifyCnt == userSet.size()) {
                    finish = true;
                    std::map<std::string, TableProcessInfo> tableProcess(item.second.tableProcess);
                    insertCount = tableProcess["sync_data"].downLoadInfo.insertCount;
                    cv.notify_one();
                }
            }
        }
    };
    auto actualRet = kvDelegatePtrS2_->Sync(syncOption, callback);
    EXPECT_EQ(actualRet, OK);
    if (actualRet == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() { return finish; });
    }
    /**
     * @tc.steps: step5. check process info, user2's insertCount should be 0.
     * @tc.expected: step5. return ok.
     */
    EXPECT_EQ(insertCount, 0u);
}

/**
 * @tc.name: AbnormalCloudKvExecutorTest001
 * @tc.desc: Check SqliteCloudKvExecutorUtils interfaces abnormal scene.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, AbnormalCloudKvExecutorTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Call PutCloudData interface with different opType when para is invalid.
     * @tc.expected: step1. return errCode.
     */
    SqliteCloudKvExecutorUtils cloudKvObj;
    DownloadData downloadData;
    downloadData.data = {{}};
    int ret = cloudKvObj.PutCloudData(nullptr, true, downloadData);
    EXPECT_EQ(ret, -E_CLOUD_ERROR);
    downloadData.opType = {OpType::UPDATE_VERSION};
    ret = cloudKvObj.PutCloudData(nullptr, true, downloadData);
    EXPECT_EQ(ret, -E_CLOUD_ERROR);

    downloadData.opType = {OpType::UPDATE_TIMESTAMP};
    ret = cloudKvObj.PutCloudData(nullptr, true, downloadData);
    EXPECT_EQ(ret, -E_INVALID_DB);
    downloadData.opType = {OpType::DELETE};
    ret = cloudKvObj.PutCloudData(nullptr, true, downloadData);
    EXPECT_EQ(ret, -E_INVALID_DB);

    /**
     * @tc.steps: step2. Call CountAllCloudData interface when para is invalid.
     * @tc.expected: step2. return -E_INVALID_ARGS.
     */
    QuerySyncObject querySyncObject;
    std::pair<int, int64_t> res = cloudKvObj.CountAllCloudData({nullptr, true}, {}, "", true, querySyncObject);
    EXPECT_EQ(res.first, -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. Call SqliteCloudKvExecutorUtils interfaces when db is nullptr.
     * @tc.expected: step3. return -E_INVALID_DB.
     */
    res = cloudKvObj.CountCloudData(nullptr, true, 0, "", true);
    EXPECT_EQ(res.first, -E_INVALID_DB);
    std::pair<int, CloudSyncData> ver = cloudKvObj.GetLocalCloudVersion(nullptr, true, "");
    EXPECT_EQ(ver.first, -E_INVALID_DB);
    std::vector<VBucket> dataVector;
    ret = cloudKvObj.GetCloudVersionFromCloud(nullptr, true, "", dataVector);
    EXPECT_EQ(ret, -E_INVALID_DB);
}

/**
 * @tc.name: AbnormalCloudKvExecutorTest002
 * @tc.desc: Check FillCloudLog interface
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, AbnormalCloudKvExecutorTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Call FillCloudLog interface when db and para is nullptr.
     * @tc.expected: step1. return -E_INVALID_ARGS.
     */
    SqliteCloudKvExecutorUtils cloudKvObj;
    sqlite3 *db = nullptr;
    CloudSyncData data;
    CloudUploadRecorder recorder;
    int ret = cloudKvObj.FillCloudLog({db, true}, OpType::INSERT, data, "", recorder);
    EXPECT_EQ(ret, -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. open db and Call FillCloudLog.
     * @tc.expected: step2. return E_OK.
     */
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/test.db";
    EXPECT_EQ(sqlite3_open_v2(fileUrl.c_str(), &db, flag, nullptr), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    data.isCloudVersionRecord = true;
    ret = cloudKvObj.FillCloudLog({db, true}, OpType::INSERT, data, "", recorder);
    EXPECT_EQ(ret, E_OK);
    ret = cloudKvObj.FillCloudLog({db, true}, OpType::DELETE, data, "", recorder);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: DeviceCollaborationTest001
 * @tc.desc: Check force override data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, DeviceCollaborationTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. open db with DEVICE_COLLABORATION.
     * @tc.expected: step1. return E_OK.
     */
    KvStoreNbDelegate* kvDelegatePtrS3 = nullptr;
    KvStoreNbDelegate::Option option;
    option.conflictResolvePolicy = ConflictResolvePolicy::DEVICE_COLLABORATION;
    ASSERT_EQ(GetKvStore(kvDelegatePtrS3, STORE_ID_3, option), OK);
    ASSERT_NE(kvDelegatePtrS3, nullptr);
    KvStoreNbDelegate* kvDelegatePtrS4 = nullptr;
    ASSERT_EQ(GetKvStore(kvDelegatePtrS4, STORE_ID_4, option), OK);
    ASSERT_NE(kvDelegatePtrS4, nullptr);
    /**
     * @tc.steps: step2. db3 put (k1,v1) sync to db4.
     * @tc.expected: step2. db4 get (k1,v1).
     */
    Key key = {'k'};
    Value value = {'v'};
    EXPECT_EQ(kvDelegatePtrS3->Put(key, value), OK);
    communicatorAggregator_->SetLocalDeviceId("DB3");
    BlockSync(kvDelegatePtrS3, OK, g_CloudSyncoption);
    communicatorAggregator_->SetLocalDeviceId("DB4");
    BlockSync(kvDelegatePtrS4, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS4->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    /**
     * @tc.steps: step3. db4 delete (k1,v1) db3 sync again to db4.
     * @tc.expected: step3. db4 get (k1,v1).
     */
    EXPECT_EQ(kvDelegatePtrS4->Delete(key), OK);
    communicatorAggregator_->SetLocalDeviceId("DB3");
    EXPECT_EQ(kvDelegatePtrS3->RemoveDeviceData("", ClearMode::FLAG_AND_DATA), OK);
    BlockSync(kvDelegatePtrS3, OK, g_CloudSyncoption);
    communicatorAggregator_->SetLocalDeviceId("DB4");
    BlockSync(kvDelegatePtrS4, OK, g_CloudSyncoption);
    EXPECT_EQ(kvDelegatePtrS4->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    CloseKvStore(kvDelegatePtrS3, STORE_ID_3);
    CloseKvStore(kvDelegatePtrS4, STORE_ID_4);
}
}