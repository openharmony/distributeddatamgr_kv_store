/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "KvUtils"

#include "kv_utils.h"
#include <endian.h>
#include "cov_util.h"
#include "log_print.h"
#include "data_query.h"
#include "kvstore_datashare_bridge.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
using namespace DistributedData;
const std::string KvUtils::KEY = "key";
const std::string KvUtils::VALUE = "value";
constexpr KvUtils::QueryHandler KvUtils::HANDLERS[LAST_TYPE];

std::shared_ptr<ResultSetBridge> KvUtils::ToResultSetBridge(std::shared_ptr<KvStoreResultSet> resultSet)
{
    if (resultSet == nullptr) {
        ZLOGE("This param is error, kvResultSet is nullptr");
        return nullptr;
    }
    return std::make_shared<KvStoreDataShareBridge>(resultSet);
}

Status KvUtils::ToQuery(const DataShareAbsPredicates &predicates, DataQuery &query)
{
    const auto &operations = predicates.GetOperationList();
    for (const auto &oper : operations) {
        if (oper.operation < 0 || oper.operation >= LAST_TYPE) {
            ZLOGE("This operation param is error");
            return Status::NOT_SUPPORT;
        }
        (*HANDLERS[oper.operation])(oper, query);
    }
    return Status::SUCCESS;
}

std::vector<Entry> KvUtils::ToEntries(const std::vector<DataShareValuesBucket> &valueBuckets)
{
    std::vector<Entry> entries;
    for (const auto &val : valueBuckets) {
        Entry entry = ToEntry(val);
        entries.push_back(entry);
    }
    return entries;
}

Entry KvUtils::ToEntry(const DataShareValuesBucket &valueBucket)
{
    const auto &values = valueBucket.valuesMap;
    if (values.empty()) {
        ZLOGE("This valuesMap is null");
        return {};
    }
    Entry entry;
    Status status = ToEntryKey(values, entry.key);
    if (status != Status::SUCCESS) {
        ZLOGE("GetEntry key failed: %{public}d", status);
        return {};
    }
    status = ToEntryValue(values, entry.value);
    if (status != Status::SUCCESS) {
        ZLOGE("GetEntry value failed: %{public}d", status);
        return {};
    }
    return entry;
}

Status KvUtils::GetKeys(const DataShareAbsPredicates &predicates, std::vector<Key> &keys)
{
    const auto &operations = predicates.GetOperationList();
    if (operations.empty()) {
        ZLOGE("This operations is null");
        return Status::ERROR;
    }

    std::vector<std::string> myKeys;
    for (const auto &oper : operations) {
        if (oper.operation != IN_KEY) {
            ZLOGE("This find operation is failed");
            return Status::NOT_SUPPORT;
        }
        auto *val = std::get_if<std::vector<std::string>>(&oper.multiParams[0]);
        if (val == nullptr) {
            continue;
        }
        myKeys.insert(myKeys.end(), val->begin(), val->end());
    }
    for (const auto &it : myKeys) {
        keys.push_back(it.c_str());
    }
    return Status::SUCCESS;
}

Status KvUtils::ToEntryKey(const std::map<std::string, DataShareValueObject::Type> &values, Blob &blob)
{
    auto it = values.find(KEY);
    if (it == values.end()) {
        ZLOGE("This field is not find!");
        return Status::ERROR;
    }
    if (auto *val = std::get_if<std::string>(&it->second)) {
        std::vector<uint8_t> uData;
        std::string data = *val;
        uData.insert(uData.end(), data.begin(), data.end());
        blob = Blob(uData);
        return Status::SUCCESS;
    }
    ZLOGE("This value bucket type is not string");
    return Status::ERROR;
}

Status KvUtils::ToEntryValue(const std::map<std::string, DataShareValueObject::Type> &values, Blob &blob)
{
    auto it = values.find(VALUE);
    if (it == values.end()) {
        ZLOGE("This field is not find!");
        return Status::ERROR;
    }

    std::vector<uint8_t> uData;
    if (auto *val = std::get_if<std::vector<uint8_t>>(&it->second)) {
        ZLOGD("Value bucket type blob");
        std::vector<uint8_t> data = *val;
        uData.push_back(KvUtils::BYTE_ARRAY);
        uData.insert(uData.end(), data.begin(), data.end());
    } else if (auto *val = std::get_if<int64_t>(&it->second)) {
        ZLOGD("Value bucket type int");
        int64_t data = *val;
        uint64_t data64 = htobe64(*reinterpret_cast<uint64_t*>(&data));
        uint8_t *dataU8 = reinterpret_cast<uint8_t*>(&data64);
        uData.push_back(KvUtils::INTEGER);
        uData.insert(uData.end(), dataU8, dataU8 + sizeof(int64_t) / sizeof(uint8_t));
    } else if (auto *val = std::get_if<double>(&it->second)) {
        ZLOGD("Value bucket type double");
        double data = *val;
        uint64_t data64 = htobe64(*reinterpret_cast<uint64_t*>(&data));
        uint8_t *dataU8 = reinterpret_cast<uint8_t*>(&data64);
        uData.push_back(KvUtils::DOUBLE);
        uData.insert(uData.end(), dataU8, dataU8 + sizeof(double) / sizeof(uint8_t));
    } else if (auto *val = std::get_if<bool>(&it->second)) {
        ZLOGD("Value bucket type bool");
        bool data = *val;
        uData.push_back(KvUtils::BOOLEAN);
        uData.push_back(static_cast<uint8_t>(data));
    } else if (auto *val = std::get_if<std::string>(&it->second)) {
        ZLOGD("Value bucket type string");
        std::string data = *val;
        uData.push_back(KvUtils::STRING);
        uData.insert(uData.end(), data.begin(), data.end());
    }
    blob = Blob(uData);
    return Status::SUCCESS;
}

void KvUtils::NoSupport(const DataShare::OperationItem &oper, DataQuery &query)
{
    ZLOGE("Invalid operation:%{public}d", oper.operation);
}

void KvUtils::InKeys(const OperationItem &oper, DataQuery &query)
{
    auto *val = std::get_if<std::vector<std::string>>(&oper.multiParams[0]);
    if (val == nullptr) {
        return;
    }
    query.InKeys(*val);
}

void KvUtils::KeyPrefix(const OperationItem &oper, DataQuery &query)
{
    query.KeyPrefix(oper.GetSingle(0));
}

void KvUtils::EqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys equal(&query, QueryType::EQUAL);
    CovUtil::FillField(oper.GetSingle(0), oper.singleParams[1], equal);
}

void KvUtils::NotEqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys notEqual(&query, QueryType::NOT_EQUAL);
    CovUtil::FillField(oper.GetSingle(0), oper.singleParams[1], notEqual);
}

void KvUtils::GreaterThan(const OperationItem &oper, DataQuery &query)
{
    Querys greater(&query, QueryType::GREATER);
    CovUtil::FillField(oper.GetSingle(0), oper.singleParams[1], greater);
}

void KvUtils::LessThan(const OperationItem &oper, DataQuery &query)
{
    Querys less(&query, QueryType::LESS);
    CovUtil::FillField(oper.GetSingle(0), oper.singleParams[1], less);
}

void KvUtils::GreaterThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys greaterOrEqual(&query, QueryType::GREATER_OR_EQUAL);
    CovUtil::FillField(oper.GetSingle(0), oper.singleParams[1], greaterOrEqual);
}

void KvUtils::LessThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys lessOrEqual(&query, QueryType::LESS_OR_EQUAL);
    CovUtil::FillField(oper.GetSingle(0), oper.singleParams[1], lessOrEqual);
}

void KvUtils::And(const OperationItem &oper, DataQuery &query)
{
    query.And();
}

void KvUtils::Or(const OperationItem &oper, DataQuery &query)
{
    query.Or();
}

void KvUtils::IsNull(const OperationItem &oper, DataQuery &query)
{
    query.IsNull(oper.GetSingle(0));
}

void KvUtils::IsNotNull(const OperationItem &oper, DataQuery &query)
{
    query.IsNotNull(oper.GetSingle(0));
}

void KvUtils::In(const OperationItem &oper, DataQuery &query)
{
    InOrNotIn in(&query, QueryType::IN);
    CovUtil::FillField(oper.GetSingle(0), oper.multiParams[0], in);
}

void KvUtils::NotIn(const OperationItem &oper, DataQuery &query)
{
    InOrNotIn notIn(&query, QueryType::NOT_IN);
    CovUtil::FillField(oper.GetSingle(0), oper.multiParams[0], notIn);
}

void KvUtils::Like(const OperationItem &oper, DataQuery &query)
{
    query.Like(oper.GetSingle(0), oper.GetSingle(1));
}

void KvUtils::Unlike(const OperationItem &oper, DataQuery &query)
{
    query.Unlike(oper.GetSingle(0), oper.GetSingle(1));
}

void KvUtils::OrderByAsc(const OperationItem &oper, DataQuery &query)
{
    query.OrderByAsc(oper.GetSingle(0));
}

void KvUtils::OrderByDesc(const OperationItem &oper, DataQuery &query)
{
    query.OrderByDesc(oper.GetSingle(0));
}

void KvUtils::Limit(const OperationItem &oper, DataQuery &query)
{
    query.Limit(oper.GetSingle(0), oper.GetSingle(1));
}
} // namespace DistributedKv
} // namespace OHOS