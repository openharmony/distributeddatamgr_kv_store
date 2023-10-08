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

#ifndef SQLITE_RELATIONAL_UTILS_H
#define SQLITE_RELATIONAL_UTILS_H

#include <vector>
#include "sqlite_import.h"
#include "data_value.h"
#include "cloud/cloud_store_types.h"

namespace DistributedDB {
class SQLiteRelationalUtils {
public:
    static int GetDataValueByType(sqlite3_stmt *statement, int cid, DataValue &value);

    static std::vector<DataValue> GetSelectValues(sqlite3_stmt *stmt);

    static int GetCloudValueByType(sqlite3_stmt *statement, int type, int cid, Type &cloudValue);

    static void CalCloudValueLen(Type &cloudValue, uint32_t &totalSize);

    static int BindStatementByType(sqlite3_stmt *statement, int cid, Type &typeVal);

    static int StepNext(bool isMemDB, sqlite3_stmt *stmt);

    static int GetSelectVBucket(sqlite3_stmt *stmt, VBucket &bucket);

    static bool GetDbFileName(sqlite3 *db, std::string &fileName);
private:
    static int BindExtendStatementByType(sqlite3_stmt *statement, int cid, Type &typeVal);

    static int GetTypeValByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal);
    static int GetBlobByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal);
};
} // namespace DistributedDB
#endif // SQLITE_RELATIONAL_UTILS_H
