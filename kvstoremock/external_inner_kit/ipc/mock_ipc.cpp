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
#include <ipc_skeleton.h>
#include <iremote_broker.h>
#include <message_option.h>
#include <message_parcel.h>
#include <peer_holder.h>
#include <securec.h>
#include <sys/unistd.h>
#include <unordered_set>
#include <optional>
#include <queue>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace OHOS {
BrokerRegistration &BrokerRegistration::Get()
{
    static BrokerRegistration instance;
    return instance;
}

BrokerRegistration::~BrokerRegistration()
{
    isUnloading = true;
    std::lock_guard<std::mutex> lockGuard(creatorMutex_);
    for (auto it = creators_.begin(); it != creators_.end();) {
        it = creators_.erase(it);
    }
    for (auto it1 = objects_.begin(); it1 != objects_.end();) {
        BrokerDelegatorBase *object = reinterpret_cast<BrokerDelegatorBase *>(it1->first);
        object->isSoUnloaded = true;
        it1 = objects_.erase(it1);
    }
}

bool BrokerRegistration::Register(const std::u16string &descriptor, const Constructor &creator,
    const BrokerDelegatorBase *object)
{
    if (descriptor.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lockGuard(creatorMutex_);
    auto it = creators_.find(descriptor);
    bool ret = false;
    if (it == creators_.end()) {
        ret = creators_.insert({ descriptor, creator }).second;
    }
    auto it1 = std::find_if(objects_.begin(), objects_.end(),
        [descriptor](const std::pair<uintptr_t, std::string> &item) {
        const BrokerDelegatorBase *obj = reinterpret_cast<BrokerDelegatorBase *>(item.first);
        return obj->descriptor_ == descriptor;
    });
    if (it1 == objects_.end()) {
        objects_.insert({ reinterpret_cast<uintptr_t>(object), "" });
    }
    return ret;
}

void BrokerRegistration::Unregister(const std::u16string &descriptor)
{
    if (isUnloading) {
        return;
    }
    std::lock_guard<std::mutex> lockGuard(creatorMutex_);
    if (!descriptor.empty()) {
        auto it = creators_.find(descriptor);
        if (it != creators_.end()) {
            creators_.erase(it);
        }
        auto it1 = std::find_if(objects_.begin(), objects_.end(),
            [descriptor](const std::pair<uintptr_t, std::string> &item) {
            const BrokerDelegatorBase *obj = reinterpret_cast<BrokerDelegatorBase *>(item.first);
            return obj->descriptor_ == descriptor;
        });
        if (it1 != objects_.end()) {
            objects_.erase(it1);
        }
    }
}

sptr<IRemoteBroker> BrokerRegistration::NewInstance(const std::u16string &descriptor, const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lockGuard(creatorMutex_);

    sptr<IRemoteBroker> broker;
    if (object != nullptr) {
        auto it = creators_.find(descriptor);
        if (it != creators_.end()) {
            broker = it->second(object);
        }
    }
    return broker;
}

PeerHolder::PeerHolder(const sptr<IRemoteObject> &object)
    : remoteObject_(object) {};
sptr<IRemoteObject> PeerHolder::Remote() {
    return remoteObject_;
}
pid_t IPCSkeleton::GetCallingUid() { return 1000; }
pid_t IPCSkeleton::GetCallingPid() { return getpid(); }
pid_t IPCSkeleton::GetCallingRealPid() { return getpid(); }
std::string IPCSkeleton::GetCallingSid() { return ""; }

uint32_t IPCSkeleton::GetCallingTokenID()
{
    OHOS::Security::AccessToken::AccessTokenIDEx fullToken;
    fullToken.tokenIDEx = GetSelfTokenID();
    if (fullToken.tokenIDEx == 0) {
        NativeTokenInfoParams params{ 0 };
        params.processName = "distributed_test";
        params.aplStr = "distributed_test";
        return GetAccessTokenId(&params);
    }
    return fullToken.tokenIdExStruct.tokenID;
}

uint64_t IPCSkeleton::GetCallingFullTokenID()
{
    return ::GetSelfTokenID();
}

uint64_t IPCSkeleton::GetSelfTokenID()
{
    return ::GetSelfTokenID();
}

std::string IPCSkeleton::ResetCallingIdentity()
{
    return "";
}
bool IPCSkeleton::SetCallingIdentity(std::string &identity, bool flag)
{
    return true;
}
bool IPCSkeleton::TriggerSystemIPCThreadReclaim()
{
    return false;
}
bool IPCSkeleton::EnableIPCThreadReclaim(bool enable)
{
    return false;
}
int32_t IPCSkeleton::GetThreadInvocationState()
{
    return 0;
}
uint32_t IPCSkeleton::GetDCallingTokenID()
{
    return 0;
}
int32_t IPCSkeleton::GetMemoryUsage(uint32_t pid, unsigned long &totalSize, unsigned long &oneWayFreeSize)
{
    return 0;
}
pid_t IPCSkeleton::GetDCallingUid()
{
    return -1;
}
std::optional<bool> IPCSkeleton::HasSoUnreleasedRemoteObject(const std::unordered_set<std::string> &targets)
{
    return std::nullopt;
}
void IPCDfx::BlockUntilThreadAvailable() {}
bool IPCDfx::SetIPCProxyLimit(uint64_t num, IPCProxyLimitCallback callback)
{
    return false;
}
bool IPCSkeleton::SetMaxWorkThreadNum(int maxThreadNum)
{
    return false;
}
void IPCSkeleton::JoinWorkThread() {}
void IPCSkeleton::StopWorkThread() {}
uint32_t IPCSkeleton::GetFirstTokenID()
{
    return 0;
}
uint64_t IPCSkeleton::GetFirstFullTokenID()
{
    return 0;
}
std::string IPCSkeleton::GetLocalDeviceID()
{
    return std::string();
}
std::string IPCSkeleton::GetCallingDeviceID()
{
    return std::string();
}
bool IPCSkeleton::IsLocalCalling()
{
    return false;
}
IPCSkeleton &IPCSkeleton::GetInstance()
{
    static IPCSkeleton instance;
    return instance;
}
sptr<IRemoteObject> IPCSkeleton::GetContextObject()
{
    return sptr<IRemoteObject>();
}
bool IPCSkeleton::SetContextObject(sptr<IRemoteObject> &object)
{
    return false;
}
int IPCSkeleton::FlushCommands(IRemoteObject *object)
{
    return 0;
}

static size_t g_cursor = 0;
static std::queue<sptr<IRemoteObject>> g_remoteObjects;
static std::u16string g_token;

MessageOption::MessageOption(int flags, int waitTime) : flags_(flags), waitTime_(waitTime) {}
void MessageOption::SetFlags(int flags) { flags_ = flags; }
int MessageOption::GetFlags() const { return flags_; }
void MessageOption::SetWaitTime(int waitTime) { waitTime_ = waitTime; }
int MessageOption::GetWaitTime() const { return waitTime_; }
MessageParcel::MessageParcel() : rawDataSize_(0) {}
MessageParcel::~MessageParcel() {}
MessageParcel::MessageParcel(Allocator *allocator)
    : Parcel(allocator) {}
bool MessageParcel::WriteRemoteObject(const sptr<IRemoteObject> &object)
{
    g_remoteObjects.push(object);
    return true;
}
sptr<IRemoteObject> MessageParcel::ReadRemoteObject()
{
    if (g_remoteObjects.empty()) {
        return nullptr;
    }
    auto object = g_remoteObjects.front();
    g_remoteObjects.pop();
    return object;
}
bool MessageParcel::WriteFileDescriptor(int fd) { return false; }
int MessageParcel::ReadFileDescriptor() { return 0; }
bool MessageParcel::ContainFileDescriptors() const { return false; }
bool MessageParcel::WriteInterfaceToken(std::u16string name)
{
    g_token = name;
    return true;
}
std::u16string MessageParcel::ReadInterfaceToken()
{
    return g_token;
}
bool MessageParcel::WriteRawData(const void *data, size_t size)
{
    std::shared_ptr<char> rawData(new char[rawDataSize_ + size]);
    memcpy_s(&(rawData.get())[rawDataSize_], size, data, size);
    if (rawData_ != nullptr) {
        memcpy_s(rawData.get(), rawDataSize_, rawData_.get(), rawDataSize_);
    }
    rawData_ = rawData;
    rawDataSize_ += size;
    return true;
}
const void *MessageParcel::ReadRawData(size_t size)
{
    if (rawDataSize_ < size) {
        return nullptr;
    }
    rawDataSize_ -= size;
    void *ptr = &(rawData_.get())[g_cursor];
    g_cursor += size;
    return ptr;
}

bool MessageParcel::RestoreRawData(std::shared_ptr<char> rawData, size_t size)
{
    rawData_ = rawData;
    rawDataSize_ = size;
    return true;
}
const void *MessageParcel::GetRawData() const { return rawData_.get(); }
size_t MessageParcel::GetRawDataSize() const { return rawDataSize_; }
size_t MessageParcel::GetRawDataCapacity() const { return MAX_RAWDATA_SIZE; }
void MessageParcel::WriteNoException() {}
int32_t MessageParcel::ReadException() { return 0; }
bool MessageParcel::WriteAshmem(sptr<Ashmem> ashmem) { return false; }
sptr<Ashmem> MessageParcel::ReadAshmem() { return sptr<Ashmem>(); }
void MessageParcel::ClearFileDescriptor() {}
bool MessageParcel::Append(MessageParcel &data) { return false; }
bool MessageParcel::WriteDBinderProxy(const sptr<IRemoteObject> &object, uint32_t handle, uint64_t stubIndex) { return false;}
}
