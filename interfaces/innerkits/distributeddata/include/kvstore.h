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

#ifndef KVSTORE_H
#define KVSTORE_H

#include <map>
#include "kvstore_observer.h"
#include "types.h"

namespace OHOS {
namespace DistributedKv {
class API_EXPORT KvStore {
public:
    /**
     * @brief Constructor.
     */
    API_EXPORT KvStore() = default;

    /**
     * @brief Forbidden copy constructor.
     */
    KvStore(const KvStore &) = delete;

    /**
     * @brief Forbidden copy constructor.
     */
    KvStore &operator=(const KvStore &) = delete;

    /**
     * @brief Destructor.
     */
    API_EXPORT virtual ~KvStore() {}

    /**
     * @brief Get kvstore name of this kvstore instance.
     * @return The kvstore name.
    */
    virtual StoreId GetStoreId() const = 0;

    /**
     * @brief Put one entry with key-value into kvstore.
     *
     * Mutation operations.
     * Key level operations.
     * Mutations are bundled together into atomic commits. If a transaction is in
     * progress, the list of mutations bundled together is tied to the current
     * transaction. If no transaction is in progress, mutations will be a unique transaction.
     *
     * @param key   The key, key length should not be greater than 256, and can not be empty.
     * @param value The value, value size should be less than IPC transport limit, and can not be empty.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Put(const Key &key, const Value &value) = 0;

    /**
     * @brief Put a list of entries to kvstore.
     *
     * all entries will be put in a transaction,
     * if entries contains invalid entry, PutBatch will all fail.
     * entries's size should be less than 128 and memory size must be less than IPC transport limit.
     *
     * @param entries The entries.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status PutBatch(const std::vector<Entry> &entries) = 0;

    /**
     * @brief Delete one entry in the kvstore.
     * @param key The key, key length should not be greater than 256, and can not be empty.
     * @return Return SUCCESS for success, others for failure.
     *         delete non-exist key still return KEY NOT FOUND error.
    */
    virtual Status Delete(const Key &key) = 0;

    /**
     * @brief Delete a list of entries in the kvstore.
     *
     * key length should not be greater than 256, and can not be empty.
     * if keys contains invalid key, all delete will fail.
     * keys memory size should not be greater than IPC transport limit, and can not be empty.
     *
     * @param keys The keys.
     * @return Return SUCCESS for success, others for failure.
     *         delete key not exist still return success,
    */
    virtual Status DeleteBatch(const std::vector<Key> &keys) = 0;

    /**
     * @brief Start transaction.
     *
     * all changes to this kvstore will be in a same transaction
     * and will not change the store until #Commit() or #Rollback() is called.
     * before this transaction is committed or rollbacked,
     * all attemption to close this store will fail.
     *
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status StartTransaction() = 0;

    /**
     * @brief Commit current transaction.
     *
     * all changes to this store will be done after calling this method.
     * any calling of this method outside a transaction will fail.
     *
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Commit() = 0;

    /**
     * @brief Rollback current transaction.
     *
     * all changes to this store during this transaction will be rollback after calling this method.
     * any calling of this method outside a transaction will fail.
     *
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Rollback() = 0;

    /**
     * @brief Subscribe kvstore to watch data change in the kvstore.
     *
     * OnChange in he observer will be called when data changed, with all the changed contents.
     * client is responsible for free observer after and only after call UnSubscribeKvStore.
     * otherwise, codes in sdk may use a freed memory and cause unexpected result.
     *
     * @param type     Strategy for this subscribe, default right now.
     * @param observer Callback client provided, client must implement KvStoreObserver and
     *                 override OnChange function, when data changed in store,
     *                 OnChange will called in Observer.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status SubscribeKvStore(SubscribeType type, std::shared_ptr<KvStoreObserver> observer) = 0;

    /**
     * @brief Unsubscribe kvstore to un-watch data change in the kvstore.
     *
     * after this call, no message will be received even data change in the kvstore.
     * client is responsible for free observer after and only after call UnSubscribeKvStore.
     * otherwise, codes in sdk may use a freed memory and cause unexpected result.
     *
     * @param type     Strategy for this subscribe, default right now.
     * @param observer Callback client provided in #SubscribeKvStore().
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status UnSubscribeKvStore(SubscribeType type, std::shared_ptr<KvStoreObserver> observer) = 0;

    /**
     * @brief Backup the store to a specified backup file.
     * @param file    Target file of backup.
     * @param baseDir Root path of store manager.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Backup(const std::string &file, const std::string &baseDir) = 0;

    /**
     * @brief Restore the store from a specified backup file.
     * @param file    The file of backup data.
     * @param baseDir Root path of store manager.
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status Restore(const std::string &file, const std::string &baseDir) = 0;

    /**
     * @brief Delete the backup files.
     * @param files   Files the list of backup file to be delete.
     * @param baseDir Root path of store manager.
     * @param status  Result of delete backup
     * @return Return SUCCESS for success, others for failure.
    */
    virtual Status DeleteBackup(const std::vector<std::string> &files, const std::string &baseDir,
        std::map<std::string, DistributedKv::Status> &status) = 0;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // KVSTORE_H
