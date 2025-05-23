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

#include "sqlite_meta_executor.h"

#include "db_common.h"
#include "db_constant.h"
namespace DistributedDB {
int SqliteMetaExecutor::GetMetaKeysByKeyPrefix(const std::string &keyPre, sqlite3 *dbHandle, MetaMode metaMode,
    bool isMemDb, std::set<std::string> &outKeys)
{
    sqlite3_stmt *statement = nullptr;
    std::string sqlStr;
    switch (metaMode) {
        case MetaMode::KV:
            sqlStr = SELECT_META_KEYS_BY_PREFIX;
            break;
        case MetaMode::KV_ATTACH:
            sqlStr = SELECT_ATTACH_META_KEYS_BY_PREFIX;
            break;
        case MetaMode::RDB:
            sqlStr = SELECT_RDB_META_KEYS_BY_PREFIX;
            break;
        default:
            return -E_INVALID_ARGS;
    }
    int errCode = SQLiteUtils::GetStatement(dbHandle, sqlStr, statement);
    if (errCode != E_OK) {
        LOGE("[SqliteMetaExecutor] Get statement failed:%d", errCode);
        return errCode;
    }

    Key keyPrefix;
    DBCommon::StringToVector(keyPre + '%', keyPrefix);
    int ret = E_OK;
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, keyPrefix); // 1: bind index for prefix key
    if (errCode != E_OK) {
        LOGE("[SqliteMetaExecutor] Bind statement failed:%d", errCode);
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }

    std::vector<Key> keys;
    errCode = GetAllKeys(statement, isMemDb, keys);
    SQLiteUtils::ResetStatement(statement, true, ret);
    for (const auto &it : keys) {
        if (it.size() >= keyPre.size() + DBConstant::HASH_KEY_SIZE) {
            outKeys.insert({it.begin() + keyPre.size(), it.begin() + keyPre.size() + DBConstant::HASH_KEY_SIZE});
        } else {
            LOGW("[SqliteMetaExecutor] Get invalid key, size=%zu", it.size());
        }
    }
    return errCode != E_OK ? errCode : ret;
}

int SqliteMetaExecutor::GetAllKeys(sqlite3_stmt *statement, bool isMemDb, std::vector<Key> &keys)
{
    if (statement == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode;
    do {
        errCode = SQLiteUtils::StepWithRetry(statement, isMemDb);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            Key key;
            errCode = SQLiteUtils::GetColumnBlobValue(statement, 0, key);
            if (errCode != E_OK) {
                break;
            }

            keys.push_back(std::move(key));
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else {
            LOGE("SQLite step for getting all keys failed:%d", errCode);
            break;
        }
    } while (true);
    return errCode;
}

int SqliteMetaExecutor::GetExistsDevicesFromMeta(sqlite3 *dbHandle, MetaMode metaMode,
    bool isMemDb, std::set<std::string> &devices)
{
    int errCode = GetMetaKeysByKeyPrefix(DBConstant::DEVICEID_PREFIX_KEY, dbHandle, metaMode, isMemDb, devices);
    if (errCode != E_OK) {
        LOGE("Get meta data key failed. err=%d", errCode);
        return errCode;
    }
    errCode = GetMetaKeysByKeyPrefix(DBConstant::QUERY_SYNC_PREFIX_KEY, dbHandle, metaMode, isMemDb, devices);
    if (errCode != E_OK) {
        LOGE("Get meta data key failed. err=%d", errCode);
        return errCode;
    }
    errCode = GetMetaKeysByKeyPrefix(DBConstant::DELETE_SYNC_PREFIX_KEY, dbHandle, metaMode, isMemDb, devices);
    if (errCode != E_OK) {
        LOGE("Get meta data key failed. err=%d", errCode);
    }
    return errCode;
}

int SqliteMetaExecutor::GetMetaDataByPrefixKey(sqlite3 *dbHandle, bool isMemDb, const std::string &metaTableName,
    const Key &keyPrefix, std::map<Key, Value> &data)
{
    std::string sql = "SELECT key,value FROM " + metaTableName + " WHERE key >= ? AND key <= ?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle, sql, statement);
    if (errCode != E_OK) {
        LOGE("[SqliteMetaExecutor][GetMetaDataByPrefixKey] Get statement failed:%d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::BindPrefixKey(statement, 1, keyPrefix); // 1 is first arg.
    if (errCode != E_OK) {
        LOGE("[SqliteMetaExecutor][GetMetaDataByPrefixKey] Bind prefix key failed:%d", errCode);
        return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
    }
    do {
        errCode = SQLiteUtils::StepWithRetry(statement, isMemDb);
        if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            break;
        }
        Key key;
        errCode = SQLiteUtils::GetColumnBlobValue(statement, 0, key); // 0 is key.
        if (errCode != E_OK) {
            LOGE("[SqliteMetaExecutor][GetMetaDataByPrefixKey] Get key failed:%d", errCode);
            return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
        }
        Value value;
        errCode = SQLiteUtils::GetColumnBlobValue(statement, 1, value); // 1 is value.
        if (errCode != E_OK) {
            LOGE("[SqliteMetaExecutor][GetMetaDataByPrefixKey] Get value failed:%d", errCode);
            return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
        }
        data[key] = value;
    } while (errCode == E_OK);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    return SQLiteUtils::ProcessStatementErrCode(statement, true, errCode);
}
}