/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file hks_type.h
 *
 * @brief Declares huks type.
 *
 * @since 8
 */

#ifndef HKS_TYPE_H
#define HKS_TYPE_H

#include "hks_type_enum.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HKS_API_PUBLIC
    #if defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__) || defined(__ICCARM__) /* __ICCARM__ for iar */
        #define HKS_API_EXPORT
    #else
        #define HKS_API_EXPORT __attribute__ ((visibility("default")))
    #endif
#else
    #define HKS_API_EXPORT __attribute__ ((visibility("default")))
#endif

/**
 * @brief hks blob
 */
struct HksBlob {
    uint32_t size;
    uint8_t *data;
};

/**
 * @brief hks param
 */
struct HksParam {
    uint32_t tag;
    union {
        bool boolParam;
        int32_t int32Param;
        uint32_t uint32Param;
        uint64_t uint64Param;
        struct HksBlob blob;
    };
};

/**
 * @brief hks param set
 */
struct HksParamSet {
    uint32_t paramSetSize;
    uint32_t paramsCnt;
    struct HksParam params[];
};

struct HksAbilityInfo {
    struct HksBlob bundleName;
    struct HksBlob abilityName;
};

struct HksExtCertInfoSet {
    uint32_t count;
    struct HksExtCertInfo *certs;
};

/**
 * @brief hks ext property operation info
 */
struct HksExtPropertyOperationInfo {
    enum HksExtPropertyOperation operation;
    const struct HksBlob *resourceId;
    const struct HksBlob *propertyId;
};

/**
 * @brief hks certificate chain
 */
struct HksCertChain {
    struct HksBlob *certs;
    uint32_t certsCount;
};

/**
 * @brief hks key info
 */
struct HksKeyInfo {
    struct HksBlob alias;
    struct HksParamSet *paramSet;
};


/**
 * @brief hks alias set
 */
struct HksKeyAliasSet {
    uint32_t aliasesCnt;
    struct HksBlob *aliases;
};

/**
 * @brief hks encapsulation result for ML-KEM
 */
struct HksEncapsulationResult {
    struct HksBlob encapsulatedData;
    struct HksBlob sharedSecret;
};

#ifndef HKS_CHIPSET_API
/*
 * a.b.c.d => a * 1000 + b * 100 + c * 10 + d
 * a,b,c,d should in [0, 9]
 */
#define HKS_SDK_VERSION "2.0.0.5"

/*
 * Align to 4-tuple
 * Before calling this function, ensure that the size does not overflow after 3 is added.
 */
#define ALIGN_SIZE(size) ((((uint32_t)(size) + 3) >> 2) << 2)
#define DEFAULT_ALIGN_MASK_SIZE 3

#define HKS_AE_TAG_LEN 16
#define HKS_BITS_PER_BYTE 8
#define MAX_KEY_SIZE 2048
#define ML_DSA_MAX_KEY_SIZE 10000
#define ML_KEM_MAX_KEY_SIZE 6000
#define HKS_AE_NONCE_LEN 12
#define HKS_MAX_KEY_ALIAS_LEN 128
#define HKS_MAX_PROCESS_NAME_LEN 50
#define HKS_MAX_RANDOM_LEN 1024
#define HKS_KEY_BYTES(keySize) (((keySize) + HKS_BITS_PER_BYTE - 1) / HKS_BITS_PER_BYTE)
#define HKS_SIGNATURE_MIN_SIZE 64
#define HKS_ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof((arr)[0])))
#define MAX_OUT_BLOB_SIZE (5 * 1024 * 1024)
#define HKS_MAX_QUERY_RESULT 2176
#define HKS_WRAPPED_FORMAT_MAX_SIZE (1024 * 1024)
#define HKS_IMPORT_WRAPPED_KEY_TOTAL_BLOBS 10
#define HKS_IMPORT_ENVELOP_TOTAL_BLOBS 2

#define TOKEN_CHALLENGE_LEN 32
#define UDID_LEN 64
#define SHA256_SIGN_LEN 32
#define TOKEN_SIZE 32
#define MAX_AUTH_TIMEOUT_SECOND_INNER 216000  //6h
#define ASSET_MAX_AUTH_TIMEOUT_SECOND 3600
#define SECURE_SIGN_VERSION 0x01000001

#define HKS_CERT_COUNT 4
#define HKS_CERT_ROOT_SIZE 2048
#define HKS_CERT_CA_SIZE 2048
#define HKS_CERT_DEVICE_SIZE 2048
#define HKS_CERT_APP_SIZE 4096

#define HKS_MAX_FILE_SIZE 10240

#define HKS_KEY_BLOB_AT_KEY_SIZE 256
#define HKS_KEY_BLOB_AT_KEY_BYTES 32

#define HKS_MAX_CERT_COUNT 2048
#define HKS_MAX_KEY_ALIAS_COUNT 2048
#define MAX_ERROR_MESSAGE_LEN 512

#define HKS_EXT_MAX_PROVIDER_NAME_LEN 128
#define HKS_EXT_MAX_RESOURCE_ID_LEN 1024
#define HKS_EXT_MAX_PROPERTY_ID_LEN 100

struct HksExtCertInfo {
    int32_t purpose;
    struct HksBlob index;
    struct HksBlob cert;
};

struct HksExtParam {
    uint32_t tag;
    union {
        bool boolParam;
        int32_t int32Param;
        uint32_t uint32Param;
        uint64_t uint64Param;
        struct HksBlob blob;
    };
};

struct HksExtParamSet {
    uint32_t paramSetSize;
    uint32_t paramsCnt;
    struct HksExtParam params[];
};

/**
 * @brief hks public key info
 */
struct HksPubKeyInfo {
    enum HksKeyAlg keyAlg;
    uint32_t keySize;
    uint32_t nOrXSize;
    uint32_t eOrYSize;
    uint32_t placeHolder;
};

/**
 * @brief hks rsa key material
 */
struct HksKeyMaterialRsa {
    enum HksKeyAlg keyAlg;
    uint32_t keySize;
    uint32_t nSize;
    uint32_t eSize;
    uint32_t dSize;
};

/**
 * @brief hks ecc key material
 */
struct HksKeyMaterialEcc {
    enum HksKeyAlg keyAlg;
    uint32_t keySize;
    uint32_t xSize;
    uint32_t ySize;
    uint32_t zSize;
};

/**
 * @brief hks ml-dsa key material
 */
struct HksKeyMaterialMlDsa {
    enum HksKeyAlg keyAlg;
    uint32_t keyParamSet;
    uint32_t pubKeySize;
    uint32_t priKeySize;
    uint32_t reserved;
};

/**
 * @brief hks dsa key material
 */
struct HksKeyMaterialDsa {
    enum HksKeyAlg keyAlg;
    uint32_t keySize;
    uint32_t xSize;
    uint32_t ySize;
    uint32_t pSize;
    uint32_t qSize;
    uint32_t gSize;
};

/**
 * @brief hks dh key material
 */
struct HksKeyMaterialDh {
    enum HksKeyAlg keyAlg;
    uint32_t keySize;
    uint32_t pubKeySize;
    uint32_t priKeySize;
    uint32_t reserved;
};

/**
 * @brief hks 25519 key material
 */
struct HksKeyMaterial25519 {
    enum HksKeyAlg keyAlg;
    uint32_t keySize;
    uint32_t pubKeySize;
    uint32_t priKeySize;
    uint32_t reserved;
};

/**
 * @brief hks ml-kem key material
 */
struct HksKeyMaterialMlKem {
    enum HksKeyAlg keyAlg;
    uint32_t keyParamSet;
    uint32_t pubKeySize;
    uint32_t priKeySize;
    uint32_t reserved;
};

/**
 * @brief hks user auth token plaintext data
 * @see `TokenDataPlain` in `drivers/peripheral/user_auth/hdi_service/user_auth/inc/user_sign_centre.h`
 */
typedef struct HksPlaintextData {
    uint8_t challenge[TOKEN_SIZE];
    uint64_t time;
    uint32_t authTrustLevel;
    uint32_t authType;
    uint32_t authMode;
    uint32_t securityLevel;
    /**
     * @see `enum TokenType` in `drivers/peripheral/user_auth/hdi_service/common/inc/defines.h`
     */
    uint32_t tokenType;
} __attribute__((__packed__)) HksPlaintextData;

/**
 * @brief hks user auth token ciphertext data
 * @see `TokenDataToEncrypt` in `drivers/peripheral/user_auth/hdi_service/user_auth/inc/user_sign_centre.h`
 */
typedef struct HksCiphertextData {
    int32_t userId;
    uint64_t secureUid;
    uint64_t enrolledId;
    uint64_t credentialId;
    uint8_t collectorUdid[UDID_LEN];
    uint8_t verifierUdid[UDID_LEN];
} __attribute__((__packed__)) HksCiphertextData;

/**
 * @brief hks user auth token
 * @see `UserAuthTokenHal` in `drivers/peripheral/user_auth/hdi_service/user_auth/inc/user_sign_centre.h`
 */
typedef struct __attribute__((__packed__)) HksUserAuthToken {
    uint32_t version;
    HksPlaintextData plaintextData;
    HksCiphertextData ciphertextData;
    uint8_t tag[HKS_AE_TAG_LEN];
    uint8_t iv[HKS_AE_NONCE_LEN];
    uint8_t sign[SHA256_SIGN_LEN];
} __attribute__((__packed__)) HksUserAuthToken;

/**
 * @brief hks user auth token key
 */
struct HksAuthTokenKey {
    uint8_t macKey[HKS_KEY_BLOB_AT_KEY_BYTES];
    uint8_t cipherKey[HKS_KEY_BLOB_AT_KEY_BYTES];
};

/**
 * @brief hks secure sign auth info
 */
typedef struct __attribute__((__packed__)) HksSecureSignAuthInfo {
    uint32_t userAuthType;
    uint32_t userAuthAtlType;
    uint64_t authenticatorId;
    uint64_t credentialId;
} __attribute__((__packed__)) HksSecureSignAuthInfo;

struct EnrolledInfoWrap {
    enum HksUserAuthType authType;
    uint64_t enrolledId;
};

struct SecInfoWrap {
    uint64_t secureUid;
    uint32_t enrolledInfoLen;
    struct EnrolledInfoWrap *enrolledInfo;
};

#define HKS_DERIVE_DEFAULT_SALT_LEN 16
#define HKS_HMAC_DIGEST_SHA512_LEN 64
#define HKS_DEFAULT_RANDOM_LEN 16
#define HKS_MAX_KEY_AUTH_ID_LEN 64
#define HKS_KEY_MATERIAL_NUM 3
#define HKS_MAX_KEY_LEN (HKS_KEY_BYTES(HKS_RSA_KEY_SIZE_4096) * HKS_KEY_MATERIAL_NUM)
#define HKS_MAX_KEY_MATERIAL_LEN (sizeof(struct HksPubKeyInfo) + HKS_MAX_KEY_LEN + HKS_AE_TAG_LEN)
#define COMMON_EVENT_HKS_BINDER_DIED "ohos.hks.action.BINDER_DIED"

#define HKS_ML_KEM_SHARED_SECRET_LEN 32
#define HKS_ML_KEM_768_CIPHERTEXT_LEN 1088
#define HKS_ML_KEM_1024_CIPHERTEXT_LEN 1568
#define HKS_ML_KEM_MAX_CIPHERTEXT_LEN HKS_ML_KEM_1024_CIPHERTEXT_LEN

/**
 * @brief hks store header info
 */
struct HksStoreHeaderInfo {
    uint16_t version;
    uint16_t keyCount;
    uint32_t totalLen; /* key buffer total len */
    uint32_t sealingAlg;
    uint8_t salt[HKS_DERIVE_DEFAULT_SALT_LEN];
    uint8_t hmac[HKS_HMAC_DIGEST_SHA512_LEN];
};

/**
 * @brief hks store key info
 */
struct HksStoreKeyInfo {
    uint16_t keyInfoLen; /* current keyinfo len */
    uint16_t keySize;    /* keySize of key from crypto hal after encrypted */
    uint8_t random[HKS_DEFAULT_RANDOM_LEN];
    uint8_t flag;        /* import or generate key */
    uint8_t keyAlg;
    uint8_t keyMode;
    uint8_t digest;
    uint8_t padding;
    uint8_t rsv;
    uint16_t keyLen;     /* keyLen from paramset, e.g. aes-256 */
    uint32_t purpose;
    uint32_t role;
    uint16_t domain;
    uint8_t aliasSize;
    uint8_t authIdSize;
};

struct ErrorInfoHead {
    int32_t version;
    int32_t errorType;
    int32_t innerErrCode;
    int32_t extErrCode;
};

struct ErrorInfo {
    struct ErrorInfoHead head;
    char erMsg[MAX_ERROR_MESSAGE_LEN + 1];
};

/**
 * @brief hks 25519 key pair
 */
struct Hks25519KeyPair {
    uint32_t publicBufferSize;
    uint32_t privateBufferSize;
};

/**
 * @brief hks import keystore args
 */
struct HksImportKeyStoreArgs {
    const struct HksBlob keyAlias;
    uint32_t uidInt;
};

static inline bool IsAdditionOverflow(uint32_t a, uint32_t b)
{
    return (UINT32_MAX - a) < b;
}

static inline bool IsInvalidLength(uint32_t length)
{
    return (length == 0) || (length > MAX_OUT_BLOB_SIZE);
}

static inline int32_t CheckBlob(const struct HksBlob *blob)
{
    if ((blob == NULL) || (blob->data == NULL) || (blob->size == 0)) {
        return HKS_ERROR_INVALID_ARGUMENT;
    }
    return HKS_SUCCESS;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* HKS_TYPE_H */
