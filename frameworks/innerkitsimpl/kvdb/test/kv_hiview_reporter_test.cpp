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
static constexpr const char *STOREID = "test_storeId";
static constexpr const char *DB_CORRUPTED_POSTFIX = ".corruptedflg";
static constexpr const char *FULL_KVDB_PATH = "/data/service/el1/public/database/KvHiviewReporterTest/kvdb/";
static constexpr const char *FULL_KEY_PATH = "/data/service/el1/public/database/KvHiviewReporterTest/key/";



class KvHiviewReporterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void KvHiviewReporterTest::SetUpTestCase(void)
{
    auto ret = mkdir(BASE_DIR, (S_IRWXU));
    if (ret != 0) {
        ZLOGE("Mkdir failed, result:%{public}d, path:%{public}s", ret, BASE_DIR);
    }
}

void KvHiviewReporterTest::TearDownTestCase(void)
{
    auto ret = remove(BASE_DIR);
    if (ret != 0) {
        ZLOGE("Remove failed, result:%{public}d, path:%{public}s", ret, BASE_DIR);
    }
}

void KvHiviewReporterTest::SetUp(void) { }

void KvHiviewReporterTest::TearDown(void) { }

/**
 * @tc.name: CorruptedFlagTest001
 * @tc.desc: Create and delete corrupted flag
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, CorruptedFlagTest001, TestSize.Level1)
{
    ZLOGI("CorruptedFlagTest001 begin.");
    KVDBFaultHiViewReporter::CreateCorruptedFlag(BASE_DIR, STOREID);
    std::string flagFilename = std::string(BASE_DIR) + std::string(STOREID) + std::string(DB_CORRUPTED_POSTFIX);
    auto ret = access(flagFilename.c_str(), F_OK);
    ASSERT_EQ(ret, 0);
    KVDBFaultHiViewReporter::DeleteCorruptedFlag(BASE_DIR, STOREID);
    ret = access(flagFilename.c_str(), F_OK);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: ReportKVFaultEvent001
 * @tc.desc: Execute the ReportFaultEvent method
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportKVFaultEvent001, TestSize.Level1)
{
    ZLOGI("ReportKVFaultEvent001 begin.");
    HiSysEventMock mock;
    EXPECT_CALL(mock, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(1);
    ReportInfo reportInfo;
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
}

/**
 * @tc.name: ReportKVFaultEvent002
 * @tc.desc: Execute the ReportFaultEvent method
 * @tc.type: FUNC
 */
HWTEST_F(KvHiviewReporterTest, ReportKVFaultEvent002, TestSize.Level1)
{
    ZLOGI("ReportKVFaultEvent002 getKvStore begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    AppId appId = { "KvHiviewReporterTest" };
    StoreId storeId = { "SingleKVStore" };
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.baseDir = BASE_DIR;
    options.backup = false;
    options.encrypt = true;
    Status status;
    kvStore = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_NE(kvStore, nullptr);

    ZLOGI("ReportKVFaultEvent002 reportKVFaultEvent begin.");
    HiSysEventMock mock;
    EXPECT_CALL(mock, HiSysEvent_Write(_, _, _, _, _, _, _)).Times(3);
    status = DATA_CORRUPTED;
    ReportInfo reportInfo = { .options = options, .errorCode = status, .systemErrorNo = errno,
                .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
    KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);

    ZLOGI("ReportKVFaultEvent002 delete kvStore begin.");
    std::string baseDir = BASE_DIR;
    StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    ZLOGI("ReportKVFaultEvent002 delete kvStore end.");

    ZLOGI("ReportKVFaultEvent002 delete dir begin.");
    std::string dbPath = KVDBFaultHiViewReporter::GetDBPath(options.GetDatabaseDir(), storeId.storeId);
    auto ret = remove(dbPath.c_str());
    ASSERT_EQ(ret, 0);
    ret = remove(FULL_KVDB_PATH);
    ASSERT_EQ(ret, 0);
    ret = remove(FULL_KEY_PATH);
    ASSERT_EQ(ret, 0);
    ZLOGI("ReportKVFaultEvent002 delete dir end.");
}
} // namespace OHOS::Test