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

#ifndef CLOUD_DB_PROXY_H
#define CLOUD_DB_PROXY_H
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include "cloud/icloud_db.h"

namespace DistributedDB {
class CloudDBProxy {
public:
    CloudDBProxy();
    ~CloudDBProxy() = default;

    int SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB);

    int BatchInsert(const std::string &tableName, std::vector<VBucket> &record,
        std::vector<VBucket> &extend, Info &uploadInfo);

    int BatchUpdate(const std::string &tableName, std::vector<VBucket> &record, std::vector<VBucket> &extend,
        Info &uploadInfo);

    int BatchDelete(const std::string &tableName, std::vector<VBucket> &record, std::vector<VBucket> &extend,
        Info &uploadInfo);

    int Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data);

    std::pair<int, uint64_t> Lock();

    int UnLock();

    int Close();

    int HeartBeat();

    bool IsNotExistCloudDB() const;

protected:
    class CloudActionContext {
    public:
        CloudActionContext();
        ~CloudActionContext() = default;

        void MoveInRecordAndExtend(std::vector<VBucket> &record, std::vector<VBucket> &extend);

        void MoveOutRecordAndExtend(std::vector<VBucket> &record, std::vector<VBucket> &extend);

        void MoveInQueryExtendAndData(VBucket &extend, std::vector<VBucket> &data);

        void MoveOutQueryExtendAndData(VBucket &extend, std::vector<VBucket> &data);

        void MoveInLockStatus(std::pair<int, uint64_t> &lockStatus);

        void MoveOutLockStatus(std::pair<int, uint64_t> &lockStatus);

        bool WaitForRes(int64_t timeout);

        void SetActionRes(int res);

        int GetActionRes();

        void FinishAndNotify();

        Info GetInfo();

        void SetInfo(const uint32_t &totalCount, const uint32_t &successCount, const uint32_t &failedCount);

        void SetTableName(const std::string &tableName);

        std::string GetTableName();
    private:
        std::mutex actionMutex_;
        std::condition_variable actionCv_;
        bool actionFinished_;
        int actionRes_;
        uint32_t totalCount_;
        uint32_t successCount_;
        uint32_t failedCount_;

        std::string tableName_;
        std::vector<VBucket> record_;
        std::vector<VBucket> extend_;
        VBucket queryExtend_;
        std::vector<VBucket> data_;
        std::pair<int, uint64_t> lockStatus_;
    };
    enum InnerActionCode : uint8_t {
        INSERT = 0,
        UPDATE,
        DELETE,
        QUERY,
        LOCK,
        UNLOCK,
        HEARTBEAT,
        // add action code before INVALID_ACTION
        INVALID_ACTION
    };
    int InnerAction(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action);

    DBStatus DMLActionTask(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action);

    void InnerActionTask(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action);

    mutable std::shared_mutex cloudMutex_;
    std::shared_ptr<ICloudDb> iCloudDb_;
    std::atomic<int64_t> timeout_;

    std::mutex asyncTaskMutex_;
    std::condition_variable asyncTaskCv_;
    int32_t asyncTaskCount_;
};
}
#endif // CLOUD_DB_PROXY_H
