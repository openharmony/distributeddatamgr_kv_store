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

#define LOG_TAG "KvStoreDataShareBridge"

#include <endian.h>
#include <securec.h>

#include "log_print.h"
#include "kvstore_datashare_bridge.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
static constexpr int32_t KEY_COLUMN_INDEX = 0;
static constexpr int32_t VALUE_COLUMN_INDEX = 1;
KvStoreDataShareBridge::KvStoreDataShareBridge(std::shared_ptr<KvStoreResultSet> kvResultSet)
    :kvResultSet_(kvResultSet) {};

int KvStoreDataShareBridge::GetRowCount(int32_t &count)
{
    count = Count();
    return count == INVALID_COUNT ? E_ERROR : E_OK;
}

int KvStoreDataShareBridge::GetAllColumnNames(std::vector<std::string> &columnsName)
{
    columnsName = { "key", "value" };
    return E_OK;
}

bool KvStoreDataShareBridge::FillBlock(int pos, ResultSetBridge::Writer &writer)
{
    if (kvResultSet_ == nullptr) {
        ZLOGE("This kvResultSet_ is nullptr");
        return false;
    }
    bool isMoved = kvResultSet_->MoveToPosition(pos);
    if (!isMoved) {
        ZLOGE("MoveToPosition failed");
        return false;
    }
    Entry entry;
    Status status = kvResultSet_->GetEntry(entry);
    if (status != Status::SUCCESS) {
        ZLOGE("GetEntry failed %{public}d", status);
        return false;
    }
    int statusAlloc = writer.AllocRow();
    if (statusAlloc != E_OK) {
        ZLOGE("SharedBlock is full: %{public}d", statusAlloc);
        return false;
    }
    int keyStatus = writer.Write(KEY_COLUMN_INDEX, entry.key.ToString().c_str(), entry.key.Size() + 1);
    if (keyStatus != E_OK) {
        ZLOGE("WriteBlob key error: %{public}d", keyStatus);
        return false;
    }
    auto blobData = entry.value.Data();
    std::vector<uint8_t> input = blobData;
    KvStoreDataShareBridge::ValueType type = KvStoreDataShareBridge::ValueType::STRING;
    if (!kvResultSet_->IsSchemaStore()) {
        if (blobData.empty()) {
            ZLOGE("blob value size is empty");
            return false;
        }
        // if store is not schema store,number 1 means: skip the first byte, byte[0] is real data type.
        type = static_cast<KvStoreDataShareBridge::ValueType>(blobData[0]);
        input = std::vector(blobData.begin() + 1, blobData.end());
    }
    auto valueStatus = WriteValue(input, writer, type);
    if (valueStatus != E_OK) {
        ZLOGE("Write value error:%{public}d, type:%{public}d", valueStatus, type);
        return false;
    }
    return true;
}

int KvStoreDataShareBridge::Count()
{
    if (kvResultSet_ == nullptr) {
        ZLOGE("This kvResultSet_ is nullptr");
        return INVALID_COUNT;
    }
    if (resultRowCount != INVALID_COUNT) {
        return resultRowCount;
    }
    int count = kvResultSet_->GetCount();
    if (count < 0) {
        ZLOGE("This kvResultSet count is invalid: %{public}d", count);
        return INVALID_COUNT;
    }
    resultRowCount = count;
    return count;
}

int KvStoreDataShareBridge::OnGo(int32_t start, int32_t target, ResultSetBridge::Writer &writer)
{
    if ((start < 0) || (target < 0) || (start > target) || (target >= Count())) {
        ZLOGE("The nowRowIndex out of line: %{public}d", target);
        return -1;
    }
    for (int pos = start; pos <= target; pos++) {
        bool ret = FillBlock(pos, writer);
        if (!ret) {
            ZLOGE("The nowRowIndex out of line: %{public}d %{public}d", pos, target);
            return pos - 1;
        }
    }
    return target;
}

int KvStoreDataShareBridge::WriteValue(std::vector<uint8_t> &input, Writer &writer,
    KvStoreDataShareBridge::ValueType type)
{
    int errcode = E_ERROR;
    switch (type) {
        case KvStoreDataShareBridge::ValueType::INT64: {
            auto [result, value] = VectorToInt64(input);
            errcode = result ? writer.Write(VALUE_COLUMN_INDEX, value) : E_ERROR;
            break;
        }
        case KvStoreDataShareBridge::ValueType::FLOAT:
        case KvStoreDataShareBridge::ValueType::DOUBLE: {
            auto [result, value] = VectorToDouble(input);
            errcode = result ? writer.Write(VALUE_COLUMN_INDEX, value) : E_ERROR;
            break;
        }
        case KvStoreDataShareBridge::ValueType::STRING: {
            std::string value(input.begin(), input.end());
            errcode = writer.Write(VALUE_COLUMN_INDEX, value.c_str(), value.size() + 1);
            break;
        }
        case KvStoreDataShareBridge::ValueType::BOOLEAN:
        case KvStoreDataShareBridge::ValueType::BYTE_ARRAY: {
            errcode = input.empty() ? E_ERROR :
                writer.Write(VALUE_COLUMN_INDEX, &input[0], input.size() * sizeof(uint8_t));
            break;
        }
        default: {
            ZLOGE("unsupported value type:%{public}d", type);
            break;
        }
    }
    return errcode;
}

std::pair<bool, int64_t> KvStoreDataShareBridge::VectorToInt64(const std::vector<uint8_t> &input)
{
    if (input.size() != sizeof(int64_t)) {
        return std::pair(false, 0);
    }
    int64_t value;
    auto ret = memcpy_s(&value, sizeof(value), input.data(), sizeof(value));
    if (ret != EOK) {
        return std::pair(false, 0);
    }
    return std::pair(true, be64toh(value));
}

std::pair<bool, double> KvStoreDataShareBridge::VectorToDouble(const std::vector<uint8_t> &input)
{
    if (input.size() != sizeof(double)) {
        return std::pair(false, 0);
    }
    uint64_t output;
    auto ret = memcpy_s(&output, sizeof(output), input.data(), sizeof(output));
    if (ret != EOK) {
        return std::pair(false, 0);
    }
    output = be64toh(output);
    double value;
    ret = memcpy_s(&value, sizeof(value), &output, sizeof(value));
    if (ret != EOK) {
        return std::pair(false, 0);
    }
    return std::pair(true, value);
}
} // namespace DistributedKv
} // namespace OHOS