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

#include "cloud/cloud_storage_utils.h"
#include <set>

#include "cloud/asset_operation_utils.h"
#include "cloud/cloud_db_types.h"
#include "db_common.h"
#include "runtime_context.h"
#include "cloud/cloud_db_constant.h"

namespace DistributedDB {
int CloudStorageUtils::BindInt64(int index, const VBucket &vBucket, const Field &field,
    sqlite3_stmt *upsertStmt)
{
    int64_t val = 0;
    int errCode = GetValueFromVBucket<int64_t>(field.colName, vBucket, val);
    if (field.nullable && errCode == -E_NOT_FOUND) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    } else {
        if (errCode != E_OK) {
            LOGE("get int from vbucket failed, %d", errCode);
            return -E_CLOUD_ERROR;
        }
        errCode = SQLiteUtils::BindInt64ToStatement(upsertStmt, index, val);
    }

    if (errCode != E_OK) {
        LOGE("Bind int to insert statement failed, %d", errCode);
    }
    return errCode;
}

int CloudStorageUtils::BindBool(int index, const VBucket &vBucket, const Field &field,
    sqlite3_stmt *upsertStmt)
{
    bool val = false;
    int errCode = GetValueFromVBucket<bool>(field.colName, vBucket, val);
    if (field.nullable && errCode == -E_NOT_FOUND) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    } else {
        if (errCode != E_OK) {
            LOGE("get bool from vbucket failed, %d", errCode);
            return -E_CLOUD_ERROR;
        }
        errCode = SQLiteUtils::BindInt64ToStatement(upsertStmt, index, val);
    }

    if (errCode != E_OK) {
        LOGE("Bind bool to insert statement failed, %d", errCode);
    }
    return errCode;
}

int CloudStorageUtils::BindDouble(int index, const VBucket &vBucket, const Field &field,
    sqlite3_stmt *upsertStmt)
{
    double val = 0.0;
    int errCode = GetValueFromVBucket<double>(field.colName, vBucket, val);
    if (field.nullable && errCode == -E_NOT_FOUND) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    } else {
        if (errCode != E_OK) {
            LOGE("get double from vbucket failed, %d", errCode);
            return -E_CLOUD_ERROR;
        }
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_double(upsertStmt, index, val));
    }

    if (errCode != E_OK) {
        LOGE("Bind double to insert statement failed, %d", errCode);
    }
    return errCode;
}

int CloudStorageUtils::BindText(int index, const VBucket &vBucket, const Field &field,
    sqlite3_stmt *upsertStmt)
{
    std::string str;
    int errCode = GetValueFromVBucket<std::string>(field.colName, vBucket, str);
    if (field.nullable && errCode == -E_NOT_FOUND) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    } else {
        if (errCode != E_OK) {
            LOGE("get string from vbucket failed, %d", errCode);
            return -E_CLOUD_ERROR;
        }
        errCode = SQLiteUtils::BindTextToStatement(upsertStmt, index, str);
    }

    if (errCode != E_OK) {
        LOGE("Bind string to insert statement failed, %d", errCode);
    }
    return errCode;
}

int CloudStorageUtils::BindBlob(int index, const VBucket &vBucket, const Field &field,
    sqlite3_stmt *upsertStmt)
{
    int errCode = E_OK;
    Bytes val;
    if (field.type == TYPE_INDEX<Bytes>) {
        errCode = GetValueFromVBucket<Bytes>(field.colName, vBucket, val);
        if (!(IsFieldValid(field, errCode))) {
            goto ERROR;
        }
    } else if (field.type == TYPE_INDEX<Asset>) {
        Asset asset;
        errCode = GetValueFromVBucket(field.colName, vBucket, asset);
        if (!(IsFieldValid(field, errCode))) {
            goto ERROR;
        }
        RuntimeContext::GetInstance()->AssetToBlob(asset, val);
    } else {
        Assets assets;
        errCode = GetValueFromVBucket(field.colName, vBucket, assets);
        if (!(IsFieldValid(field, errCode))) {
            goto ERROR;
        }
        RuntimeContext::GetInstance()->AssetsToBlob(assets, val);
    }

    if (errCode == -E_NOT_FOUND) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    } else {
        errCode = SQLiteUtils::BindBlobToStatement(upsertStmt, index, val);
    }
    if (errCode != E_OK) {
        LOGE("Bind blob to insert statement failed, %d", errCode);
    }
    return errCode;
ERROR:
    LOGE("get blob from vbucket failed, %d", errCode);
    return -E_CLOUD_ERROR;
}

int CloudStorageUtils::BindAsset(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *upsertStmt)
{
    int errCode;
    Bytes val;
    Type entry;
    bool isExisted = GetTypeCaseInsensitive(field.colName, vBucket, entry);
    if (!isExisted || entry.index() == TYPE_INDEX<Nil>) {
        if (!field.nullable) {
            LOGE("field value is not allowed to be null, %d", -E_CLOUD_ERROR);
            return -E_CLOUD_ERROR;
        }
        return SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    }

    Type type = entry;
    if (field.type == TYPE_INDEX<Asset>) {
        Asset asset;
        errCode = GetValueFromOneField(type, asset);
        if (errCode != E_OK) {
            LOGE("can not get asset from vBucket when bind, %d", errCode);
            return errCode;
        }
        asset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
        errCode = RuntimeContext::GetInstance()->AssetToBlob(asset, val);
    } else if (field.type == TYPE_INDEX<Assets>) {
        Assets assets;
        errCode = GetValueFromOneField(type, assets);
        if (errCode != E_OK) {
            LOGE("can not get assets from vBucket when bind, %d", errCode);
            return errCode;
        }
        if (!assets.empty()) {
            for (auto &asset: assets) {
                asset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
            }
            errCode = RuntimeContext::GetInstance()->AssetsToBlob(assets, val);
        }
    } else {
        LOGE("field type is not asset or assets, %d", -E_CLOUD_ERROR);
        return -E_CLOUD_ERROR;
    }
    if (errCode != E_OK) {
        LOGE("assets or asset to blob fail, %d", -E_CLOUD_ERROR);
        return -E_CLOUD_ERROR;
    }
    if (val.empty()) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    } else {
        errCode = SQLiteUtils::BindBlobToStatement(upsertStmt, index, val);
    }
    return errCode;
}

int CloudStorageUtils::Int64ToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    int64_t val = 0;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    DBCommon::StringToVector(std::to_string(val), value);
    return E_OK;
}

int CloudStorageUtils::BoolToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    bool val = false;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    DBCommon::StringToVector(std::to_string(val ? 1 : 0), value);
    return E_OK;
}

int CloudStorageUtils::DoubleToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    double val = 0.0;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    std::ostringstream s;
    s << val;
    DBCommon::StringToVector(s.str(), value);
    return E_OK;
}

int CloudStorageUtils::TextToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    std::string val;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    if (collateType == CollateType::COLLATE_NOCASE) {
        std::transform(val.begin(), val.end(), val.begin(), ::toupper);
    } else if (collateType == CollateType::COLLATE_RTRIM) {
        DBCommon::RTrim(val);
    }

    DBCommon::StringToVector(val, value);
    return E_OK;
}

int CloudStorageUtils::BlobToVector(const VBucket &vBucket, const Field &field, CollateType collateType,
    std::vector<uint8_t> &value)
{
    (void)collateType;
    if (field.type == TYPE_INDEX<Bytes>) {
        return CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, value);
    } else if (field.type == TYPE_INDEX<Asset>) {
        Asset val;
        if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
            return -E_CLOUD_ERROR;
        }
        int errCode = RuntimeContext::GetInstance()->AssetToBlob(val, value);
        if (errCode != E_OK) {
            LOGE("asset to blob fail, %d", errCode);
        }
        return errCode;
    } else {
        Assets val;
        if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
            return -E_CLOUD_ERROR;
        }
        int errCode = RuntimeContext::GetInstance()->AssetsToBlob(val, value);
        if (errCode != E_OK) {
            LOGE("assets to blob fail, %d", errCode);
        }
        return errCode;
    }
}

std::set<std::string> CloudStorageUtils::GetCloudPrimaryKey(const TableSchema &tableSchema)
{
    std::set<std::string> pkSet;
    for (const auto &field : tableSchema.fields) {
        if (field.primary) {
            pkSet.insert(field.colName);
        }
    }
    return pkSet;
}

std::vector<Field> CloudStorageUtils::GetCloudAsset(const TableSchema &tableSchema)
{
    std::vector<Field> assetFields;
    for (const auto &item: tableSchema.fields) {
        if (item.type != TYPE_INDEX<Asset> && item.type != TYPE_INDEX<Assets>) {
            continue;
        }
        assetFields.push_back(item);
    }
    return assetFields;
}

std::vector<Field> CloudStorageUtils::GetCloudPrimaryKeyField(const TableSchema &tableSchema, bool sortByName)
{
    std::vector<Field> pkVec;
    for (const auto &field : tableSchema.fields) {
        if (field.primary) {
            pkVec.push_back(field);
        }
    }
    if (sortByName) {
        std::sort(pkVec.begin(), pkVec.end(), [](const Field &a, const Field &b) {
           return a.colName < b.colName;
        });
    }
    return pkVec;
}

std::map<std::string, Field> CloudStorageUtils::GetCloudPrimaryKeyFieldMap(const TableSchema &tableSchema,
    bool sortByUpper)
{
    std::map<std::string, Field> pkMap;
    for (const auto &field : tableSchema.fields) {
        if (field.primary) {
            if (sortByUpper) {
                pkMap[DBCommon::ToUpperCase(field.colName)] = field;
            } else {
                pkMap[field.colName] = field;
            }
        }
    }
    return pkMap;
}

int CloudStorageUtils::GetAssetFieldsFromSchema(const TableSchema &tableSchema, const VBucket &vBucket,
    std::vector<Field> &fields)
{
    for (const auto &field: tableSchema.fields) {
        Type type;
        bool isExisted = GetTypeCaseInsensitive(field.colName, vBucket, type);
        if (!isExisted) {
            continue;
        }
        if (type.index() != TYPE_INDEX<Asset> && type.index() != TYPE_INDEX<Assets>) {
            continue;
        }
        fields.push_back(field);
    }
    if (fields.empty()) {
        return -E_CLOUD_ERROR;
    }
    return E_OK;
}

bool CloudStorageUtils::IsContainsPrimaryKey(const TableSchema &tableSchema)
{
    for (const auto &field : tableSchema.fields) {
        if (field.primary) {
            return true;
        }
    }
    return false;
}

void CloudStorageUtils::ObtainAssetFromVBucket(const VBucket &vBucket, VBucket &asset)
{
    for (const auto &item: vBucket) {
        if (IsAsset(item.second)) {
            Asset data = std::get<Asset>(item.second);
            asset.insert_or_assign(item.first, data);
        } else if (IsAssets(item.second)) {
            Assets data = std::get<Assets>(item.second);
            asset.insert_or_assign(item.first, data);
        }
    }
}

AssetOpType CloudStorageUtils::StatusToFlag(AssetStatus status)
{
    switch (AssetOperationUtils::EraseBitMask(status)) {
        case AssetStatus::INSERT:
            return AssetOpType::INSERT;
        case AssetStatus::DELETE:
            return AssetOpType::DELETE;
        case AssetStatus::UPDATE:
            return AssetOpType::UPDATE;
        default:
            return AssetOpType::NO_CHANGE;
    }
}

AssetStatus CloudStorageUtils::FlagToStatus(AssetOpType opType)
{
    switch (opType) {
        case AssetOpType::INSERT:
            return AssetStatus::INSERT;
        case AssetOpType::DELETE:
            return AssetStatus::DELETE;
        case AssetOpType::UPDATE:
            return AssetStatus::UPDATE;
        default:
            return AssetStatus::NORMAL;
    }
}

void CloudStorageUtils::ChangeAssetsOnVBucketToAsset(VBucket &vBucket, std::vector<Field> &fields)
{
    for (const Field &field: fields) {
        if (field.type == TYPE_INDEX<Asset>) {
            Type asset = GetAssetFromAssets(vBucket[field.colName]);
            vBucket[field.colName] = asset;
        }
    }
}

Type CloudStorageUtils::GetAssetFromAssets(Type &value)
{
    Asset assetVal;
    int errCode = GetValueFromType(value, assetVal);
    if (errCode == E_OK) {
        return assetVal;
    }

    Assets assets;
    errCode = GetValueFromType(value, assets);
    if (errCode != E_OK) {
        return Nil();
    }

    for (Asset &asset: assets) {
        if (asset.flag != static_cast<uint32_t>(AssetOpType::DELETE)) {
            return std::move(asset);
        }
    }
    return Nil();
}

int CloudStorageUtils::FillAssetBeforeDownload(Asset &asset)
{
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
    AssetStatus status = static_cast<AssetStatus>(asset.status);
    uint32_t lowStatus = AssetOperationUtils::EraseBitMask(asset.status);
    switch (flag) {
        case AssetOpType::DELETE: {
            // these asset no need to download, just remove before download
            if (lowStatus == static_cast<uint32_t>(AssetStatus::DELETE) ||
                lowStatus == static_cast<uint32_t>(AssetStatus::ABNORMAL) ||
                (asset.status == (AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL))) {
                return -E_NOT_FOUND;
            }
            break;
        }
        case AssetOpType::INSERT:
        case AssetOpType::UPDATE: {
            if (status != AssetStatus::NORMAL) {
                asset.hash = std::string("");
            }
            break;
        }
        default:
            break;
    }
    return E_OK;
}

int CloudStorageUtils::FillAssetAfterDownload(Asset &asset, Asset &dbAsset,
    AssetOperationUtils::AssetOpType assetOpType)
{
    if (assetOpType == AssetOperationUtils::AssetOpType::NOT_HANDLE) {
        return E_OK;
    }
    dbAsset = asset;
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
    if (asset.status != AssetStatus::NORMAL) {
        return E_OK;
    }
    switch (flag) {
        case AssetOpType::DELETE: {
            return -E_NOT_FOUND;
        }
        default:
            break;
    }
    return E_OK;
}

void CloudStorageUtils::FillAssetsAfterDownload(Assets &assets, Assets &dbAssets,
    const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap)
{
    MergeAssetWithFillFunc(assets, dbAssets, assetOpTypeMap, FillAssetAfterDownload);
}

int CloudStorageUtils::FillAssetForUpload(Asset &asset, Asset &dbAsset, AssetOperationUtils::AssetOpType assetOpType)
{
    if (assetOpType == AssetOperationUtils::AssetOpType::NOT_HANDLE) {
        // db assetId may be empty, need to be based on cache
        dbAsset.assetId = asset.assetId;
        return E_OK;
    }
    AssetStatus status = static_cast<AssetStatus>(dbAsset.status);
    dbAsset = asset;
    switch (StatusToFlag(status)) {
        case AssetOpType::INSERT:
        case AssetOpType::UPDATE:
        case AssetOpType::NO_CHANGE: {
            dbAsset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
            break;
        }
        case AssetOpType::DELETE: {
            return -E_NOT_FOUND;
        }
        default: {
            break;
        }
    }
    dbAsset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
    return E_OK;
}

void CloudStorageUtils::FillAssetsForUpload(Assets &assets, Assets &dbAssets,
    const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap)
{
    MergeAssetWithFillFunc(assets, dbAssets, assetOpTypeMap, FillAssetForUpload);
}

int CloudStorageUtils::FillAssetBeforeUpload(Asset &asset, Asset &dbAsset, AssetOperationUtils::AssetOpType assetOpType)
{
    if (assetOpType == AssetOperationUtils::AssetOpType::NOT_HANDLE) {
        return E_OK;
    }
    dbAsset = asset;
    switch (static_cast<AssetOpType>(asset.flag)) {
        case AssetOpType::INSERT:
        case AssetOpType::UPDATE:
        case AssetOpType::DELETE:
        case AssetOpType::NO_CHANGE:
            dbAsset.status |= static_cast<uint32_t>(AssetStatus::UPLOADING);
            break;
        default:
            break;
    }
    dbAsset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
    return E_OK;
}

void CloudStorageUtils::FillAssetsBeforeUpload(Assets &assets, Assets &dbAssets, const std::map<std::string,
    AssetOperationUtils::AssetOpType> &assetOpTypeMap)
{
    MergeAssetWithFillFunc(assets, dbAssets, assetOpTypeMap, FillAssetBeforeUpload);
}

void CloudStorageUtils::PrepareToFillAssetFromVBucket(VBucket &vBucket, std::function<int(Asset &)> fillAsset)
{
    for (auto &item: vBucket) {
        if (IsAsset(item.second)) {
            Asset asset;
            GetValueFromType(item.second, asset);
            fillAsset(asset);
            vBucket[item.first] = asset;
        } else if (IsAssets(item.second)) {
            Assets assets;
            GetValueFromType(item.second, assets);
            for (auto it = assets.begin(); it != assets.end();) {
                fillAsset(*it) == -E_NOT_FOUND ? it = assets.erase(it) : ++it;
            }
            vBucket[item.first] = assets;
        }
    }
}

void CloudStorageUtils::FillAssetFromVBucketFinish(const AssetOperationUtils::RecordAssetOpType &assetOpType,
    VBucket &vBucket, VBucket &dbAssets,
    std::function<int(Asset &, Asset &, AssetOperationUtils::AssetOpType)> fillAsset,
    std::function<void(Assets &, Assets &,
    const std::map<std::string, AssetOperationUtils::AssetOpType> &)> fillAssets)
{
    for (auto &item: dbAssets) {
        if (IsAsset(item.second)) {
            Asset cacheItem;
            GetValueFromType(vBucket[item.first], cacheItem);
            Asset dbItem;
            GetValueFromType(item.second, dbItem);
            AssetOperationUtils::AssetOpType opType = AssetOperationUtils::AssetOpType::NOT_HANDLE;
            auto iterCol = assetOpType.find(item.first);
            if (iterCol != assetOpType.end() && iterCol->second.find(dbItem.name) != iterCol->second.end()) {
                opType = iterCol->second.at(dbItem.name);
            }
            int errCode = fillAsset(cacheItem, dbItem, opType);
            if (errCode != E_OK) {
                dbAssets[item.first] = Nil();
            } else {
                dbAssets[item.first] = dbItem;
            }
            continue;
        }
        if (IsAssets(item.second)) {
            Assets cacheItems;
            GetValueFromType(vBucket[item.first], cacheItems);
            Assets dbItems;
            GetValueFromType(item.second, dbItems);
            auto iterCol = assetOpType.find(item.first);
            if (iterCol == assetOpType.end()) {
                fillAssets(cacheItems, dbItems, {});
            } else {
                fillAssets(cacheItems, dbItems, iterCol->second);
            }
            if (dbItems.empty()) {
                dbAssets[item.first] = Nil();
            } else {
                dbAssets[item.first] = dbItems;
            }
        }
    }
}

bool CloudStorageUtils::IsAsset(const Type &type)
{
    return type.index() == TYPE_INDEX<Asset>;
}

bool CloudStorageUtils::IsAssets(const Type &type)
{
    return type.index() == TYPE_INDEX<Assets>;
}

int CloudStorageUtils::CalculateHashKeyForOneField(const Field &field, const VBucket &vBucket, bool allowEmpty,
    CollateType collateType, std::vector<uint8_t> &hashValue)
{
    Type type;
    bool isExisted = GetTypeCaseInsensitive(field.colName, vBucket, type);
    if (allowEmpty && !isExisted) {
        return E_OK; // if vBucket from cloud doesn't contain primary key and allowEmpty, no need to calculate hash
    }
    static std::map<int32_t, std::function<int(const VBucket &, const Field &, CollateType,
        std::vector<uint8_t> &)>> toVecFunc = {
        { TYPE_INDEX<int64_t>, &CloudStorageUtils::Int64ToVector },
        { TYPE_INDEX<bool>, &CloudStorageUtils::BoolToVector },
        { TYPE_INDEX<double>, &CloudStorageUtils::DoubleToVector },
        { TYPE_INDEX<std::string>, &CloudStorageUtils::TextToVector },
        { TYPE_INDEX<Bytes>, &CloudStorageUtils::BlobToVector },
        { TYPE_INDEX<Asset>, &CloudStorageUtils::BlobToVector },
        { TYPE_INDEX<Assets>, &CloudStorageUtils::BlobToVector },
    };
    auto it = toVecFunc.find(field.type);
    if (it == toVecFunc.end()) {
        LOGE("unknown cloud type when convert field to vector.");
        return -E_CLOUD_ERROR;
    }
    std::vector<uint8_t> value;
    int errCode = it->second(vBucket, field, collateType, value);
    if (errCode != E_OK) {
        LOGE("convert cloud field fail, %d", errCode);
        return errCode;
    }
    return DBCommon::CalcValueHash(value, hashValue);
}

bool CloudStorageUtils::IsAssetsContainDuplicateAsset(Assets &assets)
{
    std::set<std::string> set;
    for (const auto &asset : assets) {
        if (set.find(asset.name) != set.end()) {
            LOGE("assets contain duplicate Asset");
            return true;
        }
        set.insert(asset.name);
    }
    return false;
}

void CloudStorageUtils::EraseNoChangeAsset(std::map<std::string, Assets> &assetsMap)
{
    for (auto items = assetsMap.begin(); items != assetsMap.end();) {
        for (auto item = items->second.begin(); item != items->second.end();) {
            if (static_cast<AssetOpType>((*item).flag) == AssetOpType::NO_CHANGE) {
                item = items->second.erase(item);
            } else {
                item++;
            }
        }
        if (items->second.empty()) {
            items = assetsMap.erase(items);
        } else {
            items++;
        }
    }
}

void CloudStorageUtils::MergeDownloadAsset(std::map<std::string, Assets> &downloadAssets,
    std::map<std::string, Assets> &mergeAssets)
{
    for (auto &items: mergeAssets) {
        auto downloadItem = downloadAssets.find(items.first);
        if (downloadItem == downloadAssets.end()) {
            continue;
        }
        std::map<std::string, size_t> beCoveredAssetsMap = GenAssetsIndexMap(items.second);
        for (const Asset &asset: downloadItem->second) {
            auto it = beCoveredAssetsMap.find(asset.name);
            if (it == beCoveredAssetsMap.end()) {
                continue;
            }
            items.second[it->second] = asset;
        }
    }
}

std::map<std::string, size_t> CloudStorageUtils::GenAssetsIndexMap(Assets &assets)
{
    // key of assetsIndexMap is name of asset, the value of it is index.
    std::map<std::string, size_t> assetsIndexMap;
    for (size_t i = 0; i < assets.size(); i++) {
        assetsIndexMap[assets[i].name] = i;
    }
    return assetsIndexMap;
}

bool CloudStorageUtils::IsVbucketContainsAllPK(const VBucket &vBucket, const std::set<std::string> &pkSet)
{
    if (pkSet.empty()) {
        return false;
    }
    for (const auto &pk : pkSet) {
        Type type;
        bool isExisted = GetTypeCaseInsensitive(pk, vBucket, type);
        if (!isExisted) {
            return false;
        }
    }
    return true;
}

bool CloudStorageUtils::IsSharedTable(const TableSchema &tableSchema)
{
    return tableSchema.sharedTableName == tableSchema.name;
}

static bool IsViolationOfConstraints(const std::string &name, const std::vector<FieldInfo> &fieldInfos)
{
    for (const auto &field : fieldInfos) {
        if (name != field.GetFieldName()) {
            continue;
        }
        if (field.GetStorageType() == StorageType::STORAGE_TYPE_REAL) {
            LOGE("[ConstraintsCheckForCloud] Not support create distributed table with real primary key.");
            return true;
        } else if (field.IsAssetType() || field.IsAssetsType()) {
            LOGE("[ConstraintsCheckForCloud] Not support create distributed table with asset primary key.");
            return true;
        } else {
            return false;
        }
    }
    return false;
}

int CloudStorageUtils::ConstraintsCheckForCloud(const TableInfo &table, const std::string &trimmedSql)
{
    if (DBCommon::HasConstraint(trimmedSql, "UNIQUE", " ,", " ,)(")) {
        LOGE("[ConstraintsCheckForCloud] Not support create distributed table with 'UNIQUE' constraint.");
        return -E_NOT_SUPPORT;
    }

    const std::map<int, FieldName> &primaryKeys = table.GetPrimaryKey();
    const std::vector<FieldInfo> &fieldInfos = table.GetFieldInfos();
    for (const auto &item : primaryKeys) {
        if (IsViolationOfConstraints(item.second, fieldInfos)) {
            return -E_NOT_SUPPORT;
        }
    }
    return E_OK;
}

bool CloudStorageUtils::CheckAssetStatus(const Assets &assets)
{
    for (const Asset &asset: assets) {
        if (AssetOperationUtils::EraseBitMask(asset.status) > static_cast<uint32_t>(AssetStatus::UPDATE)) {
            LOGE("assets contain invalid status:[%u]", asset.status);
            return false;
        }
    }
    return true;
}

std::string CloudStorageUtils::GetTableRefUpdateSql(const TableInfo &table, OpType opType)
{
    std::string sql;
    std::string rowid = std::string(DBConstant::SQLITE_INNER_ROWID);
    for (const auto &reference : table.GetTableReference()) {
        if (reference.columns.empty()) {
            return "";
        }
        std::string sourceLogName = DBCommon::GetLogTableName(reference.sourceTableName);
        sql += " UPDATE " + sourceLogName + " SET timestamp=get_raw_sys_time(), flag=flag|0x02 WHERE ";
        int index = 0;
        for (const auto &itCol : reference.columns) {
            if (opType != OpType::UPDATE) {
                continue;
            }
            if (index++ != 0) {
                sql += " OR ";
            }
            sql += " (OLD." + itCol.second + " IS NOT " + " NEW." + itCol.second + ")";
        }
        if (opType == OpType::UPDATE) {
            sql += " AND ";
        }
        sql += " (flag&0x08=0x00) AND data_key IN (SELECT " + sourceLogName + ".data_key FROM " + sourceLogName +
            " LEFT JOIN " + reference.sourceTableName + " ON " + sourceLogName + ".data_key = " +
            reference.sourceTableName + "." + rowid + " WHERE ";
        index = 0;
        for (const auto &itCol : reference.columns) {
            if (index++ != 0) {
                sql += " OR ";
            }
            if (opType == OpType::UPDATE) {
                sql += itCol.first + "=OLD." + itCol.second + " OR " + itCol.first + "=NEW." + itCol.second;
            } else if (opType == OpType::INSERT) {
                sql += itCol.first + "=NEW." + itCol.second;
            } else if (opType == OpType::DELETE) {
                sql += itCol.first + "=OLD." + itCol.second;
            }
        }
        sql += ");";
    }
    return sql;
}

std::string CloudStorageUtils::GetLeftJoinLogSql(const std::string &tableName, bool logAsTableA)
{
    std::string sql;
    if (logAsTableA) {
        sql += " FROM '" + DBCommon::GetLogTableName(tableName) + "' AS a LEFT JOIN '" + tableName + "' AS b " +
            " ON (a.data_key = b." + std::string(DBConstant::SQLITE_INNER_ROWID) + ")";
    } else {
        sql += " FROM '" + DBCommon::GetLogTableName(tableName) + "' AS b LEFT JOIN '" + tableName + "' AS a " +
            " ON (b.data_key = a." + std::string(DBConstant::SQLITE_INNER_ROWID) + ")";
    }
    return sql;
}

std::string CloudStorageUtils::GetUpdateLockChangedSql()
{
    return " status = CASE WHEN status == 2 THEN 3 ELSE status END";
}

std::string CloudStorageUtils::GetDeleteLockChangedSql()
{
    return " status = CASE WHEN status == 2 or status == 3 THEN 1 ELSE status END";
}

bool CloudStorageUtils::ChkFillCloudAssetParam(const CloudSyncBatch &data, int errCode)
{
    if (data.assets.empty()) {
        errCode = E_OK;
        return true;
    }
    if (data.rowid.empty() || data.timestamp.empty()) {
        errCode = -E_INVALID_ARGS;
        LOGE("param is empty when fill cloud Asset. rowidN:%u, timeN:%u", errCode, data.rowid.size(),
            data.timestamp.size());
        return true;
    }
    if (data.assets.size() != data.rowid.size() || data.assets.size() != data.timestamp.size() ||
        data.assets.size() != data.hashKey.size() || data.assets.size() != data.extend.size()) {
        errCode = -E_INVALID_ARGS;
        LOGE("the num of param is invalid when fill cloud Asset. assetsN:%u, rowidN:%u, timeN:%u, "
             "hashKeyN:%u, extendN:%u", data.assets.size(), data.rowid.size(), data.timestamp.size(),
             data.hashKey.size(), data.extend.size());
        return true;
    }
    return false;
}

void CloudStorageUtils::GetToBeRemoveAssets(const VBucket &vBucket,
    const AssetOperationUtils::RecordAssetOpType &assetOpType, std::vector<Asset> &removeAssets)
{
    for (const auto &col: assetOpType) {
        Type itItem;
        bool isExisted = GetTypeCaseInsensitive(col.first, vBucket, itItem);
        if (!isExisted) {
            continue;
        }
        if (!CloudStorageUtils::IsAsset(itItem) && !CloudStorageUtils::IsAssets(itItem)) {
            continue;
        }
        if (CloudStorageUtils::IsAsset(itItem)) {
            Asset delAsset;
            GetValueFromType(itItem, delAsset);
            auto itOp = col.second.find(delAsset.name);
            if (itOp != col.second.end() && itOp->second == AssetOperationUtils::AssetOpType::NOT_HANDLE) {
                removeAssets.push_back(delAsset);
            }
            continue;
        }
        Assets assets;
        GetValueFromType(itItem, assets);
        for (const auto &asset: assets) {
            auto itOp = col.second.find(asset.name);
            if (itOp == col.second.end() || itOp->second == AssetOperationUtils::AssetOpType::HANDLE) {
                continue;
            }
            removeAssets.push_back(asset);
        }
    }
}

int CloudStorageUtils::FillAssetForUploadFailed(Asset &asset, Asset &dbAsset,
    AssetOperationUtils::AssetOpType assetOpType)
{
    dbAsset.assetId = asset.assetId;
    dbAsset.status &= ~AssetStatus::UPLOADING;
    return E_OK;
}

void CloudStorageUtils::FillAssetsForUploadFailed(Assets &assets, Assets &dbAssets,
    const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap)
{
    MergeAssetWithFillFunc(assets, dbAssets, assetOpTypeMap, FillAssetForUploadFailed);
}

int CloudStorageUtils::FillAssetAfterDownloadFail(Asset &asset, Asset &dbAsset,
    AssetOperationUtils::AssetOpType assetOpType)
{
    AssetStatus status = static_cast<AssetStatus>(asset.status);
    if (assetOpType == AssetOperationUtils::AssetOpType::NOT_HANDLE) {
        return E_OK;
    }
    if (status != AssetStatus::ABNORMAL) {
        return FillAssetAfterDownload(asset, dbAsset, assetOpType);
    }
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
    dbAsset = asset;
    switch (flag) {
        case AssetOpType::INSERT:
        case AssetOpType::DELETE:
        case AssetOpType::UPDATE: {
            dbAsset.hash = std::string("");
            break;
        }
        default:
            // other flag type do not need to clear hash
            break;
    }
    return E_OK;
}

void CloudStorageUtils::FillAssetsAfterDownloadFail(Assets &assets, Assets &dbAssets,
    const std::map<std::string, AssetOperationUtils::AssetOpType> &assetOpTypeMap)
{
    MergeAssetWithFillFunc(assets, dbAssets, assetOpTypeMap, FillAssetAfterDownloadFail);
}

void CloudStorageUtils::MergeAssetWithFillFunc(Assets &assets, Assets &dbAssets, const std::map<std::string,
    AssetOperationUtils::AssetOpType> &assetOpTypeMap,
    std::function<int(Asset &, Asset &, AssetOperationUtils::AssetOpType)> fillAsset)
{
    std::map<std::string, size_t> indexMap = GenAssetsIndexMap(assets);
    for (auto dbAsset = dbAssets.begin(); dbAsset != dbAssets.end();) {
        Asset cacheAsset;
        auto it = indexMap.find(dbAsset->name);
        if (it != indexMap.end()) {
            cacheAsset = assets[it->second];
        }
        AssetOperationUtils::AssetOpType opType = AssetOperationUtils::AssetOpType::NOT_HANDLE;
        auto iterOp = assetOpTypeMap.find(dbAsset->name);
        if (iterOp != assetOpTypeMap.end()) {
            opType = iterOp->second;
        }
        if (fillAsset(cacheAsset, *dbAsset, opType) == -E_NOT_FOUND) {
            dbAsset = dbAssets.erase(dbAsset);
        } else {
            dbAsset++;
        }
    }
}

std::pair<int, std::vector<uint8_t>> CloudStorageUtils::GetHashValueWithPrimaryKeyMap(const VBucket &vBucket,
    const TableSchema &tableSchema, const TableInfo &localTable, const std::map<std::string, Field> &pkMap,
    bool allowEmpty)
{
    int errCode = E_OK;
    std::vector<uint8_t> hashValue;
    if (pkMap.size() == 0) {
        LOGE("do not support get hashValue when primaryKey map is empty.");
        return { -E_INTERNAL_ERROR, {} };
    } else if (pkMap.size() == 1) {
        std::vector<Field> pkVec = CloudStorageUtils::GetCloudPrimaryKeyField(tableSchema);
        FieldInfoMap fieldInfos = localTable.GetFields();
        if (fieldInfos.find(pkMap.begin()->first) == fieldInfos.end()) {
            LOGE("localSchema doesn't contain primary key.");
            return { -E_INTERNAL_ERROR, {} };
        }
        CollateType collateType = fieldInfos.at(pkMap.begin()->first).GetCollateType();
        errCode = CloudStorageUtils::CalculateHashKeyForOneField(
            pkVec.at(0), vBucket, allowEmpty, collateType, hashValue);
    } else {
        std::vector<uint8_t> tempRes;
        for (const auto &item: pkMap) {
            FieldInfoMap fieldInfos = localTable.GetFields();
            if (fieldInfos.find(item.first) == fieldInfos.end()) {
                LOGE("localSchema doesn't contain primary key in multi pks.");
                return { -E_INTERNAL_ERROR, {} };
            }
            std::vector<uint8_t> temp;
            CollateType collateType = fieldInfos.at(item.first).GetCollateType();
            errCode = CloudStorageUtils::CalculateHashKeyForOneField(
                item.second, vBucket, allowEmpty, collateType, temp);
            if (errCode != E_OK) {
                LOGE("calc hash fail when there is more than one primary key. errCode = %d", errCode);
                return { errCode, {} };
            }
            tempRes.insert(tempRes.end(), temp.begin(), temp.end());
        }
        errCode = DBCommon::CalcValueHash(tempRes, hashValue);
    }
    return { errCode, hashValue };
}

void CloudStorageUtils::TransferFieldToLower(VBucket &vBucket)
{
    for (auto it = vBucket.begin(); it != vBucket.end();) {
        std::string lowerField(it->first.length(), ' ');
        std::transform(it->first.begin(), it->first.end(), lowerField.begin(), tolower);
        if (lowerField != it->first) {
            vBucket[lowerField] = std::move(vBucket[it->first]);
            vBucket.erase(it++);
        } else {
            it++;
        }
    }
}

bool CloudStorageUtils::GetTypeCaseInsensitive(const std::string &fieldName, const VBucket &vBucket, Type &data)
{
    auto tmpFieldName = fieldName;
    auto tmpVBucket = vBucket;
    std::transform(tmpFieldName.begin(), tmpFieldName.end(), tmpFieldName.begin(), tolower);
    TransferFieldToLower(tmpVBucket);
    auto it = tmpVBucket.find(tmpFieldName);
    if (it == tmpVBucket.end()) {
        return false;
    }
    data = it->second;
    return true;
}

bool CloudStorageUtils::CheckCloudSchemaFields(const TableSchema &tableSchema, const TableSchema &oldSchema)
{
    if (tableSchema.name != oldSchema.name) {
        return true;
    }
    for (const auto &oldField : oldSchema.fields) {
        auto it = std::find_if(tableSchema.fields.begin(), tableSchema.fields.end(),
            [&oldField](const std::vector<Field>::value_type &field) {
                return oldField == field;
            });
        if (it == tableSchema.fields.end()) {
            return false;
        }
    }
    return true;
}

int CloudStorageUtils::BindUpdateLogStmtFromVBucket(const VBucket &vBucket, const TableSchema &tableSchema,
    const std::vector<std::string> &colNames, sqlite3_stmt *updateLogStmt)
{
    int index = 0;
    int errCode = E_OK;
    for (const auto &colName : colNames) {
        index++;
        if (colName == CloudDbConstant::GID_FIELD) {
            if (vBucket.find(colName) == vBucket.end()) {
                LOGE("cloud data doesn't contain gid field when bind update log stmt.");
                return -E_CLOUD_ERROR;
            }
            errCode = SQLiteUtils::BindTextToStatement(updateLogStmt, index,
                std::get<std::string>(vBucket.at(colName)));
        } else if (colName == CloudDbConstant::MODIFY_FIELD) {
            if (vBucket.find(colName) == vBucket.end()) {
                LOGE("cloud data doesn't contain modify field when bind update log stmt.");
                return -E_CLOUD_ERROR;
            }
            errCode = SQLiteUtils::BindInt64ToStatement(updateLogStmt, index, std::get<int64_t>(vBucket.at(colName)));
        } else if (colName == CloudDbConstant::VERSION_FIELD) {
            if (vBucket.find(colName) == vBucket.end()) {
                LOGW("cloud data doesn't contain version field when bind update log stmt.");
                errCode = SQLiteUtils::BindTextToStatement(updateLogStmt, index, std::string(""));
            } else {
                errCode = SQLiteUtils::BindTextToStatement(updateLogStmt, index,
                    std::get<std::string>(vBucket.at(colName)));
            }
        } else if (colName == CloudDbConstant::SHARING_RESOURCE_FIELD) {
            if (vBucket.find(colName) == vBucket.end()) {
                errCode = SQLiteUtils::BindTextToStatement(updateLogStmt, index, std::string(""));
            } else {
                errCode = SQLiteUtils::BindTextToStatement(updateLogStmt, index,
                    std::get<std::string>(vBucket.at(colName)));
            }
        } else {
            LOGE("invalid col name when bind value to update log statement.");
            return -E_INTERNAL_ERROR;
        }
        if (errCode != E_OK) {
            LOGE("fail to bind value to update log statement.");
            return errCode;
        }
    }
    return E_OK;
}

std::string CloudStorageUtils::GetUpdateRecordFlagSql(const std::string &tableName, bool recordConflict,
    const LogInfo &logInfo)
{
    std::string compensatedBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_WAIT_COMPENSATED_SYNC));
    std::string consistentBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_CONSISTENCY));
    bool gidEmpty = logInfo.cloudGid.empty();
    bool isDeleted = logInfo.dataKey == DBConstant::DEFAULT_ROW_ID;
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET flag = (case when timestamp = ? then ";

    if (recordConflict && !(isDeleted && gidEmpty)) {
        sql += "flag | " + compensatedBit + " ELSE flag | " + compensatedBit;
    } else {
        sql += "flag & ~" + compensatedBit + " & ~" + consistentBit + " ELSE flag & ~" + compensatedBit;
    }
    sql += " end), status = (case when status == 2 then 3 when (status == 1 and timestamp = ?) then 0 else status end)";
    sql += " WHERE ";
    if (!gidEmpty) {
        sql += " cloud_gid = '" + logInfo.cloudGid + "'";
    }
    if (!isDeleted) {
        if (!gidEmpty) {
            sql += " OR ";
        }
        sql += " data_key = '" + std::to_string(logInfo.dataKey) + "'";
    }
    if (gidEmpty && isDeleted) {
        sql += " hash_key = ?";
    }
    sql += ";";
    return sql;
}

int CloudStorageUtils::BindStepConsistentFlagStmt(sqlite3_stmt *stmt, const VBucket &data,
    const std::set<std::string> &gidFilters)
{
    std::string gidStr;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, data, gidStr);
    if (errCode != E_OK || gidStr.empty()) {
        LOGE("Get gid from bucket fail when mark flag as consistent, errCode = %d", errCode);
        return errCode;
    }
    if (gidStr.empty()) {
        LOGE("Get empty gid from bucket when mark flag as consistent.");
        return -E_CLOUD_ERROR;
    }
    // this data has not yet downloaded asset, skipping
    if (gidFilters.find(gidStr) != gidFilters.end()) {
        return E_OK;
    }
    errCode = SQLiteUtils::BindTextToStatement(stmt, 1, gidStr); // 1 is cloud_gid
    if (errCode != E_OK) {
        LOGE("Bind cloud_gid to mark flag as consistent stmt failed, %d", errCode);
        return errCode;
    }
    int64_t modifyTime;
    errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::MODIFY_FIELD, data, modifyTime);
    if (errCode != E_OK) {
        LOGE("Get modify time from bucket fail when mark flag as consistent, errCode = %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, 2, modifyTime); // 2 is timestamp
    if (errCode != E_OK) {
        LOGE("Bind modify time to mark flag as consistent stmt failed, %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        LOGE("[Storage Executor]Step mark flag as consistent stmt failed, %d", errCode);
    }
    return errCode;
}

bool CloudStorageUtils::IsCloudGidMismatch(const std::string &downloadGid, const std::string &curGid)
{
    return !downloadGid.empty() && !curGid.empty() && downloadGid != curGid;
}

bool CloudStorageUtils::IsGetCloudDataContinue(uint32_t curNum, uint32_t curSize, uint32_t maxSize)
{
    if (curNum == 0) {
        return true;
    }
#ifdef MAX_UPLOAD_COUNT
    if (curSize < maxSize && curNum < MAX_UPLOAD_COUNT) {
        return true;
    }
#else
    if (curSize < maxSize) {
        return true;
    }
#endif
    return false;
}

int CloudStorageUtils::IdentifyCloudType(CloudSyncData &cloudSyncData, VBucket &data, VBucket &log, VBucket &flags)
{
    int64_t *rowid = std::get_if<int64_t>(&flags[CloudDbConstant::ROWID]);
    int64_t *flag = std::get_if<int64_t>(&flags[CloudDbConstant::FLAG]);
    int64_t *timeStamp = std::get_if<int64_t>(&flags[CloudDbConstant::TIMESTAMP]);
    Bytes *hashKey = std::get_if<Bytes>(&flags[CloudDbConstant::HASH_KEY]);
    int64_t *status = std::get_if<int64_t>(&flags[CloudDbConstant::STATUS]);
    if (rowid == nullptr || flag == nullptr || timeStamp == nullptr || hashKey == nullptr) {
        return -E_INVALID_DATA;
    }
    bool isDelete = ((static_cast<uint64_t>(*flag) & DataItem::DELETE_FLAG) != 0);
    bool isInsert = (!isDelete) && (log.find(CloudDbConstant::GID_FIELD) == log.end());
    if (status != nullptr && !isInsert && (CloudStorageUtils::IsDataLocked(*status))) {
        cloudSyncData.ignoredCount++;
        cloudSyncData.lockData.extend.push_back(log);
        cloudSyncData.lockData.hashKey.push_back(*hashKey);
        cloudSyncData.lockData.timestamp.push_back(*timeStamp);
        cloudSyncData.lockData.rowid.push_back(*rowid);
        return -E_IGNORE_DATA;
    }
    if (isDelete) {
        cloudSyncData.delData.record.push_back(data);
        cloudSyncData.delData.extend.push_back(log);
        cloudSyncData.delData.hashKey.push_back(*hashKey);
        cloudSyncData.delData.timestamp.push_back(*timeStamp);
        cloudSyncData.delData.rowid.push_back(*rowid);
    } else {
        if (data.empty()) {
            LOGE("The cloud data is empty, isInsert:%d", isInsert);
            return -E_INVALID_DATA;
        }
        if (IsAbnormalData(data)) {
            LOGW("This data is abnormal, ignore it when upload, isInsert:%d", isInsert);
            cloudSyncData.ignoredCount++;
            return -E_IGNORE_DATA;
        }
        CloudSyncBatch &opData = isInsert ? cloudSyncData.insData : cloudSyncData.updData;
        opData.record.push_back(data);
        opData.rowid.push_back(*rowid);
        VBucket asset;
        CloudStorageUtils::ObtainAssetFromVBucket(data, asset);
        opData.timestamp.push_back(*timeStamp);
        opData.assets.push_back(asset);
        if (isInsert) {
            log[CloudDbConstant::HASH_KEY_FIELD] = DBCommon::VectorToHexString(*hashKey);
        }
        opData.extend.push_back(log);
        opData.hashKey.push_back(*hashKey);
    }
    return E_OK;
}

bool CloudStorageUtils::IsAbnormalData(const VBucket &data)
{
    for (const auto &item : data) {
        const Asset *asset = std::get_if<TYPE_INDEX<Asset>>(&item.second);
        if (asset != nullptr) {
            if (asset->status == static_cast<uint32_t>(AssetStatus::ABNORMAL) ||
                (asset->status & static_cast<uint32_t>(AssetStatus::DOWNLOAD_WITH_NULL)) != 0) {
                return true;
            }
            continue;
        }
        const Assets *assets = std::get_if<TYPE_INDEX<Assets>>(&item.second);
        if (assets == nullptr) {
            continue;
        }
        for (const auto &oneAsset : *assets) {
            if (oneAsset.status == static_cast<uint32_t>(AssetStatus::ABNORMAL) ||
                (oneAsset.status & static_cast<uint32_t>(AssetStatus::DOWNLOAD_WITH_NULL)) != 0) {
                return true;
            }
        }
    }
    return false;
}

std::pair<int, DataItem> CloudStorageUtils::GetDataItemFromCloudData(VBucket &data)
{
    std::pair<int, DataItem> res;
    auto &[errCode, dataItem] = res;
    GetBytesFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_KEY, data, dataItem.key);
    GetBytesFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_VALUE, data, dataItem.value);
    GetStringFromCloudData(CloudDbConstant::GID_FIELD, data, dataItem.gid);
    GetStringFromCloudData(CloudDbConstant::VERSION_FIELD, data, dataItem.version);
    GetStringFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_DEVICE, data, dataItem.dev);
    GetStringFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE, data, dataItem.origDev);
    dataItem.flag = static_cast<uint64_t>(LogInfoFlag::FLAG_CLOUD_WRITE);
    GetUInt64FromCloudData(CloudDbConstant::CLOUD_KV_FIELD_DEVICE_CREATE_TIME, data, dataItem.writeTimestamp);
    GetUInt64FromCloudData(CloudDbConstant::MODIFY_FIELD, data, dataItem.modifyTime);
    errCode = GetUInt64FromCloudData(CloudDbConstant::CREATE_FIELD, data, dataItem.createTime);
    bool isSystemRecord = IsSystemRecord(dataItem.key);
    if (isSystemRecord) {
        dataItem.hashKey = dataItem.key;
        dataItem.flag |= static_cast<uint64_t>(LogInfoFlag::FLAG_SYSTEM_RECORD);
    } else {
        (void)DBCommon::CalcValueHash(dataItem.key, dataItem.hashKey);
    }
    return res;
}

std::pair<int, DataItem> CloudStorageUtils::GetDataItemFromCloudVersionData(VBucket &data)
{
    std::pair<int, DataItem> res;
    auto &[errCode, dataItem] = res;
    GetBytesFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_KEY, data, dataItem.key);
    GetBytesFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_VALUE, data, dataItem.value);
    GetStringFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_DEVICE, data, dataItem.dev);
    errCode = E_OK;
    return res;
}

int CloudStorageUtils::GetBytesFromCloudData(const std::string &field, VBucket &data, Bytes &bytes)
{
    std::string blobStr;
    int errCode = GetValueFromVBucket(field, data, blobStr);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[CloudStorageUtils] Get %.3s failed %d", field.c_str(), errCode);
        return errCode;
    }
    DBCommon::StringToVector(blobStr, bytes);
    return errCode;
}

int CloudStorageUtils::GetStringFromCloudData(const std::string &field, VBucket &data, std::string &str)
{
    int errCode = GetValueFromVBucket(field, data, str);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[CloudStorageUtils] Get %.3s failed %d", field.c_str(), errCode);
        return errCode;
    }
    return errCode;
}

int CloudStorageUtils::GetUInt64FromCloudData(const std::string &field, VBucket &data, uint64_t &number)
{
    int64_t intNum;
    int errCode = GetValueFromVBucket(field, data, intNum);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[CloudStorageUtils] Get %.3s failed %d", field.c_str(), errCode);
        return errCode;
    }
    number = static_cast<uint64_t>(intNum);
    return errCode;
}

void CloudStorageUtils::AddUpdateColForShare(const TableSchema &tableSchema, std::string &updateLogSql,
    std::vector<std::string> &updateColName)
{
    updateLogSql += ", version = ?";
    updateColName.push_back(CloudDbConstant::VERSION_FIELD);
    updateLogSql += ", sharing_resource = ?";
    updateColName.push_back(CloudDbConstant::SHARING_RESOURCE_FIELD);
}

bool CloudStorageUtils::IsDataLocked(uint32_t status)
{
    return status == static_cast<uint32_t>(LockStatus::LOCK) ||
        status == static_cast<uint32_t>(LockStatus::LOCK_CHANGE);
}

std::pair<int, DataItem> CloudStorageUtils::GetSystemRecordFromCloudData(VBucket &data)
{
    auto res = CloudStorageUtils::GetDataItemFromCloudData(data); // only record first one
    auto &[errCode, dataItem] = res;
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get data item failed %d", errCode);
        return res;
    }
    dataItem.dev = "";
    dataItem.origDev = "";
    return res;
}

bool CloudStorageUtils::IsSystemRecord(const Key &key)
{
    std::string prefixKey = CloudDbConstant::CLOUD_VERSION_RECORD_PREFIX_KEY;
    if (key.size() < prefixKey.size()) {
        return false;
    }
    std::string keyStr(key.begin(), key.end());
    return keyStr.find(prefixKey) == 0;
}
}
