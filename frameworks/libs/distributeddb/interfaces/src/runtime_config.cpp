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
#include "runtime_config.h"

#include "db_common.h"
#include "db_constant.h"
#include "db_dfx_adapter.h"
#include "db_properties.h"
#include "kvdb_manager.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "network_adapter.h"
#include "param_check_utils.h"
#include "runtime_context.h"

namespace DistributedDB {
std::mutex RuntimeConfig::communicatorMutex_;
std::mutex RuntimeConfig::multiUserMutex_;
std::shared_ptr<IProcessCommunicator> RuntimeConfig::processCommunicator_ = nullptr;

// Used to set the process userid and appId
DBStatus RuntimeConfig::SetProcessLabel(const std::string &appId, const std::string &userId)
{
    if (appId.size() > DBConstant::MAX_APP_ID_LENGTH || appId.empty() ||
        userId.size() > DBConstant::MAX_USER_ID_LENGTH || userId.empty()) {
        LOGE("Invalid app or user info[%zu]-[%zu]", appId.length(), userId.length());
        return INVALID_ARGS;
    }

    int errCode = KvDBManager::SetProcessLabel(appId, userId);
    if (errCode != E_OK) {
        LOGE("Failed to set the process label:%d", errCode);
        return DB_ERROR;
    }
    return OK;
}

// Set process communicator.
DBStatus RuntimeConfig::SetProcessCommunicator(const std::shared_ptr<IProcessCommunicator> &inCommunicator)
{
    std::lock_guard<std::mutex> lock(communicatorMutex_);
    if (processCommunicator_ != nullptr) {
        LOGE("processCommunicator_ is not null!");
        return DB_ERROR;
    }

    std::string processLabel = RuntimeContext::GetInstance()->GetProcessLabel();
    if (processLabel.empty()) {
        LOGE("ProcessLabel is not set!");
        return DB_ERROR;
    }

    auto *adapter = new (std::nothrow) NetworkAdapter(processLabel, inCommunicator);
    if (adapter == nullptr) {
        LOGE("New NetworkAdapter failed!");
        return DB_ERROR;
    }
    processCommunicator_ = inCommunicator;
    if (RuntimeContext::GetInstance()->SetCommunicatorAdapter(adapter) != E_OK) {
        LOGE("SetProcessCommunicator not support!");
        delete adapter;
        return DB_ERROR;
    }
    KvDBManager::RestoreSyncableKvStore();
    return OK;
}

DBStatus RuntimeConfig::SetPermissionCheckCallback(const PermissionCheckCallbackV2 &callback)
{
    return TransferDBErrno(RuntimeContext::GetInstance()->SetPermissionCheckCallback(callback));
}

DBStatus RuntimeConfig::SetPermissionCheckCallback(const PermissionCheckCallbackV3 &callback)
{
    return TransferDBErrno(RuntimeContext::GetInstance()->SetPermissionCheckCallback(callback));
}

DBStatus RuntimeConfig::SetProcessSystemAPIAdapter(const std::shared_ptr<IProcessSystemApiAdapter> &adapter)
{
    return TransferDBErrno(RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter));
}

void RuntimeConfig::Dump(int fd, const std::vector<std::u16string> &args)
{
    DBDfxAdapter::Dump(fd, args);
}

DBStatus RuntimeConfig::SetSyncActivationCheckCallback(const SyncActivationCheckCallback &callback)
{
    std::lock_guard<std::mutex> lock(multiUserMutex_);
    return TransferDBErrno(RuntimeContext::GetInstance()->SetSyncActivationCheckCallback(callback));
}

DBStatus RuntimeConfig::NotifyUserChanged()
{
    std::lock_guard<std::mutex> lock(multiUserMutex_);
    return TransferDBErrno(RuntimeContext::GetInstance()->NotifyUserChanged());
}

bool RuntimeConfig::IsProcessSystemApiAdapterValid()
{
    return RuntimeContext::GetInstance()->IsProcessSystemApiAdapterValid();
}

DBStatus RuntimeConfig::SetSyncActivationCheckCallback(const SyncActivationCheckCallbackV2 &callback)
{
    std::lock_guard<std::mutex> lock(multiUserMutex_);
    return TransferDBErrno(RuntimeContext::GetInstance()->SetSyncActivationCheckCallback(callback));
}

DBStatus RuntimeConfig::SetPermissionConditionCallback(const PermissionConditionCallback &callback)
{
    return TransferDBErrno(RuntimeContext::GetInstance()->SetPermissionConditionCallback(callback));
}

void RuntimeConfig::SetDBInfoHandle(const std::shared_ptr<DBInfoHandle> &handle)
{
    RuntimeContext::GetInstance()->SetDBInfoHandle(handle);
}

void RuntimeConfig::NotifyDBInfos(const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos)
{
    RuntimeContext::GetInstance()->NotifyDBInfos(devInfos, dbInfos);
}

void RuntimeConfig::SetTranslateToDeviceIdCallback(const DistributedDB::TranslateToDeviceIdCallback &callback)
{
    RuntimeContext::GetInstance()->SetTranslateToDeviceIdCallback(callback);
}

void RuntimeConfig::SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback, DBType type)
{
    DBTypeInner innerType = DBTypeInner::DB_INVALID;
    if (type == DBType::DB_KV) {
        innerType = DBTypeInner::DB_KV;
    } else if (type == DBType::DB_RELATION) {
        innerType = DBTypeInner::DB_RELATION;
    }
    RuntimeContext::GetInstance()->SetAutoLaunchRequestCallback(callback, innerType);
}

std::string RuntimeConfig::GetStoreIdentifier(const std::string &userId, const std::string &appId,
    const std::string &storeId, bool syncDualTupleMode)
{
    if (!ParamCheckUtils::CheckStoreParameter(storeId, appId, userId, syncDualTupleMode)) {
        return "";
    }
    if (syncDualTupleMode) {
        return DBCommon::TransferHashString(appId + "-" + storeId);
    }
    return DBCommon::TransferHashString(userId + "-" + appId + "-" + storeId);
}

void RuntimeConfig::ReleaseAutoLaunch(const std::string &userId, const std::string &appId, const std::string &storeId,
    DBType type)
{
    DBProperties properties;
    properties.SetIdentifier(userId, appId, storeId);

    DBTypeInner innerType = (type == DBType::DB_KV ? DBTypeInner::DB_KV : DBTypeInner::DB_RELATION);
    RuntimeContext::GetInstance()->CloseAutoLaunchConnection(innerType, properties);
}

void RuntimeConfig::SetThreadPool(const std::shared_ptr<IThreadPool> &threadPool)
{
    RuntimeContext::GetInstance()->SetThreadPool(threadPool);
}

void RuntimeConfig::SetCloudTranslate(const std::shared_ptr<ICloudDataTranslate> &dataTranslate)
{
    RuntimeContext::GetInstance()->SetCloudTranslate(dataTranslate);
}
} // namespace DistributedDB
#endif