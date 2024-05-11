/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "sqlite_cloud_kv_executor_utils.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "res_finalizer.h"
#include "runtime_context.h"
#include "sqlite_single_ver_storage_executor_sql.h"

namespace DistributedDB {
int SqliteCloudKvExecutorUtils::GetCloudData(sqlite3 *db, bool isMemory, SQLiteSingleVerContinueToken &token,
    CloudSyncData &data)
{
    bool stepNext = false;
    auto [errCode, stmt] = token.GetCloudQueryStmt(db, data.isCloudForcePushStrategy, stepNext, data.mode);
    if (errCode != E_OK) {
        token.ReleaseCloudQueryStmt();
        return errCode;
    }
    uint32_t totalSize = 0;
    uint32_t stepNum = 0;
    do {
        if (stepNext) {
            errCode = SQLiteUtils::StepNext(stmt, isMemory);
            if (errCode != E_OK) {
                errCode = (errCode == -E_FINISHED ? E_OK : errCode);
                break;
            }
        }
        stepNext = true;
        errCode = GetCloudDataForSync(stmt, data, ++stepNum, totalSize);
    } while (errCode == E_OK);
    LOGI("[SqliteCloudKvExecutorUtils] Get cloud sync data, insData:%u, upData:%u, delLog:%u errCode:%d",
         data.insData.record.size(), data.updData.record.size(), data.delData.extend.size(), errCode);
    if (errCode != -E_UNFINISHED) {
        token.ReleaseCloudQueryStmt();
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::GetCloudDataForSync(sqlite3_stmt *statement, CloudSyncData &cloudDataResult,
    uint32_t &stepNum, uint32_t &totalSize)
{
    VBucket log;
    VBucket extraLog;
    uint32_t preSize = totalSize;
    GetCloudLog(statement, log, totalSize);
    GetCloudExtraLog(statement, extraLog);

    VBucket data;
    int64_t flag = 0;
    int errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::FLAG, extraLog, flag);
    if (errCode != E_OK) {
        return errCode;
    }

    if ((static_cast<uint64_t>(flag) & DataItem::DELETE_FLAG) == 0) {
        errCode = GetCloudKvData(statement, data, totalSize);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    if (CloudStorageUtils::IsGetCloudDataContinue(stepNum, totalSize, CloudDbConstant::MAX_UPLOAD_SIZE)) {
        errCode = CloudStorageUtils::IdentifyCloudType(cloudDataResult, data, log, extraLog);
    } else {
        errCode = -E_UNFINISHED;
    }
    if (errCode == E_OK) {
        errCode = CheckIgnoreData(data, extraLog);
    }
    if (errCode == -E_IGNORE_DATA) {
        errCode = E_OK;
        totalSize = preSize;
        stepNum--;
    }
    return errCode;
}

void SqliteCloudKvExecutorUtils::GetCloudLog(sqlite3_stmt *stmt, VBucket &logInfo,
    uint32_t &totalSize)
{
    logInfo.insert_or_assign(CloudDbConstant::MODIFY_FIELD,
        static_cast<int64_t>(sqlite3_column_int64(stmt, CLOUD_QUERY_MODIFY_TIME_INDEX)));
    logInfo.insert_or_assign(CloudDbConstant::CREATE_FIELD,
        static_cast<int64_t>(sqlite3_column_int64(stmt, CLOUD_QUERY_CREATE_TIME_INDEX)));
    totalSize += sizeof(int64_t) + sizeof(int64_t);
    if (sqlite3_column_text(stmt, CLOUD_QUERY_CLOUD_GID_INDEX) != nullptr) {
        std::string cloudGid = reinterpret_cast<const std::string::value_type *>(
            sqlite3_column_text(stmt, CLOUD_QUERY_CLOUD_GID_INDEX));
        if (!cloudGid.empty()) {
            logInfo.insert_or_assign(CloudDbConstant::GID_FIELD, cloudGid);
            totalSize += cloudGid.size();
        }
    }
    std::string version;
    SQLiteUtils::GetColumnTextValue(stmt, CLOUD_QUERY_VERSION_INDEX, version);
    logInfo.insert_or_assign(CloudDbConstant::VERSION_FIELD, version);
    totalSize += version.size();
}

void SqliteCloudKvExecutorUtils::GetCloudExtraLog(sqlite3_stmt *stmt, VBucket &flags)
{
    flags.insert_or_assign(CloudDbConstant::ROWID,
        static_cast<int64_t>(sqlite3_column_int64(stmt, CLOUD_QUERY_ROW_ID_INDEX)));
    flags.insert_or_assign(CloudDbConstant::TIMESTAMP,
        static_cast<int64_t>(sqlite3_column_int64(stmt, CLOUD_QUERY_MODIFY_TIME_INDEX)));
    flags.insert_or_assign(CloudDbConstant::FLAG,
        static_cast<int64_t>(sqlite3_column_int64(stmt, CLOUD_QUERY_FLAG_INDEX)));
    Bytes hashKey;
    (void)SQLiteUtils::GetColumnBlobValue(stmt, CLOUD_QUERY_HASH_KEY_INDEX, hashKey);
    flags.insert_or_assign(CloudDbConstant::HASH_KEY, hashKey);
}

int SqliteCloudKvExecutorUtils::GetCloudKvData(sqlite3_stmt *stmt, VBucket &data, uint32_t &totalSize)
{
    int errCode = GetCloudKvBlobData(CloudDbConstant::CLOUD_KV_FIELD_KEY, CLOUD_QUERY_KEY_INDEX, stmt, data, totalSize);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GetCloudKvBlobData(CloudDbConstant::CLOUD_KV_FIELD_VALUE, CLOUD_QUERY_VALUE_INDEX, stmt, data, totalSize);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GetCloudKvBlobData(CloudDbConstant::CLOUD_KV_FIELD_DEVICE, CLOUD_QUERY_DEV_INDEX, stmt, data, totalSize);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GetCloudKvBlobData(CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE, CLOUD_QUERY_ORI_DEV_INDEX, stmt, data,
        totalSize);
    if (errCode != E_OK) {
        return errCode;
    }
    data.insert_or_assign(CloudDbConstant::CLOUD_KV_FIELD_DEVICE_CREATE_TIME,
        static_cast<int64_t>(sqlite3_column_int64(stmt, CLOUD_QUERY_DEV_CREATE_TIME_INDEX)));
    totalSize += sizeof(int64_t);
    return E_OK;
}

int SqliteCloudKvExecutorUtils::GetCloudKvBlobData(const std::string &keyStr, int index, sqlite3_stmt *stmt,
    VBucket &data, uint32_t &totalSize)
{
    std::vector<uint8_t> blob;
    int errCode = SQLiteUtils::GetColumnBlobValue(stmt, index, blob);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get %.3s failed %d", keyStr.c_str(), errCode);
        return errCode;
    }
    std::string tmp = std::string(blob.begin(), blob.end());
    if ((keyStr == CloudDbConstant::CLOUD_KV_FIELD_DEVICE ||
        keyStr == CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE) && tmp.empty()) {
        (void)RuntimeContext::GetInstance()->GetLocalIdentity(tmp);
        tmp = DBCommon::TransferHashString(tmp);
    }
    totalSize += tmp.size();
    data.insert_or_assign(keyStr, tmp);
    return E_OK;
}

int SqliteCloudKvExecutorUtils::CheckIgnoreData(VBucket &data, VBucket &flags)
{
    auto iter = data.find(CloudDbConstant::CLOUD_KV_FIELD_VALUE);
    if (iter == data.end()) {
        return E_OK;
    }
    auto &valueStr = std::get<std::string>(iter->second);
    if (valueStr.size() <= CloudDbConstant::MAX_UPLOAD_SIZE) {
        return E_OK;
    }
    Bytes *hashKey = std::get_if<Bytes>(&flags[CloudDbConstant::HASH_KEY]);
    if (hashKey != nullptr) {
        LOGW("[SqliteCloudKvExecutorUtils] Ignore value size %zu hash is %.3s", valueStr.size(),
            std::string(hashKey->begin(), hashKey->end()).c_str());
    }
    return -E_IGNORE_DATA;
}

std::pair<int, DataInfoWithLog> SqliteCloudKvExecutorUtils::GetLogInfo(sqlite3 *db, bool isMemory,
    const VBucket &cloudData)
{
    std::pair<int, DataInfoWithLog> res;
    int &errCode = res.first;
    std::string keyStr;
    errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::CLOUD_KV_FIELD_KEY, cloudData, keyStr);
    if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get key failed %d", errCode);
        return res;
    }
    Bytes key;
    DBCommon::StringToVector(keyStr, key);
    sqlite3_stmt *stmt = nullptr;
    std::tie(errCode, stmt) = GetLogInfoStmt(db, cloudData, !key.empty());
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get stmt failed %d", errCode);
        return res;
    }
    std::string gid;
    errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, cloudData, gid);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[SqliteCloudKvExecutorUtils] Get gid failed %d", errCode);
        return res;
    }
    return GetLogInfoInner(stmt, isMemory, gid, key);
}

std::pair<int, sqlite3_stmt*> SqliteCloudKvExecutorUtils::GetLogInfoStmt(sqlite3 *db, const VBucket &cloudData,
    bool existKey)
{
    std::pair<int, sqlite3_stmt*> res;
    auto &[errCode, stmt] = res;
    std::string sql = QUERY_CLOUD_SYNC_DATA_LOG;
    std::string hashKey;
    if (existKey) {
        sql += "OR key = ?";
    }
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    return res;
}

std::pair<int, DataInfoWithLog> SqliteCloudKvExecutorUtils::GetLogInfoInner(sqlite3_stmt *stmt, bool isMemory,
    const std::string &gid, const Bytes &key)
{
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset stmt failed %d when get log", ret);
        }
    });
    std::pair<int, DataInfoWithLog> res;
    auto &[errCode, logInfo] = res;
    int index = 1;
    errCode = SQLiteUtils::BindTextToStatement(stmt, index++, gid);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind gid failed %d", errCode);
        return res;
    }
    if (!key.empty()) {
        errCode = SQLiteUtils::BindBlobToStatement(stmt, index, key);
        if (errCode != E_OK) {
            LOGE("[SqliteCloudKvExecutorUtils] Bind key failed %d", errCode);
            return res;
        }
    }
    errCode = SQLiteUtils::StepNext(stmt, isMemory);
    if (errCode == -E_FINISHED) {
        errCode = -E_NOT_FOUND;
        // not found is ok, just return error
        return res;
    }
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get log failed %d", errCode);
        return res;
    }
    logInfo = FillLogInfoWithStmt(stmt);
    return res;
}

DataInfoWithLog SqliteCloudKvExecutorUtils::FillLogInfoWithStmt(sqlite3_stmt *stmt)
{
    DataInfoWithLog dataInfoWithLog;
    int index = 0;
    dataInfoWithLog.logInfo.dataKey = sqlite3_column_int64(stmt, index++);
    dataInfoWithLog.logInfo.flag = static_cast<uint64_t>(sqlite3_column_int64(stmt, index++));
    std::vector<uint8_t> device;
    (void)SQLiteUtils::GetColumnBlobValue(stmt, index++, device);
    DBCommon::VectorToString(device, dataInfoWithLog.logInfo.device);
    std::vector<uint8_t> oriDev;
    (void)SQLiteUtils::GetColumnBlobValue(stmt, index++, oriDev);
    DBCommon::VectorToString(oriDev, dataInfoWithLog.logInfo.originDev);
    dataInfoWithLog.logInfo.timestamp = static_cast<Timestamp>(sqlite3_column_int64(stmt, index++));
    dataInfoWithLog.logInfo.wTimestamp = static_cast<Timestamp>(sqlite3_column_int64(stmt, index++));
    std::string gid;
    (void)SQLiteUtils::GetColumnTextValue(stmt, index++, gid);
    dataInfoWithLog.logInfo.cloudGid = gid;
    (void)SQLiteUtils::GetColumnBlobValue(stmt, index++, dataInfoWithLog.logInfo.hashKey);
    Bytes key;
    (void)SQLiteUtils::GetColumnBlobValue(stmt, index++, key);
    std::string keyStr(key.begin(), key.end());
    dataInfoWithLog.primaryKeys.insert_or_assign(CloudDbConstant::CLOUD_KV_FIELD_KEY, keyStr);
    (void)SQLiteUtils::GetColumnTextValue(stmt, index++, dataInfoWithLog.logInfo.version);
    return dataInfoWithLog;
}

int SqliteCloudKvExecutorUtils::PutCloudData(sqlite3 *db, bool isMemory, DownloadData &downloadData)
{
    if (downloadData.data.size() != downloadData.opType.size()) {
        LOGE("[SqliteCloudKvExecutorUtils] data size %zu != flag size %zu.", downloadData.data.size(),
            downloadData.opType.size());
        return -E_CLOUD_ERROR;
    }
    std::map<int, int> statisticMap = {};
    int errCode = ExecutePutCloudData(db, isMemory, downloadData, statisticMap);
    LOGI("[SqliteCloudKvExecutorUtils] save cloud data: %d, insert cnt = %d, update cnt = %d, delete cnt = %d,"
        " only update gid cnt = %d, set LCC flag zero cnt = %d, set LCC flag one cnt = %d,"
        " update timestamp cnt = %d, clear gid count = %d, not handle cnt = %d",
        errCode, statisticMap[static_cast<int>(OpType::INSERT)], statisticMap[static_cast<int>(OpType::UPDATE)],
        statisticMap[static_cast<int>(OpType::DELETE)], statisticMap[static_cast<int>(OpType::ONLY_UPDATE_GID)],
        statisticMap[static_cast<int>(OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO)],
        statisticMap[static_cast<int>(OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE)],
        statisticMap[static_cast<int>(OpType::UPDATE_TIMESTAMP)], statisticMap[static_cast<int>(OpType::CLEAR_GID)],
        statisticMap[static_cast<int>(OpType::NOT_HANDLE)]);
    return errCode;
}

int SqliteCloudKvExecutorUtils::ExecutePutCloudData(sqlite3 *db, bool isMemory,
    DownloadData &downloadData, std::map<int, int> &statisticMap)
{
    int index = 0;
    int errCode = E_OK;
    for (OpType op : downloadData.opType) {
        switch (op) {
            case OpType::INSERT: // fallthrough
            case OpType::UPDATE: // fallthrough
            case OpType::DELETE: // fallthrough
                errCode = OperateCloudData(db, isMemory, index, op, downloadData);
                break;
            case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO: // fallthrough
            case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE:  // fallthrough
            case OpType::UPDATE_TIMESTAMP:               // fallthrough
                errCode = OnlyUpdateSyncData(db, isMemory, index, op, downloadData);
                if (errCode != E_OK) {
                    break;
                }
            case OpType::ONLY_UPDATE_GID:                // fallthrough
            case OpType::NOT_HANDLE:                     // fallthrough
                errCode = OnlyUpdateLogTable(db, isMemory, index, downloadData);
                break;
            case OpType::CLEAR_GID:                      // fallthrough
                break;
            default:
                errCode = -E_CLOUD_ERROR;
                break;
        }
        if (errCode != E_OK) {
            LOGE("put cloud sync data fail:%d op:%d", errCode, static_cast<int>(op));
            return errCode;
        }
        statisticMap[static_cast<int>(op)]++;
        index++;
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::OperateCloudData(sqlite3 *db, bool isMemory, int index, OpType opType,
    DownloadData &downloadData)
{
    sqlite3_stmt *logStmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, GetOperateLogSql(opType), logStmt);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get insert log statement failed %d", errCode);
        return errCode;
    }
    sqlite3_stmt *dataStmt = nullptr;
    errCode = SQLiteUtils::GetStatement(db, GetOperateDataSql(opType), dataStmt);
    if (errCode != E_OK) {
        int ret = E_OK;
        SQLiteUtils::ResetStatement(logStmt, true, ret);
        LOGE("[SqliteCloudKvExecutorUtils] Get insert data statement failed %d reset %d", errCode, ret);
        return errCode;
    }
    ResFinalizer finalizerData([logStmt, dataStmt, opType]() {
        sqlite3_stmt *statement = logStmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset log stmt failed %d opType %d", ret, static_cast<int>(opType));
        }
        statement = dataStmt;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset data stmt failed %d opType %d", ret, static_cast<int>(opType));
        }
    });
    errCode = BindStmt(logStmt, dataStmt, index, opType, downloadData);
    if (errCode != E_OK) {
        return errCode;
    }
    return StepStmt(logStmt, dataStmt, isMemory);
}

std::string SqliteCloudKvExecutorUtils::GetOperateDataSql(OpType opType)
{
    switch (opType) {
        case OpType::INSERT:
            return INSERT_SYNC_SQL;
        case OpType::UPDATE: // fallthrough
        case OpType::DELETE:
            return UPDATE_SYNC_SQL;
        case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE:
            return SET_SYNC_DATA_NO_FORCE_PUSH;
        case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO:
            return SET_SYNC_DATA_FORCE_PUSH;
        case OpType::UPDATE_TIMESTAMP:
            return UPDATE_TIMESTAMP;
        default:
            return "";
    }
}

std::string SqliteCloudKvExecutorUtils::GetOperateLogSql(OpType opType)
{
    switch (opType) {
        case OpType::INSERT:
            return INSERT_CLOUD_SYNC_DATA_LOG;
        case OpType::UPDATE: // fallthrough
        case OpType::DELETE:
            return UPDATE_CLOUD_SYNC_DATA_LOG;
        default:
            return "";
    }
}

int SqliteCloudKvExecutorUtils::BindStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, int index, OpType opType,
    DownloadData &downloadData)
{
    auto [errCode, dataItem] = GetDataItem(index, downloadData);
    if (errCode != E_OK) {
        return errCode;
    }
    switch (opType) {
        case OpType::INSERT:
            return BindInsertStmt(logStmt, dataStmt, downloadData.user, dataItem);
        case OpType::UPDATE:
            return BindUpdateStmt(logStmt, dataStmt, downloadData.user, dataItem);
        case OpType::DELETE:
            dataItem.hashKey = downloadData.existDataHashKey[index];
            return BindDeleteStmt(logStmt, dataStmt, downloadData.user, dataItem);
        default:
            return E_OK;
    }
}

int SqliteCloudKvExecutorUtils::BindInsertStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt,
    const std::string &user, const DataItem &dataItem)
{
    int errCode = BindInsertLogStmt(logStmt, user, dataItem);
    if (errCode != E_OK) {
        return errCode;
    }
    return BindDataStmt(dataStmt, dataItem, true);
}

int SqliteCloudKvExecutorUtils::BindInsertLogStmt(sqlite3_stmt *logStmt, const std::string &user,
    const DataItem &dataItem)
{
    int index = 1;
    int errCode = SQLiteUtils::BindTextToStatement(logStmt, index++, user);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind user failed %d when insert", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindBlobToStatement(logStmt, index++, dataItem.hashKey);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind hashKey failed %d when insert", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(logStmt, index++, dataItem.gid);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind gid failed %d when insert", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(logStmt, index++, dataItem.version);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind version failed %d when insert", errCode);
        return errCode;
    }
    return E_OK;
}

int SqliteCloudKvExecutorUtils::BindUpdateStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, const std::string &user,
    const DataItem &dataItem)
{
    int errCode = BindUpdateLogStmt(logStmt, user, dataItem);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = BindDataStmt(dataStmt, dataItem, false);
    if (errCode != E_OK) {
        return errCode;
    }
    return E_OK;
}

int SqliteCloudKvExecutorUtils::BindUpdateLogStmt(sqlite3_stmt *logStmt, const std::string &user,
    const DataItem &dataItem)
{
    int index = 1;
    int errCode = SQLiteUtils::BindTextToStatement(logStmt, index++, dataItem.gid);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind gid failed %d when update", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(logStmt, index++, dataItem.version);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind version failed %d when update", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(logStmt, index++, user);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind user failed %d when update", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindBlobToStatement(logStmt, index++, dataItem.hashKey);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind hashKey failed %d when update", errCode);
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::BindDeleteStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, const std::string &user,
    DataItem &dataItem)
{
    dataItem.key = {};
    dataItem.value = {};
    dataItem.flag |= static_cast<uint64_t>(LogInfoFlag::FLAG_DELETE);
    return BindUpdateStmt(logStmt, dataStmt, user, dataItem);
}

int SqliteCloudKvExecutorUtils::BindDataStmt(sqlite3_stmt *dataStmt, const DataItem &dataItem, bool isInsert)
{
    int index = 1;
    int errCode = BindSyncDataStmt(dataStmt, dataItem, isInsert, index);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = BindCloudDataStmt(dataStmt, dataItem, index);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isInsert) {
        errCode = SQLiteUtils::BindBlobToStatement(dataStmt, index++, dataItem.hashKey);
        if (errCode != E_OK) {
            LOGE("[SqliteCloudKvExecutorUtils] Bind hashKey failed %d", errCode);
        }
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::BindSyncDataStmt(sqlite3_stmt *dataStmt, const DataItem &dataItem, bool isInsert,
    int &index)
{
    int errCode = SQLiteUtils::BindBlobToStatement(dataStmt, index++, dataItem.key);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind key failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindBlobToStatement(dataStmt, index++, dataItem.value);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind value failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(dataStmt, index++, static_cast<int64_t>(dataItem.timestamp));
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind timestamp failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(dataStmt, index++, static_cast<int64_t>(dataItem.flag));
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind flag failed %d", errCode);
        return errCode;
    }
    Bytes bytes;
    DBCommon::StringToVector(dataItem.dev, bytes);
    errCode = SQLiteUtils::BindBlobToStatement(dataStmt, index++, bytes);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind dev failed %d", errCode);
        return errCode;
    }
    DBCommon::StringToVector(dataItem.origDev, bytes);
    errCode = SQLiteUtils::BindBlobToStatement(dataStmt, index++, bytes);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind oriDev failed %d", errCode);
        return errCode;
    }
    if (isInsert) {
        errCode = SQLiteUtils::BindBlobToStatement(dataStmt, index++, dataItem.hashKey);
        if (errCode != E_OK) {
            LOGE("[SqliteCloudKvExecutorUtils] Bind hashKey failed %d", errCode);
            return errCode;
        }
    }
    errCode = SQLiteUtils::BindInt64ToStatement(dataStmt, index++, static_cast<int64_t>(dataItem.writeTimestamp));
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind wTime failed %d", errCode);
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::BindCloudDataStmt(sqlite3_stmt *dataStmt, const DataItem &dataItem, int &index)
{
    int errCode = SQLiteUtils::BindInt64ToStatement(dataStmt, index++, dataItem.modifyTime);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind modifyTime failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(dataStmt, index++, dataItem.createTime);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind createTime failed %d", errCode);
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::StepStmt(sqlite3_stmt *logStmt, sqlite3_stmt *dataStmt, bool isMemory)
{
    int errCode = SQLiteUtils::StepNext(logStmt, isMemory);
    if (errCode != -E_FINISHED) {
        LOGE("[SqliteCloudKvExecutorUtils] Step insert log stmt failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepNext(dataStmt, isMemory);
    if (errCode != -E_FINISHED) {
        LOGE("[SqliteCloudKvExecutorUtils] Step insert data stmt failed %d", errCode);
        return errCode;
    }
    return E_OK;
}

int SqliteCloudKvExecutorUtils::FillCloudLog(sqlite3 *db, OpType opType, const CloudSyncData &data,
    const std::string &user, bool ignoreEmptyGid)
{
    if (db == nullptr) {
        LOGE("[SqliteCloudKvExecutorUtils] Fill log got nullptr db");
        return -E_INVALID_ARGS;
    }
    if (data.isCloudVersionRecord) {
        int errCode = FillCloudVersionRecord(db, opType, data);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    switch (opType) {
        case OpType::INSERT:
            return FillCloudGid(db, data.insData, user, ignoreEmptyGid);
        case OpType::UPDATE:
            return FillCloudGid(db, data.updData, user, ignoreEmptyGid);
        default:
            return E_OK;
    }
}

int SqliteCloudKvExecutorUtils::OnlyUpdateLogTable(sqlite3 *db, bool isMemory, int index, DownloadData &downloadData)
{
    if (downloadData.existDataHashKey[index].empty()) {
        return E_OK;
    }
    sqlite3_stmt *logStmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, GetOperateLogSql(OpType::INSERT), logStmt);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get update sync data stmt failed %d", errCode);
        return errCode;
    }
    ResFinalizer finalizerData([logStmt]() {
        sqlite3_stmt *statement = logStmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset log stmt failed %d when only update log", ret);
        }
    });
    auto res = CloudStorageUtils::GetDataItemFromCloudData(downloadData.data[index]);
    if (res.first != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get data item failed %d", res.first);
        return res.first;
    }
    errCode = BindInsertLogStmt(logStmt, downloadData.user, res.second);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepNext(logStmt, isMemory);
    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::FillCloudGid(sqlite3 *db, const CloudSyncBatch &data, const std::string &user,
    bool ignoreEmptyGid)
{
    sqlite3_stmt *logStmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, GetOperateLogSql(OpType::INSERT), logStmt);
    ResFinalizer finalizerData([logStmt]() {
        sqlite3_stmt *statement = logStmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset log stmt failed %d when fill log", ret);
        }
    });
    for (size_t i = 0; i < data.hashKey.size(); ++i) {
        if (DBCommon::IsRecordError(data.extend[i])) {
            continue;
        }
        DataItem dataItem;
        errCode = CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, data.extend[i], dataItem.gid);
        if (dataItem.gid.empty() && ignoreEmptyGid) {
            continue;
        }
        if (errCode != E_OK) {
            return errCode;
        }
        CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::VERSION_FIELD, data.extend[i], dataItem.version);
        dataItem.hashKey = data.hashKey[i];
        errCode = BindInsertLogStmt(logStmt, user, dataItem);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = SQLiteUtils::StepNext(logStmt, false);
        if (errCode == -E_FINISHED) {
            errCode = E_OK;
        }
        if (errCode != E_OK) {
            LOGE("[SqliteCloudKvExecutorUtils] fill back failed %d index %zu", errCode, i);
            return errCode;
        }
        SQLiteUtils::ResetStatement(logStmt, false, errCode);
    }
    return E_OK;
}

int SqliteCloudKvExecutorUtils::OnlyUpdateSyncData(sqlite3 *db, bool isMemory, int index, OpType opType,
    DownloadData &downloadData)
{
    if (opType != OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO && opType != OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE &&
        opType != OpType::UPDATE_TIMESTAMP) {
        LOGW("[SqliteCloudKvExecutorUtils] Ignore unknown opType %d", static_cast<int>(opType));
        return E_OK;
    }
    sqlite3_stmt *dataStmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, GetOperateDataSql(opType), dataStmt);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get update sync data stmt failed %d", errCode);
        return errCode;
    }
    ResFinalizer finalizerData([dataStmt]() {
        sqlite3_stmt *statement = dataStmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset log stmt failed %d when update log", ret);
        }
    });
    errCode = BindUpdateSyncDataStmt(dataStmt, index, opType, downloadData);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind update sync data stmt failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepNext(dataStmt, isMemory);
    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::BindUpdateSyncDataStmt(sqlite3_stmt *dataStmt, int index, OpType opType,
    DownloadData &downloadData)
{
    switch (opType) {
        case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO:
        case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE:
            return SQLiteUtils::BindBlobToStatement(dataStmt, 1, downloadData.existDataHashKey[index]);
        case OpType::UPDATE_TIMESTAMP:
            return BindUpdateTimestampStmt(dataStmt, index, downloadData);
        default:
            return E_OK;
    }
}

int SqliteCloudKvExecutorUtils::BindUpdateTimestampStmt(sqlite3_stmt *dataStmt, int index, DownloadData &downloadData)
{
    auto res = CloudStorageUtils::GetDataItemFromCloudData(downloadData.data[index]);
    auto &[errCode, dataItem] = res;
    if (errCode != E_OK) {
        return errCode;
    }
    int currentBindIndex = 1; // bind sql index start at 1
    errCode = SQLiteUtils::BindInt64ToStatement(dataStmt, currentBindIndex++, dataItem.timestamp);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind timestamp failed %d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(dataStmt, currentBindIndex++, dataItem.modifyTime);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind modifyTime failed %d", errCode);
    }
    errCode = SQLiteUtils::BindBlobToStatement(dataStmt, currentBindIndex++, dataItem.hashKey);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind hashKey failed %d", errCode);
        return errCode;
    }
    return E_OK;
}

std::pair<int, DataItem> SqliteCloudKvExecutorUtils::GetDataItem(int index, DownloadData &downloadData)
{
    auto res = CloudStorageUtils::GetDataItemFromCloudData(downloadData.data[index]);
    auto &[errCode, dataItem] = res;
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get data item failed %d", errCode);
        return res;
    }
    std::string dev;
    (void)RuntimeContext::GetInstance()->GetLocalIdentity(dev);
    dev = DBCommon::TransferHashString(dev);
    if (dataItem.dev == dev) {
        dataItem.dev = "";
    }
    if (dataItem.origDev == dev) {
        dataItem.origDev = "";
    }
    dataItem.timestamp = dataItem.modifyTime + downloadData.timeOffset;
    return res;
}

std::pair<int, int64_t> SqliteCloudKvExecutorUtils::CountCloudDataInner(sqlite3 *db, bool isMemory,
    const Timestamp &timestamp, const std::string &user, std::string &sql)
{
    std::pair<int, int64_t> res;
    auto &[errCode, count] = res;
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Count data stmt failed %d", errCode);
        return res;
    }
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset log stmt failed %d when count data", ret);
        }
    });
    errCode = SQLiteUtils::BindTextToStatement(stmt, BIND_CLOUD_USER, user);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind user failed %d", errCode);
        return res;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, BIND_CLOUD_TIMESTAMP, static_cast<int64_t>(timestamp));
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind begin time failed %d", errCode);
        return res;
    }
    errCode = SQLiteUtils::StepNext(stmt, isMemory);
    if (errCode == -E_FINISHED) {
        count = 0;
        return res;
    }
    count = sqlite3_column_int64(stmt, CLOUD_QUERY_COUNT_INDEX);
    LOGD("[SqliteCloudKvExecutorUtils] Get total upload count %" PRId64, count);
    return res;
}

std::pair<int, int64_t> SqliteCloudKvExecutorUtils::CountCloudData(sqlite3 *db, bool isMemory,
    const Timestamp &timestamp, const std::string &user, bool forcePush)
{
    std::string sql = SqliteQueryHelper::GetKvCloudQuerySql(true, forcePush);
    return CountCloudDataInner(db, isMemory, timestamp, user, sql);
}

std::pair<int, int64_t> SqliteCloudKvExecutorUtils::CountAllCloudData(sqlite3 *db, bool isMemory,
    const std::vector<Timestamp> &timestampVec, const std::string &user, bool forcePush)
{
    if (timestampVec.size() != 3) { // 3 is the number of three mode.
        return std::pair(-E_INVALID_ARGS, 0);
    }
    std::vector<CloudWaterType> typeVec = {CloudWaterType::DELETE, CloudWaterType::UPDATE, CloudWaterType::INSERT};
    std::pair<int, int64_t> result = std::pair(E_OK, 0);
    for (size_t i = 0; i < typeVec.size(); i++) {
        std::string sql = SqliteQueryHelper::GetKvCloudQuerySql(true, forcePush);
        SqliteQueryHelper::AppendCloudQueryToGetDiffData(sql, typeVec[i], true);
        std::pair<int, int64_t> res = CountCloudDataInner(db, isMemory, timestampVec[i], user, sql);
        if (res.first != E_OK) {
            return res;
        }
        result.second += res.second;
    }
    return result;
}

int SqliteCloudKvExecutorUtils::FillCloudVersionRecord(sqlite3 *db, OpType opType, const CloudSyncData &data)
{
    if (opType != OpType::INSERT && opType != OpType::UPDATE) {
        return E_OK;
    }
    bool isInsert = (opType == OpType::INSERT);
    CloudSyncBatch syncBatch = isInsert ? data.insData : data.updData;
    if (syncBatch.record.empty()) {
        LOGW("[SqliteCloudKvExecutorUtils] Fill empty cloud version record");
        return E_OK;
    }
    syncBatch.record[0].insert(syncBatch.extend[0].begin(), syncBatch.extend[0].end());
    auto res = CloudStorageUtils::GetSystemRecordFromCloudData(syncBatch.record[0]); // only record first one
    auto &[errCode, dataItem] = res;
    sqlite3_stmt *dataStmt = nullptr;
    errCode = SQLiteUtils::GetStatement(db, GetOperateDataSql(opType), dataStmt);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get insert version record statement failed %d", errCode);
        return errCode;
    }
    ResFinalizer finalizerData([dataStmt]() {
        int ret = E_OK;
        sqlite3_stmt *statement = dataStmt;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset version record stmt failed %d", ret);
        }
    });
    errCode = BindDataStmt(dataStmt, dataItem, isInsert);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepNext(dataStmt, false);
    if (errCode != -E_FINISHED) {
        LOGE("[SqliteCloudKvExecutorUtils] Step insert version record stmt failed %d", errCode);
        return errCode;
    }
    return E_OK;
}

std::pair<int, CloudSyncData> SqliteCloudKvExecutorUtils::GetLocalCloudVersion(sqlite3 *db, bool isMemory,
    const std::string &user)
{
    auto res = GetLocalCloudVersionInner(db, isMemory, user);
    if (res.first != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get local cloud version failed %d", res.first);
    }
    return res;
}

std::pair<int, CloudSyncData> SqliteCloudKvExecutorUtils::GetLocalCloudVersionInner(sqlite3 *db, bool isMemory,
    const std::string &user)
{
    std::pair<int, CloudSyncData> res;
    auto &[errCode, syncData] = res;
    auto sql = SqliteQueryHelper::GetKvCloudRecordSql();
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        return res;
    }
    ResFinalizer finalizerData([stmt]() {
        int ret = E_OK;
        sqlite3_stmt *statement = stmt;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset local version record stmt failed %d", ret);
        }
    });
    std::string hashDev;
    (void)RuntimeContext::GetInstance()->GetLocalIdentity(hashDev);
    hashDev = DBCommon::TransferStringToHex(DBCommon::TransferHashString(hashDev));
    std::string key = CloudDbConstant::CLOUD_VERSION_RECORD_PREFIX_KEY + hashDev;
    Key keyVec;
    DBCommon::StringToVector(key, keyVec);
    errCode = SQLiteUtils::BindBlobToStatement(stmt, BIND_CLOUD_VERSION_RECORD_KEY_INDEX, keyVec);
    if (errCode != E_OK) {
        return res;
    }
    errCode = SQLiteUtils::BindTextToStatement(stmt, BIND_CLOUD_VERSION_RECORD_USER_INDEX, user);
    if (errCode != E_OK) {
        return res;
    }
    errCode = GetCloudVersionRecord(isMemory, stmt, syncData);
    if (errCode == -E_NOT_FOUND) {
        InitDefaultCloudVersionRecord(key, hashDev, syncData);
        errCode = E_OK;
    }
    return res;
}

int SqliteCloudKvExecutorUtils::GetCloudVersionRecord(bool isMemory, sqlite3_stmt *stmt, CloudSyncData &syncData)
{
    int errCode = SQLiteUtils::StepNext(stmt, isMemory);
    if (errCode == -E_FINISHED) {
        LOGE("[SqliteCloudKvExecutorUtils] Not found local version record");
        return -E_NOT_FOUND;
    }
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get local version failed %d", errCode);
        return errCode;
    }
    uint32_t stepNum = 0;
    uint32_t totalSize = 0;
    errCode = GetCloudDataForSync(stmt, syncData, stepNum, totalSize);
    return errCode;
}

void SqliteCloudKvExecutorUtils::InitDefaultCloudVersionRecord(const std::string &key, const std::string &dev,
    CloudSyncData &syncData)
{
    VBucket defaultRecord;
    defaultRecord[CloudDbConstant::CLOUD_KV_FIELD_KEY] = key;
    defaultRecord[CloudDbConstant::CLOUD_KV_FIELD_VALUE] = {};
    defaultRecord[CloudDbConstant::CLOUD_KV_FIELD_DEVICE] = dev;
    defaultRecord[CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE] = dev;
    syncData.insData.record.push_back(std::move(defaultRecord));
    VBucket defaultExtend;
    defaultExtend[CloudDbConstant::HASH_KEY_FIELD] = DBCommon::TransferStringToHex(key);
    syncData.insData.extend.push_back(std::move(defaultExtend));
    syncData.insData.assets.emplace_back();
    Bytes bytesHashKey;
    DBCommon::StringToVector(key, bytesHashKey);
    syncData.insData.hashKey.push_back(bytesHashKey);
}

int SqliteCloudKvExecutorUtils::BindVersionStmt(const std::string &device, const std::string &user,
    sqlite3_stmt *dataStmt)
{
    std::string hashDevice;
    (void)RuntimeContext::GetInstance()->GetLocalIdentity(hashDevice);
    Bytes bytes;
    if (device == hashDevice) {
        DBCommon::StringToVector("", bytes);
    } else {
        hashDevice = DBCommon::TransferHashString(device);
        DBCommon::StringToVector(hashDevice, bytes);
    }
    int index = 1;
    int errCode = SQLiteUtils::BindBlobToStatement(dataStmt, index, bytes);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Bind device failed %d", errCode);
    }
    return errCode;
}

int SqliteCloudKvExecutorUtils::GetCloudVersionFromCloud(sqlite3 *db, bool isMemory, const std::string &user,
    const std::string &device, std::vector<VBucket> &dataVector)
{
    sqlite3_stmt *dataStmt = nullptr;
    bool isDeviceEmpty = device.empty();
    std::string sql = SqliteQueryHelper::GetCloudVersionRecordSql(isDeviceEmpty);
    int errCode = SQLiteUtils::GetStatement(db, sql, dataStmt);
    if (errCode != E_OK) {
        LOGE("[SqliteCloudKvExecutorUtils] Get cloud version record statement failed %d", errCode);
        return errCode;
    }
    ResFinalizer finalizerData([dataStmt]() {
        int ret = E_OK;
        sqlite3_stmt *statement = dataStmt;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset cloud version record stmt failed %d", ret);
        }
    });
    if (!isDeviceEmpty) {
        errCode = BindVersionStmt(device, user, dataStmt);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    uint32_t totalSize = 0;
    do {
        errCode = SQLiteUtils::StepWithRetry(dataStmt, isMemory);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            LOGE("[SqliteCloudKvExecutorUtils] Get cloud version from cloud failed. %d", errCode);
            break;
        }
        VBucket data;
        errCode = GetCloudVersionRecordData(dataStmt, data, totalSize);
        dataVector.push_back(data);
    } while (errCode == E_OK);
    return errCode;
}

int SqliteCloudKvExecutorUtils::GetCloudVersionRecordData(sqlite3_stmt *stmt, VBucket &data, uint32_t &totalSize)
{
    int errCode = GetCloudKvBlobData(CloudDbConstant::CLOUD_KV_FIELD_KEY, CLOUD_QUERY_KEY_INDEX, stmt, data, totalSize);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GetCloudKvBlobData(CloudDbConstant::CLOUD_KV_FIELD_VALUE, CLOUD_QUERY_VALUE_INDEX, stmt, data, totalSize);
    if (errCode != E_OK) {
        return errCode;
    }
    return GetCloudKvBlobData(CloudDbConstant::CLOUD_KV_FIELD_DEVICE, CLOUD_QUERY_DEV_INDEX, stmt, data, totalSize);
}
}