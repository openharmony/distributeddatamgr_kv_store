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

#include "cloud/cloud_meta_data.h"
#include "cloud/cloud_db_constant.h"
#include "db_common.h"
#include "db_errno.h"
#include "parcel.h"

namespace DistributedDB {

CloudMetaData::CloudMetaData(ICloudSyncStorageInterface *store)
    : store_(store)
{
}

int CloudMetaData::GetLocalWaterMark(const TableName &tableName, Timestamp &localMark)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    if (cloudMetaVals_.count(tableName) == 0) {
        int ret = ReadMarkFromMeta(tableName);
        if (ret != E_OK) {
            return ret;
        }
    }
    localMark = cloudMetaVals_[tableName].localMark;
    return E_OK;
}

int CloudMetaData::GetLocalWaterMarkByType(const TableName &tableName, CloudWaterType type, Timestamp &localMark)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    if (cloudMetaVals_.count(tableName) == 0) {
        int ret = ReadMarkFromMeta(tableName);
        if (ret != E_OK) {
            return ret;
        }
    }
    if (type == CloudWaterType::INSERT) {
        localMark = cloudMetaVals_[tableName].insertLocalMark;
    } else if (type == CloudWaterType::UPDATE) {
        localMark = cloudMetaVals_[tableName].updateLocalMark;
    } else if (type == CloudWaterType::DELETE) {
        localMark = cloudMetaVals_[tableName].deleteLocalMark;
    }
    cloudMetaVals_[tableName].localMark = std::max(localMark, cloudMetaVals_[tableName].localMark);
    return E_OK;
}

int CloudMetaData::GetCloudWaterMark(const TableName &tableName, std::string &cloudMark)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    if (cloudMetaVals_.count(tableName) == 0) {
        int ret = ReadMarkFromMeta(tableName);
        if (ret != E_OK) {
            return ret;
        }
    }
    cloudMark = cloudMetaVals_[tableName].cloudMark;
    LOGD("[Meta] get cloud water mark=%s", cloudMark.c_str());
    return E_OK;
}

int CloudMetaData::SetLocalWaterMark(const TableName &tableName, Timestamp localMark)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    std::string cloudMark;
    auto iter = cloudMetaVals_.find(tableName);
    if (iter != cloudMetaVals_.end()) {
        cloudMark = iter->second.cloudMark;
    }
    int ret = WriteMarkToMeta(tableName, localMark, cloudMark);
    if (ret != E_OK) {
        return ret;
    }
    if (iter == cloudMetaVals_.end()) {
        CloudMetaValue cloudMetaVal;
        cloudMetaVal.localMark = localMark;
        cloudMetaVal.cloudMark = cloudMark;
        cloudMetaVals_[tableName] = cloudMetaVal;
    } else {
        iter->second.localMark = localMark;
    }
    return E_OK;
}

int CloudMetaData::SetLocalWaterMarkByType(const TableName &tableName, CloudWaterType type, Timestamp localMark)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    CloudMetaValue cloudMetaVal;
    auto iter = cloudMetaVals_.find(tableName);
    if (iter != cloudMetaVals_.end()) {
        cloudMetaVal = iter->second;
    }
    cloudMetaVal.localMark = std::max(localMark, cloudMetaVal.localMark);
    if (type == CloudWaterType::DELETE) {
        cloudMetaVal.deleteLocalMark = localMark;
    } else if (type == CloudWaterType::UPDATE) {
        cloudMetaVal.updateLocalMark = localMark;
        cloudMetaVal.deleteLocalMark = std::max(cloudMetaVal.deleteLocalMark, localMark);
    } else if (type == CloudWaterType::INSERT) {
        cloudMetaVal.insertLocalMark = localMark;
        cloudMetaVal.deleteLocalMark = std::max(cloudMetaVal.deleteLocalMark, localMark);
        cloudMetaVal.updateLocalMark = std::max(cloudMetaVal.updateLocalMark, localMark);
    }
    int ret = WriteTypeMarkToMeta(tableName, cloudMetaVal);
    if (ret != E_OK) {
        return ret;
    }
    if (iter == cloudMetaVals_.end()) {
        cloudMetaVals_[tableName] = cloudMetaVal;
    } else {
        iter->second = cloudMetaVal;
    }
    return E_OK;
}

int CloudMetaData::SetCloudWaterMark(const TableName &tableName, std::string &cloudMark)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    Timestamp localMark = 0;
    auto iter = cloudMetaVals_.find(tableName);
    if (iter != cloudMetaVals_.end()) {
        localMark = iter->second.localMark;
    }
    int ret = WriteMarkToMeta(tableName, localMark, cloudMark);
    if (ret != E_OK) {
        return ret;
    }
    if (iter == cloudMetaVals_.end()) {
        CloudMetaValue cloudMetaVal;
        cloudMetaVal.localMark = localMark;
        cloudMetaVal.cloudMark = cloudMark;
        cloudMetaVals_[tableName] = cloudMetaVal;
    } else {
        iter->second.cloudMark = cloudMark;
    }
    LOGD("[Meta] set cloud water mark=%s", cloudMark.c_str());
    return E_OK;
}

int CloudMetaData::ReadMarkFromMeta(const TableName &tableName)
{
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    Value blobMetaVal;
    int ret = store_->GetMetaData(DBCommon::GetPrefixTableName(tableName), blobMetaVal);
    if (ret != -E_NOT_FOUND && ret != E_OK) {
        return ret;
    }
    CloudMetaValue cloudMetaValue;
    ret = DeserializeMark(blobMetaVal, cloudMetaValue);
    if (ret != E_OK) {
        return ret;
    }
    cloudMetaVals_[tableName] = cloudMetaValue;
    return E_OK;
}

int CloudMetaData::WriteMarkToMeta(const TableName &tableName, Timestamp localmark, std::string &cloudMark)
{
    Value blobMetaVal;
    int ret = DBCommon::SerializeWaterMark(localmark, cloudMark, blobMetaVal);
    if (ret != E_OK) {
        return ret;
    }
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->PutMetaData(DBCommon::GetPrefixTableName(tableName), blobMetaVal);
}

int CloudMetaData::WriteTypeMarkToMeta(const TableName &tableName, CloudMetaValue &cloudMetaValue)
{
    Value blobMetaVal;
    int ret = SerializeWaterMark(cloudMetaValue, blobMetaVal);
    if (ret != E_OK) {
        return ret;
    }
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->PutMetaData(DBCommon::GetPrefixTableName(tableName), blobMetaVal);
}

uint64_t CloudMetaData::GetParcelCurrentLength(CloudMetaValue &cloudMetaValue)
{
    return Parcel::GetUInt64Len() + Parcel::GetStringLen(cloudMetaValue.cloudMark) + Parcel::GetUInt64Len() +
           Parcel::GetUInt64Len() + Parcel::GetUInt64Len();
}

int CloudMetaData::SerializeWaterMark(CloudMetaValue &cloudMetaValue, Value &blobMetaVal)
{
    uint64_t length = GetParcelCurrentLength(cloudMetaValue);
    blobMetaVal.resize(length);
    Parcel parcel(blobMetaVal.data(), blobMetaVal.size());
    parcel.WriteUInt64(cloudMetaValue.localMark);
    parcel.WriteString(cloudMetaValue.cloudMark);
    parcel.WriteUInt64(cloudMetaValue.insertLocalMark);
    parcel.WriteUInt64(cloudMetaValue.updateLocalMark);
    parcel.WriteUInt64(cloudMetaValue.deleteLocalMark);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error while deserializing cloud meta data.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int CloudMetaData::DeserializeMark(Value &blobMark, CloudMetaValue &cloudMetaValue)
{
    if (blobMark.empty()) {
        cloudMetaValue.localMark = 0;
        cloudMetaValue.insertLocalMark = 0;
        cloudMetaValue.updateLocalMark = 0;
        cloudMetaValue.deleteLocalMark = 0;
        cloudMetaValue.cloudMark = "";
        return E_OK;
    }
    Parcel parcel(blobMark.data(), blobMark.size());
    parcel.ReadUInt64(cloudMetaValue.localMark);
    parcel.ReadString(cloudMetaValue.cloudMark);
    if (parcel.IsContinueRead()) {
        parcel.ReadUInt64(cloudMetaValue.insertLocalMark);
        parcel.ReadUInt64(cloudMetaValue.updateLocalMark);
        parcel.ReadUInt64(cloudMetaValue.deleteLocalMark);
    }
    if (blobMark.size() < GetParcelCurrentLength(cloudMetaValue)) {
        cloudMetaValue.insertLocalMark = cloudMetaValue.localMark;
        cloudMetaValue.updateLocalMark = cloudMetaValue.localMark;
        cloudMetaValue.deleteLocalMark = cloudMetaValue.localMark;
    }
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error while deserializing cloud meta data.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int CloudMetaData::CleanWaterMark(const TableName &tableName)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    std::string cloudWaterMark;
    int ret = WriteMarkToMeta(tableName, 0, cloudWaterMark);
    if (ret != E_OK) {
        return ret;
    }
    cloudMetaVals_[tableName] = {};
    LOGD("[Meta] clean cloud water mark");
    return E_OK;
}

void CloudMetaData::CleanAllWaterMark()
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    cloudMetaVals_.clear();
    LOGD("[Meta] clean cloud water mark");
}

void CloudMetaData::CleanWaterMarkInMemory(const TableName &tableName)
{
    std::lock_guard<std::mutex> lock(cloudMetaMutex_);
    cloudMetaVals_[tableName] = {};
    LOGD("[Meta] clean cloud water mark in memory");
}
} // namespace DistributedDB