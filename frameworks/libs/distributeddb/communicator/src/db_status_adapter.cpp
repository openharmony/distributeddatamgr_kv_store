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
#include "db_status_adapter.h"

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "param_check_utils.h"
#include "runtime_context.h"

namespace DistributedDB {
DBStatusAdapter::DBStatusAdapter()
    : dbInfoHandle_(nullptr),
      localSendLabelExchange_(false),
      cacheLocalSendLabelExchange_(false)
{
}

void DBStatusAdapter::SetDBInfoHandle(const std::shared_ptr<DBInfoHandle> &dbInfoHandle)
{
    {
        std::lock_guard<std::mutex> autoLock(handleMutex_);
        dbInfoHandle_ = dbInfoHandle;
    }
    NotifyRemoteOffline();
    ClearAllCache();
    LOGI("[DBStatusAdapter] SetDBInfoHandle Finish");
}

bool DBStatusAdapter::IsSupport(const std::string &devInfo)
{
    std::shared_ptr<DBInfoHandle> dbInfoHandle = GetDBInfoHandle();
    if (dbInfoHandle == nullptr) {
        LOGD("[DBStatusAdapter] dbInfoHandle not set");
        return false;
    }
    if (IsSendLabelExchange()) {
        return false;
    }
    {
        std::lock_guard<std::mutex> autoLock(supportMutex_);
        if (remoteOptimizeInfo_.find(devInfo) != remoteOptimizeInfo_.end()) {
            return remoteOptimizeInfo_[devInfo];
        }
    }
    std::lock_guard<std::mutex> autoLock(supportMutex_);
    remoteOptimizeInfo_[devInfo] = true;
    return true;
}

int DBStatusAdapter::GetLocalDBInfos(std::vector<DBInfo> &dbInfos)
{
    std::shared_ptr<DBInfoHandle> dbInfoHandle = GetDBInfoHandle();
    if (dbInfoHandle == nullptr) {
        LOGD("[DBStatusAdapter][GetLocalDBInfos] handle not set");
        return -E_NOT_SUPPORT;
    }
    if (IsSendLabelExchange()) {
        return -E_NOT_SUPPORT;
    }
    std::lock_guard<std::mutex> autoLock(localInfoMutex_);
    for (const auto &info: localDBInfos_) {
        if (info.isNeedSync) {
            dbInfos.push_back(info);
        }
    }
    return E_OK;
}

void DBStatusAdapter::SetDBStatusChangeCallback(const RemoteDBChangeCallback &remote,
    const LocalDBChangeCallback &local, const RemoteSupportChangeCallback &supportCallback)
{
    {
        std::lock_guard<std::mutex> autoLock(callbackMutex_);
        remoteCallback_ = remote;
        localCallback_ = local;
        supportCallback_ = supportCallback;
    }
    if (remote == nullptr || local == nullptr) {
        return;
    }
    // avoid notify before set callback
    bool triggerNow = false;
    std::map<std::string, std::vector<DBInfo>> remoteDBInfos;
    {
        std::lock_guard<std::mutex> autoLock(remoteInfoMutex_);
        remoteDBInfos = remoteDBInfos_;
        triggerNow = !remoteDBInfos.empty();
    }
    bool triggerLocal = false;
    {
        std::lock_guard<std::mutex> autoLock(localInfoMutex_);
        triggerLocal = !localDBInfos_.empty();
        triggerNow |= triggerLocal;
    }
    // trigger callback async
    if (!triggerNow) {
        return;
    }
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([triggerLocal, remoteDBInfos, remote, local]() {
        for (const auto &[devInfo, dbInfos]: remoteDBInfos) {
            remote(devInfo, dbInfos);
        }
        if (triggerLocal) {
            local();
        }
    });
    if (errCode != E_OK) {
        LOGW("[DBStatusAdapter][SetDBStatusChangeCallback] Schedule Task failed! errCode = %d", errCode);
    }
}

void DBStatusAdapter::NotifyDBInfos(const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos)
{
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, devInfos, dbInfos]() {
        if (IsSendLabelExchange()) {
            LOGW("[DBStatusAdapter][NotifyDBInfos] local dev no support communication optimize");
            return;
        }
        bool isLocal = IsLocalDeviceId(devInfos.identifier);
        if (!isLocal && !IsSupport(devInfos.identifier)) {
            LOGW("[DBStatusAdapter][NotifyDBInfos] no support dev %s", STR_MASK(devInfos.identifier));
            return;
        }
        bool isChange = LoadIntoCache(isLocal, devInfos, dbInfos);
        std::lock_guard<std::mutex> autoLock(callbackMutex_);
        if (!isLocal && remoteCallback_ != nullptr) {
            remoteCallback_(devInfos.identifier, dbInfos);
        } else if (isLocal && localCallback_ != nullptr && isChange) {
            localCallback_();
        }
    });
    if (errCode != E_OK) {
        LOGW("[DBStatusAdapter][NotifyDBInfos] Schedule Task failed! errCode = %d", errCode);
    }
}

void DBStatusAdapter::TargetOffline(const std::string &device)
{
    std::shared_ptr<DBInfoHandle> dbInfoHandle = GetDBInfoHandle();
    if (dbInfoHandle == nullptr) {
        return;
    }
    {
        std::lock_guard<std::mutex> autoLock(supportMutex_);
        if (remoteOptimizeInfo_.find(device) != remoteOptimizeInfo_.end()) {
            remoteOptimizeInfo_.erase(device);
        }
    }
    RuntimeContext::GetInstance()->RemoveRemoteSubscribe(device);
}

bool DBStatusAdapter::IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
    const std::string &devInfo)
{
    std::shared_ptr<DBInfoHandle> dbInfoHandle = GetDBInfoHandle();
    if (dbInfoHandle == nullptr || IsSendLabelExchange()) {
        return true;
    }
    return dbInfoHandle->IsNeedAutoSync(userId, appId, storeId, { devInfo });
}

void DBStatusAdapter::SetRemoteOptimizeCommunication(const std::string &dev, bool optimize)
{
    std::shared_ptr<DBInfoHandle> dbInfoHandle = GetDBInfoHandle();
    if (dbInfoHandle == nullptr) {
        return;
    }
    bool triggerLocalCallback = false;
    {
        std::lock_guard<std::mutex> autoLock(supportMutex_);
        if (remoteOptimizeInfo_.find(dev) == remoteOptimizeInfo_.end()) {
            remoteOptimizeInfo_[dev] = optimize;
            return;
        }
        if (remoteOptimizeInfo_[dev] == optimize) {
            return;
        }
        if (remoteOptimizeInfo_[dev] && !optimize) {
            triggerLocalCallback = true;
        }
        remoteOptimizeInfo_[dev] = optimize;
    }
    if (!triggerLocalCallback) {
        return;
    }
    LOGI("[DBStatusAdapter][SetRemoteOptimizeCommunication] remote dev %.3s optimize change!", dev.c_str());
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, dev]() {
        RemoteSupportChangeCallback callback;
        {
            std::lock_guard<std::mutex> autoLock(callbackMutex_);
            callback = supportCallback_;
        }
        if (callback) {
            callback(dev);
        }
    });
    if (errCode != E_OK) {
        LOGW("[DBStatusAdapter][SetRemoteOptimizeCommunication] Schedule Task failed! errCode = %d", errCode);
    }
}

bool DBStatusAdapter::IsSendLabelExchange()
{
    std::shared_ptr<DBInfoHandle> dbInfoHandle = GetDBInfoHandle();
    if (dbInfoHandle == nullptr) {
        return true;
    }
    {
        std::lock_guard<std::mutex> autoLock(supportMutex_);
        if (cacheLocalSendLabelExchange_) {
            return localSendLabelExchange_;
        }
    }
    bool isSupport = dbInfoHandle->IsSupport();
    std::lock_guard<std::mutex> autoLock(supportMutex_);
    LOGD("[DBStatusAdapter][IsSendLabelExchange] local support status is %d", isSupport);
    localSendLabelExchange_ = !isSupport;
    cacheLocalSendLabelExchange_ = true;
    return localSendLabelExchange_;
}

std::shared_ptr<DBInfoHandle> DBStatusAdapter::GetDBInfoHandle() const
{
    std::lock_guard<std::mutex> autoLock(handleMutex_);
    return dbInfoHandle_;
}

bool DBStatusAdapter::LoadIntoCache(bool isLocal, const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos)
{
    if (isLocal) {
        std::lock_guard<std::mutex> autoLock(localInfoMutex_);
        return MergeDBInfos(dbInfos, localDBInfos_);
    }
    std::lock_guard<std::mutex> autoLock(remoteInfoMutex_);
    if (remoteDBInfos_.find(devInfos.identifier) == remoteDBInfos_.end()) {
        remoteDBInfos_.insert({devInfos.identifier, {}});
    }
    return MergeDBInfos(dbInfos, remoteDBInfos_[devInfos.identifier]);
}

void DBStatusAdapter::ClearAllCache()
{
    {
        std::lock_guard<std::mutex> autoLock(localInfoMutex_);
        localDBInfos_.clear();
    }
    {
        std::lock_guard<std::mutex> autoLock(remoteInfoMutex_);
        remoteDBInfos_.clear();
    }
    std::lock_guard<std::mutex> autoLock(supportMutex_);
    remoteOptimizeInfo_.clear();
    cacheLocalSendLabelExchange_ = false;
    localSendLabelExchange_ = false;
    LOGD("[DBStatusAdapter] ClearAllCache ok");
}

void DBStatusAdapter::NotifyRemoteOffline()
{
    std::map<std::string, std::vector<DBInfo>> remoteOnlineDBInfos;
    {
        std::lock_guard<std::mutex> autoLock(remoteInfoMutex_);
        for (const auto &[dev, dbInfos]: remoteDBInfos_) {
            for (const auto &dbInfo: dbInfos) {
                if (dbInfo.isNeedSync) {
                    DBInfo info = dbInfo;
                    info.isNeedSync = false;
                    remoteOnlineDBInfos[dev].push_back(info);
                }
            }
        }
    }
    RemoteDBChangeCallback callback;
    {
        std::lock_guard<std::mutex> autoLock(callbackMutex_);
        callback = remoteCallback_;
    }
    if (callback == nullptr || remoteOnlineDBInfos.empty()) {
        LOGD("[DBStatusAdapter] no need to notify offline when reset handle");
        return;
    }
    for (const auto &[dev, dbInfos]: remoteOnlineDBInfos) {
        callback(dev, dbInfos);
    }
    LOGD("[DBStatusAdapter] Notify offline ok");
}

bool DBStatusAdapter::MergeDBInfos(const std::vector<DBInfo> &srcDbInfos, std::vector<DBInfo> &dstDbInfos)
{
    bool isDbInfoChange = false;
    for (const auto &srcInfo: srcDbInfos) {
        if (!ParamCheckUtils::CheckStoreParameter(srcInfo.storeId, srcInfo.appId, srcInfo.userId)) {
            continue;
        }
        auto res = std::find_if(dstDbInfos.begin(), dstDbInfos.end(), [&srcInfo](const DBInfo &dstInfo) {
            return srcInfo.appId == dstInfo.appId && srcInfo.userId == dstInfo.userId &&
                srcInfo.storeId == dstInfo.storeId && srcInfo.syncDualTupleMode == dstInfo.syncDualTupleMode;
        });
        if (res == dstDbInfos.end()) {
            dstDbInfos.push_back(srcInfo);
            isDbInfoChange = true;
        } else if (res->isNeedSync != srcInfo.isNeedSync) {
            res->isNeedSync = srcInfo.isNeedSync;
            isDbInfoChange = true;
        }
    }
    return isDbInfoChange;
}

int DBStatusAdapter::GetLocalDeviceId(std::string &deviceId)
{
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    int errCode = RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    if (errCode != E_OK) {
        return errCode;
    }
    return communicatorAggregator->GetLocalIdentity(deviceId);
}

bool DBStatusAdapter::IsLocalDeviceId(const std::string &deviceId)
{
    std::string localId;
    if (GetLocalDeviceId(localId) != E_OK) {
        return false;
    }
    return deviceId == localId;
}
}