/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "relational_store_delegate_impl.h"

#include "db_common.h"
#include "db_errno.h"
#include "cloud/cloud_db_constant.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "param_check_utils.h"
#include "relational_store_changed_data_impl.h"
#include "relational_store_instance.h"
#include "sync_operation.h"

namespace DistributedDB {
RelationalStoreDelegateImpl::RelationalStoreDelegateImpl(RelationalStoreConnection *conn, const std::string &path)
    : conn_(conn),
      storePath_(path)
{}

RelationalStoreDelegateImpl::~RelationalStoreDelegateImpl()
{
    if (!releaseFlag_) {
        LOGF("[RelationalStore Delegate] Can't release directly");
        return;
    }

    conn_ = nullptr;
}

DBStatus RelationalStoreDelegateImpl::RemoveDeviceDataInner(const std::string &device, ClearMode mode)
{
    if (mode >= BUTT || mode < 0) {
        LOGE("Invalid mode for Remove device data, %d.", INVALID_ARGS);
        return INVALID_ARGS;
    }
    if (mode == DEFAULT) {
        return RemoveDeviceData(device, "");
    }
    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }

    int errCode = conn_->DoClean(mode);
    if (errCode != E_OK) {
        LOGE("[RelationalStore Delegate] remove device cloud data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

int32_t RelationalStoreDelegateImpl::GetCloudSyncTaskCount()
{
    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return -1;
    }
    int32_t count = conn_->GetCloudSyncTaskCount();
    if (count == -1) {
        LOGE("[RelationalStore Delegate] Failed to get cloud sync task count.");
    }
    return count;
}

DBStatus RelationalStoreDelegateImpl::CreateDistributedTableInner(const std::string &tableName, TableSyncType type)
{
    if (!ParamCheckUtils::CheckRelationalTableName(tableName)) {
        LOGE("[RelationalStore Delegate] Invalid table name.");
        return INVALID_ARGS;
    }

    if (!(type == DEVICE_COOPERATION || type == CLOUD_COOPERATION)) {
        LOGE("[RelationalStore Delegate] Invalid table sync type.");
        return INVALID_ARGS;
    }

    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }

    int errCode = conn_->CreateDistributedTable(tableName, type);
    if (errCode != E_OK) {
        LOGE("[RelationalStore Delegate] Create Distributed table failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const Query &query, const SyncStatusCallback &onComplete, bool wait)
{
    if (conn_ == nullptr) {
        LOGE("Invalid connection for operation!");
        return DB_ERROR;
    }

    if (mode > SYNC_MODE_PUSH_PULL) {
        LOGE("not support other mode");
        return NOT_SUPPORT;
    }

    if (!DBCommon::CheckQueryWithoutMultiTable(query)) {
        LOGE("not support query with tables");
        return NOT_SUPPORT;
    }
    RelationalStoreConnection::SyncInfo syncInfo{devices, mode,
        std::bind(&RelationalStoreDelegateImpl::OnSyncComplete, std::placeholders::_1, onComplete), query, wait};
    int errCode = conn_->SyncToDevice(syncInfo);
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] sync data to device failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::RemoveDeviceData(const std::string &device, const std::string &tableName)
{
    if (conn_ == nullptr) {
        LOGE("Invalid connection for operation!");
        return DB_ERROR;
    }

    if (device.empty() || device.length() > DBConstant::MAX_DEV_LENGTH ||
        !ParamCheckUtils::CheckRelationalTableName(tableName)) {
        LOGE("[RelationalStore Delegate] Remove device data with invalid device name or table name.");
        return INVALID_ARGS;
    }

    int errCode = conn_->RemoveDeviceData(device, tableName);
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] remove device data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::Close()
{
    if (conn_ == nullptr) {
        return OK;
    }

    int errCode = RelationalStoreInstance::ReleaseDataBaseConnection(conn_);
    if (errCode == -E_BUSY) {
        LOGW("[RelationalStore Delegate] busy for close");
        return BUSY;
    }
    if (errCode != E_OK) {
        LOGE("Release db connection error:%d", errCode);
        return TransferDBErrno(errCode);
    }

    LOGI("[RelationalStore Delegate] Close");
    conn_ = nullptr;
    return OK;
}

void RelationalStoreDelegateImpl::SetReleaseFlag(bool flag)
{
    releaseFlag_ = flag;
}

void RelationalStoreDelegateImpl::OnSyncComplete(const std::map<std::string, std::vector<TableStatus>> &devicesStatus,
    const SyncStatusCallback &onComplete)
{
    std::map<std::string, std::vector<TableStatus>> res;
    for (const auto &[device, tablesStatus] : devicesStatus) {
        for (const auto &tableStatus : tablesStatus) {
            TableStatus table;
            table.tableName = tableStatus.tableName;
            table.status = SyncOperation::DBStatusTrans(tableStatus.status);
            res[device].push_back(table);
        }
    }
    if (onComplete) {
        onComplete(res);
    }
}

DBStatus RelationalStoreDelegateImpl::RemoteQuery(const std::string &device, const RemoteCondition &condition,
    uint64_t timeout, std::shared_ptr<ResultSet> &result)
{
    if (conn_ == nullptr) {
        LOGE("Invalid connection for operation!");
        return DB_ERROR;
    }
    int errCode = conn_->RemoteQuery(device, condition, timeout, result);
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] remote query failed:%d", errCode);
        result = nullptr;
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::RemoveDeviceData()
{
    if (conn_ == nullptr) {
        LOGE("Invalid connection for operation!");
        return DB_ERROR;
    }

    int errCode = conn_->RemoveDeviceData();
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] remove device data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::Sync(const std::vector<std::string> &devices, SyncMode mode, const Query &query,
    const SyncProcessCallback &onProcess, int64_t waitTime)
{
    if (conn_ == nullptr) {
        return DB_ERROR;
    }
    CloudSyncOption option;
    option.devices = devices;
    option.mode = mode;
    option.query = query;
    option.waitTime = waitTime;
    int errCode = conn_->Sync(option, onProcess);
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] cloud sync failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb)
{
    if (conn_ == nullptr || conn_->SetCloudDB(cloudDb) != E_OK) {
        return DB_ERROR;
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::SetCloudDbSchema(const DataBaseSchema &schema)
{
    DataBaseSchema cloudSchema = schema;
    if (!ParamCheckUtils::CheckSharedTableName(cloudSchema)) {
        LOGE("[RelationalStore Delegate] SharedTableName check failed!");
        return INVALID_ARGS;
    }
    if (conn_ == nullptr) {
        return DB_ERROR;
    }
    // create shared table and set cloud db schema
    int errorCode = conn_->PrepareAndSetCloudDbSchema(cloudSchema);
    if (errorCode != E_OK) {
        LOGE("[RelationalStore Delegate] set cloud schema failed!");
    }
    return TransferDBErrno(errorCode);
}

DBStatus RelationalStoreDelegateImpl::RegisterObserver(StoreObserver *observer)
{
    if (observer == nullptr) {
        return INVALID_ARGS;
    }
    if (conn_ == nullptr) {
        return DB_ERROR;
    }
    std::string userId;
    std::string appId;
    std::string storeId;
    int errCode = conn_->GetStoreInfo(userId, appId, storeId);
    if (errCode != E_OK) {
        return DB_ERROR;
    }
    errCode = conn_->RegisterObserverAction(observer, [observer, userId, appId, storeId](
        const std::string &changedDevice, ChangedData &&changedData, bool isChangedData) {
        if (isChangedData && observer != nullptr) {
            observer->OnChange(Origin::ORIGIN_CLOUD, changedDevice, std::move(changedData));
            LOGD("begin to observer on changed data");
            return;
        }
        RelationalStoreChangedDataImpl data(changedDevice);
        data.SetStoreProperty({userId, appId, storeId});
        if (observer != nullptr) {
            LOGD("begin to observer on changed, changedDevice=%s", STR_MASK(changedDevice));
            observer->OnChange(data);
        }
    });
    return TransferDBErrno(errCode);
}

DBStatus RelationalStoreDelegateImpl::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    if (conn_ == nullptr || conn_->SetIAssetLoader(loader) != E_OK) {
        return DB_ERROR;
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::UnRegisterObserver()
{
    if (conn_ == nullptr) {
        return DB_ERROR;
    }
    // unregister all observer of this delegate
    return TransferDBErrno(conn_->UnRegisterObserverAction(nullptr));
}

DBStatus RelationalStoreDelegateImpl::UnRegisterObserver(StoreObserver *observer)
{
    if (observer == nullptr) {
        return INVALID_ARGS;
    }
    if (conn_ == nullptr) {
        return DB_ERROR;
    }
    return TransferDBErrno(conn_->UnRegisterObserverAction(observer));
}

DBStatus RelationalStoreDelegateImpl::Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess)
{
    if (conn_ == nullptr) {
        return DB_ERROR;
    }
    int errCode = conn_->Sync(option, onProcess);
    if (errCode != E_OK) {
        LOGE("[RelationalStore Delegate] cloud sync failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::SetTrackerTable(const TrackerSchema &schema)
{
    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }
    if (schema.tableName.empty()) {
        LOGE("[RelationalStore Delegate] tracker table is empty.");
        return INVALID_ARGS;
    }
    if (!ParamCheckUtils::CheckRelationalTableName(schema.tableName)) {
        LOGE("[RelationalStore Delegate] Invalid tracker table name.");
        return INVALID_ARGS;
    }
    int errCode = conn_->SetTrackerTable(schema);
    if (errCode != E_OK) {
        if (errCode == -E_WITH_INVENTORY_DATA) {
            LOGI("[RelationalStore Delegate] create tracker table for the first time.");
        } else {
            LOGE("[RelationalStore Delegate] Set Subscribe table failed:%d", errCode);
        }
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records)
{
    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }
    int errCode = conn_->ExecuteSql(condition, records);
    if (errCode != E_OK) {
        LOGE("[RelationalStore Delegate] execute sql failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty)
{
    if (conn_ == nullptr) {
        LOGE("[RelationalStore SetReference] Invalid connection for operation!");
        return DB_ERROR;
    }
    if (!ParamCheckUtils::CheckTableReference(tableReferenceProperty)) {
        return INVALID_ARGS;
    }
    int errCode = conn_->SetReference(tableReferenceProperty);
    if (errCode != E_OK) {
        if (errCode != -E_TABLE_REFERENCE_CHANGED) {
            LOGE("[RelationalStore] SetReference failed:%d", errCode);
        } else {
            LOGI("[RelationalStore] reference changed");
        }
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::CleanTrackerData(const std::string &tableName, int64_t cursor)
{
    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }
    int errCode = conn_->CleanTrackerData(tableName, cursor);
    if (errCode != E_OK) {
        LOGE("[RelationalStore Delegate] clean tracker data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::Pragma(PragmaCmd cmd, PragmaData &pragmaData)
{
    if (cmd != PragmaCmd::LOGIC_DELETE_SYNC_DATA) {
        return NOT_SUPPORT;
    }
    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }
    int errCode = conn_->Pragma(cmd, pragmaData);
    if (errCode != E_OK) {
        LOGE("[RelationalStore Delegate] Pragma failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::UpsertData(const std::string &tableName, const std::vector<VBucket> &records,
    RecordStatus status)
{
    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }
    int errCode = conn_->UpsertData(status, tableName, records);
    if (errCode != E_OK) {
        LOGE("[RelationalStore Delegate] Upsert data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    LOGI("[RelationalStore Delegate] Upsert data success");
    return OK;
}
} // namespace DistributedDB
#endif