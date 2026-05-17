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
#include <dlfcn.h>
#include <fcntl.h>
#include <limits>
#include <sys/file.h>
#include <unistd.h>

#include "file_ex.h"

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
static constexpr const char *SUFFIX_KEY_V1 = ".key_v1";
static constexpr const char *SUFFIX_KEY_LOCK = ".key_lock";
static constexpr const char *KEY_DIR = "/key";
static constexpr const char *SLASH = "/";

using Creator = std::shared_ptr<OHOS::DistributedKv::KVDBCrypto> (*)(const std::vector<uint8_t> &rootKeyAlias,
    const std::vector<uint8_t> vecAad);
using GenerateRandomNumFunc = std::vector<uint8_t> (*)(const uint32_t);

SecurityManager::SecurityManager()
{
    vecRootKeyAlias_ = std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
    vecNonce_ = std::vector<uint8_t>(HKS_BLOB_TYPE_NONCE, HKS_BLOB_TYPE_NONCE + strlen(HKS_BLOB_TYPE_NONCE));
    vecAad_ = std::vector<uint8_t>(HKS_BLOB_TYPE_AAD, HKS_BLOB_TYPE_AAD + strlen(HKS_BLOB_TYPE_AAD));
    (void)GetDelegate();
}

SecurityManager::~SecurityManager()
{
    kvdbCrypto_ = nullptr;
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
    }
}

SecurityManager &SecurityManager::GetInstance()
{
    static SecurityManager instance;
    return instance;
}

void* SecurityManager::GetHandle()
{
    std::lock_guard<std::mutex> lock(handleMutex_);
    if (handle_ == nullptr) {
        handle_ = dlopen("libkv_store_crypt.z.so", RTLD_LAZY);
        if (handle_ == nullptr) {
            ZLOGE("dlopen crypto so failed errno is %{public}d", errno);
        }
    }
    return handle_;
}

std::shared_ptr<KVDBCrypto> SecurityManager::CreateDelegate(const std::vector<uint8_t> &rootKeyAlias,
    const std::vector<uint8_t> vecAad)
{
    auto handle = GetHandle();
    if (handle == nullptr) {
        return nullptr;
    }
    auto creator = reinterpret_cast<Creator>(dlsym(handle, "CreateKvdbCryptoDelegate"));
    if (creator == nullptr) {
        ZLOGE("dlsym CreateKvdbCryptoDelegate failed, errno:%{public}d", errno);
        return nullptr;
    }
    return creator(rootKeyAlias, vecAad);
}

std::shared_ptr<KVDBCrypto> SecurityManager::GetDelegate()
{
    std::lock_guard<std::mutex> lock(cryptoMutex_);
    if (kvdbCrypto_ != nullptr) {
        return kvdbCrypto_;
    }
    kvdbCrypto_ = CreateDelegate(vecRootKeyAlias_, vecAad_);
    if (kvdbCrypto_ == nullptr) {
        return nullptr;
    }
    return kvdbCrypto_;
}

std::vector<uint8_t> SecurityManager::GenerateRandomNum(uint32_t length)
{
    auto handle = GetHandle();
    if (handle == nullptr) {
        return {};
    }
    auto generateRandomNum = reinterpret_cast<GenerateRandomNumFunc>(dlsym(handle, "GenerateKvdbRandomNum"));
    if (generateRandomNum == nullptr) {
        ZLOGE("dlsym GenerateRandomNum failed, errno:%{public}d", errno);
        return {};
    }
    return generateRandomNum(length);
}

bool SecurityManager::Retry()
{
    auto kvdbCrypto = GetDelegate();
    if (kvdbCrypto == nullptr) {
        return false;
    }
    if (kvdbCrypto->CheckRootKey()) {
        hasRootKey_ = true;
        ZLOGE("Root key already exist.");
        return true;
    }
    if (kvdbCrypto->GenerateRootKey()) {
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

bool SecurityManager::LoadContent(SecurityManager::SecurityContent &content, const std::string &path)
{
    if (!FileExists(path)) {
        return false;
    }
    LoadKeyFromFile(path, content);
    if (content.encryptValue.empty()) {
        return false;
    }
    auto kvdbCrypto = GetDelegate();
    if (kvdbCrypto == nullptr) {
        return false;
    }
    KVDBCryptoParam param;
    param.keyValue = content.encryptValue;
    param.nonceValue = content.nonceValue.empty() ? vecNonce_ : content.nonceValue;
    content.fullKeyValue = kvdbCrypto->Decrypt(param);
    return content.fullKeyValue.empty() ? false : true;
}

SecurityManager::DBPassword SecurityManager::GetDBPassword(const std::string &name, const std::string &path,
    bool needCreate)
{
    KeyFiles keyFiles(name, path);
    KeyFilesAutoLock fileLock(keyFiles);
    DBPassword dbPassword;
    auto oldKeyPath = path + KEY_DIR + SLASH + name + SUFFIX_KEY;
    auto newKeyPath = path + KEY_DIR + SLASH + name + SUFFIX_KEY_V1;
    SecurityContent content;
    auto result = LoadContent(content, newKeyPath);
    if (!result) {
        content.isNewStyle = false;
        result = LoadContent(content, oldKeyPath);
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
        if (!content.isNewStyle && !key.empty() && SaveKeyToFile(name, path, key)) {
            StoreUtil::Remove(oldKeyPath);
        }
        content.fullKeyValue.assign(content.fullKeyValue.size(), 0);
    }
    if (!result && needCreate) {
        key = GenerateRandomNum(SecurityContent::KEY_SIZE);
        if (key.empty() || !SaveKeyToFile(name, path, key)) {
            key.assign(key.size(), 0);
            return dbPassword;
        }
        StoreUtil::Remove(oldKeyPath);
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
    auto oldKeyPath = path + KEY_DIR + SLASH + name + SUFFIX_KEY;
    auto newKeyPath = path + KEY_DIR + SLASH + name + SUFFIX_KEY_V1;
    StoreUtil::Remove(oldKeyPath);
    StoreUtil::Remove(newKeyPath);
    fileLock.UnLockAndDestroy();
}

void SecurityManager::LoadKeyFromFile(const std::string &path, SecurityManager::SecurityContent &securityContent)
{
    std::vector<char> content;
    auto loaded = LoadBufferFromFile(path, content);
    if (!loaded) {
        return;
    }
    if (securityContent.isNewStyle) {
        LoadNewKey(content, securityContent);
    } else {
        LoadOldKey(content, securityContent);
    }
    content.assign(content.size(), 0);
}

void SecurityManager::LoadNewKey(const std::vector<char> &content, SecurityManager::SecurityContent &securityContent)
{
    if (content.size() < SecurityContent::MAGIC_NUM + SecurityContent::NONCE_SIZE + 1) {
        return;
    }
    for (size_t index = 0; index < SecurityContent::MAGIC_NUM; ++index) {
        if (content[index] != char(SecurityContent::MAGIC_CHAR)) {
            return;
        }
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
    auto kvdbCrypto = GetDelegate();
    if (kvdbCrypto == nullptr || (!hasRootKey_ && !Retry())) {
        ZLOGE("Failed! kvdbCrypto is nullptr, or not root key and generation failed");
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
    KVDBCryptoParam param;
    param.keyValue = keyContent;
    param.nonceValue = GenerateRandomNum(SecurityContent::NONCE_SIZE);
    if (param.nonceValue.empty()) {
        return false;
    }
    auto encryptKey = kvdbCrypto->Encrypt(param);
    keyContent.assign(keyContent.size(), 0);
    if (encryptKey.empty()) {
        return false;
    }
    auto keyPath = path + KEY_DIR;
    if (!StoreUtil::InitPath(keyPath)) {
        ZLOGE("Init keyPath:%{public}s failed", StoreUtil::Anonymous(keyPath).c_str());
        return false;
    }
    auto keyFullPath = keyPath + SLASH + name + SUFFIX_KEY_V1;
    auto fd = open(keyFullPath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        ZLOGE("Create file failed, ret:%{public}d", errno);
        return false;
    }
    std::string content(SecurityContent::MAGIC_NUM, static_cast<char>(SecurityContent::MAGIC_CHAR));
    content.append(reinterpret_cast<const char *>(param.nonceValue.data()), param.nonceValue.size());
    content.append(reinterpret_cast<const char *>(encryptKey.data()), encryptKey.size());
    auto ret = SaveStringToFd(fd, content);
    std::fill(content.begin(), content.end(), '\0');
    close(fd);
    if (!ret) {
        ZLOGE("Save key to file fail, ret:%{public}d", ret);
        return false;
    }
    StoreUtil::RemoveRWXForOthers(keyFullPath);
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
    lockFile_ = path + KEY_DIR + SLASH + name + SUFFIX_KEY_LOCK;
    StoreUtil::InitPath(path + KEY_DIR);
    if (!openFile) {
        return;
    }
    lockFd_ = open(lockFile_.c_str(), O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
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
