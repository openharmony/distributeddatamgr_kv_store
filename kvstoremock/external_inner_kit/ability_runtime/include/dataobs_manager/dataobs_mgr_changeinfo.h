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

#ifndef OHOS_ABILITY_RUNTIME_DATAOBS_MGR_CHANGENOTIFICATION_H
#define OHOS_ABILITY_RUNTIME_DATAOBS_MGR_CHANGENOTIFICATION_H

#include <list>
#include <map>
#include "uri.h"
#include "message_parcel.h"

namespace OHOS {
namespace AAFwk {
struct ChangeInfo {
    enum ChangeType : uint32_t {
        INSERT = 0,
        DELETE,
        UPDATE,
        OTHER,
        INVAILD,
    };
    using Value = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>>;
    using VBucket = std::map<std::string, Value>;
    using VBuckets = std::vector<VBucket>;

    static bool Marshalling(const ChangeInfo &input, MessageParcel &parcel);
    static bool Unmarshalling(ChangeInfo &output, MessageParcel &parcel);

    ChangeType changeType_ = INVAILD;
    mutable std::list<Uri> uris_ = {};
    void *data_ = nullptr;
    uint32_t size_ = 0;
    VBuckets valueBuckets_ = {};
    static constexpr int LIST_MAX_COUNT = 3000;
};
} // namespace AAFwk
} // namespace OHOS
#endif //OHOS_ABILITY_RUNTIME_DATAOBS_MGR_CHANGENOTIFICATION_H
