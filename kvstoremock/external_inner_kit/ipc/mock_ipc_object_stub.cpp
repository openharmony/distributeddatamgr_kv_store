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
#include "accesstoken_kit.h"
#include "ipc_object_stub.h"
#include "nativetoken_kit.h"
namespace OHOS {
IPCObjectStub::~IPCObjectStub() {}
IPCObjectStub::IPCObjectStub(std::u16string descriptor, bool serialInvokeFlag)
    : IRemoteObject(descriptor), serialInvokeFlag_(serialInvokeFlag) {}
// IPCObjectStub::IPCObjectStub() : IRemoteObject(), Parcelable(), RefBase() {}
int32_t IPCObjectStub::GetObjectRefCount()
{
    return 0;
}
int IPCObjectStub::Dump(int fd, const std::vector<std::u16string> &args)
{
    return 0;
}
int IPCObjectStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return 0;
}
int IPCObjectStub::SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return OnRemoteRequest(code, data, reply, option);
}
void IPCObjectStub::OnFirstStrongRef(const void *objectId)
{
    RefBase::OnFirstStrongRef(objectId);
}
void IPCObjectStub::OnLastStrongRef(const void *objectId)
{
    RefBase::OnLastStrongRef(objectId);
}
bool IPCObjectStub::AddDeathRecipient(const sptr<DeathRecipient> &recipient)
{
    return false;
}
bool IPCObjectStub::RemoveDeathRecipient(const sptr<DeathRecipient> &recipient)
{
    return false;
}
int IPCObjectStub::GetCallingPid()
{
    return 0;
}
int IPCObjectStub::GetCallingUid()
{
    return 0;
}
int IPCObjectStub::OnRemoteDump(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return 0;
}
int32_t IPCObjectStub::ProcessProto(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return 0;
}
int IPCObjectStub::GetObjectType() const
{
    return 0;
}
int32_t IPCObjectStub::InvokerThread(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return 0;
}
int32_t IPCObjectStub::NoticeServiceDie(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return 0;
}
int32_t IPCObjectStub::InvokerDataBusThread(MessageParcel &data, MessageParcel &reply)
{
    return 0;
}
int32_t IPCObjectStub::AddAuthInfo(MessageParcel &data, MessageParcel &reply, uint32_t code)
{
    return 0;
}
bool IPCObjectStub::IsDeviceIdIllegal(const std::string &deviceID)
{
    return false;
}
uint32_t IPCObjectStub::GetCallingTokenID()
{
    NativeTokenInfoParams params{0};
    params.processName = "distributed_test";
    params.aplStr = "distributed_test";
    return GetAccessTokenId(&params);
}
uint32_t IPCObjectStub::GetFirstTokenID()
{
    NativeTokenInfoParams params{0};
    params.processName = "distributed_test";
    params.aplStr = "distributed_test";
    return GetAccessTokenId(&params);
}
uint64_t IPCObjectStub::GetCallingFullTokenID()
{
    return 0;
}
uint64_t IPCObjectStub::GetFirstFullTokenID()
{
    return 0;
}
uint64_t IPCObjectStub::GetLastRequestTime()
{
    return 0;
}
bool IPCObjectStub::GetRequestSidFlag() const
{
    return false;
}
void IPCObjectStub::SetRequestSidFlag(bool flag)
{
}
int IPCObjectStub::GetAndSaveDBinderData(pid_t pid, uid_t uid)
{
    return 0;
}
bool IPCObjectStub::AddRefreshRecipient(const sptr<RefreshRecipient> &recipient)
{
    return false;
}
bool IPCObjectStub::RemoveRefreshRecipient(const sptr<RefreshRecipient> &recipient)
{
    return false;
}
#ifndef CONFIG_IPC_SINGLE
int IPCObjectStub::DBinderClearServiceState(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    return 0;
}
#endif
}
