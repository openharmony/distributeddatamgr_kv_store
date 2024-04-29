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

#ifndef SINGLE_KV_STORE_H
#define SINGLE_KV_STORE_H

#include <map>
#include "kvstore.h"
#include "kvstore_observer.h"
#include "kvstore_result_set.h"
#include "kvstore_sync_callback.h"
#include "types.h"
#include "data_query.h"

namespace OHOS {
namespace DistributedKv {
// This is a public interface. Implementation of this class is in AppKvStoreImpl.
// This class provides put, delete, search, sync and subscribe functions of a key-value store.
class API_EXPORT SingleKvStore : public virtual KvStore {
public:
    /**
     * @brief Constructor.
     */
    API_EXPORT SingleKvStore() = default;

    /**
     * @brief Destructor.
     */
    API_EXPORT virtual ~SingleKvStore() {}

    /**
     * @brief Get value from KvStore by its key.
     * @param key   Key of this entry.
     * @param value Value will be returned in this parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Get(const Key &key, Value &value) = 0;

    /**
     * @brief Get value from KvStore by its key.
     * A compensation synchronization will be automatically triggered to the
     * specified device only when the networkId is valid.
     * This interface is only effective for dynamic data, others do not support.
     *
     * @param key Key of this entry.
     * @param networkId The networkId of online device.
     * @param onResult Value and Status will be returned in this parameter.
    */
    virtual void Get(const Key &key,
        const std::string &networkId, const std::function<void(Status, Value &&)> &onResult) {}

    /**
     * @brief Get all entries in this store which key start with prefixKey.
     * A compensation synchronization will be automatically triggered to the
     * specified device only when the networkId is valid.
     * This interface is only effective for dynamic data, others do not support.
     *
     * @param prefix The prefix to be searched.
     * @param networkId The networkId of online device.
     * @param onResult Entries and Status will be returned in this parameter.
    */
    virtual void GetEntries(const Key &prefix,
        const std::string &networkId, const std::function<void(Status, std::vector<Entry> &&)> &onResult) {}

    /**
     * @brief Get all entries in this store which key start with prefixKey.
     * @param prefix  The prefix to be searched.
     * @param entries Entries will be returned in this parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status GetEntries(const Key &prefix, std::vector<Entry> &entries) const = 0;

    /**
     * @brief Get all entries in this store by query.
     * @param query   The query object.
     * @param entries Entries will be returned in this parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status GetEntries(const DataQuery &query, std::vector<Entry> &entries) const = 0;

    /**
     * @brief Get resultSet in this store which key start with prefixKey.
     * @param prefix    The prefix to be searched.
     * @param resultSet ResultSet will be returned in this parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status GetResultSet(const Key &prefix, std::shared_ptr<KvStoreResultSet> &resultSet) const = 0;

    /**
     * @brief Get resultSet in this store by query.
     * @param query     The query object.
     * @param resultSet ResultSet will be returned in this parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status GetResultSet(const DataQuery &query, std::shared_ptr<KvStoreResultSet> &resultSet) const = 0;

    /**
     * @brief Close the resultSet returned by GetResultSet.
     * @param resultSet ResultSet will be returned in this parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status CloseResultSet(std::shared_ptr<KvStoreResultSet> &resultSet) = 0;

    /**
     * @brief Get the number of result by query.
     * @param query The query object.
     * @param count Result will be returned in this parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status GetCount(const DataQuery &query, int &count) const = 0;

    /**
     * @brief Remove the device data synced from remote.
     *
     * Remove all the other devices data synced from remote if device is empty.
     *
     * @param device Device id.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status RemoveDeviceData(const std::string &device) = 0;

    /**
     * @brief Get the kvstore security level.
     *
     * The security level is set when create store by options parameter.
     *
     * @param secLevel The security level will be returned.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status GetSecurityLevel(SecurityLevel &secLevel) const = 0;

    /**
     * @brief Sync store with other devices.
     *
     * This is an asynchronous method.
     * sync will fail if there is a syncing operation in progress.
     *
     * @param devices Device list to sync.
     * @param mode    Mode can be set to SyncMode::PUSH, SyncMode::PULL and SyncMode::PUTH_PULL.
     *                PUSH_PULL will firstly push all not-local store to listed devices,
     *                then pull these stores back.
     * @param delay   Allowed delay milli-second to sync.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t delay) = 0;

    /**
     * @brief Sync store with other devices only syncing the data which is satisfied with the condition.
     *
     * This is an asynchronous method.
     * sync will fail if there is a syncing operation in progress.
     *
     * @param devices      Device list to sync.
     * @param mode         Mode can be set to SyncMode::PUSH, SyncMode::PULL and SyncMode::PUSH_PULL.
     *                     PUSH_PULL will firstly push all not-local store to listed devices,
     *                     then pull these stores back.
     * @param query        The query condition.
     * @param syncCallback The callback will be called when sync finished.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
        std::shared_ptr<KvStoreSyncCallback> syncCallback)
    {
        return Sync(devices, mode, query, syncCallback, 0);
    }

    /**
     * @brief Sync store with other devices only syncing the data which is satisfied with the condition.
     *
     * This is an asynchronous method.
     * sync will fail if there is a syncing operation in progress.
     *
     * @param devices      Device list to sync.
     * @param mode         Mode can be set to SyncMode::PUSH, SyncMode::PULL and SyncMode::PUSH_PULL.
     *                     PUSH_PULL will firstly push all not-local store to listed devices,
     *                     then pull these stores back.
     * @param query        The query condition.
     * @param delay   Allowed delay milli-second to sync.
     * @param syncCallback The callback will be called when sync finished.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
        std::shared_ptr<KvStoreSyncCallback> syncCallback, uint32_t delay)
    {
        return Status::SUCCESS;
    }

    /**
     * @brief Sync store with other device, while delay is 0.
    */
    API_EXPORT inline Status Sync(const std::vector<std::string> &devices, SyncMode mode)
    {
        return Sync(devices, mode, 0);
    }

    /**
     * @brief Sync store with other device.
     *
     * which is satisfied with the condition.
     * the callback pointer is nullptr.
    */
    API_EXPORT inline Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query)
    {
        return Sync(devices, mode, query, nullptr);
    }

    /**
     * @brief Register message for sync operation.
     * @param callback Callback to register.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status RegisterSyncCallback(std::shared_ptr<KvStoreSyncCallback> callback) = 0;

    /**
     * @brief Unregister all message for sync operation.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status UnRegisterSyncCallback() = 0;

    /**
     * @brief Set synchronization parameters of this store.
     * @param syncParam Sync policy parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status SetSyncParam(const KvSyncParam &syncParam) = 0;

    /**
     * @brief Get synchronization parameters of this store.
     * @param syncParam Sync policy parameter.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status GetSyncParam(KvSyncParam &syncParam) = 0;

    /**
     * @brief Set capability available for sync.
     *
     * If enabled is true, it will check permission before sync operation.
     * only local labels and remote labels overlapped syncing works.
     *
     * @param enabled Bool paramater
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status SetCapabilityEnabled(bool enabled) const = 0;

    /**
     * @brief Set capability range for syncing.
     *
     * Should set capability available firstly.
     *
     * @param localLabels  Local labels defined by a list of string value.
     * @param remoteLabels Remote labels defined by a list of string value.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status SetCapabilityRange(const std::vector<std::string> &localLabels,
                                      const std::vector<std::string> &remoteLabels) const = 0;

    /**
     * @brief Subscribe store with other devices consistently Synchronize the data
     *        which is satisfied with the condition.
     * @param devices Device list to sync.
     * @param query   The query condition.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) = 0;

    /**
     * @brief Unsubscribe store with other devices which is satisfied with the condition.
     * @param devices Device list to sync.
     * @param query   The query condition.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) = 0;

    /**
     * @brief Set identifier.
     * @param accountId The accountId.
     * @param appId The name of the application.
     * @param storeId The name of kvstore.
     * @param tagretDev target device list.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status SetIdentifier(const std::string &accountId, const std::string &appId,
        const std::string &storeId, const std::vector<std::string> &tagretDev)
    {
        return Status::SUCCESS;
    };

    /**
     * @brief Sync store to cloud.
     *
     * Sync store to cloud.
     *
     * @return Return SUCCESS for success, others for failure.
     */
    virtual Status CloudSync(const AsyncDetail &async)
    {
        return Status::SUCCESS;
    }
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // SINGLE_KV_STORE_H