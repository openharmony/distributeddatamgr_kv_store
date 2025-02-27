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

#ifndef DISTRIBUTED_KV_DATA_MANAGER_H
#define DISTRIBUTED_KV_DATA_MANAGER_H
#include <mutex>
#include <functional>
#include "executor_pool.h"
#include "kvstore.h"
#include "kvstore_death_recipient.h"
#include "kvstore_observer.h"
#include "single_kvstore.h"
#include "types.h"
#include "end_point.h"

namespace OHOS {
namespace DistributedKv {
class API_EXPORT DistributedKvDataManager final {
public:
    /**
     * @brief Constructor.
     */
    API_EXPORT DistributedKvDataManager();

    /**
     * @brief Destructor.
     */
    API_EXPORT ~DistributedKvDataManager();

    /**
     * @brief Open kvstore instance with the given storeId, creating it if needed.
     *
     * It is allowed to open the same kvstore concurrently
     * multiple times, but only one KvStoreImpl will be created.
     *
     * @param options The config of the kvstore, including encrypt,
     *                create if needed and whether need sync between devices.
     * @param appId   The name of the application.
     * @param storeId The name of the kvstore.
     * @param singleKvStore Output param, kvstore instance.
     * @return Callback: including status and KvStore instance returned by this call.
     *         callback will return:
     *         if Options.createIfMissing is false and kvstore has not been created before,
     *         STORE_NOT_FOUND and nullptr,
     *         if storeId is invalid, INVALID_ARGUMENT and nullptr,
     *         if appId has no permission, PERMISSION_DENIED and nullptr,
     *         otherwise, SUCCESS and the unipue_ptr of kvstore,
     *         which client can use to operate kvstore, will be returned.
    */
    API_EXPORT Status GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
                                       std::shared_ptr<SingleKvStore> &singleKvStore);

    /**
     * @brief Get all existed kvstore names.
     * @param appId    The name of the application.
     * @param storeIds Output param, all existed kvstore names.
     * @return Will return:
     *         if appId is invalid, PERMISSION_DENIED and empty vector,
     *         otherwise, SUCCESS and the vector of kvstore names, will be returned.
    */
    API_EXPORT Status GetAllKvStoreId(const AppId &appId, std::vector<StoreId> &storeIds);

    /**
     * @brief Disconnect kvstore instance from kvstoreimpl with the given storeId.
     *
     * if all kvstore created for a single kvsotreimpl, kvstoreimpl and resource below will be freed.
     * before this call, all KvStoreSnapshot must be released firstly,
     * otherwise this call will fail.
     * after this call, kvstore and kvstoresnapshot become invalid.
     * call to it will return nullptr exception.
     *
     * @warning Try to close a KvStore while other thread(s) still using it may cause process crash.
     * @param appId   The name of the application.
     * @param storeId The name of the kvstore.
     * @return Return SUCCESS for success, others for failure.
    */
    API_EXPORT Status CloseKvStore(const AppId &appId, const StoreId &storeId);

    /**
     * @brief Disconnect kvstore instance from kvstoreimpl.
     *
     * if all kvstore created for a single kvsotreimpl, kvstoreimpl and resource below will be freed.
     * before this call, all KvStoreResultSet must be released firstly,
     * otherwise this call will fail.
     * after this call, kvstore and KvStoreResultSet become invalid.
     * call to it will return nullptr exception.
     *
     * @warning Try to close a KvStore while other thread(s) still using it may cause process crash.
     * @param appId      The name of the application.
     * @param kvStorePtr The pointer of the kvstore.
     * @return Return SUCCESS for success, others for failure.
    */
    API_EXPORT Status CloseKvStore(const AppId &appId, std::shared_ptr<SingleKvStore> &kvStore);

    /**
     * @brief Close all opened kvstores for this appId.
     * @warning Try to close a KvStore while other thread(s) still using it may cause process crash.
     * @param appId  The name of the application.
     * @return Return SUCCESS for success, others for failure.
    */
    API_EXPORT Status CloseAllKvStore(const AppId &appId);

    /**
     * @brief Delete kvstore file with the given storeId.
     *
     * client should first close all connections to it and then delete it,
     * otherwise delete may return error.
     * after this call, kvstore and kvstoresnapshot become invalid.
     * call to it will return error.
     *
     * @param appId   The name of the application.
     * @param storeId The name of the kvstore.
     * @param path    The base directory of the kvstore, it must be available.
     * @return Return SUCCESS for success, others for failure.
    */
    API_EXPORT Status DeleteKvStore(const AppId &appId, const StoreId &storeId, const std::string &path = "");

    /**
     * @brief Delete all kvstore for this appId.
     *
     * delete all kvstore belong to this appId unless the kvstore is opened or not.
     *
     * @param appId The name of the application.
     * @param path  The base directory of the application, it must be available.
     * @return Return SUCCESS for success, others for failure.
    */
    API_EXPORT Status DeleteAllKvStore(const AppId &appId, const std::string &path = "");

    /**
     * @brief Observer the kvstore service death.
     * @param deathRecipient The pointer of the observer.
    */
    API_EXPORT void RegisterKvStoreServiceDeathRecipient(std::shared_ptr<KvStoreDeathRecipient> deathRecipient);

    /**
     * @brief Stop observer the kvstore service death.
     * @param deathRecipient The pointer of the observer.
    */
    API_EXPORT void UnRegisterKvStoreServiceDeathRecipient(std::shared_ptr<KvStoreDeathRecipient> deathRecipient);

    /**
     * @brief Set executors to kv client.
     * @param executors The executors.
    */
    API_EXPORT void SetExecutors(std::shared_ptr<ExecutorPool> executors);

    /**
     * @brief set endpoint for client sync.
     * @param endpoint The pointer of endpoint.
     * @return Return SUCCESS for success, others for failure.
     */
    API_EXPORT Status SetEndpoint(std::shared_ptr<Endpoint> endpoint);

    /**
     * @brief Put switch data to database.
     *
     * @param appId The name of the application.
     * @param SwitchData The value of switch data.
     * @return Return SUCCESS for success, others for failure.
     */
    API_EXPORT Status PutSwitch(const AppId &appId, const SwitchData &data);

    /**
     * @brief Get switch data by networkId.
     *
     * @param appId The name of the application.
     * @param networkId The networkId of device.
     * @return Return Status and SwitchData, Status is SUCCESS for success, others for failure.
     */
    API_EXPORT std::pair<Status, SwitchData> GetSwitch(const AppId &appId, const std::string &networkId);

    /**
     * @brief Subscribe swicth data to watch data change in the store.
     *
     * Oobserver will be called when data changed, with all the changed contents.
     * client is responsible for free observer after and only after call UnsubscribeSwitchData.
     * otherwise, codes in sdk may use a freed memory and cause unexpected result.
     *
     * @param appId The name of the application.
     * @param observer Callback client provided, observer function will called, when data changed in store.
     * @return Return SUCCESS for success, others for failure.
     */
    API_EXPORT Status SubscribeSwitchData(const AppId &appId, std::shared_ptr<KvStoreObserver> observer);

    /**
     * @brief Unsubscribe swicth data to un-watch data change in the store.
     *
     * after this call, no message will be received even data change in the store.
     * client is responsible for free observer after and only after call UnsubscribeSwitchData.
     * otherwise, codes in sdk may use a freed memory and cause unexpected result.
     *
     * @param appId The name of the application.
     * @param observer Callback client provided, observer function will called, when data changed in store.
     * @return Return SUCCESS for success, others for failure.
    */
    API_EXPORT Status UnsubscribeSwitchData(const AppId &appId, std::shared_ptr<KvStoreObserver> observer);
private:
    static bool isAlreadySet_;
    std::mutex mutex_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // DISTRIBUTED_KV_DATA_MANAGER_H
