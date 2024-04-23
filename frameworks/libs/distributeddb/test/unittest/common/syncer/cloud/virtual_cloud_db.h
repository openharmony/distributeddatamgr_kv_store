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
    struct CloudData {
        VBucket record;
        VBucket extend;
    };
    VirtualCloudDb() = default;
    ~VirtualCloudDb() override = default;
    DBStatus BatchInsert(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend) override;

    DBStatus BatchInsertWithGid(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend);

    DBStatus BatchUpdate(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend) override;

    DBStatus BatchDelete(const std::string &tableName, std::vector<VBucket> &extend) override;

    DBStatus Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data) override;

    DBStatus DeleteByGid(const std::string &tableName, VBucket &extend);

    std::pair<DBStatus, std::string> GetEmptyCursor(const std::string &tableName) override;

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

    void SetIncrementData(const std::string &tableName, const VBucket &record, const VBucket &extend);

    uint32_t GetQueryTimes(const std::string &tableName);

    void SetActionStatus(DBStatus status);

    DBStatus GetDataStatus(const std::string &gid, bool &deleteStatus);

    void ClearAllData();

    void ForkQuery(const std::function<void(const std::string &, VBucket &)> &forkFunc);

    void ForkUpload(const std::function<void(const std::string &, VBucket &)> &forkUploadFunc);

    void ForkInsertConflict(const std::function<DBStatus(const std::string &, VBucket &, VBucket &,
        std::vector<CloudData> &)> &forkUploadFunc);

    int32_t GetLockCount() const;

    void Reset();

    void SetInsertFailed(int32_t count);

    void SetClearExtend(int32_t count);

    void SetCloudNetworkError(bool cloudNetworkError);

    void SetConflictInUpload(bool conflict);

    void SetHeartbeatBlockTime(int32_t blockTime);
private:
    DBStatus InnerBatchInsert(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend);

    DBStatus InnerUpdate(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend, bool isDelete);

    DBStatus InnerUpdateWithoutLock(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend, bool isDelete);

    DBStatus UpdateCloudData(const std::string &tableName, CloudData &&cloudData);

    void GetCloudData(const std::string &cursor, bool isIncreCursor, std::vector<CloudData> allData,
        std::vector<VBucket> &data, VBucket &extend);

    bool IsCloudGidMatching(const std::vector<QueryNode> &queryNodes, VBucket &extend);

    bool IsCloudGidMatchingInner(const QueryNode &queryNode, VBucket &extend);

    bool IsPrimaryKeyMatching(const std::vector<QueryNode> &queryNodes, VBucket &record);

    bool IsPrimaryKeyMatchingInner(const QueryNode &queryNode, VBucket &record);

    void AddAssetIdForExtend(VBucket &record, VBucket &extend);

    void AddAssetsIdInner(Assets &assets);

    std::atomic<bool> cloudError_ = false;
    std::atomic<bool> cloudNetworkError_ = false;
    std::atomic<bool> heartbeatError_ = false;
    std::atomic<bool> lockStatus_ = false;
    std::atomic<bool> conflictInUpload_ = false;
    std::atomic<int32_t> blockTimeMs_ = 0;
    std::atomic<int32_t> heartbeatBlockTimeMs_ = 0;
    std::atomic<int64_t> currentGid_ = 0;
    std::atomic<int64_t> currentCursor_ = 1;
    std::atomic<int64_t> currentVersion_ = 0;
    std::atomic<int32_t> queryLimit_ = 100;
    std::atomic<int32_t> heartbeatCount_ = 0;
    std::atomic<int32_t> lockCount_ = 0;
    std::atomic<int32_t> insertFailedCount_ = 0;
    std::atomic<int32_t> missingExtendCount_ = 0;
    std::mutex cloudDataMutex_;
    std::map<std::string, std::vector<CloudData>> cloudData_;
    std::map<std::string, std::vector<CloudData>> incrementCloudData_;
    bool isSetCrementCloudData_ = false;
    std::string increPrefix_ = "increPrefix_";
    std::map<std::string, uint32_t> queryTimes_;
    DBStatus actionStatus_ = OK;
    std::function<void(const std::string &, VBucket &)> forkFunc_;
    std::function<void(const std::string &, VBucket &)> forkUploadFunc_;
    std::function<DBStatus(const std::string &, VBucket &, VBucket &,
        std::vector<CloudData> &)> forkUploadConflictFunc_;
};
}
#endif // VIRTUAL_CLOUD_DB_H
