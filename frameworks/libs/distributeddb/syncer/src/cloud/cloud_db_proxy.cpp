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
#include "cloud_db_proxy.h"
#include "db_errno.h"
#include "log_print.h"
#include "runtime_context.h"

namespace DistributedDB {
CloudDBProxy::CloudDBProxy()
    : timeout_(0),
      asyncTaskCount_(0)
{
}

int CloudDBProxy::SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB)
{
    std::unique_lock<std::shared_mutex> writeLock(cloudMutex_);
    if (!iCloudDb_) {
        iCloudDb_ = cloudDB;
    }
    return E_OK;
}

void CloudDBProxy::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    std::unique_lock<std::shared_mutex> writeLock(assetLoaderMutex_);
    iAssetLoader_ = loader;
}

int CloudDBProxy::BatchInsert(const std::string &tableName, std::vector<VBucket> &record,
    std::vector<VBucket> &extend, Info &uploadInfo)
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return -E_CLOUD_ERROR;
    }
    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    context->MoveInRecordAndExtend(record, extend);
    context->SetTableName(tableName);
    int errCode = InnerAction(context, cloudDb, INSERT);
    uploadInfo = context->GetInfo();
    context->MoveOutRecordAndExtend(record, extend);
    return errCode;
}

int CloudDBProxy::BatchUpdate(const std::string &tableName, std::vector<VBucket> &record,
    std::vector<VBucket> &extend, Info &uploadInfo)
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return -E_CLOUD_ERROR;
    }
    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    context->SetTableName(tableName);
    context->MoveInRecordAndExtend(record, extend);
    int errCode = InnerAction(context, cloudDb, UPDATE);
    uploadInfo = context->GetInfo();
    return errCode;
}

int CloudDBProxy::BatchDelete(const std::string &tableName, std::vector<VBucket> &record, std::vector<VBucket> &extend,
    Info &uploadInfo)
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return -E_CLOUD_ERROR;
    }
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    context->MoveInRecordAndExtend(record, extend);
    context->SetTableName(tableName);
    int errCode = InnerAction(context, cloudDb, DELETE);
    uploadInfo = context->GetInfo();
    return errCode;
}

int CloudDBProxy::Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data)
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return -E_CLOUD_ERROR;
    }
    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    context->MoveInQueryExtendAndData(extend, data);
    context->SetTableName(tableName);
    int errCode = InnerAction(context, cloudDb, QUERY);
    context->MoveOutQueryExtendAndData(extend, data);
    return errCode;
}

std::pair<int, uint64_t> CloudDBProxy::Lock()
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return { -E_CLOUD_ERROR, 0u };
    }
    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    std::pair<int, uint64_t> lockStatus;
    int errCode = InnerAction(context, cloudDb, LOCK);
    context->MoveOutLockStatus(lockStatus);
    lockStatus.first = errCode;
    return lockStatus;
}

int CloudDBProxy::UnLock()
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return -E_CLOUD_ERROR;
    }
    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    return InnerAction(context, cloudDb, UNLOCK);
}

int CloudDBProxy::Close()
{
    std::shared_ptr<ICloudDb> iCloudDb = nullptr;
    {
        std::unique_lock<std::shared_mutex> writeLock(cloudMutex_);
        if (iCloudDb_ == nullptr) {
            return E_OK;
        }
        iCloudDb = iCloudDb_;
        iCloudDb_ = nullptr;
    }
    {
        std::unique_lock<std::mutex> uniqueLock(asyncTaskMutex_);
        LOGD("[CloudDBProxy] wait for all asyncTask  begin");
        asyncTaskCv_.wait(uniqueLock, [this]() {
            return asyncTaskCount_ <= 0;
        });
        LOGD("[CloudDBProxy] wait for all asyncTask end");
    }
    LOGD("[CloudDBProxy] call cloudDb close begin");
    DBStatus status = iCloudDb->Close();
    LOGD("[CloudDBProxy] call cloudDb close end");
    return status == OK ? E_OK : -E_CLOUD_ERROR;
}

int CloudDBProxy::HeartBeat()
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return -E_CLOUD_ERROR;
    }

    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    return InnerAction(context, cloudDb, HEARTBEAT);
}

bool CloudDBProxy::IsNotExistCloudDB() const
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    return iCloudDb_ == nullptr;
}

int CloudDBProxy::Download(const std::string &tableName, const std::string &gid, const Type &prefix,
    std::map<std::string, Assets> &assets)
{
    std::shared_lock<std::shared_mutex> readLock(assetLoaderMutex_);
    if (iAssetLoader_ == nullptr) {
        LOGE("Asset loader has not been set %d", -E_NOT_SET);
        return -E_NOT_SET;
    }
    DBStatus status = iAssetLoader_->Download(tableName, gid, prefix, assets);
    if (status != OK) {
        LOGE("[CloudDBProxy] download asset failed %d", static_cast<int>(status));
    }
    return GetInnerErrorCode(status);
}

int CloudDBProxy::RemoveLocalAssets(const std::vector<Asset> &assets)
{
    std::shared_lock<std::shared_mutex> readLock(assetLoaderMutex_);
    if (iAssetLoader_ == nullptr) {
        LOGE("Asset loader has not been set %d", -E_NOT_SET);
        return -E_NOT_SET;
    }
    DBStatus status = iAssetLoader_->RemoveLocalAssets(assets);
    if (status != OK) {
        LOGE("[CloudDBProxy] remove local asset failed %d", static_cast<int>(status));
    }
    return GetInnerErrorCode(status);
}

int CloudDBProxy::InnerAction(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action)
{
    if (action >= InnerActionCode::INVALID_ACTION) {
        return -E_INVALID_ARGS;
    }
    {
        std::lock_guard<std::mutex> uniqueLock(asyncTaskMutex_);
        asyncTaskCount_++;
    }
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([cloudDb, context, action, this]() {
        InnerActionTask(context, cloudDb, action);
    });
    if (errCode != E_OK) {
        {
            std::lock_guard<std::mutex> uniqueLock(asyncTaskMutex_);
            asyncTaskCount_--;
        }
        asyncTaskCv_.notify_all();
        LOGW("[CloudDBProxy] Schedule async task error %d", errCode);
        return errCode;
    }
    if (context->WaitForRes(timeout_)) {
        errCode = context->GetActionRes();
    } else {
        errCode = -E_TIMEOUT;
    }
    return errCode;
}

DBStatus CloudDBProxy::DMLActionTask(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action)
{
    DBStatus status = OK;
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    context->MoveOutRecordAndExtend(record, extend);
    size_t dataSize = extend.size();

    switch (action) {
        case INSERT: {
            status = cloudDb->BatchInsert(context->GetTableName(), std::move(record), extend);
            context->MoveInExtend(extend);
            break;
        }
        case UPDATE: {
            status = cloudDb->BatchUpdate(context->GetTableName(), std::move(record), extend);
            // no need to MoveIn, only insert need extend for insert gid
            break;
        }
        case DELETE: {
            status = cloudDb->BatchDelete(context->GetTableName(), extend);
            // no need to MoveIn, only insert need extend for insert gid
            break;
        }
        default: {
            LOGE("DMLActionTask can only be used on INSERT/UPDATE/DELETE.");
            return INVALID_ARGS;
        }
    }
    if (status == OK) {
        context->SetInfo(dataSize, dataSize, 0u);
    } else {
        LOGE("[CloudSyncer] Cloud BATCH UPLOAD failed.");
        context->SetInfo(dataSize, 0u, dataSize);
    }
    return status;
}

void CloudDBProxy::InnerActionTask(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action)
{
    DBStatus status = OK;
    bool setResAlready = false;
    LOGD("[CloudDBProxy] action %" PRIu8 " begin", static_cast<uint8_t>(action));
    switch (action) {
        case INSERT:
        case UPDATE:
        case DELETE:
            status = DMLActionTask(context, cloudDb, action);
            break;
        case QUERY: {
            VBucket queryExtend;
            std::vector<VBucket> data;
            context->MoveOutQueryExtendAndData(queryExtend, data);
            status = cloudDb->Query(context->GetTableName(), queryExtend, data);
            context->MoveInQueryExtendAndData(queryExtend, data);

            if (status == QUERY_END) {
                setResAlready = true;
                context->SetActionRes(-E_QUERY_END);
            }
            break;
        }
        case LOCK: {
            status = InnerActionLock(context, cloudDb);
            break;
        }
        case UNLOCK:
            status = cloudDb->UnLock();
            break;
        case HEARTBEAT:
            status = cloudDb->HeartBeat();
            break;
        default: // should not happen
            status = DB_ERROR;
    }
    LOGD("[CloudDBProxy] action %" PRIu8 " end res:%d", static_cast<uint8_t>(action), static_cast<int>(status));

    if (!setResAlready) {
        context->SetActionRes(GetInnerErrorCode(status));
    }

    context->FinishAndNotify();
    {
        std::lock_guard<std::mutex> uniqueLock(asyncTaskMutex_);
        asyncTaskCount_--;
    }
    asyncTaskCv_.notify_all();
}

DBStatus CloudDBProxy::InnerActionLock(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb)
{
    DBStatus status = OK;
    std::pair<int, uint64_t> lockRet;
    std::pair<DBStatus, uint64_t> lockStatus = cloudDb->Lock();
    if (lockStatus.first != OK) {
        status = lockStatus.first;
    } else if (lockStatus.second == 0) {
        status = CLOUD_ERROR;
    }
    lockRet.second = lockStatus.second;
    lockRet.first = GetInnerErrorCode(status);
    context->MoveInLockStatus(lockRet);
    return status;
}

int CloudDBProxy::GetInnerErrorCode(DBStatus status)
{
    switch (status) {
        case OK:
            return E_OK;
        case CLOUD_NETWORK_ERROR:
            return -E_CLOUD_NETWORK_ERROR;
        case CLOUD_SYNC_UNSET:
            return -E_CLOUD_SYNC_UNSET;
        case CLOUD_FULL_RECORDS:
            return -E_CLOUD_FULL_RECORDS;
        case CLOUD_LOCK_ERROR:
            return -E_CLOUD_LOCK_ERROR;
        case CLOUD_ASSET_SPACE_INSUFFICIENT:
            return -E_CLOUD_ASSET_SPACE_INSUFFICIENT;
        default:
            return -E_CLOUD_ERROR;
    }
}

CloudDBProxy::CloudActionContext::CloudActionContext()
    : actionFinished_(false),
      actionRes_(OK),
      totalCount_(0u),
      successCount_(0u),
      failedCount_(0u)
{
}

void CloudDBProxy::CloudActionContext::MoveInRecordAndExtend(std::vector<VBucket> &record,
    std::vector<VBucket> &extend)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    record_ = std::move(record);
    extend_ = std::move(extend);
}

void CloudDBProxy::CloudActionContext::MoveInExtend(std::vector<VBucket> &extend)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    extend_ = std::move(extend);
}

void CloudDBProxy::CloudActionContext::MoveOutRecordAndExtend(std::vector<VBucket> &record,
    std::vector<VBucket> &extend)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    record = std::move(record_);
    extend = std::move(extend_);
}

void CloudDBProxy::CloudActionContext::MoveInQueryExtendAndData(VBucket &extend, std::vector<VBucket> &data)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    queryExtend_ = std::move(extend);
    data_ = std::move(data);
}

void CloudDBProxy::CloudActionContext::MoveOutQueryExtendAndData(VBucket &extend, std::vector<VBucket> &data)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    extend = std::move(queryExtend_);
    data = std::move(data_);
}

void CloudDBProxy::CloudActionContext::MoveInLockStatus(std::pair<int, uint64_t> &lockStatus)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    lockStatus_ = std::move(lockStatus);
}

void CloudDBProxy::CloudActionContext::MoveOutLockStatus(std::pair<int, uint64_t> &lockStatus)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    lockStatus = std::move(lockStatus_);
}

bool CloudDBProxy::CloudActionContext::WaitForRes(int64_t timeout)
{
    std::unique_lock<std::mutex> uniqueLock(actionMutex_);
    if (timeout == 0) {
        actionCv_.wait(uniqueLock, [this]() {
            return actionFinished_;
        });
        return true;
    }
    return actionCv_.wait_for(uniqueLock, std::chrono::milliseconds(timeout), [this]() {
        return actionFinished_;
    });
}

void CloudDBProxy::CloudActionContext::FinishAndNotify()
{
    {
        std::lock_guard<std::mutex> autoLock(actionMutex_);
        actionFinished_ = true;
    }
    actionCv_.notify_all();
}

void CloudDBProxy::CloudActionContext::SetActionRes(int res)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    actionRes_ = res;
}

int CloudDBProxy::CloudActionContext::GetActionRes()
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    return actionRes_;
}

Info CloudDBProxy::CloudActionContext::GetInfo()
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    Info info;
    info.total = totalCount_;
    info.successCount = successCount_;
    info.failCount = failedCount_;
    return info;
}

void CloudDBProxy::CloudActionContext::SetInfo(const uint32_t &totalCount,
    const uint32_t &successCount, const uint32_t &failedCount)
{
    totalCount_ = totalCount;
    successCount_ = successCount;
    failedCount_ = failedCount;
}

void CloudDBProxy::CloudActionContext::SetTableName(const std::string &tableName)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    tableName_ = tableName;
}

std::string CloudDBProxy::CloudActionContext::GetTableName()
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    return tableName_;
}
}