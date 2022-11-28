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

#ifndef NATIVE_SQLITE_H
#define NATIVE_SQLITE_H

#include <string>
#include <functional>

#include "db_errno.h"
#include "log_print.h"
#include "sqlite_import.h"

namespace DistributedDB {
// For call sqlite native interfaces
class NativeSqlite {
public:
    static sqlite3 *CreateDataBase(const std::string &dbUri);
    static int ExecSql(sqlite3 *db, const std::string &sql);
private:
    NativeSqlite();
    ~NativeSqlite();
};
}

#endif // NATIVE_SQLITE_H