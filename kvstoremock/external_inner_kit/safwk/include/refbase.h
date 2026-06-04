/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef UTILS_BASE_REFBASE_H
#define UTILS_BASE_REFBASE_H

#include <atomic>
#include <functional>

namespace OHOS {

#define INITIAL_PRIMARY_VALUE (1 << 28)

class RefBase;
class RefCounter {
public:
    using RefPtrCallback = std::function<void()>;

    RefCounter() : atomicStrong_(INITIAL_PRIMARY_VALUE), atomicWeak_(INITIAL_PRIMARY_VALUE),
                   atomicRefCount_(1), atomicFlags_(0), atomicAttempt_(0) {}

    explicit RefCounter(RefCounter *counter) : atomicStrong_(INITIAL_PRIMARY_VALUE),
        atomicWeak_(INITIAL_PRIMARY_VALUE), atomicRefCount_(1), atomicFlags_(0), atomicAttempt_(0) {}

    RefCounter &operator=(const RefCounter &counter) { return *this; }

    virtual ~RefCounter() {}

    void SetCallback(const RefPtrCallback &callback) { callback_ = callback; }
    void RemoveCallback() { callback_ = nullptr; }

    int GetRefCount() { return atomicRefCount_.load(); }
    void IncRefCount() { atomicRefCount_++; }
    void DecRefCount() { atomicRefCount_--; }
    bool IsRefPtrValid() { return atomicRefCount_.load() > 0; }

    int IncStrongRefCount(const void *objectId) { return ++atomicStrong_; }
    int DecStrongRefCount(const void *objectId) { return --atomicStrong_; }
    int GetStrongRefCount() { return atomicStrong_.load(); }

    int IncWeakRefCount(const void *objectId) { return ++atomicWeak_; }
    int DecWeakRefCount(const void *objectId) { return --atomicWeak_; }
    int GetWeakRefCount() { return atomicWeak_.load(); }

    void SetAttemptAcquire() { atomicAttempt_ = 1; }
    bool IsAttemptAcquireSet() { return atomicAttempt_.load() == 1; }
    void ClearAttemptAcquire() { atomicAttempt_ = 0; }

    bool AttemptIncStrongRef(const void *objectId, int &outCount) {
        outCount = ++atomicStrong_;
        return true;
    }

    bool IsLifeTimeExtended() {
        return (atomicFlags_.load() & FLAG_EXTEND_LIFE_TIME) != 0;
    }

    void ExtendObjectLifetime() { atomicFlags_ |= FLAG_EXTEND_LIFE_TIME; }

private:
    std::atomic<int> atomicStrong_;
    std::atomic<int> atomicWeak_;
    std::atomic<int> atomicRefCount_;
    std::atomic<unsigned int> atomicFlags_;
    std::atomic<int> atomicAttempt_;
    RefPtrCallback callback_ = nullptr;
    static constexpr unsigned int FLAG_EXTEND_LIFE_TIME = 0x00000002;
};

class WeakRefCounter {
public:
    WeakRefCounter(RefCounter *counter, void *cookie) : atomicWeak_(1), refCounter_(counter), cookie_(cookie) {}
    virtual ~WeakRefCounter() {}

    void *GetRefPtr() { return cookie_; }

    void IncWeakRefCount(const void *objectId) { atomicWeak_++; }
    void DecWeakRefCount(const void *objectId) { atomicWeak_--; }

    bool AttemptIncStrongRef(const void *objectId) {
        if (refCounter_ != nullptr) {
            int outCount = 0;
            return refCounter_->AttemptIncStrongRef(objectId, outCount);
        }
        return false;
    }

private:
    std::atomic<int> atomicWeak_;
    RefCounter *refCounter_ = nullptr;
    void *cookie_ = nullptr;
};

class RefBase {
public:
    RefBase() : refs_(new RefCounter()) {}
    RefBase(const RefBase &other) : refs_(new RefCounter()) {}
    RefBase &operator=(const RefBase &other) { return *this; }
    RefBase(RefBase &&other) noexcept : refs_(other.refs_) { other.refs_ = nullptr; }
    RefBase &operator=(RefBase &&other) noexcept {
        if (refs_ != nullptr) delete refs_;
        refs_ = other.refs_;
        other.refs_ = nullptr;
        return *this;
    }
    virtual ~RefBase() {
        if (refs_ != nullptr) {
            delete refs_;
            refs_ = nullptr;
        }
    }

    virtual void RefPtrCallback() {}

    void ExtendObjectLifetime() { if (refs_) refs_->ExtendObjectLifetime(); }

    void IncStrongRef(const void *objectId) { if (refs_) refs_->IncStrongRefCount(objectId); }
    void DecStrongRef(const void *objectId) { if (refs_) refs_->DecStrongRefCount(objectId); }
    int GetSptrRefCount() { return refs_ ? refs_->GetStrongRefCount() : 0; }

    WeakRefCounter *CreateWeakRef(void *cookie) { return new WeakRefCounter(refs_, cookie); }

    void IncWeakRef(const void *objectId) { if (refs_) refs_->IncWeakRefCount(objectId); }
    void DecWeakRef(const void *objectId) { if (refs_) refs_->DecWeakRefCount(objectId); }
    int GetWptrRefCount() { return refs_ ? refs_->GetWeakRefCount() : 0; }

    bool AttemptAcquire(const void *objectId) {
        if (refs_) {
            refs_->SetAttemptAcquire();
            return true;
        }
        return false;
    }

    bool AttemptIncStrongRef(const void *objectId) {
        if (refs_) {
            int outCount = 0;
            return refs_->AttemptIncStrongRef(objectId, outCount);
        }
        return false;
    }

    bool IsAttemptAcquireSet() { return refs_ ? refs_->IsAttemptAcquireSet() : false; }
    bool IsExtendLifeTimeSet() { return refs_ ? refs_->IsLifeTimeExtended() : false; }

    virtual void OnFirstStrongRef(const void *objectId) {}
    virtual void OnLastStrongRef(const void *objectId) {}
    virtual void OnLastWeakRef(const void *objectId) {}
    virtual bool OnAttemptPromoted(const void *objectId) { return true; }

private:
    RefCounter *refs_ = nullptr;
};

template<typename T> class wptr;

template<typename T>
class sptr {
    friend class wptr<T>;

public:
    sptr() : refs_(nullptr) {}
    ~sptr() {
        if (refs_ != nullptr) {
            refs_->DecStrongRef(this);
        }
    }

    sptr(T *other) : refs_(other) {
        if (refs_ != nullptr) {
            refs_->IncStrongRef(this);
        }
    }

    sptr(const sptr<T> &other) : refs_(other.GetRefPtr()) {
        if (refs_ != nullptr) {
            refs_->IncStrongRef(this);
        }
    }

    sptr(sptr<T> &&other) : refs_(other.GetRefPtr()) {
        other.ForceSetRefPtr(nullptr);
    }

    sptr<T> &operator=(sptr<T> &&other) {
        if (refs_ != nullptr) {
            refs_->DecStrongRef(this);
        }
        refs_ = other.GetRefPtr();
        other.ForceSetRefPtr(nullptr);
        return *this;
    }

    template<typename O> sptr(const sptr<O> &other) : refs_(other.GetRefPtr()) {
        if (refs_ != nullptr) {
            refs_->IncStrongRef(this);
        }
    }

    inline sptr(WeakRefCounter *p, bool force) {
        if ((p != nullptr) && p->AttemptIncStrongRef(this)) {
            refs_ = reinterpret_cast<T *>(p->GetRefPtr());
        } else {
            refs_ = nullptr;
        }
    }

    inline T *GetRefPtr() const { return refs_; }
    inline void ForceSetRefPtr(T *other) { refs_ = other; }
    void clear() {
        if (refs_) {
            refs_->DecStrongRef(this);
            refs_ = nullptr;
        }
    }

    inline operator T *() const { return refs_; }
    inline T &operator*() const { return *refs_; }
    inline T *operator->() const { return refs_; }
    inline bool operator!() const { return refs_ == nullptr; }

    sptr<T> &operator=(T *other) {
        if (other != nullptr) {
            other->IncStrongRef(this);
        }
        if (refs_ != nullptr) {
            refs_->DecStrongRef(this);
        }
        refs_ = other;
        return *this;
    }

    sptr<T> &operator=(const sptr<T> &other) {
        T *otherRef = other.GetRefPtr();
        if (otherRef != nullptr) {
            otherRef->IncStrongRef(this);
        }
        if (refs_ != nullptr) {
            refs_->DecStrongRef(this);
        }
        refs_ = otherRef;
        return *this;
    }

    sptr<T> &operator=(const wptr<T> &other) {
        if ((other != nullptr) && other.AttemptIncStrongRef(this)) {
            refs_ = other.GetRefPtr();
        } else {
            refs_ = nullptr;
        }
        return *this;
    }

    template<typename O> sptr<T> &operator=(const sptr<O> &other) {
        T *otherRef = other.GetRefPtr();
        if (otherRef != nullptr) {
            otherRef->IncStrongRef(this);
        }
        if (refs_ != nullptr) {
            refs_->DecStrongRef(this);
        }
        refs_ = otherRef;
        return *this;
    }

    bool operator==(const T *other) const { return other == refs_; }
    inline bool operator!=(const T *other) const { return !operator==(other); }
    bool operator==(const wptr<T> &other) const { return refs_ == other.GetRefPtr(); }
    inline bool operator!=(const wptr<T> &other) const { return !operator==(other); }
    bool operator==(const sptr<T> &other) const { return refs_ == other.GetRefPtr(); }
    inline bool operator!=(const sptr<T> &other) const { return !operator==(other); }

private:
    T *refs_ = nullptr;
};

template<typename T> class wptr {
    template<typename O> friend class wptr;

public:
    wptr() : refs_(nullptr) {}
    wptr(T *other) {
        if (other != nullptr) {
            refs_ = other->CreateWeakRef(other);
            if (refs_ != nullptr) {
                refs_->IncWeakRefCount(this);
            }
        } else {
            refs_ = nullptr;
        }
    }

    wptr(const wptr<T> &other) : refs_(other.refs_) {
        if (refs_ != nullptr) {
            refs_->IncWeakRefCount(this);
        }
    }

    wptr(const sptr<T> &other) {
        if (other.GetRefPtr() != nullptr) {
            refs_ = other->CreateWeakRef(other.GetRefPtr());
            if (refs_ != nullptr) {
                refs_->IncWeakRefCount(this);
            }
        }
    }

    template<typename O> wptr(const wptr<O> &other) : refs_(other.refs_) {
        if (refs_ != nullptr) {
            refs_->IncWeakRefCount(this);
        }
    }

    template<typename O> wptr(const sptr<O> &other) {
        if (other.GetRefPtr() != nullptr) {
            refs_ = other->CreateWeakRef(other.GetRefPtr());
            if (refs_ != nullptr) {
                refs_->IncWeakRefCount(this);
            }
        }
    }

    wptr<T> &operator=(T *other) {
        WeakRefCounter *newWeakRef = nullptr;
        if (other != nullptr) {
            newWeakRef = other->CreateWeakRef(other);
            if (newWeakRef != nullptr) {
                newWeakRef->IncWeakRefCount(this);
            }
        }
        if (refs_ != nullptr) {
            refs_->DecWeakRefCount(this);
        }
        refs_ = newWeakRef;
        return *this;
    }

    template<typename O> wptr<T> &operator=(O *other) {
        T *object = reinterpret_cast<T *>(other);
        WeakRefCounter *newWeakRef = nullptr;
        if (object != nullptr) {
            newWeakRef = object->CreateWeakRef(object);
            if (newWeakRef != nullptr) {
                newWeakRef->IncWeakRefCount(this);
            }
        }
        if (refs_ != nullptr) {
            refs_->DecWeakRefCount(this);
        }
        refs_ = newWeakRef;
        return *this;
    }

    wptr<T> &operator=(const wptr<T> &other) {
        if (other.refs_ != nullptr) {
            other.refs_->IncWeakRefCount(this);
        }
        if (refs_ != nullptr) {
            refs_->DecWeakRefCount(this);
        }
        refs_ = other.refs_;
        return *this;
    }

    wptr<T> &operator=(const sptr<T> &other) {
        WeakRefCounter *newWeakRef = nullptr;
        if (other.GetRefPtr() != nullptr) {
            newWeakRef = other->CreateWeakRef(other.GetRefPtr());
            if (newWeakRef != nullptr) {
                newWeakRef->IncWeakRefCount(this);
            }
        }
        if (refs_ != nullptr) {
            refs_->DecWeakRefCount(this);
        }
        refs_ = newWeakRef;
        return *this;
    }

    template<typename O> wptr<T> &operator=(const wptr<O> &other) {
        if (other.refs_ != nullptr) {
            other.refs_->IncWeakRefCount(this);
        }
        if (refs_ != nullptr) {
            refs_->DecWeakRefCount(this);
        }
        refs_ = other.refs_;
        return *this;
    }

    template<typename O> wptr<T> &operator=(const sptr<O> &other) {
        WeakRefCounter *newWeakRef = nullptr;
        if (other.GetRefPtr() != nullptr) {
            newWeakRef = other->CreateWeakRef(other->GetRefPtr());
            if (newWeakRef != nullptr) {
                newWeakRef->IncWeakRefCount(this);
            }
        }
        if (refs_ != nullptr) {
            refs_->DecWeakRefCount(this);
        }
        refs_ = newWeakRef;
        return *this;
    }

    inline T *operator*() const { return GetRefPtr(); }
    inline T *operator->() const { return reinterpret_cast<T *>(refs_->GetRefPtr()); }

    bool operator==(const T *other) const { return GetRefPtr() == other; }
    inline bool operator!=(const T *other) const { return !operator==(other); }
    bool operator==(const wptr<T> &other) const { return GetRefPtr() == other.GetRefPtr(); }
    inline bool operator!=(const wptr<T> &other) const { return !operator==(other); }
    bool operator==(const sptr<T> &other) const { return GetRefPtr() == other.GetRefPtr(); }
    inline bool operator!=(const sptr<T> &other) const { return !operator==(other); }

    T *GetRefPtr() const {
        return (refs_ != nullptr) ? reinterpret_cast<T *>(refs_->GetRefPtr()) : nullptr;
    }

    inline bool AttemptIncStrongRef(const void *objectId) const {
        return refs_->AttemptIncStrongRef(objectId);
    }

    const sptr<T> promote() const { return sptr<T>(refs_, true); }

    ~wptr() {
        if (refs_ != nullptr) {
            refs_->DecWeakRefCount(this);
        }
    }

private:
    WeakRefCounter *refs_ = nullptr;
};

}

#endif