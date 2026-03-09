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

#include "security_manager.h"

#include <gtest/gtest.h>

#include <chrono>

#include "block_data.h"
#include "file_ex.h"
#include "hks_api.h"
#include "hks_param.h"
#include "store_util.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedKv;

static constexpr int32_t KEY_SIZE = 32;
static constexpr int32_t NONCE_SIZE = 12;
static constexpr int32_t LOOP_NUM = 2;
static constexpr const char *STORE_NAME = "test_store";
static constexpr const char *BASE_DIR = "/data/service/el1/public/database/SecurityManagerNew";
static constexpr const char *KEY_DIR = "/data/service/el1/public/database/SecurityManagerNew/key";
static constexpr const char *KEY_FULL_PATH = "/data/service/el1/public/database/SecurityManagerNew/key/test_store.key";
static constexpr const char *KEY_FULL_PATH_V1 =
    "/data/service/el1/public/database/SecurityManagerNew/key/test_store.key_v1";
static constexpr const char *LOCK_FULL_PATH =
    "/data/service/el1/public/database/SecurityManagerNew/key/test_store.key_lock";
static constexpr const char *ROOT_KEY_ALIAS = "distributeddb_client_root_key";
static constexpr const char *HKS_BLOB_TYPE_NONCE = "Z5s0Bo571KoqwIi6";
static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata_client";

class SecurityManagerNew : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static std::vector<uint8_t> Random(int32_t length);
    static std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &key, const std::vector<uint8_t> &nonce);
    static bool SaveKeyToOldFile(const std::vector<uint8_t> &key);
    static void GenerateRootKey();
    static void DeleteRootKey();

    static std::vector<uint8_t> vecRootKeyAlias_;
    static std::vector<uint8_t> vecNonce_;
    static std::vector<uint8_t> vecAad_;
};

std::vector<uint8_t> SecurityManagerNew::vecRootKeyAlias_ =
    std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
std::vector<uint8_t> SecurityManagerNew::vecNonce_ =
    std::vector<uint8_t>(HKS_BLOB_TYPE_NONCE, HKS_BLOB_TYPE_NONCE + strlen(HKS_BLOB_TYPE_NONCE));
std::vector<uint8_t> SecurityManagerNew::vecAad_ =
    std::vector<uint8_t>(HKS_BLOB_TYPE_AAD, HKS_BLOB_TYPE_AAD + strlen(HKS_BLOB_TYPE_AAD));

void SecurityManagerNew::SetUpTestCase(void)
{
    mkdir(BASE_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    GenerateRootKey();
}

void SecurityManagerNew::TearDownTestCase(void)
{
    DeleteRootKey();
    (void)remove(BASE_DIR);
}

void SecurityManagerNew::SetUp()
{
    mkdir(KEY_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SecurityManagerNew::TearDown()
{
    (void)remove(LOCK_FULL_PATH);
    (void)remove(KEY_FULL_PATH);
    (void)remove(KEY_FULL_PATH_V1);
    (void)remove(KEY_DIR);
}

std::vector<uint8_t> SecurityManagerNew::Random(int32_t length)
{
    std::vector<uint8_t> value(length, 0);
    struct HksBlob blobValue = { .size = length, .data = &(value[0]) };
    auto ret = HksGenerateRandom(nullptr, &blobValue);
    if (ret != HKS_SUCCESS) {
        return {};
    }
    return value;
}

std::vector<uint8_t> SecurityManagerNew::Encrypt(const std::vector<uint8_t> &key, const std::vector<uint8_t> &nonce)
{
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), vecAad_.data() };
    struct HksBlob blobNonce = { uint32_t(nonce.size()), const_cast<uint8_t *>(nonce.data()) };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksBlob plainKey = { uint32_t(key.size()), const_cast<uint8_t *>(key.data()) };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return {};
    }
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
        return {};
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return {};
    }
    uint8_t cipherBuf[256] = { 0 };
    struct HksBlob cipherText = { sizeof(cipherBuf), cipherBuf };
    ret = HksEncrypt(&rootKeyName, params, &plainKey, &cipherText);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return {};
    }
    std::vector<uint8_t> encryptedKey(cipherText.data, cipherText.data + cipherText.size);
    std::fill(cipherBuf, cipherBuf + sizeof(cipherBuf), 0);
    return encryptedKey;
}

bool SecurityManagerNew::SaveKeyToOldFile(const std::vector<uint8_t> &key)
{
    auto encryptKey = Encrypt(key, vecNonce_);
    if (encryptKey.empty()) {
        return false;
    }
    std::vector<char> content;
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    content.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    content.insert(content.end(), date.begin(), date.end());
    content.insert(content.end(), encryptKey.begin(), encryptKey.end());
    return SaveBufferToFile(KEY_FULL_PATH, content);
}

void SecurityManagerNew::GenerateRootKey()
{
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return;
    }
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };

    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return;
    }
    ret = HksGenerateKey(&rootKeyName, params, nullptr);
    HksFreeParamSet(&params);
}

void SecurityManagerNew::DeleteRootKey()
{
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        return;
    }
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };
    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return;
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return;
    }
    ret = HksDeleteKey(&rootKeyName, params);
    HksFreeParamSet(&params);
}

/**
 * @tc.name: GetDBPasswordNewTest001
 * @tc.desc: get password with first create and needCreate is false
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest001, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: GetDBPasswordNewTest002
 * @tc.desc: get password with key file exist and the key file is empty
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest002, TestSize.Level0)
{
    std::vector<char> content;
    auto result = SaveBufferToFile(KEY_FULL_PATH, content);
    ASSERT_TRUE(result);

    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest003
 * @tc.desc: get password with old key file exist and the old key file is invalid
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest003, TestSize.Level0)
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent1;
    invalidContent1.push_back(char(sizeof(time_t) / sizeof(uint8_t)));
    invalidContent1.insert(invalidContent1.end(), date.begin(), date.end());
    auto result = SaveBufferToFile(KEY_FULL_PATH, invalidContent1);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto invalidKey = Random(KEY_SIZE);
    ASSERT_FALSE(invalidKey.empty());
    std::vector<char> invalidContent2;
    invalidContent2.push_back(char((sizeof(time_t) / sizeof(uint8_t))));
    invalidContent2.insert(invalidContent2.end(), date.begin(), date.end());
    invalidContent2.insert(invalidContent2.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent2);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<char> invalidContent3;
    invalidContent3.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    invalidContent3.insert(invalidContent3.end(), date.begin(), date.end());
    invalidContent3.insert(invalidContent3.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent3);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest004
 * @tc.desc: get password with new key file exist and the new key file is invalid
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest004, TestSize.Level0)
{
    std::vector<char> content;

    content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    auto result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    for (size_t index = 0; index < SecurityManager::SecurityContent::MAGIC_NUM - 1; ++index) {
        content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    }
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto nonce = Random(NONCE_SIZE);
    ASSERT_FALSE(nonce.empty());
    content.insert(content.end(), nonce.begin(), nonce.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent = content;
    invalidContent.insert(invalidContent.end(), date.begin(), date.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<uint8_t> keyContent;
    keyContent.push_back(SecurityManager::SecurityContent::CURRENT_VERSION);
    keyContent.insert(keyContent.end(), date.begin(), date.end());
    auto encryptValue = Encrypt(keyContent, nonce);
    ASSERT_FALSE(encryptValue.empty());
    invalidContent = content;
    invalidContent.insert(invalidContent.end(), encryptValue.begin(), encryptValue.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest005
 * @tc.desc: get password with first create and needCreate is true
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest005, TestSize.Level0)
{
    auto dbPassword1 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword1.IsValid());

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_TRUE(dbPassword2.IsValid());

    ASSERT_TRUE(dbPassword2.GetSize() == dbPassword1.GetSize());

    std::vector<uint8_t> key1(dbPassword1.GetData(), dbPassword1.GetData() + dbPassword1.GetSize());
    std::vector<uint8_t> key2(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(key1.size() == key2.size());

    for (auto index = 0; index < key1.size(); ++index) {
        ASSERT_TRUE(key1[index] == key2[index]);
    }

    key1.assign(key1.size(), 0);
    key2.assign(key2.size(), 0);
    dbPassword1.Clear();
    dbPassword2.Clear();
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest005_Duplicate
 * @tc.desc: get password with first create and needCreate is true (duplicate test)
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest005_Duplicate, TestSize.Level0)
{
    auto dbPassword1 = SecurityManager::GetInstance().GetDBPassword(std::string(STORE_NAME) + "_dup", BASE_DIR, true);
    ASSERT_TRUE(dbPassword1.IsValid());

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(std::string(STORE_NAME) + "_dup", BASE_DIR, false);
    ASSERT_TRUE(dbPassword2.IsValid());

    ASSERT_TRUE(dbPassword2.GetSize() == dbPassword1.GetSize());

    std::vector<uint8_t> key1(dbPassword1.GetData(), dbPassword1.GetData() + dbPassword1.GetSize());
    std::vector<uint8_t> key2(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(key1.size() == key2.size());

    for (auto index = 0; index < key1.size(); ++index) {
        ASSERT_TRUE(key1[index] == key2[index]);
    }

    key1.assign(key1.size(), 0);
    key2.assign(key2.size(), 0);
    dbPassword1.Clear();
    dbPassword2.Clear();
    SecurityManager::GetInstance().DelDBPassword(std::string(STORE_NAME) + "_dup", BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest006
 * @tc.desc: get password with first create and needCreate is true
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest006, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());
    auto result = SaveKeyToOldFile(key);
    ASSERT_TRUE(result);

    for (auto loop = 0; loop < LOOP_NUM; ++loop) {
        auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
        ASSERT_TRUE(dbPassword.IsValid());
        ASSERT_TRUE(dbPassword.GetSize() == key.size());
        std::vector<uint8_t> password(dbPassword.GetData(), dbPassword.GetData() + dbPassword.GetSize());
        ASSERT_TRUE(password.size() == key.size());
        for (auto index = 0; index < key.size(); ++index) {
            ASSERT_TRUE(password[index] == key[index]);
        }
        password.assign(password.size(), 0);
        dbPassword.Clear();
    }

    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: SaveDBPasswordNewTest001
 * @tc.desc: save password
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, SaveDBPasswordNewTest001, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());

    DistributedDB::CipherPassword dbPassword1;
    dbPassword1.SetValue(key.data(), key.size());
    ASSERT_TRUE(dbPassword1.GetSize() == key.size());
    auto result = SecurityManager::GetInstance().SaveDBPassword(STORE_NAME, BASE_DIR, dbPassword1);
    ASSERT_TRUE(result);
    dbPassword1.Clear();

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword2.IsValid());
    ASSERT_TRUE(dbPassword2.GetSize() == key.size());
    std::vector<uint8_t> password(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(password.size() == key.size());
    for (auto index = 0; index < key.size(); ++index) {
        ASSERT_TRUE(password[index] == key[index]);
    }
    password.assign(password.size(), 0);
    dbPassword2.Clear();
    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: DelDBPasswordNewTest001
 * @tc.desc: delete password
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, DelDBPasswordNewTest001, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword.IsValid());
    dbPassword.Clear();

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: KeyFilesMultiLockNewTest
 * @tc.desc: Test KeyFiles function
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesMultiLockNewTest, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test1";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
}

/**
 * @tc.name: KeyFilesNewTest
 * @tc.desc: Test KeyFiles function
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesNewTest, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test2";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
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
 * @tc.name: KeyFilesAutoLockNewTest
 * @tc.desc: Test KeyFilesAutoLock function
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesAutoLockNewTest, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
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

/**
 * @tc.name: GetDBPasswordNewTest007
 * @tc.desc: get password with first create and needCreate is false duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest007, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: GetDBPasswordNewTest008
 * @tc.desc: get password with key file exist and the key file is empty duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest008, TestSize.Level0)
{
    std::vector<char> content;
    auto result = SaveBufferToFile(KEY_FULL_PATH, content);
    ASSERT_TRUE(result);

    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest008_Duplicate
 * @tc.desc: get password with key file exist and the key file is empty (duplicate test)
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest008_Duplicate, TestSize.Level0)
{
    std::vector<char> content;
    auto result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);

    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(std::string(STORE_NAME) + "_dup2", BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(std::string(STORE_NAME) + "_dup2", BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(std::string(STORE_NAME) + "_dup2", BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest009
 * @tc.desc: get password with old key file exist and the old key file is invalid duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest009, TestSize.Level0)
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent1;
    invalidContent1.push_back(char(sizeof(time_t) / sizeof(uint8_t)));
    invalidContent1.insert(invalidContent1.end(), date.begin(), date.end());
    auto result = SaveBufferToFile(KEY_FULL_PATH, invalidContent1);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto invalidKey = Random(KEY_SIZE);
    ASSERT_FALSE(invalidKey.empty());
    std::vector<char> invalidContent2;
    invalidContent2.push_back(char((sizeof(time_t) / sizeof(uint8_t))));
    invalidContent2.insert(invalidContent2.end(), date.begin(), date.end());
    invalidContent2.insert(invalidContent2.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent2);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<char> invalidContent3;
    invalidContent3.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    invalidContent3.insert(invalidContent3.end(), date.begin(), date.end());
    invalidContent3.insert(invalidContent3.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent3);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest010
 * @tc.desc: get password with new key file exist and the new key file is invalid duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest010, TestSize.Level0)
{
    std::vector<char> content;

    content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    auto result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    for (size_t index = 0; index < SecurityManager::SecurityContent::MAGIC_NUM - 1; ++index) {
        content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    }
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto nonce = Random(NONCE_SIZE);
    ASSERT_FALSE(nonce.empty());
    content.insert(content.end(), nonce.begin(), nonce.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent = content;
    invalidContent.insert(invalidContent.end(), date.begin(), date.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<uint8_t> keyContent;
    keyContent.push_back(SecurityManager::SecurityContent::CURRENT_VERSION);
    keyContent.insert(keyContent.end(), date.begin(), date.end());
    auto encryptValue = Encrypt(keyContent, nonce);
    ASSERT_FALSE(encryptValue.empty());
    invalidContent = content;
    invalidContent.insert(invalidContent.end(), encryptValue.begin(), encryptValue.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest011
 * @tc.desc: get password with first create and needCreate is true duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest011, TestSize.Level0)
{
    auto dbPassword1 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword1.IsValid());

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_TRUE(dbPassword2.IsValid());

    ASSERT_TRUE(dbPassword2.GetSize() == dbPassword1.GetSize());

    std::vector<uint8_t> key1(dbPassword1.GetData(), dbPassword1.GetData() + dbPassword1.GetSize());
    std::vector<uint8_t> key2(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(key1.size() == key2.size());

    for (auto index = 0; index < key1.size(); ++index) {
        ASSERT_TRUE(key1[index] == key2[index]);
    }

    key1.assign(key1.size(), 0);
    key2.assign(key2.size(), 0);
    dbPassword1.Clear();
    dbPassword2.Clear();
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest012
 * @tc.desc: get password with old key file exit and update duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest012, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());
    auto result = SaveKeyToOldFile(key);
    ASSERT_TRUE(result);

    for (auto loop = 0; loop < LOOP_NUM; ++loop) {
        auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
        ASSERT_TRUE(dbPassword.IsValid());
        ASSERT_TRUE(dbPassword.GetSize() == key.size());
        std::vector<uint8_t> password(dbPassword.GetData(), dbPassword.GetData() + dbPassword.GetSize());
        ASSERT_TRUE(password.size() == key.size());
        for (auto index = 0; index < key.size(); ++index) {
            ASSERT_TRUE(password[index] == key[index]);
        }
        password.assign(password.size(), 0);
        dbPassword.Clear();
    }

    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: SaveDBPasswordNewTest002
 * @tc.desc: save password duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, SaveDBPasswordNewTest002, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());

    DistributedDB::CipherPassword dbPassword1;
    dbPassword1.SetValue(key.data(), key.size());
    ASSERT_TRUE(dbPassword1.GetSize() == key.size());
    auto result = SecurityManager::GetInstance().SaveDBPassword(STORE_NAME, BASE_DIR, dbPassword1);
    ASSERT_TRUE(result);
    dbPassword1.Clear();

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword2.IsValid());
    ASSERT_TRUE(dbPassword2.GetSize() == key.size());
    std::vector<uint8_t> password(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(password.size() == key.size());
    for (auto index = 0; index < key.size(); ++index) {
        ASSERT_TRUE(password[index] == key[index]);
    }
    password.assign(password.size(), 0);
    dbPassword2.Clear();
    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: DelDBPasswordNewTest002
 * @tc.desc: delete password duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, DelDBPasswordNewTest002, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword.IsValid());
    dbPassword.Clear();

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: KeyFilesMultiLockNewTest002
 * @tc.desc: Test KeyFiles function duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesMultiLockNewTest002, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test4";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
}

/**
 * @tc.name: KeyFilesNewTest002
 * @tc.desc: Test KeyFiles function duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesNewTest002, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test5";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
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
 * @tc.name: KeyFilesAutoLockNewTest002
 * @tc.desc: Test KeyFilesAutoLock function duplicate
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesAutoLockNewTest002, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test6";
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

/**
 * @tc.name: GetDBPasswordNewTest013
 * @tc.desc: get password with first create and needCreate is false three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest013, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: GetDBPasswordNewTest014
 * @tc.desc: get password with key file exist and the key file is empty three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest014, TestSize.Level0)
{
    std::vector<char> content;
    auto result = SaveBufferToFile(KEY_FULL_PATH, content);
    ASSERT_TRUE(result);

    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest015
 * @tc.desc: get password with old key file exist and the old key file is invalid three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest015, TestSize.Level0)
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent1;
    invalidContent1.push_back(char(sizeof(time_t) / sizeof(uint8_t)));
    invalidContent1.insert(invalidContent1.end(), date.begin(), date.end());
    auto result = SaveBufferToFile(KEY_FULL_PATH, invalidContent1);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto invalidKey = Random(KEY_SIZE);
    ASSERT_FALSE(invalidKey.empty());
    std::vector<char> invalidContent2;
    invalidContent2.push_back(char((sizeof(time_t) / sizeof(uint8_t))));
    invalidContent2.insert(invalidContent2.end(), date.begin(), date.end());
    invalidContent2.insert(invalidContent2.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent2);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<char> invalidContent3;
    invalidContent3.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    invalidContent3.insert(invalidContent3.end(), date.begin(), date.end());
    invalidContent3.insert(invalidContent3.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent3);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest016
 * @tc.desc: get password with new key file exist and the new key file is invalid three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest016, TestSize.Level0)
{
    std::vector<char> content;

    content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    auto result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    for (size_t index = 0; index < SecurityManager::SecurityContent::MAGIC_NUM - 1; ++index) {
        content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    }
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto nonce = Random(NONCE_SIZE);
    ASSERT_FALSE(nonce.empty());
    content.insert(content.end(), nonce.begin(), nonce.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent = content;
    invalidContent.insert(invalidContent.end(), date.begin(), date.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<uint8_t> keyContent;
    keyContent.push_back(SecurityManager::SecurityContent::CURRENT_VERSION);
    keyContent.insert(keyContent.end(), date.begin(), date.end());
    auto encryptValue = Encrypt(keyContent, nonce);
    ASSERT_FALSE(encryptValue.empty());
    invalidContent = content;
    invalidContent.insert(invalidContent.end(), encryptValue.begin(), encryptValue.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest017
 * @tc.desc: get password with first create and needCreate is true three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest017, TestSize.Level0)
{
    auto dbPassword1 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword1.IsValid());

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_TRUE(dbPassword2.IsValid());

    ASSERT_TRUE(dbPassword2.GetSize() == dbPassword1.GetSize());

    std::vector<uint8_t> key1(dbPassword1.GetData(), dbPassword1.GetData() + dbPassword1.GetSize());
    std::vector<uint8_t> key2(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(key1.size() == key2.size());

    for (auto index = 0; index < key1.size(); ++index) {
        ASSERT_TRUE(key1[index] == key2[index]);
    }

    key1.assign(key1.size(), 0);
    key2.assign(key2.size(), 0);
    dbPassword1.Clear();
    dbPassword2.Clear();
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest018
 * @tc.desc: get password with old key file exit and update three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest018, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());
    auto result = SaveKeyToOldFile(key);
    ASSERT_TRUE(result);

    for (auto loop = 0; loop < LOOP_NUM; ++loop) {
        auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
        ASSERT_TRUE(dbPassword.IsValid());
        ASSERT_TRUE(dbPassword.GetSize() == key.size());
        std::vector<uint8_t> password(dbPassword.GetData(), dbPassword.GetData() + dbPassword.GetSize());
        ASSERT_TRUE(password.size() == key.size());
        for (auto index = 0; index < key.size(); ++index) {
            ASSERT_TRUE(password[index] == key[index]);
        }
        password.assign(password.size(), 0);
        dbPassword.Clear();
    }

    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: SaveDBPasswordNewTest003
 * @tc.desc: save password three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, SaveDBPasswordNewTest003, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());

    DistributedDB::CipherPassword dbPassword1;
    dbPassword1.SetValue(key.data(), key.size());
    ASSERT_TRUE(dbPassword1.GetSize() == key.size());
    auto result = SecurityManager::GetInstance().SaveDBPassword(STORE_NAME, BASE_DIR, dbPassword1);
    ASSERT_TRUE(result);
    dbPassword1.Clear();

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword2.IsValid());
    ASSERT_TRUE(dbPassword2.GetSize() == key.size());
    std::vector<uint8_t> password(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(password.size() == key.size());
    for (auto index = 0; index < key.size(); ++index) {
        ASSERT_TRUE(password[index] == key[index]);
    }
    password.assign(password.size(), 0);
    dbPassword2.Clear();
    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: DelDBPasswordNewTest003
 * @tc.desc: delete password three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, DelDBPasswordNewTest003, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword.IsValid());
    dbPassword.Clear();

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: KeyFilesMultiLockNewTest003
 * @tc.desc: Test KeyFiles function three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesMultiLockNewTest003, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test7";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
}

/**
 * @tc.name: KeyFilesNewTest003
 * @tc.desc: Test KeyFiles function three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesNewTest003, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test8";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
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
 * @tc.name: KeyFilesAutoLockNewTest003
 * @tc.desc: Test KeyFilesAutoLock function three
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesAutoLockNewTest003, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test9";
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

/**
 * @tc.name: GetDBPasswordNewTest019
 * @tc.desc: get password with first create and needCreate is false four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest019, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: GetDBPasswordNewTest020
 * @tc.desc: get password with key file exist and the key file is empty four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest020, TestSize.Level0)
{
    std::vector<char> content;
    auto result = SaveBufferToFile(KEY_FULL_PATH, content);
    ASSERT_TRUE(result);

    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest021
 * @tc.desc: get password with old key file exist and the old key file is invalid four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest021, TestSize.Level0)
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent1;
    invalidContent1.push_back(char(sizeof(time_t) / sizeof(uint8_t)));
    invalidContent1.insert(invalidContent1.end(), date.begin(), date.end());
    auto result = SaveBufferToFile(KEY_FULL_PATH, invalidContent1);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto invalidKey = Random(KEY_SIZE);
    ASSERT_FALSE(invalidKey.empty());
    std::vector<char> invalidContent2;
    invalidContent2.push_back(char((sizeof(time_t) / sizeof(uint8_t))));
    invalidContent2.insert(invalidContent2.end(), date.begin(), date.end());
    invalidContent2.insert(invalidContent2.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent2);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<char> invalidContent3;
    invalidContent3.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    invalidContent3.insert(invalidContent3.end(), date.begin(), date.end());
    invalidContent3.insert(invalidContent3.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent3);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest022
 * @tc.desc: get password with new key file exist and the new key file is invalid four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest022, TestSize.Level0)
{
    std::vector<char> content;

    content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    auto result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    for (size_t index = 0; index < SecurityManager::SecurityContent::MAGIC_NUM - 1; ++index) {
        content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    }
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto nonce = Random(NONCE_SIZE);
    ASSERT_FALSE(nonce.empty());
    content.insert(content.end(), nonce.begin(), nonce.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent = content;
    invalidContent.insert(invalidContent.end(), date.begin(), date.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<uint8_t> keyContent;
    keyContent.push_back(SecurityManager::SecurityContent::CURRENT_VERSION);
    keyContent.insert(keyContent.end(), date.begin(), date.end());
    auto encryptValue = Encrypt(keyContent, nonce);
    ASSERT_FALSE(encryptValue.empty());
    invalidContent = content;
    invalidContent.insert(invalidContent.end(), encryptValue.begin(), encryptValue.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest023
 * @tc.desc: get password with first create and needCreate is true four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest023, TestSize.Level0)
{
    auto dbPassword1 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword1.IsValid());

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_TRUE(dbPassword2.IsValid());

    ASSERT_TRUE(dbPassword2.GetSize() == dbPassword1.GetSize());

    std::vector<uint8_t> key1(dbPassword1.GetData(), dbPassword1.GetData() + dbPassword1.GetSize());
    std::vector<uint8_t> key2(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(key1.size() == key2.size());

    for (auto index = 0; index < key1.size(); ++index) {
        ASSERT_TRUE(key1[index] == key2[index]);
    }

    key1.assign(key1.size(), 0);
    key2.assign(key2.size(), 0);
    dbPassword1.Clear();
    dbPassword2.Clear();
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest024
 * @tc.desc: get password with old key file exit and update four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest024, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());
    auto result = SaveKeyToOldFile(key);
    ASSERT_TRUE(result);

    for (auto loop = 0; loop < LOOP_NUM; ++loop) {
        auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
        ASSERT_TRUE(dbPassword.IsValid());
        ASSERT_TRUE(dbPassword.GetSize() == key.size());
        std::vector<uint8_t> password(dbPassword.GetData(), dbPassword.GetData() + dbPassword.GetSize());
        ASSERT_TRUE(password.size() == key.size());
        for (auto index = 0; index < key.size(); ++index) {
            ASSERT_TRUE(password[index] == key[index]);
        }
        password.assign(password.size(), 0);
        dbPassword.Clear();
    }

    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: SaveDBPasswordNewTest004
 * @tc.desc: save password four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, SaveDBPasswordNewTest004, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());

    DistributedDB::CipherPassword dbPassword1;
    dbPassword1.SetValue(key.data(), key.size());
    ASSERT_TRUE(dbPassword1.GetSize() == key.size());
    auto result = SecurityManager::GetInstance().SaveDBPassword(STORE_NAME, BASE_DIR, dbPassword1);
    ASSERT_TRUE(result);
    dbPassword1.Clear();

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword2.IsValid());
    ASSERT_TRUE(dbPassword2.GetSize() == key.size());
    std::vector<uint8_t> password(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(password.size() == key.size());
    for (auto index = 0; index < key.size(); ++index) {
        ASSERT_TRUE(password[index] == key[index]);
    }
    password.assign(password.size(), 0);
    dbPassword2.Clear();
    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: DelDBPasswordNewTest004
 * @tc.desc: delete password four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, DelDBPasswordNewTest004, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword.IsValid());
    dbPassword.Clear();

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: KeyFilesMultiLockNewTest004
 * @tc.desc: Test KeyFiles function four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesMultiLockNewTest004, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test10";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
}

/**
 * @tc.name: KeyFilesNewTest004
 * @tc.desc: Test KeyFiles function four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesNewTest004, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test11";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
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
 * @tc.name: KeyFilesAutoLockNewTest004
 * @tc.desc: Test KeyFilesAutoLock function four
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesAutoLockNewTest004, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test12";
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

/**
 * @tc.name: GetDBPasswordNewTest025
 * @tc.desc: get password with first create and needCreate is false five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest025, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: GetDBPasswordNewTest026
 * @tc.desc: get password with key file exist and the key file is empty five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest026, TestSize.Level0)
{
    std::vector<char> content;
    auto result = SaveBufferToFile(KEY_FULL_PATH, content);
    ASSERT_TRUE(result);

    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest027
 * @tc.desc: get password with old key file exist and the old key file is invalid five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest027, TestSize.Level0)
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent1;
    invalidContent1.push_back(char(sizeof(time_t) / sizeof(uint8_t)));
    invalidContent1.insert(invalidContent1.end(), date.begin(), date.end());
    auto result = SaveBufferToFile(KEY_FULL_PATH, invalidContent1);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto invalidKey = Random(KEY_SIZE);
    ASSERT_FALSE(invalidKey.empty());
    std::vector<char> invalidContent2;
    invalidContent2.push_back(char((sizeof(time_t) / sizeof(uint8_t))));
    invalidContent2.insert(invalidContent2.end(), date.begin(), date.end());
    invalidContent2.insert(invalidContent2.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent2);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<char> invalidContent3;
    invalidContent3.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    invalidContent3.insert(invalidContent3.end(), date.begin(), date.end());
    invalidContent3.insert(invalidContent3.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent3);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest028
 * @tc.desc: get password with new key file exist and the new key file is invalid five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest028, TestSize.Level0)
{
    std::vector<char> content;

    content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    auto result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    for (size_t index = 0; index < SecurityManager::SecurityContent::MAGIC_NUM - 1; ++index) {
        content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    }
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto nonce = Random(NONCE_SIZE);
    ASSERT_FALSE(nonce.empty());
    content.insert(content.end(), nonce.begin(), nonce.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent = content;
    invalidContent.insert(invalidContent.end(), date.begin(), date.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    std::vector<uint8_t> keyContent;
    keyContent.push_back(SecurityManager::SecurityContent::CURRENT_VERSION);
    keyContent.insert(keyContent.end(), date.begin(), date.end());
    auto encryptValue = Encrypt(keyContent, nonce);
    ASSERT_FALSE(encryptValue.empty());
    invalidContent = content;
    invalidContent.insert(invalidContent.end(), encryptValue.begin(), encryptValue.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest029
 * @tc.desc: get password with first create and needCreate is true five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest029, TestSize.Level0)
{
    auto dbPassword1 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword1.IsValid());

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_TRUE(dbPassword2.IsValid());

    ASSERT_TRUE(dbPassword2.GetSize() == dbPassword1.GetSize());

    std::vector<uint8_t> key1(dbPassword1.GetData(), dbPassword1.GetData() + dbPassword1.GetSize());
    std::vector<uint8_t> key2(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(key1.size() == key2.size());

    for (auto index = 0; index < key1.size(); ++index) {
        ASSERT_TRUE(key1[index] == key2[index]);
    }

    key1.assign(key1.size(), 0);
    key2.assign(key2.size(), 0);
    dbPassword1.Clear();
    dbPassword2.Clear();
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordNewTest030
 * @tc.desc: get password with old key file exit and update five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, GetDBPasswordNewTest030, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());
    auto result = SaveKeyToOldFile(key);
    ASSERT_TRUE(result);

    for (auto loop = 0; loop < LOOP_NUM; ++loop) {
        auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
        ASSERT_TRUE(dbPassword.IsValid());
        ASSERT_TRUE(dbPassword.GetSize() == key.size());
        std::vector<uint8_t> password(dbPassword.GetData(), dbPassword.GetData() + dbPassword.GetSize());
        ASSERT_TRUE(password.size() == key.size());
        for (auto index = 0; index < key.size(); ++index) {
            ASSERT_TRUE(password[index] == key[index]);
        }
        password.assign(password.size(), 0);
        dbPassword.Clear();
    }

    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: SaveDBPasswordNewTest005
 * @tc.desc: save password five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, SaveDBPasswordNewTest005, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());

    DistributedDB::CipherPassword dbPassword1;
    dbPassword1.SetValue(key.data(), key.size());
    ASSERT_TRUE(dbPassword1.GetSize() == key.size());
    auto result = SecurityManager::GetInstance().SaveDBPassword(STORE_NAME, BASE_DIR, dbPassword1);
    ASSERT_TRUE(result);
    dbPassword1.Clear();

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword2.IsValid());
    ASSERT_TRUE(dbPassword2.GetSize() == key.size());
    std::vector<uint8_t> password(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_TRUE(password.size() == key.size());
    for (auto index = 0; index < key.size(); ++index) {
        ASSERT_TRUE(password[index] == key[index]);
    }
    password.assign(password.size(), 0);
    dbPassword2.Clear();
    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: DelDBPasswordNewTest005
 * @tc.desc: delete password five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, DelDBPasswordNewTest005, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword.IsValid());
    dbPassword.Clear();

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: KeyFilesMultiLockNewTest005
 * @tc.desc: Test KeyFiles function five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesMultiLockNewTest005, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test13";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
    auto ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.Lock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
    ret = keyFiles.UnLock();
    EXPECT_TRUE(ret == Status::SUCCESS);
}

/**
 * @tc.name: KeyFilesNewTest005
 * @tc.desc: Test KeyFiles function five
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerNew, KeyFilesNewTest005, TestSize.Level1)
{
    std::string dbPath = "/data/service/el1/public/database/SecurityManagerNew";
    std::string dbName = "test14";
    StoreUtil::InitPath(dbPath);
    SecurityManager::KeyFiles keyFiles(dbName, dbPath);
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


} // namespace OHOS::Test
