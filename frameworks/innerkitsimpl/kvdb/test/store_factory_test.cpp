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

#include <cerrno>
#include <cstdio>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <vector>

#include "backup_manager.h"
#include "file_ex.h"
#include "store_manager.h"
#include "store_util.h"
#include "sys/stat.h"
#include "types.h"
namespace {
using namespace testing::ext;
using namespace OHOS::DistributedKv;

static StoreId storeId = { "single_test" };
static AppId appId = { "rekey" };
static Options options = {
    .encrypt = true,
    .securityLevel = S1,
    .area = EL1,
    .kvStoreType = SINGLE_VERSION,
    .baseDir = "/data/service/el1/public/database/rekey",
};

class StoreFactoryTest : public testing::Test {
public:
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using DBOption = DistributedDB::KvStoreNbDelegate::Option;
    using DBPassword = SecurityManager::DBPassword;
    using DBStatus = DistributedDB::DBStatus;

    static constexpr int OUTDATED_TIME = (24 * 500);
    static constexpr int NOT_OUTDATED_TIME = (24 * 50);

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::chrono::system_clock::time_point GetDate(const std::string &name, const std::string &path);
    bool ChangeKeyDate(const std::string &name, const std::string &path, int duration);
    bool MoveToRekeyPath(Options options, StoreId storeId);
    std::shared_ptr<StoreFactoryTest::DBManager> GetDBManager(const std::string &path, const AppId &appId);
    DBOption GetOption(const Options &options, const DBPassword &dbPassword);
    DBStatus ChangeKVStoreDate(const std::string &storeId, std::shared_ptr<DBManager> dbManager,
        const Options &options, DBPassword &dbPassword, int time);
    bool ModifyDate(int time);
    static void DeleteKVStore();
};

void StoreFactoryTest::SetUpTestCase(void)
{
}

void StoreFactoryTest::TearDownTestCase(void)
{
}

void StoreFactoryTest::SetUp(void)
{
    std::string baseDir = "/data/service/el1/public/database/rekey";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void StoreFactoryTest::TearDown(void)
{
    DeleteKVStore();
    (void)remove("/data/service/el1/public/database/rekey");
}

void StoreFactoryTest::DeleteKVStore()
{
    StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
}

std::chrono::system_clock::time_point StoreFactoryTest::GetDate(const std::string &name, const std::string &path)
{
    std::chrono::system_clock::time_point timePoint;
    auto keyPath = path + "/key/" + name + ".key";
    if (!OHOS::FileExists(keyPath)) {
        return timePoint;
    }

    std::vector<char> content;
    auto loaded = OHOS::LoadBufferFromFile(keyPath, content);
    if (!loaded) {
        return timePoint;
    }
    constexpr uint32_t DATE_FILE_OFFSET = 1;
    constexpr uint32_t DATE_FILE_LENGTH = sizeof(time_t) / sizeof(uint8_t);
    std::vector<uint8_t> date;
    date.assign(content.begin() + DATE_FILE_OFFSET, content.begin() + DATE_FILE_LENGTH + DATE_FILE_OFFSET);
    timePoint = std::chrono::system_clock::from_time_t(*reinterpret_cast<time_t *>(const_cast<uint8_t *>(&date[0])));
    return timePoint;
}

bool StoreFactoryTest::ChangeKeyDate(const std::string &name, const std::string &path, int duration)
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
    auto time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::system_clock::now() - std::chrono::hours(duration));
    std::vector<char> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::copy(date.begin(), date.end(), ++content.begin());

    auto saved = OHOS::SaveBufferToFile(keyPath, content);
    return saved;
}

bool StoreFactoryTest::MoveToRekeyPath(Options options, StoreId storeId)
{
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    std::string rekeyFileName = options.baseDir + "/rekey/key/" + storeId.storeId + ".new.key";
    bool result = StoreUtil::Rename(keyFileName, rekeyFileName);
    if (!result) {
        return false;
    }
    result = StoreUtil::Remove(keyFileName);
    return result;
}

std::shared_ptr<StoreFactoryTest::DBManager> StoreFactoryTest::GetDBManager(const std::string &path, const AppId &appId)
{
    std::string fullPath = path + "/kvdb";
    StoreUtil::InitPath(fullPath);
    std::shared_ptr<DBManager> dbManager = std::make_shared<DBManager>(appId.appId, "default");
    dbManager->SetKvStoreConfig({ fullPath });
    BackupManager::GetInstance().Init(path);
    return dbManager;
}

StoreFactoryTest::DBOption StoreFactoryTest::GetOption(const Options &options, const DBPassword &dbPassword)
{
    DBOption dbOption;
    dbOption.syncDualTupleMode = true; // tuple of (appid+storeid)
    dbOption.createIfNecessary = options.createIfMissing;
    dbOption.isNeedRmCorruptedDb = options.rebuild;
    dbOption.isMemoryDb = (!options.persistent);
    dbOption.isEncryptedDb = options.encrypt;
    if (options.encrypt) {
        dbOption.cipher = DistributedDB::CipherType::AES_256_GCM;
        dbOption.passwd = dbPassword.password;
    }

    dbOption.conflictResolvePolicy = options.kvStoreType == KvStoreType::SINGLE_VERSION
                                         ? DistributedDB::LAST_WIN
                                         : DistributedDB::DEVICE_COLLABORATION;

    dbOption.schema = options.schema;
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = StoreUtil::GetDBSecurity(options.securityLevel);
    return dbOption;
}

StoreFactoryTest::DBStatus StoreFactoryTest::ChangeKVStoreDate(const std::string &storeId,
    std::shared_ptr<DBManager> dbManager, const Options &options, DBPassword &dbPassword, int time)
{
    DBStatus status;
    const auto dbOption = GetOption(options, dbPassword);
    DBStore *store = nullptr;
    dbManager->GetKvStore(storeId, dbOption, [&status, &store](auto dbStatus, auto *dbStore) {
        status = dbStatus;
        store = dbStore;
    });
    if (!ChangeKeyDate(storeId, options.baseDir, time)) {
        std::cout << "failed" << std::endl;
    }
    dbPassword = SecurityManager::GetInstance().GetDBPassword(storeId, options.baseDir, false);
    auto dbStatus = store->Rekey(dbPassword.password);
    dbManager->CloseKvStore(store);
    return dbStatus;
}

bool StoreFactoryTest::ModifyDate(int time)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(storeId, options.baseDir, false);
    auto dbManager = GetDBManager(options.baseDir, appId);
    auto dbstatus = ChangeKVStoreDate(storeId, dbManager, options, dbPassword, time);
    return StoreUtil::ConvertStatus(dbstatus) == SUCCESS;
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
    Status status = DB_ERROR;
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    ASSERT_TRUE(ModifyDate(OUTDATED_TIME));

    auto oldKeyTime = GetDate(storeId, options.baseDir);
    ASSERT_FALSE(std::chrono::system_clock::now() - oldKeyTime < std::chrono::seconds(2));

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    auto newKeyTime = GetDate(storeId, options.baseDir);
    ASSERT_TRUE(std::chrono::system_clock::now() - newKeyTime < std::chrono::seconds(2));
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
    Status status = DB_ERROR;
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    ASSERT_TRUE(ModifyDate(NOT_OUTDATED_TIME));
    auto oldKeyTime = GetDate(storeId, options.baseDir);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    auto newKeyTime = GetDate(storeId, options.baseDir);
    ASSERT_EQ(oldKeyTime, newKeyTime);
}

/**
* @tc.name: RekeyInterrupted0
* @tc.desc: mock the situation that open store after rekey was interrupted last time,
*           which caused key file lost but rekey key file exist.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, RekeyInterruptedWhileChangeKeyFile, TestSize.Level1)
{
    Status status = DB_ERROR;
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);
    auto oldKeyTime = GetDate(storeId, options.baseDir);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_TRUE(MoveToRekeyPath(options, storeId));

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    auto isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_TRUE(isKeyExist);

    auto newKeyTime = GetDate(storeId, options.baseDir);
    ASSERT_TRUE(newKeyTime - oldKeyTime < std::chrono::seconds(2));
}

/**
* @tc.name: RekeyInterrupted1
* @tc.desc: mock the situation that open store after rekey was interrupted last time,
*           which caused key file not changed but rekey key file exist.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, RekeyInterruptedBeforeChangeKeyFile, TestSize.Level1)
{
    Status status = DB_ERROR;
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);
    auto oldKeyTime = GetDate(storeId, options.baseDir);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(MoveToRekeyPath(options, storeId), true);

    StoreId newStoreId = { "newStore" };
    StoreManager::GetInstance().GetKVStore(appId, newStoreId, options, status);

    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    std::string mockKeyFileName = options.baseDir + "/key/" + newStoreId.storeId + ".key";
    StoreUtil::Rename(mockKeyFileName, keyFileName);
    StoreUtil::Remove(mockKeyFileName);
    auto isKeyExist = StoreUtil::IsFileExist(mockKeyFileName);
    ASSERT_FALSE(isKeyExist);
    isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_TRUE(isKeyExist);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_TRUE(isKeyExist);

    auto newKeyTime = GetDate(storeId, options.baseDir);
    ASSERT_TRUE(newKeyTime - oldKeyTime < std::chrono::seconds(2));
}

/**
* @tc.name: RekeyNoPwdFile
* @tc.desc: try to open kvstore and execute RekeyRecover() without key and rekey key files.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, RekeyNoPwdFile, TestSize.Level1)
{
    Status status = DB_ERROR;
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    StoreUtil::Remove(keyFileName);

    auto isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, false);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, CRYPT_ERROR);
}
}