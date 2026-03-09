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

#define LOG_TAG "KvHiviewReporterNew"
#include <gtest/gtest.h>

#include "distributed_kv_data_manager.h"
#include "include/hisysevent_mock.h"
#include "kv_hiview_reporter.h"
#include "log_print.h"
#include "types.h"
#include <unistd.h>
namespace OHOS::Test {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedKv;

static constexpr const char *BASE_DIR = "/data/service/el1/public/database/KvHiviewReporterNew/";
static constexpr const char *ENCRYPT_STOREID = "EncryptSingleKVStoreNew";
static constexpr const char *UNENCRYPT_STOREID = "UnencryptSingleKVStoreNew";
static constexpr const char *APPID = "KvHiviewReporterNew";
static constexpr const char *DB_CORRUPTED_POSTFIX = ".corruptedflg";
static constexpr const char *FULL_KVDB_PATH = "/data/service/el1/public/database/KvHiviewReporterNew/kvdb/";
static constexpr const char *FULL_KEY_PATH = "/data/service/el1/public/database/KvHiviewReporterNew/key/";

class KvHiviewReporterNew : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
    static std::shared_ptr<SingleKvStore> encryptKvStore_;
    static std::shared_ptr<SingleKvStore> unencryptKvStore_;
    static std::shared_ptr<DistributedKvDataManager> distributedKvDataManager_;
    static std::shared_ptr<HiSysEventMock> mock_;
    static AppId appId;
    static StoreId encryptStoreId;
    static StoreId unencryptStoreId;
    static Options encryptOptions;
    static Options unencryptOptions;
};

std::shared_ptr<SingleKvStore> KvHiviewReporterNew::encryptKvStore_;
std::shared_ptr<SingleKvStore> KvHiviewReporterNew::unencryptKvStore_;
std::shared_ptr<DistributedKvDataManager> KvHiviewReporterNew::distributedKvDataManager_;
std::shared_ptr<HiSysEventMock> KvHiviewReporterNew::mock_;

AppId  KvHiviewReporterNew::appId;
StoreId KvHiviewReporterNew::encryptStoreId;
StoreId KvHiviewReporterNew::unencryptStoreId;
Options KvHiviewReporterNew::encryptOptions;
Options KvHiviewReporterNew::unencryptOptions;

void KvHiviewReporterNew::SetUpTestCase(void)
{
    auto ret = mkdir(BASE_DIR, (S_IRWXU));
    if (ret != 0) {
        ZLOGE("Mkdir failed, result:%{public}d, path:%{public}s", ret, BASE_DIR);
        return;
    }
    distributedKvDataManager_ = std::make_shared<DistributedKvDataManager>();
    mock_ = std::make_shared<HiSysEventMock>();

    appId = { APPID };
    encryptStoreId = { ENCRYPT_STOREID };
    unencryptStoreId = { UNENCRYPT_STOREID };

    encryptOptions.kvStoreType = SINGLE_VERSION;
    encryptOptions.securityLevel = S1;
    encryptOptions.encrypt = true;
    encryptOptions.area = EL1;
    encryptOptions.backup = false;
    encryptOptions.baseDir = BASE_DIR;
   
    unencryptOptions.kvStoreType = SINGLE_VERSION;
    unencryptOptions.securityLevel = S1;
    unencryptOptions.encrypt = false;
    unencryptOptions.area = EL1;
    unencryptOptions.backup = false;
    unencryptOptions.baseDir = BASE_DIR;

    auto status = distributedKvDataManager_->GetSingleKvStore(encryptOptions, appId, encryptStoreId, encryptKvStore_);
    ASSERT_TRUE(status == SUCCESS);

    status = distributedKvDataManager_->GetSingleKvStore(unencryptOptions, appId, unencryptStoreId, unencryptKvStore_);
    ASSERT_TRUE(status == SUCCESS);
    ZLOGI("KvHiviewReporterNew getSingleKvStore end.");
}

void KvHiviewReporterNew::TearDownTestCase(void)
{
    auto status = distributedKvDataManager_->DeleteKvStore(appId, encryptStoreId, BASE_DIR);
    status = distributedKvDataManager_->DeleteKvStore(appId, unencryptStoreId, BASE_DIR);
    ZLOGI("KvHiviewReporterNew delete end.");
   
    encryptKvStore_ = nullptr;
    unencryptKvStore_ = nullptr;
    distributedKvDataManager_ = nullptr;
    mock_ = nullptr;

    (void)remove(FULL_KVDB_PATH);
    (void)remove(FULL_KEY_PATH);
    (void)remove(BASE_DIR);
}

void KvHiviewReporterNew::SetUp(void) { }

void KvHiviewReporterNew::TearDown(void) { }

/**
 * @tc.name: ReportInvalidArgumentNewTest001
 * @tc.desc: Report invalid argument testing on the unencryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest001, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest001 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest001 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest002
 * @tc.desc: Report invalid argument testing on the encryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest002, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest002 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest002 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest001
 * @tc.desc: Report store corruption and store recovery testing on the encryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest001, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest001 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest001 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest002
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest002, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest002 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest002 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest001
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest001, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest001 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest003
 * @tc.desc: Report invalid argument testing on the unencryption store duplicate.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest003, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest003 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest003 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest004
 * @tc.desc: Report invalid argument testing on the encryption store duplicate.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest004, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest004 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest004 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest003
 * @tc.desc: Report store corruption and store recovery testing on the encryption store duplicate.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest003, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest003 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest003 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest004
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store duplicate.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest004, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest004 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest004 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest002
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters duplicate.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest002, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest002 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest005
 * @tc.desc: Report invalid argument testing on the unencryption store three.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest005, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest005 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest005 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest006
 * @tc.desc: Report invalid argument testing on the encryption store three.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest006, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest006 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest006 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest005
 * @tc.desc: Report store corruption and store recovery testing on the encryption store three.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest005, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest005 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest005 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest006
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store three.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest006, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest006 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest006 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest003
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters three.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest003, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest003 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest007
 * @tc.desc: Report invalid argument testing on the unencryption store four.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest007, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest007 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest007 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest008
 * @tc.desc: Report invalid argument testing on the encryption store four.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest008, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest008 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest008 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest007
 * @tc.desc: Report store corruption and store recovery testing on the encryption store four.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest007, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest007 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest007 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest008
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store four.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest008, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest008 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest008 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest004
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters four.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest004, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest004 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest009
 * @tc.desc: Report invalid argument testing on the unencryption store five.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest009, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest009 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest009 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest010
 * @tc.desc: Report invalid argument testing on the encryption store five.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest010, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest010 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest010 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest009
 * @tc.desc: Report store corruption and store recovery testing on the encryption store five.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest009, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest009 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest009 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest010
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store five.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest010, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest010 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest010 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest005
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters five.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest005, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest005 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest011
 * @tc.desc: Report invalid argument testing on the unencryption store six.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest011, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest011 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest011 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest012
 * @tc.desc: Report invalid argument testing on the encryption store six.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest012, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest012 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest012 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest011
 * @tc.desc: Report store corruption and store recovery testing on the encryption store six.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest011, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest011 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest011 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest012
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store six.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest012, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest012 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest012 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest006
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters six.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest006, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest006 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest013
 * @tc.desc: Report invalid argument testing on the unencryption store seven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest013, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest013 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest013 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest014
 * @tc.desc: Report invalid argument testing on the encryption store seven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest014, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest014 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest014 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest013
 * @tc.desc: Report store corruption and store recovery testing on the encryption store seven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest013, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest013 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest013 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest014
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store seven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest014, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest014 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest014 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest007
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters seven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest007, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest007 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest015
 * @tc.desc: Report invalid argument testing on the unencryption store eight.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest015, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest015 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest015 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest016
 * @tc.desc: Report invalid argument testing on the encryption store eight.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest016, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest016 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest016 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest015
 * @tc.desc: Report store corruption and store recovery testing on the encryption store eight.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest015, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest015 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest015 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest016
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store eight.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest016, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest016 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest016 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest008
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters eight.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest008, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest008 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest017
 * @tc.desc: Report invalid argument testing on the unencryption store nine.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest017, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest017 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest017 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest018
 * @tc.desc: Report invalid argument testing on the encryption store nine.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest018, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest018 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest018 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest017
 * @tc.desc: Report store corruption and store recovery testing on the encryption store nine.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest017, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest017 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest017 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest018
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store nine.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest018, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest018 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest018 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest009
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters nine.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest009, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest009 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest019
 * @tc.desc: Report invalid argument testing on the unencryption store ten.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest019, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest019 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest019 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest020
 * @tc.desc: Report invalid argument testing on the encryption store ten.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest020, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest020 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest020 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest019
 * @tc.desc: Report store corruption and store recovery testing on the encryption store ten.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest019, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest019 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest019 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest020
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store ten.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest020, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest020 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest020 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}

/**
 * @tc.name: ReportCorruptEventNewTest010
 * @tc.desc: Execute the reportCorruptEvent method with invalid parameters ten.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportCorruptEventNewTest010, TestSize.Level1)
{
    ZLOGI("ReportCorruptEventNewTest010 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.storeName = "";
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);

    eventInfo.dbPath = FULL_KVDB_PATH;
    KVDBFaultHiViewReporter::ReportCorruptEvent(eventInfo);
    isDuplicate = KVDBFaultHiViewReporter::IsReportedCorruptedFault(eventInfo.bundleName, eventInfo.storeName,
        eventInfo.dbPath);
    ASSERT_TRUE(isDuplicate == false);
}

/**
 * @tc.name: ReportInvalidArgumentNewTest021
 * @tc.desc: Report invalid argument testing on the unencryption store eleven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest021, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest021 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    
    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest021 end.");
}

/**
 * @tc.name: ReportInvalidArgumentNewTest022
 * @tc.desc: Report invalid argument testing on the encryption store eleven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, ReportInvalidArgumentNewTest022, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentNewTest022 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    KVDBFaultEvent eventInfo(reportInfo.options);
    eventInfo.bundleName = reportInfo.appId;
    eventInfo.storeName = reportInfo.storeId;
    eventInfo.functionName = reportInfo.functionName;
    eventInfo.errorCode = reportInfo.errorCode;
    bool isDuplicate = KVDBFaultHiViewReporter::IsReportedFault(eventInfo);
    ASSERT_TRUE(isDuplicate == true);
    ZLOGI("ReportInvalidArgumentNewTest022 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildNewTest021
 * @tc.desc: Report store corruption and store recovery testing on the encryption store eleven.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterNew, StoreCorruptedAndRebuildNewTest021, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildNewTest021 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret == 0);

    ZLOGI("StoreCorruptedAndRebuildNewTest021 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_TRUE(ret != 0);
}
} // namespace OHOS::Test