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

#ifndef FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_INSTALL_PARAM_H
#define FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_INSTALL_PARAM_H

#include <map>
#include <string>
#include <vector>

#include "application_info.h"
#include "bundle_constants.h"
#include "parcel.h"
namespace OHOS {
namespace AppExecFwk {
enum class InstallFlag : int8_t {
    NORMAL = 0,
    // Allow to replace the existing bundle when the new version isn't lower than the old one.
    // If the bundle does not exist, just like normal flag.
    REPLACE_EXISTING = 1,
    FREE_INSTALL = 0x10,
};

enum class InstallLocation : int8_t {
    INTERNAL_ONLY = 1,
    PREFER_EXTERNAL = 2,
};

enum class PermissionStatus : int8_t {
    NOT_VERIFIED_PERMISSION_STATUS = 0,
    HAVE_PERMISSION_STATUS,
    NON_HAVE_PERMISSION_STATUS
};

// provides parameters required for installing or uninstalling an application
struct InstallParam : public Parcelable {
    // is keep user data while uninstall.
    bool isKeepData = false;
    bool needSavePreInstallInfo = false;
    bool isPreInstallApp = false;
    bool removable = true;
    // whether need copy hap to install path
    bool copyHapToInstallPath = true;
    // is aging Cause uninstall.
    bool isAgingUninstall = false;
    bool needSendEvent = true;
    bool withCopyHaps = false;
    // for MDM self update
    bool isSelfUpdate = false;
    // is shell token
    bool isCallByShell = false;
    // for AOT
    bool isOTA = false;
    bool concentrateSendEvent = false;
    bool isRemoveUser = false;
    bool isFirstBootInstall = false;
    bool isCreateUser = false;
    bool allUser = false;
    bool isPatch = false;
    bool isDataPreloadHap = false;
    // allow patch app downgrade install
    bool allowPatchDowngrade = false;
    int32_t userId = Constants::UNSPECIFIED_USERID;
    InstallFlag installFlag = InstallFlag::NORMAL;
    InstallLocation installLocation = InstallLocation::INTERNAL_ONLY;
    // status of install bundle permission
    PermissionStatus installBundlePermissionStatus = PermissionStatus::NOT_VERIFIED_PERMISSION_STATUS;
    // status of install enterprise bundle permission
    PermissionStatus installEnterpriseBundlePermissionStatus = PermissionStatus::NOT_VERIFIED_PERMISSION_STATUS;
    // status of install enterprise normal bundle permission
    PermissionStatus installEtpNormalBundlePermissionStatus = PermissionStatus::NOT_VERIFIED_PERMISSION_STATUS;
    // status of install enterprise mdm bundle permission
    PermissionStatus installEtpMdmBundlePermissionStatus = PermissionStatus::NOT_VERIFIED_PERMISSION_STATUS;
    // status of install internaltesting bundle permission
    PermissionStatus installInternaltestingBundlePermissionStatus = PermissionStatus::NOT_VERIFIED_PERMISSION_STATUS;
    // status of mdm update bundle for self
    PermissionStatus installUpdateSelfBundlePermissionStatus = PermissionStatus::NOT_VERIFIED_PERMISSION_STATUS;
    ApplicationInfoFlag preinstallSourceFlag = ApplicationInfoFlag::FLAG_INSTALLED;
    int64_t crowdtestDeadline = Constants::INVALID_CROWDTEST_DEADLINE; // for crowdtesting type hap
    // Indicates the distribution type
    std::string specifiedDistributionType = "";
    // Indicates the additional Info
    std::string additionalInfo = "";
    std::string appIdentifier;
    // shared bundle directory paths
    std::vector<std::string> sharedBundleDirPaths;
    std::map<std::string, std::string> parameters;
    // the profile-guided optimization(PGO) file path
    std::map<std::string, std::string> pgoParams;
    std::map<std::string, std::string> hashParams;
    // utilizing for code-signature
    std::map<std::string, std::string> verifyCodeParams;
    // the parcel object function is not const.
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static InstallParam *Unmarshalling(Parcel &parcel);

private:
    // should force uninstall when delete userinfo.
    bool forceExecuted = false;
    // OTA upgrade skips the killing process
    bool killProcess = true;
    // system app can be uninstalled when uninstallUpdates
    bool isUninstallAndRecover = false;

public:
    static constexpr const char* RENAME_INSTALL_KEY = "ohos.bms.param.renameInstall";
    static constexpr const char* ENTERPRISE_FOR_ALL_USER_KEY = "ohos.bms.param.enterpriseForAllUser";
    static constexpr const char* PARAMETERS_VALUE_TRUE = "true";

    bool GetForceExecuted() const
    {
        return forceExecuted;
    }

    void SetForceExecuted(bool value)
    {
        if (CheckPermission()) {
            forceExecuted = value;
        }
    }

    bool GetKillProcess() const
    {
        return killProcess;
    }

    void SetKillProcess(bool value)
    {
        if (CheckPermission()) {
            killProcess = value;
        }
    }

    bool GetIsUninstallAndRecover() const
    {
        return isUninstallAndRecover;
    }

    void SetIsUninstallAndRecover(bool value)
    {
        if (CheckPermission()) {
            isUninstallAndRecover = value;
        }
    }

    bool IsRenameInstall() const
    {
        return parameters.find(RENAME_INSTALL_KEY) != parameters.end() &&
            parameters.at(RENAME_INSTALL_KEY) == PARAMETERS_VALUE_TRUE;
    }

    bool IsVerifyUninstallRule() const
    {
        return parameters.find(Constants::VERIFY_UNINSTALL_RULE_KEY) != parameters.end() &&
            parameters.at(Constants::VERIFY_UNINSTALL_RULE_KEY) == Constants::VERIFY_UNINSTALL_RULE_VALUE;
    }

    bool IsEnterpriseForAllUser() const
    {
        return parameters.find(ENTERPRISE_FOR_ALL_USER_KEY) != parameters.end() &&
            parameters.at(ENTERPRISE_FOR_ALL_USER_KEY) == PARAMETERS_VALUE_TRUE;
    }

    bool IsForcedUninstall() const
    {
        return parameters.find(Constants::VERIFY_UNINSTALL_FORCED_KEY) != parameters.end() &&
               parameters.at(Constants::VERIFY_UNINSTALL_FORCED_KEY) == Constants::VERIFY_UNINSTALL_FORCED_VALUE;
    }

private:
    bool CheckPermission() const;
};

struct UninstallParam : public Parcelable {
    int32_t versionCode = Constants::ALL_VERSIONCODE;
    int32_t userId = Constants::UNSPECIFIED_USERID;
    std::string bundleName;
    std::string moduleName;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static UninstallParam *Unmarshalling(Parcel &parcel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_INSTALL_PARAM_H
