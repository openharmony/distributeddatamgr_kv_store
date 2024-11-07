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

#include "storage_engine.h"

#include <algorithm>

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
const int StorageEngine::MAX_WAIT_TIME = 30;
const int StorageEngine::MAX_WRITE_SIZE = 1;
const int StorageEngine::MAX_READ_SIZE = 16;

StorageEngine::StorageEngine()
    : isUpdated_(false),
      isMigrating_(false),
      commitNotifyFunc_(nullptr),
      schemaChangedFunc_(nullptr),
      isSchemaChanged_(false),
      isEnhance_(false),
      isInitialized_(false),
      perm_(OperatePerm::NORMAL_PERM),
      operateAbort_(false),
      isExistConnection_(false),
      engineState_(EngineState::INVALID)
{}

StorageEngine::~StorageEngine()
{
    CloseExecutor();
}

void StorageEngine::CloseAllExecutor()
{
    CloseExecutor();
}

int StorageEngine::InitAllReadWriteExecutor()
{
    return InitReadWriteExecutors();
}

OpenDbProperties StorageEngine::GetOption()
{
    return option_;
}

int StorageEngine::InitReadWriteExecutors()
{
    int errCode = E_OK;
    std::scoped_lock initLock(writeMutex_, readMutex_);
    // only for create the database avoid the minimum number is 0.
    StorageExecutor *handle = nullptr;
    if (engineAttr_.minReadNum == 0 && engineAttr_.minWriteNum == 0) {
        errCode = CreateNewExecutor(true, handle);
        if (errCode != E_OK) {
            return errCode;
        }

        if (handle != nullptr) {
            delete handle;
            handle = nullptr;
        }
    }

    for (uint32_t i = 0; i < engineAttr_.minWriteNum; i++) {
        handle = nullptr;
        errCode = CreateNewExecutor(true, handle);
        if (errCode != E_OK) {
            return errCode;
        }
        AddStorageExecutor(handle, false);
    }

    for (uint32_t i = 0; i < engineAttr_.minReadNum; i++) {
        handle = nullptr;
        errCode = CreateNewExecutor(false, handle);
        if (errCode != E_OK) {
            return errCode;
        }
        AddStorageExecutor(handle, false);
    }
    return E_OK;
}


int StorageEngine::Init(bool isEnhance)
{
    if (isInitialized_.load()) {
        LOGD("Storage engine has been initialized!");
        return E_OK;
    }
    isEnhance_ = isEnhance;

    int errCode = InitReadWriteExecutors();
    if (errCode == E_OK) {
        isInitialized_.store(true);
        initCondition_.notify_all();
        return E_OK;
    } else if (errCode == -E_EKEYREVOKED) {
        // Assumed file system has classification function, can only get one write handle
        std::unique_lock<std::mutex> lock(writeMutex_);
        if (!writeIdleList_.empty() || !writeUsingList_.empty()) {
            isInitialized_.store(true);
            initCondition_.notify_all();
            return E_OK;
        }
    }
    initCondition_.notify_all();
    Release();
    return errCode;
}

int StorageEngine::ReInit()
{
    return E_OK;
}

StorageExecutor *StorageEngine::FindExecutor(bool writable, OperatePerm perm, int &errCode, bool isExternal,
    int waitTime)
{
    if (GetEngineState() == EngineState::ENGINE_BUSY) {
        LOGI("Storage engine is busy!");
        errCode = -E_BUSY;
        return nullptr;
    }

    {
        std::unique_lock<std::mutex> lock(initMutex_);
        bool result = initCondition_.wait_for(lock, std::chrono::seconds(waitTime), [this]() {
            return isInitialized_.load();
        });
        if (!result || !isInitialized_.load()) {
            LOGE("Storage engine is not initialized");
            errCode = -E_BUSY; // Usually in reinitialize engine, return BUSY
            return nullptr;
        }
    }

    if (writable) {
        return FindWriteExecutor(perm, errCode, waitTime, isExternal);
    }

    return FindReadExecutor(perm, errCode, waitTime, isExternal);
}

StorageExecutor *StorageEngine::FindWriteExecutor(OperatePerm perm, int &errCode, int waitTime, bool isExternal)
{
    LOGD("[FindWriteExecutor]Finding WriteExecutor");
    std::unique_lock<std::mutex> lock(writeMutex_);
    errCode = -E_BUSY;
    if (perm_ == OperatePerm::DISABLE_PERM || perm_ != perm) {
        LOGI("Not permitted to get the executor[%u]", static_cast<unsigned>(perm_));
        return nullptr;
    }
    std::list<StorageExecutor *> &writeUsingList = isExternal ? externalWriteUsingList_ : writeUsingList_;
    std::list<StorageExecutor *> &writeIdleList = isExternal ?  externalWriteIdleList_ : writeIdleList_;
    if (waitTime <= 0) { // non-blocking.
        if (writeUsingList.empty() &&
                writeIdleList.size() + writeUsingList.size() == engineAttr_.maxWriteNum) {
            return nullptr;
        }
        return FetchStorageExecutor(true, writeIdleList, writeUsingList, errCode, isExternal);
    }
    // Not prohibited and there is an available handle
    bool result = writeCondition_.wait_for(lock, std::chrono::seconds(waitTime),
        [this, &perm, &writeUsingList, &writeIdleList]() {
            return (perm_ == OperatePerm::NORMAL_PERM || perm_ == perm) && (!writeIdleList.empty() ||
                (writeIdleList.size() + writeUsingList.size() < engineAttr_.maxWriteNum) ||
                operateAbort_);
        });
    if (operateAbort_) {
        LOGI("Abort write executor and executor and busy for operate!");
        return nullptr;
    }
    if (!result) {
        LOGI("Get write handle result[%d], permissType[%u], operType[%u], write[%zu-%zu-%" PRIu32 "]", result,
            static_cast<unsigned>(perm_), static_cast<unsigned>(perm), writeIdleList.size(), writeUsingList.size(),
            engineAttr_.maxWriteNum);
        return nullptr;
    }
    return FetchStorageExecutor(true, writeIdleList, writeUsingList, errCode, isExternal);
}

StorageExecutor *StorageEngine::FindReadExecutor(OperatePerm perm, int &errCode, int waitTime, bool isExternal)
{
    std::unique_lock<std::mutex> lock(readMutex_);
    errCode = -E_BUSY;
    if (perm_ == OperatePerm::DISABLE_PERM || perm_ != perm) {
        LOGI("Not permitted to get the executor[%u]", static_cast<unsigned>(perm_));
        return nullptr;
    }

    std::list<StorageExecutor *> &readUsingList = isExternal ? externalReadUsingList_ : readUsingList_;
    std::list<StorageExecutor *> &readIdleList = isExternal ?  externalReadIdleList_ : readIdleList_;
    if (waitTime <= 0) { // non-blocking.
        if (readIdleList.empty() &&
            readIdleList.size() + readUsingList.size() == engineAttr_.maxReadNum) {
            return nullptr;
        }
        return FetchStorageExecutor(false, readIdleList, readUsingList, errCode, isExternal);
    }

    // Not prohibited and there is an available handle
    bool result = readCondition_.wait_for(lock, std::chrono::seconds(waitTime),
        [this, &perm, &readUsingList, &readIdleList]() {
            return (perm_ == OperatePerm::NORMAL_PERM || perm_ == perm) &&
                (!readIdleList.empty() || (readIdleList.size() + readUsingList.size() < engineAttr_.maxReadNum) ||
                operateAbort_);
        });
    if (operateAbort_) {
        LOGI("Abort find read executor and busy for operate!");
        return nullptr;
    }
    if (!result) {
        LOGI("Get read handle result[%d], permissType[%u], operType[%u], read[%zu-%zu-%" PRIu32 "]", result,
            static_cast<unsigned>(perm_), static_cast<unsigned>(perm), readIdleList.size(), readUsingList.size(),
            engineAttr_.maxReadNum);
        return nullptr;
    }
    return FetchStorageExecutor(false, readIdleList, readUsingList, errCode, isExternal);
}

void StorageEngine::Recycle(StorageExecutor *&handle, bool isExternal)
{
    if (handle == nullptr) {
        return;
    }
    if (!isEnhance_) {
        LOGD("Recycle executor[%d] for id[%.6s]", handle->GetWritable(), hashIdentifier_.c_str());
    }
    std::list<StorageExecutor *> &writeUsingList = isExternal ? externalWriteUsingList_ : writeUsingList_;
    std::list<StorageExecutor *> &writeIdleList = isExternal ?  externalWriteIdleList_ : writeIdleList_;
    std::list<StorageExecutor *> &readUsingList = isExternal ? externalReadUsingList_ : readUsingList_;
    std::list<StorageExecutor *> &readIdleList = isExternal ?  externalReadIdleList_ : readIdleList_;
    if (handle->GetWritable()) {
        std::unique_lock<std::mutex> lock(writeMutex_);
        auto iter = std::find(writeUsingList.begin(), writeUsingList.end(), handle);
        if (iter != writeUsingList.end()) {
            writeUsingList.remove(handle);
            if (!writeIdleList.empty()) {
                delete handle;
                handle = nullptr;
                return;
            }
            handle->Reset();
            writeIdleList.push_back(handle);
            writeCondition_.notify_one();
            idleCondition_.notify_all();
        }
    } else {
        std::unique_lock<std::mutex> lock(readMutex_);
        auto iter = std::find(readUsingList.begin(), readUsingList.end(), handle);
        if (iter != readUsingList.end()) {
            readUsingList.remove(handle);
            if (!readIdleList.empty()) {
                delete handle;
                handle = nullptr;
                return;
            }
            handle->Reset();
            readIdleList.push_back(handle);
            readCondition_.notify_one();
        }
    }
    handle = nullptr;
}

void StorageEngine::ClearCorruptedFlag()
{
    return;
}

bool StorageEngine::IsEngineCorrupted() const
{
    return false;
}

void StorageEngine::Release()
{
    CloseExecutor();
    isInitialized_.store(false);
    isUpdated_ = false;
    ClearCorruptedFlag();
    SetEngineState(EngineState::INVALID);
}

int StorageEngine::TryToDisable(bool isNeedCheckAll, OperatePerm disableType)
{
    if (engineState_ != EngineState::MAINDB && engineState_ != EngineState::INVALID) {
        LOGE("Not support disable handle when cacheDB existed! state = [%d]", engineState_);
        return(engineState_ == EngineState::CACHEDB) ? -E_NOT_SUPPORT : -E_BUSY;
    }

    std::lock(writeMutex_, readMutex_);
    std::lock_guard<std::mutex> writeLock(writeMutex_, std::adopt_lock);
    std::lock_guard<std::mutex> readLock(readMutex_, std::adopt_lock);

    if (!isNeedCheckAll) {
        goto END;
    }

    if (!writeUsingList_.empty() || !readUsingList_.empty() || !externalWriteUsingList_.empty() ||
        !externalReadUsingList_.empty()) {
        LOGE("Database handle used");
        return -E_BUSY;
    }
END:
    if (perm_ == OperatePerm::NORMAL_PERM) {
        LOGI("database is disable for re-build:%d", static_cast<int>(disableType));
        perm_ = disableType;
        writeCondition_.notify_all();
        readCondition_.notify_all();
    }
    return E_OK;
}

void StorageEngine::Enable(OperatePerm enableType)
{
    std::lock(writeMutex_, readMutex_);
    std::lock_guard<std::mutex> writeLock(writeMutex_, std::adopt_lock);
    std::lock_guard<std::mutex> readLock(readMutex_, std::adopt_lock);
    if (perm_ == enableType) {
        LOGI("Re-enable the database");
        perm_ = OperatePerm::NORMAL_PERM;
        writeCondition_.notify_all();
        readCondition_.notify_all();
    }
}

void StorageEngine::Abort(OperatePerm enableType)
{
    std::lock(writeMutex_, readMutex_);
    std::lock_guard<std::mutex> writeLock(writeMutex_, std::adopt_lock);
    std::lock_guard<std::mutex> readLock(readMutex_, std::adopt_lock);
    if (perm_ == enableType) {
        LOGI("Abort the handle occupy, release all!");
        perm_ = OperatePerm::NORMAL_PERM;
        operateAbort_ = true;

        writeCondition_.notify_all();
        readCondition_.notify_all();
    }
}

bool StorageEngine::IsNeedTobeReleased() const
{
    EngineState engineState = GetEngineState();
    return ((engineState == EngineState::MAINDB) || (engineState == EngineState::INVALID));
}

const std::string &StorageEngine::GetIdentifier() const
{
    return identifier_;
}

EngineState StorageEngine::GetEngineState() const
{
    return engineState_;
}

void StorageEngine::SetEngineState(EngineState state)
{
    if (state != EngineState::MAINDB) {
        LOGI("Storage engine state to [%d]!", state);
    }
    engineState_ = state;
}

int StorageEngine::ExecuteMigrate()
{
    LOGW("Migration is not supported!");
    return -E_NOT_SUPPORT;
}

void StorageEngine::SetNotifiedCallback(const std::function<void(int, KvDBCommitNotifyFilterAbleData *)> &callback)
{
    std::unique_lock<std::shared_mutex> lock(notifyMutex_);
    commitNotifyFunc_ = callback;
}

void StorageEngine::SetConnectionFlag(bool isExisted)
{
    return isExistConnection_.store(isExisted);
}

bool StorageEngine::IsExistConnection() const
{
    return isExistConnection_.load();
}

int StorageEngine::CheckEngineOption(const KvDBProperties &kvdbOption) const
{
    return E_OK;
}

void StorageEngine::AddStorageExecutor(StorageExecutor *handle, bool isExternal)
{
    if (handle == nullptr) {
        return;
    }

    std::list<StorageExecutor *> &writeIdleList = isExternal ?  externalWriteIdleList_ : writeIdleList_;
    std::list<StorageExecutor *> &readIdleList = isExternal ?  externalReadIdleList_ : readIdleList_;
    if (handle->GetWritable()) {
        writeIdleList.push_back(handle);
    } else {
        readIdleList.push_back(handle);
    }
}

void ClearHandleList(std::list<StorageExecutor *> &handleList)
{
    for (auto &item : handleList) {
        if (item != nullptr) {
            delete item;
            item = nullptr;
        }
    }
    handleList.clear();
}

void StorageEngine::CloseExecutor()
{
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        ClearHandleList(writeIdleList_);
        ClearHandleList(externalWriteIdleList_);
    }

    {
        std::lock_guard<std::mutex> lock(readMutex_);
        ClearHandleList(readIdleList_);
        ClearHandleList(externalReadIdleList_);
    }
}

StorageExecutor *StorageEngine::FetchStorageExecutor(bool isWrite, std::list<StorageExecutor *> &idleList,
    std::list<StorageExecutor *> &usingList, int &errCode, bool isExternal)
{
    if (idleList.empty()) {
        StorageExecutor *handle = nullptr;
        errCode = CreateNewExecutor(isWrite, handle);
        if ((errCode != E_OK) || (handle == nullptr)) {
            if (errCode != -E_EKEYREVOKED) {
                return nullptr;
            }
            LOGE("Key revoked status, couldn't create the new executor");
            if (!usingList.empty()) {
                LOGE("Can't create new executor for revoked");
                errCode = -E_BUSY;
            }
            return nullptr;
        }

        AddStorageExecutor(handle, isExternal);
    }
    auto item = idleList.front();
    usingList.push_back(item);
    idleList.remove(item);
    if (!isEnhance_) {
        LOGD("Get executor[%d] from [%.3s]", isWrite, hashIdentifier_.c_str());
    }
    errCode = E_OK;
    return item;
}

bool StorageEngine::CheckEngineAttr(const StorageEngineAttr &poolSize)
{
    return (poolSize.maxReadNum > MAX_READ_SIZE ||
            poolSize.maxWriteNum > MAX_WRITE_SIZE ||
            poolSize.minReadNum > poolSize.maxReadNum ||
            poolSize.minWriteNum > poolSize.maxWriteNum);
}

bool StorageEngine::IsMigrating() const
{
    return isMigrating_.load();
}

void StorageEngine::WaitWriteHandleIdle()
{
    std::unique_lock<std::mutex> autoLock(idleMutex_);
    LOGD("Wait wHandle release id[%s]. write[%zu-%zu-%" PRIu32 "]", hashIdentifier_.c_str(),
        writeIdleList_.size(), writeUsingList_.size(), engineAttr_.maxWriteNum);
    idleCondition_.wait(autoLock, [this]() {
        return writeUsingList_.empty();
    });
    LOGD("Wait wHandle release finish id[%s]. write[%zu-%zu-%" PRIu32 "]",
        hashIdentifier_.c_str(), writeIdleList_.size(), writeUsingList_.size(), engineAttr_.maxWriteNum);
}

void StorageEngine::IncreaseCacheRecordVersion()
{
    return;
}

uint64_t StorageEngine::GetCacheRecordVersion() const
{
    return 0;
}

uint64_t StorageEngine::GetAndIncreaseCacheRecordVersion()
{
    return 0;
}

void StorageEngine::SetSchemaChangedCallback(const std::function<int(void)> &callback)
{
    std::unique_lock<std::shared_mutex> lock(schemaChangedMutex_);
    schemaChangedFunc_ = callback;
}
}
