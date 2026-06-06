/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_CALLER_CALLBACK_H
#define OHOS_ABILITY_RUNTIME_CALLER_CALLBACK_H

#include <condition_variable>
#include <mutex>

#include "iremote_object.h"

namespace OHOS {
namespace AbilityRuntime {
constexpr const char* ON_RELEASE = "release";
constexpr const char* ON_DIED = "died";
class LocalCallRecord;
/**
 * @class CallerCallBack
 * CallerCallBack the callback function of caller.
 */
class CallerCallBack : public std::enable_shared_from_this<CallerCallBack> {
public:
    /* Caller's callback object */
    using CallBackClosure = std::function<void(const sptr<IRemoteObject> &)>;
    using OnReleaseClosure = std::function<void(const std::string &)>;
    using OnRemoteStateChangedClosure = std::function<void(const std::string &)>;

    CallerCallBack() = default;
    virtual ~CallerCallBack() = default;

    void SetCallBack(CallBackClosure callback)
    {
        callback_ = callback;
    };
    void SetOnRelease(OnReleaseClosure onRelease)
    {
        onRelease_ = onRelease;
    };
    void SetOnRemoteStateChanged(OnRemoteStateChangedClosure onRemoteStateChanged)
    {
        onRemoteStateChanged_ = onRemoteStateChanged;
    };
    void InvokeCallBack(const sptr<IRemoteObject> &remoteObject)
    {
        if (callback_) {
            callback_(remoteObject);
            isCallBack_ = true;
        }
    };
    void InvokeOnRelease(const std::string &key)
    {
        if (onRelease_) {
            onRelease_(key);
        }
    };
    void InvokeOnNotify(const std::string &state)
    {
        if (onRemoteStateChanged_) {
            onRemoteStateChanged_(state);
        }
    };
    bool IsCallBack() const
    {
        return isCallBack_;
    };

    void SetRecord(const std::weak_ptr<LocalCallRecord> &localCallRecord)
    {
        localCallRecord_ = localCallRecord;
    }

    std::shared_ptr<LocalCallRecord> GetRecord()
    {
        return localCallRecord_.lock();
    }

private:
    CallBackClosure callback_ = {};
    OnReleaseClosure onRelease_ = {};
    OnRemoteStateChangedClosure onRemoteStateChanged_ = {};
    bool isCallBack_ = false;
    std::weak_ptr<LocalCallRecord> localCallRecord_;
};

using ReleaseCallFunc = std::function<ErrCode(std::shared_ptr<CallerCallBack>)>;
struct StartAbilityByCallData {
    StartAbilityByCallData() = default;
    StartAbilityByCallData(StartAbilityByCallData &) = delete;
    void operator=(StartAbilityByCallData &) = delete;
    int32_t err = 0;
    sptr<IRemoteObject> remoteCallee;
    std::mutex mutexlock;
    std::condition_variable condition;
};

class CallUtil {
public:
    static void GenerateCallerCallBack(std::shared_ptr<StartAbilityByCallData> calls,
        std::shared_ptr<CallerCallBack> callerCallBack);
    static void SetOnReleaseOfCallerCallBack(std::shared_ptr<CallerCallBack> callerCallBack);
    static void WaitForCalleeObj(std::shared_ptr<StartAbilityByCallData> callData);
};
} // namespace AbilityRuntime
} // namespace OHOS
#endif // OHOS_ABILITY_RUNTIME_CALLER_CALLBACK_H
