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

    const int OUTDATED_TIME = (24 * 500);
    const int NOT_OUTDATED_TIME = (24 * 50);

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    bool GetDate(const std::string &name, const std::string &path, std::vector<uint8_t> &date);
    bool ChangeKeyDate(const std::string &name, const std::string &path, const int duration);
    bool MoveToRekeyPath(Options options, StoreId storeId, bool &movedToRekeyPath);
    std::shared_ptr<StoreFactoryTest::DBManager> GetDBManager(const std::string &path, const AppId &appId);
    DBOption GetOption(const Options &options, const DBPassword &dbPassword);
    DBStatus ChangeKVStoreDate(const std::string &storeId, std::shared_ptr<DBManager> dbManager,
        const Options &options, DBPassword &dbPassword, int time);
    bool ChangePwdDate(int time);
    static void DeleteKVStore();
};

void StoreFactoryTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/rekey";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void StoreFactoryTest::TearDownTestCase(void)
{
    DeleteKVStore();
    (void)remove("/data/service/el1/public/database/rekey");
}

void StoreFactoryTest::SetUp(void) {}

void StoreFactoryTest::TearDown(void) {}

void StoreFactoryTest::DeleteKVStore()
{
    StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
}

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

bool StoreFactoryTest::ChangeKeyDate(const std::string &name, const std::string &path, const int duration)
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
    for (size_t i = 0; i < date.size(); ++i) {
        content[i + 1] = date[i];
    }

    auto saved = OHOS::SaveBufferToFile(keyPath, content);
    return saved;
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

std::shared_ptr<StoreFactoryTest::DBManager> StoreFactoryTest::GetDBManager(const std::string &path, const AppId &appId)
{
    std::shared_ptr<DBManager> dbManager;
    std::string fullPath = path + "/kvdb";
    StoreUtil::InitPath(fullPath);
    dbManager = std::make_shared<DBManager>(appId.appId, "default");
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

    if (options.kvStoreType == KvStoreType::SINGLE_VERSION) {
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    } else if (options.kvStoreType == KvStoreType::DEVICE_COLLABORATION) {
        dbOption.conflictResolvePolicy = DistributedDB::DEVICE_COLLABORATION;
    }

    dbOption.schema = options.schema;
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = StoreUtil::GetDBSecurity(options.securityLevel);
    return dbOption;
}

StoreFactoryTest::DBStatus StoreFactoryTest::ChangeKVStoreDate(const std::string &storeId,
    std::shared_ptr<DBManager> dbManager, const Options &options, DBPassword &dbPassword, int time)
{
    DBStatus status;
    auto dbOption = GetOption(options, dbPassword);
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

bool StoreFactoryTest::ChangePwdDate(int time)
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
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    ASSERT_EQ(ChangePwdDate(OUTDATED_TIME), true);

    std::vector<uint8_t> date;
    bool getDate = GetDate(storeId, options.baseDir, date);
    ASSERT_EQ(getDate, true);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);

    auto isDiff = std::lexicographical_compare(newDate.begin(), newDate.end(), date.begin(), date.end(),
        [](uint8_t newDateNum, uint8_t dateNum) {
            return newDateNum != dateNum;
        });
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
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    ASSERT_EQ(ChangePwdDate(NOT_OUTDATED_TIME), true);

    std::vector<uint8_t> date;
    bool getDate = GetDate(storeId, options.baseDir, date);
    ASSERT_EQ(getDate, true);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);
    auto isDiff = std::lexicographical_compare(newDate.begin(), newDate.end(), date.begin(), date.end(),
        [](uint8_t newDateNum, uint8_t dateNum) {
            return newDateNum != dateNum;
        });
    ASSERT_EQ(isDiff, false);
}

/**
* @tc.name: RekeyInterrupted0
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
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    auto isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, true);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);
    auto isDiff = std::lexicographical_compare(newDate.begin(), newDate.end(), date.begin(), date.end(),
        [](uint8_t newDateNum, uint8_t dateNum) {
            return newDateNum != dateNum;
        });
    ASSERT_EQ(isDiff, false);
}

/**
* @tc.name: RekeyInterrupted1
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
    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);

    isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, true);

    std::vector<uint8_t> newDate;
    getDate = GetDate(storeId, options.baseDir, newDate);
    ASSERT_EQ(getDate, true);
    auto isDiff = std::lexicographical_compare(newDate.begin(), newDate.end(), date.begin(), date.end(),
        [](uint8_t newDateNum, uint8_t dateNum) {
            return newDateNum != dateNum;
        });
    ASSERT_EQ(isDiff, false);
}

/**
* @tc.name: RekeyNoPwdFile
* @tc.desc: mock the situation that open store after rekey was interrupted last time,
*           which caused key file not changed but rekey key file exist.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Cui Renjie
*/
HWTEST_F(StoreFactoryTest, RekeyNoPwdFile, TestSize.Level1)
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
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key";
    StoreUtil::Remove(keyFileName);

    auto isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, false);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, CRYPT_ERROR);
}
