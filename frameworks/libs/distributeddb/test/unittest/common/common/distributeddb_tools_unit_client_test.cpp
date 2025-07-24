/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "distributeddb_tools_unit_test.h"
#include "platform_specific.h"

using namespace DistributedDB;

namespace DistributedDBUnitTest {
int DistributedDBToolsUnitTest::GetCurrentDir(std::string &dir)
{
    static const int maxFileLength = 1024;
    dir = "";
    char buffer[maxFileLength] = {0};
    int length = readlink("/proc/self/exe", buffer, maxFileLength);
    if (length < 0 || length >= maxFileLength) {
        LOGE("read directory err length:%d", length);
        return -E_LENGTH_ERROR;
    }
    LOGD("DIR = %s", buffer);
    dir = buffer;
    if (dir.rfind("/") == std::string::npos && dir.rfind("\\") == std::string::npos) {
        LOGE("current patch format err");
        return -E_INVALID_PATH;
    }

    if (dir.rfind("/") != std::string::npos) {
        dir.erase(dir.rfind("/") + 1);
    }
    return E_OK;
}

void DistributedDBToolsUnitTest::TestDirInit(std::string &dir)
{
    if (GetCurrentDir(dir) != E_OK) {
        dir = "/";
    }

    dir.append("testDbDir");
    DIR *dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        if (OS::MakeDBDirectory(dir) != 0) {
            LOGI("MakeDirectory err!");
            dir = "/";
            return;
        }
    } else {
        closedir(dirTmp);
    }
}

int DistributedDBToolsUnitTest::RemoveTestDbFiles(const std::string &dir)
{
    bool isExisted = OS::CheckPathExistence(dir);
    if (!isExisted) {
        return E_OK;
    }

    int nFile = 0;
    std::string dirName;
    struct dirent *direntPtr = nullptr;
    DIR *dirPtr = opendir(dir.c_str());
    if (dirPtr == nullptr) {
        LOGE("opendir error!");
        return -E_INVALID_PATH;
    }
    while (true) {
        direntPtr = readdir(dirPtr);
        // condition to exit the loop
        if (direntPtr == nullptr) {
            break;
        }
        // only remove all *.db files
        std::string str(direntPtr->d_name);
        if (str == "." || str == "..") {
            continue;
        }
        dirName.clear();
        dirName.append(dir).append("/").append(str);
        if (direntPtr->d_type == DT_DIR) {
            RemoveTestDbFiles(dirName);
            rmdir(dirName.c_str());
        } else if (remove(dirName.c_str()) != 0) {
            LOGI("remove file: %s failed!", dirName.c_str());
            continue;
        }
        nFile++;
    }
    closedir(dirPtr);
    LOGI("Total %d test db files are removed!", nFile);
    return 0;
}

sqlite3 *RelationalTestUtils::CreateDataBase(const std::string &dbUri)
{
    LOGD("Create database: %s", dbUri.c_str());
    sqlite3 *db = nullptr;
    if (int r = sqlite3_open_v2(dbUri.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        LOGE("Open database [%s] failed. %d", dbUri.c_str(), r);
        if (db != nullptr) {
            (void)sqlite3_close_v2(db);
            db = nullptr;
        }
    }
    return db;
}

int RelationalTestUtils::ExecSql(sqlite3 *db, const std::string &sql)
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

int RelationalTestUtils::ExecSql(sqlite3 *db, const std::string &sql,
    const std::function<int (sqlite3_stmt *)> &bindCallback, const std::function<int (sqlite3_stmt *)> &resultCallback)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }

    bool bindFinish = true;
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto END;
    }

    do {
        if (bindCallback) {
            errCode = bindCallback(stmt);
            if (errCode != E_OK && errCode != -E_UNFINISHED) {
                goto END;
            }
            bindFinish = (errCode != -E_UNFINISHED); // continue bind if unfinished
        }

        bool isStepFinished = false;
        while (!isStepFinished) {
            errCode = SQLiteUtils::StepWithRetry(stmt);
            if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
                errCode = E_OK; // Step finished
                isStepFinished = true;
                break;
            } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
                goto END; // Step return error
            }
            if (resultCallback == nullptr) {
                continue;
            }
            errCode = resultCallback(stmt);
            if (errCode != E_OK) {
                goto END;
            }
        }
        SQLiteUtils::ResetStatement(stmt, false, errCode);
    } while (!bindFinish);

END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}
} // namespace DistributedDBUnitTest
