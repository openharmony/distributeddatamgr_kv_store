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
#include <condition_variable>
#include <gtest/gtest.h>
#include <random>
#include <vector>

#include "block_data.h"
#include "dev_manager.h"
#include "distributed_kv_data_manager.h"
#include "file_ex.h"
#include "kv_store_nb_delegate.h"
#include "store_manager.h"
#include "sys/stat.h"
#include "types.h"
#include "log_print.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
static constexpr int THUMBNBIL_MAX_SIZE = 130;
class SingleStorePerfPcTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, KvStoreType type, bool encrypt, bool backup);
    static std::shared_ptr<SingleKvStore> CreateHashKVStore(std::string storeIdTest, KvStoreType type,
        bool encrypt, bool backup);
    static std::shared_ptr<SingleKvStore> kvStore_;
};

std::shared_ptr<SingleKvStore> SingleStorePerfPcTest::kvStore_;

void SingleStorePerfPcTest::SetUpTestCase(void)
{
    printf("SetUpTestCase BEGIN\n");
    std::string baseDir = "/data/service/el1/public/database/SingleStorePerfPcTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    kvStore_ = CreateHashKVStore("SingleKVStore", LOCAL_ONLY, false, false);
    if (kvStore_ == nullptr) {
        kvStore_ = CreateHashKVStore("SingleKVStore", LOCAL_ONLY, false, false);
    }
    EXPECT_NE(kvStore_, nullptr);
    //kvstore Insert 1 million PC file management thumbnail data
    std::string strKey(8, 'k'); // The length is 8
    std::string strValue(4592, 'v'); // The length is 4592
    int count = 1000000;
    std::vector<Key> keyVec;
    /// set key,value
    for (int i = 0; i < count; ++i) {
        std::string tmpKey = std::to_string(i);
        tmpKey = strKey.replace(0, tmpKey.size(), tmpKey);
        Key key(tmpKey);
        Value value(strValue);
        auto status = kvStore_->Put(key, value);
        EXPECT_EQ(status, SUCCESS);
    }
    printf("SetUpTestCase END\n");
}

void SingleStorePerfPcTest::TearDownTestCase(void)
{
    printf("TearDownTestCase BEGIN\n");
    AppId appId = { "SingleStorePerfPcTest" };
    StoreId storeId = { "SingleKVStore" };
    kvStore_ = nullptr;
    auto status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    EXPECT_EQ(status, SUCCESS);
    std::string baseDir = "/data/service/el1/public/database/SingleStorePerfPcTest";
    status = StoreManager::GetInstance().Delete(appId, storeId, baseDir);
    EXPECT_EQ(status, SUCCESS);

    printf("remove key kvdb SingleStorePerfPcTest BEGIN\n");
    (void)remove("/data/service/el1/public/database/SingleStorePerfPcTest/key");
    (void)remove("/data/service/el1/public/database/SingleStorePerfPcTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleStorePerfPcTest");
    printf("remove key kvdb SingleStorePerfPcTest END\n");
    printf("TearDownTestCase END\n");
}

void SingleStorePerfPcTest::SetUp(void)
{
    printf("SetUp BEGIN\n");
    printf("SetUp END\n");
}

void SingleStorePerfPcTest::TearDown(void)
{
    printf("TearDown BEGIN\n");
    printf("TearDown END\n");
}

std::shared_ptr<SingleKvStore> SingleStorePerfPcTest::CreateKVStore(std::string storeIdTest, KvStoreType type,
    bool encrypt, bool backup)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.encrypt = encrypt;
    options.area = EL1;
    options.backup = backup;
    options.baseDir = "/data/service/el1/public/database/SingleStorePerfPcTest";

    AppId appId = { "SingleStorePerfPcTest" };
    StoreId storeId = { storeIdTest };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
}

std::shared_ptr<SingleKvStore> SingleStorePerfPcTest::CreateHashKVStore(std::string storeIdTest, KvStoreType type,
    bool encrypt, bool backup)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.encrypt = encrypt;
    options.area = EL1;
    options.backup = backup;
    options.baseDir = "/data/service/el1/public/database/SingleStorePerfPcTest";
    options.config.type = HASH;
    options.config.pageSize = 32u;
    options.config.cacheSize = 4096u;

    AppId appId = { "SingleStorePerfPcTest" };
    StoreId storeId = { storeIdTest };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
}

static struct timespec GetTime(void)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now;
}

static double TimeDiff(const struct timespec *beg, const struct timespec *end)
{
    // 1000000000.0 to convert nanoseconds to seconds
    return (end->tv_sec - beg->tv_sec) + ((end->tv_nsec - beg->tv_nsec) / 1000000000.0);
}

/**
 * @tc.name: HashIndexKVStoreTest001
 * @tc.desc: PC File Management simulation:1 threads use vector 2000 keys 500 batches one by one
 *              Get the value of the kv store, and the average delay of each key is calculated in 2000 keys.
 *           Then, the average delay of a screen is calculated by multiplying the delay by 130.
 * @tc.type: PERF
 * @tc.require:
 * @tc.author: Gang Wang
 */
HWTEST_F(SingleStorePerfPcTest, HashIndexKVStoreTest001, TestSize.Level0)
{
    printf("HashIndexKVStoreTest001 BEGIN\n");
    std::string strKey(8, 'k');
    int count = 1000000;
    // We can't divide 130. We use 2000
    int batchCount = 2000;
    int failCount = 0;
    std::vector<Key> keyVec;
    double dur = 0;
    double avrTime130 = 0;

    keyVec.clear();
    for (int j = 0; j < count / batchCount; j++) {
        keyVec.clear();
        for (int i = j * batchCount; i < (j + 1) * batchCount; i++) {
            std::string tmpKey = std::to_string(i);
            tmpKey = strKey.replace(0, tmpKey.size(), tmpKey);
            Key key(tmpKey);
            keyVec.emplace_back(key);
        }
        timespec beg = GetTime();
        for (auto &item : keyVec) {
            Value value;
            kvStore_->Get(item, value);
        }
        timespec end = GetTime();
        double tmp = TimeDiff(&beg, &end);
        dur += tmp;
        avrTime130 = tmp * THUMBNBIL_MAX_SIZE * 1000 / keyVec.size();
        if (avrTime130 >= 3.0) {
            failCount += 1;
        }
        printf("platform:HAD_OH_Native_innerKitAPI_Get index[%d] keyLen[%d] valueLen[%d],\n"
            "batch_time[%lfs], avrTime130[%lfms], failCount[%u]\n",
            j, 8, 4592, tmp, avrTime130, failCount);
    }
    avrTime130 = dur * THUMBNBIL_MAX_SIZE * 1000 / count;
    printf("batchCount[%d] keyLen[%d] valueLen[%d], sum_time[%lfs], avrTime130[%lfms],\n"
        "HashIndexKVStoreTest001 failCount[%u]\n",
        batchCount, 8, 4592, dur, avrTime130, failCount);
    EXPECT_LT(avrTime130, 3.0);
    printf("HashIndexKVStoreTest001 END\n");
}

/**
 * @tc.name: HashIndexKVStoreTest002
 * @tc.desc: PC File Management simulation:1 threads use vector 1000000 keys one by one Get the value of the kv store,
 *              and the average delay of each key is calculated.
 *           Then, the average delay of a screen is calculated by multiplying the delay by 130.
 * @tc.type: PERF
 * @tc.require:
 * @tc.author: Gang Wang
 */
HWTEST_F(SingleStorePerfPcTest, HashIndexKVStoreTest002, TestSize.Level0)
{
    printf("HashIndexKVStoreTest002 BEGIN\n");
    std::string strKey(8, 'k');
    int count = 1000000;

    std::vector<Key> keyVec;
    double dur = 0;
    double avrTime130 = 0;
    keyVec.clear();
    for (int i = 0; i < count; ++i) {
        std::string tmpKey = std::to_string(i);
        tmpKey = strKey.replace(0, tmpKey.size(), tmpKey);
        Key key(tmpKey);
        keyVec.emplace_back(key);
    }
    timespec beg = GetTime();
    for (auto &item : keyVec) {
        Value value;
        kvStore_->Get(item, value);
    }
    timespec end = GetTime();
    dur = TimeDiff(&beg, &end);
    avrTime130 = dur * THUMBNBIL_MAX_SIZE * 1000 / count;
    printf("count[%d] platform:HAD_OH_Native_innerKitAPI_Get keyLen[%d] valueLen[%d], time[%lfs],\n"
        "avrTime130[%lfms]\n",
        count, 8, 4592, dur, avrTime130);
    EXPECT_LT(avrTime130, 3.0);
    printf("HashIndexKVStoreTest002 END\n");
}
} // namespace OHOS::Test
