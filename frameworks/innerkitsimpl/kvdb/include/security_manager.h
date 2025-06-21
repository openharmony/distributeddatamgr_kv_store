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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SECURITY_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SECURITY_MANAGER_H
#include <atomic>

#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "task_executor.h"
#include "types.h"
#include "types_export.h"
namespace OHOS::DistributedKv {
class SecurityManager {
public:
    struct SecurityContent {
        static constexpr size_t MAGIC_NUM = 4;
        static constexpr uint8_t MAGIC_CHAR = 0x6A;
        static constexpr uint32_t MAGIC_NUMBER = 0x6A6A6A6A;
        static constexpr uint8_t INVALID_VERSION = 0x00;
        static constexpr uint8_t CURRENT_VERSION = 0x01;
        static constexpr int32_t NONCE_SIZE = 12;
        static constexpr int32_t KEY_SIZE = 32;

        bool isNewStyle = true;
        uint32_t magicNum = MAGIC_NUMBER;
        uint8_t version = INVALID_VERSION;
        std::vector<uint8_t> time;
        std::vector<uint8_t> nonceValue;
        // encryptValue contains version and time and key
        std::vector<uint8_t> encryptValue;
        std::vector<uint8_t> fullKeyValue;
    };

    struct DBPassword {
        bool isKeyOutdated = false;
        DistributedDB::CipherPassword password;
        size_t GetSize() const
        {
            return password.GetSize();
        }
        const uint8_t *GetData() const
        {
            return password.GetData();
        }
        int SetValue(const uint8_t *inputData, size_t inputSize)
        {
            return password.SetValue(inputData, inputSize);
        }
        bool IsValid()
        {
            return password.GetSize() != 0;
        }
        int Clear()
        {
            return password.Clear();
        }
    };

    class KeyFiles {
    public:
        KeyFiles(const std::string &name, const std::string &path, bool openFile = true);
        ~KeyFiles();
        int32_t Lock();
        int32_t UnLock();
        int32_t DestroyLock();
    private:
        int32_t FileLock(int32_t lockType);
        int32_t lockFd_ = -1;
        std::string lockFile_;
    };

    class KeyFilesAutoLock {
    public:
        explicit KeyFilesAutoLock(KeyFiles& keyFiles);
        ~KeyFilesAutoLock();
        KeyFilesAutoLock(const KeyFilesAutoLock&) = delete;
        KeyFilesAutoLock& operator=(const KeyFilesAutoLock&) = delete;
        int32_t UnLockAndDestroy();
    private:
        KeyFiles& keyFiles_;
    };

    static SecurityManager &GetInstance();
    DBPassword GetDBPassword(const std::string &name, const std::string &path, bool needCreate = false);
    bool SaveDBPassword(const std::string &name, const std::string &path, const DistributedDB::CipherPassword &key);
    void DelDBPassword(const std::string &name, const std::string &path);

private:
    SecurityManager();
    ~SecurityManager();
    std::vector<uint8_t> Random(int32_t length);
    bool LoadContent(SecurityContent &content, const std::string &path);
    void LoadKeyFromFile(const std::string &path, SecurityContent &securityContent);
    void LoadNewKey(const std::vector<char> &content, SecurityContent &securityContent);
    void LoadOldKey(const std::vector<char> &content, SecurityContent &securityContent);
    bool SaveKeyToFile(const std::string &name, const std::string &path, std::vector<uint8_t> &key);
    bool IsKeyOutdated(const std::vector<uint8_t> &date);
    int32_t GenerateRootKey();
    int32_t CheckRootKey();
    bool Retry();
    bool Encrypt(const std::vector<uint8_t> &key, SecurityContent &content);
    bool Decrypt(SecurityContent &content);

    std::vector<uint8_t> vecRootKeyAlias_{};
    std::vector<uint8_t> vecNonce_{};
    std::vector<uint8_t> vecAad_{};
    std::atomic_bool hasRootKey_ = false;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SECURITY_MANAGER_H