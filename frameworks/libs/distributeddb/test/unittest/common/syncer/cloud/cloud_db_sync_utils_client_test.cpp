/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "cloud_db_sync_utils_test.h"

namespace DistributedDB {
    void CloudDBSyncUtilsTest::GetHashKey(const std::string &tableName, const std::string &condition, sqlite3 *db,
        std::vector<std::vector<uint8_t>> &hashKey)
    {
        sqlite3_stmt *stmt = nullptr;
        std::string sql = "select hash_key from " + DBCommon::GetLogTableName(tableName) + " where " + condition;
        EXPECT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::vector<uint8_t> blob;
            EXPECT_EQ(SQLiteUtils::GetColumnBlobValue(stmt, 0, blob), E_OK);
            hashKey.push_back(blob);
        }
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
}
