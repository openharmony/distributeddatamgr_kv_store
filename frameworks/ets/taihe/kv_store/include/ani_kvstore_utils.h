/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_ANI_KVSTORE_UTILS_H
#define OHOS_ANI_KVSTORE_UTILS_H
#include "taihe/runtime.hpp"
#include "ohos.data.distributedkvstore.proj.hpp"
#include "ohos.data.distributedkvstore.impl.hpp"
#include <string>
#include <optional>
#include <variant>

#include "event_handler.h"
#include "event_runner.h"
#include "blob.h"
#include "datashare_values_bucket.h"
#include "types.h"
#include "kvstore_observer.h"
#include "kvstore_sync_callback.h"
#include "kvstore_death_recipient.h"

static constexpr const char* SCHEMA_VERSION = "SCHEMA_VERSION";
static constexpr const char* SCHEMA_MODE = "SCHEMA_MODE";
static constexpr const char* SCHEMA_DEFINE = "SCHEMA_DEFINE";
static constexpr const char* SCHEMA_INDEXES = "SCHEMA_INDEXES";
static constexpr const char* SCHEMA_SKIPSIZE = "SCHEMA_SKIPSIZE";
static constexpr const char* DEFAULT_SCHEMA_VERSION = "1.0";
static constexpr const char* SCHEMA_STRICT = "STRICT";
static constexpr const char* SCHEMA_COMPATIBLE = "COMPATIBLE";

static constexpr const char* FIELD_NAME = "FIELD_NAME";
static constexpr const char* VALUE_TYPE = "VALUE_TYPE";
static constexpr const char* DEFAULT_VALUE = "DEFAULT_VALUE";
static constexpr const char* IS_DEFAULT_VALUE = "IS_DEFAULT_VALUE";
static constexpr const char* IS_NULLABLE = "IS_NULLABLE";
static constexpr const char* CHILDREN = "CHILDREN";
static constexpr const char* SPLIT = ",";
static constexpr const char* NOT_NULL = ", NOT NULL,";
static constexpr const char* DEFAULT = " DEFAULT ";
static constexpr const char* MARK = "'";

enum {
    SCHEMA_MODE_SLOPPY,
    SCHEMA_MODE_STRICT,
};

enum {
    JS_SECURITY_LEVEL_S1 = 0,
    JS_SECURITY_LEVEL_S2,
    JS_SECURITY_LEVEL_S3,
    JS_SECURITY_LEVEL_S4,
};

namespace ani_kvstoreutils {
using namespace OHOS;
using ValueVariant = std::variant<std::string, int32_t, float, std::vector<uint8_t>, bool, double, int64_t>;
using DataShareValueVariant = DataShare::DataShareValueObject::Type;

enum {
    /* Blob's first byte is the blob's data ValueType */
    STRING = 0,
    INTEGER = 1,
    FLOAT = 2,
    BYTE_ARRAY = 3,
    BOOLEAN = 4,
    DOUBLE = 5,
    LONG = 6,
    INVALID = 255
};

enum {
    /* exported js SubscribeType  is (DistributedKv::SubscribeType-1) */
    SUBSCRIBE_LOCAL = 0,        /* i.e. SubscribeType::SUBSCRIBE_TYPE_LOCAL-1  */
    SUBSCRIBE_REMOTE = 1,       /* i.e. SubscribeType::SUBSCRIBE_TYPE_REMOTE-1 */
    SUBSCRIBE_LOCAL_REMOTE = 2, /* i.e. SubscribeType::SUBSCRIBE_TYPE_ALL-1   */
    SUBSCRIBE_COUNT = 3
};

bool IsValidStoreId(std::string const& storeId);
bool IsStoreTypeSupported(DistributedKv::Options const& options);
bool TaiheSecurityLevelToNative(int32_t level, int32_t &out);
bool SecurityLevelToTaihe(int32_t level, int32_t &out);
DistributedKv::SubscribeType SubscribeTypeToNative(::ohos::data::distributedkvstore::SubscribeType type);
DistributedKv::SubscribeType SubscribeTypeToNative(uint8_t type);
void TaiheValueTypeUnionToNativeVariant(::ohos::data::distributedkvstore::ValueTypeUnion const& value,
    ValueVariant &resultObj);
void TaiheValueToVariant(::ohos::data::distributedkvstore::Value const& value,
    ValueVariant &resultObj);
void TaiheDataShareValueToVariant(::ohos::data::distributedkvstore::DataShareValueTypeUnion const& value,
    DataShareValueVariant &resultObj);
::ohos::data::distributedkvstore::ValueTypeUnion Blob2TaiheValue(DistributedKv::Blob const& blob, uint8_t &resultType);
DistributedKv::Blob VariantValue2Blob(ValueVariant const& value);
bool EntryArrayToNative(::taihe::array_view<::ohos::data::distributedkvstore::Entry> const& taiheEntries,
    std::vector<DistributedKv::Entry> &out, bool hasSchema);
std::vector<std::string> StringArrayToNative(::taihe::array_view<::taihe::string> const& para);
::ohos::data::distributedkvstore::Entry GetEmptyTaiheEntry();
::ohos::data::distributedkvstore::Entry KvEntryToTaihe(DistributedKv::Entry const& kventry, bool hasSchema);
::taihe::array<::ohos::data::distributedkvstore::Entry> KvEntryArrayToTaihe(
    std::vector<DistributedKv::Entry> const& kventries, bool hasSchema);
::ohos::data::distributedkvstore::ChangeNotification KvChangeNotificationToTaihe(
    DistributedKv::ChangeNotification const& kvNotification, bool hasSchema);
::taihe::map<::taihe::string, int32_t> KvStatusMapToTaiheMap(
    std::map<std::string, DistributedKv::Status> const& mapInfo);
::taihe::array<uintptr_t> KvStatusMapToTaiheArray(ani_env* env,
    std::map<std::string, DistributedKv::Status> const& mapInfo);
}  // namespace ani_kvstoreutils
#endif  // OHOS_ANI_KVSTORE_UTILS_H
