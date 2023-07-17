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
#include <set>

#include "cloud/cloud_db_types.h"
#include "db_common.h"
#include "runtime_context.h"
#include "cloud/cloud_storage_utils.h"

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
    auto entry = vBucket.find(field.colName);
    if (entry == vBucket.end() || entry->second.index() == TYPE_INDEX<Nil>) {
        if (!field.nullable) {
            LOGE("field value is not allowed to be null, %d", -E_CLOUD_ERROR);
            return -E_CLOUD_ERROR;
        }
        return SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    }

    Type type = entry->second;
    if (field.type == TYPE_INDEX<Asset>) {
        Asset asset;
        errCode = GetValueFromOneField(type, asset);
        if (errCode != E_OK) {
            LOGE("can not get asset from vBucket when bind, %d", errCode);
            return errCode;
        }
        errCode = RuntimeContext::GetInstance()->AssetToBlob(asset, val);
    } else if (field.type == TYPE_INDEX<Assets>) {
        Assets assets;
        errCode = GetValueFromOneField(type, assets);
        if (errCode != E_OK) {
            LOGE("can not get assets from vBucket when bind, %d", errCode);
            return errCode;
        }
        if (!assets.empty()) {
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

int CloudStorageUtils::Int64ToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value)
{
    int64_t val = 0;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    DBCommon::StringToVector(std::to_string(val), value);
    return E_OK;
}

int CloudStorageUtils::BoolToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value)
{
    bool val = false;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    DBCommon::StringToVector(std::to_string(val ? 1 : 0), value);
    return E_OK;
}

int CloudStorageUtils::DoubleToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value)
{
    double val = 0.0;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    std::ostringstream s;
    s << val;
    DBCommon::StringToVector(s.str(), value);
    return E_OK;
}

int CloudStorageUtils::TextToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value)
{
    std::string val;
    if (CloudStorageUtils::GetValueFromVBucket(field.colName, vBucket, val) != E_OK) {
        return -E_CLOUD_ERROR;
    }
    DBCommon::StringToVector(val, value);
    return E_OK;
}

int CloudStorageUtils::BlobToVector(const VBucket &vBucket, const Field &field, std::vector<uint8_t> &value)
{
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

std::vector<Field> CloudStorageUtils::GetCloudPrimaryKeyField(const TableSchema &tableSchema)
{
    std::vector<Field> pkVec;
    for (const auto &field : tableSchema.fields) {
        if (field.primary) {
            pkVec.push_back(field);
        }
    }
    return pkVec;
}

std::map<std::string, Field> CloudStorageUtils::GetCloudPrimaryKeyFieldMap(const TableSchema &tableSchema)
{
    std::map<std::string, Field> pkMap;
    for (const auto &field : tableSchema.fields) {
        if (field.primary) {
            pkMap[field.colName] = field;
        }
    }
    return pkMap;
}

int CloudStorageUtils::GetAssetFieldsFromSchema(const TableSchema &tableSchema, VBucket &vBucket,
    std::vector<Field> &fields)
{
    for (const auto &field: tableSchema.fields) {
        auto it = vBucket.find(field.colName);
        if (it == vBucket.end()) {
            continue;
        }
        if (it->second.index() != TYPE_INDEX<Asset> && it->second.index() != TYPE_INDEX<Assets>) {
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
    switch (status) {
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

void CloudStorageUtils::FillAssetBeforeDownload(Asset &asset)
{
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
    AssetStatus status = static_cast<AssetStatus>(asset.status);
    switch (flag) {
        case AssetOpType::INSERT: {
            if (status != AssetStatus::NORMAL) {
                asset.hash = std::string("");
            }
            break;
        }
        default:
            break;
    }
}

void CloudStorageUtils::FillAssetAfterDownloadFail(Asset &asset)
{
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
    AssetStatus status = static_cast<AssetStatus>(asset.status);
    switch (flag) {
        case AssetOpType::INSERT:
        case AssetOpType::DELETE:
        case AssetOpType::UPDATE: {
            if (status != AssetStatus::NORMAL) {
                asset.hash = std::string("");
                asset.status = static_cast<uint32_t>(AssetStatus::ABNORMAL);
            }
            break;
        }
        default:
            break;
    }
}

int CloudStorageUtils::FillAssetAfterDownload(Asset &asset)
{
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
    switch (flag) {
        case AssetOpType::INSERT:
        case AssetOpType::UPDATE: {
            asset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
            break;
        }
        case AssetOpType::DELETE: {
            return -E_NOT_FOUND;
        }
        default:
            break;
    }
    return E_OK;
}

void CloudStorageUtils::FillAssetsAfterDownload(Assets &assets)
{
    for (auto asset = assets.begin(); asset != assets.end();) {
        if (FillAssetAfterDownload(*asset) == -E_NOT_FOUND) {
            asset = assets.erase(asset);
        } else {
            asset++;
        }
    }
}

int CloudStorageUtils::FillAssetForUpload(Asset &asset)
{
    AssetStatus status = static_cast<AssetStatus>(asset.status);
    switch (StatusToFlag(status)) {
        case AssetOpType::INSERT:
        case AssetOpType::UPDATE: {
            asset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
            break;
        }
        case AssetOpType::DELETE: {
            return -E_NOT_FOUND;
        }
        default: {
            break;
        }
    }
    return E_OK;
}

void CloudStorageUtils::FillAssetsForUpload(Assets &assets)
{
    for (auto asset = assets.begin(); asset != assets.end();) {
        if (FillAssetForUpload(*asset) == -E_NOT_FOUND) {
            asset = assets.erase(asset);
        } else {
            asset++;
        }
    }
}

void CloudStorageUtils::PrepareToFillAssetFromVBucket(VBucket &vBucket, std::function<void(Asset &)> fillAsset)
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
            for (auto &asset: assets) {
                fillAsset(asset);
            }
            vBucket[item.first] = assets;
        }
    }
}

void CloudStorageUtils::FillAssetFromVBucketFinish(VBucket &vBucket, std::function<int(Asset &)> fillAsset,
    std::function<void(Assets &)> fillAssets)
{
    for (auto &item: vBucket) {
        if (IsAsset(item.second)) {
            Asset asset;
            GetValueFromType(item.second, asset);
            int errCode = fillAsset(asset);
            if (errCode != E_OK) {
                vBucket[item.first] = Nil();
            } else {
                vBucket[item.first] = asset;
            }
            continue;
        }
        if (IsAssets(item.second)) {
            Assets assets;
            GetValueFromType(item.second, assets);
            fillAssets(assets);
            if (assets.empty()) {
                vBucket[item.first] = Nil();
            } else {
                vBucket[item.first] = assets;
            }
        }
    }
}

bool CloudStorageUtils::IsAsset(const Type &type)
{
    if (type.index() == TYPE_INDEX<Asset>) {
        return true;
    }
    return false;
}

bool CloudStorageUtils::IsAssets(const Type &type)
{
    if (type.index() == TYPE_INDEX<Assets>) {
        return true;
    }
    return false;
}

int CloudStorageUtils::CalculateHashKeyForOneField(const Field &field, const VBucket &vBucket, bool allowEmpty,
    std::vector<uint8_t> &hashValue)
{
    if (allowEmpty && vBucket.find(field.colName) == vBucket.end()) {
        return E_OK; // if vBucket from cloud doesn't contain primary key and allowEmpty, no need to calculate hash
    }
    static std::map<int32_t, std::function<int(const VBucket &, const Field &, std::vector<uint8_t> &)>> toVecFunc = {
        {TYPE_INDEX<int64_t>, &CloudStorageUtils::Int64ToVector},
        {TYPE_INDEX<bool>, &CloudStorageUtils::BoolToVector},
        {TYPE_INDEX<double>, &CloudStorageUtils::DoubleToVector},
        {TYPE_INDEX<std::string>, &CloudStorageUtils::TextToVector},
        {TYPE_INDEX<Bytes>, &CloudStorageUtils::BlobToVector},
        {TYPE_INDEX<Asset>, &CloudStorageUtils::BlobToVector},
        {TYPE_INDEX<Assets>, &CloudStorageUtils::BlobToVector},
    };
    auto it = toVecFunc.find(field.type);
    if (it == toVecFunc.end()) {
        LOGE("unknown cloud type when convert field to vector.");
        return -E_CLOUD_ERROR;
    }
    std::vector<uint8_t> value;
    int errCode = it->second(vBucket, field, value);
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
        if (vBucket.find(pk) == vBucket.end()) {
            return false;
        }
    }
    return true;
}

static bool IsViolationOfConstraints(const std::string &name, const std::vector<FieldInfo> &fieldInfos)
{
    for (const auto &field : fieldInfos) {
        if (name == field.GetFieldName()) {
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
        if (asset.status > static_cast<uint32_t>(AssetStatus::UPDATE)) {
            LOGE("assets contain invalid status:[%u]", asset.status);
            return false;
        }
    }
    return true;
}
}
