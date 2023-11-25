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
#include "virtual_cloud_data_translate.h"

#include "parcel.h"

namespace DistributedDB {
uint32_t CalculateLens(const Asset &asset)
{
    uint32_t len = 0;
    len += Parcel::GetUInt32Len();
    len += Parcel::GetStringLen(asset.name);
    len += Parcel::GetStringLen(asset.assetId);
    len += Parcel::GetStringLen(asset.uri);
    len += Parcel::GetStringLen(asset.modifyTime);
    len += Parcel::GetStringLen(asset.createTime);
    len += Parcel::GetStringLen(asset.size);
    len += Parcel::GetStringLen(asset.hash);
    len += Parcel::GetInt64Len();
    len += Parcel::GetInt64Len();
    return len;
}

uint32_t CalculateLens(const Assets &assets)
{
    uint32_t len = 0;
    for (const auto &asset : assets) {
        len += CalculateLens(asset);
    }
    return len;
}

void WriteAsset(Parcel &parcel, const Asset &asset)
{
    parcel.WriteUInt32(asset.version);
    parcel.WriteString(asset.name);
    parcel.WriteString(asset.assetId);
    parcel.WriteString(asset.uri);
    parcel.WriteString(asset.modifyTime);
    parcel.WriteString(asset.createTime);
    parcel.WriteString(asset.size);
    parcel.WriteString(asset.hash);
    parcel.WriteUInt32(asset.status);
    parcel.WriteInt64(asset.timestamp);
}

void ReadAsset(Parcel &parcel, Asset &asset)
{
    parcel.ReadUInt32(asset.version);
    parcel.ReadString(asset.name);
    parcel.ReadString(asset.assetId);
    parcel.ReadString(asset.uri);
    parcel.ReadString(asset.modifyTime);
    parcel.ReadString(asset.createTime);
    parcel.ReadString(asset.size);
    parcel.ReadString(asset.hash);
    parcel.ReadUInt32(asset.status);
    parcel.ReadInt64(asset.timestamp);
}

std::vector<uint8_t> VirtualCloudDataTranslate::AssetToBlob(const Asset &asset)
{
    uint32_t srcLen = CalculateLens(asset);
    std::vector<uint8_t> result(srcLen, 0);
    Parcel parcel(result.data(), result.size());
    WriteAsset(parcel, asset);
    return result;
}

std::vector<uint8_t> VirtualCloudDataTranslate::AssetsToBlob(const Assets &assets)
{
    // first is vector size
    uint32_t srcLen = 0;
    srcLen += Parcel::GetUInt32Len();
    srcLen += CalculateLens(assets);
    std::vector<uint8_t> result(srcLen, 0);
    Parcel parcel(result.data(), result.size());
    parcel.WriteUInt32(assets.size());
    for (const auto &asset : assets) {
        WriteAsset(parcel, asset);
    }
    return result;
}
Asset VirtualCloudDataTranslate::BlobToAsset(const std::vector<uint8_t> &blob)
{
    Parcel parcel(const_cast<uint8_t *>(blob.data()), blob.size());
    Asset asset;
    ReadAsset(parcel, asset);
    return asset;
}
Assets VirtualCloudDataTranslate::BlobToAssets(const std::vector<uint8_t> &blob)
{
    std::vector<uint8_t> inputData = blob;
    Parcel parcel(inputData.data(), inputData.size());
    uint32_t count = 0;
    parcel.ReadUInt32(count);
    Assets assets;
    assets.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        ReadAsset(parcel, assets[i]);
    }
    return assets;
}
}