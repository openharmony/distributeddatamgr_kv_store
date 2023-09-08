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
#include <mutex>
#include <openssl/sha.h>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

#include "relational_store_client.h"

// using the "sqlite3sym.h" in OHOS
#ifndef USE_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif

namespace {
constexpr int E_OK = 0;
constexpr int E_ERROR = 1;
constexpr int BUSY_TIMEOUT = 2000;  // 2s.
const int MAX_BLOB_READ_SIZE = 5 * 1024 * 1024; // 5M limit
const std::string DEVICE_TYPE = "device";
const std::string SYNC_TABLE_TYPE = "sync_table_type_";
class ValueHashCalc {
public:
    ValueHashCalc() {};
    ~ValueHashCalc()
    {
        delete context_;
        context_ = nullptr;
    }

    int Initialize()
    {
        context_ = new (std::nothrow) SHA256_CTX;
        if (context_ == nullptr) {
            return -E_ERROR;
        }

        int errCode = SHA256_Init(context_);
        if (errCode == 0) {
            return -E_ERROR;
        }
        return E_OK;
    }

    int Update(const std::vector<uint8_t> &value)
    {
        if (context_ == nullptr) {
            return -E_ERROR;
        }
        int errCode = SHA256_Update(context_, value.data(), value.size());
        if (errCode == 0) {
            return -E_ERROR;
        }
        return E_OK;
    }

    int GetResult(std::vector<uint8_t> &value)
    {
        if (context_ == nullptr) {
            return -E_ERROR;
        }

        value.resize(SHA256_DIGEST_LENGTH);
        int errCode = SHA256_Final(value.data(), context_);
        if (errCode == 0) {
            return -E_ERROR;
        }

        return E_OK;
    }

private:
    SHA256_CTX *context_ = nullptr;
};

const uint64_t MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS = 1000000;

using Timestamp = uint64_t;
using TimeOffset = int64_t;

class TimeHelper {
public:
    constexpr static int64_t BASE_OFFSET = 10000LL * 365LL * 24LL * 3600LL * 1000LL * 1000LL * 10L; // 10000 year 100ns

    constexpr static int64_t MAX_VALID_TIME = BASE_OFFSET * 2; // 20000 year 100ns

    constexpr static uint64_t TO_100_NS = 10; // 1us to 100ns

    constexpr static Timestamp INVALID_TIMESTAMP = 0;

    // Get current system time
    static Timestamp GetSysCurrentTime()
    {
        uint64_t curTime = 0;
        int errCode = GetCurrentSysTimeInMicrosecond(curTime);
        if (errCode != E_OK) {
            return INVALID_TIMESTAMP;
        }

        std::lock_guard<std::mutex> lock(systemTimeLock_);
        // If GetSysCurrentTime in 1us, we need increase the currentIncCount_
        if (curTime == lastSystemTimeUs_) {
            // if the currentIncCount_ has been increased MAX_INC_COUNT, keep the currentIncCount_
            if (currentIncCount_ < MAX_INC_COUNT) {
                currentIncCount_++;
            }
        } else {
            lastSystemTimeUs_ = curTime;
            currentIncCount_ = 0;
        }
        return (curTime * TO_100_NS) + currentIncCount_; // Currently Timestamp is uint64_t
    }

    static int GetSysCurrentRawTime(uint64_t &curTime)
    {
        int errCode = GetCurrentSysTimeInMicrosecond(curTime);
        if (errCode != E_OK) {
            return errCode;
        }
        curTime *= TO_100_NS;
        return E_OK;
    }

    // Init the TimeHelper
    static void Initialize(Timestamp maxTimestamp)
    {
        std::lock_guard<std::mutex> lock(lastLocalTimeLock_);
        if (lastLocalTime_ < maxTimestamp) {
            lastLocalTime_ = maxTimestamp;
        }
    }

    static Timestamp GetTime(TimeOffset timeOffset)
    {
        Timestamp currentSysTime = GetSysCurrentTime();
        Timestamp currentLocalTime = currentSysTime + timeOffset;
        std::lock_guard<std::mutex> lock(lastLocalTimeLock_);
        if (currentLocalTime <= lastLocalTime_ || currentLocalTime > MAX_VALID_TIME) {
            lastLocalTime_++;
            currentLocalTime = lastLocalTime_;
        } else {
            lastLocalTime_ = currentLocalTime;
        }
        return currentLocalTime;
    }

private:
    static int GetCurrentSysTimeInMicrosecond(uint64_t &outTime)
    {
        struct timeval rawTime;
        int errCode = gettimeofday(&rawTime, nullptr);
        if (errCode < 0) {
            return -E_ERROR;
        }
        outTime = static_cast<uint64_t>(rawTime.tv_sec) * MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS +
            static_cast<uint64_t>(rawTime.tv_usec);
        return E_OK;
    }

    static std::mutex systemTimeLock_;
    static Timestamp lastSystemTimeUs_;
    static Timestamp currentIncCount_;
    static const uint64_t MAX_INC_COUNT = 9; // last bit from 0-9

    static Timestamp lastLocalTime_;
    static std::mutex lastLocalTimeLock_;
};

std::mutex TimeHelper::systemTimeLock_;
Timestamp TimeHelper::lastSystemTimeUs_ = 0;
Timestamp TimeHelper::currentIncCount_ = 0;
Timestamp TimeHelper::lastLocalTime_ = 0;
std::mutex TimeHelper::lastLocalTimeLock_;

struct TransactFunc {
    void (*xFunc)(sqlite3_context*, int, sqlite3_value**) = nullptr;
    void (*xStep)(sqlite3_context*, int, sqlite3_value**) = nullptr;
    void (*xFinal)(sqlite3_context*) = nullptr;
    void(*xDestroy)(void*) = nullptr;
};

std::mutex g_clientObserverMutex;
std::map<std::string, ClientObserver> g_clientObserverMap;

void StringToVector(const std::string &src, std::vector<uint8_t> &dst)
{
    dst.resize(src.size());
    dst.assign(src.begin(), src.end());
}

void VectorToString(const std::vector<uint8_t> &src, std::string &dst)
{
    dst.clear();
    dst.assign(src.begin(), src.end());
}

int RegisterFunction(sqlite3 *db, const std::string &funcName, int nArg, void *uData, TransactFunc &func)
{
    if (db == nullptr) {
        return -E_ERROR;
    }
    return sqlite3_create_function_v2(db, funcName.c_str(), nArg, SQLITE_UTF8 | SQLITE_DETERMINISTIC, uData,
        func.xFunc, func.xStep, func.xFinal, func.xDestroy);
}

int CalcValueHash(const std::vector<uint8_t> &value, std::vector<uint8_t> &hashValue)
{
    ValueHashCalc hashCalc;
    int errCode = hashCalc.Initialize();
    if (errCode != E_OK) {
        return -E_ERROR;
    }

    errCode = hashCalc.Update(value);
    if (errCode != E_OK) {
        return -E_ERROR;
    }

    errCode = hashCalc.GetResult(hashValue);
    if (errCode != E_OK) {
        return -E_ERROR;
    }

    return E_OK;
}

void StringToUpper(std::string &str)
{
    std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
}

void RTrim(std::string &str)
{
    if (str.empty()) {
        return;
    }
    str.erase(str.find_last_not_of(" ") + 1);
}

void CalcHashKey(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    // 1 means that the function only needs one parameter, namely key
    if (ctx == nullptr || argc != 2 || argv == nullptr) { // 2 is params count
        return;
    }

    int errCode;
    std::vector<uint8_t> hashValue;
    DistributedDB::CollateType collateType = static_cast<DistributedDB::CollateType>(sqlite3_value_int(argv[1]));
    if (collateType == DistributedDB::CollateType::COLLATE_NOCASE) {
        auto colChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
        if (colChar == nullptr) {
            return;
        }
        std::string colStr(colChar);
        StringToUpper(colStr);
        std::vector<uint8_t> value;
        StringToVector(colStr, value);
        errCode = CalcValueHash(value, hashValue);
    } else if (collateType == DistributedDB::CollateType::COLLATE_RTRIM) {
        auto colChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
        if (colChar == nullptr) {
            return;
        }
        std::string colStr(colChar);
        RTrim(colStr);
        std::vector<uint8_t> value;
        StringToVector(colStr, value);
        errCode = CalcValueHash(value, hashValue);
    } else {
            auto keyBlob = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0]));
            if (keyBlob == nullptr) {
                sqlite3_result_error(ctx, "Parameters is invalid.", -1);
                return;
            }

            int blobLen = sqlite3_value_bytes(argv[0]);
            std::vector<uint8_t> value(keyBlob, keyBlob + blobLen);
            errCode = CalcValueHash(value, hashValue);
    }

    if (errCode != DistributedDB::DBStatus::OK) {
        sqlite3_result_error(ctx, "Get hash value error.", -1);
        return;
    }
    sqlite3_result_blob(ctx, hashValue.data(), hashValue.size(), SQLITE_TRANSIENT);
    return;
}

int RegisterCalcHash(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CalcHashKey;
    return RegisterFunction(db, "calc_hash", 2, nullptr, func); // 2 is params count
}

void GetSysTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 1 || argv == nullptr) { // 1: function need one parameter
        return;
    }
    int timeOffset = static_cast<int64_t>(sqlite3_value_int64(argv[0]));
    sqlite3_result_int64(ctx, (sqlite3_int64)TimeHelper::GetTime(timeOffset));
}

void GetRawSysTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 0 || argv == nullptr) { // 0: function need zero parameter
        return;
    }

    uint64_t curTime = 0;
    int errCode = TimeHelper::GetSysCurrentRawTime(curTime);
    if (errCode != DistributedDB::DBStatus::OK) {
        sqlite3_result_error(ctx, "get raw sys time failed.", errCode);
        return;
    }
    sqlite3_result_int64(ctx, (sqlite3_int64)(curTime));
}

void GetLastTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 0 || argv == nullptr) { // 0: function need zero parameter
        return;
    }

    sqlite3_result_int64(ctx, (sqlite3_int64)TimeHelper::GetTime(0));
}

int GetHashString(const std::string &str, std::string &dst)
{
    std::vector<uint8_t> strVec;
    StringToVector(str, strVec);
    std::vector<uint8_t> hashVec;
    int errCode = CalcValueHash(strVec, hashVec);
    if (errCode != E_OK) {
        return errCode;
    }
    VectorToString(hashVec, dst);
    return E_OK;
}

static bool GetDbFileName(sqlite3 *db, std::string &fileName)
{
    if (db == nullptr) {
        return false;
    }

    auto dbFilePath = sqlite3_db_filename(db, nullptr);
    if (dbFilePath == nullptr) {
        return false;
    }
    fileName = std::string(dbFilePath);
    return true;
}

void CloudDataChangedObserver(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 3 || argv == nullptr) { // 3 is param counts
        return;
    }
    sqlite3 *db = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        return;
    }

    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_clientObserverMutex);
    auto it = g_clientObserverMap.find(hashFileName);
    if (it != g_clientObserverMap.end()) {
        auto tableNameChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
        if (tableNameChar == nullptr) {
            return;
        }

        if (it->second != nullptr) {
            std::string tableName = std::string(tableNameChar);
            ClientChangedData clientChangedData { tableName };
            it->second(clientChangedData);
        }
    }
    sqlite3_result_int64(ctx, (sqlite3_int64)1);
}

int RegisterGetSysTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetSysTime;
    return RegisterFunction(db, "get_sys_time", 1, nullptr, func);
}

int RegisterGetRawSysTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetRawSysTime;
    return RegisterFunction(db, "get_raw_sys_time", 0, nullptr, func);
}

int RegisterGetLastTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetLastTime;
    return RegisterFunction(db, "get_last_time", 0, nullptr, func);
}

int RegisterCloudDataChangeObserver(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CloudDataChangedObserver;
    return RegisterFunction(db, "client_observer", 3, db, func); // 3 is param counts const_cast<char *>(dbFilePath)
}

int ResetStatement(sqlite3_stmt *&stmt)
{
    if (stmt == nullptr || sqlite3_finalize(stmt) != SQLITE_OK) {
        return -E_ERROR;
    }
    stmt = nullptr;
    return E_OK;
}

int GetStatement(sqlite3 *db, const std::string &sql, sqlite3_stmt *&stmt)
{
    int errCode = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (errCode != SQLITE_OK) {
        (void)ResetStatement(stmt);
        return -E_ERROR;
    }
    return E_OK;
}

int ExecuteRawSQL(sqlite3 *db, const std::string &sql)
{
    if (db == nullptr) {
        return -E_ERROR;
    }
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK) {
        errCode = -E_ERROR;
    }

    if (errMsg != nullptr) {
        sqlite3_free(errMsg);
        errMsg = nullptr;
    }
    return errCode;
}

int StepWithRetry(sqlite3_stmt *stmt)
{
    if (stmt == nullptr) {
        return -E_ERROR;
    }
    int errCode = sqlite3_step(stmt);
    if (errCode != SQLITE_DONE && errCode != SQLITE_ROW) {
        return -E_ERROR;
    }
    return errCode;
}

int GetColumnTestValue(sqlite3_stmt *stmt, int index, std::string &value)
{
    if (stmt == nullptr) {
        return -E_ERROR;
    }
    const unsigned char *val = sqlite3_column_text(stmt, index);
    value = (val != nullptr) ? std::string(reinterpret_cast<const char *>(val)) : std::string();
    return E_OK;
}

int GetCurrentMaxTimestamp(sqlite3 *db, Timestamp &maxTimestamp)
{
    if (db == nullptr) {
        return -E_ERROR;
    }
    std::string checkTableSql = "SELECT name FROM sqlite_master WHERE type = 'table' AND " \
        "name LIKE 'naturalbase_rdb_aux_%_log';";
    sqlite3_stmt *checkTableStmt = nullptr;
    int errCode = GetStatement(db, checkTableSql, checkTableStmt);
    if (errCode != E_OK) {
        return -E_ERROR;
    }
    while ((errCode = StepWithRetry(checkTableStmt)) != SQLITE_DONE) {
        if (errCode != SQLITE_ROW) {
            ResetStatement(checkTableStmt);
            return -E_ERROR;
        }
        std::string logTablename;
        GetColumnTestValue(checkTableStmt, 0, logTablename);
        if (logTablename.empty()) {
            continue;
        }

        std::string getMaxTimestampSql = "SELECT MAX(timestamp) FROM " + logTablename + ";";
        sqlite3_stmt *getTimeStmt = nullptr;
        errCode = GetStatement(db, getMaxTimestampSql, getTimeStmt);
        if (errCode != E_OK) {
            continue;
        }
        errCode = StepWithRetry(getTimeStmt);
        if (errCode != SQLITE_ROW) {
            ResetStatement(getTimeStmt);
            continue;
        }
        auto tableMaxTimestamp = static_cast<Timestamp>(sqlite3_column_int64(getTimeStmt, 0));
        maxTimestamp = (maxTimestamp > tableMaxTimestamp) ? maxTimestamp : tableMaxTimestamp;
        ResetStatement(getTimeStmt);
    }
    ResetStatement(checkTableStmt);
    return E_OK;
}

int GetColumnBlobValue(sqlite3_stmt *stmt, int index, std::vector<uint8_t> &value)
{
    if (stmt == nullptr) {
        return -E_ERROR;
    }

    int keySize = sqlite3_column_bytes(stmt, index);
    if (keySize < 0) {
        value.resize(0);
        return E_OK;
    }
    auto keyRead = static_cast<const uint8_t *>(sqlite3_column_blob(stmt, index));
    if (keySize == 0 || keyRead == nullptr) {
        value.resize(0);
    } else {
        if (keySize > MAX_BLOB_READ_SIZE) {
            keySize = MAX_BLOB_READ_SIZE + 1;
        }
        value.resize(keySize);
        value.assign(keyRead, keyRead + keySize);
    }
    return E_OK;
}

int GetTableSyncType(sqlite3 *db, const std::string &tableName, std::string &tableType)
{
    const char *selectSql = "SELECT value FROM naturalbase_rdb_aux_metadata WHERE key=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_prepare_v2(db, selectSql, -1, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        (void)sqlite3_finalize(statement);
        return DistributedDB::DBStatus::DB_ERROR;
    }

    std::string keyStr = SYNC_TABLE_TYPE + tableName;
    std::vector<uint8_t> key(keyStr.begin(), keyStr.end());
    if (sqlite3_bind_blob(statement, 1, static_cast<const void *>(key.data()), key.size(),
        SQLITE_TRANSIENT) != SQLITE_OK) {
        return -E_ERROR;
    }

    if (sqlite3_step(statement) == SQLITE_ROW) {
        std::vector<uint8_t> value;
        if (GetColumnBlobValue(statement, 0, value) == DistributedDB::DBStatus::OK) {
            tableType.assign(value.begin(), value.end());
            (void)sqlite3_finalize(statement);
            return E_OK;
        } else {
            (void)sqlite3_finalize(statement);
            return -E_ERROR;
        }
    } else if (sqlite3_step(statement) != SQLITE_DONE) {
        (void)sqlite3_finalize(statement);
        return -E_ERROR;
    }
    (void)sqlite3_finalize(statement);
    tableType = DEVICE_TYPE;
    return E_OK;
}

void HandleDropCloudSyncTable(sqlite3 *db, const std::string &tableName)
{
    std::string logTblName = "naturalbase_rdb_aux_" + tableName + "_log";
    std::string sql = "UPDATE " + logTblName + " SET data_key=-1, flag=0x03, timestamp=get_raw_sys_time();";
    (void)sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    std::string keyStr = SYNC_TABLE_TYPE + tableName;
    std::vector<uint8_t> key(keyStr.begin(), keyStr.end());
    sql = "delete from naturalbase_rdb_aux_metadata where key = ?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        (void)sqlite3_finalize(statement);
        return;
    }

    if (sqlite3_bind_blob(statement, 1, static_cast<const void *>(key.data()), key.size(),
        SQLITE_TRANSIENT) != SQLITE_OK) {
        return;
    }
    (void)sqlite3_step(statement);
    (void)sqlite3_finalize(statement);
}

void ClearTheLogAfterDropTable(sqlite3 *db, const char *tableName, const char *schemaName)
{
    if (db == nullptr || tableName == nullptr || schemaName == nullptr) {
        return;
    }
    sqlite3_stmt *stmt = nullptr;
    std::string tableStr = std::string(tableName);
    std::string logTblName = "naturalbase_rdb_aux_" + tableStr + "_log";
    Timestamp dropTimeStamp = TimeHelper::GetTime(0);
    std::string sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + logTblName + "';";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        (void)sqlite3_finalize(stmt);
        return;
    }

    bool isLogTblExists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW && static_cast<bool>(sqlite3_column_int(stmt, 0))) {
        isLogTblExists = true;
    }
    (void)sqlite3_finalize(stmt);
    stmt = nullptr;

    if (isLogTblExists) {
        std::string tableType = DEVICE_TYPE;
        if (GetTableSyncType(db, tableStr, tableType) != DistributedDB::DBStatus::OK) {
            return;
        }
        if (tableType == DEVICE_TYPE) {
            RegisterGetSysTime(db);
            RegisterGetLastTime(db);
            sql = "UPDATE " + logTblName + " SET flag=0x03, timestamp=get_sys_time(0) "
                "WHERE flag&0x03=0x02 AND timestamp<" + std::to_string(dropTimeStamp);
            (void)sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        } else {
            HandleDropCloudSyncTable(db, tableStr);
        }
    }
}

void PostHandle(sqlite3 *db)
{
    Timestamp currentMaxTimestamp = 0;
    (void)GetCurrentMaxTimestamp(db, currentMaxTimestamp);
    TimeHelper::Initialize(currentMaxTimestamp);
    RegisterCalcHash(db);
    RegisterGetSysTime(db);
    RegisterGetLastTime(db);
    RegisterGetRawSysTime(db);
    RegisterCloudDataChangeObserver(db);
    (void)sqlite3_set_droptable_handle(db, &ClearTheLogAfterDropTable);
    (void)sqlite3_busy_timeout(db, BUSY_TIMEOUT);
    std::string recursiveTrigger = "PRAGMA recursive_triggers = ON;";
    (void)ExecuteRawSQL(db, recursiveTrigger);
}
}

SQLITE_API int sqlite3_open_relational(const char *filename, sqlite3 **ppDb)
{
    int err = sqlite3_open(filename, ppDb);
    if (err != SQLITE_OK) {
        return err;
    }
    PostHandle(*ppDb);
    return err;
}

SQLITE_API int sqlite3_open16_relational(const void *filename, sqlite3 **ppDb)
{
    int err = sqlite3_open16(filename, ppDb);
    if (err != SQLITE_OK) {
        return err;
    }
    PostHandle(*ppDb);
    return err;
}

SQLITE_API int sqlite3_open_v2_relational(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
{
    int err = sqlite3_open_v2(filename, ppDb, flags, zVfs);
    if (err != SQLITE_OK) {
        return err;
    }
    PostHandle(*ppDb);
    return err;
}

DB_API DistributedDB::DBStatus RegisterClientObserver(sqlite3 *db, const ClientObserver &clientObserver)
{
    std::string fileName;
    if (clientObserver == nullptr || !GetDbFileName(db, fileName)) {
        return DistributedDB::INVALID_ARGS;
    }

    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        return DistributedDB::DB_ERROR;
    }
    std::lock_guard<std::mutex> lock(g_clientObserverMutex);
    g_clientObserverMap[hashFileName] = clientObserver;
    return DistributedDB::OK;
}

DB_API DistributedDB::DBStatus UnRegisterClientObserver(sqlite3 *db)
{
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        return DistributedDB::INVALID_ARGS;
    }

    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        return DistributedDB::DB_ERROR;
    }
    std::lock_guard<std::mutex> lock(g_clientObserverMutex);
    auto it = g_clientObserverMap.find(hashFileName);
    if (it != g_clientObserverMap.end()) {
        g_clientObserverMap.erase(it);
    }
    return DistributedDB::OK;
}

// hw export the symbols
#ifdef SQLITE_DISTRIBUTE_RELATIONAL
#if defined(__GNUC__)
#  define EXPORT_SYMBOLS  __attribute__ ((visibility ("default")))
#elif defined(_MSC_VER)
    #  define EXPORT_SYMBOLS  __declspec(dllexport)
#else
#  define EXPORT_SYMBOLS
#endif

struct sqlite3_api_routines_relational {
    int (*open)(const char *, sqlite3 **);
    int (*open16)(const void *, sqlite3 **);
    int (*open_v2)(const char *, sqlite3 **, int, const char *);
};

typedef struct sqlite3_api_routines_relational sqlite3_api_routines_relational;
static const sqlite3_api_routines_relational sqlite3HwApis = {
#ifdef SQLITE_DISTRIBUTE_RELATIONAL
    sqlite3_open_relational,
    sqlite3_open16_relational,
    sqlite3_open_v2_relational
#else
    0,
    0,
    0
#endif
};

EXPORT_SYMBOLS const sqlite3_api_routines_relational *sqlite3_export_relational_symbols = &sqlite3HwApis;
#endif
