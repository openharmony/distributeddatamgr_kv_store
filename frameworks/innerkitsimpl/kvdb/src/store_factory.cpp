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
#define LOG_TAG "StoreFactory"
#include "store_factory.h"

#include "backup_manager.h"
#include "device_convertor.h"
#include "log_print.h"
#include "security_manager.h"
#include "single_store_impl.h"
#include "store_util.h"
#include "system_api.h"
namespace OHOS::DistributedKv {
using namespace DistributedDB;
StoreFactory &StoreFactory::GetInstance()
{
    static StoreFactory instance;
    return instance;
}

StoreFactory::StoreFactory()
{
    convertors_[DEVICE_COLLABORATION] = new DeviceConvertor();
    convertors_[SINGLE_VERSION] = new Convertor();
    convertors_[MULTI_VERSION] = new Convertor();
    if (DBManager::IsProcessSystemApiAdapterValid()) {
        return;
    }
    (void)DBManager::SetProcessSystemAPIAdapter(std::make_shared<SystemApi>());
}

std::shared_ptr<SingleKvStore> StoreFactory::GetOrOpenStore(const AppId &appId, const StoreId &storeId,
    const Options &options, Status &status, bool &isCreate)
{
    std::shared_ptr<SingleStoreImpl> kvStore;
    isCreate = false;
    stores_.Compute(appId, [&](auto &, auto &stores) {
        if (stores.find(storeId) != stores.end()) {
            kvStore = stores[storeId];
            kvStore->AddRef();
            status = SUCCESS;
            return !stores.empty();
        }

        auto dbManager = GetDBManager(options.baseDir, appId);
        auto dbPassword = SecurityManager::GetInstance().GetDBPassword(storeId.storeId,
            options.baseDir, options.encrypt);
        if (options.encrypt && !dbPassword.IsValid()) {
            status = CRYPT_ERROR;
            ZLOGE("Crypt kvStore failed to get password, storeId is %{public}s, error is %{public}d",
                storeId.storeId.c_str(), static_cast<int>(status));
            return !stores.empty();
        }
        if (options.encrypt) {
            status = RekeyRecover(storeId, options.baseDir, dbPassword, dbManager, options);
            if (status != SUCCESS) {
                ZLOGE("KvStore password error, storeId is %{public}s, error is %{public}d",
                    storeId.storeId.c_str(), static_cast<int>(status));
                return !stores.empty();
            }
            if (dbPassword.isKeyOutdated && !ReKey(storeId, options.baseDir, dbPassword, dbManager, options)) {
                return !stores.empty();
            }
        }
        DBStatus dbStatus = DBStatus::DB_ERROR;
        dbManager->GetKvStore(storeId, GetDBOption(options, dbPassword),
            [this, &dbManager, &kvStore, &appId, &dbStatus, &options](auto status, auto *store) {
                dbStatus = status;
                if (store == nullptr) {
                    return;
                }
                auto release = [dbManager](auto *store) { dbManager->CloseKvStore(store); };
                auto dbStore = std::shared_ptr<DBStore>(store, release);
                const Convertor &convertor = *(convertors_[options.kvStoreType]);
                kvStore = std::make_shared<SingleStoreImpl>(dbStore, appId, options, convertor);
            });
        status = StoreUtil::ConvertStatus(dbStatus);
        if (kvStore == nullptr) {
            ZLOGE("failed! status:%{public}d appId:%{public}s storeId:%{public}s path:%{public}s", dbStatus,
                appId.appId.c_str(), storeId.storeId.c_str(), options.baseDir.c_str());
            return !stores.empty();
        }
        isCreate = true;
        stores[storeId] = kvStore;
        return !stores.empty();
    });
    return kvStore;
}

Status StoreFactory::Delete(const AppId &appId, const StoreId &storeId, const std::string &path)
{
    Close(appId, storeId, true);
    auto dbManager = GetDBManager(path, appId);
    auto status = dbManager->DeleteKvStore(storeId);
    SecurityManager::GetInstance().DelDBPassword(storeId.storeId, path);
    return StoreUtil::ConvertStatus(status);
}

Status StoreFactory::Close(const AppId &appId, const StoreId &storeId, bool isForce)
{
    Status status = STORE_NOT_OPEN;
    stores_.ComputeIfPresent(appId, [&storeId, &status, isForce](auto &, auto &values) {
        for (auto it = values.begin(); it != values.end();) {
            if (!storeId.storeId.empty() && (it->first != storeId.storeId)) {
                ++it;
                continue;
            }

            status = SUCCESS;
            auto ref = it->second->Close(isForce);
            if (ref <= 0) {
                it = values.erase(it);
            } else {
                ++it;
            }
        }
        return !values.empty();
    });
    return status;
}

std::shared_ptr<StoreFactory::DBManager> StoreFactory::GetDBManager(const std::string &path, const AppId &appId)
{
    std::shared_ptr<DBManager> dbManager;
    dbManagers_.Compute(path, [&dbManager, &appId](const auto &path, std::shared_ptr<DBManager> &manager) {
        if (manager != nullptr) {
            dbManager = manager;
            return true;
        }
        std::string fullPath = path + "/kvdb";
        auto result = StoreUtil::InitPath(fullPath);
        dbManager = std::make_shared<DBManager>(appId.appId, "default");
        dbManager->SetKvStoreConfig({ fullPath });
        manager = dbManager;
        BackupManager::GetInstance().Init(path);
        return result;
    });
    return dbManager;
}

StoreFactory::DBOption StoreFactory::GetDBOption(const Options &options, const DBPassword &dbPassword) const
{
    DBOption dbOption;
    dbOption.syncDualTupleMode = true; // tuple of (appid+storeid)
    dbOption.createIfNecessary = options.createIfMissing;
    dbOption.isNeedRmCorruptedDb = options.rebuild;
    dbOption.isMemoryDb = (!options.persistent);
    dbOption.isEncryptedDb = options.encrypt;
    if (options.encrypt) {
        dbOption.cipher = DistributedDB::CipherType::AES_256_GCM;
        dbOption.passwd = dbPassword.password;
    }

    if (options.kvStoreType == KvStoreType::SINGLE_VERSION) {
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    } else if (options.kvStoreType == KvStoreType::DEVICE_COLLABORATION) {
        dbOption.conflictResolvePolicy = DistributedDB::DEVICE_COLLABORATION;
    }

    dbOption.schema = options.schema;
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = StoreUtil::GetDBSecurity(options.securityLevel);
    return dbOption;
}

bool StoreFactory::ReKey(const std::string &storeId, const std::string &path, DBPassword &dbPassword,
    std::shared_ptr<DBManager> dbManager, const Options &options)
{
    int32_t retry = 0;
    DBStatus status;
    DBStore *kvStore;
    bool isRekeySuccess = false;
    auto dbOption = GetDBOption(options, dbPassword);
    dbManager->GetKvStore(storeId, dbOption, [&status, &kvStore](auto dbStatus, auto *dbStore) {
        status = dbStatus;
        kvStore = dbStore;
    });
    while (retry < REKEY_TIMES) {
        auto status = RekeyRecover(storeId, path, dbPassword, dbManager, options);
        if (status != SUCCESS) {
            break;
        }
        auto succeed = ExecuteRekey(storeId, path, dbPassword, kvStore);
        if (succeed) {
            isRekeySuccess = true;
            break;
        }
        ++retry;
    }
    dbManager->CloseKvStore(kvStore);
    kvStore = nullptr;
    return isRekeySuccess;
}

Status StoreFactory::RekeyRecover(const std::string &storeId, const std::string &path, DBPassword &dbPassword,
    std::shared_ptr<DBManager> dbManager, const Options &options)
{
    auto rekeyPath = path + "/rekey";
    auto keyName = path + "/key/" + storeId + ".key";
    Status pwdValid = DB_ERROR;
    if (StoreUtil::IsFileExist(keyName)) {
        dbPassword = SecurityManager::GetInstance().GetDBPassword(storeId, path);
        pwdValid = CheckPwdValid(storeId, dbManager, options, dbPassword);
    }

    if (pwdValid == SUCCESS) {
        StoreUtil::Remove(rekeyPath);
        return pwdValid;
    }
    auto reKeyFile = storeId + REKEY_NEW;
    auto rekeyName = path + "/rekey/key/" + reKeyFile + ".key";
    if (StoreUtil::IsFileExist(rekeyName)) {
        dbPassword = SecurityManager::GetInstance().GetDBPassword(reKeyFile, rekeyPath);
        pwdValid = CheckPwdValid(storeId, dbManager, options, dbPassword);
    } else {
        return pwdValid;
    }
    if (pwdValid == SUCCESS) {
        UpdateKeyFile(storeId, path);
    }
    return pwdValid;
}

Status StoreFactory::CheckPwdValid(const std::string &storeId, std::shared_ptr<DBManager> dbManager,
    const Options &options, DBPassword &dbPassword)
{
    DBStatus status;
    DBStore *kvstore;
    auto dbOption = GetDBOption(options, dbPassword);
    dbManager->GetKvStore(storeId, dbOption, [&status, &kvstore](auto dbStatus, auto *dbStore) {
        status = dbStatus;
        kvstore = dbStore;
    });
    dbManager->CloseKvStore(kvstore);
    return StoreUtil::ConvertStatus(status);
}

bool StoreFactory::ExecuteRekey(const std::string &storeId, const std::string &path, DBPassword &dbPassword,
    DBStore *dbStore)
{
    std::string rekeyPath = path + "/rekey";
    (void)StoreUtil::InitPath(rekeyPath);

    auto newDbPassword = SecurityManager::GetInstance().GetDBPassword(storeId + REKEY_NEW, rekeyPath, true);
    if (!newDbPassword.IsValid()) {
        ZLOGE("failed to generate new key.");
        newDbPassword.Clear();
        StoreUtil::Remove(rekeyPath);
        return false;
    }

    auto dbStatus = dbStore->Rekey(newDbPassword.password);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("failed to rekey the substitute database.");
        StoreUtil::Remove(rekeyPath);
        newDbPassword.Clear();
        return false;
    }
    UpdateKeyFile(storeId, path);
    dbPassword.password = newDbPassword.password;
    newDbPassword.Clear();
    dbPassword.isKeyOutdated = false;
    return true;
}

void StoreFactory::UpdateKeyFile(const std::string &storeId, const std::string &path)
{
    std::string rekeyFile = path + "/rekey/key/" + storeId + REKEY_NEW + ".key";
    std::string keyFile = path + "/key/" + storeId + ".key";
    StoreUtil::Rename(rekeyFile, keyFile);
    StoreUtil::Remove(rekeyFile);
}
} // namespace OHOS::DistributedKv
