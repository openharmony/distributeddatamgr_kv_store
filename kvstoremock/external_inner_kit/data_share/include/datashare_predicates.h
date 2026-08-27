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

#ifndef DATASHARE_PREDICATES_H
#define DATASHARE_PREDICATES_H

#include <string>

#include "datashare_abs_predicates.h"
#include "datashare_errno.h"
#include "datashare_predicates_object.h"
#include "datashare_predicates_objects.h"
#include "message_parcel.h"

namespace OHOS {
namespace DataShare {
class DataSharePredicates : public DataShareAbsPredicates {
public:

    /**
     * @brief Constructor.
     */
    DataSharePredicates()
    {
    }

    /**
     * @brief Constructor.
     *
     * A parameterized constructor used to create an DataSharePredicates instance.
     *
     * @param OperationList Indicates the operation list of the database.
     */
    explicit DataSharePredicates(std::vector<OperationItem> operList) : operations_(std::move(operList))
    {
    }

    /**
     * @brief Destructor.
     */
    ~DataSharePredicates()
    {
    }

    /**
     * @brief The EqualTo of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *EqualTo(const std::string &field, const SingleValue &value)
    {
        SetOperationList(EQUAL_TO, field, value);
        return this;
    }

    /**
     * @brief The NotEqualTo of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *NotEqualTo(const std::string &field, const SingleValue &value)
    {
        SetOperationList(NOT_EQUAL_TO, field, value);
        return this;
    }

    /**
     * @brief The GreaterThan of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *GreaterThan(const std::string &field, const SingleValue &value)
    {
        SetOperationList(GREATER_THAN, field, value);
        return this;
    }

    /**
     * @brief The LessThan of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *LessThan(const std::string &field, const SingleValue &value)
    {
        SetOperationList(LESS_THAN, field, value);
        return this;
    }

    /**
     * @brief The GreaterThanOrEqualTo of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *GreaterThanOrEqualTo(const std::string &field, const SingleValue &value)
    {
        SetOperationList(GREATER_THAN_OR_EQUAL_TO, field, value);
        return this;
    }

    /**
     * @brief The LessThanOrEqualTo of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *LessThanOrEqualTo(const std::string &field, const SingleValue &value)
    {
        SetOperationList(LESS_THAN_OR_EQUAL_TO, field, value);
        return this;
    }

    /**
     * @brief The In of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *In(const std::string &field, const MutliValue &values)
    {
        SetOperationList(SQL_IN, field, values);
        return this;
    }

    /**
     * @brief The NotIn of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *NotIn(const std::string &field, const MutliValue &values)
    {
        SetOperationList(NOT_IN, field, values);
        return this;
    }

    /**
     * @brief BeginWrap.
     */
    DataSharePredicates *BeginWrap()
    {
        SetOperationList(BEGIN_WARP);
        return this;
    }

    /**
     * @brief EndWrap.
     */
    DataSharePredicates *EndWrap()
    {
        SetOperationList(END_WARP);
        return this;
    }

    /**
     * @brief Or.
     */
    DataSharePredicates *Or()
    {
        SetOperationList(OR);
        return this;
    }

    /**
     * @brief And.
     */
    DataSharePredicates *And()
    {
        SetOperationList(AND);
        return this;
    }

    /**
     * @brief The Contains of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *Contains(const std::string &field, const std::string &value)
    {
        SetOperationList(CONTAINS, field, value);
        return this;
    }

    /**
     * @brief The BeginsWith of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *BeginsWith(const std::string &field, const std::string &value)
    {
        SetOperationList(BEGIN_WITH, field, value);
        return this;
    }

    /**
     * @brief The EndsWith of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *EndsWith(const std::string &field, const std::string &value)
    {
        SetOperationList(END_WITH, field, value);
        return this;
    }

    /**
     * @brief The IsNull of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *IsNull(const std::string &field)
    {
        SetOperationList(IS_NULL, field);
        return this;
    }

    /**
     * @brief The IsNotNull of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *IsNotNull(const std::string &field)
    {
        SetOperationList(IS_NOT_NULL, field);
        return this;
    }

    /**
     * @brief The Like of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *Like(const std::string &field, const std::string &value)
    {
        SetOperationList(LIKE, field, value);
        return this;
    }

    /**
     * @brief The Unlike of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *Unlike(const std::string &field, const std::string &value)
    {
        SetOperationList(UNLIKE, field, value);
        return this;
    }

    /**
     * @brief The Glob of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *Glob(const std::string &field, const std::string &value)
    {
        SetOperationList(GLOB, field, value);
        return this;
    }

    /**
     * @brief The Between of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *Between(const std::string &field, const std::string &low, const std::string &high)
    {
        SetOperationList(BETWEEN, field, low, high);
        return this;
    }

    /**
     * @brief The NotBetween of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *NotBetween(const std::string &field, const std::string &low, const std::string &high)
    {
        SetOperationList(NOTBETWEEN, field, low, high);
        return this;
    }

    /**
     * @brief The OrderByAsc of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *OrderByAsc(const std::string &field)
    {
        SetOperationList(ORDER_BY_ASC, field);
        return this;
    }

    /**
     * @brief The OrderByDesc of the predicate.
     *
     * @param field Indicates the target field.
     * @param value Indicates the queried value.
     */
    DataSharePredicates *OrderByDesc(const std::string &field)
    {
        SetOperationList(ORDER_BY_DESC, field);
        return this;
    }

    /**
     * @brief Distinct predicate condition.
     */
    DataSharePredicates *Distinct()
    {
        SetOperationList(DISTINCT);
        return this;
    }

    /**
     * @brief The Limit of the predicate.
     *
     * @param number Indicates the target number.
     * @param offset Indicates the queried value.
     */
    DataSharePredicates *Limit(const int number, const int offset)
    {
        SetOperationList(LIMIT, number, offset);
        return this;
    }

    /**
     * @brief The GroupBy of the predicate.
     *
     * @param field Indicates the target field.
     */
    DataSharePredicates *GroupBy(const std::vector<std::string> &fields)
    {
        SetOperationList(GROUP_BY, fields);
        return this;
    }

    /**
     * @brief The IndexedBy of the predicate.
     *
     * @param indexName indicates the query condition.
     */
    DataSharePredicates *IndexedBy(const std::string &indexName)
    {
        SetOperationList(INDEXED_BY, indexName);
        return this;
    }

    /**
     * @brief The KeyPrefix of the predicate.
     *
     * @param Search by prefix conditions.
     */
    DataSharePredicates *KeyPrefix(const std::string &prefix)
    {
        SetOperationList(KEY_PREFIX, prefix);
        return this;
    }

    /**
     * @brief The InKeys of the predicate.
     *
     * @param Query based on key conditions.
     */
    DataSharePredicates *InKeys(const std::vector<std::string> &keys)
    {
        SetOperationList(IN_KEY, keys);
        return this;
    }

    /**
     * @brief The CrossJoin of the predicate.
     *
     * @param tableName indicates the query condition.
     */
    DataSharePredicates *CrossJoin(const std::string &tableName)
    {
        SetOperationList(CROSSJOIN, tableName);
        return this;
    }

    /**
     * @brief The InnerJoin of the predicate.
     *
     * @param tableName indicates the query condition.
     */
    DataSharePredicates *InnerJoin(const std::string &tableName)
    {
        SetOperationList(INNERJOIN, tableName);
        return this;
    }

    /**
     * @brief The LeftOuterJoin of the predicate.
     *
     * @param tableName indicates the query condition.
     */
    DataSharePredicates *LeftOuterJoin(const std::string &tableName)
    {
        SetOperationList(LEFTOUTERJOIN, tableName);
        return this;
    }

    /**
     * @brief The Using of the predicate.
     *
     * @param field Indicates the target field.
     */
    DataSharePredicates *Using(const std::vector<std::string> &fields)
    {
        SetOperationList(USING, fields);
        return this;
    }

    /**
     * @brief The On of the predicate.
     *
     * @param field Indicates the target field.
     */
    DataSharePredicates *On(const std::vector<std::string> &fields)
    {
        SetOperationList(ON, fields);
        return this;
    }

    /**
     * @brief The GetOperationList of the predicate.
     */
    const std::vector<OperationItem> &GetOperationList() const
    {
        return operations_;
    }

    /**
     * @brief Used in the deserialization function to assign values to operations,
     * Directly assignsing values to operations with Different usage from private methods.
     * reference to the SetWhereClause, SetWhereArgs, SetOrder.
     */
    int SetOperationList(std::vector<OperationItem> operations)
    {
        if ((settingMode_ != PREDICATES_METHOD) && (!operations.empty())) {
            this->operations_ = operations;
            settingMode_ = QUERY_LANGUAGE;
            return E_OK;
        }
        return E_ERROR;
    }

    /**
     * @brief The GetWhereClause of the predicate.
     */
    std::string GetWhereClause() const
    {
        return whereClause_;
    }

    /**
     * @brief The SetWhereClause of the predicate.
     *
     * @param Query based on the whereClause.
     */
    int SetWhereClause(const std::string &whereClause)
    {
        if ((settingMode_ != PREDICATES_METHOD) && (!whereClause.empty())) {
            this->whereClause_ = whereClause;
            settingMode_ = QUERY_LANGUAGE;
            return E_OK;
        }
        return E_ERROR;
    }

    /**
     * @brief The GetWhereArgs of the predicate.
     */
    std::vector<std::string> GetWhereArgs() const
    {
        return whereArgs_;
    }

    /**
     * @brief The SetWhereArgs of the predicate.
     *
     * @param Query based on whereArgs conditions.
     */
    int SetWhereArgs(const std::vector<std::string> &whereArgs)
    {
        if ((settingMode_ != PREDICATES_METHOD) && (!whereArgs.empty())) {
            if (!whereArgs.empty()) {
                this->whereArgs_ = whereArgs;
                settingMode_ = QUERY_LANGUAGE;
                return E_OK;
            }
        }
        return E_ERROR;
    }

    /**
     * @brief The GetOrder of the predicate.
     */
    std::string GetOrder() const
    {
        return order_;
    }

    /**
     * @brief The SetOrder of the predicate.
     *
     * @param Query based on order conditions..
     */
    int SetOrder(const std::string &order)
    {
        if ((settingMode_ != PREDICATES_METHOD) && (!order.empty())) {
            this->order_ = order;
            settingMode_ = QUERY_LANGUAGE;
            return E_OK;
        }
        return E_ERROR;
    }

    /**
     * @brief The GetSettingMode of the predicate.
     */
    int16_t GetSettingMode() const
    {
        return settingMode_;
    }

    /**
     * @brief The SetSettingMode of the predicate.
     */
    void SetSettingMode(int16_t settingMode)
    {
        settingMode_ = settingMode;
    }

    /**
     * @brief The following four functions are used for serializing and deserializing objects
     * to and from shared memory during Query and BatchInsert operations,
     * which has a 128M upper limit. The upper limit of other method is 200k.
     * Other methods remain unchanged.
     */
    static bool Marshal(const DataSharePredicates &predicates, MessageParcel &parcel);

    static bool Unmarshal(DataSharePredicates &predicates, MessageParcel &parcel);

private:
    void SetOperationList(OperationType operationType, const MutliValue &param)
    {
        OperationItem operationItem {};
        operationItem.operation = operationType;
        operationItem.multiParams.push_back(param.value);
        operations_.push_back(operationItem);
        if (settingMode_ != PREDICATES_METHOD) {
            ClearQueryLanguage();
            settingMode_ = PREDICATES_METHOD;
        }
    }
    void SetOperationList(
        OperationType operationType, const SingleValue &param1, const MutliValue &param2)
    {
        OperationItem operationItem {};
        operationItem.operation = operationType;
        operationItem.singleParams.push_back(param1.value);
        operationItem.multiParams.push_back(param2.value);
        operations_.push_back(operationItem);
        if (settingMode_ != PREDICATES_METHOD) {
            ClearQueryLanguage();
            settingMode_ = PREDICATES_METHOD;
        }
    }
    void SetOperationList(OperationType operationType, const SingleValue &para1 = {},
        const SingleValue &para2 = {}, const SingleValue &para3 = {})
    {
        OperationItem operationItem {};
        operationItem.operation = operationType;
        operationItem.singleParams.push_back(para1.value);
        operationItem.singleParams.push_back(para2.value);
        operationItem.singleParams.push_back(para3.value);
        operations_.push_back(operationItem);
        if (settingMode_ != PREDICATES_METHOD) {
            ClearQueryLanguage();
            settingMode_ = PREDICATES_METHOD;
        }
    }
    void ClearQueryLanguage()
    {
        whereClause_ = "";
        whereArgs_ = {};
        order_ = "";
    }
    std::vector<OperationItem> operations_;
    std::string whereClause_;
    std::vector<std::string> whereArgs_;
    std::string order_;
    int16_t settingMode_ = {};
};
} // namespace DataShare
} // namespace OHOS
#endif