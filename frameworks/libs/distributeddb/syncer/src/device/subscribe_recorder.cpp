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
#include "subscribe_recorder.h"

namespace DistributedDB {
void SubscribeRecorder::RecordSubscribe(const DistributedDB::DBInfo &dbInfo, const DistributedDB::DeviceID &deviceId,
    const DistributedDB::QuerySyncObject &query)
{
    std::lock_guard<std::mutex> autoLock(subscribeMutex_);
    const auto &findSubscribe = std::find_if(subscribeCache_.begin(), subscribeCache_.end(),
    [&dbInfo](const SubscribeEntry &entry) {
        return entry.dbInfo.userId == dbInfo.userId && entry.dbInfo.appId == dbInfo.appId &&
            entry.dbInfo.storeId == dbInfo.storeId;
    });
    if (findSubscribe != subscribeCache_.end()) {
        findSubscribe->subscribeQuery[deviceId].push_back(query);
        return;
    }
    SubscribeEntry entry;
    entry.dbInfo = dbInfo;
    entry.subscribeQuery[deviceId].push_back(query);
    subscribeCache_.push_back(entry);
}

void SubscribeRecorder::RemoveAllSubscribe()
{
    std::lock_guard<std::mutex> autoLock(subscribeMutex_);
    subscribeCache_.clear();
}

void SubscribeRecorder::RemoveRemoteSubscribe(const DistributedDB::DeviceID &deviceId)
{
    std::lock_guard<std::mutex> autoLock(subscribeMutex_);
    for (auto &entry: subscribeCache_) {
        entry.subscribeQuery.erase(deviceId);
    }
}

void SubscribeRecorder::RemoveRemoteSubscribe(const DBInfo &dbInfo)
{
    const auto &findSubscribe = std::find_if(subscribeCache_.begin(), subscribeCache_.end(),
        [&dbInfo](const SubscribeEntry &entry) {
            return CheckSameDBInfo(dbInfo, entry.dbInfo);
        });
    if (findSubscribe == subscribeCache_.end()) {
        return;
    }
    subscribeCache_.erase(findSubscribe);
}

void SubscribeRecorder::RemoveRemoteSubscribe(const DistributedDB::DBInfo &dbInfo,
    const DistributedDB::DeviceID &deviceId)
{
    std::lock_guard<std::mutex> autoLock(subscribeMutex_);
    RemoveSubscribeQueries(dbInfo, deviceId);
}

void SubscribeRecorder::RemoveRemoteSubscribe(const DistributedDB::DBInfo &dbInfo,
    const DistributedDB::DeviceID &deviceId, const DistributedDB::QuerySyncObject &query)
{
    std::lock_guard<std::mutex> autoLock(subscribeMutex_);
    RemoveSubscribeQuery(dbInfo, deviceId, query.GetIdentify());
}

void SubscribeRecorder::GetSubscribeQuery(const DistributedDB::DBInfo &dbInfo,
    std::map<std::string, std::vector<QuerySyncObject>> &subscribeQuery) const
{
    std::lock_guard<std::mutex> autoLock(subscribeMutex_);
    const auto &findSubscribe = std::find_if(subscribeCache_.begin(), subscribeCache_.end(),
    [&dbInfo](const SubscribeEntry &entry) {
        return entry.dbInfo.userId == dbInfo.userId && entry.dbInfo.appId == dbInfo.appId &&
               entry.dbInfo.storeId == dbInfo.storeId;
    });
    if (findSubscribe != subscribeCache_.end()) {
        subscribeQuery = findSubscribe->subscribeQuery;
    }
}

void SubscribeRecorder::RemoveSubscribeQueries(const DBInfo &dbInfo, const DeviceID &deviceId)
{
    const auto &findSubscribe = std::find_if(subscribeCache_.begin(), subscribeCache_.end(),
        [&dbInfo](const SubscribeEntry &entry) {
            return CheckSameDBInfo(dbInfo, entry.dbInfo);
        });
    if (findSubscribe == subscribeCache_.end()) {
        return;
    }
    findSubscribe->subscribeQuery.erase(deviceId);
}

void SubscribeRecorder::RemoveSubscribeQuery(const DBInfo &dbInfo, const DeviceID &deviceId,
    const std::string &queryId)
{
    const auto &findSubscribe = std::find_if(subscribeCache_.begin(), subscribeCache_.end(),
        [&dbInfo](const SubscribeEntry &entry) {
            return CheckSameDBInfo(dbInfo, entry.dbInfo);
        });
    if (findSubscribe == subscribeCache_.end()) {
        return;
    }
    const auto &queryEntry = findSubscribe->subscribeQuery.find(deviceId);
    if (queryEntry == findSubscribe->subscribeQuery.end()) {
        return;
    }
    auto removeQuery = std::remove_if(
        queryEntry->second.begin(), queryEntry->second.end(), [&queryId](const QuerySyncObject &checkQuery) {
        return checkQuery.GetIdentify() == queryId;
    });
    queryEntry->second.erase(removeQuery, queryEntry->second.end());
}

bool SubscribeRecorder::CheckSameDBInfo(const DBInfo &srcDbInfo, const DBInfo &dtsDbInfo)
{
    return srcDbInfo.userId == dtsDbInfo.userId && srcDbInfo.appId == dtsDbInfo.appId &&
        srcDbInfo.storeId == dtsDbInfo.storeId;
}
}