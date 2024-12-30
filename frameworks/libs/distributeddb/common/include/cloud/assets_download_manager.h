/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ASSETS_DOWNLOAD_MANAGER_H
#define ASSETS_DOWNLOAD_MANAGER_H

#include "cloud/cloud_store_types.h"
#include "notification_chain.h"
#include "store_types.h"

namespace DistributedDB {
class AssetsDownloadManager {
public:
    AssetsDownloadManager();
    ~AssetsDownloadManager();

    int SetAsyncDownloadAssetsConfig(const AsyncDownloadAssetsConfig &config);

    using FinishAction = std::function<void(void *)>;
    using FinalizeAction = std::function<void(void)>;
    // try to add download count, add finish listener when out of limit
    std::pair<int, NotificationChain::Listener*> BeginDownloadWithListener(const FinishAction &finishAction,
        const FinalizeAction &finalizeAction = nullptr);

    void FinishDownload();

    uint32_t GetCurrentDownloadCount();

    uint32_t GetMaxDownloadAssetsCount() const;
private:
    int InitNotificationChain();

    static constexpr const int DOWNLOAD_FINISH_EVENT = 1;
    mutable std::mutex notifyMutex_;
    NotificationChain *notificationChain_;

    mutable std::mutex dataMutex_;
    uint32_t currentDownloadCount_;
    AsyncDownloadAssetsConfig config_;
};
}
#endif // ASSETS_DOWNLOAD_MANAGER_H