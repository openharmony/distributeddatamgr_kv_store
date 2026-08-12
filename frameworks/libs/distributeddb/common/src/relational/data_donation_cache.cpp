/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifdef RELATIONAL_STORE
#include "data_donation_cache.h"
#include "data_donation_sql_generator.h"
#include "db_common.h"
#include "sqlite_relational_utils.h"
#include "sqlite_utils.h"

namespace DistributedDB {

int DataDonationCache::SetSchema(const std::string &schema)
{
    Init();
    return ddSchema.Init(schema);
}

int DataDonationCache::QueryStorage(SQLiteSingleVerRelationalStorageExecutor *handle,
    const DBSubscribeCursor &cursorIn, DBSubscribeCursor &cursorOut, std::vector<VBucket> &data)
{
    if (handle == nullptr) {
        LOGE("[QueryStorage] executor is null");
        return -E_INVALID_ARGS;
    }

    const DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath();
    std::string mainTable = DataDonationSqlGenerator::BuildFromTableName(path);
    std::vector<std::string> tableNames = DataDonationSqlGenerator::GetJoinedTableNames(path);
    std::string dbPath;

    sqlite3 *dbHandle = nullptr;
    if (handle->GetDbHandle(dbHandle) != E_OK || !SQLiteRelationalUtils::GetDbFileName(dbHandle, dbPath)) {
        LOGE("[QueryStorage] Get db path failed");
        return -E_INVALID_DB;
    }

    int errCode = E_OK;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids;
    cursorOut.queryType = cursorIn.queryType;
    bool needInit = (cursorIn.cursor == 0) && (!getAllCache_.isValid || getAllCache_.mainTable != mainTable);
    if (needInit) {
        // First query: initialize
        errCode = InitGetAllQuery(dbPath, tableNames, handle, maxRowids, cursorOut.cursor);
    } else {
        // Subsequent query: load from cache or file
        errCode = LoadCursorFromCacheOrFile(mainTable, dbPath, cursorValues, maxRowids);
        // If the cursor fails to load, try restart the query
        if (errCode != E_OK) {
            LOGW("[QueryStorage] load cursor failed and try requery: %d", errCode);
            errCode = InitGetAllQuery(dbPath, tableNames, handle, maxRowids, cursorOut.cursor);
        }
    }
    if (errCode != E_OK) {
        return errCode;
    }

    SQLiteSingleVerRelationalStorageExecutor::GetAllQueryResult result;
    errCode = handle->QuerySubscribeOutputWithCursor(path, cursorValues, maxRowids, result);
    if (errCode != E_OK && errCode != -E_SUBSCRIBE_QUERY_END) {
        return errCode;
    }

    if (!result.dataOut.empty()) {
        GetAllCursorCache newCache;
        newCache.mainTable = result.mainTable;
        newCache.dbPath = result.dbPath;
        newCache.cursorValues = result.cursorValues;
        newCache.maxRowids = result.maxRowids;
        newCache.sessionCursor = result.sessionCursor;
        newCache.isValid = true;
        UpdateGetAllCache(newCache);
    }
    data = result.dataOut;
    cursorOut.cursor = result.sessionCursor;
    return errCode;
}

int DataDonationCache::InitGetAllQuery(const std::string &dbPath,
    const std::vector<std::string> &tableNames,
    SQLiteSingleVerRelationalStorageExecutor *handle,
    std::vector<std::pair<std::string, int64_t>> &maxRowids,
    uint64_t &cursorOut)
{
    // Clear old cache
    getAllCache_.isValid = false;

    int errCode = DataDonationUtils::CheckBinlogDirExist(dbPath);
    if (errCode != E_OK) {
        return errCode;
    }

    // Get maxRowid for each table
    for (const auto &tableName : tableNames) {
        int64_t tableMaxRowid = 0;
        errCode = handle->GetMaxRowid(tableName, tableMaxRowid);
        if (errCode != E_OK) {
            return errCode;
        }
        maxRowids.emplace_back(tableName, tableMaxRowid);
    }

    // If main table is empty, finish
    if (!maxRowids.empty() && maxRowids[0].second == 0) {
        cursorOut = 0;
        return -E_SUBSCRIBE_QUERY_END;
    }

    sqlite3 *dbHandle = nullptr;
    if (handle->GetDbHandle(dbHandle) != E_OK) {
        LOGE("[InitGetAllQuery] Get db handle failed");
        return -E_INVALID_DB;
    }
    errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_reset_search_hwm_binlog(dbHandle));

    return errCode;
}

int DataDonationCache::LoadCursorFromCacheOrFile(const std::string &mainTable,
    const std::string &dbPath,
    std::vector<std::pair<std::string, int64_t>> &cursorValues,
    std::vector<std::pair<std::string, int64_t>> &maxRowids)
{
    // Load from cache first
    if (getAllCache_.isValid && getAllCache_.mainTable == mainTable) {
        cursorValues = getAllCache_.cursorValues;
        maxRowids = getAllCache_.maxRowids;
        LOGI("[LoadCursorFromCacheOrFile] Loaded from cache for %s",
            DBCommon::StringMiddleMasking(mainTable).c_str());
        return E_OK;
    }

    // Load from file
    int errCode = DataDonationUtils::LoadRowidHwm(dbPath, mainTable, cursorValues, maxRowids);
    if (errCode != E_OK) {
        LOGE("[LoadCursorFromCacheOrFile] Load from file failed: %d", errCode);
        return errCode;
    }

    LOGI("[LoadCursorFromCacheOrFile] Loaded from file for %s",
        DBCommon::StringMiddleMasking(mainTable).c_str());
    return E_OK;
}

void DataDonationCache::UpdateGetAllCache(const GetAllCursorCache &newCache)
{
    getAllCache_ = newCache;
}

int DataDonationCache::FlushGetAllCursorCache()
{
    if (!getAllCache_.isValid) {
        LOGI("[FlushGetAllCursorCache] No valid cache to flush");
        return E_OK;
    }

    int errCode = DataDonationUtils::SaveRowidHwm(getAllCache_.dbPath, getAllCache_.mainTable,
        getAllCache_.cursorValues, getAllCache_.maxRowids);
    if (errCode != E_OK) {
        LOGE("[FlushGetAllCursorCache] SaveRowidHwm failed: %d", errCode);
        return errCode;
    }

    LOGI("[FlushGetAllCursorCache] Persisted cursor for table %s",
        DBCommon::StringMiddleMasking(getAllCache_.mainTable).c_str());
    return E_OK;
}

int DataDonationCache::PushDataToCache(SQLiteSingleVerRelationalStorageExecutor *handle)
{
    // First, try to push any pending data from previous partial push
    if (!pendingData_.empty()) {
        size_t pushed = PushPartial(pendingData_, pendingData_.size());
        LOGI("[PushDataToCache] pushed data size: %zu", pushed);
        if (pushed < pendingData_.size()) {
            // Some data still couldn't fit, keep it for next time
            std::vector<DdData> remaining(pendingData_.begin() + pushed, pendingData_.end());
            pendingData_ = std::move(remaining);
        } else {
            pendingData_.clear();
        }
    }

    std::vector<DdData> queryData;
    int errCode = handle->QuerySubscribeOutput(ddSchema, queryData);
    if (errCode != E_OK && errCode != -E_SUBSCRIBE_QUERY_END) {
        LOGE("[PushDataToCache] Query err: %d", errCode);
        return errCode;
    }
    if (queryData.size() >= GET_ALL_BATCH_NUM) {
        LOGI("[PushDataToCache] queryData size is: %zu", queryData.size());
    }

    // Try PushBatch first, if fails due to capacity, use PushPartial
    int ret = PushBatch(queryData, queryData.size());
    if (ret == -E_MAX_LIMITS) {
        // Capacity exhausted, push as much as possible and save rest for later
        size_t pushed = PushPartial(queryData, queryData.size());
        LOGI("[PushDataToCache] pushed data size: %zu", pushed);
        if (pushed < queryData.size()) {
            pendingData_.insert(pendingData_.end(), queryData.begin() + pushed, queryData.end());
        }
        if (pushed == 0 && !queryData.empty()) {
            LOGE("[PushDataToCache] PushPartial failed, cache is full");
            return -E_MAX_LIMITS;
        }
    } else if (ret != E_OK) {
        LOGE("[PushDataToCache] PushBatch err: %d", ret);
        return ret;
    }
    return errCode;
}

int DataDonationCache::QueryBinlog(SQLiteSingleVerRelationalStorageExecutor *handle, const DBSubscribeCursor &cursorIn,
    DBSubscribeCursor &cursorOut, std::vector<VBucket> &data)
{
    if (handle == nullptr) {
        LOGE("[QueryBinlog] executor is null");
        return -E_INVALID_ARGS;
    }
    if (cursor == UINT64_MAX) {
        cursor = cursorIn.cursor;
    }
    if (cursorIn.cursor != cursor) {
        LOGW("[QueryBinlog] Unexpected cursorIn %lu, cursor %lu, perhaps 1.reset, 2.cursorIn invalid.",
            cursorIn.cursor, cursor);
    }
    size_t readNum = 0;
    size_t readToken = GET_NEW_BATCH_NUM;

    bool hasCache = RemainReadSize() > 0;
    int errCode = TryInitCursorByLogical(cursor, cursorIn.cursor);
    if (errCode != E_OK) {
        return errCode;
    }

    if (!hasCache) {
        errCode = PushDataToCache(handle);
        if (errCode != E_OK && errCode != -E_SUBSCRIBE_QUERY_END) {
            LOGW("[QueryBinlog] PushDataToCache error %d.", errCode);
            return errCode;
        }
    }

    // Read from DataDonationCache
    readNum = ReadBatch(cacheRead, readToken);

    for (size_t i = 0; i < readNum; i++) {
        data.emplace_back(cacheRead[i].data);
    }
    cursorOut.queryType = cursorIn.queryType;
    cursorOut.cursor = cursorIn.cursor + readNum;

    if (errCode == -E_SUBSCRIBE_QUERY_END) {
        errCode = (readNum == 0 || RemainReadSize() == 0) ? -E_SUBSCRIBE_QUERY_END : E_OK;
    }
    return errCode;
}

int DataDonationCache::Query(SQLiteSingleVerRelationalStorageExecutor *handle,
    const DBSubscribeCursor &cursorIn, DBSubscribeCursor &cursorOut, std::vector<VBucket> &data)
{
    switch (cursorIn.queryType) {
        case SubQueryType::GET_ALL:
            return QueryStorage(handle, cursorIn, cursorOut, data);
        case SubQueryType::GET_NEW:
            return QueryBinlog(handle, cursorIn, cursorOut, data);
        default:
            return -E_INVALID_ARGS;
    }
}

int DataDonationCache::UpdateCursor(const DdCursor &cursorIn, DdData &ddData)
{
    int errCode = E_OK;
    uint64_t newCursor = cursorIn.cursor;
    size_t hasRead = ReadCacheSize();
    DdData *binlogData;
    if (cursor + hasRead == newCursor) {
        binlogData = AdvanceFront(static_cast<size_t>(newCursor - cursor));
        cursor = newCursor;
    } else if (cursor + hasRead < newCursor) {
        LOGE("UniqueQueue set global cursor %llu out of limit, cursor %llu, read cache num %d.", newCursor, cursor,
            hasRead);
        binlogData = AdvanceFront(hasRead);
        cursor += hasRead;
        errCode = E_MAX_LIMITS;
    } else {  // cursor + hasRead > newCursor
        LOGW("UniqueQueue set global cursor %llu less than read cache num, cursor %llu, read cache num %d.", newCursor,
            cursor, hasRead);
        binlogData = AdvanceFront(static_cast<size_t>(newCursor - cursor));
        cursor = newCursor;
    }
    if (hasRead == 0) {
        return -E_SUBSCRIBE_QUERY_END;
    }
    if (binlogData != nullptr) {
        ddData.cursor = binlogData->cursor;
        ddData.fileIdx = binlogData->fileIdx;
    }
    return E_OK;
}

}  // namespace DistributedDB

#endif
