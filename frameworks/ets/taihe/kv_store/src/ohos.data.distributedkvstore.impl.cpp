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
#include "ohos.data.distributedkvstore.proj.hpp"
#include "ohos.data.distributedkvstore.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#include <optional>
#include <nlohmann/json.hpp>

#define LOG_TAG "AniKvstoreImpl"
#include "log_print.h"
#include "ani_ability_utils.h"
#include "ani_utils.h"
#include "ani_kvstore_utils.h"
#include "ani_error_utils.h"
#include "ani_observer_utils.h"
#include "types.h"
#include "distributed_kv_data_manager.h"
#include "datashare_abs_predicates.h"
#include "js_proxy.h"
#include "kv_utils.h"
#include "kvstore_datashare_bridge.h"
#include "store_errno.h"

using namespace OHOS;
using namespace OHOS::DistributedKVStore;
using namespace ::ohos::data;
using TaiheOptions = distributedkvstore::Options;

static constexpr int MAX_APP_ID_LEN = 256;
static constexpr int DEVICEID_WIDTH = 4;
static constexpr const char* DATA_CHANGE_EVENT = "dataChange";
static constexpr const char* SYNC_COMPLETE_EVENT = "syncComplete";

static std::map<uint32_t, std::string> valueTypeToString_ = {
    { ani_kvstoreutils::STRING, std::string("STRING") },
    { ani_kvstoreutils::BYTE_ARRAY, std::string("BYTE_ARRAY") },
    { ani_kvstoreutils::BOOLEAN, std::string("BOOL") },
    { ani_kvstoreutils::DOUBLE, std::string("DOUBLE") },
    { ani_kvstoreutils::LONG, std::string("LONG") }
};

namespace {
class FieldNodeImpl {
public:
    FieldNodeImpl()
        : fieldName_(""), default_("")
    {
    }

    explicit FieldNodeImpl(::taihe::string_view name)
        : fieldName_(""), default_("")
    {
        fieldName_ = std::string(name);
    }

    int64_t GetInner()
    {
        return reinterpret_cast<int64_t>(this);
    }

    std::string TypeToString(uint32_t type)
    {
        auto it = valueTypeToString_.find(type);
        if (valueTypeToString_.find(type) != valueTypeToString_.end()) {
            return it->second;
        } else {
            return std::string();
        }
    }

    nlohmann::json GetValueForJson()
    {
        if (fields_.empty()) {
            int32_t nativeType = ani_kvstoreutils::TaiheValueTypeToNative(GetType());
            std::string jsonDesc = TypeToString(nativeType) + (GetNullable() ? SPLIT : NOT_NULL) + DEFAULT;
            if (nativeType == ani_kvstoreutils::STRING) {
                jsonDesc += MARK + std::string(GetDefaultValue()) + MARK;
            } else {
                jsonDesc += std::string(GetDefaultValue());
            }
            return jsonDesc;
        }

        nlohmann::json jsFields;
        for (auto fld : fields_) {
            FieldNodeImpl* impl = reinterpret_cast<FieldNodeImpl*>(fld->GetInner());
            if (impl == nullptr) {
                continue;
            }
            jsFields[std::string(impl->fieldName_)] = impl->GetValueForJson();
        }
        return jsFields;
    }

    bool AppendChild(distributedkvstore::weak::FieldNode child)
    {
        fields_.push_back(child);
        return true;
    }

    ::taihe::string GetDefaultValue()
    {
        return default_;
    }

    void SetDefaultValue(::taihe::string_view para)
    {
        default_ = para;
    }

    bool GetNullable()
    {
        return nullable_;
    }

    void SetNullable(bool para)
    {
        nullable_ = para;
    }

    int32_t GetType()
    {
        return valueType_;
    }

    void SetType(int32_t para)
    {
        valueType_ = para;
    }

protected:
    std::list<distributedkvstore::FieldNode> fields_;
    ::taihe::string fieldName_;
    ::taihe::string default_;
    int32_t valueType_ = static_cast<int32_t>(distributedkvstore::ValueType::key_t::STRING);
    bool nullable_ = false;
};

class SchemaImpl {
public:
    SchemaImpl()
    {
    }

    int64_t GetInner()
    {
        return reinterpret_cast<int64_t>(this);
    }

    distributedkvstore::FieldNode GetRoot()
    {
        if (!root_.has_value()) {
            root_ = taihe::make_holder<FieldNodeImpl, distributedkvstore::FieldNode>(SCHEMA_DEFINE);
        }
        return root_.value();
    }

    void SetRoot(distributedkvstore::weak::FieldNode para)
    {
        root_ = para;
    }

    ::taihe::array<::taihe::string> GetIndexes()
    {
        return indexes_;
    }

    void SetIndexes(::taihe::array_view<::taihe::string> para)
    {
        indexes_ = para;
    }

    int32_t GetMode()
    {
        return mode_;
    }

    void SetMode(int32_t para)
    {
        mode_ = para;
    }

    int32_t GetSkip()
    {
        return skip_;
    }

    void SetSkip(int32_t para)
    {
        skip_ = para;
    }

    static FieldNodeImpl* GetRootNodeImpl(distributedkvstore::Schema const& taiheSchema)
    {
        auto nativeSchemaPtr = reinterpret_cast<SchemaImpl*>(taiheSchema->GetInner());
        if (nativeSchemaPtr == nullptr) {
            ZLOGE("DumpSchema, SchemaImpl nullptr");
            return nullptr;
        }
        auto rootNodeImpl = reinterpret_cast<FieldNodeImpl*>(nativeSchemaPtr->GetRoot()->GetInner());
        return rootNodeImpl;
    }

    static std::string DumpSchema(distributedkvstore::Schema const& taiheSchema)
    {
        auto rootNodeImpl = GetRootNodeImpl(taiheSchema);
        if (rootNodeImpl == nullptr) {
            ZLOGE("DumpSchema, rootNodeImpl nullptr");
            return "";
        }
        nlohmann::json jsIndexes = nlohmann::json::array();
        auto indexs = taiheSchema->GetIndexes();
        for (auto idx : indexs) {
            jsIndexes.push_back(idx);
        }
        nlohmann::json js = {
            { SCHEMA_VERSION, DEFAULT_SCHEMA_VERSION },
            { SCHEMA_MODE, (taiheSchema->GetMode() == SCHEMA_MODE_STRICT) ? SCHEMA_STRICT : SCHEMA_COMPATIBLE },
            { SCHEMA_DEFINE, rootNodeImpl->GetValueForJson() },
            { SCHEMA_INDEXES, jsIndexes },
            { SCHEMA_SKIPSIZE, taiheSchema->GetSkip() },
        };
        return js.dump();
    }

protected:
    std::optional<distributedkvstore::FieldNode> root_;
    ::taihe::array<::taihe::string> indexes_ = {};
    int32_t mode_ = 0;
    int32_t skip_ = 0;
};

class ResultSetProxy final : public OHOS::JSProxy::JSCreator<OHOS::DataShare::ResultSetBridge> {
public:
    ResultSetProxy() = default;
    explicit ResultSetProxy(std::shared_ptr<DistributedKv::KvStoreResultSet> resultSet)
    {
        resultSet_ = resultSet;
    }

    ResultSetProxy operator=(std::shared_ptr<DistributedKv::KvStoreResultSet> resultSet)
    {
        if (resultSet_ == resultSet) {
            return *this;
        }
        resultSet_ = resultSet;
        return *this;
    }

    std::shared_ptr<OHOS::DataShare::ResultSetBridge> Create() override
    {
        if (resultSet_ == nullptr) {
            ZLOGE("resultSet_ == nullptr");
            return nullptr;
        }
        return std::make_shared<DistributedKv::KvStoreDataShareBridge>(resultSet_);
    }

protected:
    std::shared_ptr<DistributedKv::KvStoreResultSet> resultSet_;
};

class KVStoreResultSetImpl {
public:
    KVStoreResultSetImpl()
    {
    }

    explicit KVStoreResultSetImpl(std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet,
        bool isSchema)
    {
        nativeResultSet_ = kvResultSet;
        proxy_ = std::make_shared<ResultSetProxy>(kvResultSet);
        hasSchema_ = isSchema;
    }

    int64_t GetInner()
    {
        return reinterpret_cast<int64_t>(this);
    }

    int64_t GetProxy()
    {
        return reinterpret_cast<int64_t>(proxy_.get());
    }

    int32_t GetCount()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", 0);
        return nativeResultSet_->GetCount();
    }

    int32_t GetPosition()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", 0);
        return nativeResultSet_->GetPosition();
    }

    bool MoveToFirst()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->MoveToFirst();
    }

    bool MoveToLast()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->MoveToLast();
    }

    bool MoveToNext()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->MoveToNext();
    }

    bool MoveToPrevious()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->MoveToPrevious();
    }

    bool Move(int32_t offset)
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->Move(offset);
    }

    bool MoveToPosition(int32_t position)
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->MoveToPosition(position);
    }

    bool IsFirst()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->IsFirst();
    }

    bool IsLast()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->IsLast();
    }

    bool IsBeforeFirst()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->IsBeforeFirst();
    }

    bool IsAfterLast()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", false);
        return nativeResultSet_->IsAfterLast();
    }

    distributedkvstore::Entry GetEntry()
    {
        ANI_ASSERT(nativeResultSet_ != nullptr, "kvResultSet is nullptr!", ani_kvstoreutils::GetEmptyTaiheEntry());
        DistributedKv::Entry kventry;
        Status status = nativeResultSet_->GetEntry(kventry);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return ani_kvstoreutils::GetEmptyTaiheEntry();
        }
        return ani_kvstoreutils::KvEntryToTaihe(kventry, hasSchema_);
    }

    std::shared_ptr<DistributedKv::KvStoreResultSet> GetNativePtr()
    {
        return nativeResultSet_;
    }

protected:
    std::shared_ptr<DistributedKv::KvStoreResultSet> nativeResultSet_;
    std::shared_ptr<ResultSetProxy> proxy_;
    bool hasSchema_ = false;
};

class QueryImpl {
public:
    QueryImpl()
    {
        nativeQueryPtr_ = std::make_shared<DistributedKv::DataQuery>();
    }

    int64_t GetInner()
    {
        return reinterpret_cast<int64_t>(this);
    }

    distributedkvstore::Query Reset(distributedkvstore::weak::Query thiz)
    {
        nativeQueryPtr_->Reset();
        return thiz;
    }

    distributedkvstore::Query EqualTo(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, distributedkvstore::LongDoubleStringBool const& value)
    {
        std::string stdField(field);
        if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::F64) {
            nativeQueryPtr_->EqualTo(stdField, value.get_F64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::INT64) {
            nativeQueryPtr_->EqualTo(stdField, value.get_INT64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::STRING) {
            nativeQueryPtr_->EqualTo(stdField, std::string(value.get_STRING_ref()));
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::BOOL) {
            nativeQueryPtr_->EqualTo(stdField, value.get_BOOL_ref());
        }
        return thiz;
    }

    distributedkvstore::Query NotEqualTo(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, distributedkvstore::LongDoubleStringBool const& value)
    {
        std::string stdField(field);
        if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::F64) {
            nativeQueryPtr_->NotEqualTo(stdField, value.get_F64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::INT64) {
            nativeQueryPtr_->NotEqualTo(stdField, value.get_INT64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::STRING) {
            nativeQueryPtr_->NotEqualTo(stdField, std::string(value.get_STRING_ref()));
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::BOOL) {
            nativeQueryPtr_->NotEqualTo(stdField, value.get_BOOL_ref());
        }
        return thiz;
    }

    distributedkvstore::Query GreaterThan(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, distributedkvstore::LongDoubleStringBool const& value)
    {
        std::string stdField(field);
        if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::F64) {
            nativeQueryPtr_->GreaterThan(stdField, value.get_F64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::INT64) {
            nativeQueryPtr_->GreaterThan(stdField, value.get_INT64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::STRING) {
            nativeQueryPtr_->GreaterThan(stdField, std::string(value.get_STRING_ref()));
        } else if (value.get_tag() == distributedkvstore::LongDoubleStringBool::tag_t::BOOL) {
            nativeQueryPtr_->GreaterThan(stdField, value.get_BOOL_ref());
        }
        return thiz;
    }

    distributedkvstore::Query LessThan(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, distributedkvstore::LongDoubleString const& value)
    {
        std::string stdField(field);
        if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::F64) {
            nativeQueryPtr_->LessThan(stdField, value.get_F64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::INT64) {
            nativeQueryPtr_->LessThan(stdField, value.get_INT64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::STRING) {
            nativeQueryPtr_->LessThan(stdField, std::string(value.get_STRING_ref()));
        }
        return thiz;
    }

    distributedkvstore::Query GreaterThanOrEqualTo(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, distributedkvstore::LongDoubleString const& value)
    {
        std::string stdField(field);
        if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::F64) {
            nativeQueryPtr_->GreaterThanOrEqualTo(stdField, value.get_F64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::INT64) {
            nativeQueryPtr_->GreaterThanOrEqualTo(stdField, value.get_INT64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::STRING) {
            nativeQueryPtr_->GreaterThanOrEqualTo(stdField, std::string(value.get_STRING_ref()));
        }
        return thiz;
    }

    distributedkvstore::Query LessThanOrEqualTo(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, distributedkvstore::LongDoubleString const& value)
    {
        std::string stdField(field);
        if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::F64) {
            nativeQueryPtr_->LessThanOrEqualTo(stdField, value.get_F64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::INT64) {
            nativeQueryPtr_->LessThanOrEqualTo(stdField, value.get_INT64_ref());
        } else if (value.get_tag() == distributedkvstore::LongDoubleString::tag_t::STRING) {
            nativeQueryPtr_->LessThanOrEqualTo(stdField, std::string(value.get_STRING_ref()));
        }
        return thiz;
    }

    distributedkvstore::Query IsNull(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field)
    {
        std::string stdField(field);
        nativeQueryPtr_->IsNull(stdField);
        return thiz;
    }

    distributedkvstore::Query InNumber(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, ::taihe::array_view<distributedkvstore::LongDouble> valueList)
    {
        if (valueList.size() == 0) {
            return thiz;
        }
        std::string stdField(field);
        auto &tempItem = valueList[0];
        if (tempItem.get_tag() == distributedkvstore::LongDouble::tag_t::F64) {
            std::vector<double> doubleVector(valueList.size());
            std::transform(valueList.begin(), valueList.end(), doubleVector.begin(),
                [](distributedkvstore::LongDouble c) {
                return c.get_F64_ref();
            });
            nativeQueryPtr_->In(stdField, doubleVector);
        } else if (tempItem.get_tag() == distributedkvstore::LongDouble::tag_t::INT64) {
            std::vector<int64_t> longVector(valueList.size());
            std::transform(valueList.begin(), valueList.end(), longVector.begin(),
                [](distributedkvstore::LongDouble c) {
                return c.get_INT64_ref();
            });
            nativeQueryPtr_->In(stdField, longVector);
        }
        return thiz;
    }

    distributedkvstore::Query InString(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, ::taihe::array_view<::taihe::string> valueList)
    {
        auto stdArray = ani_kvstoreutils::StringArrayToNative(valueList);
        nativeQueryPtr_->In(std::string(field), stdArray);
        return thiz;
    }

    distributedkvstore::Query NotInNumber(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, ::taihe::array_view<distributedkvstore::LongDouble> valueList)
    {
        if (valueList.size() == 0) {
            return thiz;
        }
        std::string stdField(field);
        auto &tempItem = valueList[0];
        if (tempItem.get_tag() == distributedkvstore::LongDouble::tag_t::F64) {
            std::vector<double> doubleVector(valueList.size());
            std::transform(valueList.begin(), valueList.end(), doubleVector.begin(),
                [](distributedkvstore::LongDouble c) {
                return c.get_F64_ref();
            });
            nativeQueryPtr_->NotIn(stdField, doubleVector);
        } else if (tempItem.get_tag() == distributedkvstore::LongDouble::tag_t::INT64) {
            std::vector<int64_t> longVector(valueList.size());
            std::transform(valueList.begin(), valueList.end(), longVector.begin(),
                [](distributedkvstore::LongDouble c) {
                return c.get_INT64_ref();
            });
            nativeQueryPtr_->NotIn(stdField, longVector);
        }
        return thiz;
    }

    distributedkvstore::Query NotInString(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, ::taihe::array_view<::taihe::string> valueList)
    {
        auto stdArray = ani_kvstoreutils::StringArrayToNative(valueList);
        nativeQueryPtr_->NotIn(std::string(field), stdArray);
        return thiz;
    }

    distributedkvstore::Query Like(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, ::taihe::string_view value)
    {
        nativeQueryPtr_->Like(std::string(field), std::string(value));
        return thiz;
    }

    distributedkvstore::Query Unlike(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field, ::taihe::string_view value)
    {
        nativeQueryPtr_->Unlike(std::string(field), std::string(value));
        return thiz;
    }

    distributedkvstore::Query And(distributedkvstore::weak::Query thiz)
    {
        nativeQueryPtr_->And();
        return thiz;
    }

    distributedkvstore::Query Or(distributedkvstore::weak::Query thiz)
    {
        nativeQueryPtr_->Or();
        return thiz;
    }

    distributedkvstore::Query OrderByAsc(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field)
    {
        nativeQueryPtr_->OrderByAsc(std::string(field));
        return thiz;
    }

    distributedkvstore::Query OrderByDesc(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field)
    {
        nativeQueryPtr_->OrderByDesc(std::string(field));
        return thiz;
    }

    distributedkvstore::Query Limit(distributedkvstore::weak::Query thiz,
        int32_t total, int32_t offset)
    {
        nativeQueryPtr_->Limit(total, offset);
        return thiz;
    }

    distributedkvstore::Query IsNotNull(distributedkvstore::weak::Query thiz,
        ::taihe::string_view field)
    {
        nativeQueryPtr_->IsNotNull(std::string(field));
        return thiz;
    }

    distributedkvstore::Query BeginGroup(distributedkvstore::weak::Query thiz)
    {
        nativeQueryPtr_->BeginGroup();
        return thiz;
    }

    distributedkvstore::Query EndGroup(distributedkvstore::weak::Query thiz)
    {
        nativeQueryPtr_->EndGroup();
        return thiz;
    }

    distributedkvstore::Query PrefixKey(distributedkvstore::weak::Query thiz,
        ::taihe::string_view prefix)
    {
        nativeQueryPtr_->KeyPrefix(std::string(prefix));
        return thiz;
    }

    distributedkvstore::Query SetSuggestIndex(distributedkvstore::weak::Query thiz,
        ::taihe::string_view index)
    {
        nativeQueryPtr_->SetSuggestIndex(std::string(index));
        return thiz;
    }

    distributedkvstore::Query DeviceId(distributedkvstore::weak::Query thiz,
        ::taihe::string_view deviceId)
    {
        nativeQueryPtr_->DeviceId(std::string(deviceId));
        return thiz;
    }

    ::taihe::string GetSqlLike()
    {
        auto stdstr = nativeQueryPtr_->ToString();
        return ::taihe::string(stdstr);
    }

    std::shared_ptr<DistributedKv::DataQuery> GetNativePtr()
    {
        return nativeQueryPtr_;
    }

protected:
    std::shared_ptr<DistributedKv::DataQuery> nativeQueryPtr_;
};

class SingleKVStoreImpl {
public:
    SingleKVStoreImpl()
    {
        ZLOGE("SingleKVStoreImpl default constructor");
    }

    explicit SingleKVStoreImpl(std::shared_ptr<OHOS::DistributedKv::SingleKvStore> kvStore)
    {
        ZLOGI("SingleKVStoreImpl constructor");
        nativeStore_ = kvStore;
    }
    ~SingleKVStoreImpl()
    {
        ZLOGI("SingleKVStoreImpl ~");
        UnRegisterAll();
    }

    virtual int64_t GetInner()
    {
        return reinterpret_cast<int64_t>(this);
    }

    bool IsSystemApp()
    {
        return contextParam_.isSystemApp;
    }

    bool IsSchemaStore() const
    {
        return isSchemaStore_;
    }

    void SetSchemaInfo(bool isSchemaStore)
    {
        isSchemaStore_ = isSchemaStore;
    }

    void SetContextParam(ContextParam const& param)
    {
        contextParam_ = param;
    }

    void PutSync(::taihe::string_view key, distributedkvstore::ValueUnion const& value)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        ani_kvstoreutils::ValueVariant paramVariant;
        ani_kvstoreutils::TaiheValueUnionToNativeVariant(value, paramVariant);
        std::string stdkey(key);
        DistributedKv::Key kvkey(stdkey);
        bool isSchemaStore = IsSchemaStore();
        if (isSchemaStore && !std::holds_alternative<std::string>(paramVariant)) {
            ZLOGE("isSchemaStore, and value is not string");
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        DistributedKv::Value nativeValue = isSchemaStore ? DistributedKv::Blob(std::get<std::string>(paramVariant))
                                                   : ani_kvstoreutils::VariantValue2Blob(paramVariant);
        Status status = nativeStore_->Put(kvkey, nativeValue);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void PutBatchSync(::taihe::array_view<distributedkvstore::Entry> entries)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        bool isSchemaStore = IsSchemaStore();
        std::vector<DistributedKv::Entry> kvEntris;
        if (false == ani_kvstoreutils::EntryArrayToNative(entries, kvEntris, isSchemaStore)) {
            ZLOGE("isSchemaStore, and value is not string");
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        Status status = nativeStore_->PutBatch(kvEntris);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void PutValuesBucketsSync(::taihe::array_view<
        ::taihe::map<::taihe::string, distributedkvstore::DataShareValueUnion>> value)
    {
        if (!IsSystemApp()) {
            ThrowAniError(Status::PERMISSION_DENIED, "");
            return;
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        bool isSchemaStore = IsSchemaStore();
        std::vector<DistributedKv::Entry> kvEntris;
        for (auto& arrayItem : value) {
            OHOS::DataShare::DataShareValuesBucket valuesBucket;
            for (auto& [key, mapvalue] : arrayItem) {
                std::string stdkey(key);
                ani_kvstoreutils::DataShareValueVariant variant;
                ani_kvstoreutils::TaiheDataShareValueToVariant(mapvalue, variant);
                valuesBucket.valuesMap[stdkey] = variant;
            }
            auto entry = DistributedKv::KvUtils::ToEntry(valuesBucket);
            entry.key = std::vector<uint8_t>(entry.key.Data().begin(), entry.key.Data().end());
            if (isSchemaStore) {
                entry.value = std::vector<uint8_t>(entry.value.Data().begin() + 1, entry.value.Data().end());
            }
            kvEntris.emplace_back(entry);
        }
        Status status = nativeStore_->PutBatch(kvEntris);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void DeleteSync(::taihe::string_view key)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        std::string stdkey(key);
        DistributedKv::Key kvkey(stdkey);
        Status status = nativeStore_->Delete(kvkey);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void DeleteByPredicatesSync(uintptr_t predicates)
    {
        if (!IsSystemApp()) {
            ThrowAniError(Status::PERMISSION_DENIED, "");
            return;
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        ani_env *env = taihe::get_env();
        ani_object object = reinterpret_cast<ani_object>(predicates);
        OHOS::DataShare::DataShareAbsPredicates *holder =
            ani_utils::AniObjectUtils::Unwrap<OHOS::DataShare::DataShareAbsPredicates>(env, object);
        if (holder == nullptr) {
            ZLOGE("DeleteByPredicatesSync, holder is nullptr");
            return ;
        }
        std::vector<OHOS::DistributedKv::Key> kvkeys;
        Status status = OHOS::DistributedKv::KvUtils::GetKeys(*holder, kvkeys);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return;
        }
        status = nativeStore_->DeleteBatch(kvkeys);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void DeleteBatchSync(::taihe::array_view<::taihe::string> keys)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        std::vector<DistributedKv::Key> kvKeys(keys.size());
        std::transform(keys.begin(), keys.end(), kvKeys.begin(), [](::taihe::string c) {
            std::string stdkey(c);
            DistributedKv::Key kvkey(stdkey);
            return kvkey;
        });
        Status status = nativeStore_->DeleteBatch(kvKeys);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void RemoveDeviceDataSync(::taihe::string_view deviceId)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        std::string stdDeviceId(deviceId);
        if (stdDeviceId.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:deviceId empty");
            return;
        }
        Status status = nativeStore_->RemoveDeviceData(stdDeviceId);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    distributedkvstore::ValueUnion GetSync(::taihe::string_view key)
    {
        auto emptyResult = distributedkvstore::ValueUnion::make_STRING(std::string(""));
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return emptyResult;
        }
        std::string stdkey(key);
        if (stdkey.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:params key must be string and not allow empty");
            return emptyResult;
        }
        bool isSchemaStore = IsSchemaStore();
        OHOS::DistributedKv::Key kvkey(stdkey);
        OHOS::DistributedKv::Value kvblob;
        Status status = nativeStore_->Get(kvkey, kvblob);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return emptyResult;
        }
        if (isSchemaStore) {
            return distributedkvstore::ValueUnion::make_STRING(kvblob.ToString());
        } else {
            uint8_t resultType = 0;
            return ani_kvstoreutils::Blob2TaiheValue(kvblob, resultType);
        }
    }

    ::taihe::array<distributedkvstore::Entry> GetEntriesSync(::taihe::string_view keyPrefix)
    {
        if (keyPrefix.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return {};
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return {};
        }
        bool isSchemaStore = IsSchemaStore();
        std::vector<DistributedKv::Entry> kventries;
        Status status = nativeStore_->GetEntries(std::string(keyPrefix), kventries);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return {};
        }
        return ani_kvstoreutils::KvEntryArrayToTaihe(kventries, isSchemaStore);
    }

    ::taihe::array<distributedkvstore::Entry> GetEntriesByQuerySync(
        distributedkvstore::weak::Query query)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return {};
        }
        bool isSchemaStore = IsSchemaStore();
        std::vector<DistributedKv::Entry> kventries;
        auto queryImpl = reinterpret_cast<QueryImpl*>(query->GetInner());
        auto nativeQueryPtr = queryImpl->GetNativePtr();
        Status status = nativeStore_->GetEntries(*nativeQueryPtr, kventries);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return {};
        }
        return ani_kvstoreutils::KvEntryArrayToTaihe(kventries, isSchemaStore);
    }

    distributedkvstore::KVStoreResultSet GetResultSetSync(::taihe::string_view keyPrefix)
    {
        if (keyPrefix.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
        Status status = nativeStore_->GetResultSet(std::string(keyPrefix), kvResultSet);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>(
            kvResultSet, isSchemaStore_);
    }

    distributedkvstore::KVStoreResultSet GetResultSetByQuerySync(
        distributedkvstore::weak::Query query)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        auto queryImpl = reinterpret_cast<QueryImpl*>(query->GetInner());
        auto nativeQueryPtr = queryImpl->GetNativePtr();
        std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
        Status status = nativeStore_->GetResultSet(*nativeQueryPtr, kvResultSet);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>(
            kvResultSet, isSchemaStore_);
    }

    distributedkvstore::KVStoreResultSet GetResultSetByPredicatesSync(uintptr_t predicates)
    {
        if (!IsSystemApp()) {
            ThrowAniError(Status::PERMISSION_DENIED, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        ani_env *env = taihe::get_env();
        ani_object object = reinterpret_cast<ani_object>(predicates);
        OHOS::DataShare::DataShareAbsPredicates *holder =
            ani_utils::AniObjectUtils::Unwrap<OHOS::DataShare::DataShareAbsPredicates>(env, object);
        if (holder == nullptr) {
            ZLOGE("GetResultSetByPredicatesSync, holder is nullptr");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        DistributedKv::DataQuery kvquery;
        Status status = OHOS::DistributedKv::KvUtils::ToQuery(*holder, kvquery);
        if (status != Status::SUCCESS) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
        status = nativeStore_->GetResultSet(kvquery, kvResultSet);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>(
            kvResultSet, isSchemaStore_);
    }

    void CloseResultSetSync(distributedkvstore::weak::KVStoreResultSet resultSet)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        auto resultSetImpl = reinterpret_cast<KVStoreResultSetImpl*>(resultSet->GetInner());
        auto nativeResultSetPtr = resultSetImpl->GetNativePtr();
        Status status = nativeStore_->CloseResultSet(nativeResultSetPtr);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    int32_t GetResultSizeSync(distributedkvstore::weak::Query query)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return 0;
        }
        auto queryImpl = reinterpret_cast<QueryImpl*>(query->GetInner());
        auto nativeQueryPtr = queryImpl->GetNativePtr();
        int resultSize = 0;
        Status status = nativeStore_->GetCount(*nativeQueryPtr, resultSize);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
        return resultSize;
    }

    void BackupSync(::taihe::string_view file)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        Status status = nativeStore_->Backup(std::string(file), contextParam_.baseDir);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void RestoreSync(::taihe::string_view file)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        Status status = nativeStore_->Restore(std::string(file), contextParam_.baseDir);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    ::taihe::array<uintptr_t> DeleteBackupSync(::taihe::array_view<::taihe::string> files)
    {
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            return {};
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return {};
        }
        std::vector<std::string> stdfiles = ani_kvstoreutils::StringArrayToNative(files);
        std::map<std::string, DistributedKv::Status> results;
        Status status = nativeStore_->DeleteBackup(stdfiles, contextParam_.baseDir, results);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return {};
        }
        return ani_kvstoreutils::KvStatusMapToTaiheArray(env, results);
    }

    void StartTransactionSync()
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        Status status = nativeStore_->StartTransaction();
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void CommitSync()
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        Status status = nativeStore_->Commit();
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void RollbackSync()
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        Status status = nativeStore_->Rollback();
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void EnableSyncSync(bool enabled)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        Status status = nativeStore_->SetCapabilityEnabled(enabled);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void SetSyncRangeSync(::taihe::array_view<::taihe::string> localLabels,
        ::taihe::array_view<::taihe::string> remoteSupportLabels)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        std::vector<std::string> stdLocalLabels = ani_kvstoreutils::StringArrayToNative(localLabels);
        std::vector<std::string> stdRemoteSupportLabels = ani_kvstoreutils::StringArrayToNative(remoteSupportLabels);
        Status status = nativeStore_->SetCapabilityRange(stdLocalLabels, stdRemoteSupportLabels);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void SetSyncParamSync(int32_t defaultAllowedDelayMs)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        DistributedKv::KvSyncParam syncParam { defaultAllowedDelayMs };
        Status status = nativeStore_->SetSyncParam(syncParam);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void Sync(::taihe::array_view<::taihe::string> deviceIds, distributedkvstore::SyncMode mode,
        ::taihe::optional_view<int32_t> delayMs)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        std::vector<std::string> stdDeviceIds = ani_kvstoreutils::StringArrayToNative(deviceIds);
        DistributedKv::SyncMode kvmode = static_cast<DistributedKv::SyncMode>(mode.get_value());
        uint32_t kvAllowedDelayMs = 0;
        if (delayMs.has_value()) {
            kvAllowedDelayMs = delayMs.value();
        }
        Status status = nativeStore_->Sync(stdDeviceIds, kvmode, kvAllowedDelayMs);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void SyncByQuery(::taihe::array_view<::taihe::string> deviceIds,
        distributedkvstore::weak::Query query,
        distributedkvstore::SyncMode mode, ::taihe::optional_view<int32_t> delayMs)
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        auto queryImpl = reinterpret_cast<QueryImpl*>(query->GetInner());
        auto nativeQueryPtr = queryImpl->GetNativePtr();
        std::vector<std::string> stdDeviceIds = ani_kvstoreutils::StringArrayToNative(deviceIds);
        DistributedKv::SyncMode kvmode = static_cast<DistributedKv::SyncMode>(mode.get_value());
        uint32_t kvAllowedDelayMs = 0;
        if (delayMs.has_value()) {
            kvAllowedDelayMs = delayMs.value();
        }
        Status status = nativeStore_->Sync(stdDeviceIds, kvmode, *nativeQueryPtr,
            nullptr, kvAllowedDelayMs);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    void OnDataChange(distributedkvstore::SubscribeType type,
        ::taihe::callback_view<void(distributedkvstore::ChangeNotification const& info)> f, uintptr_t opq)
    {
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            return;
        }
        if (!ani_utils::AniIsInstanceOf(env, reinterpret_cast<ani_ref>(opq), "std.core.Object")) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        auto kvsubscribeType = ani_kvstoreutils::SubscribeTypeToNative(type);
        ani_observerutils::VarCallbackType varcb = f;
        RegisterListener(DATA_CHANGE_EVENT, kvsubscribeType, varcb, opq);
    }

    void OffDataChange(::taihe::optional_view<uintptr_t> opq)
    {
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            return;
        }
        if (opq.has_value() &&
            !ani_utils::AniIsInstanceOf(env, reinterpret_cast<ani_ref>(opq.value()), "std.core.Object")) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        bool isUpdated = false;
        UnregisterListener(DATA_CHANGE_EVENT, opq, isUpdated);
    }

    void OnSyncComplete(::taihe::callback_view<void(::taihe::array_view<uintptr_t> info)> f, uintptr_t opq)
    {
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            return;
        }
        if (!ani_utils::AniIsInstanceOf(env, reinterpret_cast<ani_ref>(opq), "std.core.Object")) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        ani_observerutils::VarCallbackType varcb = f;
        RegisterListener(SYNC_COMPLETE_EVENT, DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL, varcb, opq);
    }

    void OffSyncComplete(::taihe::optional_view<uintptr_t> opq)
    {
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            return;
        }
        if (opq.has_value() &&
            !ani_utils::AniIsInstanceOf(env, reinterpret_cast<ani_ref>(opq.value()), "std.core.Object")) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return;
        }
        bool isUpdated = false;
        UnregisterListener(SYNC_COMPLETE_EVENT, opq, isUpdated);
    }

    distributedkvstore::SecurityLevel GetSecurityLevelSync()
    {
        if (nativeStore_ == nullptr) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return distributedkvstore::SecurityLevel::from_value(-1);
        }
        DistributedKv::SecurityLevel secLevel;
        Status status = nativeStore_->GetSecurityLevel(secLevel);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return distributedkvstore::SecurityLevel::from_value(-1);
        }
        int32_t jslevel = -1;
        bool result = ani_kvstoreutils::SecurityLevelToTaihe(secLevel, jslevel);
        if (!result) {
            ThrowAniError(Status::ILLEGAL_STATE, "");
            return distributedkvstore::SecurityLevel::from_value(-1);
        }
        return distributedkvstore::SecurityLevel::from_value(jslevel);
    }

protected:
    void RegisterListener(std::string const& event, DistributedKv::SubscribeType type,
        ani_observerutils::VarCallbackType &cb, uintptr_t opq)
    {
        if (nativeStore_ == nullptr) {
            ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
            return;
        }
        std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
        auto &cbVec = jsCbMap_[event];
        ani_object callbackObj = reinterpret_cast<ani_object>(opq);
        ani_ref callbackRef = CreateCallbackRefIfNotDuplicate(cbVec, callbackObj);
        if (callbackRef == nullptr) {
            ZLOGE("Failed to register %{public}s", event.c_str());
            return;
        }
        Status status = Status::SUCCESS;
        if (event == DATA_CHANGE_EVENT) {
            status = RegisterDataChangeObserver(type, cb, callbackRef);
        } else if (event == SYNC_COMPLETE_EVENT) {
            status = RegisterSyncCompleteObserver(cb, callbackRef);
        }
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return;
        }
        ZLOGI("RegisterListener success type: %{public}s", event.c_str());
    }
    
    void UnregisterListener(std::string const& event, ::taihe::optional_view<uintptr_t> opq, bool &isUpdateFlag)
    {
        if (nativeStore_ == nullptr) {
            ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
            return;
        }
        std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
        const auto iter = jsCbMap_.find(event);
        if (iter == jsCbMap_.end()) {
            ZLOGE("%{public}s is not registered", event.c_str());
            return;
        }
        if (event == SYNC_COMPLETE_EVENT) {
            DistributedKv::SubscribeType kvtype = DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL;
            Status status = UnRegisterObserver(event, kvtype, opq, isUpdateFlag);
            if (status != Status::SUCCESS) {
                ZLOGE("UnregisterListener syncComplete failed, status %{public}d", status);
                ThrowAniError(status, "");
            }
            return;
        }
        for (uint8_t type = ani_kvstoreutils::SUBSCRIBE_LOCAL; type < ani_kvstoreutils::SUBSCRIBE_COUNT; type++) {
            bool updated = false;
            DistributedKv::SubscribeType kvtype = ani_kvstoreutils::SubscribeTypeToNative(type);
            ::taihe::optional<uintptr_t> empty;
            Status status = UnRegisterObserver(event, kvtype, opq, updated);
            isUpdateFlag |= updated;
            if (status != Status::SUCCESS) {
                ZLOGE("UnregisterListener failed, type = %{public}d, status %{public}d", type, status);
                ThrowAniError(status, "");
                return;
            }
        }
        ZLOGI("UnregisterListener success type:%{public}s", event.c_str());
    }

    Status RegisterDataChangeObserver(DistributedKv::SubscribeType type,
        ani_observerutils::VarCallbackType &cb, ani_ref callbackRef)
    {
        if (nativeStore_ == nullptr) {
            ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
            return Status::SUCCESS;
        }
        std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
        auto &cbVec = jsCbMap_[DATA_CHANGE_EVENT];
        auto observer = std::make_shared<ani_observerutils::DataObserver>(cb, callbackRef);
        observer->SetIsSchemaStore(isSchemaStore_);
        Status status = nativeStore_->SubscribeKvStore(type, observer);
        if (status != Status::SUCCESS) {
            ZLOGE("RegisterDataChangeObserver, SubscribeKvStore failed, %{public}d", status);
            observer->Release();
            return status;
        }
        cbVec.emplace_back(std::move(observer));
        return Status::SUCCESS;
    }

    Status RegisterSyncCompleteObserver(ani_observerutils::VarCallbackType &cb, ani_ref callbackRef)
    {
        if (nativeStore_ == nullptr) {
            ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
            return Status::SUCCESS;
        }
        std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
        auto &cbVec = jsCbMap_[SYNC_COMPLETE_EVENT];
        auto observer = std::make_shared<ani_observerutils::DataObserver>(cb, callbackRef);
        observer->SetIsSchemaStore(isSchemaStore_);
        Status status = nativeStore_->RegisterSyncCallback(observer);
        if (status != Status::SUCCESS) {
            ZLOGE("RegisterSyncCompleteObserver, RegisterSyncCallback failed, %{public}d", status);
            observer->Release();
            return status;
        }
        cbVec.emplace_back(std::move(observer));
        return Status::SUCCESS;
    }

    Status UnRegisterObserver(std::string const& event, DistributedKv::SubscribeType type,
        ::taihe::optional_view<uintptr_t> opq, bool &isUpdateFlag)
    {
        if (nativeStore_ == nullptr) {
            ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
            return Status::SUCCESS;
        }
        std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
        Status result = Status::SUCCESS;
        auto &callbackList = jsCbMap_[event];
        if (!opq.has_value()) {
            for (auto iter = callbackList.begin(); iter != callbackList.end();) {
                result = Status::SUCCESS;
                if (event == DATA_CHANGE_EVENT) {
                    result = nativeStore_->UnSubscribeKvStore(type, *iter);
                }
                if (result == Status::SUCCESS || result == Status::ALREADY_CLOSED) {
                    isUpdateFlag = true;
                    (*iter)->Release();
                    iter = callbackList.erase(iter);
                } else {
                    ZLOGE("SingleKVStoreImpl UnRegisterObserver failed, status %{public}d", result);
                    break;
                }
            }
            if (callbackList.empty()) {
                if (event == SYNC_COMPLETE_EVENT) {
                    result = nativeStore_->UnRegisterSyncCallback();
                    ZLOGI("UnRegisterObserver syncComplete, unregister native, %{public}d", result);
                }
                jsCbMap_.erase(event);
            }
            return result;
        }
        ani_env *env = taihe::get_env();
        ani_observerutils::GlobalRefGuard guard(env, reinterpret_cast<ani_object>(opq.value()));
        if (!guard) {
            ZLOGE("Failed to UnRegisterObserver, GlobalRefGuard is false!");
            return result;
        }
        return UnRegisterObserver(event, type, guard.get(), isUpdateFlag);
    }
 
    Status UnRegisterObserver(std::string const& event, DistributedKv::SubscribeType type,
        ani_ref jsCallbackRef, bool &isUpdateFlag)
    {
        if (nativeStore_ == nullptr) {
            ZLOGW("UnRegisterObserver, nativeStore_ is nullptr");
            return Status::SUCCESS;
        }
        std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            ZLOGE("Failed to UnRegisterObserver, env is nullptr");
            return Status::SUCCESS;
        }
        auto &callbackList = jsCbMap_[event];
        const auto pred = [env, jsCallbackRef](std::shared_ptr<ani_observerutils::DataObserver> &obj) {
            ani_boolean is_equal = false;
            return (ANI_OK == env->Reference_StrictEquals(jsCallbackRef, obj->jsCallbackRef_, &is_equal)) && is_equal;
        };
        const auto it = std::find_if(callbackList.begin(), callbackList.end(), pred);
        Status result = Status::SUCCESS;
        if (it != callbackList.end()) {
            if (event == DATA_CHANGE_EVENT) {
                result = nativeStore_->UnSubscribeKvStore(type, *it);
            }
            if (result == Status::SUCCESS || result == Status::ALREADY_CLOSED) {
                isUpdateFlag = true;
                (*it)->Release();
                callbackList.erase(it);
            } else {
                return result;
            }
        }
        if (callbackList.empty()) {
            if (event == SYNC_COMPLETE_EVENT) {
                result = nativeStore_->UnRegisterSyncCallback();
                ZLOGI("UnRegisterObserver syncComplete, unregister native, %{public}d", result);
            }
            jsCbMap_.erase(event);
        }
        return result;
    }

    void UnRegisterAll()
    {
        ZLOGI("SingleKVStoreImpl UnRegisterAll");
        std::lock_guard<std::recursive_mutex> lock(cbMapMutex_);
        bool isUpdated = false;
        ::taihe::optional<uintptr_t> empty;
        for (uint8_t type = ani_kvstoreutils::SUBSCRIBE_LOCAL; type < ani_kvstoreutils::SUBSCRIBE_COUNT; type++) {
            DistributedKv::SubscribeType kvtype = ani_kvstoreutils::SubscribeTypeToNative(type);
            UnRegisterObserver(DATA_CHANGE_EVENT, kvtype, empty, isUpdated);
        }
        UnRegisterObserver(SYNC_COMPLETE_EVENT, DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL, empty, isUpdated);
    }

protected:
    std::shared_ptr<OHOS::DistributedKv::SingleKvStore> nativeStore_;
    ContextParam contextParam_;
    bool isSchemaStore_ = false;
    std::recursive_mutex cbMapMutex_;
    std::map<std::string, std::vector<std::shared_ptr<ani_observerutils::DataObserver>>> jsCbMap_;
};

class DeviceKVStoreImpl : public SingleKVStoreImpl {
public:
    DeviceKVStoreImpl()
    {
        ZLOGE("DeviceKVStoreImpl default constructor");
    }

    explicit DeviceKVStoreImpl(std::shared_ptr<OHOS::DistributedKv::SingleKvStore> kvStore)
        :SingleKVStoreImpl(kvStore)
    {
        ZLOGI("DeviceKVStoreImpl constructor");
    }

    int64_t GetInner() override
    {
        return reinterpret_cast<int64_t>(this);
    }

    static std::string GetDeviceKey(const std::string& deviceId, const std::string& key)
    {
        std::ostringstream oss;
        if (!deviceId.empty()) {
            oss << std::setfill('0') << std::setw(DEVICEID_WIDTH) << deviceId.length() << deviceId;
        }
        oss << key;
        return oss.str();
    }

    distributedkvstore::ValueUnion GetByDeviceIdSync(::taihe::string_view deviceId,
        ::taihe::string_view key)
    {
        std::string stdkey(key);
        if (stdkey.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:params key must be string and not allow empty");
            return distributedkvstore::ValueUnion::make_STRING(std::string(""));
        }
        std::string stddeviceid(deviceId);
        bool isSchemaStore = IsSchemaStore();
        std::string deviceKey = GetDeviceKey(stddeviceid, stdkey);
        OHOS::DistributedKv::Key kvkey(deviceKey);
        OHOS::DistributedKv::Value kvblob;
        Status status = nativeStore_->Get(kvkey, kvblob);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return distributedkvstore::ValueUnion::make_STRING(std::string(""));
        }
        if (isSchemaStore) {
            return distributedkvstore::ValueUnion::make_STRING(kvblob.ToString());
        } else {
            uint8_t resultType = 0;
            return ani_kvstoreutils::Blob2TaiheValue(kvblob, resultType);
        }
    }

    ::taihe::array<distributedkvstore::Entry> GetEntriesByDeviceIdSync(::taihe::string_view deviceId,
        ::taihe::string_view keyPrefix)
    {
        if (keyPrefix.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return {};
        }
        std::string stdkeyprefix(keyPrefix);
        std::string stddeviceid(deviceId);
        bool isSchemaStore = IsSchemaStore();
        auto query = std::make_shared<DistributedKv::DataQuery>();
        query->KeyPrefix(stdkeyprefix);
        query->DeviceId(stddeviceid);
        std::vector<DistributedKv::Entry> kventries;
        Status status = nativeStore_->GetEntries(*query, kventries);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return {};
        }
        return ani_kvstoreutils::KvEntryArrayToTaihe(kventries, isSchemaStore);
    }

    ::taihe::array<distributedkvstore::Entry> GetEntriesByDeviceIdAndQuerySync(
        ::taihe::string_view deviceId, distributedkvstore::weak::Query query)
    {
        std::string stddeviceid(deviceId);
        bool isSchemaStore = IsSchemaStore();
        auto queryImpl = reinterpret_cast<QueryImpl*>(query->GetInner());
        auto nativeQueryPtr = queryImpl->GetNativePtr();
        nativeQueryPtr->DeviceId(stddeviceid);
        std::vector<DistributedKv::Entry> kventries;
        Status status = nativeStore_->GetEntries(*nativeQueryPtr, kventries);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return {};
        }
        return ani_kvstoreutils::KvEntryArrayToTaihe(kventries, isSchemaStore);
    }

    distributedkvstore::KVStoreResultSet GetResultSetByDeviceIdAndPrefixSync(
        ::taihe::string_view deviceId, ::taihe::string_view keyPrefix)
    {
        if (keyPrefix.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        DistributedKv::DataQuery query;
        query.KeyPrefix(std::string(keyPrefix));
        query.DeviceId(std::string(deviceId));
        std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
        Status status = nativeStore_->GetResultSet(query, kvResultSet);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>(
            kvResultSet, isSchemaStore_);
    }

    distributedkvstore::KVStoreResultSet GetResultSetByDeviceIdAndQuerySync(::taihe::string_view deviceId,
        distributedkvstore::weak::Query query)
    {
        auto queryImpl = reinterpret_cast<QueryImpl*>(query->GetInner());
        auto nativeQueryPtr = queryImpl->GetNativePtr();
        nativeQueryPtr->DeviceId(std::string(deviceId));
        std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
        Status status = nativeStore_->GetResultSet(*nativeQueryPtr, kvResultSet);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>(
            kvResultSet, isSchemaStore_);
    }

    distributedkvstore::KVStoreResultSet GetResultSetByDeviceIdAndPredicateSync(
        ::taihe::string_view deviceId, uintptr_t predicates)
    {
        if (!IsSystemApp()) {
            ThrowAniError(Status::PERMISSION_DENIED, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        ani_env *env = taihe::get_env();
        ani_object object = reinterpret_cast<ani_object>(predicates);
        OHOS::DataShare::DataShareAbsPredicates *holder =
            ani_utils::AniObjectUtils::Unwrap<OHOS::DataShare::DataShareAbsPredicates>(env, object);
        if (holder == nullptr) {
            ZLOGE("GetResultSetByDeviceIdAndPredicateSync, holder is nullptr");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        DistributedKv::DataQuery kvquery;
        Status status = OHOS::DistributedKv::KvUtils::ToQuery(*holder, kvquery);
        if (status != Status::SUCCESS) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        kvquery.DeviceId(std::string(deviceId));
        std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
        status = nativeStore_->GetResultSet(kvquery, kvResultSet);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>();
        }
        return taihe::make_holder<KVStoreResultSetImpl, distributedkvstore::KVStoreResultSet>(
            kvResultSet, isSchemaStore_);
    }

    int32_t GetResultSizeByDeviceIdSync(::taihe::string_view deviceId,
        distributedkvstore::weak::Query query)
    {
        auto queryImpl = reinterpret_cast<QueryImpl*>(query->GetInner());
        auto nativeQueryPtr = queryImpl->GetNativePtr();
        nativeQueryPtr->DeviceId(std::string(deviceId));
        int resultSize = 0;
        Status status = nativeStore_->GetCount(*nativeQueryPtr, resultSize);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
        return resultSize;
    }
};

class KVManagerImpl {
public:
    explicit KVManagerImpl(const distributedkvstore::KVManagerConfig &config)
    {
        bundleName_ = std::string(config.bundleName);
        if (bundleName_.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT,
                "Parameter error:The type of bundleName must be string.");
            return;
        }
        int32_t result = AniGetContext(reinterpret_cast<ani_object>(config.context), contextParam_);
        if (result != ANI_OK) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:get context failed");
            return;
        }
        kvDataManager_ = std::make_shared<DistributedKv::DistributedKvDataManager>();
    }

    ~KVManagerImpl()
    {
        UnregisterAllObserver();
    }

    static bool ParseOptions(TaiheOptions const& taiheOptions, DistributedKv::Options& options)
    {
        if (taiheOptions.createIfMissing.has_value()) {
            options.createIfMissing = taiheOptions.createIfMissing.value();
        }
        if (taiheOptions.encrypt.has_value()) {
            options.encrypt = taiheOptions.encrypt.value();
        }
        if (taiheOptions.backup.has_value()) {
            options.backup = taiheOptions.backup.value();
        }
        if (taiheOptions.autoSync.has_value()) {
            options.autoSync = taiheOptions.autoSync.value();
        }
        if (taiheOptions.kvStoreType.has_value()) {
            options.kvStoreType = static_cast<DistributedKv::KvStoreType>(taiheOptions.kvStoreType.value().get_value());
        }
        if (taiheOptions.schema.has_value()) {
            distributedkvstore::Schema schema = taiheOptions.schema.value();
            options.schema = SchemaImpl::DumpSchema(schema);
        }
        if (taiheOptions.securityLevel.has_value()) {
            auto taiheLevel = taiheOptions.securityLevel.value().get_value();
            bool result = ani_kvstoreutils::TaiheSecurityLevelToNative(taiheLevel, options.securityLevel);
            if (!result) {
                ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:The params type not matching option");
                return false;
            }
        }
        if (options.securityLevel == DistributedKv::SecurityLevel::INVALID_LABEL) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:unusable securityLevel");
            return false;
        }
        if (!ani_kvstoreutils::IsStoreTypeSupported(options)) {
            ThrowAniError(Status::INVALID_ARGUMENT,
                "Parameter error:only support DEVICE_COLLABORATION or SINGLE_VERSION");
            return false;
        }
        return true;
    }

    distributedkvstore::KvStoreTypeUnion MakeEmptyKvStore()
    {
        auto emptyResult = taihe::make_holder<SingleKVStoreImpl, distributedkvstore::SingleKVStore>();
        return distributedkvstore::KvStoreTypeUnion::make_singleKVStore(emptyResult);
    }

    distributedkvstore::KvStoreTypeUnion GetKVStoreSync(::taihe::string_view storeId,
        TaiheOptions const& options)
    {
        if (kvDataManager_ == nullptr) {
            ZLOGE("KVManager is null, failed!");
            ThrowAniError(Status::INVALID_ARGUMENT, "KVManager is null, failed!");
            return MakeEmptyKvStore();
        }
        DistributedKv::AppId appId = { bundleName_ };
        DistributedKv::StoreId kvStoreId = { std::string(storeId) };
        DistributedKv::Options kvOptions;
        bool parseResult = ParseOptions(options, kvOptions);
        if (!parseResult) {
            return MakeEmptyKvStore();
        }
        kvOptions.baseDir = contextParam_.baseDir;
        kvOptions.area = contextParam_.area + 1;
        kvOptions.hapName = contextParam_.hapName;
        kvOptions.apiVersion = contextParam_.apiVersion;

        std::shared_ptr<OHOS::DistributedKv::SingleKvStore> kvStore;
        Status status = kvDataManager_->GetSingleKvStore(kvOptions, appId, kvStoreId, kvStore);
        ZLOGE("GetSingleKvStore, securityLevel %{public}d, status %{public}d", kvOptions.securityLevel, status);
        if (status == OHOS::DistributedKv::DATA_CORRUPTED) {
            kvOptions.rebuild = true;
            status = kvDataManager_->GetSingleKvStore(kvOptions, appId, kvStoreId, kvStore);
        }
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return MakeEmptyKvStore();
        }
        if (options.kvStoreType.has_value() &&
            options.kvStoreType.value() == distributedkvstore::KVStoreType::key_t::SINGLE_VERSION) {
            auto nativeKVStore =
                taihe::make_holder<SingleKVStoreImpl, distributedkvstore::SingleKVStore>(kvStore);
            (reinterpret_cast<SingleKVStoreImpl*>(nativeKVStore->GetInner()))->SetContextParam(contextParam_);
            (reinterpret_cast<SingleKVStoreImpl*>(nativeKVStore->GetInner()))->SetSchemaInfo(!kvOptions.schema.empty());
            return distributedkvstore::KvStoreTypeUnion::make_singleKVStore(nativeKVStore);
        }
        auto nativeKVStore =
            taihe::make_holder<DeviceKVStoreImpl, distributedkvstore::DeviceKVStore>(kvStore);
        (reinterpret_cast<DeviceKVStoreImpl*>(nativeKVStore->GetInner()))->SetContextParam(contextParam_);
        (reinterpret_cast<DeviceKVStoreImpl*>(nativeKVStore->GetInner()))->SetSchemaInfo(!kvOptions.schema.empty());
        return distributedkvstore::KvStoreTypeUnion::make_deviceKVStore(nativeKVStore);
    }

    void CloseKVStoreSync(::taihe::string_view appId, ::taihe::string_view storeId)
    {
        if (kvDataManager_ == nullptr) {
            ThrowAniError(Status::INVALID_ARGUMENT, "KVManager is null, failed!");
            return;
        }
        if (std::string(appId).empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:appId empty");
            return;
        }
        if (!ani_kvstoreutils::IsValidStoreId(std::string(storeId))) {
            ThrowAniError(Status::INVALID_ARGUMENT,
                "Parameter error:storeId must be string,consist of letters, digits,"\
                " underscores(_), limit 128 characters");
            return;
        }
        DistributedKv::AppId kvappId = { std::string(appId) };
        DistributedKv::StoreId kvStoreId = { std::string(storeId) };
        Status status = kvDataManager_->CloseKvStore(kvappId, kvStoreId);
        if (status != Status::SUCCESS && status != Status::STORE_NOT_FOUND && status != Status::STORE_NOT_OPEN) {
            ThrowAniError(status, "");
        }
    }

    void DeleteKVStoreSync(::taihe::string_view appId, ::taihe::string_view storeId)
    {
        if (kvDataManager_ == nullptr) {
            ThrowAniError(Status::INVALID_ARGUMENT, "KVManager is null, failed!");
            return;
        }
        if (std::string(appId).empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:appId empty");
            return;
        }
        if (!ani_kvstoreutils::IsValidStoreId(std::string(storeId))) {
            ThrowAniError(Status::INVALID_ARGUMENT,
                "Parameter error:storeId must be string,consist of letters, digits,"\
                " underscores(_), limit 128 characters");
        }
        DistributedKv::AppId kvappId = { std::string(appId) };
        DistributedKv::StoreId kvStoreId = { std::string(storeId) };
        std::string databaseDir = contextParam_.baseDir;
        Status status = kvDataManager_->DeleteKvStore(kvappId, kvStoreId, databaseDir);
        ZLOGE("DeleteKVStoreSync 3, status %{public}d, DISTRIBUTEDDATAMGR_ERR_OFFSET %{public}d", status,
            DistributedKv::DISTRIBUTEDDATAMGR_ERR_OFFSET);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
        }
    }

    ::taihe::array<::taihe::string> GetAllKVStoreIdSync(::taihe::string_view appId)
    {
        std::string stdappid(appId);
        if (stdappid.empty()) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:appId empty");
            return {};
        }
        if (stdappid.size() >= MAX_APP_ID_LEN) {
            ThrowAniError(Status::INVALID_ARGUMENT, "Parameter error:appId exceed 256 characters");
            return {};
        }
        if (kvDataManager_ == nullptr) {
            ZLOGE("KVManager is null, failed!");
            ThrowAniError(ANI_INVALID_ARGS, "KVManager is null, failed!");
            return {};
        }
        DistributedKv::AppId kvappId { stdappid };
        std::vector<DistributedKv::StoreId> storeIds;
        DistributedKv::Status status = kvDataManager_->GetAllKvStoreId(kvappId, storeIds);
        if (status != Status::SUCCESS) {
            ThrowAniError(status, "");
            return {};
        }
        std::vector<std::string> stringArray(storeIds.size());
        std::transform(storeIds.begin(), storeIds.end(), stringArray.begin(), [](DistributedKv::StoreId c) {
            return c.storeId;
        });
        return ::taihe::array<::taihe::string>(::taihe::copy_data_t{}, stringArray.data(), stringArray.size());
    }

    void OnDistributedDataServiceDie(::taihe::callback_view<void(distributedkvstore::OneUndef const& para)> f,
        uintptr_t opq)
    {
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            return;
        }
        if (!ani_utils::AniIsInstanceOf(env, reinterpret_cast<ani_ref>(opq), "std.core.Object")) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        std::lock_guard<std::recursive_mutex> lock(cbDeathListMutex_);
        ani_object callbackObj = reinterpret_cast<ani_object>(opq);
        ani_ref callbackRef = CreateCallbackRefIfNotDuplicate(jsDeathCbList_, callbackObj);
        if (callbackRef == nullptr) {
            ZLOGE("failed to register");
            return;
        }
        auto observer = std::make_shared<ani_observerutils::ManagerObserver>(f, callbackRef);
        kvDataManager_->RegisterKvStoreServiceDeathRecipient(observer);
        jsDeathCbList_.emplace_back(std::move(observer));
    }

    void OffDistributedDataServiceDie(::taihe::optional_view<uintptr_t> opq)
    {
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            ZLOGE("failed to get_env");
            return;
        }
        if (opq.has_value() &&
            !ani_utils::AniIsInstanceOf(env, reinterpret_cast<ani_ref>(opq.value()), "std.core.Object")) {
            ThrowAniError(Status::INVALID_ARGUMENT, "");
            return;
        }
        std::lock_guard<std::recursive_mutex> lock(cbDeathListMutex_);
        ani_ref jsCallbackRef = nullptr;
        ani_object callbackObj = nullptr;
        if (opq.has_value()) {
            callbackObj = reinterpret_cast<ani_object>(opq.value());
        }
        ani_observerutils::GlobalRefGuard guard(env, callbackObj);
        if (callbackObj != nullptr && !guard) {
            ZLOGE("GlobalRefGuard is false!");
            return;
        }
        jsCallbackRef = guard.get();
        auto pred = [env, jsCallbackRef](std::shared_ptr<ani_observerutils::ManagerObserver> &obj) {
            ani_boolean is_equal = false;
            if (jsCallbackRef == nullptr) {
                return true;
            }
            return (ANI_OK == env->Reference_StrictEquals(jsCallbackRef, obj->jsCallbackRef_, &is_equal)) && is_equal;
        };
        for (auto iter = jsDeathCbList_.begin(); iter != jsDeathCbList_.end();) {
            if (pred(*iter) == true) {
                ZLOGI("jsDeathCbList_ erase item");
                (*iter)->Release();
                kvDataManager_->UnRegisterKvStoreServiceDeathRecipient(*iter);
                iter = jsDeathCbList_.erase(iter);
            } else {
                ++iter;
            }
        }
    }

protected:
    void UnregisterAllObserver()
    {
        ::taihe::optional<uintptr_t> empty;
        OffDistributedDataServiceDie(empty);
    }

protected:
    std::shared_ptr<DistributedKv::DistributedKvDataManager> kvDataManager_;
    ContextParam contextParam_;
    std::string bundleName_;
    std::recursive_mutex cbDeathListMutex_;
    std::vector<std::shared_ptr<ani_observerutils::ManagerObserver>> jsDeathCbList_;
};

distributedkvstore::Schema CreateSchema()
{
    return taihe::make_holder<SchemaImpl, distributedkvstore::Schema>();
}

distributedkvstore::FieldNode CreateFieldNode(::taihe::string_view name)
{
    return taihe::make_holder<FieldNodeImpl, distributedkvstore::FieldNode>(name);
}

distributedkvstore::Query CreateQuery()
{
    return taihe::make_holder<QueryImpl, distributedkvstore::Query>();
}

distributedkvstore::KVManager createKVManager(const distributedkvstore::KVManagerConfig &config)
{
    return taihe::make_holder<KVManagerImpl, distributedkvstore::KVManager>(config);
}
}  // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_CreateSchema(CreateSchema);
TH_EXPORT_CPP_API_CreateFieldNode(CreateFieldNode);
TH_EXPORT_CPP_API_CreateQuery(CreateQuery);
TH_EXPORT_CPP_API_createKVManager(createKVManager);
// NOLINTEND
