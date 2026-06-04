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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_STREAM_INSTALLER_INTERFACE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_STREAM_INSTALLER_INTERFACE_H

#include "bundle_constants.h"
#include "iremote_broker.h"

namespace OHOS {
namespace AppExecFwk {
class IBundleStreamInstaller : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.BundleStreamInstaller");

    virtual int32_t CreateStream(const std::string &fileName)
    {
        return Constants::DEFAULT_STREAM_FD;
    }
    virtual int32_t CreateSignatureFileStream(const std::string &moduleName, const std::string &fileName)
    {
        return Constants::DEFAULT_STREAM_FD;
    }
    virtual int32_t CreateSharedBundleStream(const std::string &hspName, uint32_t sharedBundleIdx)
    {
        return Constants::DEFAULT_STREAM_FD;
    }
    virtual int32_t CreatePgoFileStream(const std::string &moduleName, const std::string &fileName)
    {
        return Constants::DEFAULT_STREAM_FD;
    }
    virtual int32_t CreateExtProfileFileStream(const std::string &fileName)
    {
        return Constants::DEFAULT_STREAM_FD;
    }
    virtual bool Install()
    {
        return false;
    }
    virtual uint32_t GetInstallerId() const
    {
        return Constants::DEFAULT_INSTALLERID;
    }
    virtual void SetInstallerId(uint32_t installerId) {};
    virtual void UnInit() {};
};
} // AppExecFwk
} // OHOS

#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_STREAM_INSTALLER_INTERFACE_H
