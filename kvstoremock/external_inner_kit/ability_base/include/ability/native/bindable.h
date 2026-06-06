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

#ifndef OHOS_ABILITY_RUNTIME_BINDABLE_H
#define OHOS_ABILITY_RUNTIME_BINDABLE_H

#include <memory>
#include <mutex>

class NativeReference;
typedef class __ani_ref *ani_ref;
namespace OHOS {
namespace AbilityRuntime {
class Runtime;

class BindingObject final {
public:
    BindingObject() = default;
    ~BindingObject() = default;

    template<class T>
    void Bind(Runtime& runtime, T* object)
    {
        static_assert(IsValidType<T>(), "T must be ani_ref or NativeReference");
        const std::string typeName = GetTypeString<T>();
        std::lock_guard guard(objectsMutex_);
        std::unique_ptr<void, void (*)(void*)> obj(object, SimpleRelease<T>);
        objects_.emplace(typeName, std::move(obj));
    }

    template<class T>
    void Bind(T* object)
    {
        static_assert(IsValidType<T>(), "T must be ani_ref or NativeReference");
        const std::string typeName = GetTypeString<T>();
        std::lock_guard guard(objectsMutex_);
        std::unique_ptr<void, void (*)(void*)> obj(object, SimpleRelease<T>);
        objects_.emplace(typeName, std::move(obj));
    }

    template<class T>
    T* Get()
    {
        const std::string typeName = GetTypeString<T>();
        std::lock_guard guard(objectsMutex_);
        const auto& iter = objects_.find(typeName);
        if (iter == objects_.end()) {
            return nullptr;
        }
        return static_cast<T*>(iter->second.get());
    }

    void Unbind()
    {
        // Consistency with previous behavior
        Unbind<NativeReference>();
    }

    template<class T>
    void Unbind()
    {
        const std::string typeName = GetTypeString<T>();
        std::lock_guard guard(objectsMutex_);
        const auto& iter = objects_.find(typeName);
        if (iter == objects_.end()) {
            return;
        }
        iter->second.release();
    }

    BindingObject(const BindingObject&) = delete;
    BindingObject& operator=(const BindingObject&) = delete;
    BindingObject(BindingObject&&) = delete;
    BindingObject& operator=(BindingObject&&) = delete;

private:
    template<class T>
    static void SimpleRelease(void* ptr)
    {
        delete static_cast<T*>(ptr);
    }

    template<class T>
    static constexpr bool IsValidType()
    {
        if (std::is_same_v<T, ani_ref> || std::is_same_v<T, NativeReference>) {
            return true;
        }
        return false;
    }

    template<class T>
    static std::string GetTypeString()
    {
        if (std::is_same_v<T, ani_ref>) {
            return "ani_ref";
        } else {
            return "NativeReference";
        }
    }

    std::map<std::string, std::unique_ptr<void, void (*)(void*)>> objects_;
    std::mutex objectsMutex_;
};

class BindingObjectSubThread {
public:
    BindingObjectSubThread() = default;
    virtual ~BindingObjectSubThread() = default;

    virtual void BindSubThreadObject(void* napiEnv, void* object) {}
    virtual void* GetSubThreadObject(void* napiEnv) { return nullptr; }
    virtual void RemoveSubThreadObject(void* napiEnv) {}
    virtual void RemoveAllObject() {}

private:
    BindingObjectSubThread(const BindingObjectSubThread&) = delete;
    BindingObjectSubThread(BindingObjectSubThread&&) = delete;
    BindingObjectSubThread& operator=(const BindingObjectSubThread&) = delete;
    BindingObjectSubThread& operator=(BindingObjectSubThread&&) = delete;
};

class Bindable {
public:
    virtual ~Bindable() = default;

    template<class T>
    void Bind(Runtime& runtime, T* object)
    {
        if (object_) {
            object_->Bind(runtime, object);
        }
    }

    template<class T>
    void Bind(T* object)
    {
        if (object_) {
            object_->Bind(object);
        }
    }

    void Unbind() const
    {
        if (object_) {
            object_->Unbind();
        }

        if (subThreadObject_) {
            subThreadObject_->RemoveAllObject();
        }
    }

    template<class T>
    void Unbind() const
    {
        if (object_) {
            object_->Unbind<T>();
        }

        if (subThreadObject_) {
            subThreadObject_->RemoveAllObject();
        }
    }

    const std::unique_ptr<BindingObject>& GetBindingObject() const
    {
        return object_;
    }

    void BindSubThreadObject(void* napiEnv, void* object)
    {
        if (subThreadObject_) {
            subThreadObject_->BindSubThreadObject(napiEnv, object);
        }
    }

    void* GetSubThreadObject(void* napiEnv)
    {
        if (subThreadObject_) {
            return subThreadObject_->GetSubThreadObject(napiEnv);
        }
        return nullptr;
    }

protected:
    Bindable() = default;
    std::unique_ptr<BindingObjectSubThread> subThreadObject_ = nullptr;

private:
    std::unique_ptr<BindingObject> object_ = std::make_unique<BindingObject>();
};
} // namespace AbilityRuntime
} // namespace OHOS
#endif // OHOS_ABILITY_RUNTIME_BINDABLE_H
