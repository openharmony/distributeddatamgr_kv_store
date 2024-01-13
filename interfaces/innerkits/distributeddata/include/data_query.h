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

#ifndef DISTRIBUTED_DATA_QUERY_H
#define DISTRIBUTED_DATA_QUERY_H

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include "visibility.h"
namespace DistributedDB {
class Query;
}
namespace OHOS {
namespace DistributedKv {
class API_EXPORT DataQuery {
public:
    /**
     * @brief Constructor.
     */
    DataQuery();

    /**
     * @brief Destructor.
     */
    ~DataQuery() = default;

    /**
     * @brief Reset the query.
     * @return This query.
    */
    DataQuery &Reset();

    /**
     * @brief Equal to int value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &EqualTo(const std::string &field, const int value);

    /**
     * @brief Equal to long value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &EqualTo(const std::string &field, const int64_t value);

    /**
     * @brief Equal to double value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &EqualTo(const std::string &field, const double value);

    /**
     * @brief Equal to string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &EqualTo(const std::string &field, const std::string &value);

    /**
     * @brief Equal to boolean value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &EqualTo(const std::string &field, const bool value);

    /**
     * @brief Not equal to int value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &NotEqualTo(const std::string &field, const int value);

    /**
     * @brief Not equal to long value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &NotEqualTo(const std::string &field, const int64_t value);

    /**
     * @brief Not equal to double value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &NotEqualTo(const std::string &field, const double value);

    /**
     * @brief Not equal to string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &NotEqualTo(const std::string &field, const std::string &value);

    /**
     * @brief Not equal to boolean value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &NotEqualTo(const std::string &field, const bool value);

    /**
     * @brief Greater than int value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThan(const std::string &field, const int value);

    /**
     * @brief Greater than long value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThan(const std::string &field, const int64_t value);

    /**
     * @brief Greater than double value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThan(const std::string &field, const double value);

    /**
     * @brief Greater than string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThan(const std::string &field, const std::string &value);

    /**
     * @brief Less than int value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThan(const std::string &field, const int value);

    /**
     * @brief Less than long value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThan(const std::string &field, const int64_t value);

    /**
     * @brief Less than double value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThan(const std::string &field, const double value);

    /**
     * @brief Less than string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThan(const std::string &field, const std::string &value);

    /**
     * @brief Greater than or equal to int value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThanOrEqualTo(const std::string &field, const int value);

    /**
     * @brief Greater than or equal to long value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThanOrEqualTo(const std::string &field, const int64_t value);

    /**
     * @brief Greater than or equal to double value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThanOrEqualTo(const std::string &field, const double value);

    /**
     * @brief Greater than or equal to string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &GreaterThanOrEqualTo(const std::string &field, const std::string &value);

    /**
     * @brief Less than or equal to int value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThanOrEqualTo(const std::string &field, const int value);

    /**
     * @brief Less than or equal to long value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThanOrEqualTo(const std::string &field, const int64_t value);

    /**
     * @brief Less than or equal to double value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThanOrEqualTo(const std::string &field, const double value);

    /**
     * @brief Less than or equal to string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &LessThanOrEqualTo(const std::string &field, const std::string &value);

    /**
     * @brief Is null field value.
     * @param field The field name.
     * @return This query
    */
    DataQuery &IsNull(const std::string &field);

    /**
     * @brief Greater than or equal to small value and less than or equal to large value.
     * @param field The small value.
     * @param value The large value.
     * @return This query
    */
    DataQuery& Between(const std::string& valueLow, const std::string& valueHigh);

    /**
     * @brief Is not null field value.
     * @param field The field name.
     * @return This query
    */
    DataQuery &IsNotNull(const std::string &field);

    /**
     * @brief In int value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &In(const std::string &field, const std::vector<int> &valueList);

    /**
     * @brief In long value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &In(const std::string &field, const std::vector<int64_t> &valueList);

    /**
     * @brief In double value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &In(const std::string &field, const std::vector<double> &valueList);

    /**
     * @brief In string value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &In(const std::string &field, const std::vector<std::string> &valueList);

    /**
     * @brief Not in int value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &NotIn(const std::string &field, const std::vector<int> &valueList);

    /**
     * @brief Not in long value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &NotIn(const std::string &field, const std::vector<int64_t> &valueList);

    /**
     * @brief Not in double value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &NotIn(const std::string &field, const std::vector<double> &valueList);

    /**
     * @brief Not in string value list.
     * @param field The field name.
     * @param valueList The field value list.
     * @return This query
    */
    DataQuery &NotIn(const std::string &field, const std::vector<std::string> &valueList);

    /**
     * @brief Like string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &Like(const std::string &field, const std::string &value);

    /**
     * @brief Unlike string value.
     * @param field The field name.
     * @param value The field value.
     * @return This query
    */
    DataQuery &Unlike(const std::string &field, const std::string &value);

    /**
     * @brief And operator.
     * @return This query
    */
    DataQuery &And();

    /**
     * @brief Or operator.
     * @return This query
    */
    DataQuery &Or();

    /**
     * @brief Order by ascent.
     * @param field The field name.
     * @return This query
    */
    DataQuery &OrderByAsc(const std::string &field);

    /**
     * @brief Order by descent.
     * @param field The field name.
     * @return This query
    */
    DataQuery &OrderByDesc(const std::string &field);

    /**
     * @brief Order by write time.
     * @param isAsc Is ascent.
     * @return This query
    */
    DataQuery &OrderByWriteTime(bool isAsc);

    /**
     * @brief Limit result size.
     * @param number The number of results.
     * @param offset Start position.
     * @return This query
    */
    DataQuery &Limit(const int number, const int offset);

    /**
     * @brief Begin group.
     * @return This query
    */
    DataQuery &BeginGroup();

    /**
     * @brief End group.
     * @return This query
    */
    DataQuery &EndGroup();

    /**
     * @brief Select results with specified key prefix.
     * @param prefix key prefix.
     * @return This query
    */
    DataQuery &KeyPrefix(const std::string &prefix);

    /**
     * @brief Select results with specified device Identifier.
     * @param deviceId Device Identifier.
     * @return This query
    */
    DataQuery &DeviceId(const std::string &deviceId);

    /**
     * @brief Select results with suggested index.
     * @param index Suggested index.
     * @return This query
    */
    DataQuery &SetSuggestIndex(const std::string &index);

    /**
     * @brief Select results with many keys.
     * @param keys The vector of keys for query.
     * @return This query
    */
    DataQuery &InKeys(const std::vector<std::string> &keys);

    /**
     * @brief Get string representation
     * @return String representation of this query.
    */
    std::string ToString() const;

private:

    friend class QueryHelper;
    friend class DeviceConvertor;
    friend class Convertor;

    /**
     * @brief equal to
    */
    static const char * const EQUAL_TO;

    /**
     * @brief not equal to
    */
    static const char * const NOT_EQUAL_TO;

    /**
     * @brief greater than
    */
    static const char * const GREATER_THAN;

    /**
     * @brief less than
    */
    static const char * const LESS_THAN;

    /**
     * @brief greater than or equal to
    */
    static const char * const GREATER_THAN_OR_EQUAL_TO;

    /**
     * @brief less than or equal to
    */
    static const char * const LESS_THAN_OR_EQUAL_TO;

    /**
     * @brief is null
    */
    static const char * const IS_NULL;

    /**
     * @brief in
    */
    static const char * const IN;

    /**
     * @brief not in
    */
    static const char * const NOT_IN;

    /**
     * @brief like
    */
    static const char * const LIKE;

    /**
     * @brief not like
    */
    static const char * const NOT_LIKE;

    /**
     * @brief and
    */
    static const char * const AND;

    /**
     * @brief or
    */
    static const char * const OR;

    /**
     * @brief order by asc
    */
    static const char * const ORDER_BY_ASC;

    /**
     * @brief order by desc
    */
    static const char * const ORDER_BY_DESC;

    /**
     * @brief order by write time
    */
    static const char * const ORDER_BY_WRITE_TIME;

    /**
     * @brief order by write time asc
    */
    static const char * const IS_ASC;

    /**
     * @brief order by write time desc
    */
    static const char * const IS_DESC;

    /**
     * @brief limit
    */
    static const char * const LIMIT;

    /**
     * @brief space
    */
    static const char * const SPACE;

    /**
     * @brief '^'
    */
    static const char * const SPECIAL;

    /**
     * @brief '^' escape
    */
    static const char * const SPECIAL_ESCAPE;

    /**
     * @brief space escape
    */
    static const char * const SPACE_ESCAPE;

    /**
     * @brief empty string
    */
    static const char * const EMPTY_STRING;

    /**
     * @brief start in
    */
    static const char * const START_IN;

    /**
     * @brief end in
    */
    static const char * const END_IN;

    /**
     * @brief begin group
    */
    static const char * const BEGIN_GROUP;

    /**
     * @brief end group
    */
    static const char * const END_GROUP;

    /**
     * @brief key prefix
    */
    static const char * const KEY_PREFIX;

    /**
     * @brief device id
    */
    static const char * const DEVICE_ID;

    /**
     * @brief is not null
    */
    static const char * const IS_NOT_NULL;

    /**
     * @brief type string
    */
    static const char * const TYPE_STRING;

    /**
     * @brief type integer
    */
    static const char * const TYPE_INTEGER;

    /**
     * @brief type long
    */
    static const char * const TYPE_LONG;

    /**
     * @brief type double
    */
    static const char * const TYPE_DOUBLE;

    /**
     * @brief type boolean
    */
    static const char * const TYPE_BOOLEAN;

    /**
     * @brief value true
    */
    static const char * const VALUE_TRUE;

    /**
     * @brief value false
    */
    static const char * const VALUE_FALSE;

    /**
     * @brief suggested index
    */
    static const char * const SUGGEST_INDEX;

    /**
     * @brief in keys
    */
    static const char * const IN_KEYS;

    std::string str_;

    bool hasKeys_ = false;
    bool hasPrefix_ = false;
    std::shared_ptr<DistributedDB::Query> query_;
    std::string deviceId_;
    std::string prefix_;
    std::vector<std::string> keys_;

    template<typename T>
    void AppendCommon(const std::string &keyword, const std::string &fieldType, std::string &field, const T &value);

    void AppendCommonString(const std::string &keyword, const std::string &fieldType, std::string &field,
                            std::string &value);

    void AppendCommonBoolean(const std::string &keyword, const std::string &fieldType, std::string &field,
                             const bool &value);

    void AppendCommonString(const std::string &keyword, std::string &field, std::string &value);

    template<typename T>
    void AppendCommonList(const std::string &keyword, const std::string &fieldType, std::string &field,
                          const std::vector<T> &valueList);

    void AppendCommonListString(const std::string &keyword, const std::string &fieldType, std::string &field,
                                std::vector<std::string> &valueList);

    void EscapeSpace(std::string &input);

    bool ValidateField(const std::string &field);

    template<typename T>
    std::string BasicToString(const T &value);
};
}  // namespace DistributedKv
}  // namespace OHOS

#endif  // DISTRIBUTED_DATA_QUERY_H
