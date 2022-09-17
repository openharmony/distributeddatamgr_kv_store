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

#include <limits>
#include <random>
#include "store_util.h"
#include "security_manager.h"
namespace OHOS::DistributedKv {
SecurityManager &SecurityManager::GetInstance()
{
    static SecurityManager instance;
    return instance;
}

SecurityManager::DBPassword SecurityManager::GetDBPassword(const std::string &name,
    const std::string &path, bool needCreate)
{
    auto secKey = LoadKeyFromFile(name, path);
    if (secKey.empty()) {
        if (!needCreate) {
            return DBPassword();
        } else {
            secKey = Random(KEY_SIZE);
            SaveKeyToFile(name, path, secKey);
        }
    }
    DBPassword password;
    password.SetValue(secKey.data(), secKey.size());
    secKey.assign(secKey.size(), 0);
    return password;
}

bool SecurityManager::SaveDBPassword
    (const std::string &name, const std::string &path, const SecurityManager::DBPassword &key)
{
    std::vector<uint8_t> pwd(key.GetData(), key.GetData() + key.GetSize());
    SaveKeyToFile(name, path, pwd);
    pwd.assign(pwd.size(), 0);
    return true;
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

std::vector<uint8_t> SecurityManager::LoadKeyFromFile(const std::string &name, const std::string &path)
{
    return {};
}

bool SecurityManager::SaveKeyToFile(const std::string &name, const std::string &path, const std::vector<uint8_t> &key)
{
    return 0;
}
} // namespace OHOS::DistributedKv