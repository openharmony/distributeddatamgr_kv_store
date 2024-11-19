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

#ifndef VIRTUAL_ASSETLOADER_H
#define VIRTUAL_ASSETLOADER_H
#include <atomic>
#include <mutex>
#include "iAssetLoader.h"

namespace DistributedDB {
using DownloadCallBack = std::function<void (std::map<std::string, Assets> &assets)>;
using RemoveAssetsCallBack = std::function<DBStatus (const std::vector<Asset> &assets)>;
using RemoveLocalAssetsCallBack = std::function<DBStatus (std::map<std::string, Assets> &assets)>;
using BatchDownloadCallback = std::function<DBStatus (int rowIndex, std::map<std::string, Assets> &assets)>;
class VirtualAssetLoader : public IAssetLoader {
public:
    VirtualAssetLoader() = default;
    ~VirtualAssetLoader() override = default;

    DBStatus Download(const std::string &tableName, const std::string &gid, const Type &prefix,
        std::map<std::string, Assets> &assets) override;

    DBStatus RemoveLocalAssets(const std::vector<Asset> &assets) override;

    DBStatus RemoveLocalAssets(const std::string &tableName, const std::string &gid, const Type &prefix,
             std::map<std::string, Assets> &assets) override;

    void SetDownloadStatus(DBStatus status);

    void SetRemoveStatus(DBStatus status);

    void SetBatchRemoveStatus(DBStatus status);

    void ForkDownload(const DownloadCallBack &callback);

    void ForkRemoveLocalAssets(const RemoveAssetsCallBack &callback);

    void SetRemoveLocalAssetsCallback(const RemoveLocalAssetsCallBack &callback);

    void BatchDownload(const std::string &tableName, std::vector<AssetRecord> &downloadAssets) override;

    void BatchRemoveLocalAssets(const std::string &tableName, std::vector<AssetRecord> &removeAssets) override;

    int GetBatchDownloadCount();

    int GetBatchRemoveCount();

    void Reset();

    void ForkBatchDownload(const BatchDownloadCallback &callback);
private:
    DBStatus RemoveLocalAssetsInner(const std::string &tableName, const std::string &gid, const Type &prefix,
        std::map<std::string, Assets> &assets);

    std::mutex dataMutex_;
    DBStatus downloadStatus_ = OK;
    DBStatus removeStatus_ = OK;
    DBStatus batchRemoveStatus_ = OK;
    DownloadCallBack downloadCallBack_;
    RemoveAssetsCallBack removeAssetsCallBack_;
    RemoveLocalAssetsCallBack removeLocalAssetsCallBack_;
    std::atomic<int> downloadCount_ = 0;
    std::atomic<int> removeCount_ = 0;
    BatchDownloadCallback batchDownloadCallback_;
};
}
#endif // VIRTUAL_ASSETLOADER_H
