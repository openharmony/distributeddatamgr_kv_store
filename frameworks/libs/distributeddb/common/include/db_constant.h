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

#ifndef DISTRIBUTEDDB_CONSTANT_H
#define DISTRIBUTEDDB_CONSTANT_H

#include <string>

namespace DistributedDB {
class DBConstant {
public:
    static constexpr const size_t MAX_KEY_SIZE = 1024;
    static constexpr const size_t MAX_VALUE_SIZE = 4194304;
    static constexpr const size_t MAX_BATCH_SIZE = 128;
    static constexpr const size_t MAX_DEV_LENGTH = 128;
    static constexpr const size_t MAX_TRANSACTION_KEY_VALUE_LENS = 512 * 1024 * 1024; // 512M

    static constexpr const size_t MAX_DATA_DIR_LENGTH = 512;

    static constexpr const size_t MAX_INKEYS_SIZE = 128;
    static constexpr const size_t MAX_SQL_ARGS_COUNT = 100;
    static constexpr const size_t MAX_IN_COUNT = 100;

    static constexpr const int DB_TYPE_LOCAL = 1;
    static constexpr const int DB_TYPE_MULTI_VER = 2;
    static constexpr const int DB_TYPE_SINGLE_VER = 3;

    static constexpr const int QUEUED_SYNC_LIMIT_DEFAULT = 32;
    static constexpr const int QUEUED_SYNC_LIMIT_MIN = 1;
    static constexpr const int QUEUED_SYNC_LIMIT_MAX = 4096;

    static constexpr const int MAX_DEVICES_SIZE = 100;
    static constexpr const int MAX_COMMIT_SIZE = 1000000;
    static constexpr const int MAX_ENTRIES_SIZE = 1000000;

    static constexpr const uint32_t MAX_COLUMN  = 32767;

    static constexpr const int MAX_REMOTEDATA_SIZE = 4194304;  // 4M.

    static constexpr const int DEFAULT_ITER_TIMES = 5000;

    // In querySync, when getting query data finished,
    // if the block size reach the half of max block size, will get deleted data next;
    // if the block size not reach the half of max block size, will not get deleted data.
    static constexpr const float QUERY_SYNC_THRESHOLD = 0.50;

    static constexpr const uint64_t MAX_USER_ID_LENGTH = 128;
    static constexpr const uint64_t MAX_APP_ID_LENGTH = 128;
    static constexpr const uint64_t MAX_STORE_ID_LENGTH = 128;
    static constexpr const uint64_t MAX_SUB_USER_LENGTH = 128;

    static const std::string MULTI_SUB_DIR;
    static const std::string SINGLE_SUB_DIR;
    static const std::string LOCAL_SUB_DIR;

    static const std::string MAINDB_DIR;
    static const std::string METADB_DIR;
    static const std::string CACHEDB_DIR;

    static constexpr const char *LOCAL_DATABASE_NAME = "local";
    static constexpr const char *MULTI_VER_DATA_STORE = "multi_ver_data";
    static constexpr const char *MULTI_VER_COMMIT_STORE = "commit_logs";
    static constexpr const char *MULTI_VER_VALUE_STORE = "value_storage";
    static constexpr const char *MULTI_VER_META_STORE = "meta_storage";
    static const std::string SINGLE_VER_DATA_STORE;
    static const std::string SINGLE_VER_META_STORE;
    static const std::string SINGLE_VER_CACHE_STORE;

    static constexpr const char *SQLITE_URL_PRE = "file:";
    static constexpr const char *DB_EXTENSION = ".db";
    static constexpr const char *SQLITE_MEMDB_IDENTIFY = "?mode=memory&cache=shared";

    static constexpr const char *SCHEMA_KEY = "schemaKey";
    static const std::string RELATIONAL_SCHEMA_KEY;
    static const std::string RELATIONAL_TRACKER_SCHEMA_KEY;

    static constexpr const char *RD_KV_COLLECTION_MODE = "{\"mode\" : \"kv\"}";
    static constexpr const char *RD_KV_HASH_COLLECTION_MODE = "{\"mode\" : \"kv\",\"indextype\" : \"hash\"}";

    static constexpr const char *PATH_POSTFIX_UNPACKED = "_unpacked";
    static constexpr const char *PATH_POSTFIX_IMPORT_BACKUP = "_import_bak";
    static constexpr const char *PATH_POSTFIX_IMPORT_ORIGIN = "_import_ori";
    static constexpr const char *PATH_POSTFIX_IMPORT_DUP = "_import_dup";
    static constexpr const char *PATH_POSTFIX_EXPORT_BACKUP = "_export_bak";
    // use for make sure create datebase and set label complete
    static constexpr const char *PATH_POSTFIX_DB_INCOMPLETE = "_db_incomplete.lock";

    static constexpr const char *REKEY_FILENAME_POSTFIX_PRE = "_ctrl_pre";
    static constexpr const char *REKEY_FILENAME_POSTFIX_OK = "_ctrl_ok";
    static constexpr const char *UPGRADE_POSTFIX = "_upgrade.lock";
    // used for make sure meta split upgrade atomically
    static constexpr const char *SET_SECOPT_POSTFIX = "_secopt.lock";
    static constexpr const char *PATH_BACKUP_POSTFIX = "_bak";

    static constexpr const char *ID_CONNECTOR = "-";

    static constexpr const char *DELETE_KVSTORE_REMOVING = "_removing";
    static constexpr const char *DB_LOCK_POSTFIX = ".lock";

    static constexpr const char *SUBSCRIBE_QUERY_PREFIX = "subscribe_query_";

    static constexpr const char *TRIGGER_REFERENCES_NEW = "NEW.";
    static constexpr const char *TRIGGER_REFERENCES_OLD = "OLD.";

    static const std::string UPDATE_META_FUNC;

    // Prefix Key in meta db
    static constexpr const char *DEVICEID_PREFIX_KEY = "deviceId";
    static constexpr const char *QUERY_SYNC_PREFIX_KEY = "querySync";
    static constexpr const char *DELETE_SYNC_PREFIX_KEY = "deleteSync";

    static constexpr const size_t MAX_NORMAL_PACK_ITEM_SIZE = 4000;
    // slide window mode to reduce last ack transfer time
    static constexpr const size_t MAX_HPMODE_PACK_ITEM_SIZE = 2000;

    static constexpr const uint32_t MIN_MTU_SIZE = 1024; // 1KB
    static constexpr const uint32_t MAX_MTU_SIZE = 5242880; // 5MB

    static constexpr const uint32_t MIN_TIMEOUT = 5000; // 5s
    static constexpr const uint32_t MAX_TIMEOUT = 60000; // 60s
    static constexpr const uint32_t MAX_SYNC_TIMEOUT = 300000; // 300s
    static constexpr const int INFINITE_WAIT = -1; // -1 is infinite waiting

    static constexpr const uint8_t DEFAULT_COMPTRESS_RATE = 100;

    static constexpr const size_t MAX_SYNC_BLOCK_SIZE = 31457280; // 30MB

    static constexpr const int DOUBLE_PRECISION = 15;
    static constexpr const int MAX_DISTRIBUTED_TABLE_COUNT = 32;

    static constexpr const uint64_t MAX_LOG_SIZE_HIGH = 0x400000000ULL; // 16GB
    static constexpr const uint64_t MAX_LOG_SIZE_LOW = 0x400000ULL; // 4MB
    static constexpr const uint64_t MAX_LOG_SIZE_DEFAULT = 0x40000000ULL; // 1GB

    static constexpr const int DEF_LIFE_CYCLE_TIME = 60000; // 60S

    static constexpr const int RELATIONAL_LOG_TABLE_FIELD_NUM = 7; // field num is relational distributed log table

    static constexpr const uint64_t IGNORE_CONNECTION_ID = 0;
    // Soft limit of a connection observer count.
    static constexpr const int MAX_OBSERVER_COUNT = 8;

    // For relational
    static const std::string SYSTEM_TABLE_PREFIX;
    static const std::string RELATIONAL_PREFIX;
    static const std::string TIMESTAMP_ALIAS;
    static constexpr const char *LOG_POSTFIX = "_log";
    static constexpr const char *META_TABLE_POSTFIX = "metadata";

    static constexpr const char *LOG_TABLE_VERSION_3 = "3.0";
    static constexpr const char *LOG_TABLE_VERSION_5_1 = "5.01";
    static constexpr const char *LOG_TABLE_VERSION_5_3 = "5.03"; // add sharing_resource field
    static constexpr const char *LOG_TABLE_VERSION_5_5 = "5.05"; // add status field
    static constexpr const char *LOG_TABLE_VERSION_5_8 = "5.08"; // migrate cursor to meta table
    static constexpr const char *LOG_TABLE_VERSION_5_9 = "5.09"; // insert retains the old version
    static constexpr const char *LOG_TABLE_VERSION_5_10 = "5.10"; // increase the cursor
    static constexpr const char *LOG_TABLE_VERSION_CURRENT = LOG_TABLE_VERSION_5_10;

    static const std::string LOG_TABLE_VERSION_KEY;

    static constexpr const char *REMOTE_DEVICE_SCHEMA_KEY_PREFIX = "remote_device_schema_";

    static constexpr const uint32_t MAX_CONDITION_KEY_LEN = 128;
    static constexpr const uint32_t MAX_CONDITION_VALUE_LEN = 128;
    static constexpr const uint32_t MAX_CONDITION_COUNT = 32;

    static constexpr const uint32_t REMOTE_QUERY_MAX_SQL_LEN = 1000000U;

    static constexpr const int HASH_KEY_SIZE = 32; // size of SHA256_DIGEST_LENGTH

    static constexpr const char *TABLE_IS_DROPPED = "table_is_dropped_";

    static constexpr const char *SQLITE_INNER_ROWID = "_rowid_";
    static constexpr const int32_t DEFAULT_ROW_ID = -1;
    static constexpr const int STR_TO_LL_BY_DEVALUE = 10;
    // key in meta_data
    static constexpr const char *LOCALTIME_OFFSET_KEY = "localTimeOffset";

    static constexpr const uint64_t OBSERVER_CHANGES_MASK = 0XF00;

    static constexpr const char *STORAGE_TYPE_LONG = "long";

    static constexpr const char *KV_SYNC_TABLE_NAME = "sync_data";
    static constexpr const char *KV_LOCAL_TABLE_NAME = "local_data";
};
} // namespace DistributedDB
#endif // DISTRIBUTEDDB_CONSTANT_H
