/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEV_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEV_MANAGER_H
#include <string>
#include "concurrent_map.h"
#include "types.h"
#include "lru_bucket.h"
namespace OHOS::DistributedKv {
class DevManager {
public:
    static constexpr size_t MAX_ID_LEN = 64;
    struct DetailInfo {
        std::string uuid;
        std::string networkId;
        std::string deviceName;
        std::string deviceType;
    };
    static DevManager &GetInstance();
    std::string ToUUID(const std::string &networkId);
    std::string ToNetworkId(const std::string &uuid);
    const DetailInfo &GetLocalDevice();
    std::vector<DetailInfo> GetRemoteDevices();
    class Observer {
    public:
        Observer() = default;
        virtual ~Observer() {}
        virtual void Online(const std::string &networkId) = 0;
        virtual void Offline(const std::string &networkId) = 0;
    };

    void Register(Observer *observer);
    void Unregister(Observer *observer);

private:
    friend class DMStateCallback;
    friend class DmDeathCallback;
    DevManager(const std::string &pkgName);
    ~DevManager() = default;
    void Online(const std::string &networkId);
    void Offline(const std::string &networkId);
    void OnChanged(const std::string &networkId);
    void OnReady(const std::string &networkId);
    void RegisterDevCallback();
    void UpdateBucket();
    DetailInfo GetDvInfoFromBucket(const std::string &id);

    int32_t Init();
    std::function<void()> Retry();
    const std::string PKG_NAME;
    const DetailInfo invalidDetail_ {};
    DetailInfo localInfo_ {};
    mutable std::mutex mutex_ {};
    mutable LRUBucket<std::string, DetailInfo> deviceInfos_ {64};
    ConcurrentMap<Observer *, Observer *> observers_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEV_MANAGER_H
