/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "native_sqlite.h"

namespace DistributedDB {
sqlite3 *NativeSqlite::CreateDataBase(const std::string &dbUri)
{
    LOGD("Create database: %s", dbUri.c_str());
    sqlite3 *db = nullptr;
    int r = sqlite3_open_v2(dbUri.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (r != SQLITE_OK) {
        LOGE("Open database [%s] failed. %d", dbUri.c_str(), r);
        if (db != nullptr) {
            (void)sqlite3_close_v2(db);
            db = nullptr;
        }
    }
    return db;
}

int NativeSqlite::ExecSql(sqlite3 *db, const std::string &sql)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK && errMsg != nullptr) {
        LOGE("Execute sql failed. %d err: %s", errCode, errMsg);
    }
    sqlite3_free(errMsg);
    return errCode;
}

int NativeSqlite::ExecSql(sqlite3 *db, const std::string &sql, const std::function<int (sqlite3_stmt *)> &bindCallback,
    const std::function<int (sqlite3_stmt *)> &resultCallback)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }

    bool bindFinish = true;
    sqlite3_stmt *stmt = nullptr;
    int ret = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK) {
        goto END;
    }

    do {
        if (bindCallback) {
            ret = bindCallback(stmt);
            if (ret != E_OK && ret != -E_UNFINISHED) {
                goto END;
            }
            bindFinish = (ret != -E_UNFINISHED);
        }

        while (true) {
            ret = sqlite3_step(stmt);
            if (ret == SQLITE_DONE) {
                ret = E_OK; // step finished
                break;
            } else if (ret != SQLITE_ROW) {
                goto END; // step return error
            }
            if (resultCallback == nullptr) {
                continue;
            }
            ret = resultCallback(stmt);
            if (ret != E_OK) {
                goto END;
            }
            // continue step stmt while callback return E_OK
        }
        (void)sqlite3_reset(stmt);
    } while (!bindFinish);

END:
    (void)sqlite3_finalize(stmt);
    return ret;
}
}