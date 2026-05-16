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
#ifndef KV_STORE_CRYPT_H
#define KV_STORE_CRYPT_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS::DistributedKv {
struct KVDBCryptoParam {
    std::vector<uint8_t> keyValue;
    std::vector<uint8_t> nonceValue;
    KVDBCryptoParam() = default;
    ~KVDBCryptoParam()
    {
        keyValue.assign(keyValue.size(), 0);
        nonceValue.assign(nonceValue.size(), 0);
    }
};

class KVDBCrypto {
public:
    virtual ~KVDBCrypto();
    virtual bool CheckRootKey() = 0;
    virtual bool GenerateRootKey() = 0;
    virtual std::vector<uint8_t> Encrypt(const KVDBCryptoParam &param) = 0;
    virtual std::vector<uint8_t> Decrypt(const KVDBCryptoParam &param) = 0;
};
} // namespace OHOS::DistributedKv
#endif // KV_STORE_CRYPT_H