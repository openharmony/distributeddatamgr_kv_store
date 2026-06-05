/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_MANAGER_H
#define FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_MANAGER_H

#include "common_event_data.h"
#include "common_event_publish_info.h"
#include "common_event_subscriber.h"

namespace OHOS {
namespace EventFwk {
class CommonEventManager {
public:
    /**
     * Publishes a standard common event.
     *
     * @param data Indicates the common event data.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEvent(const CommonEventData &data);

    /**
     * Publishes a standard common event.
     *
     * @param data Indicates the common event data.
     * @param userId Indicates the user ID.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEventAsUser(const CommonEventData &data, const int32_t &userId);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEvent(const CommonEventData &data, const CommonEventPublishInfo &publishInfo);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t NewPublishCommonEvent(const CommonEventData &data, const CommonEventPublishInfo &publishInfo);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @param userId Indicates the user ID.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEventAsUser(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
        const int32_t &userId);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t NewPublishCommonEventAsUser(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
        const int32_t &userId);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @param subscriber Indicates the common event subscriber.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEvent(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
        const std::shared_ptr<CommonEventSubscriber> &subscriber);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @param subscriber Indicates the common event subscriber.
     * @param userId Indicates the user ID.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEventAsUser(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
        const std::shared_ptr<CommonEventSubscriber> &subscriber, const int32_t &userId);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @param subscriber Indicates the common event subscriber.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t NewPublishCommonEventAsUser(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
        const std::shared_ptr<CommonEventSubscriber> &subscriber, const int32_t &userId);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @param subscriber Indicates the common event subscriber.
     * @param uid Indicates the uid of application.
     * @param callerToken Indicates the caller token.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEvent(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
        const std::shared_ptr<CommonEventSubscriber> &subscriber, const uid_t &uid, const int32_t &callerToken);

    /**
     * Publishes a common event.
     *
     * @param data Indicates the common event data.
     * @param publishInfo Indicates the publish info.
     * @param subscriber Indicates the common event subscriber.
     * @param uid Indicates the Uid of application.
     * @param callerToken Indicates the caller token.
     * @param userId Indicates the user ID.
     * @return Returns true if success; false otherwise.
     */
    static bool PublishCommonEventAsUser(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
        const std::shared_ptr<CommonEventSubscriber> &subscriber, const uid_t &uid, const int32_t &callerToken,
        const int32_t &userId);

    /**
     * Subscribes to common events.
     *
     * @param subscriber Indicates the common event subscriber.
     * @return Returns true if success; false otherwise.
     */
    static bool SubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber);

    /**
     * Subscribes to common events, This interface supports subscription updates.
     *
     * @param subscriber Indicates the common event subscriber.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t Subscribe(const std::shared_ptr<CommonEventSubscriber> &subscriber);

    /**
     * Subscribes to common events.
     *
     * @param subscriber Indicates the common event subscriber.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t NewSubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber);

    /**
     * Unsubscribes from common events.
     *
     * @param subscriber Indicates the common event subscriber.
     * @return Returns true if success; false otherwise.
     */
    static bool UnSubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber);

    /**
     * Unsubscribes from common events.
     *
     * @param subscriber Indicates the common event subscriber.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t NewUnSubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber);

    /**
     * Synchronized, unsubscribes from common events.
     *
     * @param subscriber Indicates the common event subscriber.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t NewUnSubscribeCommonEventSync(const std::shared_ptr<CommonEventSubscriber> &subscriber);

    /**
     * Gets the current sticky common event
     *
     * @param event Indicates the common event.
     * @param data Indicates the common event data.
     * @return Returns true if success; false otherwise.
     */
    static bool GetStickyCommonEvent(const std::string &event, CommonEventData &data);

    /**
     * Freezes application.
     *
     * @param uid Indicates the uid of application.
     * @return Returns true if successful; false otherwise.
     */
    static bool Freeze(const uid_t &uid);

    /**
     * Unfreezes application.
     *
     * @param uid Indicates the uid of application.
     * @return Returns true if successful; false otherwise.
     */
    static bool Unfreeze(const uid_t &uid);

    /**
     * Unfreezes all frozen applications.
     *
     * @return Returns true if successful; false otherwise.
     */
    static bool UnfreezeAll();

    /**
     * Remove sticky common event.
     *
     * @param event Name of the common event.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t RemoveStickyCommonEvent(const std::string &event);

    /**
     * Set Static Subscriber State.
     *
     * @param enable static subscriber state.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t SetStaticSubscriberState(bool enable);

    /**
     * Set static subscriber state.
     *
     * @param events Static subscriber event name.
     * @param enable Static subscriber state.
     * @return Returns ERR_OK if success; otherwise failed.
     */
    static int32_t SetStaticSubscriberState(const std::vector<std::string> &events, bool enable);

    /**
    * Set freeze status of process.
    *
    * @param pidList Indicates the list of process id.
    * @param isFreeze Indicates wheather the process is freezed.
    * @return Returns true if successful; false otherwise.
    */
    static bool SetFreezeStatus(std::set<int> pidList, bool isFreeze);
};
}  // namespace EventFwk
}  // namespace OHOS

#endif  // FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_MANAGER_H
