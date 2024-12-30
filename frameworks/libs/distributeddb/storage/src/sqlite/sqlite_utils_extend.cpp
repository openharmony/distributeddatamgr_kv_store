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

#include "sqlite_import.h"
#include "securec.h"
#include "db_constant.h"
#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "value_object.h"
#include "schema_utils.h"
#include "schema_constant.h"
#include "time_helper.h"
#include "platform_specific.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
namespace {
    const int BUSY_TIMEOUT_MS = 3000; // 3000ms for sqlite busy timeout.
    const int USING_STR_LEN = -1;
    const std::string CIPHER_CONFIG_SQL = "PRAGMA codec_cipher=";
    const std::string KDF_ITER_CONFIG_SQL = "PRAGMA codec_kdf_iter=";
    const std::string USER_VERSION_SQL = "PRAGMA user_version;";
    const std::string WAL_MODE_SQL = "PRAGMA journal_mode=WAL;";
    const std::string SHA1_ALGO_SQL = "PRAGMA codec_hmac_algo=SHA1;";
    const std::string SHA256_ALGO_REKEY_SQL = "PRAGMA codec_rekey_hmac_algo=SHA256;";

    const constexpr char *CHECK_TABLE_CREATED = "SELECT EXISTS(SELECT 1 FROM sqlite_master WHERE " \
        "type='table' AND (tbl_name=? COLLATE NOCASE));";
    const constexpr char *CHECK_META_DB_TABLE_CREATED = "SELECT EXISTS(SELECT 1 FROM meta.sqlite_master WHERE " \
        "type='table' AND (tbl_name=? COLLATE NOCASE));";
}

struct ValueParseCache {
    ValueObject valueParsed;
    std::vector<uint8_t> valueOriginal;
};

namespace {
inline bool IsDeleteRecord(const uint8_t *valueBlob, int valueBlobLen)
{
    return (valueBlob == nullptr) || (valueBlobLen <= 0); // In fact, sqlite guarantee valueBlobLen not negative
}

// Use the same cache id as sqlite use for json_extract which is substituted by our json_extract_by_path
// A negative cache-id enables sharing of cache between different operation during the same statement
constexpr int VALUE_CACHE_ID = -429938;

void ValueParseCacheFree(ValueParseCache *inCache)
{
    if (inCache != nullptr) {
        delete inCache;
    }
}

// We don't use cache array since we only cache value column of sqlite table, see sqlite implementation for compare.
const ValueObject *ParseValueThenCacheOrGetFromCache(sqlite3_context *ctx, const uint8_t *valueBlob,
    uint32_t valueBlobLen, uint32_t offset)
{
    // Note: All parameter had already been check inside JsonExtractByPath, only called by JsonExtractByPath
    auto cached = static_cast<ValueParseCache *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID));
    if (cached != nullptr) { // A previous cache exist
        if (cached->valueOriginal.size() == valueBlobLen) {
            if (std::memcmp(cached->valueOriginal.data(), valueBlob, valueBlobLen) == 0) {
                // Cache match
                return &(cached->valueParsed);
            }
        }
    }
    // No cache or cache mismatch
    auto newCache = new (std::nothrow) ValueParseCache;
    if (newCache == nullptr) {
        sqlite3_result_error(ctx, "[ParseValueCache] OOM.", USING_STR_LEN);
        LOGE("[ParseValueCache] OOM.");
        return nullptr;
    }
    int errCode = newCache->valueParsed.Parse(valueBlob, valueBlob + valueBlobLen, offset);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "[ParseValueCache] Parse fail.", USING_STR_LEN);
        LOGE("[ParseValueCache] Parse fail, errCode=%d.", errCode);
        delete newCache;
        newCache = nullptr;
        return nullptr;
    }
    newCache->valueOriginal.assign(valueBlob, valueBlob + valueBlobLen);
    sqlite3_set_auxdata(ctx, VALUE_CACHE_ID, newCache, reinterpret_cast<void(*)(void*)>(ValueParseCacheFree));
    // If sqlite3_set_auxdata fail, it will immediately call ValueParseCacheFree to delete newCache;
    // Next time sqlite3_set_auxdata will call ValueParseCacheFree to delete newCache of this time;
    // At the end, newCache will be eventually deleted when call sqlite3_reset or sqlite3_finalize;
    // Since sqlite3_set_auxdata may fail, we have to call sqlite3_get_auxdata other than return newCache directly.
    auto cacheInAuxdata = static_cast<ValueParseCache *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID));
    if (cacheInAuxdata == nullptr) {
        return nullptr;
    }
    return &(cacheInAuxdata->valueParsed);
}
}

void SQLiteUtils::JsonExtractByPath(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 3 || argv == nullptr) { // 3 parameters, which are value, path and offset
        LOGE("[JsonExtract] Invalid parameter, argc=%d.", argc);
        return;
    }
    auto valueBlob = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0]));
    int valueBlobLen = sqlite3_value_bytes(argv[0]);
    if (IsDeleteRecord(valueBlob, valueBlobLen)) {
        // Currently delete records are filtered out of query and create-index sql, so not allowed here.
        sqlite3_result_error(ctx, "[JsonExtract] Delete record not allowed.", USING_STR_LEN);
        LOGE("[JsonExtract] Delete record not allowed.");
        return;
    }
    auto path = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
    int offset = sqlite3_value_int(argv[2]); // index 2 is the third parameter
    if ((path == nullptr) || (offset < 0)) {
        sqlite3_result_error(ctx, "[JsonExtract] Path nullptr or offset invalid.", USING_STR_LEN);
        LOGE("[JsonExtract] Path nullptr or offset=%d invalid.", offset);
        return;
    }
    FieldPath outPath;
    int errCode = SchemaUtils::ParseAndCheckFieldPath(path, outPath);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "[JsonExtract] Path illegal.", USING_STR_LEN);
        LOGE("[JsonExtract] Path illegal.");
        return;
    }
    // Parameter Check Done Here
    const ValueObject *valueObj = ParseValueThenCacheOrGetFromCache(ctx, valueBlob, static_cast<uint32_t>(valueBlobLen),
        static_cast<uint32_t>(offset));
    if (valueObj == nullptr) {
        return; // Necessary had been printed in ParseValueThenCacheOrGetFromCache
    }
    JsonExtractInnerFunc(ctx, *valueObj, outPath);
}

namespace {
inline bool IsExtractableType(FieldType inType)
{
    return (inType != FieldType::LEAF_FIELD_NULL && inType != FieldType::LEAF_FIELD_ARRAY &&
        inType != FieldType::LEAF_FIELD_OBJECT && inType != FieldType::INTERNAL_FIELD_OBJECT);
}
}

void SQLiteUtils::JsonExtractInnerFunc(sqlite3_context *ctx, const ValueObject &inValue, const FieldPath &inPath)
{
    FieldType outType = FieldType::LEAF_FIELD_NULL; // Default type null for invalid-path(path not exist)
    int errCode = inValue.GetFieldTypeByFieldPath(inPath, outType);
    if (errCode != E_OK && errCode != -E_INVALID_PATH) {
        sqlite3_result_error(ctx, "[JsonExtract] GetFieldType fail.", USING_STR_LEN);
        LOGE("[JsonExtract] GetFieldType fail, errCode=%d.", errCode);
        return;
    }
    FieldValue outValue;
    if (IsExtractableType(outType)) {
        errCode = inValue.GetFieldValueByFieldPath(inPath, outValue);
        if (errCode != E_OK) {
            sqlite3_result_error(ctx, "[JsonExtract] GetFieldValue fail.", USING_STR_LEN);
            LOGE("[JsonExtract] GetFieldValue fail, errCode=%d.", errCode);
            return;
        }
    }
    // FieldType null, array, object do not have value, all these FieldValue will be regarded as null in JsonReturn.
    ExtractReturn(ctx, outType, outValue);
}

// NOTE!!! This function is performance sensitive !!! Carefully not to allocate memory often!!!
void SQLiteUtils::FlatBufferExtractByPath(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 3 || argv == nullptr) { // 3 parameters, which are value, path and offset
        LOGE("[FlatBufferExtract] Invalid parameter, argc=%d.", argc);
        return;
    }
    auto schema = static_cast<SchemaObject *>(sqlite3_user_data(ctx));
    if (schema == nullptr || !schema->IsSchemaValid() ||
        (schema->GetSchemaType() != SchemaType::FLATBUFFER)) { // LCOV_EXCL_BR_LINE
        sqlite3_result_error(ctx, "[FlatBufferExtract] No SchemaObject or invalid.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] No SchemaObject or invalid.");
        return;
    }
    // Get information from argv
    auto valueBlob = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0]));
    int valueBlobLen = sqlite3_value_bytes(argv[0]);
    if (IsDeleteRecord(valueBlob, valueBlobLen)) { // LCOV_EXCL_BR_LINE
        // Currently delete records are filtered out of query and create-index sql, so not allowed here.
        sqlite3_result_error(ctx, "[FlatBufferExtract] Delete record not allowed.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] Delete record not allowed.");
        return;
    }
    auto path = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
    int offset = sqlite3_value_int(argv[2]); // index 2 is the third parameter
    if ((path == nullptr) || (offset < 0) ||
        (static_cast<uint32_t>(offset) != schema->GetSkipSize())) { // LCOV_EXCL_BR_LINE
        sqlite3_result_error(ctx, "[FlatBufferExtract] Path null or offset invalid.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] Path null or offset=%d(skipsize=%u) invalid.", offset, schema->GetSkipSize());
        return;
    }
    FlatBufferExtractInnerFunc(ctx, *schema, RawValue { valueBlob, valueBlobLen }, path);
}

namespace {
constexpr uint32_t FLATBUFFER_MAX_CACHE_SIZE = 102400; // 100 KBytes

void FlatBufferCacheFree(std::vector<uint8_t> *inCache)
{
    if (inCache != nullptr) {
        delete inCache;
    }
}
}

void SQLiteUtils::FlatBufferExtractInnerFunc(sqlite3_context *ctx, const SchemaObject &schema, const RawValue &inValue,
    RawString inPath)
{
    // All parameter had already been check inside FlatBufferExtractByPath, only called by FlatBufferExtractByPath
    if (schema.GetSkipSize() % SchemaConstant::SECURE_BYTE_ALIGN == 0) { // LCOV_EXCL_BR_LINE
        TypeValue outExtract;
        int errCode = schema.ExtractValue(ValueSource::FROM_DBFILE, inPath, inValue, outExtract, nullptr);
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            sqlite3_result_error(ctx, "[FlatBufferExtract] ExtractValue fail.", USING_STR_LEN);
            LOGE("[FlatBufferExtract] ExtractValue fail, errCode=%d.", errCode);
            return;
        }
        ExtractReturn(ctx, outExtract.first, outExtract.second);
        return;
    }
    // Not byte-align secure, we have to make a cache for copy. Check whether cache had already exist.
    auto cached = static_cast<std::vector<uint8_t> *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID)); // Share the same id
    if (cached == nullptr) { // LCOV_EXCL_BR_LINE
        // Make the cache
        auto newCache = new (std::nothrow) std::vector<uint8_t>;
        if (newCache == nullptr) {
            sqlite3_result_error(ctx, "[FlatBufferExtract] OOM.", USING_STR_LEN);
            LOGE("[FlatBufferExtract] OOM.");
            return;
        }
        newCache->resize(FLATBUFFER_MAX_CACHE_SIZE);
        sqlite3_set_auxdata(ctx, VALUE_CACHE_ID, newCache, reinterpret_cast<void(*)(void*)>(FlatBufferCacheFree));
        // If sqlite3_set_auxdata fail, it will immediately call FlatBufferCacheFree to delete newCache;
        // Next time sqlite3_set_auxdata will call FlatBufferCacheFree to delete newCache of this time;
        // At the end, newCache will be eventually deleted when call sqlite3_reset or sqlite3_finalize;
        // Since sqlite3_set_auxdata may fail, we have to call sqlite3_get_auxdata other than return newCache directly.
        // See sqlite.org for more information.
        cached = static_cast<std::vector<uint8_t> *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID));
    }
    if (cached == nullptr) { // LCOV_EXCL_BR_LINE
        LOGW("[FlatBufferExtract] Something wrong with Auxdata, but it is no matter without cache.");
    }
    TypeValue outExtract;
    int errCode = schema.ExtractValue(ValueSource::FROM_DBFILE, inPath, inValue, outExtract, cached);
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        sqlite3_result_error(ctx, "[FlatBufferExtract] ExtractValue fail.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] ExtractValue fail, errCode=%d.", errCode);
        return;
    }
    ExtractReturn(ctx, outExtract.first, outExtract.second);
}

void SQLiteUtils::ExtractReturn(sqlite3_context *ctx, FieldType type, const FieldValue &value)
{
    if (ctx == nullptr) {
        return;
    }
    switch (type) {
        case FieldType::LEAF_FIELD_BOOL:
            sqlite3_result_int(ctx, (value.boolValue ? 1 : 0));
            break;
        case FieldType::LEAF_FIELD_INTEGER:
            sqlite3_result_int(ctx, value.integerValue);
            break;
        case FieldType::LEAF_FIELD_LONG:
            sqlite3_result_int64(ctx, value.longValue);
            break;
        case FieldType::LEAF_FIELD_DOUBLE:
            sqlite3_result_double(ctx, value.doubleValue);
            break;
        case FieldType::LEAF_FIELD_STRING:
            // The SQLITE_TRANSIENT value means that the content will likely change in the near future and
            // that SQLite should make its own private copy of the content before returning.
            sqlite3_result_text(ctx, value.stringValue.c_str(), -1, SQLITE_TRANSIENT); // -1 mean use the string length
            break;
        default:
            // All other type regard as null
            sqlite3_result_null(ctx);
    }
    return;
}

static void CalcHashFunc(sqlite3_context *ctx, sqlite3_value **argv)
{
    auto keyBlob = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0]));
    if (keyBlob == nullptr) {
        sqlite3_result_error(ctx, "Parameters is invalid.", USING_STR_LEN);
        LOGE("Parameters is invalid.");
        return;
    }
    int blobLen = sqlite3_value_bytes(argv[0]);
    std::vector<uint8_t> value(keyBlob, keyBlob + blobLen);
    std::vector<uint8_t> hashValue;
    int errCode = DBCommon::CalcValueHash(value, hashValue);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Get hash value error.", USING_STR_LEN);
        LOGE("Get hash value error.");
        return;
    }
    sqlite3_result_blob(ctx, hashValue.data(), hashValue.size(), SQLITE_TRANSIENT);
}

void SQLiteUtils::CalcHashKey(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    // 1 means that the function only needs one parameter, namely key
    if (ctx == nullptr || argc != 1 || argv == nullptr) {
        LOGE("Parameter does not meet restrictions.");
        return;
    }
    CalcHashFunc(ctx, argv);
}

void SQLiteUtils::CalcHash(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 2 || argv == nullptr) { // 2 is params count
        LOGE("Parameter does not meet restrictions.");
        return;
    }
    CalcHashFunc(ctx, argv);
}


int SQLiteUtils::GetDbSize(const std::string &dir, const std::string &dbName, uint64_t &size)
{
    std::string dataDir = dir + "/" + dbName + DBConstant::DB_EXTENSION;
    uint64_t localDbSize = 0;
    int errCode = OS::CalFileSize(dataDir, localDbSize);
    if (errCode != E_OK) {
        LOGD("Failed to get the db file size, errCode:%d", errCode);
        return errCode;
    }

    std::string shmFileName = dataDir + "-shm";
    uint64_t localshmFileSize = 0;
    errCode = OS::CalFileSize(shmFileName, localshmFileSize);
    if (errCode != E_OK) {
        localshmFileSize = 0;
    }

    std::string walFileName = dataDir + "-wal";
    uint64_t localWalFileSize = 0;
    errCode = OS::CalFileSize(walFileName, localWalFileSize);
    if (errCode != E_OK) {
        localWalFileSize = 0;
    }

    // 64-bit system is Suffice. Computer storage is less than uint64_t max
    size += (localDbSize + localshmFileSize + localWalFileSize);
    return E_OK;
}

int SQLiteUtils::SetDataBaseProperty(sqlite3 *db, const OpenDbProperties &properties, bool setWal,
    const std::vector<std::string> &sqls)
{
    // Set the default busy handler to retry automatically before returning SQLITE_BUSY.
    int errCode = SetBusyTimeout(db, BUSY_TIMEOUT_MS);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!properties.isMemDb) {
        errCode = SQLiteUtils::SetKey(db, properties.cipherType, properties.passwd, setWal,
            properties.iterTimes);
        if (errCode != E_OK) {
            LOGD("SQLiteUtils::SetKey fail!!![%d]", errCode);
            return errCode;
        }
    }

    for (const auto &sql : sqls) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            LOGE("[SQLite] execute sql failed: %d", errCode);
            return errCode;
        }
    }
    // Create table if not exist according the sqls.
    if (properties.createIfNecessary) {
        for (const auto &sql : properties.sqls) {
            errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
            if (errCode != E_OK) {
                LOGE("[SQLite] execute preset sqls failed");
                return errCode;
            }
        }
    }
    return E_OK;
}

#ifndef OMIT_ENCRYPT
int SQLiteUtils::SetCipherSettings(sqlite3 *db, CipherType type, uint32_t iterTimes)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }
    std::string cipherName = GetCipherName(type);
    if (cipherName.empty()) {
        return -E_INVALID_ARGS;
    }
    std::string cipherConfig = CIPHER_CONFIG_SQL + cipherName + ";";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, cipherConfig);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][SetCipherSettings] config cipher failed:%d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, KDF_ITER_CONFIG_SQL + std::to_string(iterTimes));
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][SetCipherSettings] config iter failed:%d", errCode);
    }
    return errCode;
}

std::string SQLiteUtils::GetCipherName(CipherType type)
{
    if (type == CipherType::AES_256_GCM || type == CipherType::DEFAULT) {
        return "'aes-256-gcm'";
    }
    return "";
}
#endif

int SQLiteUtils::DropTriggerByName(sqlite3 *db, const std::string &name)
{
    const std::string dropTriggerSql = "DROP TRIGGER " + name + ";";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, dropTriggerSql);
    if (errCode != E_OK) {
        LOGE("Remove trigger failed. %d", errCode);
    }
    return errCode;
}

int SQLiteUtils::ExpandedSql(sqlite3_stmt *stmt, std::string &basicString)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    char *eSql = sqlite3_expanded_sql(stmt);
    if (eSql == nullptr) {
        LOGE("expand statement to sql failed.");
        return -E_INVALID_DATA;
    }
    basicString = std::string(eSql);
    sqlite3_free(eSql);
    return E_OK;
}

void SQLiteUtils::ExecuteCheckPoint(sqlite3 *db)
{
    if (db == nullptr) {
        return;
    }

    int chkResult = sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
    LOGI("SQLite checkpoint result:%d", chkResult);
}

int SQLiteUtils::CheckTableEmpty(sqlite3 *db, const std::string &tableName, bool &isEmpty)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    std::string cntSql = "SELECT min(rowid) FROM '" + tableName + "';";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, cntSql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        isEmpty = (sqlite3_column_type(stmt, 0) == SQLITE_NULL);
        errCode = E_OK;
    }

    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

int SQLiteUtils::SetPersistWalMode(sqlite3 *db)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }
    int opCode = 1;
    int errCode = sqlite3_file_control(db, "main", SQLITE_FCNTL_PERSIST_WAL, &opCode);
    if (errCode != SQLITE_OK) {
        LOGE("Set persist wal mode failed. %d", errCode);
    }
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

int64_t SQLiteUtils::GetLastRowId(sqlite3 *db)
{
    if (db == nullptr) {
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}

std::string SQLiteUtils::GetLastErrorMsg()
{
    std::lock_guard<std::mutex> autoLock(logMutex_);
    return lastErrorMsg_;
}

int SQLiteUtils::SetAuthorizer(sqlite3 *db,
    int (*xAuth)(void*, int, const char*, const char*, const char*, const char*))
{
    return SQLiteUtils::MapSQLiteErrno(sqlite3_set_authorizer(db, xAuth, nullptr));
}

void SQLiteUtils::GetSelectCols(sqlite3_stmt *stmt, std::vector<std::string> &colNames)
{
    colNames.clear();
    for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
        const char *name = sqlite3_column_name(stmt, i);
        colNames.emplace_back(name == nullptr ? std::string() : std::string(name));
    }
}

int SQLiteUtils::SetKeyInner(sqlite3 *db, CipherType type, const CipherPassword &passwd, uint32_t iterTimes)
{
#ifndef OMIT_ENCRYPT
    int errCode = sqlite3_key(db, static_cast<const void *>(passwd.GetData()), static_cast<int>(passwd.GetSize()));
    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtils][SetKeyInner] config key failed:(%d)", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    errCode = SQLiteUtils::SetCipherSettings(db, type, iterTimes);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][SetKeyInner] set cipher settings failed:%d", errCode);
    }
    return errCode;
#else
    return -E_NOT_SUPPORT;
#endif
}

int SQLiteUtils::BindDataValueByType(sqlite3_stmt *statement, const std::optional<DataValue> &data, int cid)
{
    int errCode = E_OK;
    StorageType type = data.value_or(DataValue()).GetType();
    switch (type) {
        case StorageType::STORAGE_TYPE_INTEGER: {
            int64_t intData = 0;
            (void)data.value().GetInt64(intData);
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_int64(statement, cid, intData));
            break;
        }

        case StorageType::STORAGE_TYPE_REAL: {
            double doubleData = 0;
            (void)data.value().GetDouble(doubleData);
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_double(statement, cid, doubleData));
            break;
        }

        case StorageType::STORAGE_TYPE_TEXT: {
            std::string strData;
            (void)data.value().GetText(strData);
            errCode = SQLiteUtils::BindTextToStatement(statement, cid, strData);
            break;
        }

        case StorageType::STORAGE_TYPE_BLOB: {
            Blob blob;
            (void)data.value().GetBlob(blob);
            std::vector<uint8_t> blobData(blob.GetData(), blob.GetData() + blob.GetSize());
            errCode = SQLiteUtils::BindBlobToStatement(statement, cid, blobData, true);
            break;
        }

        case StorageType::STORAGE_TYPE_NULL: {
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(statement, cid));
            break;
        }

        default:
            break;
    }
    return errCode;
}

int SQLiteUtils::UpdateCipherShaAlgo(sqlite3 *db, bool setWal, CipherType type, const CipherPassword &passwd,
    uint32_t iterTimes)
{
    int errCode = SetKeyInner(db, type, passwd, iterTimes);
    if (errCode != E_OK) {
        return errCode;
    }
    // set sha1 algo for old version
    errCode = SQLiteUtils::ExecuteRawSQL(db, SHA1_ALGO_SQL);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][UpdateCipherShaAlgo] set sha algo failed:%d", errCode);
        return errCode;
    }
    // try to get user version
    errCode = SQLiteUtils::ExecuteRawSQL(db, USER_VERSION_SQL);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][UpdateCipherShaAlgo] verify version failed:%d", errCode);
        if (errno == EKEYREVOKED) {
            return -E_EKEYREVOKED;
        }
        return errCode;
    }
    // try to update rekey sha algo by rekey operation
    errCode = SQLiteUtils::ExecuteRawSQL(db, SHA256_ALGO_REKEY_SQL);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][UpdateCipherShaAlgo] set rekey sha algo failed:%d", errCode);
        return errCode;
    }
    if (setWal) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, WAL_MODE_SQL);
        if (errCode != E_OK) {
            LOGE("[SQLite][UpdateCipherShaAlgo] execute wal sql failed: %d", errCode);
            return errCode;
        }
    }
    return Rekey(db, passwd);
}

int SQLiteUtils::CheckTableExists(sqlite3 *db, const std::string &tableName, bool &isCreated, bool isCheckMeta)
{
    if (db == nullptr) {
        return -1;
    }

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, isCheckMeta ? CHECK_META_DB_TABLE_CREATED : CHECK_TABLE_CREATED, stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("Get check table statement failed. err=%d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::BindTextToStatement(stmt, 1, tableName);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_OK)) {
        LOGE("Bind table name to statement failed. err=%d", errCode);
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        LOGE("Check table exists failed. err=%d", errCode); // should always return a row data
        goto END;
    }
    errCode = E_OK;
    isCreated = (sqlite3_column_int(stmt, 0) == 1);
END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

int SQLiteUtils::StepNext(sqlite3_stmt *stmt, bool isMemDb)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_FINISHED;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = E_OK;
    }
    return errCode;
}

int SQLiteUtils::GetCountBySql(sqlite3 *db, const std::string &sql, int &count)
{
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][GetCountBySql] Get stmt failed when get local data count: %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        count = static_cast<int>(sqlite3_column_int(stmt, 0));
        errCode = E_OK;
    } else {
        LOGE("[SQLiteUtils][GetCountBySql] Query local data count failed: %d", errCode);
    }
    int ret = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, ret);
    if (ret != E_OK) {
        LOGE("[SQLiteUtils][GetCountBySql] Reset stmt failed when get local data count: %d", ret);
    }
    return errCode != E_OK ? errCode : ret;
}
} // namespace DistributedDB
