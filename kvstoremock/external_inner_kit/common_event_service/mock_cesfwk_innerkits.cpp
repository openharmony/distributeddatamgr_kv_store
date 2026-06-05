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
#include <common_event_data.h>
#include <common_event_manager.h>
#include <common_event_publish_info.h>
#include <common_event_subscribe_info.h>
#include <common_event_subscriber.h>
#include <common_event_support.h>
#include <int_wrapper.h>
#include <matching_skills.h>
#include <string_wrapper.h>
#include <uri.h>
#include <want.h>

#include "want_params.h"
namespace OHOS {
namespace AAFwk {
void WantParams::SetParam(std::string const &, OHOS::AAFwk::IInterface *) {}
WantParams::WantParams(OHOS::AAFwk::WantParams const &) {}
bool WantParams::Marshalling(OHOS::Parcel &parcel) const
{
    return true;
}

const std::map<std::string, sptr<IInterface>> &WantParams::GetParams() const { return params_; }
WantParams &WantParams::operator=(const WantParams &other)
{
    params_ = other.params_;
    return *this;
}
bool WantParams::operator==(const WantParams &other)
{
    return false;
}
sptr<IInterface> WantParams::GetInterfaceByType(int typeId, const std::string &value)
{
    return sptr<IInterface>();
}
bool WantParams::CompareInterface(const sptr<IInterface> iIt1, const sptr<IInterface> iIt2, int typeId)
{
    return false;
}
int WantParams::GetDataType(const sptr<IInterface> iIt)
{
    return 0;
}
std::string WantParams::GetStringByType(const sptr<IInterface> iIt, int typeId)
{
    return std::string();
}
sptr<IInterface> WantParams::GetParam(const std::string &key) const
{
    return sptr<IInterface>();
}
WantParams WantParams::GetWantParams(const std::string &key) const
{
    return WantParams();
}
std::string WantParams::GetStringParam(const std::string &key) const
{
    return std::string();
}
int WantParams::GetIntParam(const std::string &key, const int defaultValue) const
{
    return 0;
}
const std::set<std::string> WantParams::KeySet() const
{
    return std::set<std::string>();
}
void WantParams::Remove(const std::string &key) {}
bool WantParams::HasParam(const std::string &key) const
{
    return false;
}
int WantParams::Size() const
{
    return 0;
}
bool WantParams::IsEmpty() const
{
    return false;
}
WantParams *WantParams::Unmarshalling(Parcel &parcel, int depth)
{
    (void)depth;
    return nullptr;
}
void WantParams::DumpInfo(int level) const {}

Want::Want() {}
Want::Want(OHOS::AAFwk::Want const &ref) { (void)ref; }
Want::~Want() {}
bool Want::Marshalling(OHOS::Parcel &parcel) const
{
    (void)parcel;
    return true;
}
std::string Want::GetAction() const { return ""; }
Want &Want::SetAction(std::string const &) { return *this; }
Want &Want::SetParams(OHOS::AAFwk::WantParams const &) { return *this; }
int Want::GetIntParam(std::string const &key, int value) const
{
    (void)key;
    return value;
}
bool Want::GetBoolParam(std::string const &, bool) const { return false; }
Uri Want::GetUri() const { return Uri(""); }
Want &Want::SetUri(const std::string &uri) {return *this;}
const WantParams &Want::GetParams() const { return parameters_; }
OHOS::AppExecFwk::ElementName Want::GetElement() const { return OHOS::AppExecFwk::ElementName(); }
Want &Want::SetUri(const Uri &uri) { return *this; }
Want &Want::SetUriAndType(const Uri &uri, const std::string &type) { return *this; }
std::string Want::WantToUri(Want &want) { return std::string(); }
std::string Want::ToUri() const { return std::string();}
Want &Want::FormatUri(const std::string &uri) { return *this; }
Want &Want::FormatUri(const Uri &uri) { return *this; }
std::string Want::GetBundle() const { return std::string();}
Want &Want::SetBundle(const std::string &bundleName) { return *this;}
const std::vector<std::string> &Want::GetEntities() const { static std::vector<std::string> vector; return vector; }
Want &Want::AddEntity(const std::string &entity) { return *this;}
void Want::RemoveEntity(const std::string &entity) {}
bool Want::HasEntity(const std::string &key) const { return false; }
int Want::CountEntities() { return 0; }
const std::string Want::GetScheme() const { return std::string(); }
std::string Want::GetType() const { return std::string(); }
Want &Want::SetType(const std::string &type) { return *this; }
Want &Want::FormatType(const std::string &type) { return *this; }
Want &Want::FormatUriAndType(const Uri &uri, const std::string &type) { return *this; }
std::string Want::FormatMimeType(const std::string &mimeType) { return std::string(); }
void Want::ClearWant(Want *want) {}
std::vector<bool> Want::GetBoolArrayParam(const std::string &key) const { return std::vector<bool>(); }
Want &Want::SetParam(const std::string &key, bool value) { return *this; }
Want &Want::SetParam(const std::string &key, const std::vector<bool> &value) { return *this;}
byte Want::GetByteParam(const std::string &key, byte defaultValue) const { return 0;}
std::vector<byte> Want::GetByteArrayParam(const std::string &key) const { return std::vector<byte>();}
Want &Want::SetParam(const std::string &key, byte value) { return *this;}
Want &Want::SetParam(const std::string &key, const std::vector<byte> &value) { return *this;}
zchar Want::GetCharParam(const std::string &key, zchar defaultValue) const { return 0; }
std::vector<zchar> Want::GetCharArrayParam(const std::string &key) const { return std::vector<zchar>();}
Want &Want::SetParam(const std::string &key, zchar value) { return *this;}
Want &Want::SetParam(const std::string &key, const std::vector<zchar> &value) { return *this;}
std::vector<int> Want::GetIntArrayParam(const std::string &key) const { return std::vector<int>();}
Want &Want::SetParam(const std::string &key, int value)
{
    (void)key;
    (void)value;
    return *this;
}
Want &Want::SetParam(const std::string &key, const std::vector<int> &value) { return *this;}
double Want::GetDoubleParam(const std::string &key, double defaultValue) const { return 0; }
std::vector<double> Want::GetDoubleArrayParam(const std::string &key) const { return std::vector<double>();}
Want &Want::SetParam(const std::string &key, double value) { return *this;}
Want &Want::SetParam(const std::string &key, const std::vector<double> &value) { return *this;}
float Want::GetFloatParam(const std::string &key, float defaultValue) const { return 0;}
std::vector<float> Want::GetFloatArrayParam(const std::string &key) const { return std::vector<float>();}
Want &Want::SetParam(const std::string &key, float value) { return *this;}
Want &Want::SetParam(const std::string &key, const std::vector<float> &value) { return *this;}
long Want::GetLongParam(const std::string &key, long defaultValue) const { return 0;}
std::vector<long> Want::GetLongArrayParam(const std::string &key) const { return std::vector<long>();}
Want &Want::SetParam(const std::string &key, long long int value) { return *this;}
Want &Want::SetParam(const std::string &key, long value) { return *this;}
Want &Want::SetParam(const std::string &key, const std::vector<long> &value) { return *this;}
short Want::GetShortParam(const std::string &key, short defaultValue) const { return 0;}
std::vector<short> Want::GetShortArrayParam(const std::string &key) const { return std::vector<short>();}
Want &Want::SetParam(const std::string &key, short value) { return *this;}
Want &Want::SetParam(const std::string &key, const std::vector<short> &value) { return *this;}
std::string Want::GetStringParam(const std::string &key) const { return std::string();}
std::vector<std::string> Want::GetStringArrayParam(const std::string &key) const { return std::vector<std::string>();}
Want &Want::SetParam(const std::string &key, const std::string &value) { return *this;}
Want &Want::SetParam(const std::string &key, const std::vector<std::string> &value) { return *this;}
bool Want::HasParameter(const std::string &key) const { return false;}
Want *Want::ReplaceParams(WantParams &wantParams) { return this;}
Want *Want::ReplaceParams(Want &want) { return this; }
void Want::RemoveParam(const std::string &key) {}
Operation Want::GetOperation() const { return Operation();}
void Want::SetOperation(const Operation &operation) {}
bool Want::OperationEquals(const Want &want) { return false;}
Want *Want::CloneOperation() { return this; }
Want *Want::Unmarshalling(Parcel &parcel)
{
    (void)parcel;
    return new Want;
}
void Want::DumpInfo(int level) const {}
std::string Want::ToString() const { return std::string();}
Want *Want::FromString(std::string &string) { return new Want;}
Want &Want::SetDeviceId(const std::string &deviceId) { return *this;}
std::string Want::GetDeviceId() const { return std::string();}
Want &Want::SetModuleName(const std::string &moduleName) { return *this;}
std::string Want::GetModuleName() const { return std::string();}
bool Want::ParseFlag(const std::string &content, Want &want) { return false;}
std::string Want::Decode(const std::string &str) { return std::string();}
std::string Want::Encode(const std::string &str) { return std::string();}
bool Want::ParseContent(const std::string &content, std::string &prop, std::string &value) { return false;}
bool Want::ParseUriInternal(const std::string &content, AppExecFwk::ElementName &element, Want &want) { return false;}
bool Want::ReadFromParcel(Parcel &parcel) { return false;}
bool Want::CheckAndSetParameters(Want &want, const std::string &key, std::string &prop, const std::string &value)
{
    return false;
}
Uri Want::GetLowerCaseScheme(const Uri &uri) { return Uri("");}
void Want::ToUriStringInner(std::string &uriString) const{ }
void Want::UriStringAppendParam(std::string &uriString) const {}
Want &Want::operator=(const Want &input)
{
    (void)input;
    return *this;
}

Want &Want::SetFlags(unsigned int flags)
{
    return *this;
}
unsigned int Want::GetFlags() const
{
    return 0;
}
Want &Want::AddFlags(unsigned int flags)
{
    return *this;
}
void Want::RemoveFlags(unsigned int flag) {}
Want &Want::SetElementName(const std::string &bundleName, const std::string &abilityName)
{
    return *this;
}
Want &Want::SetElementName(const std::string &deviceId, const std::string &bundleName, const std::string &abilityName,
    const std::string &moduleName)
{
    return *this;
}
Want &Want::SetElement(const AppExecFwk::ElementName &element)
{
    return *this;
}
Want *Want::MakeMainAbility(const AppExecFwk::ElementName &elementName)
{
    return nullptr;
}
Want *Want::WantParseUri(const char *uri)
{
    return nullptr;
}
Want *Want::ParseUri(const std::string &uri)
{
    return nullptr;
}
std::string Want::GetUriString() const
{
    return std::string();
}
UnsupportedData::~UnsupportedData() {}
Operation::Operation() : uri_("") {}
Operation::~Operation() {}
bool Operation::Marshalling(OHOS::Parcel &) const { return false; }
sptr<IString> String::Box(const std::string &str) { return nullptr; }
}

namespace EventFwk {
MatchingSkills::MatchingSkills() {};
MatchingSkills::~MatchingSkills() {};
bool MatchingSkills::Marshalling(OHOS::Parcel &) const { return true; }
void MatchingSkills::AddEvent(std::string const &) {}
void MatchingSkills::AddScheme(const std::string &scheme) {}
const Want &CommonEventData::GetWant() const { return want_; }
CommonEventData::CommonEventData(OHOS::AAFwk::Want const &) {}
CommonEventData::~CommonEventData() {}
CommonEventSubscribeInfo::CommonEventSubscribeInfo(OHOS::EventFwk::MatchingSkills const &) {}
CommonEventSubscribeInfo::CommonEventSubscribeInfo() {}
CommonEventSubscribeInfo::~CommonEventSubscribeInfo() {}
CommonEventSubscriber::CommonEventSubscriber() {}
CommonEventSubscriber::CommonEventSubscriber(const CommonEventSubscribeInfo &subscribeInfo) {}
CommonEventSubscriber::~CommonEventSubscriber() {}
const std::string CommonEventSupport::COMMON_EVENT_USER_REMOVED = "COMMON_EVENT_USER_REMOVED";
const std::string CommonEventSupport::COMMON_EVENT_USER_STARTING = "COMMON_EVENT_USER_STARTING";
const std::string CommonEventSupport::COMMON_EVENT_USER_UNLOCKED = "COMMON_EVENT_USER_UNLOCKED";
const std::string CommonEventSupport::COMMON_EVENT_USER_STOPPING = "COMMON_EVENT_USER_STOPPING";
const std::string CommonEventSupport::COMMON_EVENT_USER_STOPPED = "COMMON_EVENT_USER_STOPPED";
const std::string CommonEventSupport::COMMON_EVENT_HWID_TOKEN_INVALID = "COMMON_EVENT_HWID_TOKEN_INVALID";
const std::string CommonEventSupport::COMMON_EVENT_HWID_LOGOUT = "COMMON_EVENT_HWID_LOGOUT";
const std::string CommonEventSupport::COMMON_EVENT_HWID_LOGIN = "COMMON_EVENT_HWID_LOGIN";
const std::string CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED = "COMMON_EVENT_PACKAGE_ADDED";
const std::string CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED = "COMMON_EVENT_PACKAGE_REMOVED";
const std::string CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED = "COMMON_EVENT_PACKAGE_CHANGED";
const std::string CommonEventSupport::COMMON_EVENT_USER_STARTED = "COMMON_EVENT_USER_STARTED";
const std::string CommonEventSupport::COMMON_EVENT_USER_SWITCHED = "COMMON_EVENT_USER_SWITCHED";
const std::string CommonEventSupport::COMMON_EVENT_UID_REMOVED = "COMMON_EVENT_UID_REMOVED";
const std::string CommonEventSupport::COMMON_EVENT_LOCALE_CHANGED = "COMMON_EVENT_LOCALE_CHANGED";
const std::string CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED = "COMMON_EVENT_SANDBOX_PACKAGE_REMOVED";
const std::string CommonEventSupport::COMMON_EVENT_DRIVE_MODE = "COMMON_EVENT_DRIVE_MODE";
const std::string CommonEventSupport::COMMON_EVENT_HOME_MODE = "COMMON_EVENT_HOME_MODE";
const std::string CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED = "COMMON_EVENT_BOOT_COMPLETED";
const std::string CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED = "COMMON_EVENT_BATTERY_CHANGED";
const std::string CommonEventSupport::COMMON_EVENT_POWER_CONNECTED = "COMMON_EVENT_POWER_CONNECTED";
const std::string CommonEventSupport::COMMON_EVENT_TIME_TICK = "COMMON_EVENT_TIME_TICK";
const std::string CommonEventSupport::COMMON_EVENT_TIME_CHANGED = "COMMON_EVENT_TIME_CHANGED";
const std::string CommonEventSupport::COMMON_EVENT_DATE_CHANGED = "COMMON_EVENT_DATE_CHANGED";
const std::string CommonEventSupport::COMMON_EVENT_TIMEZONE_CHANGED = "COMMON_EVENT_TIMEZONE_CHANGED";
const std::string CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED = "COMMON_EVENT_SCREEN_UNLOCKED";
const std::string CommonEventSupport::COMMON_EVENT_BUNDLE_SCAN_FINISHED = "COMMON_EVENT_BUNDLE_SCAN_FINISHED";
bool CommonEventSubscribeInfo::Marshalling(OHOS::Parcel &) const { return true; }
CommonEventSubscribeInfo::CommonEventSubscribeInfo(const CommonEventSubscribeInfo &commonEventSubscribeInfo) {}
void CommonEventSubscribeInfo::SetPriority(const int32_t &priority) {}
int32_t CommonEventSubscribeInfo::GetPriority() const
{
    return 0;
}
void CommonEventSubscribeInfo::SetUserId(const int32_t &userId) {}
int32_t CommonEventSubscribeInfo::GetUserId() const
{
    return 0;
}
void CommonEventSubscribeInfo::SetPermission(const std::string &permission) {}
std::string CommonEventSubscribeInfo::GetPermission() const
{
    return std::string();
}
CommonEventSubscribeInfo::ThreadMode CommonEventSubscribeInfo::GetThreadMode() const
{
    return CommonEventSubscribeInfo::ASYNC;
}
void CommonEventSubscribeInfo::SetThreadMode(CommonEventSubscribeInfo::ThreadMode threadMode) {}
void CommonEventSubscribeInfo::SetDeviceId(const std::string &deviceId) {}
std::string CommonEventSubscribeInfo::GetDeviceId() const
{
    return std::string();
}
const MatchingSkills &CommonEventSubscribeInfo::GetMatchingSkills() const
{
    return matchingSkills_;
}
CommonEventSubscribeInfo *CommonEventSubscribeInfo::Unmarshalling(Parcel &parcel)
{
    return nullptr;
}
bool CommonEventSubscribeInfo::ReadFromParcel(Parcel &parcel)
{
    return false;
}
bool CommonEventManager::SubscribeCommonEvent(
    std::shared_ptr<OHOS::EventFwk::CommonEventSubscriber> const &) { return false; }
bool CommonEventManager::PublishCommonEvent(OHOS::EventFwk::CommonEventData const &) { return false; }
bool CommonEventManager::UnSubscribeCommonEvent(
    std::shared_ptr<OHOS::EventFwk::CommonEventSubscriber> const &) { return false; }
bool CommonEventManager::PublishCommonEvent(const CommonEventData &data, const CommonEventPublishInfo &publishInfo)
{
    return false;
}
bool CommonEventManager::PublishCommonEvent(const CommonEventData &data, const CommonEventPublishInfo &publishInfo,
    const std::shared_ptr<CommonEventSubscriber> &subscriber) { return false;}
bool CommonEventManager::GetStickyCommonEvent(const std::string &event, CommonEventData &commonEventData) {return false;}
bool CommonEventData::Marshalling(OHOS::Parcel &) const { return true; }
int CommonEventData::GetCode() const { return 0; }
CommonEventPublishInfo::CommonEventPublishInfo() {}
CommonEventPublishInfo::CommonEventPublishInfo(const CommonEventPublishInfo &commonEventPublishInfo) {}
CommonEventPublishInfo::~CommonEventPublishInfo() {}
bool CommonEventPublishInfo::Marshalling(Parcel &parcel) const { return false;}
void CommonEventPublishInfo::SetSticky(bool sticky) {}
bool CommonEventPublishInfo::IsSticky() const
{
    return false;
}
void CommonEventPublishInfo::SetSubscriberPermissions(const std::vector<std::string> &subscriberPermissions) {}
const std::vector<std::string> &CommonEventPublishInfo::GetSubscriberPermissions() const
{
    static std::vector<std::string> TMP;
    return TMP;
}
void CommonEventPublishInfo::SetOrdered(bool ordered) {}
bool CommonEventPublishInfo::IsOrdered() const
{
    return false;
}
void CommonEventPublishInfo::SetBundleName(const std::string &bundleName) {}
void CommonEventPublishInfo::SetSubscriberUid(const std::vector<int32_t> &subscriberUids) {}
std::vector<int32_t> CommonEventPublishInfo::GetSubscriberUid() const
{
    return std::vector<int32_t>();
}
void CommonEventPublishInfo::SetSubscriberType(const int32_t &subscriberType) {}
int32_t CommonEventPublishInfo::GetSubscriberType() const
{
    return 0;
}
std::string CommonEventPublishInfo::GetBundleName() const
{
    return std::string();
}
}
}
