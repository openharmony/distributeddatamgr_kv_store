/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
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
#ifdef RELATIONAL_STORE
#include "sqlite_relational_store.h"

namespace DistributedDB {
#ifdef USE_DISTRIBUTEDDB_DEVICE
int SQLiteRelationalStore::RemoveExceptDeviceData(const std::map<std::string, std::vector<std::string>> &tableMap)
{
    auto mode = static_cast<DistributedTableMode>(sqliteStorageEngine_->GetRelationalProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, static_cast<int>(DistributedTableMode::COLLABORATION)));
    if (mode != DistributedTableMode::COLLABORATION) {
        LOGE("[SQLiteRelationalStore]RemoveExceptDeviceData only support collaboration table mode");
        return -E_NOT_SUPPORT;
    }
    int errCode = E_OK;
    for (const auto &tableInfo : tableMap) {
        const std::string &tableName = tableInfo.first;
        errCode = CheckTableSyncType(tableName, TableSyncType::DEVICE_COOPERATION);
        if (errCode != E_OK) {
            return errCode;
        }
        std::vector<std::string> keepDevices = tableInfo.second;
        std::vector<std::string> targetDevices;
        errCode = GetTargetDevices(keepDevices, targetDevices); // targetDevices is after hash
        if (errCode != E_OK) {
            return errCode;
        }
        if (targetDevices.empty()) {
            continue;
        }
        // erase watermark first
        for (const auto &hashDevice : targetDevices) {
            errCode = syncAbleEngine_->EraseDeviceWaterMark(hashDevice, false, tableName);
            if (errCode != E_OK) {
                LOGE("[SQLiteRelationalStore] erase watermark failed for table, %s, %d",
                    DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
                return errCode;
            }
        }
    }
    errCode = RemoveExceptDeviceDataInner(tableMap);
    if (errCode == E_OK) {
        storageEngine_->NotifySchemaChanged();
    }
    return errCode;
}

int SQLiteRelationalStore::RemoveExceptDeviceDataInner(const std::map<std::string, std::vector<std::string>> &tableMap)
{
    SQLiteSingleVerRelationalStorageExecutor *handle = nullptr;
    int errCode = GetHandleAndStartTransaction(handle);
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer releaseGuard([this, handle, &errCode] {
        SQLiteSingleVerRelationalStorageExecutor *releaseHandle = handle;
        if (errCode != E_OK) {
            (void)releaseHandle->Rollback();
        } else {
            errCode = releaseHandle->Commit();
        }
        ReleaseHandle(releaseHandle);
    });
    errCode = handle->SetLogTriggerStatus(false);
    if (errCode != E_OK) {
        return errCode;
    };
    bool hasTrackerTable = false;
    for (const auto &iter : tableMap) {
        TrackerTable trackerTable = sqliteStorageEngine_->GetTrackerSchema().GetTrackerTable(iter.first);
        if (!trackerTable.IsEmpty()) {
            hasTrackerTable = true;
            errCode = handle->CreateTempSyncTrigger(trackerTable, true);
            if (errCode != E_OK) {
                return errCode;
            }
        }
        errCode = handle->DeleteDistributedExceptDeviceTable(iter.first, iter.second);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = handle->DeleteDistributedExceptDeviceTableLog(iter.first, iter.second, trackerTable);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    if (hasTrackerTable) {
        errCode = handle->ClearAllTempSyncTrigger();
        if (errCode != E_OK) {
            return errCode;
        }
    }
    errCode = handle->SetLogTriggerStatus(true);
    return errCode;
}
#endif

int SQLiteRelationalStore::CheckTableSyncType(const std::string &tableName, TableSyncType tableSyncType) const
{
    bool isCreated = false;
    int errCode = sqliteStorageEngine_->CheckTableExists(tableName, isCreated);
    if (errCode != E_OK) {
        LOGE("[SQLiteRelationalStore] check table exist failed, %d", errCode);
        return errCode;
    }
    if (!isCreated) {
        LOGE("[SQLiteRelationalStore] table %s dose not exist in the database",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
        return -E_TABLE_NOT_FOUND;
    }
    TableInfoMap tableInfo = sqliteStorageEngine_->GetSchema().GetTables();
    if (tableInfo.empty()) {
        LOGE("[SQLiteRelationalStore] no distributed table found");
        return -E_NOT_SUPPORT;
    }
    auto iter = tableInfo.find(tableName);
    if (iter == tableInfo.end()) {
        LOGE("[SQLiteRelationalStore] table %s is not a distributed table",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
        return -E_NOT_SUPPORT;
    }
    if (iter->second.GetTableSyncType() != tableSyncType) {
        LOGE("[SQLiteRelationalStore] table with invalid sync type");
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

int SQLiteRelationalStore::GetTargetDevices(const std::vector<std::string> &keepDevices,
    std::vector<std::string> &targetDevices)
{
    std::set<std::string> hashDevices;
    int errCode = E_OK;
    errCode = GetExistDevices(hashDevices);
    if (errCode != E_OK) {
        return errCode;
    }

    std::unordered_set<std::string> keepHashDeviceSet;
    for (const auto &device : keepDevices) {
        if (device.empty()) {
            continue; // "" mean local device
        }
        std::string hashDeviceId;
        errCode = syncAbleEngine_->GetHashDeviceId(device, hashDeviceId);
        if (errCode != E_OK && errCode != -E_NOT_SUPPORT) {
            return errCode;
        }
        if (errCode == -E_NOT_SUPPORT) {
            errCode = E_OK;
            hashDeviceId = DBCommon::TransferHashString(device);
            LOGI("[SQLiteRelationalStore] device %s not support hashDevicId",
                DBCommon::StringMiddleMaskingWithLen(device).c_str());
            // check device is uuid in meta
            if (hashDevices.find(hashDeviceId) == hashDevices.end()) {
                LOGW("[SQLiteRelationalStore] not match keep device %s", 
                    DBCommon::StringMiddleMaskingWithLen(device).c_str());
            }
        }
        keepHashDeviceSet.insert(hashDeviceId);
    }

    for (const auto &device : hashDevices) {
        if (keepHashDeviceSet.find(device) == keepHashDeviceSet.end()) {
            targetDevices.push_back(device);
        }
    }
    return errCode;
}
} // namespace DistributedDB
#endif