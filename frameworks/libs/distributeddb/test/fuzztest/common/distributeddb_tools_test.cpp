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

#include "distributeddb_tools_test.h"
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <openssl/rand.h>
#include <random>
#include <set>
#include <sys/types.h>

#include "db_common.h"
#include "db_constant.h"
#include "generic_single_ver_kv_entry.h"
#include "platform_specific.h"
#include "single_ver_data_packet.h"
#include "value_hash_calc.h"

using namespace DistributedDB;
namespace DistributedDBTest {
int DistributedDBToolsTest::GetCurrentDir(std::string &dir)
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

void DistributedDBToolsTest::TestDirInit(std::string &dir)
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

int DistributedDBToolsTest::RemoveTestDbFiles(const std::string &dir)
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

void DistributedDBToolsTest::GetRandomKeyValue(std::vector<uint8_t> &value, uint32_t defaultSize)
{
    uint32_t randSize = 0;
    if (defaultSize == 0) {
        uint8_t simSize = 0;
        RAND_bytes(&simSize, 1);
        randSize = (simSize == 0) ? 1 : simSize;
    } else {
        randSize = defaultSize;
    }

    value.resize(randSize);
    RAND_bytes(value.data(), randSize);
}

DBStatus DistributedDBToolsTest::SyncTestWithQuery(KvStoreNbDelegate* delegate,
    const std::vector<std::string>& devices, SyncMode mode,
    std::map<std::string, DBStatus>& statuses, const Query &query)
{
    std::mutex syncLock;
    std::condition_variable syncCondVar;
    statuses.clear();
    DBStatus callStatus = delegate->Sync(devices, mode,
        [&statuses, &syncLock, &syncCondVar](const std::map<std::string, DBStatus>& statusMap) {
            statuses = statusMap;
            std::unique_lock<std::mutex> innerlock(syncLock);
            syncCondVar.notify_one();
        }, query, false);
    std::unique_lock<std::mutex> lock(syncLock);
    syncCondVar.wait(lock, [callStatus, &statuses]() {
            if (callStatus != OK) {
                return true;
            }
            return !statuses.empty();
        });
    return callStatus;
}

DBStatus DistributedDBToolsTest::SyncTest(KvStoreNbDelegate* delegate, const std::vector<std::string>& devices,
    SyncMode mode, std::map<std::string, DBStatus>& statuses)
{
    std::mutex syncLock;
    std::condition_variable syncCondVar;
    statuses.clear();
    DBStatus callStatus = delegate->Sync(devices, mode,
        [&statuses,  &syncLock, &syncCondVar](const std::map<std::string, DBStatus>& statusMap) {
            statuses = statusMap;
            std::unique_lock<std::mutex> innerlock(syncLock);
            syncCondVar.notify_one();
        }, false);
    std::unique_lock<std::mutex> lock(syncLock);
    syncCondVar.wait(lock, [callStatus, &statuses]() {
            if (callStatus != OK) {
                return true;
            }
            return !statuses.empty();
        });
    return callStatus;
}

KvStoreObserverTest::KvStoreObserverTest() : callCount_(0), isCleared_(false)
{}

void KvStoreObserverTest::OnChange(const KvStoreChangedData& data)
{
    callCount_++;
    inserted_ = data.GetEntriesInserted();
    updated_ = data.GetEntriesUpdated();
    deleted_ = data.GetEntriesDeleted();
    isCleared_ = data.IsCleared();
    LOGD("Onchangedata :%zu -- %zu -- %zu -- %d", inserted_.size(), updated_.size(), deleted_.size(), isCleared_);
    LOGD("Onchange() called success!");
}

sqlite3 *RdbTestUtils::CreateDataBase(const std::string &dbUri)
{
    sqlite3 *db = nullptr;
    if (sqlite3_open_v2(dbUri.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        if (db != nullptr) {
            (void)sqlite3_close_v2(db);
            db = nullptr;
        }
    }
    return db;
}

int RdbTestUtils::ExecSql(sqlite3 *db, const std::string &sql)
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

int RdbTestUtils::CreateDeviceTable(sqlite3 *db, const std::string &table, const std::string &device)
{
    std::string deviceTable = DBCommon::GetDistributedTableName(device, table);
    TableInfo baseTbl;
    if (SQLiteUtils::AnalysisSchema(db, table, baseTbl) != E_OK) {
        return -1;
    }
    if (SQLiteUtils::CreateSameStuTable(db, baseTbl, deviceTable) != E_OK) {
        return -1;
    }
    if (SQLiteUtils::CloneIndexes(db, table, deviceTable) != E_OK) {
        return -1;
    }
    return 0;
}
}