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

#include "sqlite_relational_utils.h"
#include "db_errno.h"
#include "cloud/cloud_db_types.h"
#include "sqlite_utils.h"

namespace DistributedDB {
int SQLiteRelationalUtils::GetDataValueByType(sqlite3_stmt *statement, int cid, DataValue &value)
{
    if (statement == nullptr || cid < 0 || cid >= sqlite3_column_count(statement)) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    int storageType = sqlite3_column_type(statement, cid);
    switch (storageType) {
        case SQLITE_INTEGER: {
            value = static_cast<int64_t>(sqlite3_column_int64(statement, cid));
            break;
        }
        case SQLITE_FLOAT: {
            value = sqlite3_column_double(statement, cid);
            break;
        }
        case SQLITE_BLOB: {
            std::vector<uint8_t> blobValue;
            errCode = SQLiteUtils::GetColumnBlobValue(statement, cid, blobValue);
            if (errCode != E_OK) {
                return errCode;
            }
            auto blob = new (std::nothrow) Blob;
            if (blob == nullptr) {
                return -E_OUT_OF_MEMORY;
            }
            blob->WriteBlob(blobValue.data(), static_cast<uint32_t>(blobValue.size()));
            errCode = value.Set(blob);
            break;
        }
        case SQLITE_NULL: {
            break;
        }
        case SQLITE3_TEXT: {
            std::string str;
            (void)SQLiteUtils::GetColumnTextValue(statement, cid, str);
            value = str;
            if (value.GetType() != StorageType::STORAGE_TYPE_TEXT) {
                errCode = -E_OUT_OF_MEMORY;
            }
            break;
        }
        default: {
            break;
        }
    }
    return errCode;
}

std::vector<DataValue> SQLiteRelationalUtils::GetSelectValues(sqlite3_stmt *stmt)
{
    std::vector<DataValue> values;
    for (int cid = 0, colCount = sqlite3_column_count(stmt); cid < colCount; ++cid) {
        DataValue value;
        (void)GetDataValueByType(stmt,  cid, value);
        values.emplace_back(std::move(value));
    }
    return values;
}

int SQLiteRelationalUtils::GetCloudValueByType(sqlite3_stmt *statement, int type, int cid, Type &cloudValue)
{
    if (statement == nullptr || cid < 0 || cid >= sqlite3_column_count(statement)) {
        return -E_INVALID_ARGS;
    }
    switch (sqlite3_column_type(statement, cid)) {
        case SQLITE_INTEGER: {
            if (type == TYPE_INDEX<bool>) {
                cloudValue = static_cast<bool>(sqlite3_column_int(statement, cid));
                break;
            }
            cloudValue = static_cast<int64_t>(sqlite3_column_int64(statement, cid));
            break;
        }
        case SQLITE_FLOAT: {
            cloudValue = sqlite3_column_double(statement, cid);
            break;
        }
        case SQLITE_BLOB: {
            std::vector<uint8_t> blobValue;
            int errCode = SQLiteUtils::GetColumnBlobValue(statement, cid, blobValue);
            if (errCode != E_OK) {
                return errCode;
            }
            cloudValue = blobValue;
            break;
        }
        case SQLITE_NULL: {
            break;
        }
        case SQLITE3_TEXT: {
            bool isBlob = (type == TYPE_INDEX<Bytes> || type == TYPE_INDEX<Asset> || type == TYPE_INDEX<Assets>);
            if (isBlob) {
                std::vector<uint8_t> blobValue;
                int errCode = SQLiteUtils::GetColumnBlobValue(statement, cid, blobValue);
                if (errCode != E_OK) {
                    return errCode;
                }
                cloudValue = blobValue;
                break;
            }
            std::string str;
            (void)SQLiteUtils::GetColumnTextValue(statement, cid, str);
            cloudValue = str;
            break;
        }
        default:
            break;
    }
    return E_OK;
}

void SQLiteRelationalUtils::CalCloudValueLen(Type &cloudValue, uint32_t &totalSize)
{
    switch (cloudValue.index()) {
        case TYPE_INDEX<int64_t>:
            totalSize += sizeof(int64_t);
            break;
        case TYPE_INDEX<double>:
            totalSize += sizeof(double);
            break;
        case TYPE_INDEX<std::string>:
            totalSize += std::get<std::string>(cloudValue).size();
            break;
        case TYPE_INDEX<bool>:
            totalSize += sizeof(int32_t);
            break;
        case TYPE_INDEX<Bytes>:
        case TYPE_INDEX<Asset>:
        case TYPE_INDEX<Assets>:
            totalSize += std::get<Bytes>(cloudValue).size();
            break;
        default: {
            break;
        }
    }
}
} // namespace DistributedDB