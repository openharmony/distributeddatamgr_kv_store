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
#ifndef OHOS_SECURITY_ACCESSTOKEN_ACCESSTOKENKIT_MOCK_H
#define OHOS_SECURITY_ACCESSTOKEN_ACCESSTOKENKIT_MOCK_H

#include "accesstoken_kit.h"
#include "access_token.h"
#include <gmock/gmock.h>

namespace OHOS {
namespace Security {
namespace AccessToken {
class BAccessTokenKit {
public:
    virtual ATokenTypeEnum GetTokenTypeFlag(AccessTokenID) = 0;
    BAccessTokenKit() = default;
    virtual ~BAccessTokenKit() = default;
public:
    static inline std::shared_ptr<BAccessTokenKit> accessTokenKit = nullptr;
};

class AccessTokenKitMock : public BAccessTokenKit {
public:
    MOCK_METHOD(ATokenTypeEnum, GetTokenTypeFlag, (AccessTokenID));
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // OHOS_SECURITY_ACCESSTOKEN_ACCESSTOKENKIT_MOCK_H