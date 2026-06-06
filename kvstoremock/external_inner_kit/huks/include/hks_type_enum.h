/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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
 * @file hks_type_enum.h
 *
 * @brief Declares huks type enum.
 *
 * @since 8
 */

#ifndef HKS_TYPE_ENUM_H
#define HKS_TYPE_ENUM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hks_error_code.h"
#include "hks_tag.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief hks cipher mode
 */
enum HksCipherMode {
#ifndef HKS_CHIPSET_API
    HKS_MODE_ECB = 1,
    HKS_MODE_CBC = 2,
    HKS_MODE_CTR = 3,
    HKS_MODE_OFB = 4,
    HKS_MODE_CFB = 5,
    HKS_MODE_CCM = 31,
#endif
    HKS_MODE_GCM = 32,
};

/**
 * @brief hks key algorithm
 */
enum HksKeyAlg {
#ifndef HKS_CHIPSET_API
    HKS_ALG_RSA = 1,
#endif
    HKS_ALG_ECC = 2,
#ifndef HKS_CHIPSET_API
    HKS_ALG_DSA = 3,
#endif

    HKS_ALG_AES = 20,
    HKS_ALG_HMAC = 50,
#ifndef HKS_CHIPSET_API
    HKS_ALG_HKDF = 51,
    HKS_ALG_PBKDF2 = 52,
    HKS_ALG_GMKDF = 53,

    HKS_ALG_ECDH = 100,
    HKS_ALG_X25519 = 101,
    HKS_ALG_ED25519 = 102,
    HKS_ALG_DH = 103,

    HKS_ALG_SM2 = 150,
    HKS_ALG_SM3 = 151,
    HKS_ALG_SM4 = 152,

    HKS_ALG_DES = 160,
    HKS_ALG_3DES = 161,
    HKS_ALG_CMAC = 162,

    HKS_ALG_ML_KEM = 200,
    HKS_ALG_ML_DSA = 201,
#endif
};

/**
 * @brief hks key padding
 */
enum HksKeyPadding {
    HKS_PADDING_NONE = 0,
#ifndef HKS_CHIPSET_API
    HKS_PADDING_OAEP = 1,
    HKS_PADDING_PSS = 2,
    HKS_PADDING_PKCS1_V1_5 = 3,
    HKS_PADDING_PKCS5 = 4,
    HKS_PADDING_PKCS7 = 5,
    HKS_PADDING_ISO_IEC_9796_2 = 6,
    HKS_PADDING_ISO_IEC_9797_1 = 7,
#endif
};

/**
 * @brief hks key purpose
 */
enum HksKeyPurpose {
    HKS_KEY_PURPOSE_ENCRYPT = 1,                   /* Usable with RSA, EC, AES, SM2, and SM4 keys. */
    HKS_KEY_PURPOSE_DECRYPT = 2,                   /* Usable with RSA, EC, AES, SM2, and SM4 keys. */
    HKS_KEY_PURPOSE_SIGN = 4,                      /* Usable with RSA, EC keys. */
#ifndef HKS_CHIPSET_API
    HKS_KEY_PURPOSE_VERIFY = 8,                    /* Usable with RSA, EC keys. */
    HKS_KEY_PURPOSE_DERIVE = 16,                   /* Usable with EC keys. */
    HKS_KEY_PURPOSE_WRAP = 32,                     /* Usable with wrap key. */
#endif
    HKS_KEY_PURPOSE_UNWRAP = 64,                   /* Usable with unwrap key. */
    HKS_KEY_PURPOSE_MAC = 128,                     /* Usable with mac. */
#ifndef HKS_CHIPSET_API
    HKS_KEY_PURPOSE_AGREE = 256,                   /* Usable with agree. */
#endif
};

/**
 * @brief hks key digest
 */
enum HksKeyDigest {
    HKS_DIGEST_NONE = 0,
#ifndef HKS_CHIPSET_API
    HKS_DIGEST_MD5 = 1,
    HKS_DIGEST_SM3 = 2,
    HKS_DIGEST_SHA1 = 10,
    HKS_DIGEST_SHA224 = 11,
#endif
    HKS_DIGEST_SHA256 = 12,
#ifndef HKS_CHIPSET_API
    HKS_DIGEST_SHA384 = 13,
    HKS_DIGEST_SHA512 = 14,
#endif
};

/**
 * @brief hks key size
 */
enum HksKeySize {
#ifndef HKS_CHIPSET_API
    HKS_RSA_KEY_SIZE_512 = 512,
    HKS_RSA_KEY_SIZE_768 = 768,
    HKS_RSA_KEY_SIZE_1024 = 1024,
    HKS_RSA_KEY_SIZE_2048 = 2048,
    HKS_RSA_KEY_SIZE_3072 = 3072,
    HKS_RSA_KEY_SIZE_4096 = 4096,

    HKS_ECC_KEY_SIZE_224 = 224,
#endif
    HKS_ECC_KEY_SIZE_256 = 256,
#ifndef HKS_CHIPSET_API
    HKS_ECC_KEY_SIZE_384 = 384,
    HKS_ECC_KEY_SIZE_521 = 521,

    HKS_AES_KEY_SIZE_128 = 128,
    HKS_AES_KEY_SIZE_192 = 192,
#endif
    HKS_AES_KEY_SIZE_256 = 256,
#ifndef HKS_CHIPSET_API
    HKS_AES_KEY_SIZE_512 = 512,

    HKS_CURVE25519_KEY_SIZE_256 = 256,

    HKS_DH_KEY_SIZE_2048 = 2048,
    HKS_DH_KEY_SIZE_3072 = 3072,
    HKS_DH_KEY_SIZE_4096 = 4096,

    HKS_SM2_KEY_SIZE_256 = 256,
    HKS_SM4_KEY_SIZE_128 = 128,

    HKS_DES_KEY_SIZE_64 = 64,
    HKS_3DES_KEY_SIZE_128 = 128,
    HKS_3DES_KEY_SIZE_192 = 192,

    HKS_ML_DSA_KEY_PARAM_SET_44 = 44,
    HKS_ML_DSA_KEY_PARAM_SET_65 = 65,
    HKS_ML_DSA_KEY_PARAM_SET_87 = 87,

    HKS_ML_DSA_PUB_KEY_SIZE_1312 = 1312,
    HKS_ML_DSA_PUB_KEY_SIZE_1952 = 1952,
    HKS_ML_DSA_PUB_KEY_SIZE_2592 = 2592,
    HKS_ML_DSA_PRI_KEY_SIZE_2560 = 2560,
    HKS_ML_DSA_PRI_KEY_SIZE_4032 = 4032,
    HKS_ML_DSA_PRI_KEY_SIZE_4896 = 4896,

    HKS_ML_KEM_PUB_KEY_SIZE_800 = 800,
    HKS_ML_KEM_PUB_KEY_SIZE_1184 = 1184,
    HKS_ML_KEM_PUB_KEY_SIZE_1568 = 1568,
    HKS_ML_KEM_PRI_KEY_SIZE_1632 = 1632,
    HKS_ML_KEM_PRI_KEY_SIZE_2400 = 2400,
    HKS_ML_KEM_PRI_KEY_SIZE_3168 = 3168,

    HKS_ML_KEM_KEY_PARAM_SET_768 = 768,
    HKS_ML_KEM_KEY_PARAM_SET_1024 = 1024,
#endif
};

/**
 * @brief hks key storage type
 */
enum HksKeyStorageType {
    HKS_STORAGE_TEMP = 0,
    HKS_STORAGE_PERSISTENT = 1,
#ifndef HKS_CHIPSET_API
    HKS_STORAGE_ONLY_USED_IN_HUKS = 2,
    HKS_STORAGE_ALLOW_KEY_EXPORTED = 3,
#endif
};

/**
 * @brief hks key generate type
 */
enum HksKeyGenerateType {
    HKS_KEY_GENERATE_TYPE_DEFAULT = 0,
#ifndef HKS_CHIPSET_API
    HKS_KEY_GENERATE_TYPE_DERIVE = 1,
    HKS_KEY_GENERATE_TYPE_AGREE = 2,
#endif
};

/**
 * @brief hks algorithm suite
 */
enum HuksAlgSuite {
#ifndef HKS_CHIPSET_API
    /* Algorithm suites of unwrapping wrapped-key by huks */
    /* Unwrap suite of key agreement type */
    /* WrappedData format(Bytes Array):
     *  | x25519_plain_pubkey_length  (4 Byte) | x25519_plain_pubkey |  agreekey_aad_length (4 Byte) | agreekey_aad
     *  |   agreekey_nonce_length     (4 Byte) |   agreekey_nonce    | agreekey_aead_tag_len(4 Byte) | agreekey_aead_tag
     *  |    kek_enc_data_length      (4 Byte) |    kek_enc_data     |    kek_aad_length    (4 Byte) | kek_aad
     *  |      kek_nonce_length       (4 Byte) |      kek_nonce      |   kek_aead_tag_len   (4 Byte) | kek_aead_tag
     *  |   key_material_size_len     (4 Byte) |  key_material_size  |   key_mat_enc_length (4 Byte) | key_mat_enc_data
     */
    HKS_UNWRAP_SUITE_X25519_AES_256_GCM_NOPADDING = 1,
#endif

    /* WrappedData format(Bytes Array):
     *  |  ECC_plain_pubkey_length    (4 Byte) |  ECC_plain_pubkey   |  agreekey_aad_length (4 Byte) | agreekey_aad
     *  |   agreekey_nonce_length     (4 Byte) |   agreekey_nonce    | agreekey_aead_tag_len(4 Byte) | agreekey_aead_tag
     *  |    kek_enc_data_length      (4 Byte) |    kek_enc_data     |    kek_aad_length    (4 Byte) | kek_aad
     *  |      kek_nonce_length       (4 Byte) |      kek_nonce      |   kek_aead_tag_len   (4 Byte) | kek_aead_tag
     *  |   key_material_size_len     (4 Byte) |  key_material_size  |   key_mat_enc_length (4 Byte) | key_mat_enc_data
     */
    HKS_UNWRAP_SUITE_ECDH_AES_256_GCM_NOPADDING = 2,

#ifndef HKS_CHIPSET_API
    /* WrappedData format(Bytes Array):
     *  |  SM2_plain_pubkey_length    (4 Byte) |  SM2_plain_pubkey   | signData_size_length (4 Byte) | signData_size
     *  |     kek_enc_data_length     (4 Byte) |     kek_enc_data    | kek_material_size_len(4 Byte) | kek_material_size
     *  |       factor1_data_len      (4 Byte) |    factor1_data     |  factor2_data_len    (4 Byte) | factor2_data
     *  |       mac_data_length       (4 Byte) |       mac_data      | key_mat_enc_length   (4 Byte) | key_mat_enc_data
     *  |          iv_data_length     (4 Byte) |            iv_data  |key_material_size_len (4 Byte) | key_material_size
     */
    HKS_UNWRAP_SUITE_SM2_SM4_128_CBC_PKCS7_WITH_VERIFY_DIG_SM3 = 3,

    /* WrappedData format(Bytes Array):
     *  |     kek_enc_data_length     (4 Byte) |     kek_enc_data    | kek_material_size_len(4 Byte) | kek_material_size
     *  |       factor1_data_len      (4 Byte) |    factor1_data     |  factor2_data_len    (4 Byte) | factor2_data
     *  |       mac_data_length       (4 Byte) |       mac_data      | key_mat_enc_length   (4 Byte) | key_mat_enc_data
     *  |          iv_data_length     (4 Byte) |            iv_data  |key_material_size_len (4 Byte) | key_material_size
     */
    HKS_UNWRAP_SUITE_SM2_SM4_128_CBC_PKCS7 = 4,

    HKS_UNWRAP_SUITE_SM2_SM4_ECB_NOPADDING = 5,

    HKS_UNWRAP_SUITE_KEYSTORE = 255,
#endif
};

/**
 * @brief hks Tag
 */
enum HksTag {
    /**
     * HUKS tags for alg enum
     */
    HKS_ASSIGN_PARAM_ALG_ENUM

    /**
     * HUKS tags for key file enum
     */
    HKS_ASSIGN_PARAM_FILE_ENUM
};

#ifndef HKS_CHIPSET_API
/**
 * @brief hks key type
 */
enum HksKeyType {
    HKS_KEY_TYPE_RSA_PUBLIC_KEY = 0x01001000,
    HKS_KEY_TYPE_RSA_KEYPAIR = 0x01002000,

    HKS_KEY_TYPE_ECC_P256_PUBLIC_KEY = 0x02021000,
    HKS_KEY_TYPE_ECC_P256_KEYPAIR = 0x02022000,
    HKS_KEY_TYPE_ECC_P384_PUBLIC_KEY = 0x02031000,
    HKS_KEY_TYPE_ECC_P384_KEYPAIR = 0x02032000,
    HKS_KEY_TYPE_ECC_P521_PUBLIC_KEY = 0x02051000,
    HKS_KEY_TYPE_ECC_P521_KEYPAIR = 0x02052000,

    HKS_KEY_TYPE_ED25519_PUBLIC_KEY = 0x02101000,
    HKS_KEY_TYPE_ED25519_KEYPAIR = 0x02102000,
    HKS_KEY_TYPE_X25519_PUBLIC_KEY = 0x02111000,
    HKS_KEY_TYPE_X25519_KEYPAIR = 0x02112000,

    HKS_KEY_TYPE_AES = 0x03000000,
    HKS_KEY_TYPE_CHACHA20 = 0x04010000,
    HKS_KEY_TYPE_CHACHA20_POLY1305 = 0x04020000,

    HKS_KEY_TYPE_HMAC = 0x05000000,
    HKS_KEY_TYPE_HKDF = 0x06000000,
    HKS_KEY_TYPE_PBKDF2 = 0x07000000,
};

/**
 * @brief hks key flag
 */
enum HksKeyFlag {
    HKS_KEY_FLAG_IMPORT_KEY = 1,
    HKS_KEY_FLAG_GENERATE_KEY = 2,
    HKS_KEY_FLAG_AGREE_KEY = 3,
    HKS_KEY_FLAG_DERIVE_KEY = 4,
};

/**
 * @brief hks import key type
 */
enum HksImportKeyType {
    HKS_KEY_TYPE_PUBLIC_KEY = 0,
    HKS_KEY_TYPE_PRIVATE_KEY = 1,
    HKS_KEY_TYPE_KEY_PAIR = 2,
};

/**
 * @brief hks rsa pss salt len type
 */
enum HksRsaPssSaltLenType {
    HKS_RSA_PSS_SALTLEN_DIGEST = 0,  /* Salt length matches digest */
    HKS_RSA_PSS_SALTLEN_MAX = 1,  /* Set salt length to maximum possible, default type */
};

/**
 * @brief hks send type
 */
enum HksSendType {
    HKS_SEND_TYPE_ASYNC = 0,
    HKS_SEND_TYPE_SYNC,
};

/**
 * @brief hks user auth type
 * @see `enum AuthType` in `drivers/interface/user_auth/v4_1/UserAuthTypes.idl`
 */
enum HksUserAuthType {
    HKS_USER_AUTH_TYPE_FINGERPRINT = 1 << 0,
    HKS_USER_AUTH_TYPE_FACE = 1 << 1,
    HKS_USER_AUTH_TYPE_PIN = 1 << 2,
    HKS_USER_AUTH_TYPE_TUI_PIN = 1 << 5,
};

/**
 * @brief hks user auth type
 * @see `enum AuthType` in `base/useriam/user_auth_framework/interfaces/inner_api/iam_common_defines.h`
 */
enum HksIamUserAuthType {
    HKS_IAM_USER_AUTH_TYPE_ALL = 0,
    HKS_IAM_USER_AUTH_TYPE_PIN = 1,
    HKS_IAM_USER_AUTH_TYPE_FACE = 2,
    HKS_IAM_USER_AUTH_TYPE_FINGERPRINT = 4,
    HKS_IAM_USER_AUTH_TYPE_RECOVERY_KEY = 8,
    HKS_IAM_USER_AUTH_TYPE_PRIVATE_PIN = 16,
    HKS_IAM_USER_AUTH_TYPE_TUI_PIN = 32,
};

/**
 * @brief hks user auth atl type
 * @see `enum AuthTrustLevel` in `drivers/interface/user_auth/v4_1/UserAuthTypes.idl`
 */
enum HksIamUserAuthAtl {
    HKS_IAM_USER_AUTH_ATL1 = 10000,
    HKS_IAM_USER_AUTH_ATL2 = 20000,
    HKS_IAM_USER_AUTH_ATL3 = 30000,
    HKS_IAM_USER_AUTH_ATL4 = 40000,
};

enum HksUserAuthAtlType {
    HKS_USER_AUTH_ATL1 = 1,
    HKS_USER_AUTH_ATL2 = 2,
    HKS_USER_AUTH_ATL3 = 3,
    HKS_USER_AUTH_ATL4 = 4,
};

/**
 * @brief hks auth access type
 */
enum HksAuthAccessType {
    HKS_AUTH_ACCESS_INVALID_CLEAR_PASSWORD = 1 << 0,
    HKS_AUTH_ACCESS_INVALID_NEW_BIO_ENROLL = 1 << 1,
    HKS_AUTH_ACCESS_ALWAYS_VALID = 1 << 2,
};

/**
 * @brief hks challenge type
 */
enum HksChallengeType {
    HKS_CHALLENGE_TYPE_NORMAL = 0,
    HKS_CHALLENGE_TYPE_CUSTOM = 1,
    HKS_CHALLENGE_TYPE_NONE = 2,
};

/**
 * @brief hks auth mode
 */
enum HksUserAuthMode {
    HKS_USER_AUTH_MODE_LOCAL = 0,
    HKS_USER_AUTH_MODE_COAUTH = 1,
};

/**
 * @brief hks challenge position
 */
enum HksChallengePosition {
    HKS_CHALLENGE_POS_0 = 0,
    HKS_CHALLENGE_POS_1,
    HKS_CHALLENGE_POS_2,
    HKS_CHALLENGE_POS_3,
};

/**
 * @brief hks secure sign type
 */
enum HksSecureSignType {
    HKS_SECURE_SIGN_WITH_AUTHINFO = 1,
};

/**
 * @brief hks attestation type
 */
enum HksAttestationMode {
    HKS_ATTESTATION_MODE_DEFAULT = 0,
    HKS_ATTESTATION_MODE_ANONYMOUS
};

/**
 * @brief hks anonattestation type
 */
enum HksAnonAttestationMode {
    HKS_ANONYMOUS_ATTEST_ONLNE = 0,
    HKS_ANONYMOUS_ATTEST_OFFLINE = 1
};

/**
 * @brief hks attestation cert type
 */
enum HksAttestationCertType {
    HKS_ATTESTATION_CERT_TYPE_PROVISION = 0,
    HKS_ATTESTATION_CERT_TYPE_HARDWARE_BOUND = 1,
    HKS_ATTESTATION_CERT_TYPE_RSA = 2,
};

/**
 * @brief hks attestation Caller Type
 */
enum HksCallerType {
    HKS_HAP_TYPE = 0x1,
    HKS_SA_TYPE,
    HKS_UNIFIED_TYPE,
};

enum HksUserIamType {
    HKS_AUTH_TYPE = 0,
};

/**
 * @brief hks chipset platform decrypt scene
 */
enum HksChipsetPlatformDecryptScene {
    HKS_CHIPSET_PLATFORM_DECRYPT_SCENE_TA_TO_TA = 1,
};

/**
 * @brief hks auth storage level
 */
enum HksAuthStorageLevel {
    HKS_AUTH_STORAGE_LEVEL_DE = 0,
    HKS_AUTH_STORAGE_LEVEL_CE = 1,
    HKS_AUTH_STORAGE_LEVEL_ECE = 2,
};

enum HksAgreePubKeyType {
    HKS_PUBKEY_DEFAULT = 0
};

enum HksKeyWrapType {
    HKS_KEY_WRAP_TYPE_HUK = 2,
};

enum HksKeySecurityLevel {
    HKS_KEY_SECURITY_LEVEL_TEE = 0,
    HKS_KEY_SECURITY_LEVEL_SE = 1,
    HKS_KEY_SECURITY_LEVEL_INDEPENDENT_SE = 2,
};

enum HksWrapKeyByHukVersion {
    HKS_WRAP_KEY_BY_HUK_VERSION_DEFAULT = 0,

    /* 100 ~ 199 se wrappedKey */
    HKS_WRAP_KEY_BY_HUK_VERSION_SE = 100,
    /* 200 ~ 299  se wrappedKey */
    HKS_WRAP_KEY_BY_HUK_VERSION_INDEPENDENT_SE = 200,
};
#endif

/**
 * @brief hks ext property operation type
 */
enum HksExtPropertyOperation {
    HKS_EXT_PROPERTY_OPERATION_GET = 0,
    HKS_EXT_PROPERTY_OPERATION_SET = 1,
};

#ifdef __cplusplus
}
#endif

#endif /* HKS_TYPE_ENUM_H */
