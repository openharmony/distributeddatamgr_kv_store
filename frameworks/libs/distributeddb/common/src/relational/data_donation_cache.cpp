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

namespace DistributedDB {

int DataDonationCache::SetSchema(const std::string &schema)
{
    Init();
    return ddSchema.Init(schema);
}

int DataDonationCache::QueryStorage(SQLiteSingleVerRelationalStorageExecutor *handle, const DBSubscribeCur &cursorIn,
    DBSubscribeCur &cursorOut, std::vector<VBucket> &data)
{
    if (handle == nullptr) {
        LOGE("[QueryBinlog] executor is null");
        return -E_INVALID_ARGS;
    }

    return handle->QuerySubscribeOutput(ddSchema.GetRelationPath(), cursorIn, cursorOut, data);
}

int DataDonationCache::QueryBinlog(SQLiteSingleVerRelationalStorageExecutor *handle, const DBSubscribeCur &cursorIn,
    DBSubscribeCur &cursorOut, std::vector<VBucket> &data)
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
    int32_t readNum = 0;
    size_t readToken = GET_NEW_BATCH_NUM;

    std::vector<DdData> queryData;
    int errCode = TryInitCursor(cursorIn.cursor % Capacity());
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = handle->QuerySubscribeOutput(ddSchema, queryData);
    if (errCode != E_OK && errCode != -E_SUBSCRIBE_QUERY_END) {
        LOGE("[QueryBinlog] Query err: %d", errCode);
        return errCode;
    }
    if (queryData.size() >= GET_ALL_BATCH_NUM) {
        LOGI("[QueryBinlog] queryData size is: %zu", queryData.size());
    }
    int ret = PushBatch(queryData, queryData.size());
    if (ret != E_OK) {
        LOGE("[QueryBinlog] PushBatch err: %d", ret);
        return ret;
    }

    // Read from DataDonationCache
    readNum = ReadBatch(cacheRead, readToken);
    if (readNum < 0) {
        return -E_UNEXPECTED_DATA;
    }

    for (size_t i = 0; i < static_cast<size_t>(readNum); i++) {
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
    const DBSubscribeCur &cursorIn, DBSubscribeCur &cursorOut, std::vector<VBucket> &data)
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
