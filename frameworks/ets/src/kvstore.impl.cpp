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
#define LOG_TAG "KV_ETS"

#include "kvstore.proj.hpp"
#include "kvstore.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "napi_queue.h"
#include "uv_queue.h"
#include "ability_context_impl.h"
#include "log_print.h"
#include "ability.h"
#include "blob.h"
#include "ani_base_context.h"
#include "extension_context.h"
#include "distributed_kv_data_manager.h"
#include "types.h"
using namespace taihe;
using namespace kvstore;
using namespace OHOS;
using namespace OHOS::DistributedData;
using Status = OHOS::DistributedKv::Status;

namespace {
enum CONTEXT_MODE { INIT = -1, FA = 0, STAGE = 1 };
static CONTEXT_MODE g_contextNode = INIT;
enum ValueType {
    STRING = 0,
    INTEGER = 1,
    FLOAT = 2,
    BYTE_ARRAY = 3,
    BOOLEAN = 4,
    DOUBLE = 5,
    INVALID = 255
};
struct ContextParam {
    std::string baseDir = "";
    std::string hapName = "";
    int32_t area = DistributedKv::Area::EL1;
};

struct EtsErrorCode {
    int32_t status;
    int32_t etsCode;
    const char *message;
};

static constexpr EtsErrorCode ERROR_CODE_MSGS[] = {
    { Status::NOT_FOUND, 15100004, "Not found." },
    { Status::CRYPT_ERROR, 15100003, "Database corrupted." },
    { Status::WAL_OVER_LIMITS, 14800047, "the WAL file size exceeds the default limit."}
};

const std::optional<EtsErrorCode> GetErrorCode(int32_t errorCode)
{
    auto etsErrorCode = EtsErrorCode{ errorCode, -1, "" };
    auto iter = std::lower_bound(ERROR_CODE_MSGS,
        ERROR_CODE_MSGS + sizeof(ERROR_CODE_MSGS) / sizeof(ERROR_CODE_MSGS[0]), etsErrorCode,
        [](const EtsErrorCode &etsErrorCode1, const EtsErrorCode &etsErrorCode2) {
        return etsErrorCode1.status < etsErrorCode2.status;
    });
    if (iter < ERROR_CODE_MSGS + sizeof(ERROR_CODE_MSGS) / sizeof(ERROR_CODE_MSGS[0]) &&
        iter->status == errorCode) {
        return *iter;
    }
    return std::nullopt;
}

void ThrowErrCode(Status status)
{
    int32_t code = 0;
    std::string message;
    auto err = GetErrorCode(status);
    if (err.has_value()) {
        auto napiError = err.value();
        code = napiError.etsCode;
        message = napiError.message;
    } else {
        code = -1;
        message = "";
    }
    ::taihe::set_business_error(code, message);
    return;
}

class FieldNodeImpl {
public:
    FieldNodeImpl()
    {
    }
    bool AppendChild(FieldNode child)
    {
        TH_THROW(std::runtime_error, "appendChild not implemented");
        fields_.push_back(child);
        return true;
    }

    string GetDefaultValue()
    {
        TH_THROW(std::runtime_error, "getDefaultValue not implemented");
        return defaultValue_;
    }

    void SetDefaultValue(string_view a)
    {
        TH_THROW(std::runtime_error, "setDefaultValue not implemented");
        defaultValue_ = a;
    }

    bool GetNullable()
    {
        TH_THROW(std::runtime_error, "getNullable not implemented");
        return isNullable_;
    }

    void SetNullable(bool a)
    {
        TH_THROW(std::runtime_error, "setNullable not implemented");
        isNullable_ = a;
    }

    int32_t GetType()
    {
        TH_THROW(std::runtime_error, "getType not implemented");
        return valueType_;
    }

    void SetType(int32_t a)
    {
        TH_THROW(std::runtime_error, "setType not implemented");
        a = valueType_;
    }
private:
    std::vector<FieldNode> fields_;
    string defaultValue_ = "";
    bool isNullable_ = false;
    uint32_t valueType_ = ValueType::INVALID;
};

class SchemaImpl {
public:
    SchemaImpl()
    {
    }
    enum {
        SCHEMA_MODE_SLOPPY,
        SCHEMA_MODE_STRICT,
    };
    FieldNode GetRoot()
    {
        return rootNode_;
    }

    void SetRoot(weak::FieldNode a)
    {
        TH_THROW(std::runtime_error, "setRoot not implemented");
        rootNode_ = a;
    }

    array<string> GetIndexes()
    {
        TH_THROW(std::runtime_error, "getIndexes not implemented");
        return indexes_;
    }

    void SetIndexes(array_view<string> a)
    {
        TH_THROW(std::runtime_error, "setIndexes not implemented");
        indexes_ = a;
    }

    int32_t GetMode()
    {
        TH_THROW(std::runtime_error, "getMode not implemented");
        return mode_;
    }

    void SetMode(int32_t a)
    {
        TH_THROW(std::runtime_error, "setMode not implemented");
        mode_ = a;
    }

    int32_t GetSkip()
    {
        TH_THROW(std::runtime_error, "getSkip not implemented");
        return skip_;
    }

    void SetSkip(int32_t a)
    {
        TH_THROW(std::runtime_error, "setSkip not implemented");
        skip_ = a;
    }
private:
    FieldNode rootNode_ = make_holder<FieldNodeImpl, FieldNode>();
    array<taihe::string> indexes_ = {};
    uint32_t mode_ = SCHEMA_MODE_SLOPPY;
    uint32_t skip_ = 0;
};

char* MallocCString(const std::string& origin)
{
    if (origin.empty()) {
        return nullptr;
    }
    auto len = origin.length() + 1;
    char* res = static_cast<char*>(malloc(sizeof(char) * len));
    if (res == nullptr) {
        return nullptr;
    }
    return std::char_traits<char>::copy(res, origin.c_str(), len);
}

::kvstore::DataTypes KVValueToDataTypes(const DistributedKv::Blob& blob)
{
    auto& data = blob.Data();
    if (data.size() < 1) {
        ZLOGI("Blob have no data!");
        return kvstore::DataTypes::make_doubleType(0);
    }
    std::vector<uint8_t> real(data.begin() + 1, data.end());
    if (data[0] == ValueType::STRING) {
        return kvstore::DataTypes::make_stringType(MallocCString(std::string(real.begin(), real.end())));
    } else if (data[0] == ValueType::DOUBLE) {
        return kvstore::DataTypes::make_doubleType(real[0]);
    } else if (data[0] == ValueType::BYTE_ARRAY) {
        auto arr = ::taihe::array<uint8_t>(::taihe::copy_data_t{}, real.data(), real.size());
        return kvstore::DataTypes::make_arrayType(std::move(arr));
    }
    return kvstore::DataTypes::make_booleanType(real[0]);
}

DistributedKv::Blob DataTypesToKVValue(const ::kvstore::DataTypes value)
{
    std::vector<uint8_t> data;
    ZLOGI("valueType is %{public}d ", value.get_tag());
    switch (value.get_tag()) {
        case ::kvstore::DataTypes::tag_t::stringType: {
            std::string str = std::string(value.get_stringType_ref());
            data.push_back(ValueType::STRING);
            data.insert(data.end(), str.begin(), str.end());
            break;
        }
        case ::kvstore::DataTypes::tag_t::arrayType: {
            array<uint8_t> val = array<uint8_t>(value.get_arrayType_ref());
            data.push_back(ValueType::BYTE_ARRAY);
            data.insert(data.end(), val.begin(), val.end());
            break;
        }
        case ::kvstore::DataTypes::tag_t::doubleType: {
            double tmp = double(value.get_doubleType_ref());
            data.push_back(ValueType::DOUBLE);
            data.push_back(static_cast<double>(tmp));
            break;
        }
        case ::kvstore::DataTypes::tag_t::booleanType: {
            bool val = bool(value.get_booleanType_ref());
            data.push_back(ValueType::BOOLEAN);
            data.push_back(static_cast<uint8_t>(val));
            break;
        }
    }
    return data;
}

class SingleKVStoreImpl {
public:
    SingleKVStoreImpl()
    {
    }

    ::kvstore::DataTypes GetSync(::taihe::string_view key)
    {
        auto s_key = OHOS::DistributedKv::Key(std::string(key));
        OHOS::DistributedKv::Value value;
        auto status = kvStore_->Get(s_key, value);
        ThrowErrCode(status);
        return KVValueToDataTypes(value);
    }

    void BackupSync(::taihe::string_view file)
    {
        kvStore_->Backup(std::string(file), param_->baseDir);
    }

    void PutSync(::taihe::string_view key, ::kvstore::DataTypes const& value)
    {
        auto tempKey = DistributedKv::Key(std::string(key));
        auto status = kvStore_->Put(tempKey, DataTypesToKVValue(value));
        ThrowErrCode(status);
    }

    void SetKvStorePtr(std::shared_ptr<OHOS::DistributedKv::SingleKvStore> kvStore)
    {
        kvStore_ = kvStore;
    }

    void SetContextParam(std::shared_ptr<ContextParam> param)
    {
        param_ = param;
    }

    int64_t GetInner()
    {
        return reinterpret_cast<int64_t>(this);
    }

    std::shared_ptr<OHOS::DistributedKv::SingleKvStore> kvStore_;
    std::shared_ptr<ContextParam> param_;
};

class DeviceKVStoreImpl : public SingleKVStoreImpl {
public:
    DeviceKVStoreImpl() : SingleKVStoreImpl()
    {
    }

    ::kvstore::DataTypes GetByDevIdSync(::taihe::string_view deviceId, ::taihe::string_view key)
    {
        std::ostringstream oss;
        if (!deviceId.empty()) {
            oss << std::setfill(zeroChar) << std::setw(deviceidWidth) << deviceId.size() << deviceId;
        }
        oss << key;
        std::string deviceKey = std::string(oss.str());
        auto s_key = DistributedKv::Key(deviceKey);
        OHOS::DistributedKv::Value value;
        auto status = kvStore_->Get(s_key, value);
        ThrowErrCode(status);
        return KVValueToDataTypes(value);
    }

    int64_t GetDevInner()
    {
        return reinterpret_cast<int64_t>(this);
    }
private:
    const int deviceidWidth = 4;
    const char zeroChar = '0';
};

class KVManagerImpl {
public:
    KVManagerImpl(string bunleName, std::shared_ptr<ContextParam> param)
    {
        bundleName_ = bunleName;
        param_ = param;
    }
    OHOS::DistributedKv::DistributedKvDataManager kvDataManager_ {};
    ::kvstore::KvStoreTypes GetKVStoreSync(::taihe::string_view storeId, ::kvstore::Options const& options)
        {
        OHOS::DistributedKv::Options kvOptions;
        kvOptions.createIfMissing = options.createIfMissing.value();
        kvOptions.encrypt = options.encrypt.value();
        kvOptions.backup = options.backup.value();
        kvOptions.autoSync = options.autoSync.value();
        if (options.kvStoreType.has_value()) {
            kvOptions.kvStoreType = static_cast<OHOS::DistributedKv::KvStoreType>(
                static_cast<int32_t>(options.kvStoreType.value()));
        }
        kvOptions.securityLevel = options.securityLevel;
        kvOptions.baseDir = param_->baseDir;
        kvOptions.area = param_->area + 1;
        kvOptions.hapName = param_->hapName;
        OHOS::DistributedKv::AppId appId = { std::string(bundleName_) };
        OHOS::DistributedKv::StoreId kvStoreId = { std::string(storeId) };
        std::shared_ptr<OHOS::DistributedKv::SingleKvStore> kvStore;
        Status status = kvDataManager_.GetSingleKvStore(kvOptions, appId, kvStoreId, kvStore);
        if (status == OHOS::DistributedKv::DATA_CORRUPTED) {
            kvOptions.rebuild = true;
            status = kvDataManager_.GetSingleKvStore(kvOptions, appId, kvStoreId, kvStore);
        }
        if (options.kvStoreType.has_value() && options.kvStoreType.value() == 1) {
            auto nativeKVStore = make_holder<SingleKVStoreImpl, SingleKVStore>();
            (reinterpret_cast<SingleKVStoreImpl*>(nativeKVStore->GetInner()))->SetKvStorePtr(kvStore);
            (reinterpret_cast<SingleKVStoreImpl*>(nativeKVStore->GetInner()))->SetContextParam(param_);
            return KvStoreTypes::make_singleKVStore(nativeKVStore);
        }
        auto nativeKVStore = make_holder<DeviceKVStoreImpl, DeviceKVStore>();
        (reinterpret_cast<SingleKVStoreImpl*>(nativeKVStore->GetDevInner()))->SetKvStorePtr(kvStore);
        (reinterpret_cast<SingleKVStoreImpl*>(nativeKVStore->GetDevInner()))->SetContextParam(param_);
        return KvStoreTypes::make_deviceKVStore(nativeKVStore);
    }
private:
    std::string bundleName_ {};
    std::shared_ptr<ContextParam> param_;
};

::kvstore::FieldNode CreateFieldNode(::taihe::string_view name)
{
    return make_holder<FieldNodeImpl, ::kvstore::FieldNode>();
}

::kvstore::Schema CreateSchema()
{
    return make_holder<SchemaImpl, ::kvstore::Schema>();
}

CONTEXT_MODE GetContextMode(ani_env* env, ani_object context)
{
    if (g_contextNode == INIT) {
        ani_boolean isStageMode;
        ani_status status = OHOS::AbilityRuntime::IsStageContext(env, context, isStageMode);
        ZLOGI("GetContextMode is %{public}d", static_cast<bool>(isStageMode));
        if (status == ANI_OK) {
            g_contextNode = isStageMode ? STAGE : FA;
        }
    }
    return g_contextNode;
}

::kvstore::KVManager CreateKVManager(::kvstore::KVManagerConfig const& config)
{
    ContextParam param;
    auto env = ::taihe::get_env();
    if (GetContextMode(env, reinterpret_cast<ani_object>(config.context)) == STAGE) {
        auto context = OHOS::AbilityRuntime::GetStageModeContext(env, reinterpret_cast<ani_object>(config.context));
        param.area = context->GetArea();
        param.baseDir = context->GetDatabaseDir();
        auto hapInfo = context->GetHapModuleInfo();
        if (hapInfo != nullptr) {
            param.hapName = hapInfo->moduleName;
        }
    } else {
        ZLOGE("ContextMode is not STAGE!");
    }
    return make_holder<KVManagerImpl, ::kvstore::KVManager>(config.bundleName,
        std::make_shared<ContextParam>(std::move(param)));
}
} // namespace
TH_EXPORT_CPP_API_CreateFieldNode(CreateFieldNode);
TH_EXPORT_CPP_API_CreateSchema(CreateSchema);
TH_EXPORT_CPP_API_CreateKVManager(CreateKVManager);