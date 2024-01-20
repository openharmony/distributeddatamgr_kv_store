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

#ifndef SUBSCRIBE_RECORDER_H
#define SUBSCRIBE_RECORDER_H

#include <map>
#include <mutex>
#include "db_types.h"
#include "query_sync_object.h"
#include "store_types.h"

namespace DistributedDB {
class SubscribeRecorder final {
public:
    SubscribeRecorder() = default;
    ~SubscribeRecorder() = default;

    void RecordSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId, const QuerySyncObject &query);

    void RemoveAllSubscribe();

    void RemoveRemoteSubscribe(const DeviceID &deviceId);

    void RemoveRemoteSubscribe(const DBInfo &dbInfo);

    void RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId);

    void RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId, const QuerySyncObject &query);

    void GetSubscribeQuery(const DBInfo &dbInfo,
        std::map<std::string, std::vector<QuerySyncObject>> &subscribeQuery) const;
private:

    void RemoveSubscribeQueries(const DBInfo &dbInfo, const DeviceID &deviceId);

    void RemoveSubscribeQuery(const DBInfo &dbInfo, const DeviceID &deviceId, const std::string &queryId);

    static bool CheckSameDBInfo(const DBInfo &srcDbInfo, const DBInfo &dtsDbInfo);

    struct SubscribeEntry {
        DBInfo dbInfo;
        std::map<DeviceID, std::vector<QuerySyncObject>> subscribeQuery;
    };
    mutable std::mutex subscribeMutex_;
    std::vector<SubscribeEntry> subscribeCache_;
};
}
#endif // SUBSCRIBE_RECORDER_H
