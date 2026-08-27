/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DATASHARE_OPERATION_STATEMENT_H
#define DATASHARE_OPERATION_STATEMENT_H

#include "datashare_predicates.h"
#include "datashare_values_bucket.h"

namespace OHOS {
namespace DataShare {
enum class Operation : int32_t {
    INSERT = 0,
    UPDATE,
    DELETE,
};

struct UpdateOperation {
    DataShareValuesBucket valuesBucket;
    DataSharePredicates predicates;
};
using UpdateOperations = std::map<std::string, std::vector<UpdateOperation>>;

struct BatchUpdateResult {
    std::string uri;
    std::vector<int> codes;
};

class BackReference {
public:
    explicit BackReference(std::string col = "", int32_t index = INVALID_INDEX)
        : column_(std::move(col)), fromIndex_(index) {};

    bool IsValid() const
    {
        return fromIndex_ != INVALID_INDEX && !column_.empty();
    }
    // Setter for column
    void SetColumn(const std::string& col)
    {
        column_ = col;
    }
    // Setter for fromIndex
    void SetFromIndex(int32_t index)
    {
        fromIndex_ = index;
    }

    const std::string& GetColumn() const
    {
        return column_;
    }

    int32_t GetFromIndex() const
    {
        return fromIndex_;
    }
private:
    static const int32_t INVALID_INDEX = -1;
    std::string column_;     // column name
    int32_t fromIndex_;          // the index indicating which historical operation's result should overwrite the value
};

struct OperationStatement {
    Operation operationType;
    std::string uri;
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    BackReference backReference;

    bool HasBackReference() const
    {
        return backReference.IsValid();
    }

    bool IsOperationTypeValid() const
    {
        return operationType >= Operation::INSERT && operationType <= Operation::DELETE;
    }
};

enum ExecErrorCode : int32_t {
    EXEC_SUCCESS = 0,
    EXEC_FAILED,
    EXEC_PARTIAL_SUCCESS,
};

struct ExecResult {
    Operation operationType;
    int code;
    std::string message;

    bool IsOperationTypeValid() const
    {
        return operationType >= Operation::INSERT && operationType <= Operation::DELETE;
    }
};

struct ExecResultSet {
    ExecErrorCode errorCode;
    std::vector<ExecResult> results;

    bool IsErrorCodeValid() const
    {
        return errorCode >= ExecErrorCode::EXEC_SUCCESS && errorCode <= ExecErrorCode::EXEC_PARTIAL_SUCCESS;
    }
};

struct RegisterOption {
    bool isReconnect;
};

}
}
#endif // DATASHARE_OPERATION_STATEMENT_H
