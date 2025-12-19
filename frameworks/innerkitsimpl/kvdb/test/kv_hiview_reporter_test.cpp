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

#include "include/hisysevent_mock.h"
#include "kv_hiview_reporter.h"
#include "log_print.h"
#include "types.h"
#include <unistd.h>
#include "distributed_kv_data_manager.h"
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
Options encryptOptions_ = {
    .kvStoreType = SINGLE_VERSION,
    .securityLevel = S1,
    .encrypt = true,
    .area = EL1,
    .backup = false,
    .baseDir = BASE_DIR
};
Options unencryptOptions_ = {
    .kvStoreType = SINGLE_VERSION,
    .securityLevel = S1,
    .encrypt = false,
    .area = EL1,
    .backup = false,
    .baseDir = BASE_DIR
};

class KvHiviewReporterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
    static std::shared_ptr<SingleKvStore> encryptKvStore_;
    static std::shared_ptr<SingleKvStore> unencryptKvStore_;
    static std::shared_ptr<DistributedKvDataManager> manage_;

};

std::shared_ptr<SingleKvStore> KvHiviewReporterTest::encryptKvStore_;
std::shared_ptr<SingleKvStore> KvHiviewReporterTest::unencryptKvStore_;
std::shared_ptr<DistributedKvDataManager> KvHiviewReporterTest::manage_;




void KvHiviewReporterTest::SetUpTestCase(void)
{
    auto ret = mkdir(BASE_DIR, (S_IRWXU));
    if (ret != 0) {
        ZLOGE("Mkdir failed, result:%{public}d, path:%{public}s", ret, BASE_DIR);
    }
    AppId appId = { APPID };
    StoreId storeId = { ENCRYPT_STOREID };
    auto status = manage_->GetSingleKvStore(encryptOptions_, appId, storeId, encryptKvStore_);
    ASSERT_EQ(status, SUCCESS);

    storeId = { UNENCRYPT_STOREID };
    status = manage_->GetSingleKvStore(unencryptOptions_, appId, storeId, unencryptKvStore_);
    ASSERT_EQ(status, SUCCESS);
    ZLOGI("KvHiviewReporterTest getSingleKvStore end.");
}

void KvHiviewReporterTest::TearDownTestCase(void)
{
    AppId appId = { APPID };
    StoreId storeId = { ENCRYPT_STOREID };
    std::string baseDir = BASE_DIR;
    auto status = manage_->DeleteKvStore(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);

    storeId = { UNENCRYPT_STOREID };
    status = manage_->DeleteKvStore(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
    ZLOGI("KvHiviewReporterTest delete end.");
   
    encryptKvStore_ = nullptr;
    unencryptKvStore_ = nullptr;
    manage_ = nullptr;

    (void)remove(FULL_KVDB_PATH);
    (void)remove(FULL_KEY_PATH);
    (void)remove(BASE_DIR);
}

void KvHiviewReporterTest::SetUp(void) { }

void KvHiviewReporterTest::TearDown(void) { }

/**
 * @tc.name: ReportKVFaultEvent001
 * @tc.desc: Use an unencryption library to Execute the ReportFaultEvent method. 
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportKVFaultEvent001, TestSize.Level0)
{
    ZLOGI("ReportKVFaultEvent001 begin.");
    AppId appId = { APPID } ;
    StoreId storeId = { UNENCRYPT_STOREID };
    Status status = Status::INVALID_ARGUMENT;
    HiSysEventMock mock;
    EXPECT_CALL(mock, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    ReportInfo reportInfo = { .options = unencryptOptions_, .errorCode = status, .systemErrorNo = errno,
                .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
}

/**
 * @tc.name: ReportKVFaultAndRebuildTest001
 * @tc.desc: Use an encryption library to Execute the ReportKVFaultEvent and ReportKVRebuildEvent method. 
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportKVFaultAndRebuildTest001, TestSize.Level0)
{
    ZLOGI("KvHiviewReporterTest001 ReportKVFaultEvent begin.");
    AppId appId = { APPID } ;
    StoreId storeId = { ENCRYPT_STOREID };
    Status status = Status::DATA_CORRUPTED;
    HiSysEventMock mock;
    EXPECT_CALL(mock, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo = { .options = encryptOptions_, .errorCode = status, .systemErrorNo = errno,
                .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(encryptOptions_.GetDatabaseDir(), storeId.storeId);
    std::string flagFilename = dbPath + std::string(ENCRYPT_STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_EQ(ret, 0);

    ZLOGI("KvHiviewReporterTest001 ReportKVRebuildEvent begin.");
    status = Status::SUCCESS;
    EXPECT_CALL(mock, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    reportInfo = { .options = encryptOptions_, .errorCode = status, .systemErrorNo = errno,
                .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_NE(ret, 0);
}
} // namespace OHOS::Test