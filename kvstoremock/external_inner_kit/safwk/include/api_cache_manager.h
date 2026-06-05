/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef API_CACHE_MANAGER_H
#define API_CACHE_MANAGER_H

#include <string>
#include <vector>
#include "expire_lru_cache.h"
#include "message_parcel.h"

namespace OHOS {
class ApiCacheManager {
public:
    static ApiCacheManager& GetInstance();

    void AddCacheApi(const std::u16string& descriptor, uint32_t apiCode, int64_t expireTimeSec);

    void DelCacheApi(const std::u16string& descriptor, uint32_t apiCode);

    void ClearCache();

    void ClearCache(const std::u16string& descriptor);

    void ClearCache(const std::u16string& descriptor, int32_t apiCode);

    bool PreSendRequest(const std::u16string& descriptor, uint32_t apiCode, const MessageParcel &data,
        MessageParcel& reply);
    bool PostSendRequest(const std::u16string& descriptor, uint32_t apiCode, const MessageParcel &data,
        MessageParcel& reply);
private:
    ApiCacheManager() = default;
    ~ApiCacheManager()
    {
        std::lock_guard<std::mutex> Lock(cachesMutex_);
        for (auto i:caches_) {
            delete i.second;
        }
    };
    ApiCacheManager(const ApiCacheManager&) = delete;
    ApiCacheManager& operator= (const ApiCacheManager&) = delete;
    ApiCacheManager(ApiCacheManager&&) = delete;
    ApiCacheManager& operator= (ApiCacheManager&&) = delete;

    std::mutex cachesMutex_;
    std::map<std::pair<std::u16string, uint32_t>, ExpireLruCache<std::vector<uint8_t>, std::vector<uint8_t>>*> caches_;
};
}

#endif