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

#ifndef OHOS_RELATIONAL_STORE_JS_NAPI_COMMON_JS_PROXY_H
#define OHOS_RELATIONAL_STORE_JS_NAPI_COMMON_JS_PROXY_H
#include <memory>
namespace OHOS::JSProxy {
template<typename T>
class JSCreator {
public:
   virtual std::shared_ptr<T> Create() = 0;
protected:
   JSCreator() = default;
   virtual ~JSCreator() = default;
};

template<typename T>
class JSProxy {
public:
    void SetInstance(std::shared_ptr<T> instance)
    {
        instance_ = std::move(instance);
    }

    std::shared_ptr<T>& GetInstance()
    {
        return instance_;
    }

    std::shared_ptr<T> GetInstance() const
    {
        return instance_;
    }

protected:
    JSProxy() = default;
    virtual ~JSProxy() = default;

private:
    std::shared_ptr<T> instance_;
};

template<typename T, typename U = T>
class JSEntity : public JSCreator<U>, public JSProxy<T> {
protected:
   JSEntity() = default;
   virtual ~JSEntity() = default;
};
} // namespace OHOS::Proxy
#endif // OHOS_RELATIONAL_STORE_JS_NAPI_COMMON_JS_PROXY_H
