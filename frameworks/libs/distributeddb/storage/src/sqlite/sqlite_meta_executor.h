/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SQLITE_META_EXECUTOR_H
#define SQLITE_META_EXECUTOR_H

#include "sqlite_utils.h"

namespace DistributedDB {
    class SqliteMetaExecutor {
    public:
        enum class MetaMode {
            KV = 0,
            KV_ATTACH = 1,
            RDB = 2
        };
        static int GetMetaKeysByKeyPrefix(const std::string &keyPre, sqlite3 *dbHandle, MetaMode metaMode, bool isMemDb,
                                          std::set<std::string> &outKeys);

        static int GetAllKeys(sqlite3_stmt *statement, bool isMemDb, std::vector<Key> &keys);

        static int GetExistsDevicesFromMeta(sqlite3 *dbHandle, MetaMode metaMode,
                                            bool isMemDb, std::set<std::string> &devices);
    private:
        static constexpr const char *SELECT_ATTACH_META_KEYS_BY_PREFIX =
                "SELECT key FROM meta.meta_data where key like ?;";

        static constexpr const char *SELECT_META_KEYS_BY_PREFIX =
                "SELECT key FROM meta_data where key like ?;";

        static constexpr const char *SELECT_RDB_META_KEYS_BY_PREFIX =
                "SELECT key FROM naturalbase_rdb_aux_metadata where key like ?;";
    };
} // DistributedDB
#endif // SQLITE_META_EXECUTOR_H