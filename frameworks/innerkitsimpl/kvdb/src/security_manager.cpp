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
#define LOG_TAG "SecurityManager"

#include "security_manager.h"

#include <chrono>
#include <fcntl.h>
#include <limits>
#include <sys/file.h>
#include <unistd.h>

#include "file_ex.h"
#include "hks_api.h"
#include "hks_param.h"
#include "log_print.h"
#include "securec.h"
#include "store_types.h"
#include "store_util.h"
#include "task_executor.h"

namespace OHOS::DistributedKv {
static constexpr int HOURS_PER_YEAR = (24 * 365);
static constexpr const char *ROOT_KEY_ALIAS = "distributeddb_client_root_key";
static constexpr const char *HKS_BLOB_TYPE_NONCE = "Z5s0Bo571KoqwIi6";
static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata_client";
static constexpr const char *SUFFIX_KEY = ".key";
static constexpr const char *SUFFIX_TMP_KEY = ".key_bk";
static constexpr const char *SUFFIX_KEY_LOCK = ".key_lock";
static constexpr const char *KEY_DIR = "/key";
static constexpr const char *SLASH = "/";

SecurityManager::SecurityManager()
{
    vecRootKeyAlias_ = std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
    vecNonce_ = std::vector<uint8_t>(HKS_BLOB_TYPE_NONCE, HKS_BLOB_TYPE_NONCE + strlen(HKS_BLOB_TYPE_NONCE));
    vecAad_ = std::vector<uint8_t>(HKS_BLOB_TYPE_AAD, HKS_BLOB_TYPE_AAD + strlen(HKS_BLOB_TYPE_AAD));
}

SecurityManager::~SecurityManager()
{}

SecurityManager &SecurityManager::GetInstance()
{
    static SecurityManager instance;
    return instance;
}

bool SecurityManager::Retry()
{
    auto status = CheckRootKey();
    if (status == HKS_SUCCESS) {
        hasRootKey_ = true;
        ZLOGE("Root key already exist.");
        return true;
    }

    if (status == HKS_ERROR_NOT_EXIST && GenerateRootKey() == HKS_SUCCESS) {
        hasRootKey_ = true;
        ZLOGE("GenerateRootKey success.");
        return true;
    }

    constexpr int32_t interval = 100;
    TaskExecutor::GetInstance().Schedule(std::chrono::milliseconds(interval), [this] {
        Retry();
    });
    return false;
}

std::vector<uint8_t> SecurityManager::Random(int32_t length)
{
    std::vector<uint8_t> value(length, 0);
    struct HksBlob blobValue = { .size = length, .data = &(value[0]) };
    auto ret = HksGenerateRandom(nullptr, &blobValue);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksGenerateRandom failed, status: %{public}d", ret);
        return {};
    }
    return value;
}

bool SecurityManager::LoadContent(SecurityManager::SecurityContent &content, const std::string &path,
    const std::string &tempPath, bool isTemp)
{
    std::string realPath = isTemp ? tempPath : path;
    if (!FileExists(realPath)) {
        return false;
    }
    content = LoadKeyFromFile(realPath);
    if (content.encryptValue.empty() || !Decrypt(content)) {
        return false;
    }
    if (isTemp) {
        StoreUtil::Rename(tempPath, path);
    } else {
        StoreUtil::Remove(tempPath);
    }
    return true;
}

SecurityManager::DBPassword SecurityManager::GetDBPassword(const std::string &name, const std::string &path,
    bool needCreate)
{
    KeyFiles keyFiles(name, path);
    KeyFilesAutoLock fileLock(keyFiles);
    DBPassword dbPassword;
    auto keyPath = path + KEY_DIR + SLASH + name + SUFFIX_KEY;
    auto tempKeyPath = path + KEY_DIR + SLASH + name + SUFFIX_TMP_KEY;
    SecurityContent content;
    auto result = LoadContent(content, keyPath, tempKeyPath, false);
    if (!result) {
        result = LoadContent(content, keyPath, tempKeyPath, true);
    }
    content.encryptValue.assign(content.encryptValue.size(), 0);
    std::vector<uint8_t> key;
    if (result) {
        size_t offset = 0;
        if (content.isNewStyle && content.fullKeyValue.size() > (sizeof(time_t) / sizeof(uint8_t)) + 1) {
            content.version = content.fullKeyValue[offset++];
            content.time.assign(content.fullKeyValue.begin() + offset,
                content.fullKeyValue.begin() + offset + (sizeof(time_t) / sizeof(uint8_t)));
        }
        offset = content.isNewStyle ? (sizeof(time_t) / sizeof(uint8_t)) + 1 : 0;
        if (content.fullKeyValue.size() > offset) {
            key.assign(content.fullKeyValue.begin() + offset, content.fullKeyValue.end());
        }
        // old security key file and update key file
        if (content.nonceValue.empty() && !key.empty()) {
            SaveKeyToFile(name, path, key);
        }
        content.fullKeyValue.assign(content.fullKeyValue.size(), 0);
    }
    if (!result && needCreate) {
        StoreUtil::Remove(keyPath);
        StoreUtil::Remove(tempKeyPath);
        key = Random(SecurityContent::KEY_SIZE);
        if (key.empty() || !SaveKeyToFile(name, path, key)) {
            key.assign(key.size(), 0);
            return dbPassword;
        }
    }
    if (!content.time.empty()) {
        dbPassword.isKeyOutdated = IsKeyOutdated(content.time);
    }
    dbPassword.SetValue(key.data(), key.size());
    key.assign(key.size(), 0);
    return dbPassword;
}

bool SecurityManager::SaveDBPassword(const std::string &name, const std::string &path,
    const DistributedDB::CipherPassword &key)
{
    KeyFiles keyFiles(name, path);
    KeyFilesAutoLock fileLock(keyFiles);
    std::vector<uint8_t> pwd(key.GetData(), key.GetData() + key.GetSize());
    auto result = SaveKeyToFile(name, path, pwd);
    pwd.assign(pwd.size(), 0);
    return result;
}

void SecurityManager::DelDBPassword(const std::string &name, const std::string &path)
{
    KeyFiles keyFiles(name, path);
    KeyFilesAutoLock fileLock(keyFiles);
    auto keyPath = keyFiles.GetKeyFilePath();
    StoreUtil::Remove(keyPath);
    fileLock.UnLockAndDestroy();
}

SecurityManager::SecurityContent SecurityManager::LoadKeyFromFile(const std::string &path)
{
    SecurityContent securityContent;
    std::vector<char> content;
    auto loaded = LoadBufferFromFile(path, content);
    if (!loaded) {
        return securityContent;
    }
    if (content.size() < SecurityContent::MAGIC_NUM) {
        return securityContent;
    }
    for (size_t index = 0; index < SecurityContent::MAGIC_NUM; ++index) {
        if (content[index] != char(SecurityContent::MAGIC_CHAR)) {
            securityContent.isNewStyle = false;
        }
    }
    if (securityContent.isNewStyle) {
        LoadNewKey(content, securityContent);
    } else {
        LoadOldKey(content, securityContent);
    }
    content.assign(content.size(), 0);
    return securityContent;
}

void SecurityManager::LoadNewKey(const std::vector<char> &content, SecurityManager::SecurityContent &securityContent)
{
    if (content.size() < SecurityContent::MAGIC_NUM + SecurityContent::NONCE_SIZE + 1) {
        return;
    }
    size_t offset = SecurityContent::MAGIC_NUM;
    securityContent.nonceValue.assign(content.begin() + offset, content.begin() + offset + SecurityContent::NONCE_SIZE);
    offset += SecurityContent::NONCE_SIZE;
    securityContent.encryptValue.assign(content.begin() + offset, content.end());
}

void SecurityManager::LoadOldKey(const std::vector<char> &content, SecurityManager::SecurityContent &securityContent)
{
    size_t offset = 0;
    if (content.size() < (sizeof(time_t) / sizeof(uint8_t)) + SecurityContent::KEY_SIZE + 1) {
        return;
    }
    if (content[offset] != char((sizeof(time_t) / sizeof(uint8_t)) + SecurityContent::KEY_SIZE)) {
        return;
    }
    ++offset;
    securityContent.time.assign(content.begin() + offset, content.begin() +
        (sizeof(time_t) / sizeof(uint8_t)) + offset);
    offset += (sizeof(time_t) / sizeof(uint8_t));
    securityContent.encryptValue.assign(content.begin() + offset, content.end());
}

bool SecurityManager::SaveKeyToFile(const std::string &name, const std::string &path,
    std::vector<uint8_t> &key)
{
    if (!hasRootKey_ && !Retry()) {
        ZLOGE("Failed! no root key and generation failed");
        return false;
    }
    SecurityContent securityContent;
    securityContent.version = SecurityContent::CURRENT_VERSION;
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    securityContent.time = { reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time) };
    std::vector<uint8_t> keyContent;
    keyContent.push_back(securityContent.version);
    keyContent.insert(keyContent.end(), securityContent.time.begin(), securityContent.time.end());
    keyContent.insert(keyContent.end(), key.begin(), key.end());
    if (!Encrypt(keyContent, securityContent)) {
        keyContent.assign(keyContent.size(), 0);
        return false;
    }
    keyContent.assign(keyContent.size(), 0);
    auto keyPath = path + KEY_DIR;
    StoreUtil::InitPath(keyPath);
    std::vector<char> content;
    for (size_t index = 0; index < SecurityContent::MAGIC_NUM; ++index) {
        content.push_back(char(SecurityContent::MAGIC_CHAR));
    }
    content.insert(content.end(), securityContent.nonceValue.begin(), securityContent.nonceValue.end());
    content.insert(content.end(), securityContent.encryptValue.begin(), securityContent.encryptValue.end());
    auto keyTempPath = keyPath + SLASH + name + SUFFIX_TMP_KEY;
    auto keyFullPath = keyPath + SLASH + name + SUFFIX_KEY;
    auto ret = SaveBufferToFile(keyTempPath, content);
    content.assign(content.size(), 0);
    if (!ret) {
        ZLOGE("Save key to file fail, ret:%{public}d", ret);
        return false;
    }
    if (StoreUtil::Rename(keyTempPath, keyFullPath)) {
        StoreUtil::RemoveRWXForOthers(keyFullPath);
    }
    return ret;
}

bool SecurityManager::Encrypt(const std::vector<uint8_t> &key, SecurityManager::SecurityContent &content)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return false;
    }
    content.nonceValue = Random(SecurityContent::NONCE_SIZE);
    if (content.nonceValue.empty()) {
        return false;
    }
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_NONCE, .blob = { SecurityContent::NONCE_SIZE, &(content.nonceValue[0]) } },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { uint32_t(vecAad_.size()), &(vecAad_[0]) } },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };
    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    uint8_t cipherBuf[256] = { 0 };
    struct HksBlob cipherText = { sizeof(cipherBuf), cipherBuf };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksBlob plainKey = { uint32_t(key.size()), const_cast<uint8_t *>(key.data()) };
    ret = HksEncrypt(&rootKeyName, params, &plainKey, &cipherText);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksEncrypt failed, status: %{public}d", ret);
        return false;
    }
    content.encryptValue = std::vector<uint8_t>(cipherText.data, cipherText.data + cipherText.size);
    std::fill(cipherBuf, cipherBuf + sizeof(cipherBuf), 0);
    return true;
}

bool SecurityManager::Decrypt(SecurityManager::SecurityContent &content)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return false;
    }
    struct HksBlob blobNonce = { .size = uint32_t(vecNonce_.size()), .data = &(vecNonce_[0]) };
    if (!(content.nonceValue.empty())) {
        blobNonce.size = uint32_t(content.nonceValue.size());
        blobNonce.data = const_cast<uint8_t*>(&(content.nonceValue[0]));
    }
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = { uint32_t(vecAad_.size()), &(vecAad_[0]) } },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };
    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    uint8_t plainBuf[256] = { 0 };
    struct HksBlob plainKeyBlob = { sizeof(plainBuf), plainBuf };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), &(vecRootKeyAlias_[0]) };
    struct HksBlob encryptedKeyBlob = { uint32_t(content.encryptValue.size()),
        const_cast<uint8_t *>(content.encryptValue.data()) };
    ret = HksDecrypt(&rootKeyName, params, &encryptedKeyBlob, &plainKeyBlob);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksDecrypt, status: %{public}d", ret);
        return false;
    }
    content.fullKeyValue.assign(plainKeyBlob.data, plainKeyBlob.data + plainKeyBlob.size);
    std::fill(plainBuf, plainBuf + sizeof(plainBuf), 0);
    return true;
}

int32_t SecurityManager::GenerateRootKey()
{
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return ret;
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
        ZLOGE("HksAddParams failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return ret;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return ret;
    }

    ret = HksGenerateKey(&rootKeyName, params, nullptr);
    HksFreeParamSet(&params);
    ZLOGI("HksGenerateKey status: %{public}d", ret);
    return ret;
}

int32_t SecurityManager::CheckRootKey()
{
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return ret;
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
        ZLOGE("HksAddParams failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return ret;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return ret;
    }

    ret = HksKeyExist(&rootKeyName, params);
    HksFreeParamSet(&params);
    ZLOGI("HksKeyExist status: %{public}d", ret);
    return ret;
}

bool SecurityManager::IsKeyOutdated(const std::vector<uint8_t> &date)
{
    time_t time = *reinterpret_cast<time_t *>(const_cast<uint8_t *>(&date[0]));
    auto createTime = std::chrono::system_clock::from_time_t(time);
    return ((createTime + std::chrono::hours(HOURS_PER_YEAR)) < std::chrono::system_clock::now());
}

SecurityManager::KeyFiles::KeyFiles(const std::string &name, const std::string &path, bool openFile)
{
    keyPath_ = path + KEY_DIR + SLASH + name + SUFFIX_KEY;
    lockFile_ = path + KEY_DIR + SLASH + name + SUFFIX_KEY_LOCK;
    StoreUtil::InitPath(path + KEY_DIR);
    if (!openFile) {
        return;
    }
    lockFd_ = open(lockFile_.c_str(), O_RDONLY | O_CREAT, S_IRWXU | S_IRWXG);
    if (lockFd_ < 0) {
        ZLOGE("Open failed, errno:%{public}d, path:%{public}s", errno, StoreUtil::Anonymous(lockFile_).c_str());
    }
}

SecurityManager::KeyFiles::~KeyFiles()
{
    if (lockFd_ < 0) {
        return;
    }
    close(lockFd_);
    lockFd_ = -1;
}

const std::string &SecurityManager::KeyFiles::GetKeyFilePath()
{
    return keyPath_;
}

int32_t SecurityManager::KeyFiles::Lock()
{
    return FileLock(LOCK_EX);
}

int32_t SecurityManager::KeyFiles::UnLock()
{
    return FileLock(LOCK_UN);
}

int32_t SecurityManager::KeyFiles::DestroyLock()
{
    if (lockFd_ > 0) {
        close(lockFd_);
        lockFd_ = -1;
    }
    StoreUtil::Remove(lockFile_);
    return Status::SUCCESS;
}

int32_t SecurityManager::KeyFiles::FileLock(int32_t lockType)
{
    if (lockFd_ < 0) {
        return Status::INVALID_ARGUMENT;
    }
    int32_t errCode = 0;
    do {
        errCode = flock(lockFd_, lockType);
    } while (errCode < 0 && errno == EINTR);
    if (errCode < 0) {
        ZLOGE("This flock is failed, type:%{public}d, errno:%{public}d, path:%{public}s", lockType, errno,
            StoreUtil::Anonymous(lockFile_).c_str());
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

SecurityManager::KeyFilesAutoLock::KeyFilesAutoLock(KeyFiles& keyFiles) : keyFiles_(keyFiles)
{
    keyFiles_.Lock();
}

SecurityManager::KeyFilesAutoLock::~KeyFilesAutoLock()
{
    keyFiles_.UnLock();
}

int32_t SecurityManager::KeyFilesAutoLock::UnLockAndDestroy()
{
    keyFiles_.UnLock();
    return keyFiles_.DestroyLock();
}
} // namespace OHOS::DistributedKv
