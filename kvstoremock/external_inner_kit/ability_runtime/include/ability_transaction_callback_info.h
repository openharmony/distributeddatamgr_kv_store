/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_ABILITY_TRANSACTION_CALLBACK_INFO_H
#define OHOS_ABILITY_ABILITY_TRANSACTION_CALLBACK_INFO_H

#include <memory>
#include <stack>

namespace OHOS {
namespace AppExecFwk {
struct OnPrepareTerminationResult {
    int32_t prepareTermination;
    bool isExist;
};

template<typename T = void>
class AbilityTransactionCallbackInfo {
public:
    using CallbackFunc = std::function<void(T&)>;

    AbilityTransactionCallbackInfo() = default;
    ~AbilityTransactionCallbackInfo() = default;

    static AbilityTransactionCallbackInfo *Create()
    {
        return new(std::nothrow) AbilityTransactionCallbackInfo();
    }

    static void Destroy(AbilityTransactionCallbackInfo *callbackInfo, void* data = nullptr)
    {
        callbackInfo->Finalize(data);
        delete callbackInfo;
    }

    void Push(const CallbackFunc &callback)
    {
        callbackStack_.push(callback);
    }

    void SetFinalizeCallback(const std::function<void(void*)> &finalize)
    {
        finalizeCallback_ = finalize;
    }

    void Finalize(void* data)
    {
        if (finalizeCallback_ != nullptr) {
            finalizeCallback_(data);
            finalizeCallback_ = nullptr;
        }
    }

    void Call(T &callbackResult)
    {
        while (!callbackStack_.empty()) {
            CallbackFunc &callbackFunc = callbackStack_.top();
            callbackFunc(callbackResult);
            callbackStack_.pop();
        }
    }

    bool IsEmpty() const
    {
        return callbackStack_.empty();
    }

private:
    std::stack<CallbackFunc> callbackStack_ {};
    std::function<void(void*)> finalizeCallback_ {};
};

template<>
class AbilityTransactionCallbackInfo<void> {
public:
    using CallbackFunc = std::function<void()>;

    AbilityTransactionCallbackInfo() = default;
    ~AbilityTransactionCallbackInfo() = default;

    static AbilityTransactionCallbackInfo *Create()
    {
        return new(std::nothrow) AbilityTransactionCallbackInfo();
    }

    static void Destroy(AbilityTransactionCallbackInfo *callbackInfo, [[maybe_unused]]void* data = nullptr)
    {
        delete callbackInfo;
    }

    void Push(const CallbackFunc &callback)
    {
        callbackStack_.push(callback);
    }

    void Call()
    {
        while (!callbackStack_.empty()) {
            CallbackFunc &callbackFunc = callbackStack_.top();
            callbackFunc();
            callbackStack_.pop();
        }
    }

    bool IsEmpty() const
    {
        return callbackStack_.empty();
    }

private:
    std::stack<CallbackFunc> callbackStack_ {};
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif // OHOS_ABILITY_ABILITY_TRANSACTION_CALLBACK_INFO_H
