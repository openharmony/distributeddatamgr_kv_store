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

#ifndef STORAGE_ENGINE_MANAGER_H
#define STORAGE_ENGINE_MANAGER_H

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>

#include "storage_engine.h"

namespace DistributedDB {
class StorageEngineManager final {
public:
    static StorageEngine *GetStorageEngine(const KvDBProperties &property, int &errCode);

    static int ReleaseStorageEngine(StorageEngine *storageEngine);

    static int ForceReleaseStorageEngine(const std::string &identifier);

    static int ExecuteMigration(StorageEngine *storageEngine);

    static void DeleteInstance();
    static bool IsInstanceDestroyed();
    static void SetInstanceDestroyed(bool isDestroyed);

    DISABLE_COPY_ASSIGN_MOVE(StorageEngineManager);

    StorageEngineManager();
    ~StorageEngineManager();
private:
    // Get a StorageEngineManager instance, Singleton mode
    static std::shared_ptr<StorageEngineManager> GetInstance();

    int RegisterLockStatusListener();

    void LockStatusNotifier(bool isAccessControlled);

    StorageEngine *CreateStorageEngine(const KvDBProperties &property, int &errCode);

    StorageEngine *FindStorageEngine(const std::string &identifier);

    void InsertStorageEngine(const std::string &identifier, StorageEngine *&storageEngine);

    void EraseStorageEngine(const std::string &identifier);

    void ReleaseResources(const std::string &identifier);

    int ReleaseEngine(StorageEngine *releaseEngine);

    void ReleaseAllStorageEngines();

    void EnterGetEngineProcess(const std::string &identifier);

    void ExitGetEngineProcess(const std::string &identifier);

    static std::shared_mutex instanceMutex_;
    static std::shared_ptr<StorageEngineManager> instance_;
    static std::atomic<bool> instanceDestroyed_;
    static volatile bool isRegLockStatusListener_;

    static std::mutex storageEnginesLock_;
    std::map<std::string, StorageEngine *> storageEngines_;

    std::mutex getEngineMutex_;
    std::condition_variable getEngineCondition_;
    std::set<std::string> getEngineSet_;

    NotificationChain::Listener *lockStatusListener_;
};
} // namespace DistributedDB

#endif // STORAGE_ENGINE_MANAGER_H
