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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_BLOCK_DATA_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_BLOCK_DATA_H
#include <condition_variable>
#include <mutex>

namespace OHOS {
template<typename T>
class BlockData {
public:
    explicit BlockData(uint32_t interval, const T &invalid = T()) : INTERVAL(interval), data_(invalid) {}

    ~BlockData() {}

public:
    void SetValue(const T &data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = data;
        isSet_ = true;
        cv_.notify_one();
    }

    T GetValue()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(INTERVAL), [this]() {
            return isSet_;
        });
        T data = data_;
        cv_.notify_one();
        return data;
    }

    void Clear(const T &invalid = T())
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isSet_ = false;
        data_ = invalid;
        cv_.notify_one();
    }

private:
    bool isSet_ = false;
    const uint32_t INTERVAL;
    T data_;
    std::mutex mutex_;
    std::condition_variable cv_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_BLOCK_DATA_H