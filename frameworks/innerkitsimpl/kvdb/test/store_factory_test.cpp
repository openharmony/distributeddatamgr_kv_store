/*
* Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "store_factory.h"
#include <gtest/gtest.h>
#include <vector>
#include <sys/time.h>
#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include "sys/stat.h"
#include "file_ex.h"
#include "store_manager.h"
#include "store_util.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class StoreFactoryTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static bool GetDate(const std::string &name, const std::string &path, std::vector<uint8_t> &date);
    static bool ChangeSystemTime(int num);
    static bool MoveToRekeyPath(Options options, StoreId storeId, bool &movedToRekeyPath);

    AppId appId = { "rekey" };
    StoreId storeId = { "single_test" };
    Options options = {
        .encrypt = true,
        .securityLevel = S1,
        .area = EL1,
        .kvStoreType = SINGLE_VERSION,
        .baseDir = "/data/service/el1/public/database/rekey",
    };
};

void StoreFactoryTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/rekey";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void StoreFactoryTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/rekey");
}

void StoreFactoryTest::SetUp(void) {}

void StoreFactoryTest::TearDown(void) {}

bool StoreFactoryTest::GetDate(const std::string &name, const std::string &path, std::vector<uint8_t> &date)
{
    auto keyPath = path + "/key/" + name + ".key";
    if (!OHOS::FileExists(keyPath)) {
        return false;
    }

    std::vector<char> content;
    auto loaded = OHOS::LoadBufferFromFile(keyPath, content);
    if (!loaded) {
        return false;
    }

    size_t offset = 1;
    date.assign(content.begin() + offset, content.begin() + (sizeof(time_t) / sizeof(uint8_t)) + offset);
    return true;
}

bool StoreFactoryTest::ChangeSystemTime(int num)
{
    bool isChanged = false;
    timeval p;
    gettimeofday(&p, nullptr);
    p.tv_sec += num;
    settimeofday(&p, nullptr);
    isChanged = true;
    return isChanged;
}

bool StoreFactoryTest::MoveToRekeyPath(Options options, StoreId storeId, bool &movedToRekeyPath)
{
    movedToRekeyPath = false;
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    std::string rekeyFileName = options.baseDir + "/rekey/key/" + storeId.storeId + ".new.key";
    StoreUtil::Rename(keyFileName, rekeyFileName);
    StoreUtil::Remove(keyFileName);
    movedToRekeyPath = true;
    return movedToRekeyPath;
}

/**
* @tc.name: Rekey
* @tc.desc: test rekey function
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, Rekey, TestSize.Level1)
{
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    ASSERT_EQ(status, SUCCESS);
    int fiveHundredDays = 60 * 60 * 24 * 500;
    ASSERT_EQ(ChangeSystemTime(-fiveHundredDays), true);
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> date;
    bool getDate = GetDate(storeId, options.baseDir, date);
    ASSERT_EQ(getDate, true);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    ASSERT_EQ(ChangeSystemTime(fiveHundredDays), true);
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);
    bool isDiff = false;
    size_t size = newDate.size();
    for (size_t i = 0; i < size; ++i) {
        if (char (date[i]) != char (newDate[i])) {
            isDiff = true;
            break;
        }
    }
    ASSERT_EQ(isDiff, true);
}

/**
* @tc.name: RekeyNotOutdated
* @tc.desc: try to rekey kvstore with not outdated password
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, RekeyNotOutdated, TestSize.Level1)
{
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    ASSERT_EQ(status, SUCCESS);
    int fiftyDays = 60 * 60 * 24 * 50;
    ChangeSystemTime(-fiftyDays);
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> date;
    bool getDate = GetDate(storeId, options.baseDir, date);
    ASSERT_EQ(getDate, true);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    ChangeSystemTime(fiftyDays);
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);
    bool isDiff = false;
    size_t size = newDate.size();
    for (size_t i = 0; i < size; ++i) {
        if (date[i] != newDate[i]) {
            isDiff = true;
            break;
        }
    }
    ASSERT_EQ(isDiff, false);
}

/**
* @tc.name: RekeyNotOutdated
* @tc.desc: mock the situation that open store after rekey was interrupted last time,
*           which caused key file lost but rekey key file exist.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, RekeyInterrupted0, TestSize.Level1)
{
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    ASSERT_EQ(status, SUCCESS);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> date;
    bool getDate = GetDate(storeId, options.baseDir, date);
    ASSERT_EQ(getDate, true);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    bool movedToRekeyPath = false;
    MoveToRekeyPath(options, storeId, movedToRekeyPath);
    ASSERT_EQ(movedToRekeyPath, true);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    auto isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, true);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);
    bool isDiff = false;
    size_t size = newDate.size();
    for (size_t i = 0; i < size; ++i) {
        if (date[i] != newDate[i]) {
            isDiff = true;
            break;
        }
    }
    ASSERT_EQ(isDiff, false);
}

/**
* @tc.name: RekeyNotOutdated
* @tc.desc: mock the situation that open store after rekey was interrupted last time,
*           which caused key file not changed but rekey key file exist.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, RekeyInterrupted1, TestSize.Level1)
{
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    ASSERT_EQ(status, SUCCESS);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> date;
    bool getDate = GetDate(storeId, options.baseDir, date);
    ASSERT_EQ(getDate, true);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    bool movedToRekeyPath = false;
    MoveToRekeyPath(options, storeId, movedToRekeyPath);
    ASSERT_EQ(movedToRekeyPath, true);

    StoreId mockStoreId = { "mock" };
    std::string mockPath = options.baseDir;
    StoreManager::GetInstance().GetKVStore(appId, mockStoreId, options, status);

    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    std::string mockKeyFileName = options.baseDir + "/key/" + mockStoreId.storeId + ".key";
    StoreUtil::Rename(mockKeyFileName, keyFileName);
    StoreUtil::Remove(mockKeyFileName);
    auto isKeyExist = StoreUtil::IsFileExist(mockKeyFileName);
    ASSERT_EQ(isKeyExist, false);
    isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, true);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, true);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);
    bool isDiff = false;
    size_t size = newDate.size();
    for (size_t i = 0; i < size; ++i) {
        if (date[i] != newDate[i]) {
            isDiff = true;
            break;
        }
    }
    ASSERT_EQ(isDiff, false);
}
