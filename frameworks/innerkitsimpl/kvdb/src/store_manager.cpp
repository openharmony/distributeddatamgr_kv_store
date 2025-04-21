/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "StoreManager"
#include "store_manager.h"

#include "backup_manager.h"
#include "dev_manager.h"
#include "kvdb_service_client.h"
#include "log_print.h"
#include "security_manager.h"

#include "store_util.h"
namespace OHOS::DistributedKv {
StoreManager &StoreManager::GetInstance()
{
    static StoreManager instance;
    return instance;
}

std::shared_ptr<SingleKvStore> StoreManager::GetKVStore(const AppId &appId, const StoreId &storeId,
    const Options &options, Status &status)
{
    std::string path = options.GetDatabaseDir();
    ZLOGD("appId:%{public}s, storeId:%{public}s type:%{public}d area:%{public}d dir:%{public}s", appId.appId.c_str(),
        StoreUtil::Anonymous(storeId.storeId).c_str(), options.kvStoreType, options.area, path.c_str());
    status = ILLEGAL_STATE;
    if (!appId.IsValid() || !storeId.IsValid() || !options.IsValidType()) {
        ZLOGE("Params invalid type");
        status = INVALID_ARGUMENT;
        return nullptr;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service != nullptr) {
        status = service->BeforeCreate(appId, storeId, options);
    }
    if (status == STORE_META_CHANGED) {
        ReportInfo reportInfo = { .options = options, .errorCode = status, .systemErrorNo = errno,
            .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
        KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
        ZLOGE("appId:%{public}s, storeId:%{public}s type:%{public}d encrypt:%{public}d", appId.appId.c_str(),
            StoreUtil::Anonymous(storeId.storeId).c_str(), options.kvStoreType, options.encrypt);
        return nullptr;
    }
    StoreParams storeParams;
    auto kvStore = StoreFactory::GetInstance().GetOrOpenStore(appId, storeId, options, status, storeParams);
    if ((status == DATA_CORRUPTED || status == CRYPT_ERROR) && options.encrypt) {
        kvStore = OpenWithSecretKeyFromService(appId, storeId, options, status, storeParams);
    }
    if (status != SUCCESS) {
        ReportInfo reportInfo = { .options = options, .errorCode = status, .systemErrorNo = errno,
            .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
        KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    } else if (kvStore != nullptr && kvStore->IsRebuild()) {
        ReportInfo reportInfo = { .options = options, .errorCode = status, .systemErrorNo = errno,
            .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
        ZLOGI("Rebuild store success, storeId:%{public}s", StoreUtil::Anonymous(storeId.storeId).c_str());
        KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
    }
    if (storeParams.isCreate && options.persistent) {
        std::vector<uint8_t> pwd(storeParams.password.GetData(), storeParams.password.GetData() +
            storeParams.password.GetSize());
        if (service != nullptr) {
            // delay notify
            service->AfterCreate(appId, storeId, options, pwd);
        }
        pwd.assign(pwd.size(), 0);
        storeParams.password.Clear();
    }
    return kvStore;
}

std::shared_ptr<SingleKvStore> StoreManager::OpenWithSecretKeyFromService(const AppId &appId, const StoreId &storeId,
    const Options &options, Status &status, StoreParams &storeParams)
{
    std::shared_ptr<SingleKvStore> kvStore;
    std::vector<std::vector<uint8_t>> keys;
    if (BackupManager::GetInstance().GetSecretKeyFromService(appId, storeId, keys, options.subUser) !=
        Status::SUCCESS) {
        for (auto &key : keys) {
            key.assign(key.size(), 0);
        }
        return kvStore;
    }
    std::string path = options.GetDatabaseDir();
    for (auto &key : keys) {
        SecurityManager::DBPassword dbPassword;
        dbPassword.SetValue(key.data(), key.size());
        SecurityManager::GetInstance().SaveDBPassword(storeId.storeId, path, dbPassword.password);
        kvStore = StoreFactory::GetInstance().GetOrOpenStore(appId, storeId, options, status, storeParams);
        if (status == SUCCESS) {
            break;
        }
    }
    for (auto &key : keys) {
        key.assign(key.size(), 0);
    }
    return kvStore;
}

Status StoreManager::CloseKVStore(const AppId &appId, const StoreId &storeId, int32_t subUser)
{
    ZLOGD("appId:%{public}s, storeId:%{public}s", appId.appId.c_str(), StoreUtil::Anonymous(storeId.storeId).c_str());
    if (!appId.IsValid() || !storeId.IsValid()) {
        return INVALID_ARGUMENT;
    }

    return StoreFactory::GetInstance().Close(appId, storeId, subUser);
}

Status StoreManager::CloseAllKVStore(const AppId &appId, int32_t subUser)
{
    ZLOGD("appId:%{public}s", appId.appId.c_str());
    if (!appId.IsValid()) {
        return INVALID_ARGUMENT;
    }

    return StoreFactory::GetInstance().Close(appId, { "" }, subUser, true);
}

Status StoreManager::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds, int32_t subUser)
{
    ZLOGD("appId:%{public}s", appId.appId.c_str());
    if (!appId.IsValid()) {
        return INVALID_ARGUMENT;
    }

    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->GetStoreIds(appId, subUser, storeIds);
}

Status StoreManager::Delete(const AppId &appId, const StoreId &storeId, const std::string &path, int32_t subUser)
{
    ZLOGD("appId:%{public}s, storeId:%{public}s dir:%{public}s", appId.appId.c_str(),
        StoreUtil::Anonymous(storeId.storeId).c_str(), path.c_str());
    if (!appId.IsValid() || !storeId.IsValid()) {
        return INVALID_ARGUMENT;
    }

    auto status = StoreFactory::GetInstance().Delete(appId, storeId, path, subUser);
    ReportInfo reportInfo = { .options = { .baseDir = path }, .errorCode = status, .systemErrorNo = errno,
        .appId = appId.appId, .storeId = storeId.storeId, .functionName = std::string(__FUNCTION__) };
    if (status != SUCCESS) {
        KVDBFaultHiViewReporter::ReportKVFaultEvent(reportInfo);
    } else {
        KVDBFaultHiViewReporter::ReportKVRebuildEvent(reportInfo);
        auto service = KVDBServiceClient::GetInstance();
        if (service != nullptr) {
            service->Delete(appId, storeId, subUser);
        }
    }
    return status;
}

Status StoreManager::PutSwitch(const AppId &appId, const SwitchData &data)
{
    ZLOGD("appId:%{public}s, value len:%{public}u", appId.appId.c_str(), data.length);
    if (!appId.IsValid()) {
        return INVALID_ARGUMENT;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->PutSwitch(appId, data);
}

std::pair<Status, SwitchData> StoreManager::GetSwitch(const AppId &appId, const std::string &networkId)
{
    ZLOGD("appId:%{public}s, networkId:%{public}s", appId.appId.c_str(), StoreUtil::Anonymous(networkId).c_str());
    if (!appId.IsValid() || networkId.empty()) {
        return { INVALID_ARGUMENT, SwitchData() };
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return { SERVER_UNAVAILABLE, SwitchData() };
    }
    SwitchData data;
    auto status = service->GetSwitch(appId, networkId, data);
    return { status, std::move(data) };
}

Status StoreManager::SubscribeSwitchData(const AppId &appId, std::shared_ptr<KvStoreObserver> observer)
{
    ZLOGD("appId:%{public}s", appId.appId.c_str());
    if (!appId.IsValid() || observer == nullptr) {
        return INVALID_ARGUMENT;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    auto serviceAgent = service->GetServiceAgent(appId);
    if (serviceAgent == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    auto status = service->SubscribeSwitchData(appId);
    if (status != SUCCESS) {
        return status;
    }
    serviceAgent->AddSwitchCallback(appId.appId, observer);
    return SUCCESS;
}

Status StoreManager::UnsubscribeSwitchData(const AppId &appId, std::shared_ptr<KvStoreObserver> observer)
{
    ZLOGD("appId:%{public}s", appId.appId.c_str());
    if (!appId.IsValid() || observer == nullptr) {
        return INVALID_ARGUMENT;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    auto serviceAgent = service->GetServiceAgent(appId);
    if (serviceAgent == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    auto status = service->UnsubscribeSwitchData(appId);
    if (status != SUCCESS) {
        return status;
    }
    serviceAgent->DeleteSwitchCallback(appId.appId, observer);
    return SUCCESS;
}
} // namespace OHOS::DistributedKv