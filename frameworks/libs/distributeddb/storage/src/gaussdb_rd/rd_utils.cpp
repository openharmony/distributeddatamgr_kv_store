/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "rd_utils.h"
#include "db_errno.h"
#include "log_print.h"
#include "sqlite_single_ver_storage_executor_sql.h"

namespace {
    using namespace DistributedDB;

    bool CheckRdOptionMode(const KvStoreNbDelegate::Option &option)
    {
        return (option.mode != 0 && option.mode != 1) || option.syncDualTupleMode;
    }

    bool CheckOption(const KvStoreNbDelegate::Option &option)
    {
        if (option.storageEngineType == GAUSSDB_RD &&
            (CheckRdOptionMode(option) ||
            option.isMemoryDb ||
            option.isEncryptedDb ||
            option.cipher != CipherType::DEFAULT ||
            option.passwd != CipherPassword() ||
            !option.schema.empty() ||
            option.conflictType != 0 ||
            option.notifier != nullptr ||
            option.conflictResolvePolicy != LAST_WIN ||
            option.isNeedCompressOnSync ||
            option.compressionRate != 100 ||    // Valid in [1, 100].
            option.localOnly)) {
            return false;
        }
        return true;
    }
}

namespace DistributedDB {

std::string InitRdConfig()
{
    return R"("redoFlushByTrx": 1, "maxConnNum": 100, "crcCheckEnable": 0, "bufferPoolPolicy": "BUF_PRIORITY_INDEX")";
}

struct GrdErrnoPair {
    int32_t grdCode;
    int kvDbCode;
};

const GrdErrnoPair GRD_ERRNO_MAP[] = {
    { GRD_OK, E_OK },
    { GRD_NOT_SUPPORT, -E_NOT_SUPPORT },
    { GRD_OVER_LIMIT, -E_MAX_LIMITS },
    { GRD_INVALID_ARGS, -E_INVALID_ARGS },
    { GRD_FAILED_FILE_OPERATION, -E_SYSTEM_API_FAIL },
    { GRD_INVALID_FILE_FORMAT, -E_INVALID_PASSWD_OR_CORRUPTED_DB },
    { GRD_INSUFFICIENT_SPACE, -E_INTERNAL_ERROR },
    { GRD_INNER_ERR, -E_INTERNAL_ERROR },
    { GRD_RESOURCE_BUSY, -E_BUSY },
    { GRD_NO_DATA, -E_NOT_FOUND },
    { GRD_FAILED_MEMORY_ALLOCATE, -E_OUT_OF_MEMORY },
    { GRD_FAILED_MEMORY_RELEASE, -E_OUT_OF_MEMORY },
    { GRD_DATA_CONFLICT, -E_INVALID_DATA },
    { GRD_NOT_AVAILABLE, -E_NOT_FOUND },
    { GRD_INVALID_FORMAT, -E_INVALID_FORMAT },
    { GRD_TIME_OUT, -E_TIMEOUT },
    { GRD_DB_INSTANCE_ABNORMAL, -E_INTERNAL_ERROR },
    { GRD_DISK_SPACE_FULL, -E_INTERNAL_ERROR },
    { GRD_CRC_CHECK_DISABLED, -E_INVALID_ARGS },
    { GRD_PERMISSION_DENIED, -E_DENIED_SQL },
    { GRD_REBUILD_DATABASE, -E_REBUILD_DATABASE}, // rebuild database means ok
};
int TransferGrdErrno(int err)
{
    if (err > 0) {
        return err;
    }
    for (const auto &item : GRD_ERRNO_MAP) {
        if (item.grdCode == err) {
            return item.kvDbCode;
        }
    }
    return -E_INTERNAL_ERROR;
}

std::vector<uint8_t> KvItemToBlob(GRD_KVItemT &item)
{
    return std::vector<uint8_t>((uint8_t *)item.data, (uint8_t *)item.data + item.dataLen);
}

int GetCollNameFromType(SingleVerDataType type, std::string &collName)
{
    switch (type) {
        case SingleVerDataType::SYNC_TYPE:
            collName = SYNC_COLLECTION_NAME;
            break;
        default:
            LOGE("data type not support");
            return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RdKVPut(GRD_DB *db, const char *collectionName, const Key &key, const Value &value)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdKvPut] invalid db");
        return -E_INVALID_DB;
    }
    GRD_KVItemT innerKey{(void *)&key[0], (uint32_t)key.size()};
    GRD_KVItemT innerVal{(void *)&value[0], (uint32_t)value.size()};
    int ret = TransferGrdErrno(GRD_KVPut(db, collectionName, &innerKey, &innerVal));
    if (ret != E_OK) {
        LOGE("[rdUtils][RdKvPut] ERROR:%d", ret);
    }
    return ret;
}

int RdKVGet(GRD_DB *db, const char *collectionName, const Key &key, Value &value)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdKvGet] invalid db");
        return -E_INVALID_DB;
    }
    GRD_KVItemT innerKey{(void *)&key[0], (uint32_t)key.size()};
    GRD_KVItemT innerVal = { 0 };
    int ret = TransferGrdErrno(GRD_KVGet(db, collectionName, &innerKey, &innerVal));
    if (ret != E_OK) {
        // log print on caller
        return ret;
    }
    value = KvItemToBlob(innerVal);
    (void)GRD_KVFreeItem(&innerVal);
    return E_OK;
}

int RdKVDel(GRD_DB *db, const char *collectionName, const Key &key)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdKvDel] invalid db");
        return -E_INVALID_DB;
    }
    GRD_KVItemT innerKey{(void *)&key[0], (uint32_t)key.size()};
    int ret = TransferGrdErrno(GRD_KVDel(db, collectionName, &innerKey));
    if (ret < 0) {
        LOGE("[rdUtils][RdKvDel] failed:%d", ret);
    }
    return ret;
}

int RdKVScan(GRD_DB *db, const char *collectionName, const Key &key, GRD_KvScanModeE mode,
    GRD_ResultSet **resultSet)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdKVScan] invalid db");
        return -E_INVALID_DB;
    }
    if (key.empty()) {
        return TransferGrdErrno(GRD_KVScan(db, collectionName, NULL, mode, resultSet));
    }
    GRD_KVItemT innerKey{(void *)&key[0], (uint32_t)key.size()};
    return TransferGrdErrno(GRD_KVScan(db, collectionName, &innerKey, mode, resultSet));
}

int RdKVRangeScan(GRD_DB *db, const char *collectionName, const Key &beginKey, const Key &endKey,
    GRD_ResultSet **resultSet)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdKVScan] invalid db");
        return -E_INVALID_DB;
    }
    GRD_KVItemT beginInnerKey{(void *)&beginKey[0], (uint32_t)beginKey.size()};
    GRD_KVItemT endInnerKey{(void *)&endKey[0], (uint32_t)endKey.size()};
    GRD_FilterOption filterOpt{KV_SCAN_RANGE, beginInnerKey, endInnerKey};
    return TransferGrdErrno(GRD_KVFilter(db, collectionName, &filterOpt, resultSet));
}

int RdKvFetch(GRD_ResultSet *resultSet, Key &key, Value &value)
{
    uint32_t keyLen;
    uint32_t valueLen;
    int errCode = TransferGrdErrno(GRD_KVGetSize(resultSet, &keyLen, &valueLen));
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[rdUtils][RdKvFetch] Can not Get value lens size from resultSet");
        return errCode;
    }
    key.resize(keyLen);
    value.resize(valueLen);
    return TransferGrdErrno(GRD_GetItem(resultSet, key.data(), value.data()));
}

int RdDBClose(GRD_DB *db, uint32_t flags)
{
    return TransferGrdErrno(GRD_DBClose(db, flags));
}

int RdFreeResultSet(GRD_ResultSet *resultSet)
{
    return TransferGrdErrno(GRD_FreeResultSet(resultSet));
}

int RdKVBatchPrepare(uint16_t itemNum, GRD_KVBatchT **batch)
{
    return TransferGrdErrno(GRD_KVBatchPrepare(itemNum, batch));
}

int RdKVBatchPushback(GRD_KVBatchT *batch, const Key &key, const Value &value)
{
    GRD_KVItemT innerKey{(void *)&key[0], (uint32_t)key.size()};
    GRD_KVItemT innerVal{(void *)&value[0], (uint32_t)value.size()};
    int ret = TransferGrdErrno(
        GRD_KVBatchPushback(innerKey.data, innerKey.dataLen, innerVal.data, innerVal.dataLen, batch));
    if (ret != E_OK) {
        LOGE("[rdUtils][BatchSaveEntries] Can not push back entries to KVBatch structure");
    }
    return ret;
}

int RdKVBatchPut(GRD_DB *db, const char *kvTableName, GRD_KVBatchT *batch)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdKVBatchPut] invalid db");
        return -E_INVALID_DB;
    }
    return TransferGrdErrno(GRD_KVBatchPut(db, kvTableName, batch));
}

int RdFlush(GRD_DB *db, uint32_t flags)
{
    if (db == nullptr) {
        LOGE("[rdUtils][ForceCheckPoint] invalid db");
        return -E_INVALID_DB;
    }
    // flags means options, input 0 in current version.
    return TransferGrdErrno(GRD_Flush(db, flags));
}

int RdKVBatchDel(GRD_DB *db, const char *kvTableName, GRD_KVBatchT *batch)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdKVBatchDel] invalid db");
        return -E_INVALID_DB;
    }
    return TransferGrdErrno(GRD_KVBatchDel(db, kvTableName, batch));
}

int RdKVBatchDestroy(GRD_KVBatchT *batch)
{
    return TransferGrdErrno(GRD_KVBatchDestroy(batch));
}

int RdDbOpen(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB *&db)
{
    return TransferGrdErrno(GRD_DBOpen(dbPath, configStr, flags, &db));
}

int RdIndexPreload(GRD_DB *&db, const char *collectionName)
{
    if (db == nullptr) {
        LOGE("[rdUtils][RdIndexPreload] db is null");
        return -E_INVALID_DB;
    }
    return TransferGrdErrno(GRD_IndexPreload(db, collectionName));
}

bool CheckRdOption(const KvStoreNbDelegate::Option &option,
    const std::function<void(DBStatus, KvStoreNbDelegate *)> &callback)
{
    if (option.storageEngineType != GAUSSDB_RD && option.storageEngineType != SQLITE) {
        callback(INVALID_ARGS, nullptr);
        return false;
    }
    if (option.rdconfig.readOnly && option.isNeedRmCorruptedDb) {
        callback(INVALID_ARGS, nullptr);
        return false;
    }
    if (!CheckOption(option)) {
        callback(NOT_SUPPORT, nullptr);
        return false;
    }
    return true;
}
} // namespace DistributedDB