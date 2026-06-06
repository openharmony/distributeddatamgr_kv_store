/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_START_SETTING_H
#define OHOS_ABILITY_RUNTIME_ABILITY_START_SETTING_H

#include <map>
#include <set>
#include <string>
#include <memory>
#include "parcel.h"

namespace OHOS {
namespace AAFwk {
class AbilityStartSetting final : public Parcelable, public std::enable_shared_from_this<AbilityStartSetting> {
public:
    static const std::string BOUNDS_KEY;
    static const std::string WINDOW_DISPLAY_ID_KEY;
    static const std::string WINDOW_MODE_KEY;
    static const std::string DEFAULT_RECOVERY_KEY;
    static const std::string IS_START_BY_SCB_KEY;

    /**
     * @brief Construct copy function.
     * @param other indicates instance of abilitystartsetting object
     * @return none.
     */
    AbilityStartSetting(const AbilityStartSetting &other);
    /**
     * @brief Overload assignment operation.
     * @param other indicates instance of abilitystartsetting object.
     * @return Returns current instance of abilitystartsetting object.
     */
    AbilityStartSetting &operator=(const AbilityStartSetting &other);

    virtual ~AbilityStartSetting() = default;

    /**
     * @brief Obtains the names of all the attributes that have been added to this AbilityStartSetting object.
     *
     * @return Returns the set of attribute names included in this AbilityStartSetting object.
     */
    std::set<std::string> GetPropertiesKey();

    /**
     * @brief Obtains the names of all the attributes that have been added to this AbilityStartSetting object.
     *
     * @return Returns the set of attribute names included in this AbilityStartSetting object.
     */
    static std::shared_ptr<AbilityStartSetting> GetEmptySetting();

    /**
     * @brief Checks whether this AbilityStartSetting object is empty.
     *
     * @return Returns true if this AbilityStartSetting object is empty and animatorOption is null; returns false
     * otherwise.
     */
    bool IsEmpty();
    /**
     * @brief Sets the names of all the attributes of the AbilityStartSetting object.
     *
     * @param key Indicates the name of the key.
     * @param value The window display mode of the values.
     */
    void AddProperty(const std::string &key, const std::string &value);

    /**
     * @brief Gets the name of the attributes of the AbilityStartSetting object.
     *
     * @param key Indicates the name of the key.
     * @return Returns value Indicates the value of the attributes of the AbilityStartSetting object
     */
    std::string GetProperty(const std::string &key);

    /*
     * @brief Write the data of AbilityStartSetting to the file stream
     * @param parcel indicates write the data of AbilityStartSetting to the file stream through parcel
     * @return bool
     */
    bool Marshalling(Parcel &parcel) const;

    /**
     * @brief Reading file stream through parcel to generate AbilityStartSetting instance
     * @param parcel indicates reading file stream through parcel to generate AbilityStartSetting instance
     * @return AbilityStartSetting shared_ptr
     */
    static AbilityStartSetting *Unmarshalling(Parcel &parcel);

protected:
    AbilityStartSetting() = default;
    friend std::shared_ptr<AbilityStartSetting> AbilityStartSettingCreator();

private:
    std::map<std::string, std::string> abilityStarKey_;
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_ABILITY_START_SETTING_H
