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
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "relational_store_instance.h"
#include "db_common.h"
#include "db_dfx_adapter.h"
#include "param_check_utils.h"
#include "log_print.h"
#include "db_errno.h"
#include "kv_store_errno.h"
#include "relational_store_changed_data_impl.h"
#include "relational_store_delegate_impl.h"
#include "runtime_config.h"
#include "runtime_context.h"
#include "platform_specific.h"

namespace DistributedDB {
namespace {
const int GET_CONNECT_RETRY = 3;
const int RETRY_GET_CONN_INTER = 30;
}

RelationalStoreManager::RelationalStoreManager(const std::string &appId, const std::string &userId, int32_t instanceId)
    : appId_(appId),
      userId_(userId),
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

    if (!ParamCheckUtils::CheckStoreParameter(storeId, appId_, userId_) || path.empty()) {
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
    properties.SetIdentifier(userId_, appId_, storeId, instanceId_);
    properties.SetBoolProp(RelationalDBProperties::SYNC_DUAL_TUPLE_MODE, option.syncDualTupleMode);
    if (option.isEncryptedDb) {
        if (!ParamCheckUtils::CheckEncryptedParameter(option.cipher, option.passwd) || option.iterateTimes == 0) {
            return INVALID_ARGS;
        }
        properties.SetCipherArgs(option.cipher, option.passwd, option.iterateTimes);
    }

    int errCode = E_OK;
    auto *conn = GetOneConnectionWithRetry(properties, errCode);
    if (errCode == -E_INVALID_PASSWD_OR_CORRUPTED_DB) {
        DBDfxAdapter::ReportFault( { DBDfxAdapter::EVENT_OPEN_DATABASE_FAILED, userId_, appId_, storeId, errCode } );
    }
    if (conn == nullptr) {
        return TransferDBErrno(errCode);
    }

    delegate = new (std::nothrow) RelationalStoreDelegateImpl(conn, path);
    if (delegate == nullptr) {
        conn->Close();
        return DB_ERROR;
    }
    return option.observer != nullptr ? delegate->RegisterObserver(option.observer) : OK;
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

DB_API std::string RelationalStoreManager::GetDistributedLogTableName(const std::string &tableName)
{
    return DBCommon::GetLogTableName(tableName);
}

DB_API std::vector<uint8_t> RelationalStoreManager::CalcPrimaryKeyHash(const std::map<std::string, Type> primaryKey)
{
    std::vector<uint8_t> result;
    if (primaryKey.empty()) {
        LOGW("primaryKey is empty");
        return result;
    }
    int errCode = E_OK;
    if (primaryKey.size() == 1) {
        auto iter = primaryKey.begin();
        Field field = {iter->first, static_cast<int32_t>(iter->second.index()), true, false};
        errCode = CloudStorageUtils::CalculateHashKeyForOneField(field, primaryKey, false, result);
        if (errCode != E_OK) {
            // never happen
            LOGE("calc hash fail when there is one primary key errCode = %d", errCode);
            return result;
        }
    } else {
        std::vector<uint8_t> tempRes;
        for (const auto &item : primaryKey) {
            std::vector<uint8_t> temp;
            Field field = {item.first, static_cast<int32_t>(item.second.index()), true, false};
            errCode = CloudStorageUtils::CalculateHashKeyForOneField(field, primaryKey, false, temp);
            if (errCode != E_OK) {
                // never happen
                LOGE("calc hash fail when there is more than one primary key errCode = %d", errCode);
                return result;
            }
            tempRes.insert(tempRes.end(), temp.begin(), temp.end());
        }
        errCode = DBCommon::CalcValueHash(tempRes, result);
        if (errCode != E_OK) {
            LOGE("calc hash fail when calc the composite primary key errCode = %d", errCode);
            return result;
        }
    }
    return result;
}

void RelationalStoreManager::SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback)
{
    RuntimeContext::GetInstance()->SetAutoLaunchRequestCallback(callback, DBTypeInner::DB_RELATION);
}

std::string RelationalStoreManager::GetRelationalStoreIdentifier(const std::string &userId, const std::string &appId,
    const std::string &storeId, bool syncDualTupleMode)
{
    return RuntimeConfig::GetStoreIdentifier(userId, appId, storeId, syncDualTupleMode);
}
} // namespace DistributedDB
#endif