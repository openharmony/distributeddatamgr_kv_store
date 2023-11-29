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
#include "cloud_db_data_utils.h"

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
namespace DistributedDB {
std::vector<VBucket> CloudDBDataUtils::GenerateRecords(int recordCounts, const TableSchema &schema)
{
    std::vector<VBucket> records;
    records.resize(recordCounts);
    for (int i = 0; i < recordCounts; ++i) {
        records.push_back(GenerateRecord(schema, i));
    }
    return records;
}

VBucket CloudDBDataUtils::GenerateRecord(const DistributedDB::TableSchema &schema, int32_t start)
{
    VBucket bucket;
    Type value = Nil();
    for (const auto &field: schema.fields) {
        switch (field.type) {
            case TYPE_INDEX<int64_t>:
                value = static_cast<int64_t>(start);
                break;
            case TYPE_INDEX<double>:
                value = start * 1.0;
                break;
            case TYPE_INDEX<std::string>:
                value = std::to_string(start);
                break;
            case TYPE_INDEX<bool>:
                value = (start != 0);
                break;
            case TYPE_INDEX<Bytes>:
            case TYPE_INDEX<Asset>:
            case TYPE_INDEX<Assets>: {
                std::string blob = std::to_string(start);
                value = Bytes(blob.begin(), blob.end());
                break;
            }
            default:
                value = Nil();
        }
        bucket[field.colName] = std::move(value);
    }
    return bucket;
}

std::vector<VBucket> CloudDBDataUtils::GenerateExtends(int extendCounts)
{
    std::vector<VBucket> extends;
    extends.resize(extendCounts);
    for (int i = 0; i < extendCounts; ++i) {
        VBucket info = {
            { CloudDbConstant::DELETE_FIELD, false }
        };
        extends.push_back(info);
    }
    return extends;
}
}