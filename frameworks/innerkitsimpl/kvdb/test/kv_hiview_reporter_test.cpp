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

#define LOG_TAG "KvHiviewReporterTest"
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

static constexpr const char *BASE_DIR = "/data/service/el1/public/database/KvHiviewReporterTest/";
static constexpr const char *ENCRYPT_STOREID = "EncryptSingleKVStore";
static constexpr const char *UNENCRYPT_STOREID = "UnencryptSingleKVStore";
static constexpr const char *APPID = "KvHiviewReporterTest";
static constexpr const char *DB_CORRUPTED_POSTFIX = ".corruptedflg";
static constexpr const char *FULL_KVDB_PATH = "/data/service/el1/public/database/KvHiviewReporterTest/kvdb/";
static constexpr const char *FULL_KEY_PATH = "/data/service/el1/public/database/KvHiviewReporterTest/key/";

class KvHiviewReporterTest : public testing::Test {
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

std::shared_ptr<SingleKvStore> KvHiviewReporterTest::encryptKvStore_;
std::shared_ptr<SingleKvStore> KvHiviewReporterTest::unencryptKvStore_;
std::shared_ptr<DistributedKvDataManager> KvHiviewReporterTest::distributedKvDataManager_;
std::shared_ptr<HiSysEventMock> KvHiviewReporterTest::mock_;

AppId  KvHiviewReporterTest::appId;
StoreId KvHiviewReporterTest::encryptStoreId;
StoreId KvHiviewReporterTest::unencryptStoreId;
Options KvHiviewReporterTest::encryptOptions;
Options KvHiviewReporterTest::unencryptOptions;

void KvHiviewReporterTest::SetUpTestCase(void)
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
   
    unencryptOptions.kvStoreType = SINGLE_VERSION,
    unencryptOptions.securityLevel = S1,
    unencryptOptions.encrypt = false,
    unencryptOptions.area = EL1,
    unencryptOptions.backup = false,
    unencryptOptions.baseDir = BASE_DIR;

    auto status = distributedKvDataManager_->GetSingleKvStore(encryptOptions, appId, encryptStoreId, encryptKvStore_);
    ASSERT_EQ(status, SUCCESS);

    status = distributedKvDataManager_->GetSingleKvStore(unencryptOptions, appId, unencryptStoreId, unencryptKvStore_);
    ASSERT_EQ(status, SUCCESS);
    ZLOGI("KvHiviewReporterTest getSingleKvStore end.");
}

void KvHiviewReporterTest::TearDownTestCase(void)
{
    auto status = distributedKvDataManager_->DeleteKvStore(appId, encryptStoreId, BASE_DIR);
    ASSERT_EQ(status, SUCCESS);

    status = distributedKvDataManager_->DeleteKvStore(appId, unencryptStoreId, BASE_DIR);
    ASSERT_EQ(status, SUCCESS);
    ZLOGI("KvHiviewReporterTest delete end.");
   
    encryptKvStore_ = nullptr;
    unencryptKvStore_ = nullptr;
    distributedKvDataManager_ = nullptr;
    mock_ = nullptr;

    (void)remove(FULL_KVDB_PATH);
    (void)remove(FULL_KEY_PATH);
    (void)remove(BASE_DIR);
}

void KvHiviewReporterTest::SetUp(void) { }

void KvHiviewReporterTest::TearDown(void) { }

/**
 * @tc.name: ReportInvalidArgumentTest001
 * @tc.desc: Report invalid argument testing on the unencryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportInvalidArgumentTest001, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentTest001 begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.systemErrorNo = errno;
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
    ASSERT_TRUE(isDuplicate);
    ZLOGI("ReportInvalidArgumentTest001 end.");
}

/**
 * @tc.name: ReportInvalidArgumentTest002
 * @tc.desc: Report invalid argument testing on the encryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportInvalidArgumentTest002, TestSize.Level1)
{
    ZLOGI("ReportInvalidArgumentTest002 begin.");
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::INVALID_ARGUMENT;
    reportInfo.systemErrorNo = errno;
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
    ASSERT_TRUE(isDuplicate);
    ZLOGI("ReportInvalidArgumentTest002 end.");
}

/**
 * @tc.name: StoreCorruptedAndRebuildTest001
 * @tc.desc: Report store corruption and store recovery testing on the encryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, StoreCorruptedAndRebuildTest001, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildTest001 ReportKVFaultEvent begin.");
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo;
    reportInfo.options = encryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.systemErrorNo = errno;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = encryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions.GetDatabaseDir(), encryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_EQ(ret, 0);

    ZLOGI("StoreCorruptedAndRebuildTest001 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: StoreCorruptedAndRebuildTest002
 * @tc.desc: Report store corruption and store recovery testing on the unencryption store.
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, StoreCorruptedAndRebuildTest002, TestSize.Level1)
{
    ZLOGI("StoreCorruptedAndRebuildTest002 ReportKVFaultEvent begin.");
    ReportInfo reportInfo;
    reportInfo.options = unencryptOptions;
    reportInfo.errorCode = Status::DATA_CORRUPTED;
    reportInfo.systemErrorNo = errno;
    reportInfo.appId = appId.appId;
    reportInfo.storeId = unencryptStoreId.storeId;
    reportInfo.functionName = std::string(__FUNCTION__);
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(unencryptOptions.GetDatabaseDir(),
        unencryptStoreId.storeId);
    std::string flagFilename = dbPath + std::string(UNENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_EQ(ret, 0);

    ZLOGI("StoreCorruptedAndRebuildTest002 ReportKVRebuildEvent begin.");
    reportInfo.errorCode = Status::SUCCESS;
    EXPECT_CALL(*mock_, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_NE(ret, 0);
}
} // namespace OHOS::Test