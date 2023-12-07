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
#include "virtual_asset_loader.h"
#include "log_print.h"

namespace DistributedDB {
DBStatus VirtualAssetLoader::Download(const std::string &tableName, const std::string &gid, const Type &prefix,
    std::map<std::string, Assets> &assets)
{
    {
        std::lock_guard<std::mutex> autoLock(dataMutex_);
        if (downloadStatus_ != OK) {
            return downloadStatus_;
        }
    }
    LOGD("Download GID:%s", gid.c_str());
    if (downloadCallBack_) {
        downloadCallBack_(assets);
    }
    for (auto &item: assets) {
        for (auto &asset: item.second) {
            LOGD("asset [name]:%s, [status]:%u, [flag]:%u", asset.name.c_str(), asset.status, asset.flag);
            asset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
        }
    }
    return OK;
}

DBStatus VirtualAssetLoader::RemoveLocalAssets(const std::vector<Asset> &assets)
{
    if (removeAssetsCallBack_) {
        return removeAssetsCallBack_(assets);
    }
    return DBStatus::OK;
}

void VirtualAssetLoader::SetDownloadStatus(DBStatus status)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    LOGD("[VirtualAssetLoader] set download status :%d", static_cast<int>(status));
    downloadStatus_ = status;
}

void VirtualAssetLoader::ForkDownload(const DownloadCallBack &callback)
{
    downloadCallBack_ = callback;
}

void VirtualAssetLoader::ForkRemoveLocalAssets(const RemoveAssetsCallBack &callback)
{
    removeAssetsCallBack_ = callback;
}
}
