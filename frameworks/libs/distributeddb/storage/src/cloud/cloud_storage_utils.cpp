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
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "db_common.h"
#include "runtime_context.h"

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
        uint32_t lowStatus = AssetOperationUtils::EraseBitMask(asset.status);
        if ((asset.flag == static_cast<uint32_t>(AssetOpType::DELETE) && (lowStatus == AssetStatus::ABNORMAL ||
            lowStatus == AssetStatus::NORMAL)) || asset.flag != static_cast<uint32_t>(AssetOpType::DELETE)) {
            return std::move(asset);
        }
    }
    return Nil();
}

int CloudStorageUtils::FillAssetBeforeDownload(Asset &asset)
{
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
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
        // Notes: Assign happened when dbAsset.assetId is empty because of asset.assetId may be empty.
        if (dbAsset.assetId.empty()) {
            dbAsset.assetId = asset.assetId;
        }
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
        if (downloadItem == downloadAssets.end()) { // LCOV_EXCL_BR_LINE
            continue;
        }
        std::map<std::string, size_t> beCoveredAssetsMap = GenAssetsIndexMap(items.second);
        for (const Asset &asset: downloadItem->second) {
            auto it = beCoveredAssetsMap.find(asset.name);
            if (it == beCoveredAssetsMap.end()) { // LCOV_EXCL_BR_LINE
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
            if (itOp != col.second.end() && itOp->second == AssetOperationUtils::AssetOpType::NOT_HANDLE
                && delAsset.flag != static_cast<uint32_t>(AssetOpType::DELETE)) {
                removeAssets.push_back(delAsset);
            }
            continue;
        }
        Assets assets;
        GetValueFromType(itItem, assets);
        for (const auto &asset: assets) {
            auto itOp = col.second.find(asset.name);
            if (itOp == col.second.end() || itOp->second == AssetOperationUtils::AssetOpType::HANDLE ||
                asset.flag == static_cast<uint32_t>(AssetOpType::DELETE)) {
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
    dbAsset = asset;
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

std::string CloudStorageUtils::GetUpdateRecordFlagSql(UpdateRecordFlagStruct updateRecordFlag, const LogInfo &logInfo,
    const VBucket &uploadExtend, const CloudWaterType &type)
{
    std::string compensatedBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_WAIT_COMPENSATED_SYNC));
    std::string inconsistencyBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY));
    bool gidEmpty = logInfo.cloudGid.empty();
    bool isDeleted = logInfo.dataKey == DBConstant::DEFAULT_ROW_ID;
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(updateRecordFlag.tableName) +
        " SET flag = (CASE WHEN timestamp = ? THEN ";
    bool isNeedCompensated =
        updateRecordFlag.isRecordConflict || DBCommon::IsNeedCompensatedForUpload(uploadExtend, type);
    if (isNeedCompensated && !(isDeleted && gidEmpty)) {
        sql += "flag | " + compensatedBit + " ELSE flag | " + compensatedBit;
    } else {
        if (updateRecordFlag.isInconsistency) {
            sql += "flag & ~" + compensatedBit + " |" + inconsistencyBit + " ELSE flag & ~" + compensatedBit;
        } else {
            sql += "flag & ~" + compensatedBit + " & ~" + inconsistencyBit + " ELSE flag & ~" + compensatedBit;
        }
    }
    sql += " END), status = (CASE WHEN status == 2 THEN 3 WHEN (status == 1 AND timestamp = ?) THEN 0 ELSE status END)";
    if (DBCommon::IsCloudRecordNotFound(uploadExtend) &&
        (type == CloudWaterType::UPDATE || type == CloudWaterType::DELETE)) {
        sql += ", cloud_gid = '', version = '' ";
    }
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

std::string CloudStorageUtils::GetUpdateRecordFlagSqlUpload(const std::string &tableName, bool recordConflict,
    const LogInfo &logInfo, const VBucket &uploadExtend, const CloudWaterType &type)
{
    std::string compensatedBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_WAIT_COMPENSATED_SYNC));
    std::string inconsistencyBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_DEVICE_CLOUD_INCONSISTENCY));
    bool gidEmpty = logInfo.cloudGid.empty();
    bool isDeleted = logInfo.dataKey == DBConstant::DEFAULT_ROW_ID;
    std::string sql;
    bool isNeedCompensated = recordConflict || DBCommon::IsNeedCompensatedForUpload(uploadExtend, type);
    if (isNeedCompensated && !(isDeleted && gidEmpty)) {
        sql += "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET flag = (CASE WHEN timestamp = ? OR " +
            "flag & 0x01 = 0 THEN flag | " + compensatedBit + " ELSE flag";
    } else {
        sql += "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET flag = (CASE WHEN timestamp = ? THEN " +
            "flag & ~" + compensatedBit + " & ~" + inconsistencyBit + " ELSE flag & ~" + compensatedBit;
    }
    sql += " END), status = (CASE WHEN status == 2 THEN 3 WHEN (status == 1 AND timestamp = ?) THEN 0 ELSE status END)";
    if (DBCommon::IsCloudRecordNotFound(uploadExtend) &&
        (type == CloudWaterType::UPDATE || type == CloudWaterType::DELETE)) {
        sql += ", cloud_gid = '', version = '' ";
    }
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

std::string CloudStorageUtils::GetUpdateUploadFinishedSql(const std::string &tableName, bool isExistAssetsDownload)
{
    std::string finishedBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_UPLOAD_FINISHED));
    std::string compensatedBit = std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_WAIT_COMPENSATED_SYNC));
    // When the data flag is not in the compensation state and the local data does not change, the upload is finished.
    if (isExistAssetsDownload) {
        return "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET flag = (CASE WHEN timestamp = ? AND flag & " +
            compensatedBit + " != " + compensatedBit + " THEN flag | " + finishedBit +
            " ELSE flag END) WHERE hash_key=?";
    }
    // Clear IsWaitAssetDownload flag when no asset is to be downloaded
    return "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET flag = ((CASE WHEN timestamp = ? AND flag & " +
        compensatedBit + " != " + compensatedBit + " THEN flag | " + finishedBit +
        " ELSE flag END) & ~0x1000) WHERE hash_key=?";
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

bool CloudStorageUtils::IsGetCloudDataContinue(uint32_t curNum, uint32_t curSize, uint32_t maxSize, uint32_t maxCount)
{
    if (curNum == 0) {
        return true;
    }
    if (curSize < maxSize && curNum < maxCount) {
        return true;
    }
    return false;
}

int CloudStorageUtils::IdentifyCloudType(const CloudUploadRecorder &recorder, CloudSyncData &cloudSyncData,
    VBucket &data, VBucket &log, VBucket &flags)
{
    Bytes *hashKey = std::get_if<Bytes>(&flags[CloudDbConstant::HASH_KEY]);
    int64_t *timeStamp = std::get_if<int64_t>(&flags[CloudDbConstant::TIMESTAMP]);
    if (timeStamp == nullptr || hashKey == nullptr) {
        return -E_INVALID_DATA;
    }
    if (recorder.IsIgnoreUploadRecord(cloudSyncData.tableName, *hashKey, cloudSyncData.mode, *timeStamp)) {
        cloudSyncData.ignoredCount++;
        return -E_IGNORE_DATA;
    }
    return IdentifyCloudTypeInner(cloudSyncData, data, log, flags);
}

int CloudStorageUtils::IdentifyCloudTypeInner(CloudSyncData &cloudSyncData, VBucket &data, VBucket &log, VBucket &flags)
{
    int64_t *rowid = std::get_if<int64_t>(&flags[DBConstant::ROWID]);
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
        bool isAsyncDownloading = ((static_cast<uint64_t>(*flag) &
            static_cast<uint64_t>(LogInfoFlag::FLAG_ASSET_DOWNLOADING_FOR_ASYNC)) != 0);
        int errCode = CheckAbnormalData(cloudSyncData, data, isInsert, isAsyncDownloading);
        if (errCode != E_OK) {
            return errCode;
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

bool CloudStorageUtils::IsAssetNotDownload(const uint32_t &status)
{
    return status == static_cast<uint32_t>(AssetStatus::ABNORMAL) ||
        status == static_cast<uint32_t>(AssetStatus::DOWNLOADING) ||
        (status & static_cast<uint32_t>(AssetStatus::DOWNLOAD_WITH_NULL)) != 0;
}

void CloudStorageUtils::CheckAbnormalDataInner(const bool isAsyncDownloading, VBucket &data,
    bool &isSyncAssetAbnormal, bool &isAsyncAssetAbnormal)
{
    std::vector<std::string> abnormalAssetFields;
    for (auto &item : data) {
        const Asset *asset = std::get_if<TYPE_INDEX<Asset>>(&item.second);
        if (asset != nullptr) {
            bool isAssetNotDownload = IsAssetNotDownload(asset->status);
            if (!isAssetNotDownload) {
                continue;
            }
            if (!isAsyncDownloading) {
                isSyncAssetAbnormal = true;
                return;
            }
            isAsyncAssetAbnormal = true;
            abnormalAssetFields.push_back(item.first);
            continue;
        }
        auto *assets = std::get_if<TYPE_INDEX<Assets>>(&item.second);
        if (assets == nullptr) {
            continue;
        }
        for (auto it = assets->begin(); it != assets->end();) {
            const auto &oneAsset = *it;
            bool isOneAssetNotDownload = IsAssetNotDownload(oneAsset.status);
            if (!isOneAssetNotDownload) {
                ++it;
                continue;
            }
            if (!isAsyncDownloading) {
                isSyncAssetAbnormal = true;
                return;
            }
            isAsyncAssetAbnormal = true;
            it = assets->erase(it);
        }
    }
    for (const auto &item : abnormalAssetFields) {
        data.erase(item);
    }
}

int CloudStorageUtils::CheckAbnormalData(CloudSyncData &cloudSyncData, VBucket &data, bool isInsert,
    bool isAsyncDownloading)
{
    if (data.empty()) {
        LOGE("The cloud data is empty, isInsert:%d", static_cast<int>(isInsert));
        return -E_INVALID_DATA;
    }
    bool isSyncAssetAbnormal = false;
    bool isAsyncAssetAbnormal = false;
    CheckAbnormalDataInner(isAsyncDownloading, data, isSyncAssetAbnormal, isAsyncAssetAbnormal);
    if (isSyncAssetAbnormal) {
        std::string gid;
        (void)GetValueFromVBucket(CloudDbConstant::GID_FIELD, data, gid);
        LOGW("This data is abnormal, ignore it when upload, isInsert:%d, gid:%s",
            static_cast<int>(isInsert), gid.c_str());
        cloudSyncData.ignoredCount++;
        return -E_IGNORE_DATA;
    }
    if (isAsyncAssetAbnormal) {
        std::string gid;
        (void)GetValueFromVBucket(CloudDbConstant::GID_FIELD, data, gid);
        LOGW("This data has assets that are not downloaded, upload data only, gid:%s", gid.c_str());
    }
    return E_OK;
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
    } else if (!dataItem.key.empty()) {
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
    errCode = GetStringFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_DEVICE, data, dataItem.dev);
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

std::string CloudStorageUtils::GetCursorUpgradeSql(const std::string &tableName)
{
    return "INSERT OR REPLACE INTO " + DBCommon::GetMetaTableName() + "(key,value) VALUES (x'" +
        DBCommon::TransferStringToHex(DBCommon::GetCursorKey(tableName)) + "', (SELECT CASE WHEN MAX(cursor) IS" +
        " NULL THEN 0 ELSE MAX(cursor) END FROM " + DBCommon::GetLogTableName(tableName) + "));";
}

int CloudStorageUtils::FillQueryByPK(const std::string &tableName, bool isKv, std::map<std::string, size_t> dataIndex,
    std::vector<std::map<std::string, std::vector<Type>>> &syncPkVec, std::vector<QuerySyncObject> &syncQuery)
{
    for (const auto &syncPk : syncPkVec) {
        Query query = Query::Select().From(tableName);
        if (isKv) {
            QueryUtils::FillQueryInKeys(syncPk, dataIndex, query);
        } else {
            for (const auto &[col, pkList] : syncPk) {
                QueryUtils::FillQueryIn(col, pkList, dataIndex[col], query);
            }
        }
        auto objectList = QuerySyncObject::GetQuerySyncObject(query);
        if (objectList.size() != 1u) { // only support one QueryExpression
            return -E_INTERNAL_ERROR;
        }
        syncQuery.push_back(objectList[0]);
    }
    return E_OK;
}

void CloudStorageUtils::PutSyncPkVec(const std::string &col, std::map<std::string, std::vector<Type>> &syncPk,
    std::vector<std::map<std::string, std::vector<Type>>> &syncPkVec)
{
    if (syncPk[col].size() >= CloudDbConstant::MAX_CONDITIONS_SIZE) {
        syncPkVec.push_back(syncPk);
        syncPk[col].clear();
    }
}

int CloudStorageUtils::GetSyncQueryByPk(const std::string &tableName, const std::vector<VBucket> &data, bool isKv,
    std::vector<QuerySyncObject> &syncQuery)
{
    std::map<std::string, size_t> dataIndex;
    std::vector<std::map<std::string, std::vector<Type>>> syncPkVec;
    std::map<std::string, std::vector<Type>> syncPk;
    int ignoreCount = 0;
    for (const auto &oneRow : data) {
        if (oneRow.size() >= 2u) { // mean this data has more than 2 pk
            LOGW("compensated sync does not support composite PK, oneRow size: %zu, tableName: %s",
                oneRow.size(), DBCommon::StringMiddleMasking(tableName).c_str());
            return -E_NOT_SUPPORT;
        }
        for (const auto &[col, value] : oneRow) {
            bool isFind = dataIndex.find(col) != dataIndex.end();
            if (!isFind && value.index() == TYPE_INDEX<Nil>) {
                ignoreCount++;
                continue;
            }
            if (!isFind && value.index() != TYPE_INDEX<Nil>) {
                dataIndex[col] = value.index();
                syncPk[col].push_back(value);
                PutSyncPkVec(col, syncPk, syncPkVec);
                continue;
            }
            if (isFind && dataIndex[col] != value.index()) {
                ignoreCount++;
                continue;
            }
            syncPk[col].push_back(value);
            PutSyncPkVec(col, syncPk, syncPkVec);
        }
    }
    syncPkVec.push_back(syncPk);
    LOGI("match %zu data for compensated sync, ignore %d", data.size(), ignoreCount);
    return FillQueryByPK(tableName, isKv, dataIndex, syncPkVec, syncQuery);
}

bool CloudStorageUtils::IsAssetsContainDownloadRecord(const VBucket &dbAssets)
{
    for (const auto &item: dbAssets) {
        if (IsAsset(item.second)) {
            const auto &asset = std::get<Asset>(item.second);
            if (AssetOperationUtils::IsAssetNeedDownload(asset)) {
                return true;
            }
        } else if (IsAssets(item.second)) {
            const auto &assets = std::get<Assets>(item.second);
            if (AssetOperationUtils::IsAssetsNeedDownload(assets)) {
                return true;
            }
        }
    }
    return false;
}

int CloudStorageUtils::UpdateRecordFlagAfterUpload(SQLiteSingleVerRelationalStorageExecutor *handle,
    const CloudSyncParam &param, const CloudSyncBatch &updateData, CloudUploadRecorder &recorder, bool isLock)
{
    if (updateData.timestamp.size() != updateData.extend.size()) {
        LOGE("the num of extend:%zu and timestamp:%zu is not equal.",
            updateData.extend.size(), updateData.timestamp.size());
        return -E_INVALID_ARGS;
    }
    for (size_t i = 0; i < updateData.extend.size(); ++i) {
        const auto &record = updateData.extend[i];
        if (DBCommon::IsRecordError(record) || DBCommon::IsRecordAssetsMissing(record) ||
            DBCommon::IsRecordVersionConflict(record) || isLock) {
            if (DBCommon::IsRecordAssetsMissing(record)) {
                LOGI("[CloudStorageUtils][UpdateRecordFlagAfterUpload] Record assets missing, skip update.");
            }
            int errCode = handle->UpdateRecordStatus(param.tableName, CloudDbConstant::TO_LOCAL_CHANGE,
                updateData.hashKey[i]);
            if (errCode != E_OK) {
                LOGE("[CloudStorageUtils] Update record status failed in index %zu", i);
                return errCode;
            }
            continue;
        }
        const auto &rowId = updateData.rowid[i];
        std::string cloudGid;
        (void)CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, record, cloudGid);
        LogInfo logInfo;
        logInfo.cloudGid = cloudGid;
        logInfo.timestamp = updateData.timestamp[i];
        logInfo.dataKey = rowId;
        logInfo.hashKey = updateData.hashKey[i];
        std::string sql = CloudStorageUtils::GetUpdateRecordFlagSqlUpload(
            param.tableName, DBCommon::IsRecordIgnored(record), logInfo, record, param.type);
        int errCode = handle->UpdateRecordFlag(param.tableName, sql, logInfo);
        if (errCode != E_OK) {
            LOGE("[CloudStorageUtils] Update record flag failed in index %zu", i);
            return errCode;
        }

        std::vector<VBucket> assets;
        errCode = handle->GetDownloadAssetRecordsByGid(param.tableSchema, logInfo.cloudGid, assets);
        if (errCode != E_OK) {
            LOGE("[RDBExecutor]Get downloading assets records by gid failed: %d", errCode);
            return errCode;
        }
        handle->MarkFlagAsUploadFinished(param.tableName, updateData.hashKey[i], updateData.timestamp[i],
            !assets.empty());
        recorder.RecordUploadRecord(param.tableName, logInfo.hashKey, param.type, updateData.timestamp[i]);
    }
    return E_OK;
}

int CloudStorageUtils::FillCloudQueryToExtend(QuerySyncObject &obj, VBucket &extend)
{
    Bytes bytes;
    bytes.resize(obj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel parcel(bytes.data(), bytes.size());
    int errCode = obj.SerializeData(parcel, SOFTWARE_VERSION_CURRENT);
    if (errCode != E_OK) {
        LOGE("[CloudStorageUtils] Query serialize failed %d", errCode);
        return errCode;
    }
    extend[CloudDbConstant::TYPE_FIELD] = static_cast<int64_t>(CloudQueryType::QUERY_FIELD);
    extend[CloudDbConstant::QUERY_FIELD] = bytes;
    return E_OK;
}

void CloudStorageUtils::SaveChangedDataByType(const DataValue &dataValue, Type &value)
{
    int ret = E_OK;
    switch (dataValue.GetType()) {
        case StorageType::STORAGE_TYPE_TEXT:
            {
                std::string sValue;
                ret = dataValue.GetText(sValue);
                if (ret != E_OK) {
                    LOGE("[CloudStorageUtils] save changed string data failed %d", ret);
                    return;
                }
                value = sValue;
            } break;
        case StorageType::STORAGE_TYPE_BLOB:
            {
                Blob blob;
                (void)dataValue.GetBlob(blob);
                if (blob.GetSize() == 0u) {
                    LOGE("[CloudStorageUtils] save changed Blob data failed");
                    return;
                }
                value = std::vector<uint8_t>(blob.GetData(), blob.GetData() + blob.GetSize());
            } break;
        case StorageType::STORAGE_TYPE_INTEGER:
            {
                int64_t iValue;
                ret = dataValue.GetInt64(iValue);
                if (ret != E_OK) {
                    LOGE("[CloudStorageUtils] save changed int64 data failed %d", ret);
                    return;
                }
                value = iValue;
            } break;
        case StorageType::STORAGE_TYPE_REAL:
            {
                double dValue;
                ret = dataValue.GetDouble(dValue);
                if (ret != E_OK) {
                    LOGE("[CloudStorageUtils] save changed double data failed %d", ret);
                    return;
                }
                value = dValue;
            } break;
        default:
            LOGE("[CloudStorageUtils] save changed failed, wrong storage type :%" PRIu32, dataValue.GetType());
            return;
    }
}
}
