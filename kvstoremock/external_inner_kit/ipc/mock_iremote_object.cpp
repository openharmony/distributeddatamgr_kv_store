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

#include "iremote_object.h"
#include "iremote_stub.h"
#include "iremote_proxy.h"
#include "napi_remote_object.h"
namespace OHOS {
bool IRemoteObject::IsProxyObject() const { return true;}
bool IRemoteObject::IsObjectDead() const { return false; }
bool IRemoteObject::CheckObjectLegality() const
{
    return false;
}
bool IRemoteObject::Marshalling(Parcel &parcel) const
{
    return false;
}
sptr<IRemoteObject> IRemoteObject::Unmarshalling(Parcel &parcel)
{
    return nullptr;
}
bool IRemoteObject::Marshalling(Parcel &parcel, const sptr<IRemoteObject> &object)
{
    return false;
}
sptr<IRemoteBroker> IRemoteObject::AsInterface()
{
    return sptr<IRemoteBroker>();
}
std::u16string IRemoteObject::GetObjectDescriptor() const
{
    return descriptor_;
}
std::u16string IRemoteObject::GetInterfaceDescriptor()
{
    return descriptor_;
}
IRemoteObject::IRemoteObject(std::u16string descriptor)
 : descriptor_(descriptor)
{


}

}
