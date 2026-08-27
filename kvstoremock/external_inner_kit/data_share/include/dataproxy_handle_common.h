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

#ifndef DATASHARE_PROXY_COMMON_H
#define DATASHARE_PROXY_COMMON_H

#include "datashare_observer.h"

namespace OHOS {
namespace DataShare {
static constexpr int32_t URI_MAX_SIZE = 256;
static constexpr int32_t APPIDENTIFIER_MAX_SIZE = 128;
static constexpr int32_t URI_MAX_COUNT = 64;
static constexpr int32_t PROXY_DATA_MAX_COUNT = 64;
static constexpr int32_t ALLOW_LIST_MAX_COUNT = 256;
static constexpr size_t MAX_MULTIVALUE_COUNT_PER_APP = 10;
static constexpr uint32_t MAX_SINGLE_MULTIVALUE_LENGTH = 4096;
constexpr const char* ALLOW_ALL = "all";
constexpr const char* DATA_PROXY_SCHEME = "datashareproxy://";

enum DataProxyErrorCode {
    SUCCESS = 0,
    URI_NOT_EXIST,
    NO_PERMISSION,
    OVER_LIMIT,
    INNER_ERROR,
    INCOMPATIBLE_CONFIG_TYPE,
    KEY_NOT_EXIST,
};

/**
 * @brief DataProxy Value Type .
 */
enum DataProxyValueType : int32_t {
    /** DataProxy Value Types is int.*/
    VALUE_INT = 0,
    /** DataProxy Value Types is double.*/
    VALUE_DOUBLE,
    /** DataProxy Value Types is string.*/
    VALUE_STRING,
    /** DataProxy Value Types is bool.*/
    VALUE_BOOL,
};

using DataProxyValue = std::variant<int64_t, double, std::string, bool>;

enum DataProxyType {
    SHARED_CONFIG = 0,
};

enum DataProxyMaxValueLength : int32_t {
    MAX_LENGTH_4K = 4096,
    MAX_LENGTH_100K = 102400,
};

struct DataProxyConfig {
    DataProxyType type_ = DataProxyType::SHARED_CONFIG;
    DataProxyMaxValueLength maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;

    bool operator==(const DataProxyConfig& other) const
    {
        return (type_ == other.type_) && (maxValueLength_ == other.maxValueLength_);
    }
};

struct DataShareProxyData {
    /**
     * @brief Constructor.
     */
    DataShareProxyData() = default;

    /**
     * @brief Destructor.
     */
    ~DataShareProxyData() = default;
    
    DataShareProxyData(const std::string &uri, const DataProxyValue &value,
        const std::vector<std::string> &allowList = {}) : uri_(uri), value_(value), allowList_(allowList) {}

    std::string uri_;
    DataProxyValue value_ = "";
    std::vector<std::string> allowList_;
    std::vector<std::string> trustProviders_;
    std::map<std::string, std::map<std::string, DataProxyValue>> multiValues_;
    DataProxyMaxValueLength maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;
    bool isValueUndefined = false;
    bool isAllowListUndefined = false;
    bool isTrustProvidersUndefined = false;
    bool isMultiValues_ = false;
};

struct DataProxyResult {
    DataProxyResult() = default;
    DataProxyResult(const std::string &uri, const DataProxyErrorCode &result) : uri_(uri), result_(result) {}
    std::string uri_;
    DataProxyErrorCode result_;
};

struct DataProxyGetResult {
    DataProxyGetResult() = default;
    DataProxyGetResult(const std::string &uri, const DataProxyErrorCode &result,
        const DataProxyValue &value = {}, const std::vector<std::string> allowList = {})
        : uri_(uri), result_(result), value_(value), allowList_(allowList) {}
    std::string uri_;
    DataProxyErrorCode result_;
    DataProxyValue value_;
    std::vector<std::string> allowList_;
    std::vector<DataProxyValue> multiValues_;
};

struct DataProxyChangeInfo {
    DataProxyChangeInfo() = default;
    DataProxyChangeInfo(const DataShareObserver::ChangeType &changeType,
        const std::string &uri, const DataProxyValue &value, bool isMultiValues = false)
        : changeType_(changeType), uri_(uri), value_(value), isMultiValues_(isMultiValues) {}
    DataShareObserver::ChangeType changeType_ = DataShareObserver::INVAILD;
    std::string uri_;
    DataProxyValue value_;
    bool isMultiValues_ = false;
    std::vector<DataProxyValue> multiValues_;
};
} // namespace DataShare
} // namespace OHOS
#endif // DATASHARE_PROXY_COMMON_H