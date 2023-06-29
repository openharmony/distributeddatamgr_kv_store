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

#include "cloud/cloud_store_types.h"
#include "sqlite_utils.h"

namespace DistributedDB {
class CloudStorageUtils final {
public:
    static int BindInt64(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindBool(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindDouble(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindText(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindBlob(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);
    static int BindAsset(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt);

    static inline bool IsFieldValid(const Field &field, int errCode)
    {
        return (errCode == E_OK || (field.nullable && errCode == -E_NOT_FOUND));
    }

    static int Int64ToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int BoolToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int DoubleToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int TextToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);
    static int BlobToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value);

    static std::set<std::string> GetCloudPrimaryKey(const TableSchema &tableSchema);
    static std::vector<Field> GetCloudPrimaryKeyField(const TableSchema &tableSchema);
    static std::map<std::string, Field> GetCloudPrimaryKeyFieldMap(const TableSchema &tableSchema);
    static bool IsContainsPrimaryKey(const TableSchema &tableSchema);
    static std::vector<Field> GetCloudAsset(const TableSchema &tableSchema);
    static int CheckAssetFromSchema(const TableSchema &tableSchema, VBucket &vBucket,
        std::vector<Field> &fields);
    static void ObtainAssetFromVBucket(const VBucket &vBucket, VBucket &asset);
    static AssetOpType StatusToFlag(AssetStatus status);
    static AssetStatus FlagToStatus(AssetOpType opType);
    static int FillAssetForDownload(Asset &asset);
    static void FillAssetsForDownload(Assets &assets);
    static int FillAssetForUpload(Asset &asset);
    static void FillAssetsForUpload(Assets &assets);
    static void PrepareToFillAssetFromVBucket(VBucket &vBucket);
    static void FillAssetFromVBucketFinish(VBucket &vBucket, std::function<int(Asset &)> fillAsset,
        std::function<void(Assets &)> fillAssets);
    static bool IsAsset(const Type &type);
    static bool IsAssets(const Type &type);
    static bool IsAssetsContainDuplicateAsset(Assets &assets);

    template<typename T>
    static int GetValueFromOneField(Type &cloudValue, T &outVal)
    {
        T *value = std::get_if<T>(&cloudValue);
        if (value == nullptr) {
            LOGE("Get cloud data fail because type mismatch.");
            return -E_CLOUD_ERROR;
        }
        outVal = *value;
        return E_OK;
    }

    template<typename T>
    static int GetValueFromVBucket(const std::string &fieldName, const VBucket &vBucket, T &outVal)
    {
        if (vBucket.find(fieldName) == vBucket.end()) {
            LOGW("vbucket doesn't contains the field want to get");
            return -E_NOT_FOUND;
        }
        Type cloudValue = vBucket.at(fieldName);
        return GetValueFromOneField(cloudValue, outVal);
    }

    template<typename T>
    static int GetValueFromType(Type &cloudValue, T &outVal)
    {
        T *value = std::get_if<T>(&cloudValue);
        if (value == nullptr) {
            return -E_CLOUD_ERROR;
        }
        outVal = *value;
        return E_OK;
    }

    static int CalculateHashKeyForOneField(const Field &field, const VBucket &vBucket, bool allowEmpty,
        std::vector<uint8_t> &hashValue);
};
}
#endif // CLOUD_STORAGE_UTILS_H
