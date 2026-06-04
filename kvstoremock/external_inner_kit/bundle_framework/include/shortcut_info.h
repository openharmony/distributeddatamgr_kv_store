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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_SHORTCUT_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_SHORTCUT_INFO_H

#include <map>
#include <string>
#include <vector>
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct ShortcutWant {
    std::string bundleName;
    std::string moduleName;
    std::string abilityName;
    std::map<std::string, std::string> parameters;
};

struct Shortcut {
    uint32_t iconId = 0;
    uint32_t labelId = 0;
    std::string shortcutId;
    std::string icon;
    std::string label;
    bool visible = true;
    std::vector<ShortcutWant> wants;
};

struct ShortcutJson {
    std::vector<Shortcut> shortcuts;
};

struct ShortcutIntent {
    std::string targetBundle;
    std::string targetModule;
    std::string targetClass;
    std::map<std::string, std::string> parameters;
};

struct ShortcutInfo : public Parcelable {
    bool isStatic = false;
    bool isHomeShortcut = false;
    bool isEnables = false;
    bool visible = true;
    uint32_t iconId = 0;
    uint32_t labelId = 0;
    int32_t appIndex = 0;
    int32_t sourceType = 0;
    std::string id;
    std::string bundleName;
    std::string moduleName;
    std::string hostAbility;
    std::string icon;
    std::string label;
    std::string disableMessage;
    std::vector<ShortcutIntent> intents;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static ShortcutInfo *Unmarshalling(Parcel &parcel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_SHORTCUT_INFO_H
