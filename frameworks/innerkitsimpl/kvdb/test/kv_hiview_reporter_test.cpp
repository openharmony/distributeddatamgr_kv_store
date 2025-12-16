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
#include "store_manager.h"

namespace OHOS::Test {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedKv;

static constexpr const char *BASE_DIR = "/data/service/el1/public/database/KvHiviewReporterTest/";
static constexpr const char *STOREID = "SingleKVStore";
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
    std::shared_ptr<SingleKvStore> kvStore_;
};

void KvHiviewReporterTest::SetUpTestCase(void)
{
    auto ret = mkdir(BASE_DIR, (S_IRWXU));
    if (ret != 0) {
        ZLOGE("Mkdir failed, result:%{public}d, path:%{public}s", ret, BASE_DIR);
    }
    ASSERT_EQ(ret, 0);
    kvStore_ = CreateKVStore(STOREID, SINGLE_VERSION, true, false);
    ASSERT_NE(kvStore_, nullptr);

}

void KvHiviewReporterTest::TearDownTestCase(void)
{
    kvStore_ = nullptr;
    AppId appId = { APPID } ;
    StoreId storeId = { STOREID };
    std::string baseDir = BASE_DIR;
    auto status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ASSERT_EQ(status, SUCCESS);
    (void)remove(FULL_KVDB_PATH);
    (void)remove(FULL_KEY_PATH);
    (void)remove(BASE_DIR);

}

void KvHiviewReporterTest::SetUp(void) { }

void KvHiviewReporterTest::TearDown(void) { }

std::shared_ptr<SingleKvStore> KvHiviewReporterTest::CreateKVStore(
    std::string storeIdTest, KvStoreType type, bool encrypt, bool backup)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.encrypt = encrypt;
    options.area = EL1;
    options.backup = backup;
    options.baseDir = BASE_DIR;

    AppId appId = { APPID };
    StoreId storeId = { storeIdTest };
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
}

/**
 * @tc.name: ReportKVFaultEvent001
 * @tc.desc: Execute the ReportFaultEvent method
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportKVFaultEvent001, TestSize.Level0)
{
    ZLOGI("ReportKVFaultEvent001 begin.");
    AppId appId = { APPID } ;
    StoreId storeId = { STOREID };
    Options options;
    options.baseDir = BASE_DIR;
    options.encrypt = true;
    Status status = DATA_CORRUPTED;;
    HiSysEventMock mock;
    EXPECT_CALL(mock, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(2);
    ReportInfo reportInfo = { .options = options, .errorCode = status, .systemErrorNo = errno,
                .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    std::string flagFilename = std::string(BASE_DIR) + std::string(STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: ReportKVRebuildEvent001
 * @tc.desc: Execute the ReportKVRebuildEvent method
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportKVRebuildEvent001, TestSize.Level0)
{
    ZLOGI("ReportKVRebuildEvent001 begin.");
    AppId appId = { APPID } ;
    StoreId storeId = { STOREID };
    Options options;
    options.baseDir = BASE_DIR;
    options.encrypt = true;
    Status status = SUCCESS;
    HiSysEventMock mock;
    EXPECT_CALL(mock, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    ReportInfo reportInfo = { .options = options, .errorCode = status, .systemErrorNo = errno,
                .appId = APPID, .storeId = STOREID, .functionName = std::string(__FUNCTION__) };
    KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(options.GetDatabaseDir(), storeId.storeId);
    auto ret = remove(dbPath.c_str());
    ASSERT_EQ(ret, 0);

}
} // namespace OHOS::Test