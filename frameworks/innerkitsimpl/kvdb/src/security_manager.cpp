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
#define LOG_TAG "SECURITYMANAGER"
#include "security_manager.h"

#include <chrono>
#include <fcntl.h>
#include <limits>
#include <random>
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

SecurityManager::DBPassword SecurityManager::GetDBPassword(const std::string &name,
    const std::string &path, bool needCreate)
{
    DBPassword dbPassword;
    KeyFiles keyFiles(name, path);
    KeyFilesAutoLock fileLock(keyFiles);
    auto secKey = LoadKeyFromFile(name, path, dbPassword.isKeyOutdated);
    std::vector<uint8_t> key{};

    if (secKey.empty() && needCreate) {
        key = Random(KEY_SIZE);
        if (!SaveKeyToFile(name, path, key)) {
            secKey.assign(secKey.size(), 0);
            key.assign(key.size(), 0);
            return dbPassword;
        }
    }

    if ((!secKey.empty() && Decrypt(secKey, key)) || !key.empty()) {
        dbPassword.SetValue(key.data(), key.size());
    }

    secKey.assign(secKey.size(), 0);
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

std::vector<uint8_t> SecurityManager::Random(int32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (int32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

std::vector<uint8_t> SecurityManager::LoadKeyFromFile(const std::string &name, const std::string &path,
    bool &isOutdated)
{
    auto keyPath = path + KEY_DIR + SLASH + name + SUFFIX_KEY;
    if (!FileExists(keyPath)) {
        return {};
    }
    StoreUtil::RemoveRWXForOthers(path + KEY_DIR);

    std::vector<char> content;
    auto loaded = LoadBufferFromFile(keyPath, content);
    if (!loaded) {
        return {};
    }

    if (content.size() < (sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE + 1) {
        return {};
    }

    size_t offset = 0;
    if (content[offset] != char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE)) {
        return {};
    }

    offset++;
    std::vector<uint8_t> date;
    date.assign(content.begin() + offset, content.begin() + (sizeof(time_t) / sizeof(uint8_t)) + offset);
    isOutdated = IsKeyOutdated(date);
    offset += (sizeof(time_t) / sizeof(uint8_t));
    std::vector<uint8_t> key{ content.begin() + offset, content.end() };
    content.assign(content.size(), 0);
    return key;
}

bool SecurityManager::SaveKeyToFile(const std::string &name, const std::string &path, std::vector<uint8_t> &key)
{
    if (!hasRootKey_ && !Retry()) {
        ZLOGE("Failed! no root key and generation failed");
        return false;
    }
    auto secretKey = Encrypt(key);
    auto keyPath = path + KEY_DIR;
    StoreUtil::InitPath(keyPath);
    std::vector<char> content;
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    content.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    content.insert(content.end(), date.begin(), date.end());
    content.insert(content.end(), secretKey.begin(), secretKey.end());
    auto keyFullPath = keyPath + SLASH + name + SUFFIX_KEY;
    auto ret = SaveBufferToFile(keyFullPath, content);
    if (access(keyFullPath.c_str(), F_OK) == 0) {
        StoreUtil::RemoveRWXForOthers(keyFullPath);
    }
    content.assign(content.size(), 0);
    if (!ret) {
        ZLOGE("Client SaveSecretKey failed!");
        return false;
    }
    return ret;
}

std::vector<uint8_t> SecurityManager::Encrypt(const std::vector<uint8_t> &key)
{
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), vecAad_.data() };
    struct HksBlob blobNonce = { uint32_t(vecNonce_.size()), vecNonce_.data() };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksBlob plainKey = { uint32_t(key.size()), const_cast<uint8_t *>(key.data()) };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
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
        ZLOGE("HksAddParams failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return {};
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return {};
    }

    uint8_t cipherBuf[256] = { 0 };
    struct HksBlob cipherText = { sizeof(cipherBuf), cipherBuf };
    ret = HksEncrypt(&rootKeyName, params, &plainKey, &cipherText);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksEncrypt failed, status: %{public}d", ret);
        return {};
    }
    std::vector<uint8_t> encryptedKey(cipherText.data, cipherText.data + cipherText.size);
    (void)memset_s(cipherBuf, sizeof(cipherBuf), 0, sizeof(cipherBuf));
    return encryptedKey;
}

bool SecurityManager::Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key)
{
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), &(vecAad_[0]) };
    struct HksBlob blobNonce = { uint32_t(vecNonce_.size()), &(vecNonce_[0]) };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), &(vecRootKeyAlias_[0]) };
    struct HksBlob encryptedKeyBlob = { uint32_t(source.size()), source.data() };

    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return false;
    }
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
    ret = HksDecrypt(&rootKeyName, params, &encryptedKeyBlob, &plainKeyBlob);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksDecrypt, status: %{public}d", ret);
        return false;
    }

    key.assign(plainKeyBlob.data, plainKeyBlob.data + plainKeyBlob.size);
    (void)memset_s(plainBuf, sizeof(plainBuf), 0, sizeof(plainBuf));
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
