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

#ifndef DISTRIBUTED_KVSTORE_MOCK_ACCESSTOKEN_KIT_H
#define DISTRIBUTED_KVSTORE_MOCK_ACCESSTOKEN_KIT_H

namespace OHOS {
namespace Security {
namespace AccessToken {
typedef uint32_t AccessTokenID;
typedef enum TypeATokenTypeEnum {
    TOKEN_INVALID = -1,
    TOKEN_HAP = 0,
    TOKEN_NATIVE,
    TOKEN_SHELL,
    TOKEN_TYPE_BUTT,
} ATokenTypeEnum;

class AccessTokenKit {
public:
    static ATokenTypeEnum GetTokenTypeFlag(AccessTokenID tokenID)
    {
        return TOKEN_INVALID;
    }

    static ATokenTypeEnum mockTokenTypeFlag_;
};
}
}
}
#endif // DISTRIBUTED_KVSTORE_MOCK_ACCESSTOKEN_KIT_H
