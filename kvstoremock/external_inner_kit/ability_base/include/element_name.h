/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_BASE_ELEMENT_NAME_H
#define OHOS_ABILITY_BASE_ELEMENT_NAME_H

#include <string>
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
class ElementName : public Parcelable {
public:
    ElementName() {}
    ~ElementName() {}

    std::string GetURI() const { return ""; }
    bool ParseURI(const std::string &uri) { return false; }
    bool operator==(const ElementName &element) const { return false; }

    void SetDeviceID(const std::string &id) {}
    std::string GetDeviceID() const { return ""; }
    void SetBundleName(const std::string &name) {}
    std::string GetBundleName() const { return ""; }
    void SetAbilityName(const std::string &name) {}
    std::string GetAbilityName() const { return ""; }
    void SetModuleName(const std::string &moduleName) {}
    std::string GetModuleName() const { return ""; }

    bool ReadFromParcel(Parcel &parcel) { return false; }
    virtual bool Marshalling(Parcel &parcel) const override { return false; }
    static ElementName *Unmarshalling(Parcel &parcel) { return nullptr; }
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // OHOS_ABILITY_BASE_ELEMENT_NAME_H