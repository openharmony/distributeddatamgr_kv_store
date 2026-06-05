/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_MATCHING_SKILLS_H
#define FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_MATCHING_SKILLS_H

#include "parcel.h"
#include "want.h"

namespace OHOS {
namespace EventFwk {
using Want = OHOS::AAFwk::Want;

class MatchingSkills : public Parcelable {
public:
    MatchingSkills();

    /**
     * A constructor used to create a MatchingSkills object instance by using the parameters of an existing
     * MatchingSkills object.
     */
    MatchingSkills(const MatchingSkills &matchingSkills);

    ~MatchingSkills();

    /**
     * Obtains an entity.
     *
     * @param index Indicates the entity index.
     * @return Returns entity.
     */
    std::string GetEntity(size_t index) const;

    /**
     * Adds an entity to this MatchingSkills object.
     *
     * @param entity Indicates the entity.
     */
    void AddEntity(const std::string &entity);

    /**
     * Checks whether the entity is in this MatchingSkills object.
     *
     * @param event Indicates specified entity in MatchingSkills object.
     * @return Returns whether the entity is in MatchingSkills object or not.
     */
    bool HasEntity(const std::string &entity) const;

    /**
     * Removes entity.
     *
     * @param event Indicates specified entity in MatchingSkills object.
     */
    void RemoveEntity(const std::string &entity);

    /**
     * Gets entity count.
     *
     * @return Returns entity count.
     */
    size_t CountEntities() const;

    /**
     * Adds an event to this MatchingSkills object.
     *
     * @param event Indicates the event.
     */
    void AddEvent(const std::string &event);

    /**
     * Gets event count.
     *
     * @return Indicates the event count in MatchingSkills object.
     */
    size_t CountEvent() const;

    /**
     * Gets event.
     *
     * @param index Indicates the scheme index.
     * @return Returns the event in MatchingSkills object.
     */
    std::string GetEvent(size_t index) const;

    /**
     * Gets events.
     *
     * @return Returns the events in MatchingSkills object.
     */
    std::vector<std::string> GetEvents() const;

    /**
     * Removes events.
     *
     * @return Indicates the event in MatchingSkills object.
     */
    void RemoveEvent(const std::string &event);

    /**
     * Checks whether the event is in this MatchingSkills object.
     *
     * @param event Indicates specified event in MatchingSkills object
     * @return Returns whether the event in MatchingSkills object or not.
     */
    bool HasEvent(const std::string &event) const;

    /**
     * Obtains an Scheme.
     *
     * @param index Indicates the scheme index.
     * @return Returns scheme.
     */
    std::string GetScheme(size_t index) const;

    /**
     * Adds scheme to this MatchingSkills object.
     *
     * @param scheme Indicates the scheme.
     */
    void AddScheme(const std::string &scheme);

    /**
     * Checks whether the scheme is in this MatchingSkills object.
     *
     * @param scheme Indicates specified scheme in MatchingSkills object.
     * @return Returns whether the scheme in MatchingSkills object or not.
     */
    bool HasScheme(const std::string &scheme) const;

    /**
     * Removes entity.
     *
     * @param event Indicates the entity in MatchingSkills object.
     */
    void RemoveScheme(const std::string &scheme);

    /**
     * Gets scheme count.
     *
     * @return Returns scheme count.
     */
    size_t CountSchemes() const;

    /**
     * Matches want.
     *
     * @param want Indicates the want in MatchingSkills object.
     * @return Returns the match want result.
     */
    bool Match(const Want &want) const;

    /**
     * Marshals this MatchingSkills object into a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     * @return Returns the marshalling result.
     */
    virtual bool Marshalling(Parcel &parcel) const override;

    /**
     * Unmarshals this MatchingSkills object from a Parcel.
     *
     * @param parcel Indicates the parcel.
     * @return Returns the MatchingSkills.
     */
    static MatchingSkills *Unmarshalling(Parcel &parcel);

    bool WriteVectorInfo(Parcel &parcel, std::vector<std::u16string>vectorInfo) const;

private:
    /**
     * Reads MatchingSkills object from a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     * @return Returns read from parcel success or fail.
     */
    bool ReadFromParcel(Parcel &parcel);

    /**
     * Matches event.
     *
     * @param parcel Indicates the event in want.
     * @return Returns match event success or fail.
     */
    bool MatchEvent(const std::string &event) const;

    /**
     * Matches entity.
     *
     * @param parcel Indicates the entity in want.
     * @return Returns match entity success or fail.
     */
    bool MatchEntity(const std::vector<std::string> &entities) const;

    /**
     * Matches scheme.
     *
     * @param parcel Indicates the scheme in want.
     * @return Returns match scheme success or fail.
     */
    bool MatchScheme(const std::string &scheme) const;

    std::string ToString() const;

private:
    std::vector<std::string> entities_;
    std::vector<std::string> events_;
    std::vector<std::string> schemes_;
    static constexpr int32_t VALUE_NULL = -1;
    static constexpr int32_t VALUE_OBJECT = 1;
    friend class CommonEvent;
};
}  // namespace EventFwk
}  // namespace OHOS

#endif  // FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_MATCHING_SKILLS_H