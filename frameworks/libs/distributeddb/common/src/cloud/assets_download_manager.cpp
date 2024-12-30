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
#include "cloud/assets_download_manager.h"

#include "cloud/cloud_db_constant.h"
#include "db_errno.h"
#include "log_print.h"
namespace DistributedDB {
AssetsDownloadManager::AssetsDownloadManager()
    : notificationChain_(nullptr), currentDownloadCount_(0)
{
}

AssetsDownloadManager::~AssetsDownloadManager()
{
    NotificationChain *notificationChain = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(notifyMutex_);
        if (notificationChain_ == nullptr) {
            return;
        }
        notificationChain = notificationChain_;
        notificationChain_ = nullptr;
    }
    RefObject::KillAndDecObjRef(notificationChain);
}

int AssetsDownloadManager::SetAsyncDownloadAssetsConfig(const AsyncDownloadAssetsConfig &config)
{
    if (config.maxDownloadTask < CloudDbConstant::MIN_ASYNC_DOWNLOAD_TASK ||
        config.maxDownloadTask > CloudDbConstant::MAX_ASYNC_DOWNLOAD_TASK) {
        LOGE("[AssetsDownloadManager] Invalid max download task %" PRIu32, config.maxDownloadTask);
        return -E_INVALID_ARGS;
    }
    if (config.maxDownloadAssetsCount < CloudDbConstant::MIN_ASYNC_DOWNLOAD_ASSETS ||
        config.maxDownloadAssetsCount > CloudDbConstant::MAX_ASYNC_DOWNLOAD_ASSETS) {
        LOGE("[AssetsDownloadManager] Invalid max download asset count %" PRIu32, config.maxDownloadAssetsCount);
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    config_ = config;
    LOGI("[AssetsDownloadManager] Config max task %" PRIu32 " max count %" PRIu32,
        config.maxDownloadTask, config.maxDownloadAssetsCount);
    return E_OK;
}

std::pair<int, NotificationChain::Listener*> AssetsDownloadManager::BeginDownloadWithListener(
    const FinishAction &finishAction, const FinalizeAction &finalizeAction)
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    if (currentDownloadCount_ < config_.maxDownloadTask) {
        currentDownloadCount_++;
        LOGI("[AssetsDownloadManager] Begin download task now %" PRIu32, currentDownloadCount_);
        return {E_OK, nullptr};
    }
    LOGW("[AssetsDownloadManager] Too muck download task now %" PRIu32, currentDownloadCount_);
    NotificationChain *notificationChain = nullptr;
    int errCode = E_OK;
    {
        std::lock_guard<std::mutex> notifyLock(notifyMutex_);
        errCode = InitNotificationChain();
        if (errCode != E_OK) {
            return {errCode, nullptr};
        }
        notificationChain = notificationChain_;
        RefObject::IncObjRef(notificationChain);
    }
    NotificationChain::Listener *listener =
        notificationChain->RegisterListener(DOWNLOAD_FINISH_EVENT, finishAction, finalizeAction, errCode);
    RefObject::DecObjRef(notificationChain);
    if (errCode != E_OK) {
        LOGW("[AssetsDownloadManager] Register listener failed %d", errCode);
    } else {
        errCode = -E_MAX_LIMITS;
    }
    return {errCode, listener};
}

void AssetsDownloadManager::FinishDownload()
{
    NotificationChain *notificationChain = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(notifyMutex_);
        notificationChain = notificationChain_;
        RefObject::IncObjRef(notificationChain);
    }
    {
        std::lock_guard<std::mutex> autoLock(dataMutex_);
        currentDownloadCount_--;
    }
    if (notificationChain != nullptr) {
        notificationChain->NotifyEvent(DOWNLOAD_FINISH_EVENT, nullptr);
    }
    RefObject::DecObjRef(notificationChain);
    LOGI("[AssetsDownloadManager] NotifyDownloadFinish currentDownloadCount %" PRIu32, currentDownloadCount_);
}

uint32_t AssetsDownloadManager::GetCurrentDownloadCount()
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    return currentDownloadCount_;
}

uint32_t AssetsDownloadManager::GetMaxDownloadAssetsCount() const
{
    std::lock_guard<std::mutex> autoLock(dataMutex_);
    return config_.maxDownloadAssetsCount;
}

int AssetsDownloadManager::InitNotificationChain()
{
    if (notificationChain_ != nullptr) {
        return E_OK;
    }
    notificationChain_ = new(std::nothrow) NotificationChain();
    if (notificationChain_ == nullptr) {
        LOGE("[AssetsDownloadManager] create notification chain failed %d", errno);
        return -E_OUT_OF_MEMORY;
    }
    int errCode = notificationChain_->RegisterEventType(DOWNLOAD_FINISH_EVENT);
    if (errCode != E_OK) {
        LOGE("[AssetsDownloadManager] register even type failed %d", errCode);
        RefObject::KillAndDecObjRef(notificationChain_);
        notificationChain_ = nullptr;
    }
    return errCode;
}
}