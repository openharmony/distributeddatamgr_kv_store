/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "KVDBCrypto"

#include "kv_store_crypt.h"
#include "hks_api.h"
#include "hks_param.h"
#include "log_print.h"
#include "visibility.h"

using namespace OHOS::DistributedKv;

API_EXPORT std::shared_ptr<OHOS::DistributedKv::KVDBCrypto> Create(
    const std::vector<uint8_t> &rootKeyAlias, const std::vector<uint8_t> vecAad) asm("CreateKvdbCryptoDelegate");

API_EXPORT std::vector<uint8_t> GenerateRandomNum(const uint32_t length) asm(
    "GenerateKvdbRandomNum");

std::vector<uint8_t> GenerateRandomNum(const uint32_t length)
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

namespace OHOS::DistributedKv {
KVDBCrypto::~KVDBCrypto()
{}

class KVDBCryptoImpl : public KVDBCrypto {
public:
    explicit KVDBCryptoImpl(const std::vector<uint8_t> &rootKeyAlias, const std::vector<uint8_t> vecAad);
    ~KVDBCryptoImpl() override = default;
    bool CheckRootKey() override;
    bool GenerateRootKey() override;
    std::vector<uint8_t> Encrypt(const KVDBCryptoParam &param) override;
    std::vector<uint8_t> Decrypt(const KVDBCryptoParam &param) override;

private:
    std::vector<uint8_t> vecRootKeyAlias_;
    std::vector<uint8_t> vecAad_;
};

KVDBCryptoImpl::KVDBCryptoImpl(const std::vector<uint8_t> &rootKeyAlias, const std::vector<uint8_t> vecAad)
    : vecRootKeyAlias_(rootKeyAlias), vecAad_(vecAad)
{}

bool KVDBCryptoImpl::GenerateRootKey()
{
    std::vector<uint8_t> tempRootKeyAlias = vecRootKeyAlias_;
    struct HksBlob rootKeyName = { uint32_t(tempRootKeyAlias.size()), tempRootKeyAlias.data() };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return false;
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
        return false;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }

    ret = HksGenerateKey(&rootKeyName, params, nullptr);
    HksFreeParamSet(&params);
    ZLOGI("HksGenerateKey status: %{public}d", ret);
    if (ret != HKS_SUCCESS) {
        return false;
    }
    return true;
}

bool KVDBCryptoImpl::CheckRootKey()
{
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return false;
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
        return false;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }

    ret = HksKeyExist(&rootKeyName, params);
    HksFreeParamSet(&params);
    ZLOGI("HksKeyExist status: %{public}d", ret);
    if (ret != HKS_SUCCESS) {
        return false;
    }
    return true;
}

std::vector<uint8_t> KVDBCryptoImpl::Encrypt(const KVDBCryptoParam &param)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return {};
    }
    struct HksBlob blobNonce = { .size = uint32_t(param.nonceValue.size()),
        .data = const_cast<uint8_t*>(&(param.nonceValue[0])) };
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
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
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksBlob plainKey = { uint32_t(param.keyValue.size()), const_cast<uint8_t *>(param.keyValue.data()) };
    ret = HksEncrypt(&rootKeyName, params, &plainKey, &cipherText);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksEncrypt failed, status: %{public}d", ret);
        return {};
    }
    std::vector<uint8_t> encryptKey = std::vector<uint8_t>(cipherText.data, cipherText.data + cipherText.size);
    std::fill(cipherBuf, cipherBuf + sizeof(cipherBuf), 0);
    return encryptKey;
}

std::vector<uint8_t> KVDBCryptoImpl::Decrypt(const KVDBCryptoParam &param)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed, status: %{public}d", ret);
        return {};
    }
    struct HksBlob blobNonce = { .size = uint32_t(param.nonceValue.size()),
        .data = const_cast<uint8_t*>(&(param.nonceValue[0])) };
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
        return {};
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed, status: %{public}d", ret);
        HksFreeParamSet(&params);
        return {};
    }
    uint8_t plainBuf[256] = { 0 };
    struct HksBlob plainKeyBlob = { sizeof(plainBuf), plainBuf };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), &(vecRootKeyAlias_[0]) };
    struct HksBlob encryptedKeyBlob = { uint32_t(param.keyValue.size()),
        const_cast<uint8_t *>(param.keyValue.data()) };
    ret = HksDecrypt(&rootKeyName, params, &encryptedKeyBlob, &plainKeyBlob);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksDecrypt, status: %{public}d", ret);
        return {};
    }
    std::vector<uint8_t> decryptKey = std::vector<uint8_t>(plainKeyBlob.data, plainKeyBlob.data + plainKeyBlob.size);
    std::fill(plainBuf, plainBuf + sizeof(plainBuf), 0);
    return decryptKey;
}
} // namespace OHOS::DistributedKv

std::shared_ptr<OHOS::DistributedKv::KVDBCrypto> Create(const std::vector<uint8_t> &rootKeyAlias,
    const std::vector<uint8_t> vecAad)
{
    return std::make_shared<OHOS::DistributedKv::KVDBCryptoImpl>(rootKeyAlias, vecAad);
}
