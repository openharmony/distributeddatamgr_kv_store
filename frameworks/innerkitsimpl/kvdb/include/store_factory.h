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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_FACTORY_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_FACTORY_H
#include <memory>
#include "concurrent_map.h"
#include "convertor.h"
#include "kv_store_delegate_manager.h"
#include "security_manager.h"
#include "single_store_impl.h"
namespace OHOS::DistributedKv {
struct StoreParams {
    bool isCreate = false;
    SecurityManager::DBPassword password;
};
class StoreFactory {
public:
    using DBPassword = DistributedKv::SecurityManager::DBPassword;
    static StoreFactory &GetInstance();
    std::shared_ptr<SingleKvStore> GetOrOpenStore(const AppId &appId, const StoreId &storeId, const Options &options,
        Status &status, StoreParams &storeParams);
    Status Delete(const AppId &appId, const StoreId &storeId, const std::string &path, int32_t subUser = 0);
    Status Close(const AppId &appId, const StoreId &storeId, int32_t subUser = 0, bool isForce = false);
    Status Close(const AppId &appId, const StoreId &storeId, const std::string &baseDir, bool isForce = false);

private:
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using DBOption = DistributedDB::KvStoreNbDelegate::Option;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBStatus = DistributedDB::DBStatus;

    static constexpr int REKEY_TIMES = 3;
    static constexpr const char *REKEY_NEW = ".new";
    static constexpr uint64_t MAX_WAL_SIZE = 200 * 1024 * 1024; // the max size of WAL is 200MB

    StoreFactory();
    std::shared_ptr<DBManager> GetDBManager(const std::string &path, const AppId &appId, int32_t subUser = 0);
    DBOption GetDBOption(const Options &options, const DBPassword &dbPassword) const;
    void ReKey(const std::string &storeId, const std::string &path, DBPassword &dbPassword,
        std::shared_ptr<DBManager> dbManager, const Options &options);
    Status RekeyRecover(const std::string &storeId, const std::string &path, DBPassword &dbPassword,
        std::shared_ptr<DBManager> dbManager, const Options &options);
    bool ExecuteRekey(const std::string &storeId, const std::string &path, DBPassword &dbPassword, DBStore *dbStore);
    Status IsPwdValid(const std::string &storeId, std::shared_ptr<DBManager> dbManager, const Options &options,
        DBPassword &dbPassword);
    Status SetDbConfig(std::shared_ptr<DBStore> dbStore);
    std::string GenerateKey(const std::string &keySuffix, const std::string &storeId) const;
    Status CloseInner(const AppId &appId, const StoreId &storeId, const std::string &keySuffix, bool isForce);
    ConcurrentMap<std::string, std::shared_ptr<DBManager>> dbManagers_;
    ConcurrentMap<std::string, std::map<std::string, std::shared_ptr<SingleStoreImpl>>> stores_;
    Convertor *convertors_[INVALID_TYPE];
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_FACTORY_H
