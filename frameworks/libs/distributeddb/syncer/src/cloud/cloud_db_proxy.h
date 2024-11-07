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
#include "cloud/cloud_db_types.h"
#include "cloud/icloud_db.h"
#include "cloud/iAssetLoader.h"

namespace DistributedDB {
class CloudDBProxy {
public:
    CloudDBProxy();
    ~CloudDBProxy() = default;

    void SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB);

    int SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs);

    const std::map<std::string, std::shared_ptr<ICloudDb>> GetCloudDB() const;

    void SwitchCloudDB(const std::string &user);

    void SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader);

    int BatchInsert(const std::string &tableName, std::vector<VBucket> &record,
        std::vector<VBucket> &extend, Info &uploadInfo, uint32_t &retryCount);

    int BatchUpdate(const std::string &tableName, std::vector<VBucket> &record, std::vector<VBucket> &extend,
        Info &uploadInfo, uint32_t &retryCount);

    int BatchDelete(const std::string &tableName, std::vector<VBucket> &record, std::vector<VBucket> &extend,
        Info &uploadInfo, uint32_t &retryCount);

    int Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data);

    std::pair<int, std::string> GetEmptyCursor(const std::string &tableName);

    std::pair<int, uint64_t> Lock();

    int UnLock();

    int Close();

    int HeartBeat();

    bool IsNotExistCloudDB() const;

    int Download(const std::string &tableName, const std::string &gid, const Type &prefix,
        std::map<std::string, Assets> &assets);

    int RemoveLocalAssets(const std::vector<Asset> &assets);

    int RemoveLocalAssets(const std::string &tableName, const std::string &gid, const Type &prefix,
        std::map<std::string, Assets> &assets);

    void SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback);

    bool IsExistCloudVersionCallback() const;

    std::pair<int, std::string> GetCloudVersion(const std::string &originVersion) const;

    void SetPrepareTraceId(const std::string &traceId) const;

    int BatchDownload(const std::string &tableName, std::vector<IAssetLoader::AssetRecord> &downloadAssets);

    int BatchRemoveLocalAssets(const std::string &tableName, std::vector<IAssetLoader::AssetRecord> &removeAssets);

    static int GetInnerErrorCode(DBStatus status);
protected:
    class CloudActionContext {
    public:
        CloudActionContext();
        ~CloudActionContext() = default;

        void MoveInRecordAndExtend(std::vector<VBucket> &record, std::vector<VBucket> &extend);

        void MoveInExtend(std::vector<VBucket> &extend);

        void MoveOutRecordAndExtend(std::vector<VBucket> &record, std::vector<VBucket> &extend);

        void MoveInQueryExtendAndData(VBucket &extend, std::vector<VBucket> &data);

        void MoveOutQueryExtendAndData(VBucket &extend, std::vector<VBucket> &data);

        void MoveInLockStatus(std::pair<int, uint64_t> &lockStatus);

        void MoveOutLockStatus(std::pair<int, uint64_t> &lockStatus);

        void MoveInCursorStatus(std::pair<int, std::string> &cursorStatus);

        void MoveOutCursorStatus(std::pair<int, std::string> &cursorStatus);

        void SetActionRes(int res);

        int GetActionRes();

        void FinishAndNotify();

        Info GetInfo();

        void SetInfo(const CloudWaterType &type, DBStatus status, uint32_t size);

        void SetTableName(const std::string &tableName);

        std::string GetTableName();

        uint32_t GetRetryCount();
    private:
        static bool IsEmptyAssetId(const Assets &assets);

        static bool IsRecordActionFail(const VBucket &extend, const CloudWaterType &type, DBStatus status);

        std::mutex actionMutex_;
        std::condition_variable actionCv_;
        bool actionFinished_;
        int actionRes_;
        uint32_t totalCount_;
        uint32_t successCount_;
        uint32_t failedCount_;
        uint32_t retryCount_;

        std::string tableName_;
        std::vector<VBucket> record_;
        std::vector<VBucket> extend_;
        VBucket queryExtend_;
        std::vector<VBucket> data_;
        std::pair<int, uint64_t> lockStatus_;
        std::pair<int, std::string> cursorStatus_;
    };
    enum InnerActionCode : uint8_t {
        INSERT = 0,
        UPDATE,
        DELETE,
        QUERY,
        GET_EMPTY_CURSOR,
        LOCK,
        UNLOCK,
        HEARTBEAT,
        // add action code before INVALID_ACTION
        INVALID_ACTION
    };

    enum InnerBatchOpType : uint8_t {
        BATCH_DOWNLOAD = 0,
        BATCH_REMOVE_LOCAL
    };

    static int InnerAction(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action);

    static DBStatus DMLActionTask(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action);

    static void InnerActionTask(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action);

    static DBStatus InnerActionLock(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb);

    static DBStatus InnerActionGetEmptyCursor(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb);

    static DBStatus QueryAction(const std::shared_ptr<CloudActionContext> &context,
        const std::shared_ptr<ICloudDb> &cloudDb);

    int BatchOperateAssetsWithAllRecords(const std::string &tableName,
        std::vector<IAssetLoader::AssetRecord> &allRecords, const InnerBatchOpType operationType);

    int BatchOperateAssetsInner(const std::string &tableName,
        std::vector<IAssetLoader::AssetRecord> &necessaryRecords, const InnerBatchOpType operationType);

    // save record with assets in nonEmptyRecords, return the indexes of these records in the original vector
    std::vector<int> GetNotEmptyAssetRecords(std::vector<IAssetLoader::AssetRecord> &originalRecords,
        std::vector<IAssetLoader::AssetRecord> &nonEmptyRecords);

    // copy newRecords's assets and status back to originalRecords, based on indexes
    void CopyAssetsBack(std::vector<IAssetLoader::AssetRecord> &originalRecords, const std::vector<int> indexes,
        std::vector<IAssetLoader::AssetRecord> &newRecords);

    mutable std::shared_mutex cloudMutex_;
    mutable std::shared_mutex assetLoaderMutex_;
    std::shared_ptr<ICloudDb> iCloudDb_;
    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs_;
    std::shared_ptr<IAssetLoader> iAssetLoader_;
    std::atomic<int64_t> timeout_;

    mutable std::mutex genVersionMutex_;
    GenerateCloudVersionCallback genVersionCallback_;
};
}
#endif // CLOUD_DB_PROXY_H
