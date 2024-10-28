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
#define LOG_TAG "SingleStorePerfPhoneTest"
#include "distributed_kv_data_manager.h"
#include <gtest/gtest.h>
#include "log_print.h"
#include <random>
#include <sys/time.h>
#include <thread>
#include <vector>
#include "store_manager.h"
#include "types.h"
#include "process_system_api_adapter_impl.h"

#ifdef DB_DEBUG_ENV

#include "system_time.h"

#endif // DB_DEBUG_ENV

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace std;
namespace OHOS::Test {
// define some variables to init a KvStoreDelegateManager object.
std::shared_ptr<SingleKvStore> store1;
std::shared_ptr<SingleKvStore> store2;
std::shared_ptr<SingleKvStore> store3;

class SingleStorePerfPhoneTest : public testing::Test {
public:
    std::shared_ptr<SingleKvStore> CreateKVStore(std::string storeIdTest, KvStoreType type, bool encrypt, bool backup);
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

OHOS::DistributedKv::Key GenerateBytes(const string &str)
{
    DistributedDB::Key bytes;
    const char *buffer = str.c_str();
    for (uint32_t i = 0; i < str.size(); i++) {
        bytes.emplace_back(buffer[i]);
    }
    return bytes;
}

string GenerateRandomString(long length)
{
    std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string out;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    for (int i = 0; i < length; i++) {
        out += chars[dis(gen)];
    }
    return out;
}

string GetDate(char *cType, int iDay)
{
    char cTimePARTNUMBER[256] = "";
    time_t rawtime;
    struct tm timeinfo{};
    rawtime = time(NULL) + iDay * 24 * 3600; // 24 hours a day, 3600 seconds per hour
    localtime_r(&rawtime, &timeinfo);
    char buffer1[256];
    size_t n = strftime(cTimePARTNUMBER, sizeof(buffer1), cType, &timeinfo); //20180623
    if (n != 0) {
        string strTimePARTNUMBER = cTimePARTNUMBER;
        return strTimePARTNUMBER;
    } else {
        return "";
    }
}

/**
 * Presets data to a specified single-key value store.
 * @param store Smart pointer of a single key-value store.
 * @param batch The number of batches, representing the batch in which the data was inserted.
 * @param count Number of pictures on a screen.
 * @param size Size of the value in kilobytes for each key-value pair.
 * @param ratio Ratio of compression
 */
void PresetData(std::shared_ptr<SingleKvStore> &store, int batch, int count, int size, int ratio)
{
    std::ostringstream s;
    s << std::setw(16) << std::setfill('0') << 0; // 16 bytes
    string strDate;
    string ss = to_string(size) + "K";
    for (int i = batch; i >= 1; i--) {
        strDate = GetDate((char*)"%Y%m%d", -i);
        for (int index = 1; index <= count; index++) {
            std::ostringstream s2;
            string tmp =
            s.str() + "_" + ss + "_" + strDate + string(3 - to_string(index).length(), '0') + to_string(index);
            string val;
            if (ratio != 0) {
                val = GenerateRandomString((long)(size * 1024) / ratio); // 1024 bytes per 1K
            } else {
                val = GenerateRandomString(0);
            }
            const DistributedDB::Key key = GenerateBytes(tmp);
            const DistributedDB::Value value = GenerateBytes(val);
            ASSERT_EQ(store->Put(key, value), SUCCESS);
            // 32 bytes
            for (int keyRange = 0; keyRange < 32; keyRange ++) {
                printf("%c", key[keyRange]);
            }
        }
        printf("put success!");
    }
}

/**
* @brief Calculates the duration of a RangeResultSet.
* @param store Smart pointer of a single key-value store.
* @param batch The number of batches, representing the batch in which the data was inserted.
* @param size Size of the value in kilobytes for each key-value pair.
* @param count Number of pictures on a screen.
* @details This function is used to calculate the duration for obtaining data on a screen.
*      Data is obtained through multiple cycles and the average time is calculated.
*/
void CalcRangeResultSetDuration(std::shared_ptr<SingleKvStore> &store, int batch, int size, int count)
{
    // batch = totolCnt / (512/ 150 / 30) size(30 10 3 ) count (512 150 30 )
    double dur;
    double totalTime;
    double avrTime = 0;
    int failCount = 0;
    for (int n = 0; n < 100; ++n) { // 100 times
        for (int ind = batch; ind >= 1; ind--) {
            std::shared_ptr<KvStoreResultSet> readResultSet;
            struct timeval startTime{};
            struct timeval endTime{};
            string tmp = string(16, '0') + "_" + to_string(size) + "K" + "_" +
                GetDate((char *) "%Y%m%d", -ind);
            string keyStartVar = tmp + "000";
            string keyEndVar = tmp + "999";
            DataQuery query;
            query.Between(GenerateBytes(keyStartVar).ToString(), GenerateBytes(keyEndVar).ToString());
            (void) gettimeofday(&startTime, nullptr);
            EXPECT_EQ(store->GetResultSet(query, readResultSet), SUCCESS);
            EXPECT_TRUE(readResultSet!= nullptr);
            for (int i = 0; i < count; ++i) {
                EXPECT_TRUE(readResultSet->MoveToNext());
                Entry entry;
                EXPECT_EQ(readResultSet->GetEntry(entry), 0);
            }
            (void) gettimeofday(&endTime, nullptr);
            EXPECT_EQ(store->CloseResultSet(readResultSet), SUCCESS);

            double startUsec = (double) (startTime.tv_sec * 1000 * 1000) + (double) startTime.tv_usec;
            double endUsec = (double) (endTime.tv_sec * 1000 * 1000) + (double) endTime.tv_usec;
            dur = endUsec - startUsec;
            totalTime = totalTime + dur;
            avrTime = (dur / 1000); // 1000 is to convert ms
            if (avrTime >= 3.0) { // 3.0 ms is upper bound on performance
                failCount += 1;
            }
            cout << "avrTime = " << avrTime << "failCount = " << failCount<< endl;
        }
    }
    if (batch != 0) {
        // 100 is for unit conversion
        avrTime = (((totalTime / batch) / 100) / 1000); // 1000 is to convert ms
        cout << "Scan Range ResultSet avg  cost = " << avrTime << " ms." << endl;
        EXPECT_LT(avrTime, 3.0); // 3.0 ms is upper bound on performance
    } else {
        cout << "Error: Division by zero." << endl;
    }
}

void SingleStorePerfPhoneTest::SetUpTestCase()
{
    std::string baseDir = "/data/service/el1/public/database/SingleStorePerfPhoneTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SingleStorePerfPhoneTest::TearDownTestCase()
{
    std::string baseDir = "/data/service/el1/public/database/SingleStorePerfPhoneTest";
    StoreManager::GetInstance().Delete({ "SingleStorePerfPhoneTest" }, { "SingleKVStore" }, baseDir);

    (void)remove("/data/service/el1/public/database/SingleStorePerfPhoneTest/key");
    (void)remove("/data/service/el1/public/database/SingleStorePerfPhoneTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleStorePerfPhoneTest");
}

void SingleStorePerfPhoneTest::SetUp()
{
    store1 = CreateKVStore("SingleKVStore1", LOCAL_ONLY, false, false);
    if (store1 == nullptr) {
        store1 = CreateKVStore("SingleKVStore1", LOCAL_ONLY, false, false);
    }
    ASSERT_NE(store1, nullptr);

    store2 = CreateKVStore("SingleKVStore2", LOCAL_ONLY, false, false);
    if (store2 == nullptr) {
        store2 = CreateKVStore("SingleKVStore2", LOCAL_ONLY, false, false);
    }
    ASSERT_NE(store2, nullptr);

    store3 = CreateKVStore("SingleKVStore3", LOCAL_ONLY, false, false);
    if (store3 == nullptr) {
        store3 = CreateKVStore("SingleKVStore3", LOCAL_ONLY, false, false);
    }
    ASSERT_NE(store3, nullptr);
}

void SingleStorePerfPhoneTest::TearDown()
{
    AppId appId = { "SingleStorePerfPhoneTest" };
    StoreId storeId1 = { "SingleKVStore1" };
    StoreId storeId2 = { "SingleKVStore2" };
    StoreId storeId3 = { "SingleKVStore3" };
    store1 = nullptr;
    store2 = nullptr;
    store3 = nullptr;
    auto status = StoreManager::GetInstance().CloseKVStore(appId, storeId1);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId2);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId3);
    ASSERT_EQ(status, SUCCESS);
    auto baseDir = "/data/service/el1/public/database/SingleStorePerfPhoneTest";
    status = StoreManager::GetInstance().Delete(appId, storeId1, baseDir);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId2, baseDir);
    ASSERT_EQ(status, SUCCESS);
    status = StoreManager::GetInstance().Delete(appId, storeId3, baseDir);
    ASSERT_EQ(status, SUCCESS);
}

std::shared_ptr<SingleKvStore> SingleStorePerfPhoneTest::CreateKVStore(std::string storeIdTest, KvStoreType type,
    bool encrypt, bool backup)
{
    Options options;
    options.kvStoreType = type;
    options.securityLevel = S1;
    options.encrypt = encrypt;
    options.area = EL1;
    options.backup = backup;
    options.baseDir = "/data/service/el1/public/database/SingleStorePerfPhoneTest";

    AppId appId = { "SingleStorePerfPhoneTest" };
    StoreId storeId = { storeIdTest };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    return StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
}

/**
 * @tc.name: Gallery1WThumbnailsKVStoreBetweenTest
 * @tc.desc: Gallery 10,000 thumbnails High-performance KV database Native interface Between
 *               query performance less than 3 ms
 * @tc.type: PERF
 * @tc.require:
 * @tc.author: Gang Wang
 */
HWTEST_F(SingleStorePerfPhoneTest, Gallery1WThumbnailsKVStoreBetweenTest, TestSize.Level0)
{
    int monthlyBatch = (int) (10000 / 112);
    int annuallyBatch = (int) (10000 / 448);
    int dailyBatch = (int) (10000 / 32);
    int ratio = 1;
    printf("monthly put start");
    PresetData(store1, monthlyBatch, 112, 8, ratio);
    printf("annually put start");
    PresetData(store2, annuallyBatch, 448, 4, ratio);
    printf("daily put start");
    PresetData(store3, dailyBatch, 32, 16, ratio);

    CalcRangeResultSetDuration(store1, monthlyBatch, 8, 112);
    CalcRangeResultSetDuration(store2, annuallyBatch, 4, 448);
    CalcRangeResultSetDuration(store3, dailyBatch, 16, 32);
}

/**
 * @tc.name: Gallery5WThumbnailsKVStoreBetweenTest
 * @tc.desc: Gallery 50,000 thumbnails High-performance KV database Native interface Between
 *               query performance less than 3 ms
 * @tc.type: PERF
 * @tc.require:
 * @tc.author: Gang Wang
 */
HWTEST_F(SingleStorePerfPhoneTest, Gallery5WThumbnailsKVStoreBetweenTest, TestSize.Level0)
{
    int monthlyBatch = (int) (50000 / 112);
    int annuallyBatch = (int) (50000 / 448);
    int dailyBatch = (int) (50000 / 32);
    int ratio = 1;
    printf("monthly put start");
    PresetData(store1, monthlyBatch, 112, 8, ratio);
    printf("annually put start");
    PresetData(store2, annuallyBatch, 448, 4, ratio);
    printf("daily put start");
    PresetData(store3, dailyBatch, 32, 16, ratio);

    CalcRangeResultSetDuration(store1, monthlyBatch, 8, 112);
    CalcRangeResultSetDuration(store2, annuallyBatch, 4, 448);
    CalcRangeResultSetDuration(store3, dailyBatch, 16, 32);
}
} // namespace OHOS::Test