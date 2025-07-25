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

#include <chrono>
#include <gtest/gtest.h>

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
static constexpr const char *BASE_DIR = "/data/service/el1/public/database/SecurityManagerTest";
static constexpr const char *KEY_DIR = "/data/service/el1/public/database/SecurityManagerTest/key";
static constexpr const char *KEY_FULL_PATH = "/data/service/el1/public/database/SecurityManagerTest/key/test_store.key";
static constexpr const char *KEY_FULL_PATH_V1 =
    "/data/service/el1/public/database/SecurityManagerTest/key/test_store.key_v1";
static constexpr const char *LOCK_FULL_PATH =
    "/data/service/el1/public/database/SecurityManagerTest/key/test_store.key_lock";
static constexpr const char *ROOT_KEY_ALIAS = "distributeddb_client_root_key";
static constexpr const char *HKS_BLOB_TYPE_NONCE = "Z5s0Bo571KoqwIi6";
static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata_client";

class SecurityManagerTest : public testing::Test {
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

std::vector<uint8_t> SecurityManagerTest::vecRootKeyAlias_ =
    std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
std::vector<uint8_t> SecurityManagerTest::vecNonce_ =
    std::vector<uint8_t>(HKS_BLOB_TYPE_NONCE, HKS_BLOB_TYPE_NONCE + strlen(HKS_BLOB_TYPE_NONCE));
std::vector<uint8_t> SecurityManagerTest::vecAad_ =
    std::vector<uint8_t>(HKS_BLOB_TYPE_AAD, HKS_BLOB_TYPE_AAD + strlen(HKS_BLOB_TYPE_AAD));

void SecurityManagerTest::SetUpTestCase(void)
{
    mkdir(BASE_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    GenerateRootKey();
}

void SecurityManagerTest::TearDownTestCase(void)
{
    DeleteRootKey();
    (void)remove(BASE_DIR);
}

void SecurityManagerTest::SetUp()
{
    mkdir(KEY_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SecurityManagerTest::TearDown()
{
    (void)remove(LOCK_FULL_PATH);
    (void)remove(KEY_FULL_PATH);
    (void)remove(KEY_FULL_PATH_V1);
    (void)remove(KEY_DIR);
}

std::vector<uint8_t> SecurityManagerTest::Random(int32_t length)
{
    std::vector<uint8_t> value(length, 0);
    struct HksBlob blobValue = { .size = length, .data = &(value[0]) };
    auto ret = HksGenerateRandom(nullptr, &blobValue);
    if (ret != HKS_SUCCESS) {
        return {};
    }
    return value;
}

std::vector<uint8_t> SecurityManagerTest::Encrypt(const std::vector<uint8_t> &key, const std::vector<uint8_t> &nonce)
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

bool SecurityManagerTest::SaveKeyToOldFile(const std::vector<uint8_t> &key)
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

void SecurityManagerTest::GenerateRootKey()
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

void SecurityManagerTest::DeleteRootKey()
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
 * @tc.name: GetDBPasswordTest001
 * @tc.desc: get password with first create and needCreate is false
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, GetDBPasswordTest001, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
}

/**
 * @tc.name: GetDBPasswordTest002
 * @tc.desc: get password with key file exist and the key file is empty
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, GetDBPasswordTest002, TestSize.Level0)
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
 * @tc.name: GetDBPasswordTest003
 * @tc.desc: get password with old key file exist and the old key file is invalid
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, GetDBPasswordTest003, TestSize.Level0)
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    // 1.the size of content is invalid
    std::vector<char> invalidContent1;
    invalidContent1.push_back(char(sizeof(time_t) / sizeof(uint8_t)));
    invalidContent1.insert(invalidContent1.end(), date.begin(), date.end());
    auto result = SaveBufferToFile(KEY_FULL_PATH, invalidContent1);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    auto invalidKey = Random(KEY_SIZE);
    ASSERT_FALSE(invalidKey.empty());
    // 2.the pos 0 of content is invalid
    std::vector<char> invalidContent2;
    invalidContent2.push_back(char((sizeof(time_t) / sizeof(uint8_t))));
    invalidContent2.insert(invalidContent2.end(), date.begin(), date.end());
    invalidContent2.insert(invalidContent2.end(), invalidKey.begin(), invalidKey.end());
    result = SaveBufferToFile(KEY_FULL_PATH, invalidContent2);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    // 3.the key of content decrypt fail
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
 * @tc.name: GetDBPasswordTest004
 * @tc.desc: get password with new key file exist and the new key file is invalid
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, GetDBPasswordTest004, TestSize.Level0)
{
    std::vector<char> content;

    // 1.the size of content is invalid
    content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    auto result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    // 2.the size of content is invalid
    for (size_t index = 0; index < SecurityManager::SecurityContent::MAGIC_NUM - 1; ++index) {
        content.push_back(char(SecurityManager::SecurityContent::MAGIC_CHAR));
    }
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    // 3.the size of content is invalid
    auto nonce = Random(NONCE_SIZE);
    ASSERT_FALSE(nonce.empty());
    content.insert(content.end(), nonce.begin(), nonce.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, content);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    // 4.the content decrypt fail
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    std::vector<char> invalidContent = content;
    invalidContent.insert(invalidContent.end(), date.begin(), date.end());
    result = SaveBufferToFile(KEY_FULL_PATH_V1, invalidContent);
    ASSERT_TRUE(result);
    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());

    // 5.the content decrypt success and key is empty
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
 * @tc.name: GetDBPasswordTest005
 * @tc.desc: get password with first create and needCreate is true
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, GetDBPasswordTest005, TestSize.Level0)
{
    auto dbPassword1 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword1.IsValid());

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_TRUE(dbPassword2.IsValid());

    ASSERT_EQ(dbPassword2.GetSize(), dbPassword1.GetSize());

    std::vector<uint8_t> key1(dbPassword1.GetData(), dbPassword1.GetData() + dbPassword1.GetSize());
    std::vector<uint8_t> key2(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_EQ(key1.size(), key2.size());

    for (auto index = 0; index < key1.size(); ++index) {
        ASSERT_EQ(key1[index], key2[index]);
    }

    key1.assign(key1.size(), 0);
    key2.assign(key2.size(), 0);
    dbPassword1.Clear();
    dbPassword2.Clear();
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: GetDBPasswordTest006
 * @tc.desc: get password with old key file exit and update
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, GetDBPasswordTest006, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());
    auto result = SaveKeyToOldFile(key);
    ASSERT_TRUE(result);

    for (auto loop = 0; loop < LOOP_NUM; ++loop) {
        auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
        ASSERT_TRUE(dbPassword.IsValid());
        ASSERT_EQ(dbPassword.GetSize(), key.size());
        std::vector<uint8_t> password(dbPassword.GetData(), dbPassword.GetData() + dbPassword.GetSize());
        ASSERT_EQ(password.size(), key.size());
        for (auto index = 0; index < key.size(); ++index) {
            ASSERT_EQ(password[index], key[index]);
        }
        password.assign(password.size(), 0);
        dbPassword.Clear();
    }

    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: SaveDBPasswordTest001
 * @tc.desc: save password
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, SaveDBPasswordTest001, TestSize.Level0)
{
    auto key = Random(KEY_SIZE);
    ASSERT_FALSE(key.empty());

    DistributedDB::CipherPassword dbPassword1;
    dbPassword1.SetValue(key.data(), key.size());
    ASSERT_EQ(dbPassword1.GetSize(), key.size());
    auto result = SecurityManager::GetInstance().SaveDBPassword(STORE_NAME, BASE_DIR, dbPassword1);
    ASSERT_TRUE(result);
    dbPassword1.Clear();

    auto dbPassword2 = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword2.IsValid());
    ASSERT_EQ(dbPassword2.GetSize(), key.size());
    std::vector<uint8_t> password(dbPassword2.GetData(), dbPassword2.GetData() + dbPassword2.GetSize());
    ASSERT_EQ(password.size(), key.size());
    for (auto index = 0; index < key.size(); ++index) {
        ASSERT_EQ(password[index], key[index]);
    }
    password.assign(password.size(), 0);
    dbPassword2.Clear();
    key.assign(key.size(), 0);
    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);
}

/**
 * @tc.name: DelDBPasswordTest001
 * @tc.desc: delete password
 * @tc.type: FUNC
 */
HWTEST_F(SecurityManagerTest, DelDBPasswordTest001, TestSize.Level0)
{
    auto dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, true);
    ASSERT_TRUE(dbPassword.IsValid());
    dbPassword.Clear();

    SecurityManager::GetInstance().DelDBPassword(STORE_NAME, BASE_DIR);

    dbPassword = SecurityManager::GetInstance().GetDBPassword(STORE_NAME, BASE_DIR, false);
    ASSERT_FALSE(dbPassword.IsValid());
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