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
#include "virtual_cloud_db.h"

#include <thread>
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "db_constant.h"
#include "log_print.h"
#include "relational_store_manager.h"
#include "time_helper.h"

namespace DistributedDB {
namespace {
    const char *g_deleteField = CloudDbConstant::DELETE_FIELD;
    const char *g_gidField = CloudDbConstant::GID_FIELD;
    const char *g_cursorField = CloudDbConstant::CURSOR_FIELD;
    const char *g_modifiedField = CloudDbConstant::MODIFY_FIELD;
    const char *g_queryField = CloudDbConstant::QUERY_FIELD;
}

DBStatus VirtualCloudDb::BatchInsert(const std::string &tableName, std::vector<VBucket> &&record,
    std::vector<VBucket> &extend)
{
    if (cloudError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    if (record.size() != extend.size()) {
        LOGE("[VirtualCloudDb] not equal records");
        return DB_ERROR;
    }
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    DBStatus status = InnerBatchInsert(tableName, std::move(record), extend);
    if (status != OK) {
        return status;
    }
    if (missingExtendCount_ > 0) {
        extend.erase(extend.end());
    } else if (missingExtendCount_ < 0) {
        VBucket vBucket;
        extend.push_back(vBucket);
    }
    if (insertFailedCount_ > 0) {
        insertFailedCount_--;
        LOGW("[VirtualCloud] Insert failed by testcase config");
        return DB_ERROR;
    }
    if (cloudNetworkError_) {
        return CLOUD_NETWORK_ERROR;
    }
    return OK;
}

DBStatus VirtualCloudDb::InnerBatchInsert(const std::string &tableName, std::vector<VBucket> &&record,
    std::vector<VBucket> &extend)
{
    DBStatus res = OK;
    for (size_t i = 0; i < record.size(); ++i) {
        if (extend[i].find(g_gidField) != extend[i].end()) {
            LOGE("[VirtualCloudDb] Insert data should not have gid");
            return DB_ERROR;
        }
        if (forkUploadConflictFunc_) {
            DBStatus ret = forkUploadConflictFunc_(tableName, extend[i], record[i], cloudData_[tableName]);
            if (ret != OK) {
                res = ret;
                continue;
            }
        }
        if (conflictInUpload_) {
            extend[i][CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_EXIST_CONFLICT);
        }
        extend[i][g_gidField] = std::to_string(currentGid_++);
        extend[i][g_cursorField] = std::to_string(currentCursor_++);
        extend[i][g_deleteField] = false;
        extend[i][CloudDbConstant::VERSION_FIELD] = std::to_string(currentVersion_++);
        AddAssetIdForExtend(record[i], extend[i]);
        if (forkUploadFunc_) {
            forkUploadFunc_(tableName, extend[i]);
        }
        CloudData cloudData = {
            .record = std::move(record[i]),
            .extend = extend[i]
        };
        cloudData_[tableName].push_back(cloudData);
        auto gid = std::get<std::string>(extend[i][g_gidField]);
    }
    return res;
}

DBStatus VirtualCloudDb::BatchInsertWithGid(const std::string &tableName, std::vector<VBucket> &&record,
    std::vector<VBucket> &extend)
{
    if (cloudError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    if (record.size() != extend.size()) {
        LOGE("[VirtualCloudDb] not equal records");
        return DB_ERROR;
    }
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    for (size_t i = 0; i < record.size(); ++i) {
        if (extend[i].find(g_gidField) == extend[i].end()) {
            extend[i][g_gidField] = std::to_string(currentGid_++);
        } else {
            currentGid_++;
        }
        extend[i][g_cursorField] = std::to_string(currentCursor_++);
        extend[i][g_deleteField] = false;
        extend[i][CloudDbConstant::VERSION_FIELD] = std::to_string(currentVersion_++);
        AddAssetIdForExtend(record[i], extend[i]);
        if (forkUploadFunc_) {
            forkUploadFunc_(tableName, extend[i]);
        }
        CloudData cloudData = {
            .record = std::move(record[i]),
            .extend = extend[i]
        };
        cloudData_[tableName].push_back(cloudData);
    }
    if (missingExtendCount_ > 0) {
        extend.erase(extend.end());
    } else if (missingExtendCount_ < 0) {
        VBucket vBucket;
        extend.push_back(vBucket);
    }
    if (cloudNetworkError_) {
        return CLOUD_NETWORK_ERROR;
    }
    LOGI("[VirtualCloudDb] BatchInsertWithGid records");
    return OK;
}

DBStatus VirtualCloudDb::BatchUpdate(const std::string &tableName, std::vector<VBucket> &&record,
    std::vector<VBucket> &extend)
{
    if (cloudError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    return InnerUpdate(tableName, std::move(record), extend, false);
}

DBStatus VirtualCloudDb::BatchDelete(const std::string &tableName, std::vector<VBucket> &extend)
{
    if (cloudError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    std::vector<VBucket> record;
    for (size_t i = 0; i < extend.size(); ++i) {
        record.emplace_back();
    }
    return InnerUpdate(tableName, std::move(record), extend, true);
}

DBStatus VirtualCloudDb::HeartBeat()
{
    heartbeatCount_++;
    if (actionStatus_ != OK) {
        return actionStatus_;
    }
    if (cloudError_) {
        return DB_ERROR;
    }
    if (heartbeatBlockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeatBlockTimeMs_));
    }
    if (heartbeatError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    lockStatus_ = true;
    return OK;
}

std::pair<DBStatus, uint32_t> VirtualCloudDb::Lock()
{
    lockCount_++;
    if (actionStatus_ != OK) {
        return { actionStatus_, DBConstant::MIN_TIMEOUT };
    }
    if (cloudError_) {
        return { DB_ERROR, DBConstant::MIN_TIMEOUT };
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    lockStatus_ = true;
    return { OK, DBConstant::MIN_TIMEOUT };
}

DBStatus VirtualCloudDb::UnLock()
{
    if (cloudError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    lockStatus_ = false;
    return OK;
}

DBStatus VirtualCloudDb::Close()
{
    if (cloudError_) {
        return DB_ERROR;
    }
    return OK;
}

std::pair<DBStatus, std::string> VirtualCloudDb::GetEmptyCursor(const std::string &tableName)
{
    return { OK, "0" };
}

DBStatus VirtualCloudDb::DeleteByGid(const std::string &tableName, VBucket &extend)
{
    for (auto &tableData : cloudData_[tableName]) {
        if (std::get<std::string>(tableData.extend[g_gidField]) == std::get<std::string>(extend[g_gidField])) {
            tableData.extend[g_modifiedField] = (int64_t)TimeHelper::GetSysCurrentTime() /
                CloudDbConstant::TEN_THOUSAND;
            tableData.extend[g_deleteField] = true;
            tableData.extend[g_cursorField] = std::to_string(currentCursor_++);
            LOGD("[VirtualCloudDb] DeleteByGid, gid %s", std::get<std::string>(extend[g_gidField]).c_str());
            tableData.record.clear();
            break;
        }
    }
    return OK;
}

DBStatus VirtualCloudDb::Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data)
{
    LOGW("begin query %s", tableName.c_str());
    if (actionStatus_ != OK) {
        return actionStatus_;
    }
    if (cloudError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    if (forkFunc_) {
        forkFunc_(tableName, extend);
    }
    if (queryTimes_.find(tableName) == queryTimes_.end()) {
        queryTimes_.try_emplace(tableName, 0);
    }
    queryTimes_[tableName]++;
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    if (cloudData_.find(tableName) == cloudData_.end()) {
        return QUERY_END;
    }
    std::string cursor = std::get<std::string>(extend[g_cursorField]);
    bool isIncreCursor = (cursor.substr(0, increPrefix_.size()) == increPrefix_);
    LOGD("extend size: %zu type: %zu  expect: %zu, cursor: %s", extend.size(), extend[g_cursorField].index(),
        TYPE_INDEX<std::string>, cursor.c_str());
    if (isIncreCursor) {
        GetCloudData(cursor, isIncreCursor, incrementCloudData_[tableName], data, extend);
    } else {
        cursor = cursor.empty() ? "0" : cursor;
        GetCloudData(cursor, isIncreCursor, cloudData_[tableName], data, extend);
    }
    if (!isIncreCursor && data.empty() && isSetCrementCloudData_) {
        extend[g_cursorField] = increPrefix_;
        return OK;
    }
    return (data.empty() || data.size() < static_cast<size_t>(queryLimit_)) ? QUERY_END : OK;
}

void VirtualCloudDb::GetCloudData(const std::string &cursor, bool isIncreCursor, std::vector<CloudData> allData,
    std::vector<VBucket> &data, VBucket &extend)
{
    std::vector<QueryNode> queryNodes;
    auto it = extend.find(g_queryField);
    if (it != extend.end()) {
        Bytes bytes = std::get<Bytes>(extend[g_queryField]);
        DBStatus status = OK;
        queryNodes = RelationalStoreManager::ParserQueryNodes(bytes, status);
    }
    for (auto &tableData : allData) {
        std::string srcCursor = std::get<std::string>(tableData.extend[g_cursorField]);
        if ((!isIncreCursor && std::stol(srcCursor) > std::stol(cursor)) || isIncreCursor) {
            if ((!queryNodes.empty()) && (!IsCloudGidMatching(queryNodes, tableData.extend)) &&
                (!IsPrimaryKeyMatching(queryNodes, tableData.record))) {
                continue;
            }
            VBucket bucket = tableData.record;
            for (const auto &ex: tableData.extend) {
                bucket.insert(ex);
            }
            data.push_back(std::move(bucket));
        }
        if (data.size() >= static_cast<size_t>(queryLimit_)) {
            return;
        }
    }
}

bool VirtualCloudDb::IsPrimaryKeyMatching(const std::vector<QueryNode> &queryNodes, VBucket &record)
{
    if (record.empty()) {
        return false;
    }
    for (const auto &queryNode : queryNodes) {
        if ((queryNode.type == QueryNodeType::IN) && (queryNode.fieldName != g_gidField)) {
            if (IsPrimaryKeyMatchingInner(queryNode, record)) {
                return true;
            }
        }
    }
    return false;
}

bool VirtualCloudDb::IsPrimaryKeyMatchingInner(const QueryNode &queryNode, VBucket &record)
{
    for (const auto &value : queryNode.fieldValue) {
        size_t type = record[queryNode.fieldName].index();
        switch (type) {
            case TYPE_INDEX<std::string>: {
                if (std::get<std::string>(record[queryNode.fieldName]) == std::get<std::string>(value)) {
                    return true;
                }
                break;
            }
            case TYPE_INDEX<int64_t>: {
                if (std::get<int64_t>(record[queryNode.fieldName]) == std::get<int64_t>(value)) {
                    return true;
                }
                break;
            }
            case TYPE_INDEX<double>: {
                if (std::get<double>(record[queryNode.fieldName]) == std::get<double>(value)) {
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }
    return false;
}

bool VirtualCloudDb::IsCloudGidMatching(const std::vector<QueryNode> &queryNodes, VBucket &extend)
{
    for (const auto &queryNode : queryNodes) {
        if ((queryNode.type == QueryNodeType::IN) && (queryNode.fieldName == g_gidField)) {
            if (IsCloudGidMatchingInner(queryNode, extend)) {
                return true;
            }
        }
    }
    return false;
}

bool VirtualCloudDb::IsCloudGidMatchingInner(const QueryNode &queryNode, VBucket &extend)
{
    for (const auto &value : queryNode.fieldValue) {
        if (std::get<std::string>(extend[g_gidField]) == std::get<std::string>(value)) {
            return true;
        }
    }
    return false;
}

DBStatus VirtualCloudDb::InnerUpdate(const std::string &tableName, std::vector<VBucket> &&record,
    std::vector<VBucket> &extend, bool isDelete)
{
    if (record.size() != extend.size()) {
        return DB_ERROR;
    }
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    DBStatus res = InnerUpdateWithoutLock(tableName, std::move(record), extend, isDelete);
    if (res != OK) {
        return res;
    }
    if (missingExtendCount_ > 0) {
        extend.erase(extend.end());
    } else if (missingExtendCount_ < 0) {
        VBucket vBucket;
        extend.push_back(vBucket);
    }
    if (isDelete) {
        for (auto &vb: extend) {
            for (auto &[key, value]: vb) {
                std::ignore = std::move(value);
                vb.insert_or_assign(key, value);
            }
        }
    }
    if (cloudNetworkError_) {
        return CLOUD_NETWORK_ERROR;
    }
    return OK;
}

DBStatus VirtualCloudDb::InnerUpdateWithoutLock(const std::string &tableName, std::vector<VBucket> &&record,
    std::vector<VBucket> &extend, bool isDelete)
{
    DBStatus res = OK;
    for (size_t i = 0; i < record.size(); ++i) {
        if (extend[i].find(g_gidField) == extend[i].end()) {
            LOGE("[VirtualCloudDb] Update data should have gid");
            return DB_ERROR;
        }
        if (conflictInUpload_) {
            extend[i][CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_EXIST_CONFLICT);
        }
        extend[i][g_cursorField] = std::to_string(currentCursor_++);
        AddAssetIdForExtend(record[i], extend[i]);
        if (forkUploadFunc_) {
            forkUploadFunc_(tableName, extend[i]);
        }
        if (isDelete) {
            extend[i][g_deleteField] = true;
        } else {
            extend[i][g_deleteField] = false;
        }
        CloudData cloudData = {
            .record = std::move(record[i]),
            .extend = extend[i]
        };
        extend[i][CloudDbConstant::VERSION_FIELD] = std::to_string(currentVersion_++);
        DBStatus ret = UpdateCloudData(tableName, std::move(cloudData));
        if (ret == CLOUD_VERSION_CONFLICT) {
            extend[i][CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_EXIST_CONFLICT);
            res = CLOUD_VERSION_CONFLICT;
        } else if (ret != OK) {
            return ret;
        }
    }
    return res;
}

DBStatus VirtualCloudDb::UpdateCloudData(const std::string &tableName, VirtualCloudDb::CloudData &&cloudData)
{
    if (cloudData_.find(tableName) == cloudData_.end()) {
        LOGE("[VirtualCloudDb] update cloud data failed, not found tableName %s", tableName.c_str());
        return DB_ERROR;
    }
    std::string paramGid = std::get<std::string>(cloudData.extend[g_gidField]);
    bool paramDelete = std::get<bool>(cloudData.extend[g_deleteField]);
    for (auto &data: cloudData_[tableName]) {
        std::string srcGid = std::get<std::string>(data.extend[g_gidField]);
        if (srcGid != paramGid) {
            continue;
        }
        if (paramDelete) {
            if (data.extend.find(g_deleteField) != data.extend.end() &&
                std::get<bool>(data.extend[g_deleteField])) {
                LOGE("[VirtualCloudDb] current data has been delete gid %s", paramGid.c_str());
                return DB_ERROR;
            }
            LOGD("[VirtualCloudDb] delete data, gid %s", paramGid.c_str());
        }
        if (cloudData.extend.find(CloudDbConstant::VERSION_FIELD) != cloudData.extend.end()) {
            if (std::get<std::string>(data.extend[CloudDbConstant::VERSION_FIELD]) !=
                std::get<std::string>(cloudData.extend[CloudDbConstant::VERSION_FIELD])) {
                return CLOUD_VERSION_CONFLICT;
            }
        }
        cloudData.extend[CloudDbConstant::VERSION_FIELD] = std::to_string(currentVersion_ - 1);
        data = std::move(cloudData);
        return OK;
    }
    LOGE("[VirtualCloudDb] update cloud data failed, not found gid %s", paramGid.c_str());
    return DB_ERROR;
}

void VirtualCloudDb::SetCloudError(bool cloudError)
{
    cloudError_ = cloudError;
}

void VirtualCloudDb::SetBlockTime(int32_t blockTime)
{
    blockTimeMs_ = blockTime;
}

void VirtualCloudDb::ClearHeartbeatCount()
{
    heartbeatCount_ = 0;
}

int32_t VirtualCloudDb::GetHeartbeatCount()
{
    return heartbeatCount_;
}

bool VirtualCloudDb::GetLockStatus()
{
    return lockStatus_;
}

void VirtualCloudDb::SetHeartbeatError(bool heartbeatError)
{
    heartbeatError_ = heartbeatError;
}

void VirtualCloudDb::SetIncrementData(const std::string &tableName, const VBucket &record, const VBucket &extend)
{
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    isSetCrementCloudData_ = true;
    auto iter = incrementCloudData_.find(tableName);
    if (iter == incrementCloudData_.end()) {
        return;
    }
    CloudData data = {record, extend};
    iter->second.push_back(data);
}

uint32_t VirtualCloudDb::GetQueryTimes(const std::string &tableName)
{
    if (queryTimes_.find(tableName) == queryTimes_.end()) {
        return 0;
    }
    return queryTimes_[tableName];
}

void VirtualCloudDb::SetActionStatus(DBStatus status)
{
    actionStatus_ = status;
}

DBStatus VirtualCloudDb::GetDataStatus(const std::string &gid, bool &deleteStatus)
{
    for (const auto &[tableName, tableDataList]: cloudData_) {
        for (auto &tableData : tableDataList) {
            if (std::get<std::string>(tableData.extend.at(g_gidField)) == gid) {
                deleteStatus = std::get<bool>(tableData.extend.at(g_deleteField));
                LOGI("tableName %s gid %s deleteStatus is %d", tableName.c_str(), gid.c_str(), deleteStatus);
                return OK;
            }
        }
    }
    LOGE("not found gid %s ", gid.c_str());
    return NOT_FOUND;
}

void VirtualCloudDb::ClearAllData()
{
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    cloudData_.clear();
    incrementCloudData_.clear();
    queryTimes_.clear();
}

void VirtualCloudDb::ForkQuery(const std::function<void(const std::string &, VBucket &)> &forkFunc)
{
    forkFunc_ = forkFunc;
}

void VirtualCloudDb::ForkUpload(const std::function<void(const std::string &, VBucket &)> &forkUploadFunc)
{
    forkUploadFunc_ = forkUploadFunc;
}

int32_t VirtualCloudDb::GetLockCount() const
{
    return lockCount_;
}

void VirtualCloudDb::Reset()
{
    lockCount_ = 0;
}

void VirtualCloudDb::SetInsertFailed(int32_t count)
{
    insertFailedCount_ = count;
}

void VirtualCloudDb::SetClearExtend(int32_t count)
{
    missingExtendCount_ = count;
}

void VirtualCloudDb::SetCloudNetworkError(bool cloudNetworkError)
{
    cloudNetworkError_ = cloudNetworkError;
}

void VirtualCloudDb::SetConflictInUpload(bool conflict)
{
    conflictInUpload_ = conflict;
}

void VirtualCloudDb::AddAssetIdForExtend(VBucket &record, VBucket &extend)
{
    for (auto &recordData : record) {
        if (recordData.second.index() == TYPE_INDEX<Asset>) {
            auto &asset = std::get<Asset>(recordData.second);
            if (asset.flag == static_cast<uint32_t>(DistributedDB::AssetOpType::INSERT)) {
                asset.assetId = "10";
            }
            extend[recordData.first] = asset;
        }
        if (recordData.second.index() == TYPE_INDEX<Assets>) {
            auto &assets = std::get<Assets>(recordData.second);
            AddAssetsIdInner(assets);
            extend[recordData.first] = assets;
        }
    }
}

void VirtualCloudDb::AddAssetsIdInner(Assets &assets)
{
    for (auto &asset : assets) {
        if (asset.flag == static_cast<uint32_t>(DistributedDB::AssetOpType::INSERT)) {
            asset.assetId = "10";
        }
    }
}

void VirtualCloudDb::SetHeartbeatBlockTime(int32_t blockTime)
{
    heartbeatBlockTimeMs_ = blockTime;
}

void VirtualCloudDb::ForkInsertConflict(const std::function<DBStatus(const std::string &, VBucket &, VBucket &,
    std::vector<CloudData> &)> &func)
{
    forkUploadConflictFunc_ = func;
}
}