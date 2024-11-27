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

#include "block_data.h"
#include "log_print.h"
#include "security_manager.h"
#include "store_util.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class SecurityManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SecurityManagerTest::SetUpTestCase(void) { }

void SecurityManagerTest::TearDownTestCase(void) { }

void SecurityManagerTest::SetUp(void) { }

void SecurityManagerTest::TearDown(void) { }

/**
 * @tc.name: DBPasswordTest
 * @tc.desc: Test DBPassword function
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, DBPasswordTest, TestSize.Level1)
{
    SecurityManager::DBPassword passwd;
    EXPECT_FALSE(passwd.IsValid());
    std::vector<uint8_t> key = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
    passwd.SetValue(key.data(), key.size());
    EXPECT_TRUE(passwd.IsValid());
    EXPECT_EQ(passwd.GetSize(), 32);
    auto newKey = passwd.GetData();
    EXPECT_EQ(newKey[0], 0x00);
    passwd.Clear();
    EXPECT_FALSE(passwd.IsValid());
}

/**
 * @tc.name: KeyFilesMultiLockTest
 * @tc.desc: Test KeyFiles function
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, KeyFilesMultiLockTest, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerTest";
    std::string dbName = "test1";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto keyPath = keyFiles.GetKeyFilePath();
    EXPECT_EQ(keyPath, "/data/service/el1/public/database/SecurityManagerTest/key/test1.key");
    auto ret = keyFiles.Lock();
    EXPECT_EQ(ret, Status::SUCCESS);
    ret = keyFiles.Lock();
    EXPECT_EQ(ret, Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_EQ(ret, Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_EQ(ret, Status::SUCCESS);
}

/**
 * @tc.name: KeyFilesTest
 * @tc.desc: Test KeyFiles function
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, KeyFilesTest, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerTest";
    std::string dbName = "test2";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto keyPath = keyFiles.GetKeyFilePath();
    EXPECT_EQ(keyPath, "/data/service/el1/public/database/SecurityManagerTest/key/test2.key");
    keyFiles.Lock();
    auto blockResult = std::make_shared<OHOS::BlockData<bool>>(1, false);
    std::thread thread([dbPath, dbName, blockResult]() {
        SecurityManager::KeyFiles keyFiles(dbName, dbPath);
        keyFiles.Lock();
        keyFiles.UnLock();
        blockResult->SetValue(true);
    });
    auto beforeUnlock = blockResult->GetValue();
    EXPECT_FALSE(beforeUnlock);
    blockResult->Clear();
    keyFiles.UnLock();
    auto afterUnlock = blockResult->GetValue();
    EXPECT_TRUE(afterUnlock);
    thread.join();
}

/**
 * @tc.name: KeyFilesAutoLockTest
 * @tc.desc: Test KeyFilesAutoLock function
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, KeyFilesAutoLockTest, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerTest";
    std::string dbName = "test3";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto blockResult = std::make_shared<OHOS::BlockData<bool>>(1, false);
    {
        SecurityManager::KeyFilesAutoLock fileLock(keyFiles);
        std::thread thread([dbPath, dbName, blockResult]() {
            SecurityManager::KeyFiles keyFiles(dbName, dbPath);
            SecurityManager::KeyFilesAutoLock fileLock(keyFiles);
            blockResult->SetValue(true);
        });
        EXPECT_FALSE(blockResult->GetValue());
        blockResult->Clear();
        thread.detach();
    }
    EXPECT_TRUE(blockResult->GetValue());
}
} // namespace OHOS::Test