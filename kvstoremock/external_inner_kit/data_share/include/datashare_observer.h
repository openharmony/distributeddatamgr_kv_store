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

#ifndef DATASHARE_OBSERVER_H
#define DATASHARE_OBSERVER_H

#include <list>
#include <map>
#include <string>

#include "uri.h"

namespace OHOS::DataShare {
class DataShareObserver {
public:
    DataShareObserver() = default;
    virtual ~DataShareObserver() = default;
    enum SubscriptionType : uint32_t {
        SUBSCRIPTION_TYPE_EXACT_URI = 0,
    };
    enum ChangeType : uint32_t {
        INSERT = 0,
        DELETE,
        UPDATE,
        OTHER,
        INVAILD,
    };

    struct ChangeInfo {
        using Value = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>>;
        using VBucket = std::map<std::string, Value>;
        using VBuckets = std::vector<VBucket>;
        ChangeType changeType_ = INVAILD;
        std::list<Uri> uris_ = {};
        const void* data_ = nullptr;
        uint32_t size_ = 0;
        VBuckets valueBuckets_ = {};
    };

    virtual void OnChange(const ChangeInfo& changeInfo) = 0;
};
}
#endif // DATASHARE_OBSERVER_H
