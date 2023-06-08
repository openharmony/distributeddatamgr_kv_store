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

#ifndef VIRTUAL_CLOUD_DB_H
#define VIRTUAL_CLOUD_DB_H
#include <atomic>
#include <mutex>
#include "icloud_db.h"

namespace DistributedDB {
class VirtualCloudDb : public ICloudDb {
public:
    VirtualCloudDb() = default;
    ~VirtualCloudDb() override = default;
    DBStatus BatchInsert(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend) override;

    DBStatus BatchUpdate(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend) override;

    DBStatus BatchDelete(const std::string &tableName, std::vector<VBucket> &extend) override;

    DBStatus Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data) override;

    std::pair<DBStatus, uint32_t> Lock() override;

    DBStatus UnLock() override;

    DBStatus HeartBeat() override;

    DBStatus Close() override;

    void SetCloudError(bool cloudError);

    void SetBlockTime(int32_t blockTime);

    void ClearHeartbeatCount();

    int32_t GetHeartbeatCount();

    bool GetLockStatus();

    void SetHeartbeatError(bool heartbeatError);
private:
    struct CloudData {
        VBucket record;
        VBucket extend;
    };

    DBStatus InnerUpdate(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend, bool isDelete);

    DBStatus UpdateCloudData(const std::string &tableName, CloudData &&cloudData);

    std::atomic<bool> cloudError_ = false;
    std::atomic<bool> heartbeatError_ = false;
    std::atomic<bool> lockStatus_ = false;
    std::atomic<int32_t> blockTimeMs_ = 0;
    std::atomic<int64_t> currentGid_ = 0;
    std::atomic<int64_t> currentCursor_ = 1;
    std::atomic<int32_t> queryLimit_ = 100;
    std::atomic<int32_t> heartbeatCount_ = 0;
    std::mutex cloudDataMutex_;
    std::map<std::string, std::vector<CloudData>> cloudData_;
};
}
#endif // VIRTUAL_CLOUD_DB_H