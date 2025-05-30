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

#include "query_sync_water_mark_helper.h"

#include <algorithm>
#include <version.h>
#include "platform_specific.h"
#include "parcel.h"
#include "db_errno.h"
#include "db_common.h"
#include "log_print.h"

namespace DistributedDB {
namespace {
    const uint32_t MAX_STORE_ITEMS = 100000;
    // WaterMark Version
    constexpr uint32_t QUERY_WATERMARK_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_6_0;
    constexpr uint32_t DELETE_WATERMARK_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_3_0;
}

QuerySyncWaterMarkHelper::QuerySyncWaterMarkHelper()
    : storage_(nullptr)
{}

QuerySyncWaterMarkHelper::~QuerySyncWaterMarkHelper()
{
    storage_ = nullptr;
    deviceIdToHashQuerySyncIdMap_.clear();
    deviceIdToHashDeleteSyncIdMap_.clear();
}

int QuerySyncWaterMarkHelper::GetMetadataFromDb(const std::vector<uint8_t> &key, std::vector<uint8_t> &outValue)
{
    if (storage_ == nullptr) {
        return -E_INVALID_DB;
    }
    return storage_->GetMetaData(key, outValue);
}

int QuerySyncWaterMarkHelper::GetMetadataByPrefixKeyFromDb(const Key &prefixKey, std::map<Key, Value> &data)
{
    if (storage_ == nullptr) {
        return -E_INVALID_DB;
    }
    return storage_->GetMetaDataByPrefixKey(prefixKey, data);
}

int QuerySyncWaterMarkHelper::SetMetadataToDb(const std::vector<uint8_t> &key, const std::vector<uint8_t> &inValue)
{
    if (storage_ == nullptr) {
        return -E_INVALID_DB;
    }
    return storage_->PutMetaData(key, inValue, false);
}

int QuerySyncWaterMarkHelper::DeleteMetaDataFromDB(const std::vector<Key> &keys) const
{
    if (storage_ == nullptr) {
        return -E_INVALID_DB;
    }
    return storage_->DeleteMetaData(keys);
}

int QuerySyncWaterMarkHelper::Initialize(ISyncInterface *storage)
{
    storage_ = storage;
    return E_OK;
}

int QuerySyncWaterMarkHelper::LoadDeleteSyncDataToCache(const Key &deleteWaterMarkKey)
{
    std::vector<uint8_t> value;
    int errCode = GetMetadataFromDb(deleteWaterMarkKey, value);
    if (errCode != E_OK) {
        return errCode;
    }
    DeleteWaterMark deleteWaterMark;
    std::string dbKey(deleteWaterMarkKey.begin(), deleteWaterMarkKey.end());
    errCode = DeSerializeDeleteWaterMark(value, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    return errCode;
}

int QuerySyncWaterMarkHelper::GetQueryWaterMarkInCacheAndDb(const std::string &cacheKey,
    QueryWaterMark &queryWaterMark)
{
    // second get from db
    int errCode = GetQueryWaterMarkFromDB(cacheKey, queryWaterMark);
    if (errCode == -E_NOT_FOUND) {
        // third generate one and save to db
        errCode = PutQueryWaterMarkToDB(cacheKey, queryWaterMark);
    }
    // something error return
    if (errCode != E_OK) {
        LOGE("[Meta]GetQueryWaterMark Fail code = %d", errCode);
    }
    return errCode;
}

int QuerySyncWaterMarkHelper::GetQueryWaterMark(const std::string &queryIdentify, const std::string &deviceId,
    const std::string &userId, QueryWaterMark &queryWaterMark)
{
    std::string cacheKey = GetHashQuerySyncDeviceId(deviceId, userId, queryIdentify);
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    return GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SetRecvQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const std::string &userId, const WaterMark &waterMark)
{
    std::string cacheKey = GetHashQuerySyncDeviceId(deviceId, userId, queryIdentify);
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    return SetRecvQueryWaterMarkWithoutLock(cacheKey, waterMark);
}

int QuerySyncWaterMarkHelper::SetLastQueryTime(const std::string &queryIdentify,
    const std::string &deviceId, const std::string &userId, const Timestamp &timestamp)
{
    std::string cacheKey = GetHashQuerySyncDeviceId(deviceId, userId, queryIdentify);
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    QueryWaterMark queryWaterMark;
    int errCode = GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.lastQueryTime = timestamp;
    return UpdateCacheAndSave(cacheKey, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SetRecvQueryWaterMarkWithoutLock(const std::string &cacheKey,
    const WaterMark &waterMark)
{
    QueryWaterMark queryWaterMark;
    int errCode = GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.recvWaterMark = waterMark;
    return UpdateCacheAndSave(cacheKey, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SetSendQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const std::string &userId, const WaterMark &waterMark)
{
    std::string cacheKey = GetHashQuerySyncDeviceId(deviceId, userId, queryIdentify);
    QueryWaterMark queryWaterMark;
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    int errCode = GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.sendWaterMark = waterMark;
    return UpdateCacheAndSave(cacheKey, queryWaterMark);
}

int QuerySyncWaterMarkHelper::UpdateCacheAndSave(const std::string &cacheKey,
    QueryWaterMark &queryWaterMark)
{
    // update lastUsedTime
    int errCode = OS::GetCurrentSysTimeInMicrosecond(queryWaterMark.lastUsedTime);
    if (errCode != E_OK) {
        return errCode;
    }
    // save db
    return SaveQueryWaterMarkToDB(cacheKey, queryWaterMark);
}

int QuerySyncWaterMarkHelper::PutQueryWaterMarkToDB(const DeviceID &dbKeyString, QueryWaterMark &queryWaterMark)
{
    int errCode = OS::GetCurrentSysTimeInMicrosecond(queryWaterMark.lastUsedTime);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.version = QUERY_WATERMARK_VERSION_CURRENT;
    return SaveQueryWaterMarkToDB(dbKeyString, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SaveQueryWaterMarkToDB(const DeviceID &dbKeyString, const QueryWaterMark &queryWaterMark)
{
    // serialize value
    Value dbValue;
    int errCode = SerializeQueryWaterMark(queryWaterMark, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    // serialize key
    Key dbKey;
    DBCommon::StringToVector(dbKeyString, dbKey);
    // save
    errCode = SetMetadataToDb(dbKey, dbValue);
    if (errCode != E_OK) {
        LOGE("QuerySyncWaterMarkHelper::SaveQueryWaterMarkToDB failed errCode:%d", errCode);
    }
    return errCode;
}

int QuerySyncWaterMarkHelper::GetQueryWaterMarkFromDB(const DeviceID &dbKeyString, QueryWaterMark &queryWaterMark)
{
    // serialize key
    Key dbKey;
    DBCommon::StringToVector(dbKeyString, dbKey);
    // search in db
    Value dbValue;
    int errCode = GetMetadataFromDb(dbKey, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    return DeSerializeQueryWaterMark(dbValue, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SerializeQueryWaterMark(const QueryWaterMark &queryWaterMark, Value &outValue)
{
    uint64_t length = CalculateQueryWaterMarkSize(queryWaterMark);
    outValue.resize(length);
    Parcel parcel(outValue.data(), outValue.size());
    parcel.WriteUInt32(queryWaterMark.version);
    parcel.EightByteAlign();
    parcel.WriteUInt64(queryWaterMark.sendWaterMark);
    parcel.WriteUInt64(queryWaterMark.recvWaterMark);
    parcel.WriteUInt64(queryWaterMark.lastUsedTime);
    parcel.WriteString(queryWaterMark.sql);
    parcel.WriteUInt64(queryWaterMark.lastQueryTime);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when serialize queryWaterMark");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int QuerySyncWaterMarkHelper::DeSerializeQueryWaterMark(const Value &dbQueryWaterMark, QueryWaterMark &queryWaterMark)
{
    Parcel parcel(const_cast<uint8_t *>(dbQueryWaterMark.data()), dbQueryWaterMark.size());
    parcel.ReadUInt32(queryWaterMark.version);
    parcel.EightByteAlign();
    parcel.ReadUInt64(queryWaterMark.sendWaterMark);
    parcel.ReadUInt64(queryWaterMark.recvWaterMark);
    parcel.ReadUInt64(queryWaterMark.lastUsedTime);
    parcel.ReadString(queryWaterMark.sql);
    if (queryWaterMark.version >= SOFTWARE_VERSION_RELEASE_6_0) {
        parcel.ReadUInt64(queryWaterMark.lastQueryTime);
    }
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when deserialize queryWaterMark");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

uint64_t QuerySyncWaterMarkHelper::CalculateQueryWaterMarkSize(const QueryWaterMark &queryWaterMark)
{
    uint64_t length = Parcel::GetUInt32Len(); // version
    length = Parcel::GetEightByteAlign(length);
    length += Parcel::GetUInt64Len(); // sendWaterMark
    length += Parcel::GetUInt64Len(); // recvWaterMark
    length += Parcel::GetUInt64Len(); // lastUsedTime
    length += Parcel::GetStringLen(queryWaterMark.sql);
    length += Parcel::GetUInt64Len(); // lastQueryTime
    return length;
}

DeviceID QuerySyncWaterMarkHelper::GetHashQuerySyncDeviceId(const DeviceID &deviceId, const DeviceID &userId,
    const DeviceID &queryId)
{
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    DeviceID hashQuerySyncId;
    DeviceSyncTarget deviceInfo(deviceId, userId);
    if (deviceIdToHashQuerySyncIdMap_[deviceInfo].count(queryId) == 0) {
        // do not modify this
        hashQuerySyncId = DBConstant::QUERY_SYNC_PREFIX_KEY + DBCommon::TransferHashString(deviceId) + queryId +
            DBConstant::USERID_PREFIX_KEY + userId;
        deviceIdToHashQuerySyncIdMap_[deviceInfo][queryId] = hashQuerySyncId;
    } else {
        hashQuerySyncId = deviceIdToHashQuerySyncIdMap_[deviceInfo][queryId];
    }
    return hashQuerySyncId;
}

int QuerySyncWaterMarkHelper::GetDeleteSyncWaterMark(const std::string &deviceId, const std::string &userId,
    DeleteWaterMark &deleteWaterMark)
{
    std::string hashId = GetHashDeleteSyncDeviceId(deviceId, userId);
    // lock prevent different thread visit deleteSyncCache_
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    return GetDeleteWaterMarkFromCache(hashId, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::SetSendDeleteSyncWaterMark(const DeviceID &deviceId, const std::string &userId,
    const WaterMark &waterMark)
{
    std::string hashId = GetHashDeleteSyncDeviceId(deviceId, userId);
    DeleteWaterMark deleteWaterMark;
    // lock prevent different thread visit deleteSyncCache_
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    int errCode = GetDeleteWaterMarkFromCache(hashId, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    deleteWaterMark.sendWaterMark = waterMark;
    return UpdateDeleteSyncCacheAndSave(hashId, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::SetRecvDeleteSyncWaterMark(const DeviceID &deviceId, const std::string &userId,
    const WaterMark &waterMark, bool isNeedHash)
{
    std::string hashId = GetHashDeleteSyncDeviceId(deviceId, userId, isNeedHash);
    // lock prevent different thread visit deleteSyncCache_
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    int errCode = E_OK;
    std::map<Key, DeleteWaterMark> deleteWaterMarks;
    if (!userId.empty()) {
        DeleteWaterMark deleteWaterMark;
        Key dbKey;
        DBCommon::StringToVector(hashId, dbKey);
        errCode = GetDeleteWaterMarkFromCache(hashId, deleteWaterMark);
        deleteWaterMark.recvWaterMark = waterMark;
        deleteWaterMarks[dbKey] = deleteWaterMark;
    } else {
        errCode = GetDeleteWatersMarkFromDB(hashId, deleteWaterMarks);
    }
    if (errCode != E_OK) {
        return errCode;
    }
    for (auto &deleteWaterMark : deleteWaterMarks) {
        deleteWaterMark.second.recvWaterMark = waterMark;
        errCode = UpdateDeleteSyncCacheAndSave(hashId, deleteWaterMark.second);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int QuerySyncWaterMarkHelper::UpdateDeleteSyncCacheAndSave(const std::string &dbKey,
    const DeleteWaterMark &deleteWaterMark)
{
    // save db
    return SaveDeleteWaterMarkToDB(dbKey, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::GetDeleteWaterMarkFromCache(const DeviceID &hashDeviceId,
    DeleteWaterMark &deleteWaterMark)
{
    deleteWaterMark.version = DELETE_WATERMARK_VERSION_CURRENT;
    int errCode = GetDeleteWaterMarkFromDB(hashDeviceId, deleteWaterMark);
    if (errCode == -E_NOT_FOUND) {
        deleteWaterMark.sendWaterMark = 0;
        deleteWaterMark.recvWaterMark = 0;
        errCode = E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[Meta]GetDeleteWaterMark Fail code = %d", errCode);
    }
    return errCode;
}

int QuerySyncWaterMarkHelper::GetDeleteWaterMarkFromDB(const DeviceID &hashDeviceId,
    DeleteWaterMark &deleteWaterMark)
{
    Key dbKey;
    DBCommon::StringToVector(hashDeviceId, dbKey);
    // search in db
    Value dbValue;
    int errCode = GetMetadataFromDb(dbKey, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    // serialize value
    return DeSerializeDeleteWaterMark(dbValue, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::GetDeleteWatersMarkFromDB(const DeviceID &hashId,
    std::map<Key, DeleteWaterMark> &deleteWaterMarks)
{
    Key dbKeyPrefix;
    DBCommon::StringToVector(hashId, dbKeyPrefix);
    // search in db
    std::map<Key, Value> dbData;
    int errCode = GetMetadataByPrefixKeyFromDb(dbKeyPrefix, dbData);
    if (errCode == -E_NOT_FOUND) {
        DeleteWaterMark deleteWaterMark;
        deleteWaterMarks[dbKeyPrefix] = deleteWaterMark;
        return E_OK;
    }
    if (errCode != E_OK) {
        return errCode;
    }
    // serialize value
    for (const auto &oneDbData : dbData) {
        DeleteWaterMark deleteWaterMark;
        errCode = DeSerializeDeleteWaterMark(oneDbData.second, deleteWaterMark);
        if (errCode != E_OK) {
            return errCode;
        }
        deleteWaterMarks[oneDbData.first] = deleteWaterMark;
    }
    return E_OK;
}

int QuerySyncWaterMarkHelper::SaveDeleteWaterMarkToDB(const DeviceID &hashDeviceId,
    const DeleteWaterMark &deleteWaterMark)
{
    // serialize value
    Value dbValue;
    int errCode = SerializeDeleteWaterMark(deleteWaterMark, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    Key dbKey;
    DBCommon::StringToVector(hashDeviceId, dbKey);
    // save
    errCode = SetMetadataToDb(dbKey, dbValue);
    if (errCode != E_OK) {
        LOGE("QuerySyncWaterMarkHelper::SaveDeleteWaterMarkToDB failed errCode:%d", errCode);
    }
    return errCode;
}

DeviceID QuerySyncWaterMarkHelper::GetHashDeleteSyncDeviceId(const DeviceID &deviceId, const DeviceID &userId,
    bool isNeedHash)
{
    DeviceID hashDeleteSyncId;
    DeviceSyncTarget deviceInfo(deviceId, userId);
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    if (deviceIdToHashDeleteSyncIdMap_.count(deviceInfo) == 0) {
        hashDeleteSyncId = DBConstant::DELETE_SYNC_PREFIX_KEY +
            (isNeedHash ? DBCommon::TransferHashString(deviceId) : deviceId) + DBConstant::USERID_PREFIX_KEY + userId;
        deviceIdToHashDeleteSyncIdMap_.insert(std::pair<DeviceSyncTarget, DeviceID>(deviceInfo, hashDeleteSyncId));
    } else {
        hashDeleteSyncId = deviceIdToHashDeleteSyncIdMap_[deviceInfo];
    }
    return hashDeleteSyncId;
}

int QuerySyncWaterMarkHelper::SerializeDeleteWaterMark(const DeleteWaterMark &deleteWaterMark,
    std::vector<uint8_t> &outValue)
{
    uint64_t length = CalculateDeleteWaterMarkSize();
    outValue.resize(length);
    Parcel parcel(outValue.data(), outValue.size());
    parcel.WriteUInt32(deleteWaterMark.version);
    parcel.EightByteAlign();
    parcel.WriteUInt64(deleteWaterMark.sendWaterMark);
    parcel.WriteUInt64(deleteWaterMark.recvWaterMark);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when serialize deleteWaterMark.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int QuerySyncWaterMarkHelper::DeSerializeDeleteWaterMark(const std::vector<uint8_t> &inValue,
    DeleteWaterMark &deleteWaterMark)
{
    Parcel parcel(const_cast<uint8_t *>(inValue.data()), inValue.size());
    parcel.ReadUInt32(deleteWaterMark.version);
    parcel.EightByteAlign();
    parcel.ReadUInt64(deleteWaterMark.sendWaterMark);
    parcel.ReadUInt64(deleteWaterMark.recvWaterMark);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when deserialize deleteWaterMark.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

uint64_t QuerySyncWaterMarkHelper::CalculateDeleteWaterMarkSize()
{
    uint64_t length = Parcel::GetUInt32Len(); // version
    length = Parcel::GetEightByteAlign(length);
    length += Parcel::GetUInt64Len(); // sendWaterMark
    length += Parcel::GetUInt64Len(); // recvWaterMark
    return length;
}

std::string QuerySyncWaterMarkHelper::GetQuerySyncPrefixKey()
{
    return DBConstant::QUERY_SYNC_PREFIX_KEY;
}

std::string QuerySyncWaterMarkHelper::GetDeleteSyncPrefixKey()
{
    return DBConstant::DELETE_SYNC_PREFIX_KEY;
}

int QuerySyncWaterMarkHelper::RemoveLeastUsedQuerySyncItems(const std::vector<Key> &querySyncIds)
{
    if (querySyncIds.size() < MAX_STORE_ITEMS) {
        return E_OK;
    }
    std::vector<std::pair<std::string, Timestamp>> allItems;
    std::map<std::string, std::vector<uint8_t>> idMap;
    std::vector<std::vector<uint8_t>> waitToRemove;
    for (const auto &id : querySyncIds) {
        Value value;
        int errCode = GetMetadataFromDb(id, value);
        if (errCode != E_OK) {
            waitToRemove.push_back(id);
            continue; // may be this failure cause by wrong data
        }
        QueryWaterMark queryWaterMark;
        std::string queryKey(id.begin(), id.end());
        errCode = DeSerializeQueryWaterMark(value, queryWaterMark);
        if (errCode != E_OK) {
            waitToRemove.push_back(id);
            continue; // may be this failure cause by wrong data
        }
        idMap.insert({queryKey, id});
        allItems.emplace_back(queryKey, queryWaterMark.lastUsedTime);
    }
    // we only remove broken data below
    // 1. common data size less then 10w
    // 2. allItems.size() - MAX_STORE_ITEMS - waitToRemove.size() < 0
    // so we only let allItems.size() < MAX_STORE_ITEMS + waitToRemove.size()
    if (allItems.size() < MAX_STORE_ITEMS + waitToRemove.size()) {
        // remove in db
        return DeleteMetaDataFromDB(waitToRemove);
    }
    uint32_t removeCount = allItems.size() - MAX_STORE_ITEMS - waitToRemove.size();
    // quick select the k_th least used
    std::nth_element(allItems.begin(), allItems.begin() + removeCount, allItems.end(),
        [](const std::pair<std::string, Timestamp> &w1, const std::pair<std::string, Timestamp> &w2) {
            return w1.second < w2.second;
        });
    for (uint32_t i = 0; i < removeCount; ++i) {
        waitToRemove.push_back(idMap[allItems[i].first]);
    }
    // remove in db
    return DeleteMetaDataFromDB(waitToRemove);
}

int QuerySyncWaterMarkHelper::ResetRecvQueryWaterMark(const DeviceID &deviceId, const DeviceID &userId,
    const std::string &tableName, bool isNeedHash)
{
    // lock prevent other thread modify queryWaterMark at this moment
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    std::string prefixKeyStr = DBConstant::QUERY_SYNC_PREFIX_KEY +
        (isNeedHash ? DBCommon::TransferHashString(deviceId) : deviceId);
    if (!tableName.empty()) {
        std::string hashTableName = DBCommon::TransferHashString(tableName);
        std::string hexTableName = DBCommon::TransferStringToHex(hashTableName);
        prefixKeyStr += hexTableName;
    }
    if (!userId.empty()) {
        prefixKeyStr += DBConstant::USERID_PREFIX_KEY + userId;
    }

    // remove in db
    Key prefixKey;
    DBCommon::StringToVector(prefixKeyStr, prefixKey);
    int errCode = storage_->DeleteMetaDataByPrefixKey(prefixKey);
    if (errCode != E_OK) {
        LOGE("[META]ResetRecvQueryWaterMark fail errCode:%d", errCode);
        return errCode;
    }
    return E_OK;
}
}  // namespace DistributedDB