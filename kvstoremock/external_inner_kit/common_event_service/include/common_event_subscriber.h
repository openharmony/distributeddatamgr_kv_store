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

#ifndef FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_SUBSCRIBER_H
#define FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_SUBSCRIBER_H

#include "async_common_event_result.h"
#include "common_event_data.h"
#include "common_event_subscribe_info.h"

namespace OHOS {
namespace EventFwk {
class CommonEventSubscriber {
public:
    CommonEventSubscriber();

    /**
     * A constructor used to create a CommonEventSubscriber instance with the
     * subscribeInfo parameter passed.
     *
     * @param subscribeInfo Indicates the subscribeInfo
     */
    explicit CommonEventSubscriber(const CommonEventSubscribeInfo &subscribeInfo);

    virtual ~CommonEventSubscriber();

    /**
     * Calls back when the application receives a new common event.
     * Don't call other common event interface such as UnSubscribeCommonEvent() in the callback
     *
     * @param data Indicates the common event data.
     */
    virtual void OnReceiveEvent(const CommonEventData &data) = 0;

    /**
     * Gets common event subscriber info
     *
     * @return Returns common event subscriber info
     */
    const CommonEventSubscribeInfo &GetSubscribeInfo() const;

    /**
     * Sets the common event subscriber info
     *
     * @param subscribeInfo Indicates the common event subscriber info be set
     */
    void SetSubscribeInfo(const CommonEventSubscribeInfo &subscribeInfo);

    /**
     * Sets the result code of the current ordered common event.
     *
     * @param code Indicates the result code of the current ordered common event
     * @return Returns true if success; false otherwise.
     */
    bool SetCode(const int32_t &code);

    /**
     * Obtains the result code of the current ordered common event.
     *
     * @return Returns the result code of the current ordered common event.
     */
    int32_t GetCode() const;

    /**
     * Sets the result data of the current ordered common event.
     *
     * @param data Indicates the result data of the current ordered common event.
     * @return Returns true if success; false otherwise.
     */
    bool SetData(const std::string &data);

    /**
     * Obtains the result data of the current ordered common event.
     *
     * @return Returns the result data of the current ordered common event
     */
    std::string GetData() const;

    /**
     * Sets the result of the current ordered common event.
     *
     * @param code Indicates the result code of the current ordered common event.
     * @param data Indicates the result data of the current ordered common event.
     * @return Returns true if success; false otherwise.
     */
    bool SetCodeAndData(const int32_t &code, const std::string &data);

    /**
     * Cancels the current ordered common event.
     *
     * @return Returns true if success; false otherwise.
     */
    bool AbortCommonEvent();

    /**
     * Clears the abort state of the current ordered common event.
     *
     * @return Returns true if success; false otherwise.
     */
    bool ClearAbortCommonEvent();

    /**
     * Checks whether the current ordered common event should be aborted.
     *
     * @return Returns true if success; false otherwise.
     */
    bool GetAbortCommonEvent() const;

    /**
     * Enables asynchronous processing for the current ordered common event.
     * @return Returns async common event result.
     */
    std::shared_ptr<AsyncCommonEventResult> GoAsyncCommonEvent();

    /**
     * Checks whether the current common event is an ordered common event.

     * @return Returns true if success; false otherwise.
     */
    bool IsOrderedCommonEvent() const;

    /**
     * Checks whether the current common event is a sticky common event.
     *
     * @return Returns true if success; false otherwise.
     */
    bool IsStickyCommonEvent() const;

private:
    /**
     * Sets AsyncCommonEventResult for init before perform onReceiveEvent.
     *
     * @return Returns true if success; false otherwise.
     */
    bool SetAsyncCommonEventResult(const std::shared_ptr<AsyncCommonEventResult> &result);

    /**
     * Gets AsyncCommonEventResult for check after perform onReceiveEvent.
     *
     * @return Returns async common event result.
     */
    std::shared_ptr<AsyncCommonEventResult> GetAsyncCommonEventResult();

    /**
     * Checks whether the current common event is ordered.
     *
     * @return Returns true if success; false otherwise.
     */
    bool CheckSynchronous() const;

private:
    friend class CommonEventListener;

    CommonEventSubscribeInfo subscribeInfo_;
    std::shared_ptr<AsyncCommonEventResult> result_;
};
}  // namespace EventFwk
}  // namespace OHOS

#endif  // FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_SUBSCRIBER_H
