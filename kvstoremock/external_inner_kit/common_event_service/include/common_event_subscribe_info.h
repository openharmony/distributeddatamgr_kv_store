/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_SUBSCRIBE_INFO_H
#define FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_SUBSCRIBE_INFO_H

#include "matching_skills.h"

namespace OHOS {
namespace EventFwk {
class CommonEventSubscribeInfo : public Parcelable {
public:
    enum ThreadMode {
        HANDLER,     // the main thread of this ability.
        POST,        // the event dispatch thread.
        ASYNC,       // an asynchronous thread.
        BACKGROUND,  // the background thread.
        COMMON,      // common listening thread of a process.
    };

    /**
     * A constructor used to create a CommonEventSubscribeInfo instance
     * with the matchingSkills parameters passed.
     *
     * @param matchingSkills Indicates the matching skills.
     */
    explicit CommonEventSubscribeInfo(const MatchingSkills &matchingSkills);

    CommonEventSubscribeInfo();

    /**
     * A constructor used to create a CommonEventSubscribeInfo instance by
     * copying parameters from an existing one.
     *
     *  @param commonEventSubscribeInfo Indicates the commonEventSubscribeInfo.
     */
    explicit CommonEventSubscribeInfo(const CommonEventSubscribeInfo &commonEventSubscribeInfo);

    virtual ~CommonEventSubscribeInfo();

    /**
     * Sets the subscriber priority for this CommonEventSubscribeInfo object.
     *
     * @param priority Indicates the subscriber priority.
     */
    void SetPriority(const int32_t &priority);

    /**
     * Obtains the subscriber priority of this CommonEventSubscribeInfo object.
     *
     * @return Returns the subscriber priority.
     */
    int32_t GetPriority() const;

    /**
     * Sets the subscriber userId for this CommonEventSubscribeInfo object.
     *
     * @param userId Indicates the user ID of the subscriber.
     */
    void SetUserId(const int32_t &userId);

    /**
     * Obtains the subscriber userId of this CommonEventSubscribeInfo object.
     *
     * @return Returns the user ID of the subscriber.
     */
    int32_t GetUserId() const;

    /**
     * Sets the permission that the publisher must have in order to send
     * a common event to this subscriber.
     *
     * @param permission Indicates the subscriber permission.
     */
    void SetPermission(const std::string &permission);

    /**
     * Obtains the publisher permission of this CommonEventSubscribeInfo object.
     *
     * @return Returns the subscriber permission.
     */
    std::string GetPermission() const;

    /**
     * Obtains the thread mode of this CommonEventSubscribeInfo object.
     *
     * @return Returns the thread mode.
     */
    CommonEventSubscribeInfo::ThreadMode GetThreadMode() const;

    /**
     * Sets the thread mode of this CommonEventSubscribeInfo object.
     *
     * @param threadMode Indicates the thread mode to be set, which is specified in ThreadMode. Currently, only the
     * HANDLER mode is supported.
     */
    void SetThreadMode(CommonEventSubscribeInfo::ThreadMode threadMode);

    /**
     * Sets the device ID for this CommonEventSubscribeInfo object.
     * Your application will only receive common events sent from the specified device.
     *
     * @param deviceId Indicates the device ID. The value must be an existing device ID on the same ohos network.
     * Otherwise, it is invalid.
     */
    void SetDeviceId(const std::string &deviceId);

    /**
     * Obtains the device ID in this CommonEventSubscribeInfo object.
     *
     * @return Returns the device ID.
     */
    std::string GetDeviceId() const;

    /**
     * Obtains the MatchingSkills object carried in this CommonEventSubscribeInfo object.
     *
     * @return Returns the matchingSkills object.
     */
    const MatchingSkills &GetMatchingSkills() const;

    /**
     * Sets the publisher bundle name for this CommonEventSubscribeInfo object.
     *
     * @param publisherBundleName Indicates the bundle name of the publisher.
     */
    void SetPublisherBundleName(const std::string &publisherBundleName);

    /**
     * Obtains the publisher bundle name of this CommonEventSubscribeInfo object.
     *
     * @return Returns the bundle name of the publisher.
     */
    std::string GetPublisherBundleName() const;

    /**
     * Sets the publisher uid for this CommonEventSubscribeInfo object.
     *
     * @param publisherUid Indicates the uid of the publisher.
     */
    void SetPublisherUid(int32_t publisherUid);

    /**
     * Obtains the publisher uid of this CommonEventSubscribeInfo object.
     *
     * @return Returns the uid of the publisher.
     */
    int32_t GetPublisherUid() const;

    /**
     * Marshals a subscriber info object into a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     * @return Returns true if success; false otherwise.
     */
    virtual bool Marshalling(Parcel &parcel) const override;

    /**
     * Unmarshals a Parcel object into a subscriber info.
     *
     * @return Returns the subscriber info.
     */
    static CommonEventSubscribeInfo *Unmarshalling(Parcel &parcel);

private:
    /**
     * Read a subscriber object from a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     * @return Returns true if success; false otherwise.
     */
    bool ReadFromParcel(Parcel &parcel);

private:
    MatchingSkills matchingSkills_;
    int32_t priority_;
    int32_t userId_;
    std::string permission_;
    std::string deviceId_;
    CommonEventSubscribeInfo::ThreadMode threadMode_;
    std::string publisherBundleName_;
    int32_t publisherUid_;
};
}  // namespace EventFwk
}  // namespace OHOS

#endif  // FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_SUBSCRIBE_INFO_H
