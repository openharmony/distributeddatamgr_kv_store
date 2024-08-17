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
            LOGD("user = %s, status = %d", item.first.c_str(), item.second.process);
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
 * @tc.name: QueryParsingProcessTest001
 * @tc.desc: Test Query parsing process.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: luoguo
 */
HWTEST_F(DistributedDBCloudKvSyncerTest, QueryParsingProcessTest001, TestSize.Level0)
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
}