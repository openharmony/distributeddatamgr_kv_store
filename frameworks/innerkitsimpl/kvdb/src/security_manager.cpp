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
#include <limits>
#include <random>
#include <unistd.h>
#include <chrono>
#include <store_types.h>
#include "log_print.h"
#include "hks_api.h"
#include "hks_param.h"
#include "file_ex.h"
#include "securec.h"
#include "store_util.h"
#include "task_executor.h"
namespace OHOS::DistributedKv {
namespace {
    constexpr const char *REKEY_STORE_POSTFIX = ".bak";
    constexpr const char *REKEY_OLD = ".old";
    constexpr const char *REKEY_NEW = ".new";
}
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
        ZLOGE("root key already exist.");
        return true;
    }

    if (status == HKS_ERROR_NOT_EXIST && GenerateRootKey() == HKS_SUCCESS) {
        hasRootKey_ = true;
        ZLOGE("GenerateRootKey success.");
        return true;
    }

    constexpr int32_t interval = 100;
    TaskExecutor::GetInstance().Execute([this] { Retry(); }, interval);
    return false;
}

SecurityManager::DBPasswordData SecurityManager::GetDBPassword(const std::string &name,
    const std::string &path, bool needCreate)
{
    DBPasswordData passwordData;
    auto secKey = LoadKeyFromFile(name, path, passwordData.isKeyOutdated);
    ZLOGE("name and path and needCreate is: %{public}s, %{public}s, %{public}d",
        name.c_str(), path.c_str(), needCreate);
    if (secKey.empty()) {
        if (!needCreate) {
            return {false, DBPassword()};
        } else {
            secKey = Random(KEY_SIZE);
            SaveKeyToFile(name, path, secKey);
        }
    }

    passwordData.password.SetValue(secKey.data(), secKey.size());
    secKey.assign(secKey.size(), 0);
    return passwordData;
}

bool SecurityManager::SaveDBPassword(const std::string &name, const std::string &path,
    const SecurityManager::DBPassword &key)
{
    std::vector<uint8_t> pwd(key.GetData(), key.GetData() + key.GetSize());
    auto result = SaveKeyToFile(name, path, pwd);
    pwd.assign(pwd.size(), 0);
    return result;
}

void SecurityManager::DelDBPassword(const std::string &name, const std::string &path)
{
    auto keyPath = path + "/key/" + name + ".key";
    StoreUtil::Remove(keyPath);
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
                                                      bool &isKeyOutdated)
{
    auto keyPath = path + "/key/" + name + ".key";
    if (!FileExists(keyPath)) {
        return {};
    }

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
    isKeyOutdated = IsKeyOutdated(date);
    ZLOGE("isKeyOutdated is: %{public}d", isKeyOutdated);

    offset += (sizeof(time_t) / sizeof(uint8_t));
    std::vector<uint8_t> key{content.begin() + offset, content.end()};
    ZLOGE("run in key load %{public}d", key.size());
    std::vector<uint8_t> secretKey {};
    content.assign(content.size(), 0);
    if(!Decrypt(key, secretKey)) {
        ZLOGE("Decrypt failed");
        return {};
    }
    ZLOGE("run in secretKey load %{public}d", secretKey.size());
    return secretKey;
}

bool SecurityManager::SaveKeyToFile(const std::string &name, const std::string &path, std::vector<uint8_t> &key)
{
    if (!hasRootKey_ && !Retry()) {
        ZLOGE("failed! no root key and generation failed");
        return false;
    }

    auto secretKey = Encrypt(key);
    auto keyPath = path + "/key";
    StoreUtil::InitPath(keyPath);
    std::vector<char> content;
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::system_clock::now());
    std::vector<uint8_t> date(reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time));
    content.push_back(char((sizeof(time_t) / sizeof(uint8_t)) + KEY_SIZE));
    content.insert(content.end(), date.begin(), date.end());
    content.insert(content.end(), secretKey.begin(), secretKey.end());
    auto keyFullPath = keyPath+ "/" + name + ".key";
    auto ret = SaveBufferToFile(keyFullPath, content);
    content.assign(content.size(), 0);
    if (!ret) {
        ZLOGE("client SaveSecretKey failed!");
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
    if (ret != HKS_SUCCESS) {
        ZLOGE("IsExistRootKey HksEncrypt-client failed with error %{public}d", ret);
    }
    return ret == HKS_SUCCESS;
}

bool SecurityManager::IsKeyOutdated(const std::vector<uint8_t> &date)
{
    std::vector<uint8_t> timeVec(date);
    auto createTime = TransferByteArrayToType<time_t>(timeVec);
    std::chrono::system_clock::time_point createTimePointer = std::chrono::system_clock::from_time_t(createTime);
    auto oneYearLater = std::chrono::system_clock::to_time_t(createTimePointer + std::chrono::hours(525600));
    std::chrono::system_clock::time_point currentTimePointer = std::chrono::system_clock::now();
    auto currentTime = std::chrono::system_clock::to_time_t(currentTimePointer);
    return (oneYearLater > currentTime);
}

bool SecurityManager::ReKey(const std::string &name, const std::string &path, DBPasswordData &passwordData,
                            const std::shared_ptr<DBManager>& dbManager, DBOption &dbOption)
{
    int32_t rekeyTimes = 0;
    DBStatus status = DBStatus::DB_ERROR;
    DBStore *kvStore = nullptr;
    bool isRekeySuccess = false;
    while (passwordData.isKeyOutdated && rekeyTimes < REKET_TIMES) {
        IsKeyValid(name, status, kvStore, dbManager, dbOption);
        if (ExecuteRekey(name, path, passwordData, dbManager, kvStore) == Status::SUCCESS) {
            isRekeySuccess = true;
            return isRekeySuccess;
        } else {
            RekeyRecover(name, path, passwordData, dbManager, dbOption);
            rekeyTimes++;
        }
    }
    if (rekeyTimes == REKET_TIMES) {
        isRekeySuccess = false;
    }
    return isRekeySuccess;
}

void SecurityManager::RekeyRecover(const std::string &name, const std::string &path, DBPasswordData &passwordData,
    const std::shared_ptr<DBManager>& dbManager, DBOption &dbOption)
{
    DBStatus status = DBStatus::DB_ERROR;
    DBStore *kvStore = nullptr;
    auto isKeyValid = IsKeyValid(name, status, kvStore, dbManager, dbOption);
    if (isKeyValid) {
        return;
    }
    auto newKeyName = name + REKEY_NEW;
    auto newKeyPath = path + "/rekey";
    auto newPasswordData = GetDBPassword(newKeyName, newKeyPath);
    dbOption.passwd = newPasswordData.password;
    isKeyValid = IsKeyValid(name, status, kvStore, dbManager, dbOption);
    if (isKeyValid) {
        passwordData.password = newPasswordData.password;
        SaveDBPassword(name, path, newPasswordData.password);
        StoreUtil::Remove(path + "/rekey");
    }
}

bool SecurityManager::IsKeyValid(const std::string &name, DBStatus status, DBStore *kvStore,
    const std::shared_ptr<DBManager>& dbManager, DBOption &dbOption)
{
    bool isKeyValid = true;
    dbManager->GetKvStore(name, dbOption, [&status, &kvStore, &isKeyValid](auto dbStatus, auto *dbStore) {
        status = dbStatus;
        kvStore = dbStore;
        if (kvStore == nullptr) {
            isKeyValid = false;
        }
    });
    return isKeyValid;
}

Status SecurityManager::ExecuteRekey(const std::string &name, const std::string &path, DBPasswordData &passwordData,
    const std::shared_ptr<DBManager>& dbManager, DBStore *dbStore)
{
    std::string rekeyPath = path + "/rekey";
    std::string backupOldFullName = rekeyPath + "/" + name + REKEY_STORE_POSTFIX + REKEY_OLD;
    std::string backupNewFullName = rekeyPath + "/" + name + REKEY_STORE_POSTFIX + REKEY_NEW;
    std::string rekeyKeyPath = rekeyPath + "/key";
    (void)StoreUtil::InitPath(rekeyPath);
    (void)StoreUtil::InitPath(rekeyKeyPath);
    
    auto dbStatus = dbStore->Export(backupOldFullName, passwordData.password);
    if (dbStatus != DBStatus::OK) {
        ZLOGE("failed to backup the database.");
        return ExitRekey(dbStatus, rekeyPath);
    }

    DBStore *backupStore = nullptr;
    dbStatus = backupStore->Import(backupOldFullName, passwordData.password);
    if (dbStatus != DBStatus::OK) {
        ZLOGE("failed to make the substitute database.");
        return ExitRekey(dbStatus, rekeyPath);
    }

    std::vector<uint8_t> secKey = Random(KEY_SIZE);
    DBPassword password;
    auto pwdStatus = password.SetValue(secKey.data(), secKey.size());
    if (pwdStatus != DBPassword::ErrorCode::OK) {
        dbStatus = DBStatus::DB_ERROR;
        ZLOGE("failed to set the passwd.");
        return ExitRekey(dbStatus, rekeyPath);
    }
    SaveDBPassword(name + REKEY_NEW, rekeyPath, password);
    secKey.assign(secKey.size(), 0);

    dbStatus = backupStore->Rekey(password);
    if (dbStatus != DBStatus::OK) {
        ZLOGE("failed to rekey the substitute database.");
        return ExitRekey(dbStatus, rekeyPath);
    }

    dbStatus = backupStore->Export(backupNewFullName, password);
    if (dbStatus != DBStatus::OK) {
        ZLOGE("failed to export the rekeyed database.");
        return ExitRekey(dbStatus, rekeyPath);
    }

    dbStatus = dbStore->Import(backupNewFullName, password);
    if (dbStatus != DBStatus::OK) {
        ZLOGE("failed to change the database.");
        return ExitRekey(dbStatus, rekeyPath);
    }

    passwordData.password = password;
    SaveDBPassword(name, path, password);
    return ExitRekey(dbStatus, rekeyPath);
}

Status SecurityManager::ExitRekey(DBStatus &dbStatus, const std::string &rekeyPath)
{
    StoreUtil::Remove(rekeyPath);
    return StoreUtil::ConvertStatus(dbStatus);
}
} // namespace OHOS::DistributedKv
