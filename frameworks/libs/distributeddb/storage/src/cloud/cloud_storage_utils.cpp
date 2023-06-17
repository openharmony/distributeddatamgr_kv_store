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
    if (field.type == TYPE_INDEX<Asset>) {
        Asset asset;
        errCode = GetValueFromVBucket(field.colName, vBucket, asset);
        if (!(IsFieldValid(field, errCode))) {
            goto ERROR;
        }
        errCode = RebuildFillAsset(asset);
        if (errCode == E_OK) {
            RuntimeContext::GetInstance()->AssetToBlob(asset, val);
        }
    } else {
        Assets assets;
        errCode = GetValueFromVBucket(field.colName, vBucket, assets);
        if (!(IsFieldValid(field, errCode))) {
            goto ERROR;
        }
        RebuildFillAssets(assets);
        if (assets.empty()) {
            errCode = -E_NOT_FOUND;
        } else {
            RuntimeContext::GetInstance()->AssetsToBlob(assets, val);
        }
    }
    if (errCode == -E_NOT_FOUND) {
        errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(upsertStmt, index));
    } else {
        errCode = SQLiteUtils::BindBlobToStatement(upsertStmt, index, val);
    }
    if (errCode != E_OK) {
        LOGE("Bind blob to asset failed, %d", errCode);
    }
    return errCode;
ERROR:
    LOGE("get Asset from vbucket failed, %d", errCode);
    return -E_CLOUD_ERROR;
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

int CloudStorageUtils::RebuildFillAsset(Asset &asset)
{
    AssetOpType flag = static_cast<AssetOpType>(asset.flag);
    AssetStatus status = static_cast<AssetStatus>(asset.status);
    switch (status) {
        case AssetStatus::ABNORMAL:
        case AssetStatus::DOWNLOADING: {
            if (flag == AssetOpType::INSERT) {
                Asset insAsset;
                insAsset.name = asset.name;
                insAsset.status = asset.status;
                insAsset.timestamp = asset.timestamp;
                asset = insAsset;
            }
            break;
        }
        case AssetStatus::INSERT:
        case AssetStatus::UPDATE: {
            asset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
            break;
        }
        case AssetStatus::DELETE: {
            return -E_NOT_FOUND;
        }
        default:
            break;
    }
    return E_OK;
}

void CloudStorageUtils::RebuildFillAssets(Assets &assets)
{
    for (auto asset = assets.begin(); asset != assets.end();) {
        RebuildFillAsset(*asset);
        AssetOpType flag = static_cast<AssetOpType>(asset->flag);
        AssetStatus status = static_cast<AssetStatus>(asset->status);
        if ((flag == AssetOpType::DELETE && status == AssetStatus::NORMAL) || status == AssetStatus::DELETE) {
            asset = assets.erase(asset);
        } else {
            asset++;
        }
    }
}

int CloudStorageUtils::CheckAssetFromSchema(const TableSchema &tableSchema, VBucket &vBucket,
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
        if (field.type == TYPE_INDEX<Asset>) {
            auto assets = std::get_if<Assets>(&it->second);
            if (assets != nullptr && assets->size() == 1) {
                Asset asset = (*assets)[0];
                vBucket[field.colName] = asset;
            }
        }
        fields.push_back(field);
    }
    if (fields.size() == 0) {
        LOGE("No assets need to be filled.");
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
        if (item.second.index() == TYPE_INDEX<Asset>) {
            Asset data = std::get<Asset>(item.second);
            asset.insert_or_assign(item.first, data);
        } else if (item.second.index() == TYPE_INDEX<Assets>) {
            Assets data = std::get<Assets>(item.second);
            asset.insert_or_assign(item.first, data);
        }
    }
}
}
