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
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
CloudDBProxy::CloudDBProxy()
    : timeout_(0)
{
}

void CloudDBProxy::SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB)
{
    std::unique_lock<std::shared_mutex> writeLock(cloudMutex_);
    if (!iCloudDb_) {
        iCloudDb_ = cloudDB;
    }
}

int CloudDBProxy::SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
{
    std::unique_lock<std::shared_mutex> writeLock(cloudMutex_);
    auto it = std::find_if(cloudDBs.begin(), cloudDBs.end(), [](const auto &item) { return item.second == nullptr; });
    if (it != cloudDBs.end()) {
        LOGE("[CloudDBProxy] User %s setCloudDB with nullptr", it->first.c_str());
        return -E_INVALID_ARGS;
    }
    cloudDbs_ = cloudDBs;
    return E_OK;
}

const std::map<std::string, std::shared_ptr<ICloudDb>> CloudDBProxy::GetCloudDB() const
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    return cloudDbs_;
}

void CloudDBProxy::SwitchCloudDB(const std::string &user)
{
    std::unique_lock<std::shared_mutex> writeLock(cloudMutex_);
    if (cloudDbs_.find(user) == cloudDbs_.end()) {
        return;
    }
    iCloudDb_ = cloudDbs_[user];
}

void CloudDBProxy::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    std::unique_lock<std::shared_mutex> writeLock(assetLoaderMutex_);
    iAssetLoader_ = loader;
}

static void RecordSyncDataTimeStampLog(std::vector<VBucket> &data, uint8_t action)
{
    if (data.empty()) {
        LOGI("[CloudDBProxy] sync data is empty");
        return;
    }

    int64_t first = 0;
    int errCode = CloudStorageUtils::GetValueFromVBucket<int64_t>(CloudDbConstant::MODIFY_FIELD, data[0], first);
    if (errCode != E_OK) {
        LOGE("get first modify time for bucket failed, %d", errCode);
        return;
    }

    int64_t last = 0;
    errCode = CloudStorageUtils::GetValueFromVBucket<int64_t>(CloudDbConstant::MODIFY_FIELD, data[data.size() - 1],
        last);
    if (errCode != E_OK) {
        LOGE("get last modify time for bucket failed, %d", errCode);
        return;
    }

    LOGI("[CloudDBProxy] sync action is %d and size is %d, sync data: first timestamp %lld, last timestamp %lld",
        action, data.size(), first, last);
}

int CloudDBProxy::BatchInsert(const std::string &tableName, std::vector<VBucket> &record,
    std::vector<VBucket> &extend, Info &uploadInfo, uint32_t &retryCount)
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
    retryCount = context->GetRetryCount();
    context->MoveOutRecordAndExtend(record, extend);
    return errCode;
}

int CloudDBProxy::BatchUpdate(const std::string &tableName, std::vector<VBucket> &record,
    std::vector<VBucket> &extend, Info &uploadInfo, uint32_t &retryCount)
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
    retryCount = context->GetRetryCount();
    context->MoveOutRecordAndExtend(record, extend);
    return errCode;
}

int CloudDBProxy::BatchDelete(const std::string &tableName, std::vector<VBucket> &record, std::vector<VBucket> &extend,
    Info &uploadInfo, uint32_t &retryCount)
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
    retryCount = context->GetRetryCount();
    context->MoveOutRecordAndExtend(record, extend);
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
    for (auto &item : data) {
        for (auto &row : item) {
            auto assets = std::get_if<Assets>(&row.second);
            if (assets == nullptr) {
                continue;
            }
            DBCommon::RemoveDuplicateAssetsData(*assets);
        }
    }
    RecordSyncDataTimeStampLog(data, QUERY);
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
    std::vector<std::shared_ptr<ICloudDb>> waitForClose;
    {
        std::unique_lock<std::shared_mutex> writeLock(cloudMutex_);
        if (iCloudDb_ != nullptr) {
            iCloudDb = iCloudDb_;
            iCloudDb_ = nullptr;
        }
        for (const auto &item : cloudDbs_) {
            if (iCloudDb == item.second) {
                iCloudDb = nullptr;
            }
            waitForClose.push_back(item.second);
        }
        cloudDbs_.clear();
    }
    LOGD("[CloudDBProxy] call cloudDb close begin");
    DBStatus status = OK;
    if (iCloudDb != nullptr) {
        status = iCloudDb->Close();
    }
    for (const auto &item : waitForClose) {
        DBStatus ret = item->Close();
        status = (status == OK ? ret : status);
    }
    if (status != OK) {
        LOGW("[CloudDBProxy] cloud db close failed %d", static_cast<int>(status));
    }
    waitForClose.clear();
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
    return iCloudDb_ == nullptr && cloudDbs_.empty();
}

int CloudDBProxy::Download(const std::string &tableName, const std::string &gid, const Type &prefix,
    std::map<std::string, Assets> &assets)
{
    if (assets.empty()) {
        return E_OK;
    }
    std::shared_lock<std::shared_mutex> readLock(assetLoaderMutex_);
    if (iAssetLoader_ == nullptr) {
        LOGE("Asset loader has not been set %d", -E_NOT_SET);
        return -E_NOT_SET;
    }
    DBStatus status = iAssetLoader_->Download(tableName, gid, prefix, assets);
    if (status != OK) {
        LOGW("[CloudDBProxy] download asset failed %d", static_cast<int>(status));
    }
    return GetInnerErrorCode(status);
}

int CloudDBProxy::RemoveLocalAssets(const std::vector<Asset> &assets)
{
    if (assets.empty()) {
        return E_OK;
    }
    std::shared_lock<std::shared_mutex> readLock(assetLoaderMutex_);
    if (iAssetLoader_ == nullptr) {
        LOGW("Asset loader has not been set");
        return E_OK;
    }
    DBStatus status = iAssetLoader_->RemoveLocalAssets(assets);
    if (status != OK) {
        LOGE("[CloudDBProxy] remove local asset failed %d", static_cast<int>(status));
        return -E_REMOVE_ASSETS_FAILED;
    }
    return E_OK;
}

int CloudDBProxy::RemoveLocalAssets(const std::string &tableName, const std::string &gid, const Type &prefix,
    std::map<std::string, Assets> &assets)
{
    if (assets.empty()) {
        return E_OK;
    }
    std::shared_lock<std::shared_mutex> readLock(assetLoaderMutex_);
    if (iAssetLoader_ == nullptr) {
        LOGE("Asset loader has not been set %d", -E_NOT_SET);
        return -E_NOT_SET;
    }
    DBStatus status = iAssetLoader_->RemoveLocalAssets(tableName, gid, prefix, assets);
    if (status != OK) {
        LOGE("[CloudDBProxy] remove local asset failed %d", static_cast<int>(status));
        return -E_REMOVE_ASSETS_FAILED;
    }
    return E_OK;
}

std::pair<int, std::string> CloudDBProxy::GetEmptyCursor(const std::string &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(cloudMutex_);
    if (iCloudDb_ == nullptr) {
        return { -E_CLOUD_ERROR, "" };
    }
    std::shared_ptr<ICloudDb> cloudDb = iCloudDb_;
    std::shared_ptr<CloudActionContext> context = std::make_shared<CloudActionContext>();
    context->SetTableName(tableName);
    int errCode = InnerAction(context, cloudDb, GET_EMPTY_CURSOR);
    std::pair<int, std::string> cursorStatus;
    context->MoveOutCursorStatus(cursorStatus);
    cursorStatus.first = errCode;
    return cursorStatus;
}

int CloudDBProxy::InnerAction(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action)
{
    if (action >= InnerActionCode::INVALID_ACTION) {
        return -E_INVALID_ARGS;
    }
    InnerActionTask(context, cloudDb, action);
    return context->GetActionRes();
}

DBStatus CloudDBProxy::DMLActionTask(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb, InnerActionCode action)
{
    DBStatus status = OK;
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    context->MoveOutRecordAndExtend(record, extend);
    RecordSyncDataTimeStampLog(extend, action);
    uint32_t recordSize = record.size();

    switch (action) {
        case INSERT: {
            status = cloudDb->BatchInsert(context->GetTableName(), std::move(record), extend);
            context->MoveInExtend(extend);
            context->SetInfo(CloudWaterType::INSERT, status, recordSize);
            break;
        }
        case UPDATE: {
            status = cloudDb->BatchUpdate(context->GetTableName(), std::move(record), extend);
            context->MoveInExtend(extend);
            context->SetInfo(CloudWaterType::UPDATE, status, recordSize);
            break;
        }
        case DELETE: {
            status = cloudDb->BatchDelete(context->GetTableName(), extend);
            context->MoveInRecordAndExtend(record, extend);
            context->SetInfo(CloudWaterType::DELETE, status, recordSize);
            break;
        }
        default: {
            LOGE("DMLActionTask can only be used on INSERT/UPDATE/DELETE.");
            return INVALID_ARGS;
        }
    }
    if (status == CLOUD_VERSION_CONFLICT) {
        LOGI("[CloudSyncer] Version conflict during cloud batch upload.");
    } else if (status != OK) {
        LOGE("[CloudSyncer] Cloud BATCH UPLOAD failed.");
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
            status = QueryAction(context, cloudDb);
            if (status == QUERY_END) {
                setResAlready = true;
            }
            break;
        }
        case GET_EMPTY_CURSOR:
            status = InnerActionGetEmptyCursor(context, cloudDb);
            break;
        case LOCK:
            status = InnerActionLock(context, cloudDb);
            break;
        case UNLOCK:
            status = cloudDb->UnLock();
            if (status != OK) {
                LOGE("[CloudDBProxy] UnLock cloud DB failed: %d", static_cast<int>(status));
            }
            break;
        case HEARTBEAT:
            status = cloudDb->HeartBeat();
            if (status != OK) {
                LOGE("[CloudDBProxy] Heart beat error: %d", static_cast<int>(status));
            }
            break;
        default: // should not happen
            status = DB_ERROR;
    }
    LOGD("[CloudDBProxy] action %" PRIu8 " end res:%d", static_cast<uint8_t>(action), static_cast<int>(status));

    if (!setResAlready) {
        context->SetActionRes(GetInnerErrorCode(status));
    }

    context->FinishAndNotify();
}

DBStatus CloudDBProxy::InnerActionLock(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb)
{
    DBStatus status = OK;
    std::pair<int, uint64_t> lockRet;
    std::pair<DBStatus, uint64_t> lockStatus = cloudDb->Lock();
    if (lockStatus.first != OK) {
        status = lockStatus.first;
        LOGE("[CloudDBProxy] Lock cloud DB failed: %d", static_cast<int>(status));
    } else if (lockStatus.second == 0) {
        LOGE("[CloudDBProxy] Lock successfully but timeout is 0");
        status = CLOUD_ERROR;
    }
    lockRet.second = lockStatus.second;
    lockRet.first = GetInnerErrorCode(status);
    context->MoveInLockStatus(lockRet);
    return status;
}

DBStatus CloudDBProxy::InnerActionGetEmptyCursor(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb)
{
    std::string tableName = context->GetTableName();
    std::pair<DBStatus, std::string> cursorStatus = cloudDb->GetEmptyCursor(tableName);
    DBStatus status = OK;
    if (cursorStatus.first != OK) {
        status = cursorStatus.first;
        LOGE("[CloudDBProxy] Get empty cursor failed: %d", static_cast<int>(status));
    }
    std::pair<int, std::string> cursorRet;
    cursorRet.second = cursorStatus.second;
    cursorRet.first = GetInnerErrorCode(status);
    context->MoveInCursorStatus(cursorRet);
    return status;
}

int CloudDBProxy::GetInnerErrorCode(DBStatus status)
{
    if (status < DB_ERROR || status >= BUTT_STATUS) {
        return static_cast<int>(status);
    }
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
        case CLOUD_VERSION_CONFLICT:
            return -E_CLOUD_VERSION_CONFLICT;
        case CLOUD_RECORD_EXIST_CONFLICT:
            return -E_CLOUD_RECORD_EXIST_CONFLICT;
        default:
            return -E_CLOUD_ERROR;
    }
}

DBStatus CloudDBProxy::QueryAction(const std::shared_ptr<CloudActionContext> &context,
    const std::shared_ptr<ICloudDb> &cloudDb)
{
    VBucket queryExtend;
    std::vector<VBucket> data;
    context->MoveOutQueryExtendAndData(queryExtend, data);
    DBStatus status = cloudDb->Query(context->GetTableName(), queryExtend, data);
    context->MoveInQueryExtendAndData(queryExtend, data);
    if (status == QUERY_END) {
        context->SetActionRes(-E_QUERY_END);
    }
    return status;
}

CloudDBProxy::CloudActionContext::CloudActionContext()
    : actionFinished_(false),
      actionRes_(OK),
      totalCount_(0u),
      successCount_(0u),
      failedCount_(0u),
      retryCount_(0u)
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

void CloudDBProxy::CloudActionContext::MoveInCursorStatus(std::pair<int, std::string> &cursorStatus)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    cursorStatus_ = std::move(cursorStatus);
}

void CloudDBProxy::CloudActionContext::MoveOutCursorStatus(std::pair<int, std::string> &cursorStatus)
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    cursorStatus = std::move(cursorStatus_);
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

bool CloudDBProxy::CloudActionContext::IsEmptyAssetId(const Assets &assets)
{
    for (auto &asset : assets) {
        if (asset.assetId.empty()) {
            return true;
        }
    }
    return false;
}

bool CloudDBProxy::CloudActionContext::IsRecordActionFail(const VBucket &extend, const CloudWaterType &type,
    DBStatus status)
{
    if (DBCommon::IsRecordAssetsMissing(extend) || DBCommon::IsRecordIgnoredForReliability(extend, type) ||
        DBCommon::IsRecordIgnored(extend)) {
        return false;
    }
    if (extend.count(CloudDbConstant::GID_FIELD) == 0 || DBCommon::IsRecordFailed(extend, status)) {
        return true;
    }
    bool isInsert = type == CloudWaterType::INSERT;
    auto gid = std::get_if<std::string>(&extend.at(CloudDbConstant::GID_FIELD));
    if (gid == nullptr || (isInsert && (*gid).empty())) {
        return true;
    }
    for (auto &entry : extend) {
        auto asset = std::get_if<Asset>(&entry.second);
        if (asset != nullptr && (*asset).assetId.empty()) {
            return true;
        }
        auto assets = std::get_if<Assets>(&entry.second);
        if (assets != nullptr && IsEmptyAssetId(*assets)) {
            return true;
        }
    }
    return false;
}

void CloudDBProxy::CloudActionContext::SetInfo(const CloudWaterType &type, DBStatus status, uint32_t size)
{
    totalCount_ = size;
    retryCount_ = 0; // reset retryCount in each batch

    // totalCount_ should be equal to extend_ or batch data failed.
    if (totalCount_ != extend_.size()) {
        failedCount_ += totalCount_;
        return;
    }
    for (auto &extend : extend_) {
        if (DBCommon::IsRecordVersionConflict(extend)) {
            retryCount_++;
        } else if (IsRecordActionFail(extend, type, status)) {
            failedCount_++;
        } else {
            successCount_++;
        }
    }
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

uint32_t CloudDBProxy::CloudActionContext::GetRetryCount()
{
    std::lock_guard<std::mutex> autoLock(actionMutex_);
    return retryCount_;
}

void CloudDBProxy::SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback)
{
    std::lock_guard<std::mutex> autoLock(genVersionMutex_);
    genVersionCallback_ = callback;
    LOGI("[CloudDBProxy] Set generate cloud version callback ok");
}

bool CloudDBProxy::IsExistCloudVersionCallback() const
{
    std::lock_guard<std::mutex> autoLock(genVersionMutex_);
    return genVersionCallback_ != nullptr;
}

std::pair<int, std::string> CloudDBProxy::GetCloudVersion(const std::string &originVersion) const
{
    GenerateCloudVersionCallback genVersionCallback;
    {
        std::lock_guard<std::mutex> autoLock(genVersionMutex_);
        if (genVersionCallback_ == nullptr) {
            return {-E_NOT_SUPPORT, ""};
        }
        genVersionCallback = genVersionCallback_;
    }
    LOGI("[CloudDBProxy] Begin get cloud version");
    std::string version = genVersionCallback(originVersion);
    LOGI("[CloudDBProxy] End get cloud version");
    return {E_OK, version};
}

void CloudDBProxy::SetPrepareTraceId(const std::string &traceId) const
{
    std::shared_ptr<ICloudDb> iCloudDb = nullptr;
    std::unique_lock<std::shared_mutex> writeLock(cloudMutex_);
    if (iCloudDb_ != nullptr) {
        iCloudDb = iCloudDb_;
        iCloudDb->SetPrepareTraceId(traceId);
    }
}

int CloudDBProxy::BatchDownload(const std::string &tableName, std::vector<IAssetLoader::AssetRecord> &downloadAssets)
{
    return BatchOperateAssetsWithAllRecords(tableName, downloadAssets, CloudDBProxy::BATCH_DOWNLOAD);
}

int CloudDBProxy::BatchRemoveLocalAssets(const std::string &tableName,
    std::vector<IAssetLoader::AssetRecord> &removeAssets)
{
    return BatchOperateAssetsWithAllRecords(tableName, removeAssets, CloudDBProxy::BATCH_REMOVE_LOCAL);
}

int CloudDBProxy::BatchOperateAssetsWithAllRecords(const std::string &tableName,
    std::vector<IAssetLoader::AssetRecord> &allRecords, const InnerBatchOpType operationType)
{
    std::vector<IAssetLoader::AssetRecord> nonEmptyRecords;
    auto indexes = GetNotEmptyAssetRecords(allRecords, nonEmptyRecords);
    if (nonEmptyRecords.empty()) {
        return E_OK;
    }

    int errCode = BatchOperateAssetsInner(tableName, nonEmptyRecords, operationType);

    CopyAssetsBack(allRecords, indexes, nonEmptyRecords);
    return errCode;
}

int CloudDBProxy::BatchOperateAssetsInner(const std::string &tableName,
    std::vector<IAssetLoader::AssetRecord> &necessaryRecords, const InnerBatchOpType operationType)
{
    std::shared_lock<std::shared_mutex> readLock(assetLoaderMutex_);
    if (iAssetLoader_ == nullptr) {
        LOGE("[CloudDBProxy] Asset loader has not been set %d", -E_NOT_SET);
        return -E_NOT_SET;
    }
    if (operationType == CloudDBProxy::BATCH_DOWNLOAD) {
        iAssetLoader_->BatchDownload(tableName, necessaryRecords);
    } else if (operationType == CloudDBProxy::BATCH_REMOVE_LOCAL) {
        iAssetLoader_->BatchRemoveLocalAssets(tableName, necessaryRecords);
    } else {
        LOGE("[CloudDBProxy][BatchOperateAssetsInner] Internal error! Operation type is invalid: %d", operationType);
        return -E_NOT_SET;
    }
    return E_OK;
}

std::vector<int> CloudDBProxy::GetNotEmptyAssetRecords(std::vector<IAssetLoader::AssetRecord> &originalRecords,
    std::vector<IAssetLoader::AssetRecord> &nonEmptyRecords)
{
    std::vector<int> indexes;
    if (originalRecords.empty()) {
        return indexes;
    }

    int index = 0;
    for (auto &record : originalRecords) {
        bool isEmpty = true;
        for (const auto &recordAssets : record.assets) {
            if (!recordAssets.second.empty()) {
                isEmpty = false;
                break;
            }
        }
        if (!isEmpty) {
            indexes.push_back(index);
            IAssetLoader::AssetRecord newRecord = {
                record.gid,
                record.prefix,
                std::move(record.assets)
            };
            nonEmptyRecords.emplace_back(newRecord);
        }
        index++;
    }
    return indexes;
}

void CloudDBProxy::CopyAssetsBack(std::vector<IAssetLoader::AssetRecord> &originalRecords,
    const std::vector<int> &indexes, std::vector<IAssetLoader::AssetRecord> &newRecords)
{
    int i = 0;
    for (const auto index : indexes) {
        originalRecords[index].status = newRecords[i].status;
        originalRecords[index].assets = std::move(newRecords[i].assets);
        i++;
    }
}
}
