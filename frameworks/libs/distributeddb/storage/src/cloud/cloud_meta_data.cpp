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
#include "db_errno.h"
#include "parcel.h"

namespace DistributedDB {

CloudMetaData::CloudMetaData(ICloudSyncStorageInterface *store)
    : store_(store)
{
}

Key CloudMetaData::GetPrefixTableName(const TableName &tableName)
{
    TableName newName = CloudDbConstant::CLOUD_META_TABLE_PREFIX + tableName;
    Key prefixedTableName(newName.begin(), newName.end());
    return prefixedTableName;
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
    std::string cloudMark = "";
    auto iter = cloudMetaVals_.find(tableName);
    if (iter != cloudMetaVals_.end()) {
        cloudMark = iter->second.cloudMark;
    }
    int ret = WriteMarkToMeta(tableName, localMark, cloudMark);
    if (ret != E_OK) {
        return ret;
    }
    if (iter == cloudMetaVals_.end()) {
        CloudMetaValue cloudMetaVal = { .localMark = localMark, .cloudMark = cloudMark };
        cloudMetaVals_[tableName] = cloudMetaVal;
    } else {
        iter->second.localMark = localMark;
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
        CloudMetaValue cloudMetaVal = { .localMark = localMark, .cloudMark = cloudMark };
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
    int ret = store_->GetMetaData(GetPrefixTableName(tableName), blobMetaVal);
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
    int ret = SerializeMark(localmark, cloudMark, blobMetaVal);
    if (ret != E_OK) {
        return ret;
    }
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->PutMetaData(GetPrefixTableName(tableName), blobMetaVal);
}

int CloudMetaData::SerializeMark(Timestamp localMark, std::string &cloudMark, Value &blobMeta)
{
    uint64_t length = Parcel::GetUInt64Len() + Parcel::GetStringLen(cloudMark);
    blobMeta.resize(length);
    Parcel parcel(blobMeta.data(), blobMeta.size());
    parcel.WriteUInt64(localMark);
    parcel.WriteString(cloudMark);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error while serializing cloud meta data.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int CloudMetaData::DeserializeMark(Value &blobMark, CloudMetaValue &cloudMetaValue)
{
    if (blobMark.empty()) {
        cloudMetaValue.localMark = 0;
        cloudMetaValue.cloudMark = "";
        return E_OK;
    }
    Parcel parcel(blobMark.data(), blobMark.size());
    parcel.ReadUInt64(cloudMetaValue.localMark);
    parcel.ReadString(cloudMetaValue.cloudMark);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error while deserializing cloud meta data.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}
} // namespace DistributedDB