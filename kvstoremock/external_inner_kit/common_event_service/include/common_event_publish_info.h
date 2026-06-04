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

#ifndef FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_PUBLISH_INFO_H
#define FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_PUBLISH_INFO_H

#include "parcel.h"

namespace OHOS {
namespace EventFwk {

enum SubscriberType {
    ALL_SUBSCRIBER_TYPE,
    SYSTEM_SUBSCRIBER_TYPE
};

enum class ValidationRule {
    AND = 1,
    OR = 2,
};

static constexpr uint16_t SUBSCRIBER_FILTER_BUNDLE_INDEX = 1 << 0;
static constexpr uint16_t SUBSCRIBER_FILTER_PERMISSION_INDEX = 1 << 1;
static constexpr uint16_t SUBSCRIBER_FILTER_SUBSCRIBER_TYPE_INDEX = 1 << 2;
static constexpr uint16_t SUBSCRIBER_FILTER_SUBSCRIBER_UID_INDEX = 1 << 3;
static constexpr uint16_t SUBSCRIBER_FILTER_VERSION = 1 << 4;
static constexpr int32_t UNINITIALIZATED_SUBSCRIBER_TYPE = -1;

class CommonEventPublishInfo : public Parcelable {
public:
    CommonEventPublishInfo();

    /**
     * A constructor used to create a CommonEventPublishInfo instance by copying
     * parameters from an existing one.
     *
     * @param commonEventPublishInfo Indicates the publish info.
     */
    explicit CommonEventPublishInfo(const CommonEventPublishInfo &commonEventPublishInfo);

    ~CommonEventPublishInfo();

    /**
     * Sets whether the type of a common event is sticky or not.
     *
     * @param sticky Indicates the type of a common event is sticky or not
     */
    void SetSticky(bool sticky);

    /**
     * Obtains whether it is a sticky common event, which can be set
     * by SetSticky(bool).
     *
     * @return Returns the common event is sticky or not.
     */
    bool IsSticky() const;

    /**
     * Sets permissions for subscribers.
     *
     * @param subscriberPermissions Indicates the permissions for subscribers.
     */
    void SetSubscriberPermissions(const std::vector<std::string> &subscriberPermissions);

    /**
     * Obtains subscriber permissions to a common event, which can be set by
     * setSubscriberPermissions(const std::vector<std::string>&).
     *
     * @return Returns the permissions for subscribers.
     */
    const std::vector<std::string> &GetSubscriberPermissions() const;

    /**
     * Sets whether the type of a common event is ordered or not.
     *
     * @param ordered Indicates the type of a common event is ordered or not.
     */
    void SetOrdered(bool ordered);

    /**
     *
     * Obtains whether it is an ordered common event, which can be set by setOrdered(boolean).
     * set by SetOrdered(bool).
     *
     * @return Returns the type of a common event is ordered or not.
     */
    bool IsOrdered() const;

    /**
     * Sets the bundleName.
     *
     * @param bundleName Indicates the bundleName of a common event.
     */
    void SetBundleName(const std::string &bundleName);

    /**
     * Sets uid of subscriber. A maximum of three receiver UIDs are supported.
     * If there are more subscribers, the publisher needs to control the permission.
     *
     * @param subscriberUids Indicates the uids of subscribers.
     */
    void SetSubscriberUid(const std::vector<int32_t> &subscriberUids);
 
    /**
     * Obtains the uids of subscribers of this CommonEventPublishInfo object.
     *
     * @return Returns the uids of subscribers.
     */
    std::vector<int32_t> GetSubscriberUid() const;
 
    /**
     * Sets type of subscriber.
     *
     * @param subscriberType Indicates the type of subscriber, which can be ALL_SUBSCRIBER_TYPE = 0
     * or SYSTEM_SUBSCRIBER_TYPE = 1. default is ALL_SUBSCRIBER_TYPE when param is out of range.
     */
    void SetSubscriberType(const int32_t &subscriberType);
 
    /**
     * Obtains the type of subscriber of this CommonEventPublishInfo object.
     *
     * @return Returns the type of subscriber.
     */
    int32_t GetSubscriberType() const;

    /**
     * Obtains the bundleName of a common event
     *
     * @return Returns the bundleName of a common event.
     */
    std::string GetBundleName() const;

    /**
     * Sets validationRule of subscriber.
     *
     * @param rule Indicates the validation rule of subscriber, which can be AND or OR. default is AND.
     */
    void SetValidationRule(const ValidationRule &rule);

    /**
     * Obtains the validationRule of a common event
     *
     * @return Returns the validationRule of a common event.
     */
    ValidationRule GetValidationRule() const;

    /**
     * Set a version. Subscribers whose version is later than this version cannot receive common event.
     *
     * @param version version number.
     */
    void SetSubscriberMaximumVersion(const int32_t version);

    /**
     * Obtains the version of a common event
     *
     * @return Returns the version of a common event.
     */
    int32_t GetSubscriberMaximumVersion() const;

    /**
     * Obtains the settings of filter parameters
     *  SUBSCRIBER_FILTER_BUNDLE_INDEX = 1 << 0;
     *  SUBSCRIBER_FILTER_PERMISSION_INDEX = 1 << 1;
     *  SUBSCRIBER_FILTER_SUBSCRIBER_TYPE_INDEX = 1 << 2;
     *  SUBSCRIBER_FILTER_SUBSCRIBER_UID_INDEX = 1 << 3;
     *  SUBSCRIBER_FILTER_VERSION = 1 << 4;
     *
     * @return Returns the bits of filter settings.
     */
    uint16_t GetFilterSettings() const;

    /**
     * Marshals a CommonEventData object into a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     * @return Returns true if success; false otherwise.
     */
    virtual bool Marshalling(Parcel &parcel) const override;

    /**
     * UnMarshals a Parcel object into a CommonEventData.
     *
     * @return Returns the common event data.
     */
    static CommonEventPublishInfo *Unmarshalling(Parcel &parcel);

private:
    /**
     * Reads a CommonEventData object from a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     */
    bool ReadFromParcel(Parcel &parcel);

    bool isSubscriberType(int32_t subsciberType);

private:
    bool sticky_;
    bool ordered_;
    std::string bundleName_;
    std::vector<std::string> subscriberPermissions_;
    std::vector<int32_t> subscriberUids_;
    int32_t subscriberType_;
    ValidationRule rule_;
    int32_t maximumVersion_ = -1;
};
}  // namespace EventFwk
}  // namespace OHOS

#endif  // FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_PUBLISH_INFO_H