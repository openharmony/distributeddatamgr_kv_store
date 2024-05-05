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

#include "cloud/asset_operation_utils.h"
#include "cloud/cloud_store_types.h"
#include "icloud_sync_storage_interface.h"
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

    static int Int64ToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
        std::vector<uint8_t> &value);
    static int BoolToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
        std::vector<uint8_t> &value);
    static int DoubleToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
        std::vector<uint8_t> &value);
    static int TextToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
        std::vector<uint8_t> &value);
    static int BlobToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
        std::vector<uint8_t> &value);

    static std::set<std::string> GetCloudPrimaryKey(const TableSchema &tableSchema);
    static std::vector<Field> GetCloudPrimaryKeyField(const TableSchema &tableSchema, bool sortByName = false);
    static std::map<std::string, Field> GetCloudPrimaryKeyFieldMap(const TableSchema &tableSchema,
        bool sortByUpper = false);
    static bool IsContainsPrimaryKey(const TableSchema &tableSchema);
    static std::vector<Field> GetCloudAsset(const TableSchema &tableSchema);
    static int GetAssetFieldsFromSchema(const TableSchema &tableSchema, const VBucket &vBucket,
        std::vector<Field> &fields);
    static void ObtainAssetFromVBucket(const VBucket &vBucket, VBucket &asset);
    static AssetOpType StatusToFlag(AssetStatus status);
    static AssetStatus FlagToStatus(AssetOpType opType);
    static void ChangeAssetsOnVBucketToAsset(VBucket &vBucket, std::vector<Field> &fields);
    static Type GetAssetFromAssets(Type &value);
    static int FillAssetBeforeDownload(Asset &asset);
    static int FillAssetAfterDownloadFail(Asset &asset, Asset &dbAsset, AssetOperationUtils::AssetOpType assetOpType);
    static void FillAssetsAfterDownloadFail(Assets &assets, Assets &dbAssets,
        const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap);
    static void MergeAssetWithFillFunc(Assets &assets, Assets &dbAssets, const std::map<std::string,
        AssetOperationUtils::AssetOpType> &assetOpTypeMap,
        std::function<int(Asset &, Asset &, AssetOperationUtils::AssetOpType)> fillAsset);
    static int FillAssetAfterDownload(Asset &asset, Asset &dbAsset, AssetOperationUtils::AssetOpType assetOpType);
    static void FillAssetsAfterDownload(Assets &assets, Assets &dbAssets,
        const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap);
    static int FillAssetBeforeUpload(Asset &asset, Asset &dbAsset, AssetOperationUtils::AssetOpType assetOpType);
    static void FillAssetsBeforeUpload(Assets &assets, Assets &dbAssets,
        const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap);
    static int FillAssetForUpload(Asset &asset, Asset &dbAsset, AssetOperationUtils::AssetOpType assetOpType);
    static void FillAssetsForUpload(Assets &assets, Assets &dbAssets,
        const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap);
    static int FillAssetForUploadFailed(Asset &asset, Asset &dbAsset, AssetOperationUtils::AssetOpType assetOpType);
    static void FillAssetsForUploadFailed(Assets &assets, Assets &dbAssets,
        const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap);
    static void PrepareToFillAssetFromVBucket(VBucket &vBucket, std::function<int(Asset &)> fillAsset);
    static void FillAssetFromVBucketFinish(const AssetOperationUtils::RecordAssetOpType &assetOpType, VBucket &vBucket,
        VBucket &dbAssets, std::function<int(Asset &, Asset &, AssetOperationUtils::AssetOpType)> fillAsset,
        std::function<void(Assets &, Assets &,
        const std::map<std::string, AssetOperationUtils::AssetOpType> &)> fillAssets);
    static bool IsAsset(const Type &type);
    static bool IsAssets(const Type &type);
    static bool IsAssetsContainDuplicateAsset(Assets &assets);
    static void EraseNoChangeAsset(std::map<std::string, Assets> &assetsMap);
    static void MergeDownloadAsset(std::map<std::string, Assets> &downloadAssets,
        std::map<std::string, Assets> &mergeAssets);
    static std::map<std::string, size_t> GenAssetsIndexMap(Assets &assets);
    static bool IsVbucketContainsAllPK(const VBucket &vBucket, const std::set<std::string> &pkSet);
    static bool CheckAssetStatus(const Assets &assets);

    static int ConstraintsCheckForCloud(const TableInfo &table, const std::string &trimmedSql);
    static std::string GetTableRefUpdateSql(const TableInfo &table, OpType opType);
    static std::string GetLeftJoinLogSql(const std::string &tableName, bool logAsTableA = true);
    static std::string GetUpdateLockChangedSql();
    static std::string GetDeleteLockChangedSql();
    static void AddUpdateColForShare(const TableSchema &tableSchema, std::string &updateLogSql,
        std::vector<std::string> &updateColName);

    static bool IsSharedTable(const TableSchema &tableSchema);
    static bool ChkFillCloudAssetParam(const CloudSyncBatch &data, int errCode);
    static void GetToBeRemoveAssets(const VBucket &vBucket, const AssetOperationUtils::RecordAssetOpType &assetOpType,
        std::vector<Asset> &removeAssets);
    static std::pair<int, std::vector<uint8_t>> GetHashValueWithPrimaryKeyMap(const VBucket &vBucket,
        const TableSchema &tableSchema, const TableInfo &localTable, const std::map<std::string, Field> &pkMap,
        bool allowEmpty);
    static std::string GetUpdateRecordFlagSql(const std::string &tableName, bool recordConflict,
        const LogInfo &logInfo);
    static int BindStepConsistentFlagStmt(sqlite3_stmt *stmt, const VBucket &data,
        const std::set<std::string> &gidFilters);
    static bool IsCloudGidMismatch(const std::string &downloadGid, const std::string &curGid);

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
        Type cloudValue;
        bool isExisted = GetTypeCaseInsensitive(fieldName, vBucket, cloudValue);
        if (!isExisted) {
            return -E_NOT_FOUND;
        }
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
        CollateType collateType, std::vector<uint8_t> &hashValue);

    static void TransferFieldToLower(VBucket &vBucket);

    static bool GetTypeCaseInsensitive(const std::string &fieldName, const VBucket &vBucket, Type &data);

    static bool CheckCloudSchemaFields(const TableSchema &tableSchema, const TableSchema &oldSchema);

    static int BindUpdateLogStmtFromVBucket(const VBucket &vBucket, const TableSchema &tableSchema,
        const std::vector<std::string> &colNames, sqlite3_stmt *updateLogStmt);

    static bool IsGetCloudDataContinue(uint32_t curNum, uint32_t curSize, uint32_t maxSize);

    static int IdentifyCloudType(CloudSyncData &cloudSyncData, VBucket &data, VBucket &log, VBucket &flags);

    static bool IsAbnormalData(const VBucket &data);

    static std::pair<int, DataItem> GetDataItemFromCloudData(VBucket &data);

    static int GetBytesFromCloudData(const std::string &field, VBucket &data, Bytes &bytes);

    static int GetStringFromCloudData(const std::string &field, VBucket &data, std::string &str);

    static int GetUInt64FromCloudData(const std::string &field, VBucket &data, uint64_t &number);

    static bool IsDataLocked(uint32_t status);

    static std::pair<int, DataItem> GetDataItemFromCloudVersionData(VBucket &data);

    static std::pair<int, DataItem> GetSystemRecordFromCloudData(VBucket &data);

    static bool IsSystemRecord(const Key &key);
};
}
#endif // CLOUD_STORAGE_UTILS_H
