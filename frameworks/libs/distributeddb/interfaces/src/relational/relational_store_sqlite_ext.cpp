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
#include <variant>
#include <vector>

#include "cloud/cloud_db_constant.h"
#include "concurrent_adapter.h"
#include "db_common.h"
#include "db_constant.h"
#include "kv_store_errno.h"
#include "param_check_utils.h"
#include "platform_specific.h"
#include "relational_store_client.h"
#include "runtime_context.h"
#include "sqlite_utils.h"
#include "concurrent_adapter.h"

// using the "sqlite3sym.h" in OHOS
#ifndef USE_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif

#if defined _WIN32
#ifndef RUNNING_ON_WIN
#define RUNNING_ON_WIN
#endif
#else
#ifndef RUNNING_ON_LINUX
#define RUNNING_ON_LINUX
#endif
#endif

#if defined(RUNNING_ON_LINUX)
#include <unistd.h>
#elif defined RUNNING_ON_WIN
#include <io.h>
#else
#error "PLATFORM NOT SPECIFIED!"
#endif

#ifdef DB_DEBUG_ENV
#include "system_time.h"
#include "cloud/cloud_db_constant.h"

using namespace DistributedDB::OS;
#endif
using namespace DistributedDB;

namespace {
constexpr int E_OK = 0;
constexpr int E_ERROR = 1;
constexpr int STR_TO_LL_BY_DEVALUE = 10;
constexpr int BUSY_TIMEOUT = 2000;  // 2s.
constexpr int MAX_BLOB_READ_SIZE = 5 * 1024 * 1024; // 5M limit
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

using Timestamp = uint64_t;
using TimeOffset = int64_t;

class TimeHelper {
public:
    // 10000 year 100ns
    static constexpr int64_t BASE_OFFSET = 10000LL * 365LL * 24LL * 3600LL * 1000LL * 1000LL * 10L;

    static constexpr int64_t MAX_VALID_TIME = BASE_OFFSET * 2; // 20000 year 100ns

    static constexpr uint64_t TO_100_NS = 10; // 1us to 100ns

    static constexpr Timestamp INVALID_TIMESTAMP = 0;

    static constexpr uint64_t MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS = 1000000;

    static constexpr int64_t MAX_NOISE = 9 * 100 * 1000; // 900ms

    static constexpr uint64_t MAX_INC_COUNT = 9; // last bit from 0-9

    static int GetSysCurrentRawTime(uint64_t &curTime)
    {
        int errCode = GetCurrentSysTimeInMicrosecond(curTime);
        if (errCode != E_OK) {
            return errCode;
        }
        curTime *= TO_100_NS;
        return E_OK;
    }

    // Init the TimeHelper for time skew
    void Initialize()
    {
        // it maybe a rebuild db when initialize with localTimeOffset is TimeHelper::BASE_OFFSET
        // reinitialize for this db
        if (isInitialized_) {
            return;
        }
        (void)GetCurrentSysTimeInMicrosecond(lastSystemTime_);
        (void)OS::GetMonotonicRelativeTimeInMicrosecond(lastMonotonicTime_);
        LOGD("Initialize time helper skew: %" PRIu64 " %" PRIu64, lastSystemTime_, lastMonotonicTime_);
        isInitialized_ = true;
    }

    Timestamp GetTime(TimeOffset timeOffset, const std::function<Timestamp()> &getDbMaxTimestamp,
        const std::function<Timestamp()> &getDbLocalTimeOffset)
    {
        if (!isLoaded_) { // First use, load max time stamp and local time offset from db;
            if (getDbMaxTimestamp != nullptr) {
                lastLocalTime_ = getDbMaxTimestamp();
            }
            if (getDbLocalTimeOffset != nullptr) {
                localTimeOffset_ = getDbLocalTimeOffset();
            }
            LOGD("Use time helper with maxTimestamp:%" PRIu64 " localTimeOffset:%" PRIu64 " first time", lastLocalTime_,
                localTimeOffset_);
            isLoaded_ = true;
        }
        Timestamp currentSystemTime = 0u;
        (void)GetCurrentSysTimeInMicrosecond(currentSystemTime);
        Timestamp currentMonotonicTime = 0u;
        (void)OS::GetMonotonicRelativeTimeInMicrosecond(currentMonotonicTime);
        auto deltaTime = static_cast<int64_t>(currentMonotonicTime - lastMonotonicTime_);
        Timestamp currentSysTime = GetSysCurrentTime();
        Timestamp currentLocalTime = currentSysTime + timeOffset + localTimeOffset_;
        if (currentLocalTime <= lastLocalTime_ || currentLocalTime > MAX_VALID_TIME) {
            LOGD("Ext invalid time: lastLocalTime_: %" PRIu64 ", currentLocalTime: %" PRIu64 ", deltaTime: %" PRId64,
                lastLocalTime_, currentLocalTime, deltaTime);
            lastLocalTime_ = static_cast<Timestamp>(static_cast<int64_t>(lastLocalTime_) +
                deltaTime * static_cast<int64_t>(TO_100_NS));
            currentLocalTime = lastLocalTime_;
        } else {
            lastLocalTime_ = currentLocalTime;
        }

        lastSystemTime_ = currentSystemTime;
        lastMonotonicTime_ = currentMonotonicTime;
        return currentLocalTime;
    }

    bool TimeSkew(const std::function<Timestamp()> &getLocalTimeOffsetFromDB, Timestamp timeOffset)
    {
        Timestamp currentSystemTime = 0u;
        (void)GetCurrentSysTimeInMicrosecond(currentSystemTime);
        Timestamp currentMonotonicTime = 0u;
        (void)OS::GetMonotonicRelativeTimeInMicrosecond(currentMonotonicTime);

        auto systemTimeOffset = static_cast<int64_t>(currentSystemTime - lastSystemTime_);
        auto monotonicTimeOffset = static_cast<int64_t>(currentMonotonicTime - lastMonotonicTime_);
        if (std::abs(systemTimeOffset - monotonicTimeOffset) > MAX_NOISE) {
            // time skew detected
            Timestamp localTimeOffset = getLocalTimeOffsetFromDB();
            Timestamp currentSysTime = GetSysCurrentTime();
            Timestamp currentLocalTime = currentSysTime + timeOffset + localTimeOffset;
            auto virtualTimeOffset = static_cast<int64_t>(currentLocalTime - lastLocalTime_);
            auto changedOffset = static_cast<int64_t>(virtualTimeOffset - monotonicTimeOffset * TO_100_NS);
            if (std::abs(changedOffset) > static_cast<int64_t>(MAX_NOISE * TO_100_NS)) {
                // localTimeOffset was not flushed, use temporary calculated value
                localTimeOffset_ = static_cast<Timestamp>(static_cast<int64_t>(localTimeOffset_) -
                    (systemTimeOffset - monotonicTimeOffset) * static_cast<int64_t>(TO_100_NS));
                LOGD("Save ext local time offset: %" PRIu64 ", changedOffset: %" PRId64, localTimeOffset_,
                    changedOffset);
            } else {
                localTimeOffset_ = localTimeOffset;
                LOGD("Save ext local time offset: %" PRIu64, localTimeOffset_);
            }
        }
        lastSystemTime_ = currentSystemTime;
        lastMonotonicTime_ = currentMonotonicTime;
        return std::abs(systemTimeOffset - monotonicTimeOffset) > MAX_NOISE;
    }

    Timestamp GetLastTime() const
    {
        return lastLocalTime_;
    }

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

#ifndef DB_DEBUG_ENV
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
#endif

    static std::mutex systemTimeLock_;
    static Timestamp lastSystemTimeUs_;
    static Timestamp currentIncCount_;

    Timestamp lastLocalTime_ = 0;
    Timestamp localTimeOffset_ = TimeHelper::BASE_OFFSET;
    bool isLoaded_ = false;

    Timestamp lastSystemTime_ = 0;
    Timestamp lastMonotonicTime_ = 0;
    bool isInitialized_ = false;
};

class TimeHelperManager {
public:
    static TimeHelperManager *GetInstance()
    {
        static auto instance = new TimeHelperManager();
        return instance;
    }

    void AddStore(const std::string &storeId)
    {
        std::lock_guard<std::mutex> lock(metaDataLock_);
        if (metaData_.find(storeId) != metaData_.end()) {
            return;
        }
        TimeHelper timeHelper;
        timeHelper.Initialize();
        metaData_[storeId] = timeHelper;
    }

    void Restore(const std::string &storeId)
    {
        std::lock_guard<std::mutex> lock(metaDataLock_);
        if (metaData_.find(storeId) != metaData_.end()) {
            LOGD("Restore time helper");
            metaData_.erase(storeId);
        }
    }

    Timestamp GetLastTime(const std::string &storeId)
    {
        std::lock_guard<std::mutex> lock(metaDataLock_);
        auto it = metaData_.find(storeId);
        if (it == metaData_.end()) {
            return TimeHelper::INVALID_TIMESTAMP;
        }
        return it->second.GetLastTime();
    }

    bool TimeSkew(const std::string &storeId, const std::function<Timestamp()> &getLocalTimeOffsetFromDB,
        Timestamp timeOffset)
    {
        std::lock_guard<std::mutex> lock(metaDataLock_);
        auto it = metaData_.find(storeId);
        if (it == metaData_.end()) {
            return false;
        }
        return it->second.TimeSkew(getLocalTimeOffsetFromDB, timeOffset);
    }

    Timestamp GetTime(const std::string &storeId, TimeOffset timeOffset,
        const std::function<Timestamp()> &getDbMaxTimestamp, const std::function<Timestamp()> &getDbLocalTimeOffset)
    {
        std::lock_guard<std::mutex> lock(metaDataLock_);
        auto it = metaData_.find(storeId);
        if (it == metaData_.end()) {
            return TimeHelper::INVALID_TIMESTAMP;
        }
        return it->second.GetTime(timeOffset, getDbMaxTimestamp, getDbLocalTimeOffset);
    }
private:
    TimeHelperManager() = default;
    std::mutex metaDataLock_;
    std::map<std::string, TimeHelper> metaData_;
};

std::mutex TimeHelper::systemTimeLock_;
Timestamp TimeHelper::lastSystemTimeUs_ = 0;
Timestamp TimeHelper::currentIncCount_ = 0;

int GetStatement(sqlite3 *db, const std::string &sql, sqlite3_stmt *&stmt);
int ResetStatement(sqlite3_stmt *&stmt);
int BindBlobToStatement(sqlite3_stmt *stmt, int index, const std::vector<uint8_t> &value, bool permEmpty = false);
int StepWithRetry(sqlite3_stmt *stmt);
int GetColumnBlobValue(sqlite3_stmt *stmt, int index, std::vector<uint8_t> &value);
int GetColumnTextValue(sqlite3_stmt *stmt, int index, std::string &value);
int GetDBIdentity(sqlite3 *db, std::string &identity);

struct TransactFunc {
    void (*xFunc)(sqlite3_context*, int, sqlite3_value**) = nullptr;
    void (*xStep)(sqlite3_context*, int, sqlite3_value**) = nullptr;
    void (*xFinal)(sqlite3_context*) = nullptr;
    void(*xDestroy)(void*) = nullptr;
};

std::mutex g_clientObserverMutex;
std::map<std::string, ClientObserver> g_clientObserverMap;
std::mutex g_clientChangedDataMutex;
std::map<std::string, ClientChangedData> g_clientChangedDataMap;

std::mutex g_storeObserverMutex;
std::map<std::string, std::list<std::shared_ptr<StoreObserver>>> g_storeObserverMap;
std::mutex g_storeChangedDataMutex;
std::map<std::string, std::vector<ChangedData>> g_storeChangedDataMap;

std::mutex g_clientCreateTableMutex;
std::set<std::string> g_clientCreateTable;

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
        value.assign(colStr.begin(), colStr.end());
        errCode = CalcValueHash(value, hashValue);
    } else if (collateType == DistributedDB::CollateType::COLLATE_RTRIM) {
        auto colChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
        if (colChar == nullptr) {
            return;
        }
        std::string colStr(colChar);
        DBCommon::RTrim(colStr);
        std::vector<uint8_t> value;
        value.assign(colStr.begin(), colStr.end());
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
}

int RegisterCalcHash(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CalcHashKey;
    return RegisterFunction(db, "calc_hash", 2, nullptr, func); // 2 is params count
}

int GetLocalTimeOffsetFromMeta(sqlite3 *db, TimeOffset &offset)
{
    if (db == nullptr) {
        return -E_ERROR;
    }

    sqlite3_stmt *stmt = nullptr;
    int errCode = GetStatement(db, "SELECT value FROM naturalbase_rdb_aux_metadata WHERE key = ?", stmt);
    if (errCode != E_OK) {
        LOGE("Prepare meta data stmt failed. %d", errCode);
        return -E_ERROR;
    }

    std::string keyStr = "localTimeOffset";
    std::vector<uint8_t> key(keyStr.begin(), keyStr.end());
    errCode = BindBlobToStatement(stmt, 1, key);
    if (errCode != E_OK) {
        (void)ResetStatement(stmt);
        LOGE("Bind meta data stmt failed, Key: %s. %d", keyStr.c_str(), errCode);
        return -E_ERROR;
    }

    std::vector<uint8_t> value;
    errCode = StepWithRetry(stmt);
    if (errCode == SQLITE_ROW) {
        GetColumnBlobValue(stmt, 0, value);
    } else if (errCode == SQLITE_DONE) {
        (void)ResetStatement(stmt);
        LOGE("Get meta data not found, Key: %s. %d", keyStr.c_str(), errCode);
        return E_OK;
    } else {
        (void)ResetStatement(stmt);
        LOGE("Get meta data failed, Key: %s. %d", keyStr.c_str(), errCode);
        return -E_ERROR;
    }

    std::string valueStr(value.begin(), value.end());
    int64_t result = std::strtoll(valueStr.c_str(), nullptr, STR_TO_LL_BY_DEVALUE);
    if (errno != ERANGE && result != LLONG_MIN && result != LLONG_MAX) {
        offset = result;
    }
    (void)ResetStatement(stmt);
    LOGD("Get local time offset from meta: %" PRId64, offset);
    return E_OK;
}

int GetCurrentMaxTimestamp(sqlite3 *db, Timestamp &maxTimestamp);

void GetSysTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 1 || argv == nullptr) { // 1: function need one parameter
        return;
    }

    auto *db = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    if (db == nullptr) {
        sqlite3_result_error(ctx, "Sqlite context is invalid.", -1);
        return;
    }

    std::function<Timestamp()> getDbMaxTimestamp = [db]() -> Timestamp {
        Timestamp maxTimestamp = 0;
        (void)GetCurrentMaxTimestamp(db, maxTimestamp);
        return maxTimestamp;
    };
    std::function<Timestamp()> getDbLocalTimeOffset = [db]() -> Timestamp {
        TimeOffset localTimeOffset = TimeHelper::BASE_OFFSET;
        (void)GetLocalTimeOffsetFromMeta(db, localTimeOffset);
        return localTimeOffset;
    };

    std::string identity;
    GetDBIdentity(db, identity);
    auto timeOffset = static_cast<TimeOffset>(sqlite3_value_int64(argv[0]));
    (void)TimeHelperManager::GetInstance()->TimeSkew(identity, getDbLocalTimeOffset, timeOffset);
    Timestamp currentTime = TimeHelperManager::GetInstance()->GetTime(identity, timeOffset, getDbMaxTimestamp,
        getDbLocalTimeOffset);
    sqlite3_result_int64(ctx, (sqlite3_int64)currentTime);
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

    auto *db = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    if (db == nullptr) {
        sqlite3_result_error(ctx, "Sqlite context is invalid.", -1);
        return;
    }
    std::string identity;
    GetDBIdentity(db, identity);

    sqlite3_result_int64(ctx, (sqlite3_int64)TimeHelperManager::GetInstance()->GetLastTime(identity));
}

int GetHashString(const std::string &str, std::string &dst)
{
    std::vector<uint8_t> strVec;
    strVec.assign(str.begin(), str.end());
    std::vector<uint8_t> hashVec;
    int errCode = CalcValueHash(strVec, hashVec);
    if (errCode != E_OK) {
        LOGE("calc hash value fail, %d", errCode);
        return errCode;
    }
    dst.assign(hashVec.begin(), hashVec.end());
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

static bool GetDbHashString(sqlite3 *db, std::string &hashFileName)
{
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        return false;
    }
    return GetHashString(fileName, hashFileName) == DistributedDB::DBStatus::OK;
}

void CloudDataChangedObserver(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 4 || argv == nullptr) { // 4 is param counts
        return;
    }
    sqlite3 *db = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    std::string hashFileName = "";
    if (!GetDbHashString(db, hashFileName)) {
        return;
    }
    auto tableNameChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
    if (tableNameChar == nullptr) {
        return;
    }
    std::string tableName = static_cast<std::string>(tableNameChar);

    uint64_t isTrackerChange = static_cast<uint64_t>(sqlite3_value_int(argv[3])); // 3 is param index
    bool isExistObserver = false;
    {
        std::lock_guard<std::mutex> lock(g_clientObserverMutex);
        auto it = g_clientObserverMap.find(hashFileName);
        isExistObserver = (it != g_clientObserverMap.end());
    }
    {
        std::lock_guard<std::mutex> lock(g_clientChangedDataMutex);
        if (isExistObserver) {
            auto itTable = g_clientChangedDataMap[hashFileName].tableData.find(tableName);
            if (itTable != g_clientChangedDataMap[hashFileName].tableData.end()) {
                itTable->second.isTrackedDataChange =
                    (static_cast<uint8_t>(itTable->second.isTrackedDataChange) | isTrackerChange) > 0;
            } else {
                DistributedDB::ChangeProperties properties = { .isTrackedDataChange = (isTrackerChange > 0) };
                g_clientChangedDataMap[hashFileName].tableData.insert_or_assign(tableName, properties);
            }
        }
    }
    sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(1));
}

int JudgeIfGetRowid(sqlite3 *db, const std::string &tableName, std::string &type, bool &isRowid)
{
    if (db == nullptr) {
        return -E_ERROR;
    }
    std::string checkPrimaryKeySql = "SELECT count(1), type FROM pragma_table_info('" + tableName;
    checkPrimaryKeySql += "') WHERE pk = 1";
    sqlite3_stmt *checkPrimaryKeyStmt = nullptr;
    int errCode = GetStatement(db, checkPrimaryKeySql, checkPrimaryKeyStmt);
    if (errCode != E_OK) {
        LOGE("Prepare get primarykey info statement failed. err=%d", errCode);
        return -E_ERROR;
    }
    errCode = SQLiteUtils::StepWithRetry(checkPrimaryKeyStmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        int count = sqlite3_column_int(checkPrimaryKeyStmt, 0);
        GetColumnTextValue(checkPrimaryKeyStmt, 1, type);
        isRowid = (count != 1) || (type != "TEXT" && type != "INT" && type != "INTEGER");
        errCode = E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    } else {
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
    }
    ResetStatement(checkPrimaryKeyStmt);
    return errCode;
}

void SaveChangedData(const std::string &hashFileName, const std::string &tableName, const std::string &columnName,
    const DistributedDB::Type &data, ChangeType option)
{
    if (option < ChangeType::OP_INSERT || option >= ChangeType::OP_BUTT) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_storeChangedDataMutex);
    auto itTable = std::find_if(g_storeChangedDataMap[hashFileName].begin(), g_storeChangedDataMap[hashFileName].end(),
        [tableName](DistributedDB::ChangedData changedData) {
            return tableName == changedData.tableName;
        });
    if (itTable != g_storeChangedDataMap[hashFileName].end()) {
        std::vector<Type> dataVec;
        dataVec.push_back(data);
        itTable->primaryData[option].push_back(dataVec);
    } else {
        DistributedDB::ChangedData changedData;
        changedData.tableName = tableName;
        changedData.field.push_back(columnName);
        std::vector<DistributedDB::Type> dataVec;
        dataVec.push_back(data);
        changedData.primaryData[option].push_back(dataVec);
        g_storeChangedDataMap[hashFileName].push_back(changedData);
    }
}

void DataChangedObserver(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 4 || argv == nullptr) { // 4 is param counts
        return;
    }
    sqlite3 *db = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    std::string hashFileName = "";
    if (!GetDbHashString(db, hashFileName)) {
        return;
    }
    bool isExistObserver = false;
    {
        std::lock_guard<std::mutex> lock(g_storeObserverMutex);
        auto it = g_storeObserverMap.find(hashFileName);
        isExistObserver = (it != g_storeObserverMap.end());
    }
    if (!isExistObserver) {
        return;
    }
    auto tableNameChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
    if (tableNameChar == nullptr) {
        return;
    }
    std::string tableName = static_cast<std::string>(tableNameChar);
    auto columnNameChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[1])); // 1 is param index of column
    if (columnNameChar == nullptr) {
        return;
    }
    std::string columnName = static_cast<std::string>(columnNameChar);
    DistributedDB::Type data;
    std::string type = "";
    bool isRowid = false;
    int errCode = JudgeIfGetRowid(db, tableName, type, isRowid);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Get primary key info error.", -1);
        return;
    }
    if (!isRowid && type == "TEXT") {
        auto dataChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[2])); // 2 is param index of changed data
        if (dataChar == nullptr) {
            return;
        }
        data = static_cast<std::string>(dataChar);
    } else {
        data = static_cast<int64_t>(sqlite3_value_int64(argv[2])); // 2 is param index of data
    }
    ChangeType option = static_cast<ChangeType>(sqlite3_value_int64(argv[3])); // 3 is param index of option type
    SaveChangedData(hashFileName, tableName, columnName, data, option);
    sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(1)); // 1 is result ok
}

std::string GetInsertTrigger(const std::string &tableName, bool isRowid, const std::string &primaryKey)
{
    std::string insertTrigger = "CREATE TEMP TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + tableName + "_local_ON_INSERT AFTER INSERT\n";
    insertTrigger += "ON '" + tableName + "'\n";
    insertTrigger += "BEGIN\n";
    if (isRowid || primaryKey.empty()) {
        insertTrigger += "SELECT data_change('" + tableName + "', 'rowid', NEW._rowid_, 0);\n";
    } else {
        insertTrigger += "SELECT data_change('" + tableName + "', ";
        insertTrigger += "(SELECT name as a FROM pragma_table_info('" + tableName + "') WHERE pk=1), ";
        insertTrigger += "NEW." + primaryKey + ", 0);\n";
    }
    insertTrigger += "END;";
    return insertTrigger;
}

std::string GetUpdateTrigger(const std::string &tableName, bool isRowid, const std::string &primaryKey)
{
    std::string updateTrigger = "CREATE TEMP TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + tableName + "_local_ON_UPDATE AFTER UPDATE\n";
    updateTrigger += "ON '" + tableName + "'\n";
    updateTrigger += "BEGIN\n";
    if (isRowid || primaryKey.empty()) {
        updateTrigger += "SELECT data_change('" + tableName + "', 'rowid', NEW._rowid_, 2);\n";
    } else {
        updateTrigger += "SELECT data_change('" + tableName + "', ";
        updateTrigger += "(SELECT name as a FROM pragma_table_info('" + tableName + "') WHERE pk=1), ";
        updateTrigger += "NEW." + primaryKey + ", 1);\n";
    }
    updateTrigger += "END;";
    return updateTrigger;
}

std::string GetDeleteTrigger(const std::string &tableName, bool isRowid, const std::string &primaryKey)
{
    std::string deleteTrigger = "CREATE TEMP TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + tableName + "_local_ON_DELETE AFTER DELETE\n";
    deleteTrigger += "ON '" + tableName + "'\n";
    deleteTrigger += "BEGIN\n";
    if (isRowid || primaryKey.empty()) {
        deleteTrigger += "SELECT data_change('" + tableName + "', 'rowid', OLD._rowid_, 2);\n";
    } else {
        deleteTrigger += "SELECT data_change('" + tableName + "', ";
        deleteTrigger += "(SELECT name as a FROM pragma_table_info('" + tableName + "') WHERE pk=1), ";
        deleteTrigger += "OLD." + primaryKey + ", 2);\n";
    }
    deleteTrigger += "END;";
    return deleteTrigger;
}

int GetPrimaryKeyName(sqlite3 *db, const std::string &tableName, std::string &primaryKey)
{
    if (db == nullptr) {
        return -E_ERROR;
    }
    std::string sql = "SELECT name FROM pragma_table_info('";
    sql += tableName + "') WHERE pk = 1";
    sqlite3_stmt *stmt = nullptr;
    int errCode = GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Prepare get primary key name statement failed. err=%d", errCode);
        return -E_ERROR;
    }
    while ((errCode = StepWithRetry(stmt)) != SQLITE_DONE) {
        if (errCode != SQLITE_ROW) {
            ResetStatement(stmt);
            return -E_ERROR;
        }
        GetColumnTextValue(stmt, 0, primaryKey);
    }
    ResetStatement(stmt);
    return E_OK;
}

int GetTriggerSqls(sqlite3 *db, const std::map<std::string, bool> &tableInfos, std::vector<std::string> &triggerSqls)
{
    for (const auto &tableInfo : tableInfos) {
        std::string primaryKey = "";
        if (!tableInfo.second) {
            int errCode = GetPrimaryKeyName(db, tableInfo.first, primaryKey);
            if (errCode != E_OK) {
                return errCode;
            }
        }
        std::string sql = GetInsertTrigger(tableInfo.first, tableInfo.second, primaryKey);
        triggerSqls.push_back(sql);
        sql = GetUpdateTrigger(tableInfo.first, tableInfo.second, primaryKey);
        triggerSqls.push_back(sql);
        sql = GetDeleteTrigger(tableInfo.first, tableInfo.second, primaryKey);
        triggerSqls.push_back(sql);
    }
    return E_OK;
}

int AuthorizerCallback(void *data, int operation, const char *tableNameChar, const char *, const char *, const char *)
{
    if (operation != SQLITE_CREATE_TABLE || tableNameChar == nullptr) {
        return SQLITE_OK;
    }
    std::lock_guard<std::mutex> clientCreateTableLock(g_clientCreateTableMutex);
    std::string tableName = static_cast<std::string>(tableNameChar);
    if (ParamCheckUtils::CheckRelationalTableName(tableName) && tableName.find("sqlite_") != 0 &&
        tableName.find("naturalbase_") != 0) {
        g_clientCreateTable.insert(tableName);
    }
    return SQLITE_OK;
}

void ClientObserverCallback(const std::string &hashFileName)
{
    ClientObserver clientObserver;
    {
        std::lock_guard<std::mutex> clientObserverLock(g_clientObserverMutex);
        auto it = g_clientObserverMap.find(hashFileName);
        if (it != g_clientObserverMap.end() && it->second != nullptr) {
            clientObserver = it->second;
        } else {
            return;
        }
    }
    std::lock_guard<std::mutex> clientChangedDataLock(g_clientChangedDataMutex);
    auto it = g_clientChangedDataMap.find(hashFileName);
    if (it != g_clientChangedDataMap.end() && !it->second.tableData.empty()) {
        ClientChangedData clientChangedData = g_clientChangedDataMap[hashFileName];
        ConcurrentAdapter::ScheduleTask([clientObserver, clientChangedData] {
            ClientChangedData taskClientChangedData = clientChangedData;
            clientObserver(taskClientChangedData);
        });
        g_clientChangedDataMap[hashFileName].tableData.clear();
    }
}

void TriggerObserver(std::vector<std::shared_ptr<StoreObserver>> storeObservers, const std::string &hashFileName)
{
    std::lock_guard<std::mutex> storeChangedDataLock(g_storeChangedDataMutex);
    for (const auto &storeObserver : storeObservers) {
        auto it = g_storeChangedDataMap.find(hashFileName);
        if (it != g_storeChangedDataMap.end() && !it->second.empty()) {
            std::vector<DistributedDB::ChangedData> storeChangedData = g_storeChangedDataMap[hashFileName];
            storeObserver->OnChange(std::move(storeChangedData));
        }
    }
    g_storeChangedDataMap[hashFileName].clear();
}

void StoreObserverCallback(sqlite3 *db, const std::string &hashFileName)
{
    std::vector<std::shared_ptr<StoreObserver>> storeObserver;
    {
        std::lock_guard<std::mutex> storeObserverLock(g_storeObserverMutex);
        auto it = g_storeObserverMap.find(hashFileName);
        if (it != g_storeObserverMap.end() && !it->second.empty()) {
            for (const auto &observer : it->second) {
                storeObserver.push_back(observer);
            }
        } else {
            return;
        }
    }
    TriggerObserver(storeObserver, hashFileName);
    std::map<std::string, bool> tableInfos;
    {
        std::lock_guard<std::mutex> clientCreateTableLock(g_clientCreateTableMutex);
        if (g_clientCreateTable.empty()) {
            return;
        }
        for (const auto &tableName : g_clientCreateTable) {
            bool isRowid = true;
            std::string type = "";
            JudgeIfGetRowid(db, tableName, type, isRowid);
            tableInfos.insert(std::make_pair(tableName, isRowid));
        }
        g_clientCreateTable.clear();
    }
    std::vector<std::string> triggerSqls;
    int errCode = GetTriggerSqls(db, tableInfos, triggerSqls);
    if (errCode != E_OK) {
        return;
    }
    for (const auto &sql : triggerSqls) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            LOGE("Create data change trigger failed %d", errCode);
            return;
        }
    }
    return;
}

int LogCommitHookCallback(void *data, sqlite3 *db, const char *zDb, int size)
{
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        return 0;
    }
    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        return 0;
    }
    ClientObserverCallback(hashFileName);
    StoreObserverCallback(db, hashFileName);
    return 0;
}

void RollbackHookCallback(void* data)
{
    sqlite3 *db = static_cast<sqlite3 *>(data);
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        return;
    }
    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        return;
    }
    std::lock_guard<std::mutex> clientChangedDataLock(g_clientChangedDataMutex);
    auto it = g_clientChangedDataMap.find(hashFileName);
    if (it != g_clientChangedDataMap.end() && !it->second.tableData.empty()) {
        g_clientChangedDataMap[hashFileName].tableData.clear();
    }
}

int RegisterGetSysTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetSysTime;
    return RegisterFunction(db, "get_sys_time", 1, db, func);
}

int RegisterGetLastTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetLastTime;
    return RegisterFunction(db, "get_last_time", 0, db, func);
}

int RegisterGetRawSysTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetRawSysTime;
    return RegisterFunction(db, "get_raw_sys_time", 0, nullptr, func);
}

int RegisterCloudDataChangeObserver(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CloudDataChangedObserver;
    return RegisterFunction(db, "client_observer", 4, db, func); // 4 is param counts
}

int RegisterDataChangeObserver(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &DataChangedObserver;
    return RegisterFunction(db, "data_change", 4, db, func); // 4 is param counts
}

void RegisterCommitAndRollbackHook(sqlite3 *db)
{
    sqlite3_set_authorizer(db, AuthorizerCallback, nullptr);
    sqlite3_wal_hook(db, LogCommitHookCallback, db);
    sqlite3_rollback_hook(db, RollbackHookCallback, db);
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
        LOGE("Get stmt failed. %d", errCode);
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

int BindBlobToStatement(sqlite3_stmt *stmt, int index, const std::vector<uint8_t> &value, bool permEmpty)
{
    if (stmt == nullptr || (value.empty() && !permEmpty)) {
        return -E_ERROR;
    }
    int errCode;
    if (value.empty()) {
        errCode = sqlite3_bind_zeroblob(stmt, index, -1); // -1: zero-length blob
    } else {
        errCode = sqlite3_bind_blob(stmt, index, static_cast<const void*>(value.data()), value.size(),
            SQLITE_TRANSIENT);
    }
    return errCode == E_OK ? E_OK : -E_ERROR;
}

int GetColumnTextValue(sqlite3_stmt *stmt, int index, std::string &value)
{
    if (stmt == nullptr) {
        return -E_ERROR;
    }
    const unsigned char *val = sqlite3_column_text(stmt, index);
    value = (val != nullptr) ? std::string(reinterpret_cast<const char *>(val)) : std::string();
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
        LOGE("Prepare get max log timestamp statement failed. err=%d", errCode);
        return -E_ERROR;
    }
    while ((errCode = StepWithRetry(checkTableStmt)) != SQLITE_DONE) {
        if (errCode != SQLITE_ROW) {
            ResetStatement(checkTableStmt);
            return -E_ERROR;
        }
        std::string logTableName;
        GetColumnTextValue(checkTableStmt, 0, logTableName);
        if (logTableName.empty()) {
            continue;
        }

        std::string getMaxTimestampSql = "SELECT MAX(timestamp) FROM " + logTableName + ";";
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

bool CheckTableExists(sqlite3 *db, const std::string &tableName)
{
    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + tableName + "';";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        (void)sqlite3_finalize(stmt);
        return false;
    }

    bool isLogTblExists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW && static_cast<bool>(sqlite3_column_int(stmt, 0))) {
        isLogTblExists = true;
    }
    (void)sqlite3_finalize(stmt);
    return isLogTblExists;
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
    sql = "DELETE FROM naturalbase_rdb_aux_metadata WHERE key = ?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        (void)sqlite3_finalize(statement);
        return;
    }

    if (sqlite3_bind_blob(statement, 1, static_cast<const void *>(key.data()), key.size(),
        SQLITE_TRANSIENT) != SQLITE_OK) {
        (void)sqlite3_finalize(statement);
        return;
    }
    (void)sqlite3_step(statement);
    (void)sqlite3_finalize(statement);
}

int HandleDropLogicDeleteData(sqlite3 *db, const std::string &tableName, uint64_t cursor)
{
    std::string logTblName = DBCommon::GetLogTableName(tableName);
    std::string sql = "INSERT OR REPLACE INTO " + DBConstant::RELATIONAL_PREFIX + "metadata" +
        " VALUES ('log_trigger_switch', 'false')";
    int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("close log_trigger_switch failed. %d", errCode);
        return errCode;
    }
    sql = "DELETE FROM " + tableName + " WHERE _rowid_ IN (SELECT data_key FROM " + logTblName + " WHERE "
        " flag&0x08=0x08" + (cursor == 0 ? ");" : " AND cursor <= '" + std::to_string(cursor) + "');");
    errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("delete logic deletedData failed. %d", errCode);
        return errCode;
    }
    sql = "UPDATE " + logTblName + " SET data_key = -1, flag = (flag & ~0x08) | 0x01 WHERE flag&0x08=0x08" +
        (cursor == 0 ? ";" : " AND cursor <= '" + std::to_string(cursor) + "';");
    errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("update logic deletedData failed. %d", errCode);
        return errCode;
    }
    sql = "INSERT OR REPLACE INTO " + DBConstant::RELATIONAL_PREFIX + "metadata" +
        " VALUES ('log_trigger_switch', 'true')";
    errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("open log_trigger_switch failed. %d", errCode);
    }
    return errCode;
}

int SaveDeleteFlagToDB(sqlite3 *db, const std::string &tableName)
{
    std::string keyStr = DBConstant::TABLE_IS_DROPPED + tableName;
    Key key;
    DBCommon::StringToVector(keyStr, key);
    Value value;
    DBCommon::StringToVector("1", value); // 1 means delete
    std::string sql = "INSERT OR REPLACE INTO naturalbase_rdb_aux_metadata VALUES(?, ?);";
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("[SaveDeleteFlagToDB] prepare statement failed, %d", errCode);
        return -E_ERROR;
    }

    if (sqlite3_bind_blob(statement, 1, static_cast<const void *>(key.data()), key.size(),
        SQLITE_TRANSIENT) != SQLITE_OK) {
        (void)sqlite3_finalize(statement);
        LOGE("[SaveDeleteFlagToDB] bind key failed, %d", errCode);
        return -E_ERROR;
    }
    if (sqlite3_bind_blob(statement, 2, static_cast<const void *>(value.data()), value.size(), // 2 is column index
        SQLITE_TRANSIENT) != SQLITE_OK) {
        (void)sqlite3_finalize(statement);
        LOGE("[SaveDeleteFlagToDB] bind value failed, %d", errCode);
        return -E_ERROR;
    }
    errCode = sqlite3_step(statement);
    if (errCode != SQLITE_DONE) {
        LOGE("[SaveDeleteFlagToDB] step failed, %d", errCode);
        (void)sqlite3_finalize(statement);
        return -E_ERROR;
    }
    (void)sqlite3_finalize(statement);
    return E_OK;
}

void ClearTheLogAfterDropTable(sqlite3 *db, const char *tableName, const char *schemaName)
{
    if (db == nullptr || tableName == nullptr || schemaName == nullptr) {
        return;
    }
    auto filePath = sqlite3_db_filename(db, schemaName);
    if (filePath == nullptr) {
        return;
    }
    std::string fileName = std::string(filePath);
    Timestamp dropTimeStamp = TimeHelperManager::GetInstance()->GetTime(fileName, 0, nullptr, nullptr);
    std::string tableStr = std::string(tableName);
    std::string logTblName = DBCommon::GetLogTableName(tableStr);
    if (CheckTableExists(db, logTblName)) {
        if (SaveDeleteFlagToDB(db, tableStr) != E_OK) {
            // the failure of this step does not affect the following step, so we just write log
            LOGW("[ClearTheLogAfterDropTable] save delete flag failed.");
        }
        std::string tableType = DEVICE_TYPE;
        if (GetTableSyncType(db, tableStr, tableType) != DistributedDB::DBStatus::OK) {
            return;
        }
        if (tableType == DEVICE_TYPE) {
            RegisterGetSysTime(db);
            RegisterGetLastTime(db);
            std::string sql = "UPDATE " + logTblName + " SET data_key=-1, flag=0x03, timestamp=get_sys_time(0) "
                "WHERE flag&0x03=0x02 AND timestamp<" + std::to_string(dropTimeStamp);
            (void)sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        } else {
            HandleDropCloudSyncTable(db, tableStr);
        }
    }
}

bool CheckUnLockingDataExists(sqlite3 *db, const std::string &tableName)
{
    sqlite3_stmt *stmt = nullptr;
    std::string sql = "SELECT count(1) FROM " + tableName + " WHERE status=1";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        (void)sqlite3_finalize(stmt);
        return false;
    }

    bool isExists = ((sqlite3_step(stmt) == SQLITE_ROW) && (sqlite3_column_int(stmt, 0) > 0));
    (void)sqlite3_finalize(stmt);
    return isExists;
}

int HandleDataStatus(sqlite3 *db, const std::string &tableName, const std::vector<std::vector<uint8_t>> &hashKey,
    bool isLock)
{
    std::string sql = "UPDATE " + tableName + " SET " + (isLock ? CloudDbConstant::TO_LOCK :
        CloudDbConstant::TO_UNLOCK) + " WHERE hash_key in (";
    for (size_t i = 0; i < hashKey.size(); i++) {
        sql += "?,";
    }
    sql.pop_back();
    sql += ");";

    sqlite3_stmt *stmt = nullptr;
    int errCode = GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Prepare handle status stmt failed:%d, isLock:%d", errCode, isLock);
        return errCode;
    }
    int index = 1;
    for (const auto &hash: hashKey) {
        errCode = BindBlobToStatement(stmt, index++, hash);
        if (errCode != E_OK) {
            (void)ResetStatement(stmt);
            LOGE("Bind handle status stmt failed:%d, index:%d, isLock:%d", errCode, index, isLock);
            return errCode;
        }
    }
    errCode = StepWithRetry(stmt);
    (void)ResetStatement(stmt);
    if (errCode == SQLITE_DONE) {
        if (!isLock && CheckUnLockingDataExists(db, tableName)) {
            return -E_WAIT_COMPENSATED_SYNC;
        }
        if (sqlite3_changes(db) == 0) {
            return -E_NOT_FOUND;
        }
    } else {
        LOGE("step handle status failed:%d, isLock:%d", errCode, isLock);
        return -E_ERROR;
    }
    return E_OK;
}

DistributedDB::DBStatus HandleDataLock(const std::string &tableName, const std::vector<std::vector<uint8_t>> &hashKey,
    sqlite3 *db, bool isLock)
{
    std::string fileName;
    if (!GetDbFileName(db, fileName) || tableName.empty() || hashKey.empty()) {
        return DistributedDB::INVALID_ARGS;
    }
    std::string logTblName = DBCommon::GetLogTableName(tableName);
    if (!CheckTableExists(db, logTblName)) {
        return DistributedDB::INVALID_ARGS;
    }
    int errCode = SQLiteUtils::BeginTransaction(db, TransactType::IMMEDIATE);
    if (errCode != DistributedDB::E_OK) {
        LOGE("begin transaction failed before lock data:%d, isLock:%d", errCode, isLock);
        return DistributedDB::TransferDBErrno(errCode);
    }
    errCode = HandleDataStatus(db, logTblName, hashKey, isLock);
    if (errCode != DistributedDB::E_OK && errCode != -DistributedDB::E_NOT_FOUND &&
        errCode != -DistributedDB::E_WAIT_COMPENSATED_SYNC) {
        int ret = SQLiteUtils::RollbackTransaction(db);
        if (ret != DistributedDB::E_OK) {
            LOGE("rollback failed when lock data:%d, isLock:%d", ret, isLock);
        }
        return DistributedDB::TransferDBErrno(errCode);
    }
    int ret = SQLiteUtils::CommitTransaction(db);
    if (ret != DistributedDB::E_OK) {
        LOGE("commit failed when lock data:%d, isLock:%d", ret, isLock);
    }
    return errCode == DistributedDB::E_OK ? DistributedDB::TransferDBErrno(ret) :
        DistributedDB::TransferDBErrno(errCode);
}

int GetDBIdentity(sqlite3 *db, std::string &identity)
{
    auto filePath = sqlite3_db_filename(db, "main");
    if (filePath == nullptr) {
        return -E_ERROR;
    }
    identity = std::string(filePath);
    return E_OK;
}

void PostHandle(bool isExists, sqlite3 *db)
{
    std::string dbIdentity;
    (void)GetDBIdentity(db, dbIdentity);
    if (!isExists) { // first create db, clean old time helper
        TimeHelperManager::GetInstance()->Restore(dbIdentity);
    }
    TimeHelperManager::GetInstance()->AddStore(dbIdentity);
    RegisterCalcHash(db);
    RegisterGetSysTime(db);
    RegisterGetLastTime(db);
    RegisterGetRawSysTime(db);
    RegisterCloudDataChangeObserver(db);
    RegisterDataChangeObserver(db);
    RegisterCommitAndRollbackHook(db);
    (void)sqlite3_set_droptable_handle(db, &ClearTheLogAfterDropTable);
    (void)sqlite3_busy_timeout(db, BUSY_TIMEOUT);
    std::string recursiveTrigger = "PRAGMA recursive_triggers = ON;";
    (void)ExecuteRawSQL(db, recursiveTrigger);
}

int GetTableInfos(sqlite3 *db, std::map<std::string, bool> &tableInfos)
{
    if (db == nullptr) {
        return -E_ERROR;
    }
    std::string sql = "SELECT name FROM main.sqlite_master where type = 'table'";
    sqlite3_stmt *stmt = nullptr;
    int errCode = GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Prepare get table and primary key statement failed. err=%d", errCode);
        return -E_ERROR;
    }
    while ((errCode = StepWithRetry(stmt)) != SQLITE_DONE) {
        if (errCode != SQLITE_ROW) {
            ResetStatement(stmt);
            return -E_ERROR;
        }
        std::string tableName;
        GetColumnTextValue(stmt, 0, tableName);
        if (tableName.empty() || !ParamCheckUtils::CheckRelationalTableName(tableName) ||
            tableName.find("sqlite_") == 0 || tableName.find("naturalbase_") == 0) {
            continue;
        }
        tableInfos.insert(std::make_pair(tableName, true));
    }
    ResetStatement(stmt);
    for (auto &tableInfo : tableInfos) {
        std::string type = "";
        JudgeIfGetRowid(db, tableInfo.first, type, tableInfo.second);
    }
    return E_OK;
}

int CreateTempTrigger(sqlite3 *db)
{
    std::map<std::string, bool> tableInfos;
    int errCode = GetTableInfos(db, tableInfos);
    if (errCode != E_OK) {
        return errCode;
    }
    std::vector<std::string> triggerSqls;
    errCode = GetTriggerSqls(db, tableInfos, triggerSqls);
    if (errCode != E_OK) {
        return errCode;
    }
    for (const auto &sql : triggerSqls) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            LOGE("Create data change trigger failed %d", errCode);
            return errCode;
        }
    }
    return errCode;
}
}

SQLITE_API int sqlite3_open_relational(const char *filename, sqlite3 **ppDb)
{
    bool isExists = (access(filename, 0) == 0);
    int err = sqlite3_open(filename, ppDb);
    if (err != SQLITE_OK) {
        return err;
    }
    PostHandle(isExists, *ppDb);
    return err;
}

SQLITE_API int sqlite3_open16_relational(const void *filename, sqlite3 **ppDb)
{
    bool isExists = (access(static_cast<const char *>(filename), 0) == 0);
    int err = sqlite3_open16(filename, ppDb);
    if (err != SQLITE_OK) {
        return err;
    }
    PostHandle(isExists, *ppDb);
    return err;
}

SQLITE_API int sqlite3_open_v2_relational(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
{
    bool isExists = (access(filename, 0) == 0);
    int err = sqlite3_open_v2(filename, ppDb, flags, zVfs);
    if (err != SQLITE_OK) {
        return err;
    }
    PostHandle(isExists, *ppDb);
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

DB_API DistributedDB::DBStatus RegisterStoreObserver(sqlite3 *db, const std::shared_ptr<StoreObserver> &storeObserver)
{
    if (storeObserver == nullptr) {
        LOGE("[RegisterStoreObserver] StoreObserver is invalid.");
        return DistributedDB::INVALID_ARGS;
    }

    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        LOGE("[RegisterStoreObserver] Get db filename failed.");
        return DistributedDB::INVALID_ARGS;
    }

    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        LOGE("[RegisterStoreObserver] Get db filename hash string failed.");
        return DistributedDB::DB_ERROR;
    }

    errCode = CreateTempTrigger(db);
    if (errCode != DistributedDB::DBStatus::OK) {
        LOGE("[RegisterStoreObserver] Create trigger failed.");
        return DistributedDB::DB_ERROR;
    }

    std::lock_guard<std::mutex> lock(g_storeObserverMutex);
    if (std::find(g_storeObserverMap[hashFileName].begin(), g_storeObserverMap[hashFileName].end(), storeObserver) !=
        g_storeObserverMap[hashFileName].end()) {
        LOGE("[RegisterStoreObserver] Duplicate observer.");
        return DistributedDB::INVALID_ARGS;
    }
    g_storeObserverMap[hashFileName].push_back(storeObserver);
    return DistributedDB::OK;
}

DB_API DistributedDB::DBStatus UnregisterStoreObserver(sqlite3 *db, const std::shared_ptr<StoreObserver> &storeObserver)
{
    if (storeObserver == nullptr) {
        LOGE("[UnregisterStoreObserver] StoreObserver is invalid.");
        return DistributedDB::INVALID_ARGS;
    }

    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        LOGE("[UnregisterStoreObserver] Get db filename failed.");
        return DistributedDB::INVALID_ARGS;
    }

    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        LOGE("[UnregisterStoreObserver] Get db filename hash string failed.");
        return DistributedDB::DB_ERROR;
    }

    std::lock_guard<std::mutex> lock(g_storeObserverMutex);
    auto it = g_storeObserverMap.find(hashFileName);
    if (it != g_storeObserverMap.end()) {
        it->second.remove(storeObserver);
    }
    if (it->second.empty()) {
        g_storeObserverMap.erase(it);
    }

    return DistributedDB::OK;
}

DB_API DistributedDB::DBStatus UnregisterStoreObserver(sqlite3 *db)
{
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        LOGE("[UnregisterAllStoreObserver] StoreObserver is invalid.");
        return DistributedDB::INVALID_ARGS;
    }

    std::string hashFileName;
    int errCode = GetHashString(fileName, hashFileName);
    if (errCode != DistributedDB::DBStatus::OK) {
        LOGE("[UnregisterAllStoreObserver] Get db filename hash string failed.");
        return DistributedDB::DB_ERROR;
    }

    if (errCode != DistributedDB::DBStatus::OK) {
        return DistributedDB::DB_ERROR;
    }

    std::lock_guard<std::mutex> lock(g_storeObserverMutex);
    auto it = g_storeObserverMap.find(hashFileName);
    if (it != g_storeObserverMap.end()) {
        g_storeObserverMap.erase(it);
    }
    return DistributedDB::OK;
}

DB_API DistributedDB::DBStatus DropLogicDeletedData(sqlite3 *db, const std::string &tableName, uint64_t cursor)
{
    std::string fileName;
    if (!GetDbFileName(db, fileName)) {
        return DistributedDB::INVALID_ARGS;
    }
    if (tableName.empty()) {
        return DistributedDB::INVALID_ARGS;
    }
    int errCode = SQLiteUtils::BeginTransaction(db, TransactType::IMMEDIATE);
    if (errCode != DistributedDB::E_OK) {
        LOGE("begin transaction failed before drop logic deleted data. %d", errCode);
        return DistributedDB::TransferDBErrno(errCode);
    }
    errCode = HandleDropLogicDeleteData(db, tableName, cursor);
    if (errCode != SQLITE_OK) {
        int ret = SQLiteUtils::RollbackTransaction(db);
        if (ret != DistributedDB::E_OK) {
            LOGE("rollback failed when drop logic deleted data. %d", ret);
        }
        return DistributedDB::TransferDBErrno(errCode);
    }
    int ret = SQLiteUtils::CommitTransaction(db);
    if (ret != DistributedDB::E_OK) {
        LOGE("commit failed when drop logic deleted data. %d", ret);
    }
    return ret == DistributedDB::E_OK ? DistributedDB::OK : DistributedDB::TransferDBErrno(ret);
}

DB_API DistributedDB::DBStatus Lock(const std::string &tableName, const std::vector<std::vector<uint8_t>> &hashKey,
    sqlite3 *db)
{
    return HandleDataLock(tableName, hashKey, db, true);
}

DB_API DistributedDB::DBStatus UnLock(const std::string &tableName, const std::vector<std::vector<uint8_t>> &hashKey,
    sqlite3 *db)
{
    return HandleDataLock(tableName, hashKey, db, false);
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
