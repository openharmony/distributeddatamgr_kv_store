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
#include "hks_api.h"
#include "hks_param.h"
#include "security_manager.h"
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
static constexpr const char *ROOT_KEY_ALIAS = "distributeddb_client_root_key";
static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata_client";

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
    bool ChangeKeyDate(const std::string &name, const std::string &path, int duration, const std::vector<uint8_t> &key);
    bool MoveToRekeyPath(Options options, StoreId storeId);
    std::shared_ptr<StoreFactoryTest::DBManager> GetDBManager(const std::string &path, const AppId &appId);
    DBOption GetOption(const Options &options, const DBPassword &dbPassword);
    DBStatus ChangeKVStoreDate(const std::string &storeId, std::shared_ptr<DBManager> dbManager, const Options &options,
        DBPassword &dbPassword, int time);
    bool ModifyDate(int time);
    bool Encrypt(const std::vector<uint8_t> &key, SecurityManager::SecurityContent &content);
    bool Decrypt(const SecurityManager::SecurityContent &content, std::vector<uint8_t> &key);
    static void DeleteKVStore();

    static std::vector<uint8_t> vecRootKeyAlias_;
    static std::vector<uint8_t> vecAad_;
};

std::vector<uint8_t> StoreFactoryTest::vecRootKeyAlias_ =
    std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
std::vector<uint8_t> StoreFactoryTest::vecAad_ =
    std::vector<uint8_t>(HKS_BLOB_TYPE_AAD, HKS_BLOB_TYPE_AAD + strlen(HKS_BLOB_TYPE_AAD));

void StoreFactoryTest::SetUpTestCase(void) { }

void StoreFactoryTest::TearDownTestCase(void) { }

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

bool StoreFactoryTest::Encrypt(const std::vector<uint8_t> &key, SecurityManager::SecurityContent &content)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return false;
    }

    uint8_t nonceValue[SecurityManager::SecurityContent::NONCE_SIZE] = {0};
    struct HksBlob blobNonce = { .size = SecurityManager::SecurityContent::NONCE_SIZE, .data = nonceValue };
    ret = HksGenerateRandom(nullptr, &blobNonce);
    if (ret != HKS_SUCCESS) {
        return false;
    }

    struct HksBlob blobAad = { uint32_t(vecAad_.size()), vecAad_.data() };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksBlob plainKey = { uint32_t(key.size()), const_cast<uint8_t *>(key.data()) };
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };
    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return false;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return false;
    }

    uint8_t cipherBuf[256] = { 0 };
    struct HksBlob cipherText = { sizeof(cipherBuf), cipherBuf };
    ret = HksEncrypt(&rootKeyName, params, &plainKey, &cipherText);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return false;
    }
    std::vector<uint8_t> nonceContent(blobNonce.data, blobNonce.data + blobNonce.size);
    content.nonceValue = nonceContent;
    std::vector<uint8_t> encryptValue(cipherText.data, cipherText.data + cipherText.size);
    content.encryptValue = encryptValue;
    std::fill(cipherBuf, cipherBuf + sizeof(cipherBuf), 0);
    return true;
}

bool StoreFactoryTest::Decrypt(const SecurityManager::SecurityContent &content, std::vector<uint8_t> &key)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return false;
    }

    struct HksBlob blobNonce = { .size = uint32_t(content.nonceValue.size()),
        .data = const_cast<uint8_t*>(&(content.nonceValue[0])) };
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), &(vecAad_[0]) };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), &(vecRootKeyAlias_[0]) };
    struct HksBlob encryptedKeyBlob = { uint32_t(content.encryptValue.size()),
        const_cast<uint8_t *>(content.encryptValue.data()) };
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };
    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return false;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return false;
    }

    uint8_t plainBuf[256] = { 0 };
    struct HksBlob plainKeyBlob = { sizeof(plainBuf), plainBuf };
    ret = HksDecrypt(&rootKeyName, params, &encryptedKeyBlob, &plainKeyBlob);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return false;
    }

    key.assign(plainKeyBlob.data, plainKeyBlob.data + plainKeyBlob.size);
    std::fill(plainBuf, plainBuf + sizeof(plainBuf), 0);
    return true;
}

std::chrono::system_clock::time_point StoreFactoryTest::GetDate(const std::string &name, const std::string &path)
{
    std::chrono::system_clock::time_point timePoint;
    auto keyPath = path + "/key/" + name + ".key_v1";
    if (!OHOS::FileExists(keyPath)) {
        return timePoint;
    }
    std::vector<char> content;
    auto loaded = OHOS::LoadBufferFromFile(keyPath, content);
    if (!loaded) {
        return timePoint;
    }
    SecurityManager::SecurityContent securityContent;
    size_t offset = SecurityManager::SecurityContent::MAGIC_NUM;
    securityContent.nonceValue.assign(content.begin() + offset,
        content.begin() + offset + SecurityManager::SecurityContent::NONCE_SIZE);
    offset += SecurityManager::SecurityContent::NONCE_SIZE;
    securityContent.encryptValue.assign(content.begin() + offset, content.end());
    std::vector<uint8_t> fullKey;
    if (!Decrypt(securityContent, fullKey)) {
        return timePoint;
    }
    offset = 0;
    securityContent.version = fullKey[offset++];
    securityContent.time.assign(fullKey.begin() + offset,
        fullKey.begin() + offset + (sizeof(time_t) / sizeof(uint8_t)));
    timePoint = std::chrono::system_clock::from_time_t(
        *reinterpret_cast<time_t *>(const_cast<uint8_t *>(securityContent.time.data())));
    return timePoint;
}

bool StoreFactoryTest::ChangeKeyDate(const std::string &name, const std::string &path, int duration,
    const std::vector<uint8_t> &key)
{
    auto keyPath = path + "/key/" + name + ".key_v1";
    if (!OHOS::FileExists(keyPath)) {
        return false;
    }
    SecurityManager::SecurityContent securityContent;
    securityContent.version = SecurityManager::SecurityContent::CURRENT_VERSION;
    auto time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::system_clock::now() - std::chrono::hours(duration));
    securityContent.time = { reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time) };
    std::vector<uint8_t> keyContent;
    keyContent.push_back(securityContent.version);
    keyContent.insert(keyContent.end(), securityContent.time.begin(), securityContent.time.end());
    keyContent.insert(keyContent.end(), key.begin(), key.end());
    if (!Encrypt(keyContent, securityContent)) {
        return false;
    }
    std::vector<char> content;
    for (size_t index = 0; index < SecurityManager::SecurityContent::MAGIC_NUM; ++index) {
        content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    }
    content.insert(content.end(), securityContent.nonceValue.begin(), securityContent.nonceValue.end());
    content.insert(content.end(), securityContent.encryptValue.begin(), securityContent.encryptValue.end());
    return OHOS::SaveBufferToFile(keyPath, content);
}

bool StoreFactoryTest::MoveToRekeyPath(Options options, StoreId storeId)
{
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key_v1";
    std::string rekeyFileName = options.baseDir + "/rekey/key/" + storeId.storeId + ".new.key_v1";
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

    dbOption.conflictResolvePolicy = options.kvStoreType == KvStoreType::SINGLE_VERSION ?
        DistributedDB::LAST_WIN :
        DistributedDB::DEVICE_COLLABORATION;

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
    dbPassword = SecurityManager::GetInstance().GetDBPassword(storeId, options.baseDir, false);
    std::vector<uint8_t> key(dbPassword.GetData(), dbPassword.GetData() + dbPassword.GetSize());
    if (!ChangeKeyDate(storeId, options.baseDir, time, key)) {
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
    options.autoRekey = true;
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
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key_v1";
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

    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key_v1";
    std::string mockKeyFileName = options.baseDir + "/key/" + newStoreId.storeId + ".key_v1";
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
    options.autoRekey = false;
    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    status = StoreManager::GetInstance().CloseKVStore(appId, storeId);
    ASSERT_EQ(status, SUCCESS);
    std::string keyFileName = options.baseDir + "/key/" + storeId.storeId + ".key_v1";
    StoreUtil::Remove(keyFileName);

    auto isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, false);

    StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);

    isKeyExist = StoreUtil::IsFileExist(keyFileName);
    ASSERT_EQ(isKeyExist, true);
}
} // namespace