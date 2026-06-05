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

#ifndef FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_ASYNC_COMMON_EVENT_RESULT_H
#define FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_ASYNC_COMMON_EVENT_RESULT_H

#include "iremote_object.h"

namespace OHOS {
namespace EventFwk {
class AsyncCommonEventResult {
public:
    /**
     * Creates a AsyncCommonEventResult instance based on the parameters.
     *
     * @param resultCode Indicates the result code of the current ordered common event.
     * @param resultData Indicates the result data of the current ordered common event.
     * @param ordered Indicates the type of the current ordered common event is ordered or not.
     * @param sticky Indicates the type of the current sticky common event is sticky or not.
     * @param token Indicates the remote object of the current ordered common event.
     */
    AsyncCommonEventResult(const int32_t &resultCode, const std::string &resultData, const bool &ordered,
        const bool &sticky, const wptr<IRemoteObject> &token);

    ~AsyncCommonEventResult();

    /**
     * Sets the result code of the current ordered common event.
     *
     * @param code Indicates the result code of the current ordered common event
     * @return Returns true if successful; false otherwise.
     */
    bool SetCode(const int32_t &code);

    /**
     * Obtains the result code of the current ordered common event.
     *
     * @return Returns the result code of the current ordered common event
     */
    int32_t GetCode() const;

    /**
     * Sets the result data of the current ordered common event.
     *
     * @param data Indicates the result data of the current ordered common event.
     * @return Returns true if successful; false otherwise.
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
     * @return Returns true if successful; false otherwise.
     */
    bool SetCodeAndData(const int32_t &code, const std::string &data);

    /**
     * Cancels the current ordered common event.
     *
     * @return Returns true if successful; false otherwise.
     */
    bool AbortCommonEvent();

    /**
     * Clears the abort state of the current ordered common event.
     *
     * @return Returns true if successful; false otherwise.
     */
    bool ClearAbortCommonEvent();

    /**
     * Checks whether the current ordered common event should be aborted.
     *
     * @return Returns true common event should be aborted; false otherwise.
     */
    bool GetAbortCommonEvent() const;

    /**
     * Finishes processing the current ordered common event.
     *
     * @return Returns true if finishes processing the current ordered common event; false otherwise.
     */
    bool FinishCommonEvent();

    /**
     * Checks whether the current common event is an ordered common event.
     *
     * @return Returns true if the current common event is an ordered common event; false otherwise.
     */
    bool IsOrderedCommonEvent() const;

    /**
     * Checks whether the current common event is a sticky common event.
     *
     * @return Returns true if the current common event is a sticky common event; false otherwise.
     */
    bool IsStickyCommonEvent() const;

    /**
     * Checks whether the current common event is ordered.
     *
     * @return Returns true if the current common event is ordered; false otherwise.
     */
    bool CheckSynchronous() const;

private:
    int32_t resultCode_;
    std::string resultData_;
    bool ordered_;
    bool sticky_;
    wptr<IRemoteObject> token_;
    bool abortEvent_;
    bool finished_;
};
}  // namespace EventFwk
}  // namespace OHOS

#endif  // FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_ASYNC_COMMON_EVENT_RESULT_H