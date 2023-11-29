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

#ifndef STORAGE_EXECUTOR_H
#define STORAGE_EXECUTOR_H

#include "macro_utils.h"
#include "single_ver_natural_store_commit_notify_data.h"
#include "types_export.h"

namespace DistributedDB {
enum class EngineState {
    INVALID = -1, // default value, representative database is not generated
    CACHEDB,
    ATTACHING, // main db and cache db attach together
    MIGRATING, // began to Migrate data
    MAINDB,
    ENGINE_BUSY, // In order to change handle during the migration process, it is temporarily unavailable
};

enum class SingleVerDataType {
    META_TYPE,
    LOCAL_TYPE_SQLITE,
    SYNC_TYPE,
};

enum class DataStatus {
    NOEXISTED,
    DELETED,
    EXISTED,
};

enum class ExecutorState {
    INVALID = -1,
    MAINDB,
    CACHEDB,
    MAIN_ATTACH_CACHE, // After process crash and cacheDb existed
    CACHE_ATTACH_MAIN, // while cacheDb migrating to mainDb
};

struct DataOperStatus {
    DataStatus preStatus = DataStatus::NOEXISTED;
    bool isDeleted = false;
    bool isDefeated = false; // whether the put data is defeated.
};

struct SingleVerRecord {
    Key key;
    Value value;
    Timestamp timestamp = 0;
    uint64_t flag = 0;
    std::string device;
    std::string origDevice;
    Key hashKey;
    Timestamp writeTimestamp = 0;
};

struct DeviceInfo {
    bool isLocal = false;
    std::string deviceName;
};

struct LocalDataItem {
    Key key;
    Value value;
    Timestamp timestamp = 0;
    Key hashKey;
    uint64_t flag = 0;
};

struct NotifyConflictAndObserverData {
    SingleVerNaturalStoreCommitNotifyData *committedData = nullptr;
    DataItem getData;
    Key hashKey;
    DataOperStatus dataStatus;
};

struct NotifyMigrateSyncData {
    bool isRemote = false;
    bool isRemoveDeviceData = false;
    bool isPermitForceWrite = true;
    SingleVerNaturalStoreCommitNotifyData *committedData = nullptr;
    std::vector<Entry> entries{};
};

struct SyncDataDevices {
    std::string origDev;
    std::string dev;
};

class StorageExecutor {
public:
    explicit StorageExecutor(bool writable);
    virtual ~StorageExecutor();

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(StorageExecutor);

    virtual bool GetWritable() const;

    virtual int CheckCorruptedStatus(int errCode) const;

    virtual bool GetCorruptedStatus() const;

    virtual void SetCorruptedStatus() const;

    virtual int Reset() = 0;

protected:
    bool writable_;
    mutable bool isCorrupted_;
};
} // namespace DistributedDB

#endif // STORAGE_EXECUTOR_H
