/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SERVICES_SAFWK_INCLUDE_SYSTEM_ABILITY_ONDEMAND_REASON_H
#define SERVICES_SAFWK_INCLUDE_SYSTEM_ABILITY_ONDEMAND_REASON_H

#include <map>
#include "parcel.h"

namespace OHOS {
enum class OnDemandReasonId {
    INTERFACE_CALL = 0,
    DEVICE_ONLINE = 1,
    SETTING_SWITCH = 2,
    PARAM = 3,
    COMMON_EVENT = 4,
    TIMED_EVENT = 5,
    UNREF_EVENT = 6,
};

class OnDemandReasonExtraData : public Parcelable {
public:
    OnDemandReasonExtraData() = default;
    OnDemandReasonExtraData(int32_t code, const std::string& data, const std::map<std::string, std::string>& want);
    std::string GetData() const;
    int32_t GetCode() const;
    const std::map<std::string, std::string>& GetWant() const;
    bool Marshalling(Parcel& parcel) const override;
    static OnDemandReasonExtraData *Unmarshalling(Parcel& parcel);
private:
    int32_t code_ = -1;
    std::string data_;
    std::map<std::string, std::string> want_;
};

class SystemAbilityOnDemandReason {
public:
    SystemAbilityOnDemandReason() = default;
    SystemAbilityOnDemandReason(OnDemandReasonId reasonId, const std::string& reasonName,
        const std::string& reasonValue, int64_t extraDataId);
    OnDemandReasonId GetId() const;
    void SetId(OnDemandReasonId reasonId);
    std::string GetName() const;
    void SetName(const std::string& reasonName);
    std::string GetValue() const;
    void SetValue(const std::string& reasonValue);
    int64_t GetExtraDataId() const;
    void SetExtraDataId(int64_t extraDataId);
    bool HasExtraData() const;
    void SetExtraData(OnDemandReasonExtraData& extraData);
    const OnDemandReasonExtraData& GetExtraData() const;
private:
    OnDemandReasonId reasonId_;
    std::string reasonName_;
    std::string reasonValue_;
    int64_t extraDataId_ = -1;
    OnDemandReasonExtraData extraData_;
};
}
#endif /* SERVICES_SAFWK_INCLUDE_SYSTEM_ABILITY_ONDEMAND_REASON_H */