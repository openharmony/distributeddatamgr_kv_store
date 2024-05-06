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

#include "meta_data.h"

#include <openssl/rand.h>

#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "hash.h"
#include "log_print.h"
#include "platform_specific.h"
#include "securec.h"
#include "sync_types.h"
#include "time_helper.h"
#include "version.h"

namespace DistributedDB {
namespace {
    // store local timeoffset;this is a special key;
    constexpr const char *CLIENT_ID_PREFIX_KEY = "clientId";
    constexpr const char *LOCAL_META_DATA_KEY = "localMetaData";
}

Metadata::Metadata()
    : localTimeOffset_(0),
      naturalStoragePtr_(nullptr),
      lastLocalTime_(0)
{}

Metadata::~Metadata()
{
    naturalStoragePtr_ = nullptr;
    metadataMap_.clear();
}

int Metadata::Initialize(ISyncInterface* storage)
{
    naturalStoragePtr_ = storage;
    std::vector<uint8_t> key;
    std::vector<uint8_t> timeOffset;
    DBCommon::StringToVector(std::string(DBConstant::LOCALTIME_OFFSET_KEY), key);

    int errCode = GetMetadataFromDb(key, timeOffset);
    if (errCode == -E_NOT_FOUND) {
        int err = SaveLocalTimeOffset(TimeHelper::BASE_OFFSET);
        if (err != E_OK) {
            LOGD("[Metadata][Initialize]SaveLocalTimeOffset failed errCode:%d", err);
            return err;
        }
    } else if (errCode == E_OK) {
        localTimeOffset_ = StringToLong(timeOffset);
    } else {
        LOGE("Metadata::Initialize get meatadata from db failed,err=%d", errCode);
        return errCode;
    }
    {
        std::lock_guard<std::mutex> lockGuard(metadataLock_);
        metadataMap_.clear();
    }
    (void)querySyncWaterMarkHelper_.Initialize(storage);
    return LoadAllMetadata();
}

int Metadata::SaveTimeOffset(const DeviceID &deviceId, TimeOffset inValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    metadata.timeOffset = inValue;
    metadata.lastUpdateTime = TimeHelper::GetSysCurrentTime();
    LOGD("Metadata::SaveTimeOffset = %" PRId64 " dev %s", inValue, STR_MASK(deviceId));
    return SaveMetaDataValue(deviceId, metadata);
}

void Metadata::GetTimeOffset(const DeviceID &deviceId, TimeOffset &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    outValue = metadata.timeOffset;
}

void Metadata::GetLocalWaterMark(const DeviceID &deviceId, uint64_t &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    outValue = metadata.localWaterMark;
}

int Metadata::SaveLocalWaterMark(const DeviceID &deviceId, uint64_t inValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    metadata.localWaterMark = inValue;
    LOGD("Metadata::SaveLocalWaterMark = %" PRIu64, inValue);
    return SaveMetaDataValue(deviceId, metadata);
}

void Metadata::GetPeerWaterMark(const DeviceID &deviceId, uint64_t &outValue, bool isNeedHash)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, isNeedHash);
    outValue = metadata.peerWaterMark;
}

int Metadata::SavePeerWaterMark(const DeviceID &deviceId, uint64_t inValue, bool isNeedHash)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, isNeedHash);
    metadata.peerWaterMark = inValue;
    LOGD("Metadata::SavePeerWaterMark = %" PRIu64, inValue);
    return SaveMetaDataValue(deviceId, metadata, isNeedHash);
}

int Metadata::SaveLocalTimeOffset(TimeOffset timeOffset)
{
    std::string timeOffsetString = std::to_string(timeOffset);
    std::vector<uint8_t> timeOffsetValue(timeOffsetString.begin(), timeOffsetString.end());
    std::string keyStr(DBConstant::LOCALTIME_OFFSET_KEY);
    std::vector<uint8_t> localTimeOffsetValue(keyStr.begin(), keyStr.end());

    std::lock_guard<std::mutex> lockGuard(localTimeOffsetLock_);
    localTimeOffset_ = timeOffset;
    LOGD("Metadata::SaveLocalTimeOffset offset = %" PRId64, timeOffset);
    int errCode = SetMetadataToDb(localTimeOffsetValue, timeOffsetValue);
    if (errCode != E_OK) {
        LOGE("Metadata::SaveLocalTimeOffset SetMetadataToDb failed errCode:%d", errCode);
    }
    return errCode;
}

TimeOffset Metadata::GetLocalTimeOffset() const
{
    TimeOffset localTimeOffset = localTimeOffset_.load(std::memory_order_seq_cst);
    return localTimeOffset;
}

int Metadata::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash)
{
    return EraseDeviceWaterMark(deviceId, isNeedHash, "");
}

int Metadata::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash, const std::string &tableName)
{
    // reload meta data again
    (void)LoadAllMetadata();
    std::lock_guard<std::recursive_mutex> autoLock(waterMarkMutex_);
    // try to erase all the waterMark
    // erase deleteSync recv waterMark
    WaterMark waterMark = 0;
    int errCodeDeleteSync = SetRecvDeleteSyncWaterMark(deviceId, waterMark, isNeedHash);
    if (errCodeDeleteSync != E_OK) {
        LOGE("[Metadata] erase deleteWaterMark failed errCode:%d", errCodeDeleteSync);
        return errCodeDeleteSync;
    }
    // erase querySync recv waterMark
    int errCodeQuerySync = ResetRecvQueryWaterMark(deviceId, tableName, isNeedHash);
    if (errCodeQuerySync != E_OK) {
        LOGE("[Metadata] erase queryWaterMark failed errCode:%d", errCodeQuerySync);
        return errCodeQuerySync;
    }
    // peerWaterMark must be erased at last
    int errCode = SavePeerWaterMark(deviceId, 0, isNeedHash);
    if (errCode != E_OK) {
        LOGE("[Metadata] erase peerWaterMark failed errCode:%d", errCode);
        return errCode;
    }
    return E_OK;
}

void Metadata::SetLastLocalTime(Timestamp lastLocalTime)
{
    std::lock_guard<std::mutex> lock(lastLocalTimeLock_);
    if (lastLocalTime > lastLocalTime_) {
        lastLocalTime_ = lastLocalTime;
    }
}

Timestamp Metadata::GetLastLocalTime() const
{
    std::lock_guard<std::mutex> lock(lastLocalTimeLock_);
    return lastLocalTime_;
}

int Metadata::SaveMetaDataValue(const DeviceID &deviceId, const MetaDataValue &inValue, bool isNeedHash)
{
    std::vector<uint8_t> value;
    int errCode = SerializeMetaData(inValue, value);
    if (errCode != E_OK) {
        return errCode;
    }

    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, isNeedHash);
    std::vector<uint8_t> key;
    DBCommon::StringToVector(hashDeviceId, key);
    errCode = SetMetadataToDb(key, value);
    if (errCode != E_OK) {
        LOGE("Metadata::SetMetadataToDb failed errCode:%d", errCode);
        return errCode;
    }
    PutMetadataToMap(hashDeviceId, inValue);
    return E_OK;
}

void Metadata::GetMetaDataValue(const DeviceID &deviceId, MetaDataValue &outValue, bool isNeedHash)
{
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, isNeedHash);
    GetMetadataFromMap(hashDeviceId, outValue);
}

int Metadata::SerializeMetaData(const MetaDataValue &inValue, std::vector<uint8_t> &outValue)
{
    outValue.resize(sizeof(MetaDataValue));
    errno_t err = memcpy_s(outValue.data(), outValue.size(), &inValue, sizeof(MetaDataValue));
    if (err != EOK) {
        return -E_SECUREC_ERROR;
    }
    return E_OK;
}

int Metadata::DeSerializeMetaData(const std::vector<uint8_t> &inValue, MetaDataValue &outValue)
{
    if (inValue.empty()) {
        return -E_INVALID_ARGS;
    }
    errno_t err = memcpy_s(&outValue, sizeof(MetaDataValue), inValue.data(), inValue.size());
    if (err != EOK) {
        return -E_SECUREC_ERROR;
    }
    return E_OK;
}

int Metadata::GetMetadataFromDb(const std::vector<uint8_t> &key, std::vector<uint8_t> &outValue) const
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->GetMetaData(key, outValue);
}

int Metadata::SetMetadataToDb(const std::vector<uint8_t> &key, const std::vector<uint8_t> &inValue)
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->PutMetaData(key, inValue, false);
}

void Metadata::PutMetadataToMap(const DeviceID &deviceId, const MetaDataValue &value)
{
    metadataMap_[deviceId] = value;
}

void Metadata::GetMetadataFromMap(const DeviceID &deviceId, MetaDataValue &outValue)
{
    outValue = metadataMap_[deviceId];
}

int64_t Metadata::StringToLong(const std::vector<uint8_t> &value) const
{
    std::string valueString(value.begin(), value.end());
    int64_t longData = std::strtoll(valueString.c_str(), nullptr, DBConstant::STR_TO_LL_BY_DEVALUE);
    LOGD("Metadata::StringToLong longData = %" PRId64, longData);
    return longData;
}

int Metadata::GetAllMetadataKey(std::vector<std::vector<uint8_t>> &keys)
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->GetAllMetaKeys(keys);
}

namespace {
bool IsMetaDataKey(const Key &inKey, const std::string &expectPrefix)
{
    if (inKey.size() < expectPrefix.size()) {
        return false;
    }
    std::string prefixInKey(inKey.begin(), inKey.begin() + expectPrefix.size());
    if (prefixInKey != expectPrefix) {
        return false;
    }
    return true;
}
}

int Metadata::LoadAllMetadata()
{
    std::vector<std::vector<uint8_t>> metaDataKeys;
    int errCode = GetAllMetadataKey(metaDataKeys);
    if (errCode != E_OK) {
        LOGE("[Metadata] get all metadata key failed err=%d", errCode);
        return errCode;
    }

    std::vector<std::vector<uint8_t>> querySyncIds;
    for (const auto &deviceId : metaDataKeys) {
        if (IsMetaDataKey(deviceId, DBConstant::DEVICEID_PREFIX_KEY)) {
            errCode = LoadDeviceIdDataToMap(deviceId);
            if (errCode != E_OK) {
                return errCode;
            }
        } else if (IsMetaDataKey(deviceId, QuerySyncWaterMarkHelper::GetQuerySyncPrefixKey())) {
            querySyncIds.push_back(deviceId);
        } else if (IsMetaDataKey(deviceId, QuerySyncWaterMarkHelper::GetDeleteSyncPrefixKey())) {
            errCode = querySyncWaterMarkHelper_.LoadDeleteSyncDataToCache(deviceId);
            if (errCode != E_OK) {
                return errCode;
            }
        }
    }
    errCode = querySyncWaterMarkHelper_.RemoveLeastUsedQuerySyncItems(querySyncIds);
    if (errCode != E_OK) {
        return errCode;
    }
    return InitLocalMetaData();
}

int Metadata::LoadDeviceIdDataToMap(const Key &key)
{
    std::vector<uint8_t> value;
    int errCode = GetMetadataFromDb(key, value);
    if (errCode != E_OK) {
        return errCode;
    }
    MetaDataValue metaValue;
    std::string metaKey(key.begin(), key.end());
    errCode = DeSerializeMetaData(value, metaValue);
    if (errCode != E_OK) {
        return errCode;
    }
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    PutMetadataToMap(metaKey, metaValue);
    return errCode;
}

void Metadata::GetHashDeviceId(const DeviceID &deviceId, DeviceID &hashDeviceId, bool isNeedHash)
{
    if (!isNeedHash) {
        hashDeviceId = DBConstant::DEVICEID_PREFIX_KEY + deviceId;
        return;
    }
    if (deviceIdToHashDeviceIdMap_.count(deviceId) == 0) {
        hashDeviceId = DBConstant::DEVICEID_PREFIX_KEY + DBCommon::TransferHashString(deviceId);
        deviceIdToHashDeviceIdMap_.insert(std::pair<DeviceID, DeviceID>(deviceId, hashDeviceId));
    } else {
        hashDeviceId = deviceIdToHashDeviceIdMap_[deviceId];
    }
}

int Metadata::GetRecvQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, WaterMark &waterMark)
{
    QueryWaterMark queryWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetQueryWaterMark(queryIdentify, deviceId, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    WaterMark peerWaterMark;
    GetPeerWaterMark(deviceId, peerWaterMark);
    waterMark = std::max(queryWaterMark.recvWaterMark, peerWaterMark);
    return E_OK;
}

int Metadata::SetRecvQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const WaterMark &waterMark)
{
    return querySyncWaterMarkHelper_.SetRecvQueryWaterMark(queryIdentify, deviceId, waterMark);
}

int Metadata::GetSendQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, WaterMark &waterMark, bool isAutoLift)
{
    QueryWaterMark queryWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetQueryWaterMark(queryIdentify, deviceId, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    if (isAutoLift) {
        WaterMark localWaterMark;
        GetLocalWaterMark(deviceId, localWaterMark);
        waterMark = std::max(queryWaterMark.sendWaterMark, localWaterMark);
    } else {
        waterMark = queryWaterMark.sendWaterMark;
    }
    return E_OK;
}

int Metadata::SetSendQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const WaterMark &waterMark)
{
    return querySyncWaterMarkHelper_.SetSendQueryWaterMark(queryIdentify, deviceId, waterMark);
}

int Metadata::GetLastQueryTime(const std::string &queryIdentify, const std::string &deviceId, Timestamp &timestamp)
{
    QueryWaterMark queryWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetQueryWaterMark(queryIdentify, deviceId, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    timestamp = queryWaterMark.lastQueryTime;
    return E_OK;
}

int Metadata::SetLastQueryTime(const std::string &queryIdentify, const std::string &deviceId,
    const Timestamp &timestamp)
{
    return querySyncWaterMarkHelper_.SetLastQueryTime(queryIdentify, deviceId, timestamp);
}

int Metadata::GetSendDeleteSyncWaterMark(const DeviceID &deviceId, WaterMark &waterMark, bool isAutoLift)
{
    DeleteWaterMark deleteWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetDeleteSyncWaterMark(deviceId, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    if (isAutoLift) {
        WaterMark localWaterMark;
        GetLocalWaterMark(deviceId, localWaterMark);
        waterMark = std::max(deleteWaterMark.sendWaterMark, localWaterMark);
    } else {
        waterMark = deleteWaterMark.sendWaterMark;
    }
    return E_OK;
}

int Metadata::SetSendDeleteSyncWaterMark(const DeviceID &deviceId, const WaterMark &waterMark)
{
    return querySyncWaterMarkHelper_.SetSendDeleteSyncWaterMark(deviceId, waterMark);
}

int Metadata::GetRecvDeleteSyncWaterMark(const DeviceID &deviceId, WaterMark &waterMark)
{
    DeleteWaterMark deleteWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetDeleteSyncWaterMark(deviceId, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    WaterMark peerWaterMark;
    GetPeerWaterMark(deviceId, peerWaterMark);
    waterMark = std::max(deleteWaterMark.recvWaterMark, peerWaterMark);
    return E_OK;
}

int Metadata::SetRecvDeleteSyncWaterMark(const DeviceID &deviceId, const WaterMark &waterMark, bool isNeedHash)
{
    return querySyncWaterMarkHelper_.SetRecvDeleteSyncWaterMark(deviceId, waterMark, isNeedHash);
}

int Metadata::ResetRecvQueryWaterMark(const DeviceID &deviceId, const std::string &tableName, bool isNeedHash)
{
    return querySyncWaterMarkHelper_.ResetRecvQueryWaterMark(deviceId, tableName, isNeedHash);
}

void Metadata::GetDbCreateTime(const DeviceID &deviceId, uint64_t &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        outValue = metadata.dbCreateTime;
        return;
    }
    outValue = 0;
    LOGI("Metadata::GetDbCreateTime, not found dev = %s dbCreateTime", STR_MASK(deviceId));
}

int Metadata::SetDbCreateTime(const DeviceID &deviceId, uint64_t inValue, bool isNeedHash)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, isNeedHash);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        if (metadata.dbCreateTime != 0 && metadata.dbCreateTime != inValue) {
            metadata.clearDeviceDataMark = REMOVE_DEVICE_DATA_MARK;
            LOGI("Metadata::SetDbCreateTime,set clear data mark,dev=%s,dbCreateTime=%" PRIu64,
                STR_MASK(deviceId), inValue);
        }
        if (metadata.dbCreateTime == 0) {
            LOGI("Metadata::SetDbCreateTime,update dev=%s,dbCreateTime=%" PRIu64, STR_MASK(deviceId), inValue);
        }
    }
    metadata.dbCreateTime = inValue;
    return SaveMetaDataValue(deviceId, metadata);
}

int Metadata::ResetMetaDataAfterRemoveData(const DeviceID &deviceId)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        metadata.clearDeviceDataMark = 0; // clear mark
        return SaveMetaDataValue(deviceId, metadata);
    }
    return -E_NOT_FOUND;
}

void Metadata::GetRemoveDataMark(const DeviceID &deviceId, uint64_t &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        outValue = metadata.clearDeviceDataMark;
        return;
    }
    outValue = 0;
}

uint64_t Metadata::GetQueryLastTimestamp(const DeviceID &deviceId, const std::string &queryId) const
{
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
    std::string hashqueryId = DBConstant::SUBSCRIBE_QUERY_PREFIX + DBCommon::TransferHashString(queryId);
    DBCommon::StringToVector(hashqueryId, key);
    int errCode = GetMetadataFromDb(key, value);
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    if (errCode == -E_NOT_FOUND) {
        auto iter = queryIdMap_.find(deviceId);
        if (iter != queryIdMap_.end()) {
            if (iter->second.find(hashqueryId) == iter->second.end()) {
                iter->second.insert(hashqueryId);
                return INT64_MAX;
            }
            return 0;
        } else {
            queryIdMap_[deviceId] = { hashqueryId };
            return INT64_MAX;
        }
    }
    auto iter = queryIdMap_.find(deviceId);
    // while value is found in db, it can be found in db later when db is not closed
    // so no need to record the hashqueryId in map
    if (errCode == E_OK && iter != queryIdMap_.end()) {
        iter->second.erase(hashqueryId);
    }
    return StringToLong(value);
}

void Metadata::RemoveQueryFromRecordSet(const DeviceID &deviceId, const std::string &queryId)
{
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    std::string hashqueryId = DBConstant::SUBSCRIBE_QUERY_PREFIX + DBCommon::TransferHashString(queryId);
    auto iter = queryIdMap_.find(deviceId);
    if (iter != queryIdMap_.end() && iter->second.find(hashqueryId) != iter->second.end()) {
        iter->second.erase(hashqueryId);
    }
}

int Metadata::SaveClientId(const std::string &deviceId, const std::string &clientId)
{
    {
        // already save in cache
        std::lock_guard<std::mutex> autoLock(clientIdLock_);
        if (clientIdCache_[deviceId] == clientId) {
            return E_OK;
        }
    }
    std::string keyStr;
    keyStr.append(CLIENT_ID_PREFIX_KEY).append(clientId);
    std::string valueStr = DBCommon::TransferHashString(deviceId);
    Key key;
    DBCommon::StringToVector(keyStr, key);
    Value value;
    DBCommon::StringToVector(valueStr, value);
    int errCode = SetMetadataToDb(key, value);
    if (errCode != E_OK) {
        return errCode;
    }
    std::lock_guard<std::mutex> autoLock(clientIdLock_);
    clientIdCache_[deviceId] = clientId;
    return E_OK;
}

int Metadata::GetHashDeviceId(const std::string &clientId, std::string &hashDevId) const
{
    // don't use cache here avoid invalid cache
    std::string keyStr;
    keyStr.append(CLIENT_ID_PREFIX_KEY).append(clientId);
    Key key;
    DBCommon::StringToVector(keyStr, key);
    Value value;
    int errCode = GetMetadataFromDb(key, value);
    if (errCode == -E_NOT_FOUND) {
        LOGD("[Metadata] not found clientId by %.3s", clientId.c_str());
        return -E_NOT_SUPPORT;
    }
    if (errCode != E_OK) {
        LOGE("[Metadata] reload clientId failed %d", errCode);
        return errCode;
    }
    DBCommon::VectorToString(value, hashDevId);
    return E_OK;
}

void Metadata::LockWaterMark() const
{
    waterMarkMutex_.lock();
}

void Metadata::UnlockWaterMark() const
{
    waterMarkMutex_.unlock();
}

int Metadata::GetWaterMarkInfoFromDB(const std::string &dev, bool isNeedHash, WatermarkInfo &info)
{
    Key devKey;
    DeviceID hashDeviceId;
    {
        std::lock_guard<std::mutex> lockGuard(metadataLock_);
        GetHashDeviceId(dev, hashDeviceId, isNeedHash);
        DBCommon::StringToVector(hashDeviceId, devKey);
    }
    // read from db avoid watermark update in diff process
    int errCode = LoadDeviceIdDataToMap(devKey);
    if (errCode == -E_NOT_FOUND) {
        LOGD("[Metadata] not found meta value");
        return E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[Metadata] reload meta value failed %d", errCode);
        return errCode;
    }
    {
        std::lock_guard<std::mutex> lockGuard(metadataLock_);
        MetaDataValue metadata;
        GetMetadataFromMap(hashDeviceId, metadata);
        info.sendMark = metadata.localWaterMark;
        info.receiveMark = metadata.peerWaterMark;
    }
    return E_OK;
}

int Metadata::ClearAllAbilitySyncFinishMark()
{
    return ClearAllMetaDataValue(static_cast<uint32_t>(MetaValueAction::CLEAR_ABILITY_SYNC_MARK) |
        static_cast<uint32_t>(MetaValueAction::CLEAR_REMOTE_SCHEMA_VERSION));
}

int Metadata::ClearAllTimeSyncFinishMark()
{
    return ClearAllMetaDataValue(static_cast<uint32_t>(MetaValueAction::CLEAR_TIME_SYNC_MARK) |
        static_cast<uint32_t>(MetaValueAction::CLEAR_SYSTEM_TIME_OFFSET) |
        static_cast<uint32_t>(MetaValueAction::SET_TIME_CHANGE_MARK));
}

int Metadata::ClearAllMetaDataValue(uint32_t innerClearAction)
{
    int errCode = E_OK;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    for (auto &[hashDev, metaDataValue] : metadataMap_) {
        ClearMetaDataValue(innerClearAction, metaDataValue);
        std::vector<uint8_t> value;
        int innerErrCode = SerializeMetaData(metaDataValue, value);
        // clear sync mark without transaction here
        // just ignore error metadata value
        if (innerErrCode != E_OK) {
            LOGW("[Metadata][ClearAllMetaDataValue] action %" PRIu32 " serialize meta data failed %d", innerClearAction,
                innerErrCode);
            errCode = errCode == E_OK ? innerErrCode : errCode;
            continue;
        }

        std::vector<uint8_t> key;
        DBCommon::StringToVector(hashDev, key);
        innerErrCode = SetMetadataToDb(key, value);
        if (innerErrCode != E_OK) {
            LOGW("[Metadata][ClearAllMetaDataValue] action %" PRIu32 " save meta data failed %d", innerClearAction,
                innerErrCode);
            errCode = errCode == E_OK ? innerErrCode : errCode;
        }
    }
    if (errCode == E_OK) {
        LOGI("[Metadata][ClearAllMetaDataValue] success clear action %" PRIu32, innerClearAction);
    }
    return errCode;
}

void Metadata::ClearMetaDataValue(uint32_t innerClearAction, MetaDataValue &metaDataValue)
{
    auto mark = static_cast<uint32_t>(MetaValueAction::CLEAR_ABILITY_SYNC_MARK);
    if ((innerClearAction & mark) == mark) {
        metaDataValue.syncMark =
            DBCommon::EraseBit(metaDataValue.syncMark, static_cast<uint64_t>(SyncMark::SYNC_MARK_ABILITY_SYNC));
    }
    mark = static_cast<uint32_t>(MetaValueAction::CLEAR_TIME_SYNC_MARK);
    if ((innerClearAction & mark) == mark) {
        metaDataValue.syncMark =
            DBCommon::EraseBit(metaDataValue.syncMark, static_cast<uint64_t>(SyncMark::SYNC_MARK_TIME_SYNC));
    }
    mark = static_cast<uint32_t>(MetaValueAction::CLEAR_REMOTE_SCHEMA_VERSION);
    if ((innerClearAction & mark) == mark) {
        metaDataValue.remoteSchemaVersion = 0;
    }
    mark = static_cast<uint32_t>(MetaValueAction::CLEAR_SYSTEM_TIME_OFFSET);
    if ((innerClearAction & mark) == mark) {
        metaDataValue.systemTimeOffset = 0;
    }
    mark = static_cast<uint32_t>(MetaValueAction::SET_TIME_CHANGE_MARK);
    if ((innerClearAction & mark) == mark) {
        metaDataValue.syncMark |= static_cast<uint64_t>(SyncMark::SYNC_MARK_TIME_CHANGE);
    }
}

int Metadata::SetAbilitySyncFinishMark(const std::string &deviceId, bool finish)
{
    return SetSyncMark(deviceId, SyncMark::SYNC_MARK_ABILITY_SYNC, finish);
}

bool Metadata::IsAbilitySyncFinish(const std::string &deviceId)
{
    return IsContainSyncMark(deviceId, SyncMark::SYNC_MARK_ABILITY_SYNC);
}

int Metadata::SetTimeSyncFinishMark(const std::string &deviceId, bool finish)
{
    return SetSyncMark(deviceId, SyncMark::SYNC_MARK_TIME_SYNC, finish);
}

int Metadata::SetTimeChangeMark(const std::string &deviceId, bool change)
{
    return SetSyncMark(deviceId, SyncMark::SYNC_MARK_TIME_CHANGE, change);
}

bool Metadata::IsTimeSyncFinish(const std::string &deviceId)
{
    return IsContainSyncMark(deviceId, SyncMark::SYNC_MARK_TIME_SYNC);
}

bool Metadata::IsTimeChange(const std::string &deviceId)
{
    return IsContainSyncMark(deviceId, SyncMark::SYNC_MARK_TIME_CHANGE);
}

int Metadata::SetSyncMark(const std::string &deviceId, SyncMark syncMark, bool finish)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    auto mark = static_cast<uint64_t>(syncMark);
    if (finish) {
        metadata.syncMark |= mark;
    } else {
        metadata.syncMark = DBCommon::EraseBit(metadata.syncMark, mark);
    }
    LOGD("[Metadata] Mark:%" PRIx64 " sync finish:%d sync mark:%" PRIu64, mark, static_cast<int>(finish),
        metadata.syncMark);
    return SaveMetaDataValue(deviceId, metadata);
}

bool Metadata::IsContainSyncMark(const std::string &deviceId, SyncMark syncMark)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    auto mark = static_cast<uint64_t>(syncMark);
    return (metadata.syncMark & mark) == mark;
}

int Metadata::SetRemoteSchemaVersion(const std::string &deviceId, uint64_t schemaVersion)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    metadata.remoteSchemaVersion = schemaVersion;
    LOGI("[Metadata] Set %.3s schema version %" PRIu64, deviceId.c_str(), schemaVersion);
    int errCode = SaveMetaDataValue(deviceId, metadata);
    if (errCode != E_OK) {
        LOGW("[Metadata] Set remote schema version failed");
    }
    return errCode;
}

uint64_t Metadata::GetRemoteSchemaVersion(const std::string &deviceId)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    LOGI("[Metadata] Get %.3s schema version %" PRIu64, deviceId.c_str(), metadata.remoteSchemaVersion);
    return metadata.remoteSchemaVersion;
}

int Metadata::SetSystemTimeOffset(const std::string &deviceId, int64_t systemTimeOffset)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    metadata.systemTimeOffset = systemTimeOffset;
    LOGI("[Metadata] Set %.3s systemTimeOffset %" PRId64, deviceId.c_str(), systemTimeOffset);
    return SaveMetaDataValue(deviceId, metadata);
}

int64_t Metadata::GetSystemTimeOffset(const std::string &deviceId)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    LOGI("[Metadata] Get %.3s systemTimeOffset %" PRId64, deviceId.c_str(), metadata.systemTimeOffset);
    return metadata.systemTimeOffset;
}

std::pair<int, uint64_t> Metadata::GetLocalSchemaVersion()
{
    std::lock_guard<std::mutex> autoLock(localMetaDataMutex_);
    auto [errCode, localMetaData] = GetLocalMetaData();
    if (errCode != E_OK) {
        return {errCode, 0};
    }
    LOGI("[Metadata] Get local schema version %" PRIu64, localMetaData.localSchemaVersion);
    return {errCode, localMetaData.localSchemaVersion};
}

int Metadata::SetLocalSchemaVersion(uint64_t schemaVersion)
{
    std::lock_guard<std::mutex> autoLock(localMetaDataMutex_);
    auto [errCode, localMetaData] = GetLocalMetaData();
    if (errCode != E_OK) {
        return errCode;
    }
    localMetaData.localSchemaVersion = schemaVersion;
    LOGI("[Metadata] Set local schema version %" PRIu64, schemaVersion);
    return SaveLocalMetaData(localMetaData);
}

int Metadata::SaveLocalMetaData(const LocalMetaData &localMetaData)
{
    auto [errCode, value] = SerializeLocalMetaData(localMetaData);
    if (errCode != E_OK) {
        LOGE("[Metadata] Serialize local meta data failed %d", errCode);
        return errCode;
    }
    std::string k(LOCAL_META_DATA_KEY);
    Key key(k.begin(), k.end());
    return SetMetadataToDb(key, value);
}

std::pair<int, LocalMetaData> Metadata::GetLocalMetaData()
{
    std::string k(LOCAL_META_DATA_KEY);
    Key key(k.begin(), k.end());
    Value value;
    int errCode = GetMetadataFromDb(key, value);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        return {errCode, {}};
    }
    return DeSerializeLocalMetaData(value);
}

std::pair<int, Value> Metadata::SerializeLocalMetaData(const LocalMetaData &localMetaData)
{
    std::pair<int, Value> res = {E_OK, {}};
    auto &[errCode, value] = res;
    value.resize(CalculateLocalMetaDataLength());
    Parcel parcel(value.data(), value.size());
    (void)parcel.WriteUInt32(localMetaData.version);
    (void)parcel.WriteUInt64(localMetaData.localSchemaVersion);
    if (parcel.IsError()) {
        LOGE("[Metadata] Serialize localMetaData failed");
        errCode = -E_SERIALIZE_ERROR;
        value.clear();
        return res;
    }
    return res;
}

std::pair<int, LocalMetaData> Metadata::DeSerializeLocalMetaData(const Value &value)
{
    std::pair<int, LocalMetaData> res;
    auto &[errCode, meta] = res;
    if (value.empty()) {
        errCode = E_OK;
        meta.version = SOFTWARE_VERSION_RELEASE_8_0;
        return res;
    }
    Parcel parcel(const_cast<uint8_t *>(value.data()), value.size());
    parcel.ReadUInt32(meta.version);
    parcel.ReadUInt64(meta.localSchemaVersion);
    if (parcel.IsError()) {
        LOGE("[Metadata] DeSerialize localMetaData failed");
        errCode = -E_SERIALIZE_ERROR;
        return res;
    }
    return res;
}

uint64_t Metadata::CalculateLocalMetaDataLength()
{
    uint64_t length = Parcel::GetUInt32Len(); // version
    length += Parcel::GetUInt64Len(); // local schema version
    return length;
}

Metadata::MetaWaterMarkAutoLock::MetaWaterMarkAutoLock(std::shared_ptr<Metadata> metadata)
    : metadataPtr_(std::move(metadata))
{
    if (metadataPtr_ != nullptr) {
        metadataPtr_->LockWaterMark();
    }
}

Metadata::MetaWaterMarkAutoLock::~MetaWaterMarkAutoLock()
{
    if (metadataPtr_ != nullptr) {
        metadataPtr_->UnlockWaterMark();
    }
}

int Metadata::InitLocalMetaData()
{
    std::lock_guard<std::mutex> autoLock(localMetaDataMutex_);
    auto [errCode, localMetaData] = GetLocalMetaData();
    if (errCode != E_OK) {
        return errCode;
    }
    if (localMetaData.localSchemaVersion != 0) {
        return E_OK;
    }
    uint64_t curTime = 0;
    errCode = OS::GetCurrentSysTimeInMicrosecond(curTime);
    if (errCode != E_OK) {
        LOGW("[Metadata] get system time failed when init schema version!");
        localMetaData.localSchemaVersion += 1;
    } else {
        localMetaData.localSchemaVersion = curTime;
    }
    errCode = SaveLocalMetaData(localMetaData);
    if (errCode != E_OK) {
        LOGE("[Metadata] init local schema version failed:%d", errCode);
    }
    return errCode;
}
}  // namespace DistributedDB