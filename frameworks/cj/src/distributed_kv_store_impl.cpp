/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdlib>
#include <string>
#include <variant>
#include <vector>
#include <map>
#include <iomanip>
#include "securec.h"
#include "ffi_remote_data.h"

#include "distributed_kv_store_impl.h"
#include "distributed_kv_store_utils.h"

using namespace OHOS::FFI;

namespace OHOS::DistributedKVStore {

static int32_t ConvertCJErrCode(Status status)
{
    switch (status) {
        case PERMISSION_DENIED:
            // 202
            return CJ_ERROR_PERMISSION_DENIED;
        case INVALID_ARGUMENT:
            // 401
            return CJ_ERROR_INVALID_ARGUMENT;
        case OVER_MAX_LIMITS:
            // 15100001
            return CJ_ERROR_OVER_MAX_LIMITS;
        case STORE_META_CHANGED:
        case SECURITY_LEVEL_ERROR:
            // 15100002
            return CJ_ERROR_STORE_META_CHANGED;
        case CRYPT_ERROR:
            // 15100003
            return CJ_ERROR_CRYPT_ERROR;
        case NOT_FOUND:
        case DB_ERROR:
            // 15100004
            return CJ_ERROR_NOT_FOUND;
        case ALREADY_CLOSED:
            // 15100005
            return CJ_ERROR_ALREADY_CLOSED;
        default:
            return static_cast<int32_t>(status);
    }
}

static CArrByte VectorToByteArray(std::vector<uint8_t> bytes)
{
    uint8_t* head = static_cast<uint8_t*>(malloc(bytes.size() * sizeof(uint8_t)));
    if (head == nullptr) {
        return CArrByte{};
    }
    for (unsigned long i = 0; i < bytes.size(); i++) {
        head[i] = bytes[i];
    }
    CArrByte byteArray = { head, bytes.size() };
    return byteArray;
}

static ValueType KVValueToValueType(const DistributedKv::Blob& blob)
{
    auto& data = blob.Data();
    ValueType v = { 0 };
    // number 2 means: valid Blob must have more than 2 bytes.
    if (data.size() < 1) {
        LOGI("Blob have no data!");
        return {0};
    }
    // number 1 means: skip the first byte, byte[0] is real data type.
    std::vector<uint8_t> real(data.begin() + 1, data.end());
    if (data[0] == STRING) {
        v.string = MallocCString(std::string(real.begin(), real.end()));
        v.tag = STRING;
    } else if (data[0] == INTEGER) {
        uint32_t tmp4int = be32toh(*reinterpret_cast<uint32_t*>(&(real[0])));
        v.integer = *reinterpret_cast<int32_t*>(&tmp4int);
        v.tag = INTEGER;
    } else if (data[0] == FLOAT) {
        uint32_t tmp4flt = be32toh(*reinterpret_cast<uint32_t*>(&(real[0])));
        v.flo = *reinterpret_cast<float*>((void*)(&tmp4flt));
        v.tag = FLOAT;
    } else if (data[0] == BYTE_ARRAY) {
        v.byteArray = VectorToByteArray(std::vector<uint8_t>(real.begin(), real.end()));
        v.tag = BYTE_ARRAY;
    } else if (data[0] == BOOLEAN) {
        v.boolean = static_cast<bool>(real[0]);
        v.tag = BOOLEAN;
    } else if (data[0] == DOUBLE) {
        uint64_t tmp4dbl = be64toh(*reinterpret_cast<uint64_t*>(&(real[0])));
        v.dou = *reinterpret_cast<double*>((void*)(&tmp4dbl));
        v.tag = DOUBLE;
    } else {
        // for schema-db, if (data[0] == STRING), no beginning byte!
        v.string = MallocCString(std::string(data.begin(), data.end()));
        v.tag = STRING;
    }
    return v;
}

static void PushData(const ValueType &value, std::vector<uint8_t> &data, uint8_t tag)
{
    switch (tag) {
        case INTEGER: {
            int32_t tmp = value.integer; // copy value, and make it available in stack space.
            uint32_t tmp32 = htobe32(*reinterpret_cast<uint32_t*>(&tmp));
            uint8_t *res = reinterpret_cast<uint8_t*>(&tmp32);
            data.push_back(INTEGER);
            data.insert(data.end(), res, res + sizeof(int32_t) / sizeof(uint8_t));
            break;
        }
        case FLOAT: {
            float tmp = value.flo; // copy value, and make it available in stack space.
            uint32_t tmp32 = htobe32(*reinterpret_cast<uint32_t*>(&tmp));
            uint8_t *res = reinterpret_cast<uint8_t*>(&tmp32);
            data.push_back(FLOAT);
            data.insert(data.end(), res, res + sizeof(float) / sizeof(uint8_t));
            break;
        }
        case DOUBLE: {
            double tmp = value.dou; // copy value, and make it available in stack space.
            uint64_t tmp64 = htobe64(*reinterpret_cast<uint64_t*>(&tmp));
            uint8_t *res = reinterpret_cast<uint8_t*>(&tmp64);
            data.push_back(DOUBLE);
            data.insert(data.end(), res, res + sizeof(double) / sizeof(uint8_t));
            break;
        }
        default:
            break;
    }
}

static DistributedKv::Value ValueTypeToKVValue(const ValueType &value)
{
    std::vector<uint8_t> data;
    switch (value.tag) {
        case STRING: {
            std::string str = value.string;
            data.push_back(STRING);
            data.insert(data.end(), str.begin(), str.end());
            break;
        }
        case INTEGER: {
            PushData(value, data, value.tag);
            break;
        }
        case FLOAT: {
            PushData(value, data, value.tag);
            break;
        }
        case BYTE_ARRAY: {
            std::vector<uint8_t> bytes = std::vector<uint8_t>();
            for (int64_t i = 0; i < value.byteArray.size; i++) {
                bytes.push_back(value.byteArray.head[i]);
            }
            data.push_back(BYTE_ARRAY);
            data.insert(data.end(), bytes.begin(), bytes.end());
            break;
        }
        case BOOLEAN: {
            data.push_back(BOOLEAN);
            data.push_back(static_cast<uint8_t>(value.boolean));
            break;
        }
        case DOUBLE: {
            PushData(value, data, value.tag);
            break;
        }
        default:
            break;
    }
    return DistributedKv::Blob(data);
}

CJKVManager::CJKVManager() {};
CJKVManager::CJKVManager(const char* boudleName, OHOS::AbilityRuntime::Context* context)
{
    ContextParam param;
    param.area = context->GetArea();
    param.baseDir = context->GetDatabaseDir();
    auto hapInfo = context->GetHapModuleInfo();
    if (hapInfo != nullptr) {
        param.hapName = hapInfo->moduleName;
    }
    param_ = std::make_shared<ContextParam>(std::move(param));
    bundleName_ = boudleName;
}

uint64_t CJKVManager::GetKVStore(const char* cStoreId, const CJOptions cjOptions, int32_t& errCode)
{
    Options options;
    options.createIfMissing = cjOptions.createIfMissing;
    options.encrypt = cjOptions.encrypt;
    options.backup = cjOptions.backup;
    options.autoSync = cjOptions.autoSync;
    options.kvStoreType = static_cast<KvStoreType>(cjOptions.kvStoreType);
    options.securityLevel = cjOptions.securityLevel;
    AppId appId = { bundleName_ };
    std::string sStoreId = cStoreId;
    StoreId storeId = { sStoreId };
    options.baseDir = param_->baseDir;
    options.area = param_->area + 1;
    options.hapName = param_->hapName;
    std::shared_ptr<DistributedKv::SingleKvStore> kvStore;
    Status status = kvDataManager_.GetSingleKvStore(options, appId, storeId, kvStore);
    if (status == CRYPT_ERROR) {
        options.rebuild = true;
        status = kvDataManager_.GetSingleKvStore(options, appId, storeId, kvStore);
        LOGE("Data has corrupted, rebuild db");
    }
    errCode = ConvertCJErrCode(status);
    if (errCode != 0) {
        return 0;
    }
    auto nativeKVStore = FFIData::Create<CJSingleKVStore>(sStoreId);
    nativeKVStore->SetKvStorePtr(kvStore);
    nativeKVStore->SetContextParam(param_);
    return nativeKVStore->GetID();
}

int32_t CJKVManager::CloseKVStore(const char* appId, const char* storeId)
{
    std::string sAppId = appId;
    std::string sStoreId = storeId;
    AppId appIdBox = { sAppId };
    StoreId storeIdBox { sStoreId };
    Status status = kvDataManager_.CloseKvStore(appIdBox, storeIdBox);
    if ((status == Status::SUCCESS) || (status == Status::STORE_NOT_FOUND) || (status == Status::STORE_NOT_OPEN)) {
        status = Status::SUCCESS;
    }
    return ConvertCJErrCode(status);
}

int32_t CJKVManager::DeleteKVStore(const char* appId, const char* storeId)
{
    std::string sAppId = appId;
    std::string sStoreId = storeId;
    AppId appIdBox = { sAppId };
    StoreId storeIdBox { sStoreId };
    std::string databaseDir = param_->baseDir;
    Status status = kvDataManager_.DeleteKvStore(appIdBox, storeIdBox, databaseDir);
    return ConvertCJErrCode(status);
}

static CArrStr VectorAppIdToCArr(const std::vector<StoreId> storeIdList)
{
    CArrStr strArray;
    strArray.size = storeIdList.size();
    strArray.head = static_cast<char**>(malloc(strArray.size * sizeof(char*)));
    if (strArray.head == nullptr) {
        return CArrStr{0};
    }
    for (int64_t i = 0; i < strArray.size; i++) {
        strArray.head[i] = MallocCString(storeIdList[i].storeId);
    }
    return strArray;
}

CArrStr CJKVManager::GetAllKVStoreId(const char* appId, int32_t& errCode)
{
    std::string sAppId = appId;
    AppId appIdBox = { sAppId };
    std::vector<StoreId> storeIdList;
    Status status = kvDataManager_.GetAllKvStoreId(appIdBox, storeIdList);
    errCode = ConvertCJErrCode(status);
    return VectorAppIdToCArr(storeIdList);
}

CJSingleKVStore::CJSingleKVStore(const std::string& storeId)
{
    storeId_ = storeId;
}

std::shared_ptr<SingleKvStore> CJSingleKVStore::GetKvStorePtr()
{
    return kvStore_;
}

void CJSingleKVStore::SetKvStorePtr(std::shared_ptr<SingleKvStore> kvStore)
{
    kvStore_ = kvStore;
}

void CJSingleKVStore::SetContextParam(std::shared_ptr<ContextParam> param)
{
    param_ = param;
}

int32_t CJSingleKVStore::Put(const std::string &key, const ValueType &value)
{
    auto tempKey = DistributedKv::Key(key);
    Status status = kvStore_->Put(tempKey, ValueTypeToKVValue(value));
    return ConvertCJErrCode(status);
}

static Entry CEntryToEntry(const CEntry &cEntry)
{
    std::string key = cEntry.key;
    Entry entry = {DistributedKv::Key(key), ValueTypeToKVValue(cEntry.value)};
    return entry;
}

static std::vector<Entry> CArrayEntryToEntries(const CArrEntry &cArrEntry)
{
    std::vector<Entry> entrys;
    int64_t arrSize = cArrEntry.size;

    for (int64_t i = 0; i < arrSize; i++) {
        Entry entry = CEntryToEntry(cArrEntry.head[i]);
        entrys.push_back(entry);
    }
    return entrys;
}

int32_t CJSingleKVStore::PutBatch(const CArrEntry &cArrEntry)
{
    Status status = kvStore_->PutBatch(CArrayEntryToEntries(cArrEntry));
    return ConvertCJErrCode(status);
}

int32_t CJSingleKVStore::Delete(const std::string &key)
{
    auto tempKey = DistributedKv::Key(key);
    Status status = kvStore_->Delete(tempKey);
    return ConvertCJErrCode(status);
}

static std::vector<Key> CArrStrToVectorKey(const CArrStr &cArrStr)
{
    std::vector<Key> keys;
    int64_t size = cArrStr.size;
    for (int64_t i = 0; i < size; i++) {
        std::string str = cArrStr.head[i];
        keys.push_back(DistributedKv::Key(str));
    }
    return keys;
}

int32_t CJSingleKVStore::DeleteBatch(const CArrStr &cArrStr)
{
    Status status = kvStore_->DeleteBatch(CArrStrToVectorKey(cArrStr));
    return ConvertCJErrCode(status);
}

ValueType CJSingleKVStore::Get(const std::string &key, int32_t& errCode)
{
    auto s_key = DistributedKv::Key(key);
    OHOS::DistributedKv::Value value;
    Status status = kvStore_->Get(key, value);
    errCode = ConvertCJErrCode(status);
    return KVValueToValueType(value);
}

int32_t CJSingleKVStore::Backup(const std::string &file)
{
    Status status = kvStore_->Backup(file, param_->baseDir);
    return ConvertCJErrCode(status);
}

int32_t CJSingleKVStore::Restore(const std::string &file)
{
    Status status = kvStore_->Restore(file, param_->baseDir);
    return ConvertCJErrCode(status);
}

int32_t CJSingleKVStore::StartTransaction()
{
    Status status = kvStore_->StartTransaction();
    return ConvertCJErrCode(status);
}

int32_t CJSingleKVStore::Commit()
{
    Status status = kvStore_->Commit();
    return ConvertCJErrCode(status);
}

int32_t CJSingleKVStore::Rollback()
{
    Status status = kvStore_->Rollback();
    return ConvertCJErrCode(status);
}

int32_t CJSingleKVStore::EnableSync(bool enabled)
{
    Status status = kvStore_->SetCapabilityEnabled(enabled);
    return ConvertCJErrCode(status);
}

int32_t CJSingleKVStore::SetSyncParam(uint32_t defaultAllowedDelayMs)
{
    KvSyncParam syncParam { defaultAllowedDelayMs };
    Status status = kvStore_->SetSyncParam(syncParam);
    return ConvertCJErrCode(status);
}

constexpr int DEVICEID_WIDTH = 4;

static std::string GetDeviceKey(const std::string& deviceId, const std::string& key)
{
    std::ostringstream oss;
    if (!deviceId.empty()) {
        oss << std::setfill('0') << std::setw(DEVICEID_WIDTH) << deviceId.length() << deviceId;
    }
    oss << key;
    return oss.str();
}

CJDeviceKVStore::CJDeviceKVStore(const std::string& storeId)
    : CJSingleKVStore(storeId)
{
}

ValueType CJDeviceKVStore::Get(const std::string &deviceId, const std::string &key, int32_t& errCode)
{
    std::string deviceKey = GetDeviceKey(deviceId, key);
    auto s_key = DistributedKv::Key(key);
    OHOS::DistributedKv::Value value;
    Status status = GetKvStorePtr()->Get(key, value);
    errCode = ConvertCJErrCode(status);
    return KVValueToValueType(value);
}

CArrEntry CJDeviceKVStore::GetEntriesByDataQuery(DistributedKVStore::DataQuery dataQuery, int32_t& errCode)
{
    std::vector<DistributedKVStore::Entry> entries;
    Status status = GetKvStorePtr()->GetEntries(dataQuery, entries);
    errCode = ConvertCJErrCode(status);
    CEntry *cEntries = static_cast<CEntry*>(malloc(entries.size() * sizeof(CEntry)));
    if (cEntries == nullptr) {
        errCode = -1;
        return CArrEntry{};
    }
    for (size_t i = 0; i < entries.size(); i++) {
        cEntries[i].key = MallocCString(entries[i].key.ToString());
        cEntries[i].value = KVValueToValueType(entries[i].value);
    }
    return CArrEntry{.head = cEntries, .size = int64_t(entries.size())};
}

CArrEntry CJDeviceKVStore::GetEntries(const std::string &deviceId, const std::string &keyPrefix, int32_t& errCode)
{
    DistributedKVStore::DataQuery dataQuery;
    dataQuery.KeyPrefix(keyPrefix);
    dataQuery.DeviceId(deviceId);

    return GetEntriesByDataQuery(dataQuery, errCode);
}

CArrEntry CJDeviceKVStore::GetEntries(const std::string &deviceId, OHOS::sptr<CQuery> query, int32_t& errCode)
{
    DistributedKVStore::DataQuery dataQuery = query->GetDataQuery();
    dataQuery.DeviceId(deviceId);

    return GetEntriesByDataQuery(dataQuery, errCode);
}

int64_t CJDeviceKVStore::GetResultSet(const std::string &deviceId, const std::string &keyPrefix, int32_t& errCode)
{
    DistributedKVStore::DataQuery dataQuery;
    dataQuery.KeyPrefix(keyPrefix);
    dataQuery.DeviceId(deviceId);

    std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
    Status status = GetKvStorePtr()->GetResultSet(dataQuery, kvResultSet);
    errCode = ConvertCJErrCode(status);
    auto nativeCKvStoreResultSet = FFIData::Create<OHOS::DistributedKVStore::CKvStoreResultSet>(kvResultSet);
    return nativeCKvStoreResultSet->GetID();
}

int64_t CJDeviceKVStore::GetResultSetQuery(const std::string &deviceId, OHOS::sptr<CQuery> query, int32_t& errCode)
{
    DistributedKVStore::DataQuery dataQuery = query->GetDataQuery();
    dataQuery.DeviceId(deviceId);

    std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
    Status status = GetKvStorePtr()->GetResultSet(dataQuery, kvResultSet);
    errCode = ConvertCJErrCode(status);
    auto nativeCKvStoreResultSet = FFIData::Create<OHOS::DistributedKVStore::CKvStoreResultSet>(kvResultSet);
    return nativeCKvStoreResultSet->GetID();
}

int32_t CJDeviceKVStore::GetResultSize(const std::string &deviceId, OHOS::sptr<CQuery> query, int32_t& errCode)
{
    DistributedKVStore::DataQuery dataQuery = query->GetDataQuery();
    dataQuery.DeviceId(deviceId);

    int32_t resultSize = 0;
    Status status = GetKvStorePtr()->GetCount(dataQuery, resultSize);
    errCode = ConvertCJErrCode(status);
    return resultSize;
}

CKvStoreResultSet::CKvStoreResultSet(std::shared_ptr<DistributedKv::KvStoreResultSet> cKvResultSet)
{
    kvResultSet = cKvResultSet;
}

std::shared_ptr<DistributedKv::KvStoreResultSet> CKvStoreResultSet::GetKvStoreResultSet()
{
    return kvResultSet;
}

int32_t CKvStoreResultSet::GetCount()
{
    return kvResultSet->GetCount();
}

const DistributedKv::DataQuery& CQuery::GetDataQuery() const
{
    return query_;
}

void CQuery::Reset()
{
    query_.Reset();
}

void CQuery::EqualTo(const std::string &field, ValueType &value)
{
    switch (value.tag) {
        case STRING: {
            query_.EqualTo(field, value.string);
            break;
        }
        case INTEGER: {
            query_.EqualTo(field, value.integer);
            break;
        }
        case FLOAT: {
            query_.EqualTo(field, value.flo);
            break;
        }
        case BOOLEAN: {
            query_.EqualTo(field, value.boolean);
            break;
        }
        case DOUBLE: {
            query_.EqualTo(field, value.dou);
            break;
        }
        default: {
            break;
        }
    }
}
}
