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

#include "sqlite_utils.h"

#include <climits>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <algorithm>
#include <sys/stat.h>

#include "sqlite_import.h"
#include "securec.h"
#include "db_constant.h"
#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "value_object.h"
#include "schema_utils.h"
#include "schema_constant.h"
#include "sqlite_single_ver_storage_executor_sql.h"
#include "time_helper.h"
#include "platform_specific.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
    std::mutex SQLiteUtils::logMutex_;
    std::string SQLiteUtils::lastErrorMsg_;
namespace {
    const int BIND_KEY_INDEX = 1;
    const int BIND_VAL_INDEX = 2;
    const int USING_STR_LEN = -1;
    const int HEAD_SIZE = 3;
    const int END_SIZE = 3;
    constexpr int MIN_SIZE = HEAD_SIZE + END_SIZE + 3;
    const std::string REPLACE_CHAIN = "***";
    const std::string DEFAULT_ANONYMOUS = "******";
    const std::string WAL_MODE_SQL = "PRAGMA journal_mode=WAL;";
    const std::string SYNC_MODE_FULL_SQL = "PRAGMA synchronous=FULL;";
    const std::string USER_VERSION_SQL = "PRAGMA user_version;";
    const std::string DEFAULT_ATTACH_CIPHER = "PRAGMA cipher_default_attach_cipher=";
    const std::string DEFAULT_ATTACH_KDF_ITER = "PRAGMA cipher_default_attach_kdf_iter=5000";
    const std::string SHA256_ALGO_SQL = "PRAGMA codec_hmac_algo=SHA256;";
    const std::string SHA256_ALGO_REKEY_SQL = "PRAGMA codec_rekey_hmac_algo=SHA256;";
    const std::string SHA1_ALGO_ATTACH_SQL = "PRAGMA cipher_default_attach_hmac_algo=SHA1;";
    const std::string SHA256_ALGO_ATTACH_SQL = "PRAGMA cipher_default_attach_hmac_algo=SHA256;";
    const std::string EXPORT_BACKUP_SQL = "SELECT export_database('backup');";
    const std::string BACK_CIPHER_CONFIG_SQL = "PRAGMA backup.codec_cipher=";
    const std::string BACK_KDF_ITER_CONFIG_SQL = "PRAGMA backup.codec_kdf_iter=5000;";
    const std::string META_CIPHER_CONFIG_SQL = "PRAGMA meta.codec_cipher=";
    const std::string META_KDF_ITER_CONFIG_SQL = "PRAGMA meta.codec_kdf_iter=5000;";

    const constexpr char *DETACH_BACKUP_SQL = "DETACH 'backup'";
    const constexpr char *UPDATE_META_SQL = "INSERT OR REPLACE INTO meta_data VALUES (?, ?);";

    bool g_configLog = false;
    std::mutex g_serverChangedDataMutex;
    std::map<std::string, std::map<std::string, DistributedDB::ChangeProperties>> g_serverChangedDataMap;
}

std::string SQLiteUtils::Anonymous(const std::string &name)
{
    if (name.length() <= HEAD_SIZE) {
        return DEFAULT_ANONYMOUS;
    }

    if (name.length() < MIN_SIZE) {
        return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN);
    }

    return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN + name.substr(name.length() - END_SIZE, END_SIZE));
}

bool IsNeedSkipLog(const unsigned int errType, const char *msg)
{
    return errType == SQLITE_ERROR && strstr(msg, "\"?\": syntax error in \"PRAGMA user_ve") != nullptr;
}

void SQLiteUtils::SqliteLogCallback(void *data, int err, const char *msg)
{
    bool verboseLog = (data != nullptr);
    auto errType = static_cast<unsigned int>(err);
    bool isWarningDump = errType == (SQLITE_WARNING | (2 << 8)); // SQLITE_WARNING_DUMP
    std::string logMsg = msg == nullptr ? "NULL" : msg;
    errType &= 0xFF;
    if (IsNeedSkipLog(errType, logMsg.c_str())) {
        return;
    }
    if (errType == 0 || errType == SQLITE_CONSTRAINT || errType == SQLITE_SCHEMA ||
        errType == SQLITE_NOTICE || err == SQLITE_WARNING_AUTOINDEX) {
        if (verboseLog) {
            LOGD("[SQLite] Error[%d] sys[%d] %s ", err, errno, sqlite3_errstr(err));
        }
    } else if ((errType == SQLITE_WARNING && !isWarningDump) ||
        errType == SQLITE_IOERR || errType == SQLITE_CANTOPEN) {
        LOGI("[SQLite] Error[%d], sys[%d], %s, msg: %s ", err, errno,
            sqlite3_errstr(err), SQLiteUtils::Anonymous(logMsg).c_str());
    } else {
        LOGE("[SQLite] Error[%d], sys[%d], msg: %s ", err, errno, logMsg.c_str());
        return;
    }

    const char *errMsg = sqlite3_errstr(err);
    std::lock_guard<std::mutex> autoLock(logMutex_);
    if (errMsg != nullptr) {
        lastErrorMsg_ = std::string(errMsg);
    }
}

int SQLiteUtils::CreateDataBase(const OpenDbProperties &properties, sqlite3 *&dbTemp, bool setWal)
{
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE;
    if (properties.createIfNecessary) {
        flag |= SQLITE_OPEN_CREATE;
    }
    std::string cipherName = GetCipherName(properties.cipherType);
    if (cipherName.empty()) {
        LOGE("[SQLite] GetCipherName failed");
        return -E_INVALID_ARGS;
    }
    std::string defaultAttachCipher = DEFAULT_ATTACH_CIPHER + cipherName + ";";
    std::vector<std::string> sqls {defaultAttachCipher, DEFAULT_ATTACH_KDF_ITER};
    if (setWal) {
        sqls.push_back(WAL_MODE_SQL);
    }
    std::string fileUrl = DBConstant::SQLITE_URL_PRE + properties.uri;
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &dbTemp, flag, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLite] open database failed: %d - sys err(%d)", errCode, errno);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }

    errCode = SetDataBaseProperty(dbTemp, properties, setWal, sqls);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLite] SetDataBaseProperty failed: %d", errCode);
        goto END;
    }

END:
    if (errCode != E_OK && dbTemp != nullptr) {
        (void)sqlite3_close_v2(dbTemp);
        dbTemp = nullptr;
    }

    return errCode;
}

int SQLiteUtils::OpenDatabase(const OpenDbProperties &properties, sqlite3 *&db, bool setWal)
{
    {
        // Only for register the sqlite3 log callback
        std::lock_guard<std::mutex> lock(logMutex_);
        if (!g_configLog) {
            sqlite3_config(SQLITE_CONFIG_LOG, &SqliteLogCallback, &properties.createIfNecessary);
            sqlite3_config(SQLITE_CONFIG_LOOKASIDE, 0, 0);
            g_configLog = true;
        }
    }
    sqlite3 *dbTemp = nullptr;
    int errCode = CreateDataBase(properties, dbTemp, setWal);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = RegisterJsonFunctions(dbTemp);
    if (errCode != E_OK) {
        goto END;
    }
    // Set the synchroized mode, default for full mode.
    errCode = ExecuteRawSQL(dbTemp, SYNC_MODE_FULL_SQL);
    if (errCode != E_OK) {
        LOGE("SQLite sync mode failed: %d", errCode);
        goto END;
    }

    if (!properties.isMemDb) {
        errCode = SQLiteUtils::SetPersistWalMode(dbTemp);
        if (errCode != E_OK) {
            LOGE("SQLite set persist wall mode failed: %d", errCode);
        }
    }

END:
    if (errCode != E_OK && dbTemp != nullptr) {
        (void)sqlite3_close_v2(dbTemp);
        dbTemp = nullptr;
    }
    if (errCode != E_OK && errno == EKEYREVOKED) {
        errCode = -E_EKEYREVOKED;
    }
    db = dbTemp;
    return errCode;
}

int SQLiteUtils::BindPrefixKey(sqlite3_stmt *statement, int index, const Key &keyPrefix)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    const size_t maxKeySize = DBConstant::MAX_KEY_SIZE;
    // bind the first prefix key
    int errCode = BindBlobToStatement(statement, index, keyPrefix, true);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the prefix first error:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    // bind the second prefix key
    uint8_t end[maxKeySize];
    errno_t status = memset_s(end, maxKeySize, UCHAR_MAX, maxKeySize); // max byte value is 0xFF.
    if (status != EOK) {
        LOGE("memset error:%d", status);
        return -E_SECUREC_ERROR;
    }

    if (!keyPrefix.empty()) {
        status = memcpy_s(end, maxKeySize, keyPrefix.data(), keyPrefix.size());
        if (status != EOK) {
            LOGE("memcpy error:%d", status);
            return -E_SECUREC_ERROR;
        }
    }

    // index wouldn't be too large, just add one to the first index.
    errCode = sqlite3_bind_blob(statement, index + 1, end, maxKeySize, SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the prefix second error:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }
    return E_OK;
}

int SQLiteUtils::SetKey(sqlite3 *db, CipherType type, const CipherPassword &passwd, bool setWal, uint32_t iterTimes)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    if (passwd.GetSize() != 0) {
#ifndef OMIT_ENCRYPT
        int errCode = SetKeyInner(db, type, passwd, iterTimes);
        if (errCode != E_OK) {
            LOGE("[SQLiteUtils][Setkey] set keyInner failed:%d", errCode);
            return errCode;
        }
        errCode = SQLiteUtils::ExecuteRawSQL(db, SHA256_ALGO_SQL);
        if (errCode != E_OK) {
            LOGE("[SQLiteUtils][Setkey] set sha algo failed:%d", errCode);
            return errCode;
        }
        errCode = SQLiteUtils::ExecuteRawSQL(db, SHA256_ALGO_REKEY_SQL);
        if (errCode != E_OK) {
            LOGE("[SQLiteUtils][Setkey] set rekey sha algo failed:%d", errCode);
            return errCode;
        }
#else
        return -E_NOT_SUPPORT;
#endif
    }

    // verify key
    int errCode = SQLiteUtils::ExecuteRawSQL(db, USER_VERSION_SQL);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][Setkey] verify version failed:%d", errCode);
        if (errno == EKEYREVOKED) {
            return -E_EKEYREVOKED;
        }
        if (errCode == -E_BUSY) {
            return errCode;
        }
#ifndef OMIT_ENCRYPT
        if (passwd.GetSize() != 0) {
            errCode = UpdateCipherShaAlgo(db, setWal, type, passwd, iterTimes);
            if (errCode != E_OK) {
                LOGE("[SQLiteUtils][Setkey] upgrade cipher sha algo failed:%d", errCode);
            }
        }
#endif
    }
    return errCode;
}

int SQLiteUtils::AttachNewDatabase(sqlite3 *db, CipherType type, const CipherPassword &password,
    const std::string &attachDbAbsPath, const std::string &attachAsName)
{
#ifndef OMIT_ENCRYPT
    int errCode = SQLiteUtils::ExecuteRawSQL(db, SHA256_ALGO_ATTACH_SQL);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][AttachNewDatabase] set attach sha256 algo failed:%d", errCode);
        return errCode;
    }
#endif
    errCode = AttachNewDatabaseInner(db, type, password, attachDbAbsPath, attachAsName);
#ifndef OMIT_ENCRYPT
    if (errCode == -E_INVALID_PASSWD_OR_CORRUPTED_DB) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, SHA1_ALGO_ATTACH_SQL);
        if (errCode != E_OK) {
            LOGE("[SQLiteUtils][AttachNewDatabase] set attach sha1 algo failed:%d", errCode);
            return errCode;
        }
        errCode = AttachNewDatabaseInner(db, type, password, attachDbAbsPath, attachAsName);
        if (errCode != E_OK) {
            LOGE("[SQLiteUtils][AttachNewDatabase] attach db failed:%d", errCode);
            return errCode;
        }
        errCode = SQLiteUtils::ExecuteRawSQL(db, SHA256_ALGO_ATTACH_SQL);
        if (errCode != E_OK) {
            LOGE("[SQLiteUtils][AttachNewDatabase] set attach sha256 algo failed:%d", errCode);
        }
    }
#endif
    return errCode;
}

int SQLiteUtils::AttachNewDatabaseInner(sqlite3 *db, CipherType type, const CipherPassword &password,
    const std::string &attachDbAbsPath, const std::string &attachAsName)
{
    int errCode = AttachNewDatabaseOnly(db, type, password, attachDbAbsPath, attachAsName);
    if (errCode != E_OK) {
        return errCode;
    }
    return SetPersistWalMode(db, attachAsName);
}

int SQLiteUtils::AttachNewDatabaseOnly(sqlite3 *db, CipherType type, const CipherPassword &password,
    const std::string &attachDbAbsPath, const std::string &attachAsName)
{
    // example: "ATTACH '../new.db' AS backup KEY XXXX;"
    std::string attachSql = "ATTACH ? AS " + attachAsName + " KEY ?;"; // Internal interface not need verify alias name

    sqlite3_stmt* statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, attachSql, statement);
    if (errCode != E_OK) {
        return errCode;
    }
    // 1st is name.
    errCode = sqlite3_bind_text(statement, 1, attachDbAbsPath.c_str(), attachDbAbsPath.length(), SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the attached db name failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }
    // Passwords do not allow vector operations, so we can not use function BindBlobToStatement here.
    errCode = sqlite3_bind_blob(statement, 2, static_cast<const void *>(password.GetData()),  // 2 means password index.
        password.GetSize(), SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the attached key failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("Execute the SQLite attach failed:%d", errCode);
        goto END;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, WAL_MODE_SQL);
    if (errCode != E_OK) {
        LOGE("Set journal mode failed: %d", errCode);
    }

END:
    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteUtils::CreateMetaDatabase(const std::string &metaDbPath)
{
    OpenDbProperties metaProperties {metaDbPath, true, false};
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(metaProperties, db);
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        LOGE("[CreateMetaDatabase] Failed to create the meta database[%d]", errCode);
    }
    if (db != nullptr) { // LCOV_EXCL_BR_LINE
        (void)sqlite3_close_v2(db);
        db = nullptr;
    }
    return errCode;
}

int SQLiteUtils::CheckIntegrity(const std::string &dbFile, CipherType type, const CipherPassword &passwd)
{
    std::vector<std::string> createTableSqls;
    OpenDbProperties option = {dbFile, true, false, createTableSqls, type, passwd};
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(option, db);
    if (errCode != E_OK) {
        LOGE("CheckIntegrity, open db error:%d", errCode);
        return errCode;
    }
    errCode = CheckIntegrity(db, CHECK_DB_INTEGRITY_SQL);
    if (db != nullptr) {
        (void)sqlite3_close_v2(db);
        db = nullptr;
    }
    return errCode;
}

int SQLiteUtils::CheckIntegrity(sqlite3 *db, const std::string &sql)
{
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("Prepare the integrity check statement error:%d", errCode);
        return errCode;
    }
    int resultCnt = 0;
    bool checkResultOK = false;
    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            auto result = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
            if (result == nullptr) {
                continue;
            }
            resultCnt = (resultCnt > 1) ? resultCnt : (resultCnt + 1);
            if (strcmp(result, "ok") == 0) {
                checkResultOK = true;
            }
        } else {
            checkResultOK = false;
            LOGW("Step for the integrity check failed:%d", errCode);
            break;
        }
    } while (true);
    if (resultCnt == 1 && checkResultOK) {
        errCode = E_OK;
    } else {
        errCode = -E_INVALID_PASSWD_OR_CORRUPTED_DB;
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode != E_OK ? errCode : ret;
}

#ifndef OMIT_ENCRYPT
int SQLiteUtils::ExportDatabase(sqlite3 *db, CipherType type, const CipherPassword &passwd,
    const std::string &newDbName)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = AttachNewDatabase(db, type, passwd, newDbName);
    if (errCode != E_OK) {
        LOGE("Attach New Db fail!");
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, EXPORT_BACKUP_SQL);
    if (errCode != E_OK) {
        LOGE("Execute the SQLite export failed:%d", errCode);
    }

    int detachError = SQLiteUtils::ExecuteRawSQL(db, DETACH_BACKUP_SQL);
    if (errCode == E_OK) {
        errCode = detachError;
        if (detachError != E_OK) {
            LOGE("Execute the SQLite detach failed:%d", errCode);
        }
    }
    return errCode;
}

int SQLiteUtils::Rekey(sqlite3 *db, const CipherPassword &passwd)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = sqlite3_rekey(db, static_cast<const void *>(passwd.GetData()), static_cast<int>(passwd.GetSize()));
    if (errCode != E_OK) {
        LOGE("SQLite rekey failed:(%d)", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}
#else
int SQLiteUtils::ExportDatabase(sqlite3 *db, CipherType type, const CipherPassword &passwd,
    const std::string &newDbName)
{
    (void)db;
    (void)type;
    (void)passwd;
    (void)newDbName;
    return -E_NOT_SUPPORT;
}

int SQLiteUtils::Rekey(sqlite3 *db, const CipherPassword &passwd)
{
    (void)db;
    (void)passwd;
    return -E_NOT_SUPPORT;
}
#endif

int SQLiteUtils::GetVersion(const OpenDbProperties &properties, int &version)
{
    if (properties.uri.empty()) { // LCOV_EXCL_BR_LINE
        return -E_INVALID_ARGS;
    }

    sqlite3 *dbTemp = nullptr;
    // Please make sure the database file exists and is working properly
    std::string fileUrl = DBConstant::SQLITE_URL_PRE + properties.uri;
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &dbTemp, SQLITE_OPEN_URI | SQLITE_OPEN_READONLY, nullptr);
    if (errCode != SQLITE_OK) { // LCOV_EXCL_BR_LINE
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        LOGE("Open database failed: %d, sys:%d", errCode, errno);
        goto END;
    }
    // in memory mode no need cipher
    if (!properties.isMemDb) { // LCOV_EXCL_BR_LINE
        errCode = SQLiteUtils::SetKey(dbTemp, properties.cipherType, properties.passwd, false,
            properties.iterTimes);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            LOGE("Set key failed: %d", errCode);
            goto END;
        }
    }

    errCode = GetVersion(dbTemp, version);

END:
    if (dbTemp != nullptr) { // LCOV_EXCL_BR_LINE
        (void)sqlite3_close_v2(dbTemp);
        dbTemp = nullptr;
    }
    return errCode;
}

int SQLiteUtils::GetVersion(sqlite3 *db, int &version)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    std::string strSql = "PRAGMA user_version;";
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_prepare(db, strSql.c_str(), -1, &statement, nullptr);
    if (errCode != SQLITE_OK || statement == nullptr) {
        LOGE("[SqlUtil][GetVer] sqlite3_prepare failed.");
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        return errCode;
    }
    int ret = E_OK;
    errCode = sqlite3_step(statement);
    if (errCode == SQLITE_ROW) {
        // Get pragma user_version at first column
        version = sqlite3_column_int(statement, 0);
    } else {
        LOGE("[SqlUtil][GetVer] Get db user_version failed.");
        ret = SQLiteUtils::MapSQLiteErrno(errCode);
    }
    errCode = E_OK;
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return ret != E_OK ? ret : errCode;
}

int SQLiteUtils::GetJournalMode(sqlite3 *db, std::string &mode)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    std::string sql = "PRAGMA journal_mode;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK || statement == nullptr) {
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = SQLiteUtils::GetColumnTextValue(statement, 0, mode);
    } else {
        LOGE("[SqlUtil][GetJournal] Get db journal_mode failed.");
    }

    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    return errCode != E_OK ? errCode : ret;
}

int SQLiteUtils::SetUserVer(const OpenDbProperties &properties, int version)
{
    if (properties.uri.empty()) { // LCOV_EXCL_BR_LINE
        return -E_INVALID_ARGS;
    }

    // Please make sure the database file exists and is working properly
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(properties, db);
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        return errCode;
    }

    // Set user version
    errCode = SQLiteUtils::SetUserVer(db, version);
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        LOGE("Set user version fail: %d", errCode);
        goto END;
    }

END:
    if (db != nullptr) { // LCOV_EXCL_BR_LINE
        (void)sqlite3_close_v2(db);
        db = nullptr;
    }

    return errCode;
}

int SQLiteUtils::SetUserVer(sqlite3 *db, int version)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }
    std::string userVersionSql = "PRAGMA user_version=" + std::to_string(version) + ";";
    return SQLiteUtils::ExecuteRawSQL(db, userVersionSql);
}

int SQLiteUtils::SetBusyTimeout(sqlite3 *db, int timeout)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    // Set the default busy handler to retry automatically before returning SQLITE_BUSY.
    int errCode = sqlite3_busy_timeout(db, timeout);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLite] set busy timeout failed:%d", errCode);
    }

    return SQLiteUtils::MapSQLiteErrno(errCode);
}

#ifndef OMIT_ENCRYPT
int SQLiteUtils::ExportDatabase(const std::string &srcFile, CipherType type, const CipherPassword &srcPasswd,
    const std::string &targetFile, const CipherPassword &passwd)
{
    std::vector<std::string> createTableSqls;
    OpenDbProperties option = {srcFile, true, false, createTableSqls, type, srcPasswd};
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(option, db);
    if (errCode != E_OK) {
        LOGE("Open db error while exporting:%d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::ExportDatabase(db, type, passwd, targetFile);
    if (db != nullptr) {
        (void)sqlite3_close_v2(db);
        db = nullptr;
    }
    return errCode;
}
#else
int SQLiteUtils::ExportDatabase(const std::string &srcFile, CipherType type, const CipherPassword &srcPasswd,
    const std::string &targetFile, const CipherPassword &passwd)
{
    (void)srcFile;
    (void)type;
    (void)srcPasswd;
    (void)targetFile;
    (void)passwd;
    return -E_NOT_SUPPORT;
}
#endif

int SQLiteUtils::SaveSchema(sqlite3 *db, const std::string &strSchema)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *statement = nullptr;
    std::string sql = "INSERT OR REPLACE INTO meta_data VALUES(?,?);";
    int errCode = GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    Key schemaKey;
    DBCommon::StringToVector(DBConstant::SCHEMA_KEY, schemaKey);
    errCode = BindBlobToStatement(statement, BIND_KEY_INDEX, schemaKey, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    Value schemaValue;
    DBCommon::StringToVector(strSchema, schemaValue);
    errCode = BindBlobToStatement(statement, BIND_VAL_INDEX, schemaValue, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    errCode = StepWithRetry(statement); // memory db does not support schema
    if (errCode != MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("[SqlUtil][SetSchema] StepWithRetry fail, errCode=%d.", errCode);
        ResetStatement(statement, true, errCode);
        return errCode;
    }
    errCode = E_OK;
    ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::GetSchema(sqlite3 *db, std::string &strSchema)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    bool isExists = false;
    int errCode = CheckTableExists(db, "meta_data", isExists);
    if (errCode != E_OK || !isExists) {
        LOGW("[GetSchema] err=%d, meta=%d", errCode, isExists);
        return errCode;
    }

    sqlite3_stmt *statement = nullptr;
    std::string sql = "SELECT value FROM meta_data WHERE key=?;";
    errCode = GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    Key schemakey;
    DBCommon::StringToVector(DBConstant::SCHEMA_KEY, schemakey);
    errCode = BindBlobToStatement(statement, 1, schemakey, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    errCode = StepWithRetry(statement); // memory db does not support schema
    if (errCode == MapSQLiteErrno(SQLITE_DONE)) {
        ResetStatement(statement, true, errCode);
        return -E_NOT_FOUND;
    } else if (errCode != MapSQLiteErrno(SQLITE_ROW)) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    Value schemaValue;
    errCode = GetColumnBlobValue(statement, 0, schemaValue);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }
    DBCommon::VectorToString(schemaValue, strSchema);
    ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::IncreaseIndex(sqlite3 *db, const IndexName &name, const IndexInfo &info, SchemaType type,
    uint32_t skipSize)
{
    if (db == nullptr) {
        LOGE("[IncreaseIndex] Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    if (name.empty() || info.empty()) {
        LOGE("[IncreaseIndex] Name or info can not be empty.");
        return -E_NOT_PERMIT;
    }
    std::string indexName = SchemaUtils::FieldPathString(name);
    std::string sqlCommand = "CREATE INDEX IF NOT EXISTS '" + indexName + "' ON sync_data (";
    for (uint32_t i = 0; i < info.size(); i++) {
        if (i != 0) {
            sqlCommand += ", ";
        }
        std::string extractSql = SchemaObject::GenerateExtractSQL(type, info[i].first, info[i].second,
            skipSize);
        if (extractSql.empty()) { // Unlikely
            LOGE("[IncreaseIndex] GenerateExtractSQL fail at field=%u.", i);
            return -E_INTERNAL_ERROR;
        }
        sqlCommand += extractSql;
    }
    sqlCommand += ") WHERE (flag&0x01=0);";
    return SQLiteUtils::ExecuteRawSQL(db, sqlCommand);
}

int SQLiteUtils::ChangeIndex(sqlite3 *db, const IndexName &name, const IndexInfo &info, SchemaType type,
    uint32_t skipSize)
{
    // Currently we change index by drop it then create it, SQLite "REINDEX" may be used in the future
    int errCode = DecreaseIndex(db, name);
    if (errCode != E_OK) {
        LOGE("[ChangeIndex] Decrease fail=%d.", errCode);
        return errCode;
    }
    errCode = IncreaseIndex(db, name, info, type, skipSize);
    if (errCode != E_OK) {
        LOGE("[ChangeIndex] Increase fail=%d.", errCode);
        return errCode;
    }
    return E_OK;
}

int SQLiteUtils::DecreaseIndex(sqlite3 *db, const IndexName &name)
{
    if (db == nullptr) {
        LOGE("[DecreaseIndex] Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    if (name.empty()) {
        LOGE("[DecreaseIndex] Name can not be empty.");
        return -E_NOT_PERMIT;
    }
    std::string indexName = SchemaUtils::FieldPathString(name);
    std::string sqlCommand = "DROP INDEX IF EXISTS '" + indexName + "';";
    return ExecuteRawSQL(db, sqlCommand);
}

int SQLiteUtils::RegisterJsonFunctions(sqlite3 *db)
{
    if (db == nullptr) {
        LOGE("Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    int errCode = sqlite3_create_function_v2(db, "calc_hash_key", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        nullptr, &CalcHashKey, nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about calc_hash_key returned %d", errCode);
        return MapSQLiteErrno(errCode);
    }
#ifdef USING_DB_JSON_EXTRACT_AUTOMATICALLY
    // Specify need 3 parameter in json_extract_by_path function
    errCode = sqlite3_create_function_v2(db, "json_extract_by_path", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        nullptr, &JsonExtractByPath, nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about json_extract_by_path returned %d", errCode);
        return MapSQLiteErrno(errCode);
    }
#endif
    return E_OK;
}

namespace {
void SchemaObjectDestructor(SchemaObject *inObject)
{
    delete inObject;
    inObject = nullptr;
}
}
#ifdef RELATIONAL_STORE
int SQLiteUtils::RegisterCalcHash(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CalcHash;
    return SQLiteUtils::RegisterFunction(db, "calc_hash", 2, nullptr, func); // 2 is params count
}

void SQLiteUtils::GetSysTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 1 || argv == nullptr) {
        LOGE("Parameter does not meet restrictions.");
        return;
    }

    sqlite3_result_int64(ctx, (sqlite3_int64)TimeHelper::GetSysCurrentTime());
}

void SQLiteUtils::GetRawSysTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 0 || argv == nullptr) {
        LOGE("Parameter does not meet restrictions.");
        return;
    }

    uint64_t curTime = 0;
    int errCode = TimeHelper::GetSysCurrentRawTime(curTime);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "get raw sys time failed in sqlite utils.", errCode);
        return;
    }
    sqlite3_result_int64(ctx, (sqlite3_int64)(curTime));
}

void SQLiteUtils::GetLastTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 0 || argv == nullptr) { // LCOV_EXCL_BR_LINE
        LOGE("Parameter does not meet restrictions.");
        return;
    }
    // Never used internally, just for sql prepare
    sqlite3_result_int64(ctx, (sqlite3_int64)TimeHelper::GetSysCurrentTime());
}

void SQLiteUtils::CloudDataChangedObserver(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 4 || argv == nullptr) { // 4 is param counts
        return;
    }
    sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(1));
}

void SQLiteUtils::CloudDataChangedServerObserver(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 2 || argv == nullptr) { // 2 is param counts
        return;
    }
    sqlite3 *db = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    std::string fileName;
    if (!SQLiteRelationalUtils::GetDbFileName(db, fileName)) {
        return;
    }
    auto tableNameChar = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
    if (tableNameChar == nullptr) {
        return;
    }
    std::string tableName = static_cast<std::string>(tableNameChar);

    uint64_t isTrackerChange = static_cast<uint64_t>(sqlite3_value_int(argv[1])); // 1 is param index
    LOGD("Cloud data changed, server observer callback %u", isTrackerChange);
    {
        std::lock_guard<std::mutex> lock(g_serverChangedDataMutex);
        auto itTable = g_serverChangedDataMap[fileName].find(tableName);
        if (itTable != g_serverChangedDataMap[fileName].end()) {
            itTable->second.isTrackedDataChange =
                (static_cast<uint8_t>(itTable->second.isTrackedDataChange) | isTrackerChange) > 0;
        } else {
            DistributedDB::ChangeProperties properties = { .isTrackedDataChange = (isTrackerChange > 0) };
            g_serverChangedDataMap[fileName].insert_or_assign(tableName, properties);
        }
    }
    sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(1));
}

void SQLiteUtils::GetAndResetServerObserverData(const std::string &dbName, const std::string &tableName,
    ChangeProperties &changeProperties)
{
    std::lock_guard<std::mutex> lock(g_serverChangedDataMutex);
    auto itDb = g_serverChangedDataMap.find(dbName);
    if (itDb != g_serverChangedDataMap.end() && !itDb->second.empty()) {
        auto itTable = itDb->second.find(tableName);
        if (itTable == itDb->second.end()) {
            return;
        }
        changeProperties = itTable->second;
        g_serverChangedDataMap[dbName].erase(itTable);
    }
}

int SQLiteUtils::RegisterGetSysTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetSysTime;
    return SQLiteUtils::RegisterFunction(db, "get_sys_time", 1, nullptr, func);
}

int SQLiteUtils::RegisterGetLastTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetLastTime;
    return SQLiteUtils::RegisterFunction(db, "get_last_time", 0, nullptr, func);
}

int SQLiteUtils::RegisterGetRawSysTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetRawSysTime;
    return SQLiteUtils::RegisterFunction(db, "get_raw_sys_time", 0, nullptr, func);
}

int SQLiteUtils::RegisterCloudDataChangeObserver(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CloudDataChangedObserver;
    return RegisterFunction(db, "client_observer", 4, db, func); // 4 is param counts
}

int SQLiteUtils::RegisterCloudDataChangeServerObserver(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CloudDataChangedServerObserver;
    return RegisterFunction(db, "server_observer", 2, db, func); // 2 is param counts
}

int SQLiteUtils::CreateSameStuTable(sqlite3 *db, const TableInfo &baseTbl, const std::string &newTableName)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS '" + newTableName + "' (";
    const FieldInfoMap &fields = baseTbl.GetFields();
    for (uint32_t cid = 0; cid < fields.size(); ++cid) {
        std::string fieldName = baseTbl.GetFieldName(cid);
        const auto &it = fields.find(fieldName);
        if (it == fields.end()) {
            return -E_INVALID_DB;
        }
        sql += "'" + fieldName + "' '" + it->second.GetDataType() + "'";
        if (it->second.IsNotNull()) {
            sql += " NOT NULL";
        }
        if (it->second.HasDefaultValue()) {
            sql += " DEFAULT " + it->second.GetDefaultValue();
        }
        sql += ",";
    }
    // base table has primary key
    if (!(baseTbl.GetPrimaryKey().size() == 1 && baseTbl.GetPrimaryKey().at(0) == "rowid")) {
        sql += " PRIMARY KEY (";
        for (const auto &it : baseTbl.GetPrimaryKey()) {
            sql += "'" + it.second + "',";
        }
        sql.pop_back();
        sql += "),";
    }
    sql.pop_back();
    sql += ");";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[SQLite] execute create table sql failed");
    }
    return errCode;
}

int SQLiteUtils::CloneIndexes(sqlite3 *db, const std::string &oriTableName, const std::string &newTableName)
{
    std::string sql =
        "SELECT 'CREATE ' || CASE WHEN il.'unique' THEN 'UNIQUE ' ELSE '' END || 'INDEX IF NOT EXISTS ' || '" +
            newTableName + "_' || il.name || ' ON ' || '" + newTableName +
            "' || '(' || GROUP_CONCAT(ii.name) || ');' "
        "FROM sqlite_master AS m,"
            "pragma_index_list(m.name) AS il,"
            "pragma_index_info(il.name) AS ii "
        "WHERE m.type='table' AND m.name='" + oriTableName + "' AND il.origin='c' "
        "GROUP BY il.name;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Prepare the clone sql failed:%d", errCode);
        return errCode;
    }

    std::vector<std::string> indexes;
    while (true) {
        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string indexSql;
            (void)GetColumnTextValue(stmt, 0, indexSql);
            indexes.emplace_back(indexSql);
            continue;
        }
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        }
        (void)ResetStatement(stmt, true, errCode);
        break;
    }

    if (errCode != E_OK) {
        return errCode;
    }

    for (const auto &it : indexes) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, it);
        if (errCode != E_OK) {
            LOGE("[SQLite] execute clone index sql failed");
        }
    }
    return errCode;
}

int SQLiteUtils::GetRelationalSchema(sqlite3 *db, std::string &schema, const std::string &key)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *statement = nullptr;
    std::string sql = "SELECT value FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata WHERE key=?;";
    int errCode = GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    Key schemakey;
    DBCommon::StringToVector(key, schemakey);
    errCode = BindBlobToStatement(statement, 1, schemakey, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    errCode = StepWithRetry(statement);
    if (errCode == MapSQLiteErrno(SQLITE_DONE)) {
        ResetStatement(statement, true, errCode);
        return -E_NOT_FOUND;
    } else if (errCode != MapSQLiteErrno(SQLITE_ROW)) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    Value schemaValue;
    errCode = GetColumnBlobValue(statement, 0, schemaValue);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }
    DBCommon::VectorToString(schemaValue, schema);
    ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::GetLogTableVersion(sqlite3 *db, std::string &version)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *statement = nullptr;
    std::string sql = "SELECT value FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata WHERE key=?;";
    int errCode = GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    Key logTableKey;
    DBCommon::StringToVector(DBConstant::LOG_TABLE_VERSION_KEY, logTableKey);
    errCode = BindBlobToStatement(statement, 1, logTableKey, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    errCode = StepWithRetry(statement);
    if (errCode == MapSQLiteErrno(SQLITE_DONE)) {
        ResetStatement(statement, true, errCode);
        return -E_NOT_FOUND;
    } else if (errCode != MapSQLiteErrno(SQLITE_ROW)) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    Value value;
    errCode = GetColumnBlobValue(statement, 0, value);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }
    DBCommon::VectorToString(value, version);
    ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::RegisterFunction(sqlite3 *db, const std::string &funcName, int nArg, void *uData, TransactFunc &func)
{
    if (db == nullptr) {
        LOGE("Sqlite DB not exists.");
        return -E_INVALID_DB;
    }

    int errCode = sqlite3_create_function_v2(db, funcName.c_str(), nArg, SQLITE_UTF8 | SQLITE_DETERMINISTIC, uData,
        func.xFunc, func.xStep, func.xFinal, func.xDestroy);
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about [%s] returned %d", funcName.c_str(), errCode);
        return MapSQLiteErrno(errCode);
    }
    return E_OK;
}
#endif
int SQLiteUtils::RegisterFlatBufferFunction(sqlite3 *db, const std::string &inSchema)
{
    if (db == nullptr) {
        LOGE("Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    auto heapSchemaObj = new (std::nothrow) SchemaObject;
    if (heapSchemaObj == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    int errCode = heapSchemaObj->ParseFromSchemaString(inSchema);
    if (errCode != E_OK) { // Unlikely, it has been parsed before
        delete heapSchemaObj;
        heapSchemaObj = nullptr;
        return -E_INTERNAL_ERROR;
    }
    if (heapSchemaObj->GetSchemaType() != SchemaType::FLATBUFFER) { // Do not need to register FlatBufferExtract
        delete heapSchemaObj;
        heapSchemaObj = nullptr;
        return E_OK;
    }
    errCode = sqlite3_create_function_v2(db, SchemaObject::GetExtractFuncName(SchemaType::FLATBUFFER).c_str(),
        3, SQLITE_UTF8 | SQLITE_DETERMINISTIC, heapSchemaObj, &FlatBufferExtractByPath, nullptr, nullptr, // 3 args
        reinterpret_cast<void(*)(void*)>(SchemaObjectDestructor));
    // About the release of heapSchemaObj: SQLite guarantee that at following case, sqlite will invoke the destructor
    // (that is SchemaObjectDestructor) we passed to it. See sqlite.org for more information.
    // The destructor is invoked when the function is deleted, either by being overloaded or when the database
    // connection closes. The destructor is also invoked if the call to sqlite3_create_function_v2() fails
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about flatbuffer_extract_by_path return=%d.", errCode);
        // As mentioned above, SQLite had invoked the SchemaObjectDestructor to release the heapSchemaObj
        return MapSQLiteErrno(errCode);
    }
    return E_OK;
}

void SQLiteUtils::UpdateMetaDataWithinTrigger(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 2 || argv == nullptr) { // 2 : Number of parameters for sqlite register function
        LOGE("[UpdateMetaDataWithinTrigger] Invalid parameter, argc=%d.", argc);
        return;
    }
    auto *handle = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    if (handle == nullptr) {
        sqlite3_result_error(ctx, "Sqlite context is invalid.", USING_STR_LEN);
        LOGE("Sqlite context is invalid.");
        return;
    }
    auto *keyPtr = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0])); // 0 : first argv for key
    int keyLen = sqlite3_value_bytes(argv[0]); // 0 : first argv for key
    if (keyPtr == nullptr || keyLen <= 0 || keyLen > static_cast<int>(DBConstant::MAX_KEY_SIZE)) {
        sqlite3_result_error(ctx, "key is invalid.", USING_STR_LEN);
        LOGE("key is invalid.");
        return;
    }
    auto val = sqlite3_value_int64(argv[1]); // 1 : second argv for value

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(handle, UPDATE_META_SQL, stmt);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Get update meta_data statement failed.", USING_STR_LEN);
        LOGE("Get update meta_data statement failed. %d", errCode);
        return;
    }

    Key key(keyPtr, keyPtr + keyLen);
    errCode = SQLiteUtils::BindBlobToStatement(stmt, BIND_KEY_INDEX, key, false);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Bind key to statement failed.", USING_STR_LEN);
        LOGE("Bind key to statement failed. %d", errCode);
        goto END;
    }

    errCode = SQLiteUtils::BindInt64ToStatement(stmt, BIND_VAL_INDEX, val);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Bind value to statement failed.", USING_STR_LEN);
        LOGE("Bind value to statement failed. %d", errCode);
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        sqlite3_result_error(ctx, "Execute the update meta_data attach failed.", USING_STR_LEN);
        LOGE("Execute the update meta_data attach failed. %d", errCode);
    }
END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

int SQLiteUtils::RegisterMetaDataUpdateFunction(sqlite3 *db)
{
    int errCode = sqlite3_create_function_v2(db, DBConstant::UPDATE_META_FUNC,
        2, // 2: argc for register function
        SQLITE_UTF8 | SQLITE_DETERMINISTIC, db, &SQLiteUtils::UpdateMetaDataWithinTrigger, nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about update_meta_within_trigger returned %d", errCode);
    }
    return SQLiteUtils::MapSQLiteErrno(errCode);
}
} // namespace DistributedDB
