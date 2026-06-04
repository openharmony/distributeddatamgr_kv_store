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

#ifndef FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_BASE_SHARED_BUNDLE_INFO_H
#define FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_BASE_SHARED_BUNDLE_INFO_H

#include <string>
#include <vector>

#include "bundle_constants.h"
#include "hap_module_info.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct BaseSharedBundleInfo : public Parcelable {
    bool compressNativeLibs = true;
    uint32_t versionCode;
    AOTCompileStatus aotCompileStatus = AOTCompileStatus::NOT_COMPILED;
    std::string bundleName;
    std::string moduleName;
    std::string nativeLibraryPath;
    std::string hapPath;
    std::string moduleArkTSMode = Constants::ARKTS_MODE_DYNAMIC;
    std::vector<std::string> nativeLibraryFileNames;
    std::vector<std::string> librarySupportDirectory;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static BaseSharedBundleInfo *Unmarshalling(Parcel &parcel);
};
}
}
#endif  // FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_BASE_SHARED_BUNDLE_INFO_H
