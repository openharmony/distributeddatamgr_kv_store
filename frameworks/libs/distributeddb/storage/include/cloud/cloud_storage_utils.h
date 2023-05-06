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

#ifndef CLOUD_STORAGE_UTILS_H
#define CLOUD_STORAGE_UTILS_H

#include "cloud_store_types.h"
#include "sqlite_utils.h"

namespace DistributedDB {
class CloudStorageUtils final {
public:
    static int BindInt64(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindBool(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindDouble(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindText(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindBlob(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);

    static inline bool IsFieldValid(const Field &field, int errCode)
    {
        return (errCode == E_OK || (field.nullable && errCode == -E_NOT_FOUND));
    }

    static int Int64ToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int BoolToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int DoubleToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int TextToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int BlobToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);

    template<typename T>
    static int GetValueFromVBucket(const std::string &fieldName, const VBucket &vBucket, T &outVal)
    {
        if (vBucket.find(fieldName) == vBucket.end()) {
            LOGW("vbucket doesn't contains field: %s.", fieldName.c_str());
            return -E_NOT_FOUND;
        }
        Type cloudValue = vBucket.at(fieldName);
        T *value = std::get_if<T>(&cloudValue);
        if (value == nullptr) {
            LOGE("Get cloud data fail because type mismatch.");
            return -E_INVALID_DATA;
        }
        outVal = *value;
        return E_OK;
    }
};
}
#endif // CLOUD_STORAGE_UTILS_H
