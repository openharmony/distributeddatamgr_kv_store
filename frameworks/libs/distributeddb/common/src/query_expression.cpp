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
#include "query_expression.h"
#include "log_print.h"
#include "schema_utils.h"
#include "db_errno.h"

namespace DistributedDB {
namespace {
    const int MAX_OPR_TIMES = 256;
} // namespace

void QueryExpression::AssemblyQueryInfo(const QueryObjType queryOperType, const std::string& field,
    const QueryValueType type, const std::vector<FieldValue> &values, bool isNeedFieldPath = true)
{
    if (useFromTable_) {
        expressions_[fromTable_].AssemblyQueryInfo(queryOperType, field, type, values, isNeedFieldPath);
        SetNotSupportIfNeed(queryOperType);
        return;
    }
    if (!tables_.empty()) {
        validStatus_ = -E_NOT_SUPPORT;
    }
    if (queryInfo_.size() > MAX_OPR_TIMES) {
        SetErrFlag(false);
        LOGE("Operate too much times!");
        return;
    }

    if (!GetErrFlag()) {
        LOGE("Illegal data node!");
        return;
    }

    FieldPath outPath;
    if (isNeedFieldPath) {
        if (SchemaUtils::ParseAndCheckFieldPath(field, outPath) != E_OK) {
            SetErrFlag(false);
            LOGE("Field path illegal!");
            return;
        }
    }
    std::string formatedField;
    if (isTableNameSpecified_) { // remove '$.' prefix in relational query
        for (auto it = outPath.begin(); it < outPath.end(); ++it) {
            if (it != outPath.begin()) {
                formatedField += ".";
            }
            formatedField += *it;
        }
    } else {
        formatedField = field;
    }
    if (!queryInfo_.empty() && queryInfo_.back().operFlag == queryOperType) {
        validStatus_ = -E_INVALID_ARGS;
    }
    queryInfo_.emplace_back(QueryObjNode{queryOperType, formatedField, type, values});
}

QueryExpression::QueryExpression()
    : errFlag_(true),
      tableName_("sync_data"), // default kv type store table name
      isTableNameSpecified_(false) // default no specify for kv type store table name
{}

void QueryExpression::EqualTo(const std::string& field, const QueryValueType type, const FieldValue &value)
{
    std::vector<FieldValue> fieldValues{value};
    AssemblyQueryInfo(QueryObjType::EQUALTO, field, type, fieldValues);
}

void QueryExpression::NotEqualTo(const std::string& field, const QueryValueType type, const FieldValue &value)
{
    std::vector<FieldValue> fieldValues{value};
    AssemblyQueryInfo(QueryObjType::NOT_EQUALTO, field, type, fieldValues);
}

void QueryExpression::GreaterThan(const std::string& field, const QueryValueType type, const FieldValue &value)
{
    if (type == QueryValueType::VALUE_TYPE_BOOL) {
        LOGD("Prohibit the use of bool for comparison!");
        SetErrFlag(false);
    }
    std::vector<FieldValue> fieldValues{value};
    AssemblyQueryInfo(QueryObjType::GREATER_THAN, field, type, fieldValues);
}

void QueryExpression::LessThan(const std::string& field, const QueryValueType type, const FieldValue &value)
{
    if (type == QueryValueType::VALUE_TYPE_BOOL) {
        LOGD("Prohibit the use of bool for comparison!");
        SetErrFlag(false);
    }
    std::vector<FieldValue> fieldValues{value};
    AssemblyQueryInfo(QueryObjType::LESS_THAN, field, type, fieldValues);
}

void QueryExpression::GreaterThanOrEqualTo(const std::string& field, const QueryValueType type, const FieldValue &value)
{
    if (type == QueryValueType::VALUE_TYPE_BOOL) {
        LOGD("Prohibit the use of bool for comparison!");
        SetErrFlag(false);
    }
    std::vector<FieldValue> fieldValues{value};
    AssemblyQueryInfo(QueryObjType::GREATER_THAN_OR_EQUALTO, field, type, fieldValues);
}

void QueryExpression::LessThanOrEqualTo(const std::string& field, const QueryValueType type, const FieldValue &value)
{
    if (type == QueryValueType::VALUE_TYPE_BOOL) {
        LOGD("Prohibit the use of bool for comparison!");
        SetErrFlag(false);
    }
    std::vector<FieldValue> fieldValues{value};
    AssemblyQueryInfo(QueryObjType::LESS_THAN_OR_EQUALTO, field, type, fieldValues);
}

void QueryExpression::OrderBy(const std::string& field, bool isAsc)
{
    FieldValue fieldValue;
    fieldValue.boolValue = isAsc;
    std::vector<FieldValue> fieldValues{fieldValue};
    AssemblyQueryInfo(QueryObjType::ORDERBY, field, QueryValueType::VALUE_TYPE_BOOL, fieldValues);
}

void QueryExpression::Like(const std::string& field, const std::string &value)
{
    FieldValue fieldValue;
    fieldValue.stringValue = value;
    std::vector<FieldValue> fieldValues{fieldValue};
    AssemblyQueryInfo(QueryObjType::LIKE, field, QueryValueType::VALUE_TYPE_STRING, fieldValues);
}

void QueryExpression::NotLike(const std::string& field, const std::string &value)
{
    FieldValue fieldValue;
    fieldValue.stringValue = value;
    std::vector<FieldValue> fieldValues{fieldValue};
    AssemblyQueryInfo(QueryObjType::NOT_LIKE, field, QueryValueType::VALUE_TYPE_STRING, fieldValues);
}

void QueryExpression::Limit(int number, int offset)
{
    FieldValue fieldNumber;
    fieldNumber.integerValue = number;
    FieldValue fieldOffset;
    fieldOffset.integerValue = offset;
    std::vector<FieldValue> fieldValues{fieldNumber, fieldOffset};
    AssemblyQueryInfo(QueryObjType::LIMIT, std::string(), QueryValueType::VALUE_TYPE_INTEGER, fieldValues, false);
}

void QueryExpression::IsNull(const std::string& field)
{
    AssemblyQueryInfo(QueryObjType::IS_NULL, field, QueryValueType::VALUE_TYPE_NULL, std::vector<FieldValue>());
}

void QueryExpression::IsNotNull(const std::string& field)
{
    AssemblyQueryInfo(QueryObjType::IS_NOT_NULL, field, QueryValueType::VALUE_TYPE_NULL, std::vector<FieldValue>());
}

void QueryExpression::In(const std::string& field, const QueryValueType type, const std::vector<FieldValue> &values)
{
    AssemblyQueryInfo(QueryObjType::IN, field, type, values);
}

void QueryExpression::NotIn(const std::string& field, const QueryValueType type, const std::vector<FieldValue> &values)
{
    AssemblyQueryInfo(QueryObjType::NOT_IN, field, type, values);
}

void QueryExpression::And()
{
    AssemblyQueryInfo(QueryObjType::AND, std::string(), QueryValueType::VALUE_TYPE_NULL,
        std::vector<FieldValue>(), false);
}

void QueryExpression::Or()
{
    AssemblyQueryInfo(QueryObjType::OR, std::string(), QueryValueType::VALUE_TYPE_NULL,
        std::vector<FieldValue>(), false);
}

void QueryExpression::QueryByPrefixKey(const std::vector<uint8_t> &key)
{
    if (useFromTable_) {
        expressions_[fromTable_].QueryByPrefixKey(key);
        validStatus_ = -E_NOT_SUPPORT;
        return;
    }
    SetNotSupportIfFromTables();
    queryInfo_.emplace_front(QueryObjNode{QueryObjType::QUERY_BY_KEY_PREFIX, std::string(),
        QueryValueType::VALUE_TYPE_NULL, std::vector<FieldValue>()});
    prefixKey_ = key;
}

void QueryExpression::QueryByKeyRange(const std::vector<uint8_t> &keyBegin, const std::vector<uint8_t> &keyEnd)
{
    if (useFromTable_) {
        expressions_[fromTable_].QueryByKeyRange(keyBegin, keyEnd);
        validStatus_ = -E_NOT_SUPPORT;
        return;
    }
    SetNotSupportIfFromTables();
    queryInfo_.emplace_front(QueryObjNode{QueryObjType::KEY_RANGE, std::string(),
        QueryValueType::VALUE_TYPE_NULL, std::vector<FieldValue>()});
    beginKey_ = keyBegin;
    endKey_ = keyEnd;
}

void QueryExpression::QueryBySuggestIndex(const std::string &indexName)
{
    if (useFromTable_) {
        expressions_[fromTable_].QueryBySuggestIndex(indexName);
        validStatus_ = -E_NOT_SUPPORT;
        return;
    }
    SetNotSupportIfFromTables();
    queryInfo_.emplace_back(QueryObjNode{QueryObjType::SUGGEST_INDEX, indexName,
        QueryValueType::VALUE_TYPE_STRING, std::vector<FieldValue>()});
    suggestIndex_ = indexName;
}

void QueryExpression::InKeys(const std::set<Key> &keys)
{
    if (useFromTable_) {
        expressions_[fromTable_].InKeys(keys);
        validStatus_ = -E_NOT_SUPPORT;
        return;
    }
    SetNotSupportIfFromTables();
    queryInfo_.emplace_front(QueryObjNode{QueryObjType::IN_KEYS, std::string(), QueryValueType::VALUE_TYPE_NULL,
        std::vector<FieldValue>()});
    keys_ = keys;
}

const std::list<QueryObjNode> &QueryExpression::GetQueryExpression()
{
    if (!GetErrFlag()) {
        queryInfo_.clear();
        queryInfo_.emplace_back(QueryObjNode{QueryObjType::OPER_ILLEGAL});
        LOGE("Query operate illegal!");
    }
    return queryInfo_;
}

std::vector<uint8_t> QueryExpression::GetBeginKey() const
{
    return beginKey_;
}

std::vector<uint8_t> QueryExpression::GetEndKey() const
{
    return endKey_;
}

std::vector<uint8_t> QueryExpression::GetPreFixKey() const
{
    return prefixKey_;
}

void QueryExpression::SetTableName(const std::string &tableName)
{
    tableName_ = tableName;
    isTableNameSpecified_ = true;
}

const std::string &QueryExpression::GetTableName()
{
    return tableName_;
}

bool QueryExpression::IsTableNameSpecified() const
{
    return isTableNameSpecified_;
}

std::string QueryExpression::GetSuggestIndex() const
{
    return suggestIndex_;
}

const std::set<Key> &QueryExpression::GetKeys() const
{
    return keys_;
}

void QueryExpression::BeginGroup()
{
    if (useFromTable_) {
        expressions_[fromTable_].BeginGroup();
        return;
    }
    SetNotSupportIfFromTables();
    queryInfo_.emplace_back(QueryObjNode{QueryObjType::BEGIN_GROUP, std::string(),
        QueryValueType::VALUE_TYPE_NULL, std::vector<FieldValue>()});
}

void QueryExpression::EndGroup()
{
    if (useFromTable_) {
        expressions_[fromTable_].EndGroup();
        return;
    }
    SetNotSupportIfFromTables();
    queryInfo_.emplace_back(QueryObjNode{QueryObjType::END_GROUP, std::string(),
        QueryValueType::VALUE_TYPE_NULL, std::vector<FieldValue>()});
}

void QueryExpression::Reset()
{
    errFlag_ = true;
    queryInfo_.clear();
    prefixKey_.clear();
    prefixKey_.shrink_to_fit();
    suggestIndex_.clear();
    keys_.clear();
}

void QueryExpression::SetErrFlag(bool flag)
{
    errFlag_ = flag;
}

bool QueryExpression::GetErrFlag()
{
    return errFlag_;
}

int QueryExpression::GetSortType() const
{
    return sortType_;
}

void QueryExpression::SetSortType(bool isAsc)
{
    if (useFromTable_) {
        expressions_[fromTable_].SetSortType(isAsc);
        validStatus_ = -E_NOT_SUPPORT;
        return;
    }
    SetNotSupportIfFromTables();
    WriteTimeSort sortType = isAsc ? WriteTimeSort::TIMESTAMP_ASC : WriteTimeSort::TIMESTAMP_DESC;
    sortType_ = static_cast<int>(sortType);
}

std::vector<std::string> QueryExpression::GetTables()
{
    return tables_;
}

void QueryExpression::SetTables(const std::vector<std::string> &tableNames)
{
    // filter same table
    std::vector<std::string> syncTable;
    std::set<std::string> addTable;
    for (const auto &table: tableNames) {
        if (addTable.find(table) == addTable.end()) {
            addTable.insert(table);
            syncTable.push_back(table);
        }
    }
    tables_ = syncTable;
    if (useFromTable_) {
        validStatus_ = validStatus_ != E_OK ? validStatus_ : -E_NOT_SUPPORT;
    }
    SetNotSupportIfCondition();
}

void QueryExpression::From(const std::string &tableName)
{
    useFromTable_ = true;
    fromTable_ = tableName;
    for (const auto &item: tableSequence_) {
        if (item == tableName) {
            return;
        }
    }
    tableSequence_.push_back(fromTable_);
    expressions_[fromTable_].SetTableName(fromTable_);
    if (tableName.empty()) {
        validStatus_ = validStatus_ != E_OK ? validStatus_ : -E_INVALID_ARGS;
    } else if (!tables_.empty()) {
        validStatus_ = validStatus_ != E_OK ? validStatus_ : -E_NOT_SUPPORT;
    }
    SetNotSupportIfFromTables();
    SetNotSupportIfCondition();
}

int QueryExpression::GetExpressionStatus() const
{
    return validStatus_;
}

std::vector<QueryExpression> QueryExpression::GetQueryExpressions() const
{
    if (!useFromTable_) {
        return {};
    }
    std::vector<QueryExpression> res;
    for (const auto &item : tableSequence_) {
        res.push_back(expressions_.at(item));
    }
    return res;
}

void QueryExpression::SetNotSupportIfFromTables()
{
    if (validStatus_ != E_OK) {
        return;
    }
    if (!tables_.empty()) {
        validStatus_ = -E_NOT_SUPPORT;
    }
}

void QueryExpression::SetNotSupportIfCondition()
{
    if (validStatus_ != E_OK) {
        return;
    }
    if (!queryInfo_.empty()) {
        validStatus_ = -E_NOT_SUPPORT;
    }
}

void QueryExpression::SetNotSupportIfNeed(QueryObjType type)
{
    if (validStatus_ != E_OK) {
        return;
    }
    if (type != QueryObjType::IN && type != QueryObjType::EQUALTO && type != QueryObjType::AND &&
        type != QueryObjType::OR) {
        validStatus_ = -E_NOT_SUPPORT;
    }
    if (validStatus_ != E_OK) {
        return;
    }
    for (const auto &item: queryInfo_) {
        if (((item.operFlag == QueryObjType::EQUALTO) && (type == QueryObjType::IN)) ||
            ((item.operFlag == QueryObjType::IN) && (type == QueryObjType::EQUALTO))) {
            validStatus_ = -E_NOT_SUPPORT;
            LOGW("[Query] Not support use in and equal to at same time when use from table");
            break;
        }
    }
}

int QueryExpression::RangeParamCheck() const
{
    if (queryInfo_.size() > 1) { // Only Support one query filter.
        return -E_INVALID_ARGS;
    }
    for (const auto &queryObjNode : queryInfo_) {
        if (queryObjNode.operFlag != QueryObjType::KEY_RANGE) {
            return -E_INVALID_ARGS;
        }
    }
    if (this->beginKey_.size() > DBConstant::MAX_KEY_SIZE ||
        this->endKey_.size() > DBConstant::MAX_KEY_SIZE) {
        return -E_INVALID_ARGS;
    }
    return E_OK;
}
} // namespace DistributedDB
