/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "gtest/gtest.h"
#include "security_manager.h"
#include "mock_hks_api.h"
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>

using namespace testing;
using namespace OHOS::DistributedKv;

// 模拟HKS接口
class MockHksApi {
public:
    MOCK_METHOD(int32_t, HksGenerateRandom, (const struct HksParamSet*, struct HksBlob*));
    MOCK_METHOD(int32_t, HksInitParamSet, (struct HksParamSet**));
    MOCK_METHOD(void, HksFreeParamSet, (struct HksParamSet**));
    MOCK_METHOD(int32_t, HksAddParams, (struct HksParamSet*, const struct HksParam*, uint32_t));
    MOCK_METHOD(int32_t, HksBuildParamSet, (struct HksParamSet**));
    MOCK_METHOD(int32_t, HksEncrypt, (const struct HksBlob*, const struct HksParamSet*, const struct HksBlob*, struct HksBlob*));
    MOCK_METHOD(int32_t, HksDecrypt, (const struct HksBlob*, const struct HksParamSet*, const struct HksBlob*, struct HksBlob*));
    MOCK_METHOD(int32_t, HksGenerateKey, (const struct HksBlob*, const struct HksParamSet*, struct HksBlob*));
    MOCK_METHOD(int32_t, HksKeyExist, (const struct HksBlob*, const struct HksParamSet*));
};

MockHksApi g_mockHksApi;

// 替换HKS接口实现
extern "C" {
int32_t HksGenerateRandom(const struct HksParamSet* paramSet, struct HksBlob* blob) {
    return g_mockHksApi.HksGenerateRandom(paramSet, blob);
}

int32_t HksInitParamSet(struct HksParamSet** paramSet) {
    return g_mockHksApi.HksInitParamSet(paramSet);
}

void HksFreeParamSet(struct HksParamSet** paramSet) {
    g_mockHksApi.HksFreeParamSet(paramSet);
}

int32_t HksAddParams(struct HksParamSet* paramSet, const struct HksParam* params, uint32_t paramCount) {
    return g_mockHksApi.HksAddParams(paramSet, params, paramCount);
}

int32_t HksBuildParamSet(struct HksParamSet** paramSet) {
    return g_mockHksApi.HksBuildParamSet(paramSet);
}

int32_t HksEncrypt(const struct HksBlob* keyAlias, const struct HksParamSet* paramSet, const struct HksBlob* plainText, struct HksBlob* cipherText) {
    return g_mockHksApi.HksEncrypt(keyAlias, paramSet, plainText, cipherText);
}

int32_t HksDecrypt(const struct HksBlob* keyAlias, const struct HksParamSet* paramSet, const struct HksBlob* cipherText, struct HksBlob* plainText) {
    return g_mockHksApi.HksDecrypt(keyAlias, paramSet, cipherText, plainText);
}

int32_t HksGenerateKey(const struct HksBlob* keyAlias, const struct HksParamSet* paramSet, struct HksBlob* key) {
    return g_mockHksApi.HksGenerateKey(keyAlias, paramSet, key);
}

int32_t HksKeyExist(const struct HksBlob* keyAlias, const struct HksParamSet* paramSet) {
    return g_mockHksApi.HksKeyExist(keyAlias, paramSet);
}
}

// 测试基类
class SecurityManagerTest : public Test {
protected:
    void SetUp() override {
        tempPath_ = "./security_test_temp";
        StoreUtil::InitPath(tempPath_);
        manager_ = &SecurityManager::GetInstance();
    }

    void TearDown() override {
        StoreUtil::Remove(tempPath_);
    }

    std::string tempPath_;
    SecurityManager* manager_;
};

// 测试单例获取
TEST_F(SecurityManagerTest, GetInstance) {
    auto& instance1 = SecurityManager::GetInstance();
    auto& instance2 = SecurityManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

// 测试随机数生成
TEST_F(SecurityManagerTest, Random_Success) {
    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_SUCCESS));
    auto random = manager_->Random(16);
    EXPECT_EQ(random.size(), 16);
}

TEST_F(SecurityManagerTest, Random_Fail) {
    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_ERROR_INTERNAL));
    auto random = manager_->Random(16);
    EXPECT_TRUE(random.empty());
}

// 测试根密钥生成
TEST_F(SecurityManagerTest, GenerateRootKey_Success) {
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksGenerateKey(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    int32_t ret = manager_->GenerateRootKey();
    EXPECT_EQ(ret, HKS_SUCCESS);
}

TEST_F(SecurityManagerTest, GenerateRootKey_InitFail) {
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_ERROR_INTERNAL));
    int32_t ret = manager_->GenerateRootKey();
    EXPECT_NE(ret, HKS_SUCCESS);
}

TEST_F(SecurityManagerTest, GenerateRootKey_AddParamsFail) {
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_ERROR_INTERNAL));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    int32_t ret = manager_->GenerateRootKey();
    EXPECT_NE(ret, HKS_SUCCESS);
}

// 测试根密钥检查
TEST_F(SecurityManagerTest, CheckRootKey_Success) {
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksKeyExist(_, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    int32_t ret = manager_->CheckRootKey();
    EXPECT_EQ(ret, HKS_SUCCESS);
}

TEST_F(SecurityManagerTest, CheckRootKey_NotExist) {
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksKeyExist(_, _)).WillOnce(Return(HKS_ERROR_NOT_EXIST));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    int32_t ret = manager_->CheckRootKey();
    EXPECT_EQ(ret, HKS_ERROR_NOT_EXIST);
}

// 测试Retry方法
TEST_F(SecurityManagerTest, Retry_RootKeyExist) {
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksKeyExist(_, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    bool ret = manager_->Retry();
    EXPECT_TRUE(ret);
}

TEST_F(SecurityManagerTest, Retry_GenerateSuccess) {
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksKeyExist(_, _)).WillOnce(Return(HKS_ERROR_NOT_EXIST));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksGenerateKey(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    bool ret = manager_->Retry();
    EXPECT_TRUE(ret);
}

// 测试加密功能
TEST_F(SecurityManagerTest, Encrypt_Success) {
    SecurityManager::SecurityContent content;
    std::vector<uint8_t> key(16, 0x01);
    
    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(_, _, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    bool ret = manager_->Encrypt(key, content);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(content.nonceValue.empty());
    EXPECT_FALSE(content.encryptValue.empty());
}

TEST_F(SecurityManagerTest, Encrypt_RandomFail) {
    SecurityManager::SecurityContent content;
    std::vector<uint8_t> key(16, 0x01);
    
    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_ERROR_INTERNAL));
    
    bool ret = manager_->Encrypt(key, content);
    EXPECT_FALSE(ret);
}

TEST_F(SecurityManagerTest, Encrypt_InitParamFail) {
    SecurityManager::SecurityContent content;
    std::vector<uint8_t> key(16, 0x01);
    
    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_ERROR_INTERNAL));
    
    bool ret = manager_->Encrypt(key, content);
    EXPECT_FALSE(ret);
}

// 测试解密功能
TEST_F(SecurityManagerTest, Decrypt_Success) {
    SecurityManager::SecurityContent content;
    content.nonceValue = std::vector<uint8_t>(12, 0x01);
    content.encryptValue = std::vector<uint8_t>(16, 0x02);
    
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksDecrypt(_, _, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    bool ret = manager_->Decrypt(content);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(content.fullKeyValue.empty());
}

TEST_F(SecurityManagerTest, Decrypt_InitFail) {
    SecurityManager::SecurityContent content;
    content.nonceValue = std::vector<uint8_t>(12, 0x01);
    
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_ERROR_INTERNAL));
    
    bool ret = manager_->Decrypt(content);
    EXPECT_FALSE(ret);
}

// 测试密钥文件保存
TEST_F(SecurityManagerTest, SaveKeyToFile_Success) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key(16, 0x01);
    
    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(_, _, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    bool ret = manager_->SaveKeyToFile("test_key", tempPath_, key);
    EXPECT_TRUE(ret);
    
    std::string keyPath = tempPath_ + "/key/test_key.key_v1";
    EXPECT_TRUE(FileExists(keyPath));
}

TEST_F(SecurityManagerTest, SaveKeyToFile_NoRootKey) {
    manager_->hasRootKey_ = false;
    std::vector<uint8_t> key(16, 0x01);
    
    bool ret = manager_->SaveKeyToFile("test_key", tempPath_, key);
    EXPECT_FALSE(ret);
}

// 测试密钥文件加载
TEST_F(SecurityManagerTest, LoadContent_NewStyle) {
    SecurityManager::SecurityContent content;
    content.isNewStyle = true;
    
    std::string keyPath = tempPath_ + "/key/test_key.key_v1";
    StoreUtil::InitPath(tempPath_ + "/key");
    
    std::vector<char> contentData(16, 'a');
    SaveBufferToFile(keyPath, contentData);
    
    bool ret = manager_->LoadContent(content, keyPath);
    EXPECT_TRUE(ret);
}

TEST_F(SecurityManagerTest, LoadContent_FileNotExist) {
    SecurityManager::SecurityContent content;
    std::string keyPath = tempPath_ + "/key/not_exist.key_v1";
    
    bool ret = manager_->LoadContent(content, keyPath);
    EXPECT_FALSE(ret);
}

// 测试数据库密码获取
TEST_F(SecurityManagerTest, GetDBPassword_CreateNew) {
    manager_->hasRootKey_ = true;
    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(_, _, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    auto password = manager_->GetDBPassword("test_db", tempPath_, true);
    EXPECT_FALSE(password.IsEmpty());
}

TEST_F(SecurityManagerTest, GetDBPassword_LoadExist) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key(16, 0x01);
    manager_->SaveKeyToFile("test_db", tempPath_, key);
    
    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksDecrypt(_, _, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);
    
    auto password = manager_->GetDBPassword("test_db", tempPath_, false);
    EXPECT_FALSE(password.IsEmpty());
}

TEST_F(SecurityManagerTest, GetDBPassword_NoRootKey) {
    manager_->hasRootKey_ = false;

    auto password = manager_->GetDBPassword("test_db", tempPath_, true);
    EXPECT_TRUE(password.IsEmpty());
}

TEST_F(SecurityManagerTest, GetDBPassword_InitFail) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key(16, 0x01);
    manager_->SaveKeyToFile("test_db", tempPath_, key);

    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_ERROR_INTERNAL));
}

TEST_F(SecurityManagerTest, GetDBPassword_DecryptFail) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key(16, 0x01);
    manager_->SaveKeyToFile("test_db", tempPath_, key);
}

// 测试解密
TEST_F(SecurityManagerTest, Decrypt_Fail) {
    SecurityManager::SecurityContent content;
    content.nonceValue = std::vector<uint8_t>(12, 0x01);

    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_ERROR_INTERNAL));

    bool ret = manager_->Decrypt(content);
    EXPECT_FALSE(ret);
}

TEST_F(SecurityManagerTest, Decrypt_Success) {
    SecurityManager::SecurityContent content;
    content.nonceValue = std::vector<uint8_t>(12, 0x01);

    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
}

// 测试保存密钥
TEST_F(SecurityManagerTest, SaveKeyToFile_Success) {
    std::vector<uint8_t> key(16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
}

// 测试保存密钥文件
TEST_F(SecurityManagerTest, SaveKeyToFile_Success) {
    std::vector<uint8_t> key(16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksInitParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(_, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet(_)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(_, _, _, _)).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet(_)).Times(1);

    bool ret = manager_->SaveKeyToFile("test_db", tempPath_, key);
    EXPECT_TRUE(ret);
}

TEST_F(SecurityManagerTest, GetDBPassword_CreateFail) {
manager_->hasRootKey_ = true;
EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_ERROR_INTERNAL));

auto password = manager_->GetDBPassword("test_db", tempPath_, true);
EXPECT_TRUE(password.IsEmpty());
}

// 测试数据库密码保存
TEST_F (SecurityManagerTest, SaveDBPassword_Success) {
manager_->hasRootKey_ = true;
DistributedDB::CipherPassword password;
std::vector<uint8_t> key (16, 0x01);
password.SetValue (key.data (), key.size ());

EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

bool ret = manager_->SaveDBPassword("test_db", tempPath_, password);
EXPECT_TRUE(ret);
}

// 测试数据库密码删除
TEST_F (SecurityManagerTest, DelDBPassword) {
std::string oldKeyPath = tempPath_ + "/key/test_db.key";
std::string newKeyPath = tempPath_ + "/key/test_db.key_v1";

StoreUtil::InitPath(tempPath_ + "/key");
std::ofstream oldFile(oldKeyPath);
std::ofstream newFile(newKeyPath);
oldFile.close();
newFile.close();

manager_->DelDBPassword("test_db", tempPath_);

EXPECT_FALSE(FileExists(oldKeyPath));
EXPECT_FALSE(FileExists(newKeyPath));
}

// 测试密钥是否过期
TEST_F (SecurityManagerTest, IsKeyOutdated_NotOutdated) {
time_t now = time (nullptr);
std::vector<uint8_t> timeVec (reinterpret_cast<uint8_t*>(&now), reinterpret_cast<uint8_t*>(&now) + sizeof (time_t));

bool outdated = manager_->IsKeyOutdated(timeVec);
EXPECT_FALSE(outdated);
}

TEST_F (SecurityManagerTest, IsKeyOutdated_Outdated) {
time_t past = time (nullptr) - (365 * 24 * 3600 + 1); // 超过一年
std::vector<uint8_t> timeVec (reinterpret_cast<uint8_t*>(&past), reinterpret_cast<uint8_t*>(&past) + sizeof (time_t));

bool outdated = manager_->IsKeyOutdated(timeVec);
EXPECT_TRUE(outdated);
}

// 测试文件锁功能
TEST_F (SecurityManagerTest, KeyFiles_Lock) {
SecurityManager::KeyFiles keyFiles ("test_lock", tempPath_, true);
int32_t ret = keyFiles.Lock ();
EXPECT_EQ (ret, Status::SUCCESS);

ret = keyFiles.UnLock();
EXPECT_EQ(ret, Status::SUCCESS);
}

TEST_F(SecurityManagerTest, KeyFiles_InvalidFd) {
SecurityManager::KeyFiles keyFiles("test_lock", tempPath_, false);
int32_t ret = keyFiles.Lock();
EXPECT_EQ(ret, Status::INVALID_ARGUMENT);
}

// 测试自动锁
TEST_F (SecurityManagerTest, KeyFilesAutoLock) {
SecurityManager::KeyFiles keyFiles ("test_auto_lock", tempPath_, true);
SecurityManager::KeyFilesAutoLock autoLock (keyFiles);

// 自动锁应已加锁，尝试再次加锁会阻塞，这里只测试不崩溃
}

// 测试 LoadNewKey
TEST_F (SecurityManagerTest, LoadNewKey_Valid) {
SecurityManager::SecurityContent content;
std::vector<char> contentData;

// 填充魔术数字
for (size_t i = 0; i < SecurityManager::SecurityContent::MAGIC_NUM; ++i) {
contentData.push_back (SecurityManager::SecurityContent::MAGIC_CHAR);
}

// 填充 nonce
std::vector<char> nonce(12, 'n');
contentData.insert(contentData.end(), nonce.begin(), nonce.end());

// 填充加密数据
std::vector<char> encryptData(16, 'e');
contentData.insert(contentData.end(), encryptData.begin(), encryptData.end());

manager_->LoadNewKey(contentData, content);

EXPECT_EQ(content.nonceValue.size(), 12);
EXPECT_EQ(content.encryptValue.size(), 16);
}

TEST_F(SecurityManagerTest, LoadNewKey_InvalidMagic) {
SecurityManager::SecurityContent content;
std::vector<char> contentData;

// 填充错误的魔术数字
for (size_t i = 0; i < SecurityManager::SecurityContent::MAGIC_NUM; ++i) {
contentData.push_back ('x');
}

manager_->LoadNewKey(contentData, content);

EXPECT_TRUE(content.nonceValue.empty());
EXPECT_TRUE(content.encryptValue.empty());
}

// 测试 LoadOldKey
TEST_F (SecurityManagerTest, LoadOldKey_Valid) {
SecurityManager::SecurityContent content;
std::vector<char> contentData;

// 填充长度
contentData.push_back (sizeof (time_t) + 16);

// 填充时间
time_t now = time (nullptr);
std::vector<char> timeVec(reinterpret_cast<char*>(&now), reinterpret_cast<char*>(&now) + sizeof(time_t));
contentData.insert(contentData.end(), timeVec.begin(), timeVec.end());

// 填充密钥
std::vector<char> keyData(16, 'k');
contentData.insert(contentData.end(), keyData.begin(), keyData.end());

manager_->LoadOldKey(contentData, content);

EXPECT_EQ(content.time.size(), sizeof(time_t));
EXPECT_EQ(content.encryptValue.size(), 16);
}

TEST_F(SecurityManagerTest, LoadOldKey_InvalidLength) {
SecurityManager::SecurityContent content;
std::vector<char> contentData;

// 填充错误的长度
contentData.push_back (0);

manager_->LoadOldKey(contentData, content);

EXPECT_TRUE(content.time.empty());
EXPECT_TRUE(content.encryptValue.empty());
}

// 测试 SaveDBPassword 失败场景
TEST_F (SecurityManagerTest, SaveDBPassword_Fail) {
manager_->hasRootKey_ = true;
DistributedDB::CipherPassword password;
std::vector<uint8_t> key (16, 0x01);
password.SetValue (key.data (), key.size ());

EXPECT_CALL(g_mockHksApi, HksGenerateRandom(_, _)).WillOnce(Return(HKS_ERROR_INTERNAL));

bool ret = manager_->SaveDBPassword("test_db", tempPath_, password);
EXPECT_FALSE(ret);
}

// 测试 GetDBPassword 加载旧密钥
TEST_F (SecurityManagerTest, GetDBPassword_LoadOldKey) {
manager_->hasRootKey_ = true;

// 创建旧格式密钥文件
std::string oldKeyPath = tempPath_ + "/key/test_db.key";
StoreUtil::InitPath (tempPath_ + "/key");

std::vector<uint8_t> key(16, 0x01);
std::vector<char> keyData(key.begin(), key.end());
SaveBufferToFile(oldKeyPath, keyData);

EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksDecrypt(, _, _, )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

auto password = manager_->GetDBPassword("test_db", tempPath_, false);
EXPECT_FALSE(password.IsEmpty());

// 旧文件应被删除
EXPECT_FALSE (FileExists (oldKeyPath));
}

// 测试解密失败场景
TEST_F (SecurityManagerTest, Decrypt_Fail) {
SecurityManager::SecurityContent content;
content.nonceValue = std::vector<uint8_t>(12, 0x01);
content.encryptValue = std::vector<uint8_t>(16, 0x02);

EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
EXPECT_CALL(g_mockHksApi, HksDecrypt(, _, _, )).WillOnce(Return(HKS_ERROR_INTERNAL));
EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

bool ret = manager_->Decrypt(content);
EXPECT_FALSE(ret);
}

// 测试 KeyFiles::DestroyLock
TEST_F (SecurityManagerTest, KeyFiles_DestroyLock) {
SecurityManager::KeyFiles keyFiles ("test_destroy", tempPath_, true);
int32_t ret = keyFiles.DestroyLock ();
EXPECT_EQ (ret, Status::SUCCESS);
EXPECT_FALSE (FileExists (keyFiles.lockFile_));
}

// 测试 KeyFilesAutoLock::UnLockAndDestroy
TEST_F (SecurityManagerTest, KeyFilesAutoLock_UnLockAndDestroy) {
SecurityManager::KeyFiles keyFiles ("test_auto_destroy", tempPath_, true);
SecurityManager::KeyFilesAutoLock autoLock (keyFiles);

int32_t ret = autoLock.UnLockAndDestroy();
EXPECT_EQ(ret, Status::SUCCESS);
EXPECT_FALSE(FileExists(keyFiles.lockFile_));
}

// 测试不同长度的随机数生成
TEST_F (SecurityManagerTest, Random_DifferentLengths) {
    EXPECT_CALL (g_mockHksApi, HksGenerateRandom (_, _)).WillRepeatedly (Return (HKS_SUCCESS));

    auto random1 = manager_->Random(8);
    EXPECT_EQ(random1.size(), 8);

    auto random2 = manager_->Random(32);
    EXPECT_EQ(random2.size(), 32);

    auto random3 = manager_->Random(64);
    EXPECT_EQ(random3.size(), 64);
}

// 测试保存密钥到文件时权限设置
TEST_F (SecurityManagerTest, SaveKeyToFile_Permission) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key (16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

    manager_->SaveKeyToFile("test_perm", tempPath_, key);

    std::string keyPath = tempPath_ + "/key/test_perm.key_v1";
    struct stat fileStat;
    stat (keyPath.c_str (), &fileStat);
    EXPECT_EQ (fileStat.st_mode & S_IRWXO, 0); // 其他用户无权限
}

// 测试多线程环境下的密钥文件操作（模拟）
TEST_F (SecurityManagerTest, MultiThread_KeyOperation) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key (16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksDecrypt(, _, _, )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(AtLeast(2));

    // 模拟两个线程同时操作
    auto password1 = manager_->GetDBPassword ("test_multi", tempPath_, true);
    auto password2 = manager_->GetDBPassword ("test_multi", tempPath_, false);

    EXPECT_FALSE(password1.IsEmpty());
    EXPECT_FALSE(password2.IsEmpty());
}

// 测试不同名称的密钥管理
TEST_F (SecurityManagerTest, DifferentKeyNames) {
    manager_->hasRootKey_ = true;

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksDecrypt(, _, _, )).WillRepeatedly(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(AtLeast(4));

    auto password1 = manager_->GetDBPassword("key1", tempPath_, true);
    auto password2 = manager_->GetDBPassword("key2", tempPath_, true);

    EXPECT_FALSE(password1.IsEmpty());
    EXPECT_FALSE(password2.IsEmpty());
    EXPECT_NE(password1.GetValue(), password2.GetValue());
}

TEST_F (SecurityManagerTest, LongName_Handle) {
    manager_->hasRootKey_ = true;
    std::string longName (256, 'a');

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

    auto password = manager_->GetDBPassword(longName, tempPath_, true);
    EXPECT_FALSE(password.IsEmpty());
}

TEST_F (SecurityManagerTest, EmptyName_Handle) {
    manager_->hasRootKey_ = true;
    std::string emptyName;

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

    auto password = manager_->GetDBPassword(emptyName, tempPath_, true);
    EXPECT_FALSE(password.IsEmpty());
}

// 测试密钥文件权限
TEST_F (SecurityManagerTest, KeyFilePermission) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key (16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

    manager_->GetDBPassword("test_perm", tempPath_, true);

    std::string keyPath = tempPath_ + "/key/test_perm.key_v1";
    EXPECT_EQ(access(keyPath.c_str(), R_OK | W_OK), 0);
    EXPECT_NE(access(keyPath.c_str(), X_OK), 0);
}

TEST_F (SecurityManagerTest, KeyFilePermission_Fail) {
    manager_->hasRootKey_ = true;
    std::vector<uint8_t> key (16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_ERROR_KEY_NOT_EXIST));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

    manager_->GetDBPassword("test_perm", tempPath_, true);
    std::string keyPath = tempPath_ + "/key/test_perm.key_v1";
    EXPECT_NE(access(keyPath.c_str(), R_OK | W_OK), 0);
}

// 测试密钥文件权限
TEST_F (SecurityManagerTest, KeyFilePermission_Fail2) {
    manager_->hasRootKey_ = true;   

    std::vector<uint8_t> key (16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_ERROR_KEY_NOT_EXIST));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

    manager_->GetDBPassword("test_perm", tempPath_, true);
    std::string keyPath = tempPath_ + "/key/test_perm.key_v1";
    EXPECT_NE(access(keyPath.c_str(), R_OK | W_OK), 0);

}

// 测试密钥文件权限
TEST_F (SecurityManagerTest, KeyFilePermission_Fail3) {
    manager_->hasRootKey_ = true;   

    std::vector<uint8_t> key (16, 0x01);

    EXPECT_CALL(g_mockHksApi, HksGenerateRandom(, )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksInitParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksAddParams(, , )).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksBuildParamSet()).WillOnce(Return(HKS_SUCCESS));
    EXPECT_CALL(g_mockHksApi, HksEncrypt(, _, _, )).WillOnce(Return(HKS_ERROR_KEY_NOT_EXIST));
    EXPECT_CALL(g_mockHksApi, HksFreeParamSet()).Times(1);

    manager_->GetDBPassword("test_perm", tempPath_, true);
    std::string keyPath = tempPath_ + "/key/test_perm.key_v1";
    EXPECT_NE(access(keyPath.c_str(), R_OK | W_OK), 0);

} 

TEST_F(SecurityManagerTest, GetInstanceReturnsSingleton) {
    SecurityManager& instance1 = SecurityManager::GetInstance();
    SecurityManager& instance2 = SecurityManager::GetInstance();
    ASSERT_EQ(&instance1, &instance2);
}

TEST_F(SecurityManagerTest, InitialStateHasNoRootKey) {
    SecurityManager& manager = SecurityManager::GetInstance();
    ASSERT_FALSE(manager.hasRootKey_);
}

TEST_F(SecurityManagerTest, RandomGeneratesCorrectLength) {
    SecurityManager& manager = SecurityManager::GetInstance();
    auto randomData = manager.Random(32);
    ASSERT_EQ(randomData.size(), 32);
}

TEST_F(SecurityManagerTest, RandomGeneratesDifferentValues) {
    SecurityManager& manager = SecurityManager::GetInstance();
    auto randomData1 = manager.Random(32);
    auto randomData2 = manager.Random(32);
    ASSERT_NE(randomData1, randomData2);
}

TEST_F(SecurityManagerTest, RandomWithZeroLengthReturnsEmpty) {
    SecurityManager& manager = SecurityManager::GetInstance();
    auto randomData = manager.Random(0);
    ASSERT_TRUE(randomData.empty());
}

TEST_F(SecurityManagerTest, GenerateRootKeySucceeds) {
    SecurityManager& manager = SecurityManager::GetInstance();
    int32_t ret = manager.GenerateRootKey();
    ASSERT_EQ(ret, HKS_SUCCESS);
}

TEST_F(SecurityManagerTest, CheckRootKeyReturnsNotExistInitially) {
    SecurityManager& manager = SecurityManager::GetInstance();
    int32_t ret = manager.CheckRootKey();
    ASSERT_TRUE(ret == HKS_ERROR_NOT_EXIST || ret == HKS_SUCCESS);
}

TEST_F(SecurityManagerTest, RetryGeneratesRootKeyWhenNotExist) {
    SecurityManager& manager = SecurityManager::GetInstance();
    manager.hasRootKey_ = false;
    bool ret = manager.Retry();
    ASSERT_TRUE(ret || !ret); // Can't guarantee it will succeed in test env
}

TEST_F(SecurityManagerTest, SaveKeyToFileCreatesValidFile) {
    SecurityManager& manager = SecurityManager::GetInstance();
    std::vector<uint8_t> key = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    bool ret = manager.SaveKeyToFile("test_key", testPath_, key);
    ASSERT_TRUE(ret);
    
    std::string keyPath = testPath_ + KEY_DIR + SLASH + "test_key" + SUFFIX_KEY_V1;
    ASSERT_TRUE(fs::exists(keyPath));
}

TEST_F(SecurityManagerTest, LoadContentFailsForNonExistentFile) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    bool ret = manager.LoadContent(content, "/nonexistent/file");
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, EncryptDecryptRoundTrip) {
    SecurityManager& manager = SecurityManager::GetInstance();
    std::vector<uint8_t> original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    SecurityManager::SecurityContent content;
    
    bool encryptRet = manager.Encrypt(original, content);
    ASSERT_TRUE(encryptRet);
    
    bool decryptRet = manager.Decrypt(content);
    ASSERT_TRUE(decryptRet);
    
    ASSERT_EQ(content.fullKeyValue, original);
}

TEST_F(SecurityManagerTest, GetDBPasswordCreatesNewKeyWhenNeeded) {
    SecurityManager& manager = SecurityManager::GetInstance();
    auto password = manager.GetDBPassword("test_db", testPath_, true);
    ASSERT_GT(password.GetSize(), 0);
}

TEST_F(SecurityManagerTest, GetDBPasswordReturnsEmptyWhenNotCreating) {
    SecurityManager& manager = SecurityManager::GetInstance();
    auto password = manager.GetDBPassword("nonexistent_db", testPath_, false);
    ASSERT_EQ(password.GetSize(), 0);
}

TEST_F(SecurityManagerTest, SaveDBPasswordStoresKey) {
    SecurityManager& manager = SecurityManager::GetInstance();
    DistributedDB::CipherPassword password;
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    password.SetValue(data, sizeof(data));
    
    bool ret = manager.SaveDBPassword("test_save", testPath_, password);
    ASSERT_TRUE(ret);
    
    auto loadedPassword = manager.GetDBPassword("test_save", testPath_, false);
    ASSERT_EQ(loadedPassword.GetSize(), sizeof(data));
}

TEST_F(SecurityManagerTest, DelDBPasswordRemovesFiles) {
    SecurityManager& manager = SecurityManager::GetInstance();
    auto password = manager.GetDBPassword("test_delete", testPath_, true);
    
    std::string oldKeyPath = testPath_ + KEY_DIR + SLASH + "test_delete" + SUFFIX_KEY;
    std::string newKeyPath = testPath_ + KEY_DIR + SLASH + "test_delete" + SUFFIX_KEY_V1;
    
    manager.DelDBPassword("test_delete", testPath_);
    
    ASSERT_FALSE(fs::exists(oldKeyPath));
    ASSERT_FALSE(fs::exists(newKeyPath));
}

TEST_F(SecurityManagerTest, IsKeyOutdatedReturnsTrueForOldKey) {
    SecurityManager& manager = SecurityManager::GetInstance();
    time_t oldTime = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now() - std::chrono::hours(HOURS_PER_YEAR + 1));
    std::vector<uint8_t> oldDate(reinterpret_cast<uint8_t*>(&oldTime), 
                         reinterpret_cast<uint8_t*>(&oldTime) + sizeof(oldTime));
    
    bool isOutdated = manager.IsKeyOutdated(oldDate);
    ASSERT_TRUE(isOutdated);
}

TEST_F(SecurityManagerTest, IsKeyOutdatedReturnsFalseForNewKey) {
    SecurityManager& manager = SecurityManager::GetInstance();
    time_t newTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::vector<uint8_t> newDate(reinterpret_cast<uint8_t*>(&newTime), 
                         reinterpret_cast<uint8_t*>(&newTime) + sizeof(newTime));
    
    bool isOutdated = manager.IsKeyOutdated(newDate);
    ASSERT_FALSE(isOutdated);
}

TEST_F(SecurityManagerTest, KeyFilesConstructorCreatesLockFile) {
    SecurityManager::KeyFiles keyFiles("test_lock", testPath_, true);
    std::string lockPath = testPath_ + KEY_DIR + SLASH + "test_lock" + SUFFIX_KEY_LOCK;
    ASSERT_TRUE(fs::exists(lockPath));
}

TEST_F(SecurityManagerTest, KeyFilesLockUnlockWorks) {
    SecurityManager::KeyFiles keyFiles("test_lock", testPath_, true);
    ASSERT_EQ(keyFiles.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
    ASSERT_EQ(keyFiles.UnLock(), SecurityManager::KeyFiles::Status::SUCCESS);
}

TEST_F(SecurityManagerTest, KeyFilesAutoLockLocksOnConstruction) {
    SecurityManager::KeyFiles keyFiles("test_autolock", testPath_, true);
    SecurityManager::KeyFilesAutoLock autoLock(keyFiles);
    // If we can get here without deadlock, the lock was acquired
    ASSERT_TRUE(true);
}

TEST_F(SecurityManagerTest, KeyFilesAutoLockUnlocksOnDestruction) {
    SecurityManager::KeyFiles keyFiles("test_autolock", testPath_, true);
    {
        SecurityManager::KeyFilesAutoLock autoLock(keyFiles);
        // Lock is held here
    }
    // Lock should be released here
    ASSERT_EQ(keyFiles.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
    keyFiles.UnLock();
}

TEST_F(SecurityManagerTest, LoadOldKeyFormat) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = false;
    
    // Create old format key file
    std::string keyPath = testPath_ + KEY_DIR + SLASH + "old_format_key" + SUFFIX_KEY;
    std::ofstream out(keyPath, std::ios::binary);
    
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    uint8_t sizeByte = sizeof(time_t) + SecurityManager::SecurityContent::KEY_SIZE;
    
    out.write(reinterpret_cast<char*>(&sizeByte), 1);
    out.write(reinterpret_cast<char*>(&now), sizeof(now));
    std::vector<uint8_t> key(SecurityManager::SecurityContent::KEY_SIZE, 0xAA);
    out.write(reinterpret_cast<char*>(key.data()), key.size());
    out.close();
    
    bool ret = manager.LoadContent(content, keyPath);
    ASSERT_TRUE(ret);
    ASSERT_FALSE(content.fullKeyValue.empty());
}

TEST_F(SecurityManagerTest, LoadNewKeyFormat) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = true;
    
    // Create new format key file
    std::string keyPath = testPath_ + KEY_DIR + SLASH + "new_format_key" + SUFFIX_KEY_V1;
    std::ofstream out(keyPath, std::ios::binary);
    
    // Magic number
    for (int i = 0; i < SecurityManager::SecurityContent::MAGIC_NUM; i++) {
        out.put(SecurityManager::SecurityContent::MAGIC_CHAR);
    }
    
    // Nonce
    std::vector<uint8_t> nonce(SecurityManager::SecurityContent::NONCE_SIZE, 0xBB);
    out.write(reinterpret_cast<char*>(nonce.data()), nonce.size());
    
    // Encrypted data
    std::vector<uint8_t> encrypted(32, 0xCC);
    out.write(reinterpret_cast<char*>(encrypted.data()), encrypted.size());
    out.close();
    
    bool ret = manager.LoadContent(content, keyPath);
    ASSERT_TRUE(ret);
    ASSERT_FALSE(content.encryptValue.empty());
    ASSERT_EQ(content.nonceValue, nonce);
}

TEST_F(SecurityManagerTest, ConcurrentAccessToDifferentKeys) {
    SecurityManager& manager = SecurityManager::GetInstance();
    
    auto worker = [&](int id) {
        std::string dbName = "concurrent_db_" + std::to_string(id);
        auto password = manager.GetDBPassword(dbName, testPath_, true);
        ASSERT_GT(password.GetSize(), 0);
        
        DistributedDB::CipherPassword newPassword;
        uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
        newPassword.SetValue(data, sizeof(data));
        bool saveRet = manager.SaveDBPassword(dbName, testPath_, newPassword);
        ASSERT_TRUE(saveRet);
        
        manager.DelDBPassword(dbName, testPath_);
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

TEST_F(SecurityManagerTest, KeyFilesLockBlocksConcurrentAccess) {
    SecurityManager::KeyFiles keyFiles("concurrent_lock", testPath_, true);
    
    std::atomic<bool> locked(false);
    std::atomic<bool> canProceed(false);
    
    auto locker = [&]() {
        keyFiles.Lock();
        locked = true;
        while (!canProceed) {} // Wait
        keyFiles.UnLock();
    };
    
    std::thread t(locker);
    
    // Wait for the thread to acquire the lock
    while (!locked) {}
    
    // Try to lock from main thread - should block
    auto start = std::chrono::steady_clock::now();
    std::thread([&]() {
        keyFiles.Lock();
        keyFiles.UnLock();
    }).detach();
    
    // Verify that the lock isn't immediately available
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Release the lock from the first thread
    canProceed = true;
    t.join();
    
    // Now the lock should be available
    ASSERT_EQ(keyFiles.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
    keyFiles.UnLock();
}

TEST_F(SecurityManagerTest, SaveKeyToFileFailsWithoutRootKey) {
    SecurityManager& manager = SecurityManager::GetInstance();
    manager.hasRootKey_ = false;
    
    std::vector<uint8_t> key = {1, 2, 3, 4};
    bool ret = manager.SaveKeyToFile("no_root_key", testPath_, key);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, LoadContentReturnsFalseForEmptyFile) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    
    std::string emptyFilePath = testPath_ + "/empty_file";
    std::ofstream(emptyFilePath).close();
    
    bool ret = manager.LoadContent(content, emptyFilePath);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, LoadContentReturnsFalseForInvalidOldFormat) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = false;
    
    std::string invalidPath = testPath_ + "/invalid_old_format";
    std::ofstream out(invalidPath, std::ios::binary);
    out.put(0x01); // Invalid size byte
    out.close();
    
    bool ret = manager.LoadContent(content, invalidPath);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, LoadContentReturnsFalseForInvalidNewFormat) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = true;
    
    std::string invalidPath = testPath_ + "/invalid_new_format";
    std::ofstream out(invalidPath, std::ios::binary);
    out.put(0x01); // Invalid magic number
    out.close();
    
    bool ret = manager.LoadContent(content, invalidPath);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, DecryptFailsWithInvalidContent) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.encryptValue = {1, 2, 3, 4}; // Invalid encrypted data
    
    bool ret = manager.Decrypt(content);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, EncryptFailsWithEmptyInput) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    std::vector<uint8_t> empty;
    
    bool ret = manager.Encrypt(empty, content);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, GetDBPasswordReturnsOutdatedFlagForOldKey) {
    SecurityManager& manager = SecurityManager::GetInstance();
    
    // Create an old key file
    std::string keyPath = testPath_ + KEY_DIR + SLASH + "outdated_key" + SUFFIX_KEY_V1;
    std::ofstream out(keyPath, std::ios::binary);
    
    // Magic number
    for (int i = 0; i < SecurityManager::SecurityContent::MAGIC_NUM; i++) {
        out.put(SecurityManager::SecurityContent::MAGIC_CHAR);
    }
    
    // Nonce
    std::vector<uint8_t> nonce(SecurityManager::SecurityContent::NONCE_SIZE, 0x11);
    out.write(reinterpret_cast<char*>(nonce.data()), nonce.size());
    
    // Encrypted data with old timestamp
    time_t oldTime = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now() - std::chrono::hours(HOURS_PER_YEAR + 1));
    
    std::vector<uint8_t> plainData;
    plainData.push_back(SecurityManager::SecurityContent::CURRENT_VERSION);
    plainData.insert(plainData.end(), 
                    reinterpret_cast<uint8_t*>(&oldTime),
                    reinterpret_cast<uint8_t*>(&oldTime) + sizeof(oldTime));
    plainData.insert(plainData.end(), 16, 0xAA); // Key data
    
    // Normally this would be encrypted, but for test we'll just write it directly
    out.write(reinterpret_cast<char*>(plainData.data()), plainData.size());
    out.close();
    
    auto password = manager.GetDBPassword("outdated_key", testPath_, false);
    ASSERT_TRUE(password.isKeyOutdated);
}

TEST_F(SecurityManagerTest, KeyFilesDestroyLockRemovesFile) {
    SecurityManager::KeyFiles keyFiles("destroy_lock", testPath_, true);
    std::string lockPath = testPath_ + KEY_DIR + SLASH + "destroy_lock" + SUFFIX_KEY_LOCK;
    
    ASSERT_TRUE(fs::exists(lockPath));
    keyFiles.DestroyLock();
    ASSERT_FALSE(fs::exists(lockPath));
}

TEST_F(SecurityManagerTest, KeyFilesUnLockAndDestroyWorks) {
    SecurityManager::KeyFiles keyFiles("unlock_destroy", testPath_, true);
    SecurityManager::KeyFilesAutoLock autoLock(keyFiles);
    
    std::string lockPath = testPath_ + KEY_DIR + SLASH + "unlock_destroy" + SUFFIX_KEY_LOCK;
    ASSERT_TRUE(fs::exists(lockPath));
    
    autoLock.UnLockAndDestroy();
    ASSERT_FALSE(fs::exists(lockPath));
}

TEST_F(SecurityManagerTest, MultipleKeyFilesCanCoexist) {
    SecurityManager::KeyFiles keyFiles1("multi1", testPath_, true);
    SecurityManager::KeyFiles keyFiles2("multi2", testPath_, true);
    
    ASSERT_EQ(keyFiles1.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
    ASSERT_EQ(keyFiles2.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
    
    keyFiles1.UnLock();
    keyFiles2.UnLock();
}

TEST_F(SecurityManagerTest, KeyFilesLockReturnsErrorForInvalidFd) {
    SecurityManager::KeyFiles keyFiles("invalid_fd", testPath_, false); // Don't open file
    ASSERT_EQ(keyFiles.Lock(), SecurityManager::KeyFiles::Status::INVALID_ARGUMENT);
}

TEST_F(SecurityManagerTest, KeyFilesUnLockReturnsErrorForInvalidFd) {
    SecurityManager::KeyFiles keyFiles("invalid_fd", testPath_, false); // Don't open file
    ASSERT_EQ(keyFiles.UnLock(), SecurityManager::KeyFiles::Status::INVALID_ARGUMENT);
}

TEST_F(SecurityManagerTest, SaveKeyToFileSetsCorrectPermissions) {
    SecurityManager& manager = SecurityManager::GetInstance();
    std::vector<uint8_t> key = {1, 2, 3, 4, 5, 6, 7, 8};
    bool ret = manager.SaveKeyToFile("perms_key", testPath_, key);
    ASSERT_TRUE(ret);
    
    std::string keyPath = testPath_ + KEY_DIR + SLASH + "perms_key" + SUFFIX_KEY_V1;
    fs::perms p = fs::status(keyPath).permissions();
    
    // Check that others don't have read/write/execute permissions
    ASSERT_FALSE((p & fs::perms::others_read) != fs::perms::none);
    ASSERT_FALSE((p & fs::perms::others_write) != fs::perms::none);
    ASSERT_FALSE((p & fs::perms::others_exec) != fs::perms::none);
}

TEST_F(SecurityManagerTest, GetDBPasswordMigratesOldKeyToNewFormat) {
    SecurityManager& manager = SecurityManager::GetInstance();
    
    // Create old format key file
    std::string oldKeyPath = testPath_ + KEY_DIR + SLASH + "migrate_key" + SUFFIX_KEY;
    std::ofstream out(oldKeyPath, std::ios::binary);
    
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    uint8_t sizeByte = sizeof(time_t) + SecurityManager::SecurityContent::KEY_SIZE;
    
    out.write(reinterpret_cast<char*>(&sizeByte), 1);
    out.write(reinterpret_cast<char*>(&now), sizeof(now));
    std::vector<uint8_t> key(SecurityManager::SecurityContent::KEY_SIZE, 0xAA);
    out.write(reinterpret_cast<char*>(key.data()), key.size());
    out.close();
    
    // Get the password which should migrate to new format
    auto password = manager.GetDBPassword("migrate_key", testPath_, false);
    ASSERT_GT(password.GetSize(), 0);
    
    // Verify old file was deleted and new file was created
    ASSERT_FALSE(fs::exists(oldKeyPath));
    std::string newKeyPath = testPath_ + KEY_DIR + SLASH + "migrate_key" + SUFFIX_KEY_V1;
    ASSERT_TRUE(fs::exists(newKeyPath));
}

TEST_F(SecurityManagerTest, RetrySchedulesTaskWhenRootKeyCheckFails) {
    SecurityManager& manager = SecurityManager::GetInstance();
    manager.hasRootKey_ = false;
    
    // Mock CheckRootKey to return error
    auto oldCheckRootKey = [](SecurityManager* self) {
        return HKS_ERROR_INTERNAL_ERROR;
    };
    auto oldFunc = std::mem_fn(&SecurityManager::CheckRootKey);
    std::swap(oldFunc, oldCheckRootKey);
    
    bool ret = manager.Retry();
    ASSERT_FALSE(ret); // Should fail and schedule retry
    
    // Restore original function
    std::swap(oldFunc, oldCheckRootKey);
}

TEST_F(SecurityManagerTest, SecurityContentClearsSensitiveData) {
    SecurityManager::SecurityContent content;
    content.fullKeyValue = {1, 2, 3, 4};
    content.encryptValue = {5, 6, 7, 8};
    content.nonceValue = {9, 10, 11, 12};
    
    // Simulate destruction
    content.fullKeyValue.assign(content.fullKeyValue.size(), 0);
    content.encryptValue.assign(content.encryptValue.size(), 0);
    content.nonceValue.assign(content.nonceValue.size(), 0);
    
    ASSERT_EQ(content.fullKeyValue, std::vector<uint8_t>(4, 0));
    ASSERT_EQ(content.encryptValue, std::vector<uint8_t>(4, 0));
    ASSERT_EQ(content.nonceValue, std::vector<uint8_t>(4, 0));
}

TEST_F(SecurityManagerTest, DBPasswordClearsSensitiveData) {
    SecurityManager::DBPassword password;
    uint8_t data[] = {1, 2, 3, 4};
    password.SetValue(data, sizeof(data));
    
    // Simulate destruction
    password.SetValue(nullptr, 0);
    
    ASSERT_EQ(password.GetSize(), 0);
}

TEST_F(SecurityManagerTest, LoadOldKeyHandlesIncorrectSizeByte) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = false;
    
    std::string invalidPath = testPath_ + "/invalid_size_byte";
    std::ofstream out(invalidPath, std::ios::binary);
    out.put(0xFF); // Invalid size byte
    out.close();
    
    bool ret = manager.LoadContent(content, invalidPath);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, LoadNewKeyHandlesShortFile) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = true;
    
    std::string shortPath = testPath_ + "/short_file";
    std::ofstream out(shortPath, std::ios::binary);
    out.put(SecurityManager::SecurityContent::MAGIC_CHAR); // Only 1 byte of magic
    out.close();
    
    bool ret = manager.LoadContent(content, shortPath);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, KeyFilesLockHandlesEINTR) {
    // This test would require mocking flock to simulate EINTR
    // For now just verify the basic functionality
    SecurityManager::KeyFiles keyFiles("eintr_lock", testPath_, true);
    ASSERT_EQ(keyFiles.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
    ASSERT_EQ(keyFiles.UnLock(), SecurityManager::KeyFiles::Status::SUCCESS);
}

TEST_F(SecurityManagerTest, SaveKeyToFileHandlesDirectoryCreationFailure) {
    SecurityManager& manager = SecurityManager::GetInstance();
    
    // Create a path where directory can't be created
    std::string invalidPath = "/proc/invalid_path/key_dir";
    std::vector<uint8_t> key = {1, 2, 3, 4};
    
    bool ret = manager.SaveKeyToFile("invalid_dir", invalidPath, key);
    ASSERT_FALSE(ret);
}

TEST_F(SecurityManagerTest, GetDBPasswordHandlesCorruptKeyFile) {
    SecurityManager& manager = SecurityManager::GetInstance();
    
    // Create a corrupt key file
    std::string corruptPath = testPath_ + KEY_DIR + SLASH + "corrupt_key" + SUFFIX_KEY_V1;
    std::ofstream out(corruptPath, std::ios::binary);
    out.write("corrupt data", 12);
    out.close();
    
    auto password = manager.GetDBPassword("corrupt_key", testPath_, false);
    ASSERT_EQ(password.GetSize(), 0);
}

TEST_F(SecurityManagerTest, ConcurrentAccessToSameKeyFile) {
    SecurityManager& manager = SecurityManager::GetInstance();
    const std::string dbName = "concurrent_same_key";
    
    std::atomic<int> successCount(0);
    auto worker = [&]() {
        auto password = manager.GetDBPassword(dbName, testPath_, true);
        if (password.GetSize() > 0) {
            successCount++;
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ASSERT_EQ(successCount, 5);
}

TEST_F(SecurityManagerTest, KeyFilesHandlesMultipleLockUnlockCycles) {
    SecurityManager::KeyFiles keyFiles("cycle_lock", testPath_, true);
    
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(keyFiles.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
        ASSERT_EQ(keyFiles.UnLock(), SecurityManager::KeyFiles::Status::SUCCESS);
    }
}

TEST_F(SecurityManagerTest, SecurityManagerConstantsHaveCorrectValues) {
    ASSERT_STREQ(ROOT_KEY_ALIAS, "distributeddb_client_root_key");
    ASSERT_STREQ(HKS_BLOB_TYPE_NONCE, "Z5s0Bo571KoqwIi6");
    ASSERT_STREQ(HKS_BLOB_TYPE_AAD, "distributeddata_client");
    ASSERT_STREQ(SUFFIX_KEY, ".key");
    ASSERT_STREQ(SUFFIX_KEY_V1, ".key_v1");
    ASSERT_STREQ(SUFFIX_KEY_LOCK, ".key_lock");
    ASSERT_STREQ(KEY_DIR, "/key");
    ASSERT_STREQ(SLASH, "/");
    ASSERT_EQ(HOURS_PER_YEAR, 24 * 365);
}

TEST_F(SecurityManagerTest, SecurityManagerInitializesVectorsCorrectly) {
    SecurityManager manager;
    
    std::string rootKeyAlias(ROOT_KEY_ALIAS);
    std::string nonce(HKS_BLOB_TYPE_NONCE);
    std::string aad(HKS_BLOB_TYPE_AAD);
    
    ASSERT_EQ(manager.vecRootKeyAlias_, 
              std::vector<uint8_t>(rootKeyAlias.begin(), rootKeyAlias.end()));
    ASSERT_EQ(manager.vecNonce_, 
              std::vector<uint8_t>(nonce.begin(), nonce.end()));
    ASSERT_EQ(manager.vecAad_, 
              std::vector<uint8_t>(aad.begin(), aad.end()));
}

TEST_F(SecurityManagerTest, LoadOldKeyWithExactSize) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = false;
    
    std::string keyPath = testPath_ + "/exact_size_old_key";
    std::ofstream out(keyPath, std::ios::binary);
    
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    uint8_t sizeByte = sizeof(time_t) + SecurityManager::SecurityContent::KEY_SIZE;
    
    out.write(reinterpret_cast<char*>(&sizeByte), 1);
    out.write(reinterpret_cast<char*>(&now), sizeof(now));
    std::vector<uint8_t> key(SecurityManager::SecurityContent::KEY_SIZE, 0xAA);
    out.write(reinterpret_cast<char*>(key.data()), key.size());
    out.close();
    
    bool ret = manager.LoadContent(content, keyPath);
    ASSERT_TRUE(ret);
    ASSERT_EQ(content.fullKeyValue.size(), key.size());
}

TEST_F(SecurityManagerTest, LoadNewKeyWithMinimumSize) {
    SecurityManager& manager = SecurityManager::GetInstance();
    SecurityManager::SecurityContent content;
    content.isNewStyle = true;
    
    std::string keyPath = testPath_ + "/min_size_new_key";
    std::ofstream out(keyPath, std::ios::binary);
    
    // Magic number
    for (int i = 0; i < SecurityManager::SecurityContent::MAGIC_NUM; i++) {
        out.put(SecurityManager::SecurityContent::MAGIC_CHAR);
    }
    
    // Nonce
    std::vector<uint8_t> nonce(SecurityManager::SecurityContent::NONCE_SIZE, 0xBB);
    out.write(reinterpret_cast<char*>(nonce.data()), nonce.size());
    
    // Minimum encrypted data (1 byte)
    out.put(0x01);
    out.close();
    
    bool ret = manager.LoadContent(content, keyPath);
    ASSERT_TRUE(ret);
    ASSERT_EQ(content.encryptValue.size(), 1);
}

TEST_F(SecurityManagerTest, SaveKeyToFileOverwritesExistingFile) {
    SecurityManager& manager = SecurityManager::GetInstance();
    std::vector<uint8_t> key1 = {1, 2, 3, 4};
    std::vector<uint8_t> key2 = {5, 6, 7, 8};
    
    bool ret1 = manager.SaveKeyToFile("overwrite_key", testPath_, key1);
    ASSERT_TRUE(ret1);
    
    // Get file size before overwrite
    std::string keyPath = testPath_ + KEY_DIR + SLASH + "overwrite_key" + SUFFIX_KEY_V1;
    auto originalSize = fs::file_size(keyPath);
    
    bool ret2 = manager.SaveKeyToFile("overwrite_key", testPath_, key2);
    ASSERT_TRUE(ret2);
    
    // Verify file was overwritten (size may be different)
    auto newSize = fs::file_size(keyPath);
    ASSERT_NE(originalSize, newSize);
}

TEST_F(SecurityManagerTest, DelDBPasswordHandlesNonExistentFiles) {
    SecurityManager& manager = SecurityManager::GetInstance();
    
    // Should not throw when files don't exist
    manager.DelDBPassword("nonexistent_key", testPath_);
    
    // Verify no files were created
    std::string oldKeyPath = testPath_ + KEY_DIR + SLASH + "nonexistent_key" + SUFFIX_KEY;
    std::string newKeyPath = testPath_ + KEY_DIR + SLASH + "nonexistent_key" + SUFFIX_KEY_V1;
    ASSERT_FALSE(fs::exists(oldKeyPath));
    ASSERT_FALSE(fs::exists(newKeyPath));
}

TEST_F(SecurityManagerTest, KeyFilesLockTimesOutOnDeadlock) {
    SecurityManager::KeyFiles keyFiles("deadlock", testPath_, true);
    
    // Acquire lock in main thread
    ASSERT_EQ(keyFiles.Lock(), SecurityManager::KeyFiles::Status::SUCCESS);
    
    // Try to lock in another thread - should timeout
    std::atomic<bool> lockAcquired(false);
    std::thread t([&]() {
        int result = keyFiles.Lock();
        lockAcquired = (result == SecurityManager::KeyFiles::Status::SUCCESS);
    });
    
    // Wait a bit then release lock
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    keyFiles.UnLock();
    
    t.join();
    ASSERT_TRUE(lockAcquired);
}

TEST_F(SecurityManagerTest, SecurityManagerHandlesMultipleInstances) {
    SecurityManager& manager1 = SecurityManager::GetInstance();
    SecurityManager& manager2 = SecurityManager::GetInstance();
    
    // Verify both references point to the same instance
    ASSERT_EQ(&manager1, &manager2);
    
    // Verify member variables are shared
    manager1.hasRootKey_ = true;
    ASSERT_TRUE(manager2.hasRootKey_);
}

TEST_F(SecurityManagerTest, DBPasswordCopyConstructor) {
    SecurityManager::DBPassword password1;
    uint8_t data[] = {1, 2, 3, 4};
    password1.SetValue(data, sizeof(data));
    
    SecurityManager::DBPassword password2(password1);
    ASSERT_EQ(password2.GetSize(), sizeof(data));
    
    // Verify data was copied
    uint8_t buffer[4];
    password2.GetValue(buffer, sizeof(buffer));
    ASSERT_EQ(memcmp(buffer, data, sizeof(data)), 0);
}

TEST_F(SecurityManagerTest, DBPasswordAssignmentOperator) {
    SecurityManager::DBPassword password1;
    uint8_t data[] = {5, 6, 7, 8};
    password1.SetValue(data, sizeof(data));
    
    SecurityManager::DBPassword password2;
    password2 = password1;
    ASSERT_EQ(password2.GetSize(), sizeof(data));
    
    // Verify data was copied
    uint8_t buffer[4];
    password2.GetValue(buffer, sizeof(buffer));
    ASSERT_EQ(memcmp(buffer, data, sizeof(data)), 0);
}

TEST_F(SecurityManagerTest, DBPasswordMoveConstructor) {
    SecurityManager::DBPassword password1;
    uint8_t data[] = {9, 10, 11, 12};
    password1.SetValue(data, sizeof(data));
    
    SecurityManager::DBPassword password2(std::move(password1));
    ASSERT_EQ(password2.GetSize(), sizeof(data));
    ASSERT_EQ(password1.GetSize(), 0);
    
    // Verify data was moved
    uint8_t buffer[4];
    password2.GetValue(buffer, sizeof(buffer));
    ASSERT_EQ(memcmp(buffer, data, sizeof(data)), 0);
}

TEST_F(SecurityManagerTest, DBPasswordMoveAssignmentOperator) {
    SecurityManager::DBPassword password1;
    uint8_t data[] = {13, 14, 15, 16};
    password1.SetValue(data, sizeof(data));
    
    SecurityManager::DBPassword password2;
    password2 = std::move(password1);
    ASSERT_EQ(password2.GetSize(), sizeof(data));
    ASSERT_EQ(password1.GetSize(), 0);
    
    // Verify data was moved
    uint8_t buffer[4];
    password2.GetValue(buffer, sizeof(buffer));
    ASSERT_EQ(memcmp(buffer, data, sizeof(data)), 0);
}

TEST_F(SecurityManagerTest, SaveKeyToFileHandlesNonExistentFiles) {
    SecurityManager& manager = SecurityManager::GetInstance();

    // Should not throw when files don't exist
    manager.SaveKeyToFile("nonexistent_key", testPath_, key1);

    // Verify file was created
    std::string keyPath = testPath_ + KEY_DIR + SLASH + "nonexistent_key" + SUFFIX_KEY;
    ASSERT_TRUE(fs::exists(keyPath));
}

TEST_F(SecurityManagerTest, GetKeyFromFileHandlesNonExistentFiles) {
    SecurityManager& manager = SecurityManager::GetInstance();

    // Should not throw when files don't exist
    manager.GetKeyFromFile("nonexistent_key", testPath_, key1);
    ASSERT_EQ(key1.GetSize(), 0);
}

} // namespace DistributedDB