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
#include "relational_store_manager.h"

#include <thread>

#include "auto_launch.h"
#include "db_common.h"
#include "db_dfx_adapter.h"
#include "db_errno.h"
#include "cloud/cloud_db_constant.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "param_check_utils.h"
#include "platform_specific.h"
#include "query_sync_object.h"
#include "relational_store_changed_data_impl.h"
#include "relational_store_delegate_impl.h"
#include "relational_store_instance.h"
#include "runtime_config.h"
#include "runtime_context.h"

namespace DistributedDB {
namespace {
const int GET_CONNECT_RETRY = 3;
const int RETRY_GET_CONN_INTER = 30;

DBStatus InitProperties(const RelationalStoreDelegate::Option &option, RelationalDBProperties &properties)
{
    properties.SetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE, static_cast<int>(option.tableMode));
    properties.SetBoolProp(RelationalDBProperties::SYNC_DUAL_TUPLE_MODE, option.syncDualTupleMode);
    if (option.isEncryptedDb) {
        if (!ParamCheckUtils::CheckEncryptedParameter(option.cipher, option.passwd) || option.iterateTimes == 0) {
            return INVALID_ARGS;
        }
        properties.SetCipherArgs(option.cipher, option.passwd, option.iterateTimes);
    }
    properties.SetBoolProp(DBProperties::COMPRESS_ON_SYNC, option.isNeedCompressOnSync);
    if (option.isNeedCompressOnSync) {
        properties.SetIntProp(
            DBProperties::COMPRESSION_RATE, ParamCheckUtils::GetValidCompressionRate(option.compressionRate));
    }
    return OK;
}
}

RelationalStoreManager::RelationalStoreManager(const std::string &appId, const std::string &userId, int32_t instanceId)
    : appId_(appId),
      userId_(userId),
      instanceId_(instanceId)
{}

RelationalStoreManager::RelationalStoreManager(const std::string &appId, const std::string &userId,
    const std::string &subUser, int32_t instanceId)
    : appId_(appId),
      userId_(userId),
      subUser_(subUser),
      instanceId_(instanceId)
{}

static RelationalStoreConnection *GetOneConnectionWithRetry(const RelationalDBProperties &properties, int &errCode)
{
    for (int i = 0; i < GET_CONNECT_RETRY; i++) {
        auto conn = RelationalStoreInstance::GetDatabaseConnection(properties, errCode);
        if (conn != nullptr) {
            return conn;
        }
        if (errCode == -E_STALE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_GET_CONN_INTER));
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

bool RelationalStoreManager::PreCheckOpenStore(const std::string &path, const std::string &storeId,
    RelationalStoreDelegate *&delegate, std::string &canonicalDir)
{
    if (delegate != nullptr) {
        LOGE("[RelationalStoreMgr] Invalid delegate!");
        return false;
    }

    if (!ParamCheckUtils::CheckDataDir(path, canonicalDir)) {
        return false;
    }

    if (!ParamCheckUtils::CheckStoreParameter({userId_, appId_, storeId}, false, subUser_, true) || path.empty()) {
        return false;
    }

    return true;
}

DB_API DBStatus RelationalStoreManager::OpenStore(const std::string &path, const std::string &storeId,
    const RelationalStoreDelegate::Option &option, RelationalStoreDelegate *&delegate)
{
    std::string canonicalDir;
    if (!PreCheckOpenStore(path, storeId, delegate, canonicalDir)) {
        return INVALID_ARGS;
    }

    RelationalDBProperties properties;
    properties.SetStringProp(RelationalDBProperties::DATA_DIR, canonicalDir);
    properties.SetIdentifier(userId_, appId_, storeId, subUser_, instanceId_);
    auto ret = InitProperties(option, properties);
    if (ret != OK) {
        return ret;
    }
    int errCode = E_OK;
    auto *conn = GetOneConnectionWithRetry(properties, errCode);
    if (conn == nullptr) {
        return TransferDBErrno(errCode);
    }

    delegate = new (std::nothrow) RelationalStoreDelegateImpl(conn, path);
    if (delegate == nullptr) {
        conn->Close();
        return DB_ERROR;
    }

    if (option.observer == nullptr) {
        return OK;
    }
    DBStatus status = delegate->RegisterObserver(option.observer);
    if (status != OK) {
        LOGE("register observer failed when open store: %d", status);
        conn->Close();
    }
    return status;
}

DBStatus RelationalStoreManager::CloseStore(RelationalStoreDelegate *store)
{
    if (store == nullptr) {
        return INVALID_ARGS;
    }

    auto storeImpl = static_cast<RelationalStoreDelegateImpl *>(store);
    DBStatus status = storeImpl->Close();
    if (status == BUSY) {
        LOGD("NbDelegateImpl is busy now.");
        return BUSY;
    }
    storeImpl->SetReleaseFlag(true);
    delete store;
    store = nullptr;
    return OK;
}

std::string RelationalStoreManager::GetDistributedTableName(const std::string &device, const std::string &tableName)
{
    if ((!RuntimeContext::GetInstance()->ExistTranslateDevIdCallback() && device.empty()) || tableName.empty()) {
        return {};
    }
    return DBCommon::GetDistributedTableName(device, tableName);
}

void RelationalStoreManager::SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback)
{
#ifdef USE_DISTRIBUTEDDB_DEVICE
    RuntimeContext::GetInstance()->SetAutoLaunchRequestCallback(callback, DBTypeInner::DB_RELATION);
#endif
}

std::string RelationalStoreManager::GetRelationalStoreIdentifier(const std::string &userId, const std::string &appId,
    const std::string &storeId, bool syncDualTupleMode)
{
    return RelationalStoreManager::GetRelationalStoreIdentifier(userId, "", appId, storeId, syncDualTupleMode);
}

std::string RelationalStoreManager::GetRelationalStoreIdentifier(const std::string &userId,
    const std::string &subUserId, const std::string &appId, const std::string &storeId, bool syncDualTupleMode)
{
    StoreInfo info;
    info.storeId = storeId;
    info.appId = appId;
    info.userId = userId;
    return DBCommon::GetStoreIdentifier(info, subUserId, syncDualTupleMode, true);
}

std::vector<QueryNode> RelationalStoreManager::ParserQueryNodes(const Bytes &queryBytes,
    DBStatus &status)
{
    std::vector<QueryNode> res;
    status = TransferDBErrno(QuerySyncObject::ParserQueryNodes(queryBytes, res));
    return res;
}
} // namespace DistributedDB
#endif
