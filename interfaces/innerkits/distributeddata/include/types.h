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

#ifndef DISTRIBUTED_KVSTORE_TYPES_H
#define DISTRIBUTED_KVSTORE_TYPES_H

#include <algorithm>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include "blob.h"
#include "store_errno.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
/**
 * @brief Key set by client, can be any non-empty bytes array, and less than 1024 size.
*/
using Key = OHOS::DistributedKv::Blob;

/**
 * @brief Value set by client, can be any bytes array.
*/
using Value = OHOS::DistributedKv::Blob;

/**
 * @brief User identifier from user-account.
*/
struct UserId {
    std::string userId;
};

/**
 * @brief App identifier from bms.
*/
struct API_EXPORT AppId {
    std::string appId;

    /**
     * @brief Support appId convert to std::string.
    */
    operator std::string &() noexcept
    {
        return appId;
    }

    /**
     * @brief Support appId convert to const std::string.
    */
    operator const std::string &() const noexcept
    {
        return appId;
    }

    /**
     * @brief Check appId.
     */
    inline bool IsValid() const
    {
        if (appId.empty() || appId.size() > MAX_APP_ID_LEN) {
            return false;
        }
        int count = 0;
        auto iter = std::find_if_not(appId.begin(), appId.end(),
            [&count](char c) {
            count = (c == SEPARATOR_CHAR) ? (count + 1) : (count >= SEPARATOR_COUNT ? count : 0);
            return (std::isprint(c) && c != '/');
        });

        return (iter == appId.end()) && (count < SEPARATOR_COUNT);
    }
private:
    static constexpr int MAX_APP_ID_LEN = 256;
    static constexpr int SEPARATOR_COUNT = 3;
    static constexpr char SEPARATOR_CHAR = '#';
};

/**
 * @brief Kvstore name set by client by calling GetKvStore.
 *
 * storeId length must be less or equal than 256,
 * and can not be empty and all space.
*/
struct API_EXPORT StoreId {
    std::string storeId;

    /**
     * @brief Support storeId convert to std::string.
    */
    operator std::string &() noexcept
    {
        return storeId;
    }

    /**
     * @brief Support storeId convert to const std::string.
    */
    operator const std::string &() const noexcept
    {
        return storeId;
    }

    /**
     * @brief Operator <.
     */
    bool operator<(const StoreId &id) const noexcept
    {
        return this->storeId < id.storeId;
    }

    /**
     * @brief Check storeId.
     */
    inline bool IsValid() const
    {
        if (storeId.empty() || storeId.size() > MAX_STORE_ID_LEN) {
            return false;
        }
        auto iter = std::find_if_not(storeId.begin(), storeId.end(),
            [](char c) { return (std::isdigit(c) || std::isalpha(c) || c == '_'); });
        return (iter == storeId.end());
    }
private:
    static constexpr int MAX_STORE_ID_LEN = 128;
};

/**
 * @brief Identifier unique database.
*/
struct KvStoreTuple {
    std::string userId;
    std::string appId;
    std::string storeId;
};

/**
 * @brief App thread information.
*/
struct AppThreadInfo {
    std::int32_t pid;
    std::int32_t uid;
};

/**
 * @brief The type for observer database change.
*/
enum SubscribeType : uint32_t {
    /**
     * Local changes of syncable kv store.
    */
    SUBSCRIBE_TYPE_LOCAL = 1,
    /**
     * Synced data changes from remote devices
    */
    SUBSCRIBE_TYPE_REMOTE = 2,
    /**
     * Synced data changes from remote devices
    */
    SUBSCRIBE_TYPE_CLOUD = 4,
    /**
     * Both local changes and synced data changes.
    */
    SUBSCRIBE_TYPE_ALL = 7,
};

struct DataOrigin {
    enum OriginType : int32_t {
        ORIGIN_NEARBY,
        ORIGIN_CLOUD,
        ORIGIN_ALL,
        ORIGIN_BUTT,
    };
    int32_t origin = ORIGIN_ALL;
    // origin is ORIGIN_NEARBY, the id is networkId;
    // origin is ORIGIN_CLOUD, the id is the cloud account id
    std::vector<std::string> id;
    std::string store;
};

/**
 * @brief Data is organized by entry definition.
*/
struct Entry {
    Key key;
    Value value;

    static constexpr size_t MAX_KEY_LENGTH = 1024;
    static constexpr size_t MAX_VALUE_LENGTH = 4 * 1024 * 1024;

    /**
     * Write blob size and data to memory buffer.
     * Return error when bufferLeftSize not enough.
    */
    bool WriteToBuffer(uint8_t *&cursorPtr, int &bufferLeftSize) const
    {
        return key.WriteToBuffer(cursorPtr, bufferLeftSize) && value.WriteToBuffer(cursorPtr, bufferLeftSize);
    }

    /**
     * Read a blob from memory buffer.
    */
    bool ReadFromBuffer(const uint8_t *&cursorPtr, int &bufferLeftSize)
    {
        return key.ReadFromBuffer(cursorPtr, bufferLeftSize) && value.ReadFromBuffer(cursorPtr, bufferLeftSize);
    }

    int RawSize() const
    {
        return key.RawSize() + value.RawSize();
    }
};

/**
 * @brief Indicate how to sync data on sync operation.
*/
enum SyncMode : int32_t {
    /**
     * Sync remote data to local.
    */
    PULL,
    /**
     * Sync local data to remote.
    */
    PUSH,
    /**
     * Both push and pull.
    */
    PUSH_PULL,
};

/**
 * @brief The kvstore type.
*/
enum KvStoreType : int32_t {
    /**
     * Multi-device collaboration.
     * The data is managed by the dimension of the device,
     * and there is no conflict.
     * Support to query data according to the dimension of equipment.
    */
    DEVICE_COLLABORATION,
    /**
     * Data is not divided into devices.
     * Modifying the same key between devices will overwrite.
    */
    SINGLE_VERSION,
    /**
     * Not support type.
    */
    MULTI_VERSION,
    LOCAL_ONLY,
    INVALID_TYPE,
};

/**
 * @brief Enumeration of database security level.
*/
enum SecurityLevel : int32_t {
    INVALID_LABEL = -1,
    NO_LABEL,
    S0,
    S1,
    S2,
    S3_EX,
    S3,
    S4,
};

/**
 * @brief Enumeration of database base directory.
*/
enum Area : int32_t {
    EL0,
    EL1,
    EL2,
    EL3,
    EL4
};

enum KvControlCmd : int32_t {
    SET_SYNC_PARAM = 1,
    GET_SYNC_PARAM,
};

using KvParam = OHOS::DistributedKv::Blob;

struct KvSyncParam {
    uint32_t allowedDelayMs { 0 };
};

/**
 * @brief Device basic information.
 *
 * Including device id, name and type.
*/
struct DeviceInfo {
    std::string deviceId;
    std::string deviceName;
    std::string deviceType;
};

/**
 * @brief Device filter strategy.
*/
enum class DeviceFilterStrategy {
    FILTER = 0,
    NO_FILTER = 1,
};

/**
 * @brief Indicate how and when to sync data with other device.
*/
enum PolicyType : uint32_t {
    /**
     * Data synchronization within valid time.
    */
    TERM_OF_SYNC_VALIDITY,
    /**
     * Data synchronization when device manager module call online operation.
    */
    IMMEDIATE_SYNC_ON_ONLINE,
    /**
     * Data synchronization when put, delete or update database.
    */
    IMMEDIATE_SYNC_ON_CHANGE,
    /**
     * Data synchronization when device manager module call onready operation.
    */
    IMMEDIATE_SYNC_ON_READY,
    POLICY_BUTT
};

/**
 * @brief Policy Type value.
*/
struct SyncPolicy {
    uint32_t type;
    std::variant<std::monostate, uint32_t> value;
};

/**
 * @brief Role Type value.
*/
enum RoleType : uint32_t {
    /**
      * The user has administrative rights.
    */
    OWNER = 0,
    /**
      * The user has read-only permission.
    */
    VISITOR,
};

struct Group {
    std::string groupDir = "";
    std::string groupId = "";
};

enum IndexType : uint32_t {
    /**
      * use btree index type in database
    */
    BTREE = 0,
    /**
      * use hash index type in database
    */
    HASH,
};

/**
 * @brief Data type, that determined the way and timing of data synchronization.
*/
enum DataType : uint32_t {
    /**
      * TYPE_STATICS: means synchronize on link establishment or device online.
    */
    TYPE_STATICS = 0,

    /**
      * TYPE_DYNAMICAL: means synchronize on link establishment.
      * synchronize can also triggered by the sync and async get interface.
    */
    TYPE_DYNAMICAL,
};

/**
 * @brief Provide configuration information for database creation.
*/
struct Options {
    /**
     * Whether to create a database when the database file does not exist.
     * It is created by default.
    */
    bool createIfMissing = true;
    /**
     * Set whether the database file is encrypted.
     * It is not encrypted by default.
    */
    bool encrypt = false;
    /**
     * Data storage disk by default.
    */
    bool persistent = true;
    /**
     * Set whether the database file is backed up.
     * It is back up by default.
    */
    bool backup = true;
    /**
     * Set whether the database file is automatically synchronized.
     * It is not automatically synchronized by default.
     * 'ohos.permission.DISTRIBUTED_DATASYNC' permission is necessary.
     * AutoSync do not guarantee real-time consistency, sync interface is suggested if necessary.
    */
    bool autoSync = false;
    /**
     * True if is distributed store, false if is local store
    */
    bool syncable = true;
    /**
     * Set Whether rebuild the database.
    */
    bool rebuild = false;
    /**
     * Set Whether the database is public.
    */
    bool isPublic = false;
    /**
     * Set database security level.
    */
    int32_t securityLevel = INVALID_LABEL;
    /**
     * Set database directory area.
    */
    int32_t area = EL1;
    /**
     * Set the type of database to be created.
     * The default is multi-device collaboration database.
    */
    KvStoreType kvStoreType = DEVICE_COLLABORATION;
    /**
     * The sync policy vector.
    */
    std::vector<SyncPolicy> policies{ { IMMEDIATE_SYNC_ON_CHANGE } };
    /**
     * Set the value stored in the database.
     * Schema is not used by default.
    */
    std::string schema = "";
    /**
     * Set database directory hapName.
    */
    std::string hapName = "";
    /**
     * Set database directory baseDir.
    */
    std::string baseDir = "";
    /**
     * Whether the kvstore type is valid.
    */
    inline bool IsValidType() const
    {
        bool isValid = kvStoreType == KvStoreType::DEVICE_COLLABORATION ||
            kvStoreType == KvStoreType::SINGLE_VERSION || kvStoreType == KvStoreType::LOCAL_ONLY;
        return isValid && (dataType == DataType::TYPE_STATICS || dataType == DataType::TYPE_DYNAMICAL);
    }
    /**
     * Get the databaseDir.
    */
    inline std::string GetDatabaseDir() const
    {
        if (baseDir.empty()) {
            return group.groupDir;
        }
        return !group.groupDir.empty() ? "" : baseDir;
    }
    /**
     * Whether the databaseDir is valid.
    */
    inline bool IsPathValid() const
    {
        if ((baseDir.empty() && group.groupDir.empty()) || (!baseDir.empty() && !group.groupDir.empty())) {
            return false;
        }
        return true;
    }
    Group group;
    /**
     * Set database role.
    */
    RoleType role;
    /**
     * Whether the sync happend in client.
    */
    bool isClientSync = false;
    /**
     * Whether the sync need compress.
    */
    bool isNeedCompress = true;
    /**
     * Indicates data type.
     * Only dynamic data support auto sync.
    */
    DataType dataType = DataType::TYPE_DYNAMICAL;
    /**
     * config database details.
    */
    struct Config {
        IndexType type = BTREE;
        uint32_t pageSize = 32u;
        uint32_t cacheSize = 2048u;
    } config;
};

/**
 * @brief Provide the user information.
*/
struct UserInfo {
    /**
     * The userId Info.
    */
    std::string userId;

    /**
     * The userType Info.
    */
    int32_t userType;
};

/**
 * @brief Cloud sync progress status.
*/
enum Progress {
    SYNC_BEGIN = 0,
    SYNC_IN_PROGRESS,
    SYNC_FINISH,
};

/**
 * @brief Cloud sync statistic.
*/
struct Statistic {
    uint32_t total;
    uint32_t success;
    uint32_t failed;
    uint32_t untreated;
};

/**
 * @brief Cloud sync table detail.
*/
struct TableDetail {
    Statistic upload;
    Statistic download;
};

/**
 * @brief Cloud sync process detail.
*/
struct ProgressDetail {
    /**
     * Cloud sync progress status
    */
    int32_t progress;
    int32_t code;
    TableDetail details;
};

using AsyncDetail = std::function<void(ProgressDetail &&)>;

/**
 * @brief Provide the switch data.
*/
struct SwitchData {
    /**
     * The value of switch data, one bit represents a switch state.
    */
    uint32_t value;

    /**
     * The effective bit count from low bit to high bit, must be 8, 16 or 24, max is 24.
    */
    uint16_t length;
};

/**
 * @brief Switch data opertaion.
*/
enum SwitchState: uint32_t {
    /**
     * INSERT: means insert data.
    */
    INSERT = 0,

    /**
     * UPDATE: means update data.
    */
    UPDATE,

    /**
     * DELETE: means delete data.
    */
    DELETE,
};

/**
 * @brief Switch data notification for change.
*/
struct SwitchNotification {
    /**
     * Switch data.
    */
    SwitchData data;

    /**
     * The device networkId.
    */
    std::string deviceId;

    /**
     * Switch state.
    */
    SwitchState state = SwitchState::INSERT;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // DISTRIBUTED_KVSTORE_TYPES_H
