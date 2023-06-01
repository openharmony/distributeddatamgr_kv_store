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
#include "cloud_db_types.h"
#include "db_constant.h"
#include "log_print.h"
namespace DistributedDB {
namespace {
    const char *g_deleteField = "#_deleted";
    const char *g_gidField = "#_gid";
    const char *g_cursorField = "#_cursor";
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
    for (size_t i = 0; i < record.size(); ++i) {
        if (extend[i].find(g_gidField) != extend[i].end()) {
            LOGE("[VirtualCloudDb] Insert data should not have gid");
            return DB_ERROR;
        }
        extend[i][g_gidField] = std::to_string(currentGid_++);
        extend[i][g_cursorField] = std::to_string(currentCursor_++);
        extend[i][g_deleteField] = false;
        CloudData cloudData = {
            .record = std::move(record[i]),
            .extend = extend[i]
        };
        cloudData_[tableName].push_back(cloudData);
        auto gid = std::get<std::string>(extend[i][g_gidField]);
    }
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
    if (cloudError_) {
        return DB_ERROR;
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

DBStatus VirtualCloudDb::Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data)
{
    if (cloudError_) {
        return DB_ERROR;
    }
    if (blockTimeMs_ != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(blockTimeMs_));
    }
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    if (cloudData_.find(tableName) == cloudData_.end()) {
        return QUERY_END;
    }
    LOGD("extend size: %zu type: %zu  expect: %zu", extend.size(), extend[g_cursorField].index(),
        TYPE_INDEX<std::string>);
    std::string cursor = std::get<std::string>(extend[g_cursorField]);
    cursor = cursor.empty() ? "0" : cursor;
    for (auto &tableData : cloudData_[tableName]) {
        std::string srcCursor = std::get<std::string>(tableData.extend[g_cursorField]);
        if (std::stol(srcCursor) > std::stol(cursor)) {
            VBucket bucket = tableData.record;
            for (const auto &ex: tableData.extend) {
                bucket.insert(ex);
            }
            data.push_back(std::move(bucket));
        }
        if (data.size() >= static_cast<size_t>(queryLimit_)) {
            return OK;
        }
    }
    return data.empty() ? QUERY_END : OK;
}

DBStatus VirtualCloudDb::InnerUpdate(const std::string &tableName, std::vector<VBucket> &&record,
    std::vector<VBucket> &extend, bool isDelete)
{
    if (record.size() != extend.size()) {
        return DB_ERROR;
    }
    std::lock_guard<std::mutex> autoLock(cloudDataMutex_);
    for (size_t i = 0; i < record.size(); ++i) {
        if (extend[i].find(g_gidField) == extend[i].end()) {
            LOGE("[VirtualCloudDb] Update data should have gid");
            return DB_ERROR;
        }
        extend[i][g_cursorField] = std::to_string(currentCursor_++);
        if (isDelete) {
            extend[i][g_deleteField] = true;
        } else {
            extend[i][g_deleteField] = false;
        }
        CloudData cloudData = {
            .record = std::move(record[i]),
            .extend = extend[i]
        };
        if (UpdateCloudData(tableName, std::move(cloudData)) != OK) {
            return DB_ERROR;
        }
    }
    return OK;
}

DBStatus VirtualCloudDb::UpdateCloudData(const std::string &tableName, VirtualCloudDb::CloudData &&cloudData)
{
    if (cloudData_.find(tableName) == cloudData_.end()) {
        LOGE("[VirtualCloudDb] update cloud data failed, not found tableName");
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
}