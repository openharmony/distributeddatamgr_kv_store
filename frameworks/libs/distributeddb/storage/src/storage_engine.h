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

#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H

#include <condition_variable>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <sys/stat.h>

#include "db_types.h"
#include "macro_utils.h"
#include "sqlite_utils.h"
#include "storage_executor.h"
#include "kvdb_commit_notify_filterable_data.h"
#include "runtime_context.h"

namespace DistributedDB {
struct StorageEngineAttr {
    uint32_t minWriteNum = 1;
    uint32_t maxWriteNum = 1;
    uint32_t minReadNum = 1;
    uint32_t maxReadNum = 1;
};

class StorageEngine : public RefObject {
public:
    StorageEngine();
    ~StorageEngine() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(StorageEngine);

    int Init(bool isEnhance = false);

    virtual int ReInit();

    StorageExecutor *FindExecutor(bool writable, OperatePerm perm, int &errCode, bool isExternal = false,
        int waitTime = MAX_WAIT_TIME);

    void Recycle(StorageExecutor *&handle, bool isExternal = false);

    // Enable/disable the delayed release of read executors. delayTimeMs in milliseconds.
    void SetReadExecutorDelayRelease(bool isDelayRelease, uint32_t delayTimeMs);

    // Stop the delayed release timer if it is running.
    void StopDelayedReleaseTimer();

    virtual bool IsEngineCorrupted() const;

    void Release();

    int TryToDisable(bool isNeedCheckAll, OperatePerm disableType = OperatePerm::DISABLE_PERM);

    void Enable(OperatePerm enableType = OperatePerm::NORMAL_PERM);

    void Abort(OperatePerm enableType = OperatePerm::NORMAL_PERM);

    virtual bool IsNeedTobeReleased() const;

    virtual const std::string &GetIdentifier() const;

    EngineState GetEngineState() const;

    void SetEngineState(EngineState state);

    virtual int ExecuteMigrate();

    virtual void SetNotifiedCallback(const std::function<void(int, KvDBCommitNotifyFilterAbleData *)> &callback);

    void SetConnectionFlag(bool isExisted);

    bool IsExistConnection() const;

    virtual int CheckEngineOption(const KvDBProperties &kvdbOption) const;

    virtual bool IsMigrating() const;

    void WaitWriteHandleIdle();

    virtual void IncreaseCacheRecordVersion();
    virtual uint64_t GetCacheRecordVersion() const;
    virtual uint64_t GetAndIncreaseCacheRecordVersion();

    virtual void SetSchemaChangedCallback(const std::function<int(void)> &callback);

    void CloseAllExecutor();

    int InitAllReadWriteExecutor();

    OpenDbProperties GetOption() const;

    void SetUri(const std::string &uri);

    const std::string GetDataDirIdentifier() const;

    void SetDataDirIdentifier(const std::string &dataDirIdentifier);
protected:
    virtual int CreateNewExecutor(bool isWrite, StorageExecutor *&handle) = 0;

    void CloseExecutor();

    virtual void AddStorageExecutor(StorageExecutor *handle, bool isExternal);

    static bool CheckEngineAttr(const StorageEngineAttr &poolSize);

    int InitReadWriteExecutors();

    void SetSQL(const std::vector<std::string> &sql);
    void SetSecurityOption(const SecurityOption &option);
    void SetCreateIfNecessary(bool isCreateIfNecessary);

    mutable std::mutex optionMutex_;
    OpenDbProperties option_;

    StorageEngineAttr engineAttr_;
    bool isUpdated_;
    std::atomic<bool> isMigrating_;
    std::string identifier_;
    std::string hashIdentifier_;
    mutable std::mutex dataDirIdMutex_;
    std::string dataDirIdentifier_;

    // Mutex for commitNotifyFunc_.
    mutable std::shared_mutex notifyMutex_;

    // Callback function for commit notify.
    std::function<void(int, KvDBCommitNotifyFilterAbleData *)> commitNotifyFunc_;

    // Mutex for schemaChangedFunc_.
    mutable std::shared_mutex schemaChangedMutex_;

    // Callback function for schema changed.
    std::function<int(void)> schemaChangedFunc_;

    bool isSchemaChanged_;

    bool isEnhance_;

private:
    StorageExecutor *FetchStorageExecutor(bool isWrite, std::list<StorageExecutor *> &idleList,
        std::list<StorageExecutor *> &usingList, int &errCode, bool isExternal = false);

    StorageExecutor *FindWriteExecutor(OperatePerm perm, int &errCode, int waitTime, bool isExternal = false);
    StorageExecutor *FindReadExecutor(OperatePerm perm, int &errCode, int waitTime, bool isExternal = false);

    StorageExecutor *FetchReadStorageExecutor(int &errCode, bool isExternal, bool isNeedCreate);

    // Put an excess read executor into the delayed release list instead of deleting it immediately.
    void AddToDelayedRelease(StorageExecutor *handle, bool isExternal);

    // Try to reuse a not-yet-expired read executor from the delayed release list.
    StorageExecutor *FetchFromDelayedRelease(bool isExternal);

    // Start the delayed release timer to release expired read executors. Returns -1 on failure.
    int StartDelayedReleaseTimer();

    // Compute the earliest expire time among all pending delayed executors.
    std::chrono::steady_clock::time_point GetEarliestDelayedExpireTime();

    // Timer callback: release the expired read executors.
    int DelayedReleaseTimerCallback(TimerId timerId);

    // Lazily destroy the read executors whose delay time has elapsed.
    void ReleaseExpiredDelayedReadExecutors();

    // Collect the expired read executors from a single delayed release list.
    static void CollectExpiredDelayedExecutors(
        std::list<std::pair<StorageExecutor *, std::chrono::steady_clock::time_point>> &delayedList,
        const std::chrono::steady_clock::time_point &now, std::list<StorageExecutor *> &expiredHandles);

    // Recycle an excess read executor (delayed release or immediate delete). Returns the handle to be deleted.
    StorageExecutor *RecycleExcessReadExecutor(StorageExecutor *handle, bool isExternal, bool &needStartTimer);

    // Delete all executors kept in a delayed release list.
    static void ClearDelayedReleaseList(
        std::list<std::pair<StorageExecutor *, std::chrono::steady_clock::time_point>> &delayedList);

    virtual void ClearCorruptedFlag();

    std::string LogAndCheckFileStat(const std::string& filePath, struct stat& fileStat, const std::string& logPrefix);

    void PrintDbFileMsg(bool isOpen);

    static const int MAX_WAIT_TIME;
    static const int MAX_WRITE_SIZE;
    static const int MAX_READ_SIZE;

    std::mutex initMutex_;
    std::condition_variable initCondition_;
    std::atomic<bool> isInitialized_;
    OperatePerm perm_;
    bool operateAbort_;

    std::mutex readMutex_;
    std::mutex writeMutex_;
    std::condition_variable writeCondition_;
    std::condition_variable readCondition_;
    std::list<StorageExecutor *> writeUsingList_;
    std::list<StorageExecutor *> writeIdleList_;
    std::list<StorageExecutor *> readUsingList_;
    std::list<StorageExecutor *> readIdleList_;
    std::list<StorageExecutor *> externalWriteUsingList_;
    std::list<StorageExecutor *> externalWriteIdleList_;
    std::list<StorageExecutor *> externalReadUsingList_;
    std::list<StorageExecutor *> externalReadIdleList_;
    std::atomic<bool> isExistConnection_;

    // Delayed release of read executors: when enabled, excess read executors are kept alive
    // for delayTime_ milliseconds so that a subsequent read request can reuse them.
    std::list<std::pair<StorageExecutor *, std::chrono::steady_clock::time_point>> readDelayedReleaseList_;
    std::list<std::pair<StorageExecutor *, std::chrono::steady_clock::time_point>> externalReadDelayedReleaseList_;
    std::mutex delayedTimerMutex_;
    TimerId delayedReleaseTimerId_ = 0;

    std::mutex idleMutex_;
    std::condition_variable idleCondition_;

    std::atomic<int> readPendingCount_;
    std::atomic<int> externalReadPendingCount_;

    mutable std::mutex stateMutex_;
    EngineState engineState_;

    bool isDelayRelease_;
    uint32_t delayTime_;
};
} // namespace DistributedDB
#endif // STORAGE_ENGINE_H
