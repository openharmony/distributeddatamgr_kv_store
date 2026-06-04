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

#ifndef OHOS_APPEXECFWK_IVERIFYMANAGER_H
#define OHOS_APPEXECFWK_IVERIFYMANAGER_H

#include <cstdint>
#include <vector>
#include <iremote_broker.h>
#include <string_ex.h>

namespace OHOS {
namespace AppExecFwk {

enum class IVerifyManagerIpcCode {
    COMMAND_VERIFY = 0,
    COMMAND_DELETE_ABC = 2,
};

class IVerifyManager : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.bundleManager.VerifyManager");

    virtual ErrCode Verify(
        const std::vector<std::string>& abcPaths,
        int32_t& funcResult) = 0;

    virtual ErrCode DeleteAbc(
        const std::string& path,
        int32_t& funcResult) = 0;
protected:
    const int VECTOR_MAX_SIZE = 102400;
    const int LIST_MAX_SIZE = 102400;
    const int SET_MAX_SIZE = 102400;
    const int MAP_MAX_SIZE = 102400;
};
} // namespace AppExecFwk
} // namespace OHOS
#endif // OHOS_APPEXECFWK_IVERIFYMANAGER_H
