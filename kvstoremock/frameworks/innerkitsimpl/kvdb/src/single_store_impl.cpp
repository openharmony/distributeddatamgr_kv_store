/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "SingleStoreImpl"
#include "single_store_impl.h"
#include "dev_manager.h"
#include "log_print.h"
#include "store_result_set.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
SingleStoreImpl::SingleStoreImpl(std::shared_ptr<DBStore> dbStore, const AppId &appId, const Options &options,
    const Convertor &cvt) : convertor_(cvt), dbStore_(std::move(dbStore))
{
    appId_ = appId.appId;
    storeId_ = dbStore_->GetStoreId();
    autoSync_ = options.autoSync;
}

StoreId SingleStoreImpl::GetStoreId() const
{
    return { storeId_ };
}

Status SingleStoreImpl::Put(const Key &key, const Value &value)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = convertor_.ToLocalDBKey(key);
    if (dbKey.empty() || value.Size() > MAX_VALUE_LENGTH) {
        ZLOGE("invalid key:%{public}s size:[k:%{public}zu v:%{public}zu]",
            StoreUtil::Anonymous(key.ToString()).c_str(), key.Size(), value.Size());
        return INVALID_ARGUMENT;
    }

    auto dbStatus = dbStore_->Put(dbKey, value);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x key:%{public}s, value size:%{public}zu", status,
            StoreUtil::Anonymous(key.ToString()).c_str(), value.Size());
    }
    return status;
}

Status SingleStoreImpl::PutBatch(const std::vector<Entry> &entries)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBEntry> dbEntries;
    for (const auto &entry : entries) {
        DBEntry dbEntry;
        dbEntry.key = convertor_.ToLocalDBKey(entry.key);
        if (dbEntry.key.empty() || entry.value.Size() > MAX_VALUE_LENGTH) {
            ZLOGE("invalid key:%{public}s size:[k:%{public}zu v:%{public}zu]",
                StoreUtil::Anonymous(entry.key.ToString()).c_str(), entry.key.Size(), entry.value.Size());
            return INVALID_ARGUMENT;
        }
        dbEntry.value = entry.value;
        dbEntries.push_back(std::move(dbEntry));
    }
    auto dbStatus = dbStore_->PutBatch(dbEntries);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x entries size:%{public}zu", status, entries.size());
    }
    return status;
}

Status SingleStoreImpl::Delete(const Key &key)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = convertor_.ToLocalDBKey(key);
    if (dbKey.empty()) {
        ZLOGE("invalid key:%{public}s size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
        return INVALID_ARGUMENT;
    }

    auto dbStatus = dbStore_->Delete(dbKey);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x key:%{public}s", status, StoreUtil::Anonymous(key.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::DeleteBatch(const std::vector<Key> &keys)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBKey> dbKeys;
    for (const auto &key : keys) {
        DBKey dbKey = convertor_.ToLocalDBKey(key);
        if (dbKey.empty()) {
            ZLOGE("invalid key:%{public}s size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
            return INVALID_ARGUMENT;
        }
        dbKeys.push_back(std::move(dbKey));
    }

    auto dbStatus = dbStore_->DeleteBatch(dbKeys);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x keys size:%{public}zu", status, keys.size());
    }
    return status;
}

Status SingleStoreImpl::StartTransaction()
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->StartTransaction();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, StoreUtil::Anonymous(storeId_).c_str());
    }
    return status;
}

Status SingleStoreImpl::Commit()
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->Commit();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, dbStore_->GetStoreId().c_str());
    }
    return status;
}

Status SingleStoreImpl::Rollback()
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->Rollback();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, StoreUtil::Anonymous(storeId_).c_str());
    }
    return status;
}

Status SingleStoreImpl::SubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer)
{
    return SERVER_UNAVAILABLE;
}

Status SingleStoreImpl::UnSubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer)
{
    return SERVER_UNAVAILABLE;
}

Status SingleStoreImpl::Get(const Key &key, Value &value)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = convertor_.ToWholeDBKey(key);
    if (dbKey.empty()) {
        ZLOGE("invalid key:%{public}s size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
        return INVALID_ARGUMENT;
    }

    DBValue dbValue;
    auto dbStatus = dbStore_->Get(dbKey, dbValue);
    value = std::move(dbValue);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x key:%{public}s", status, StoreUtil::Anonymous(key.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetEntries(const Key &prefix, std::vector<Entry> &entries) const
{
    DBKey dbPrefix = convertor_.GetPrefix(prefix);
    if (dbPrefix.empty() && !prefix.Empty()) {
        ZLOGE("invalid prefix:%{public}s size:%{public}zu", StoreUtil::Anonymous(prefix.ToString()).c_str(),
            prefix.Size());
        return INVALID_ARGUMENT;
    }

    DBQuery dbQuery = DBQuery::Select();
    dbQuery.PrefixKey(dbPrefix);
    auto status = GetEntries(dbQuery, entries);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x prefix:%{public}s", status, StoreUtil::Anonymous(prefix.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetEntries(const DataQuery &query, std::vector<Entry> &entries) const
{
    DBQuery dbQuery = convertor_.GetDBQuery(query);
    auto status = GetEntries(dbQuery, entries);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x query:%{public}s", status, StoreUtil::Anonymous(query.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetResultSet(const Key &prefix, std::shared_ptr<ResultSet> &resultSet) const
{
    DBKey dbPrefix = convertor_.GetPrefix(prefix);
    if (dbPrefix.empty() && !prefix.Empty()) {
        ZLOGE("invalid prefix:%{public}s size:%{public}zu", StoreUtil::Anonymous(prefix.ToString()).c_str(),
            prefix.Size());
        return INVALID_ARGUMENT;
    }

    DBQuery dbQuery = DistributedDB::Query::Select();
    dbQuery.PrefixKey(dbPrefix);
    auto status = GetResultSet(dbQuery, resultSet);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x prefix:%{public}s", status, StoreUtil::Anonymous(prefix.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetResultSet(const DataQuery &query, std::shared_ptr<ResultSet> &resultSet) const
{
    DBQuery dbQuery = convertor_.GetDBQuery(query);
    auto status = GetResultSet(dbQuery, resultSet);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x query:%{public}s", status, StoreUtil::Anonymous(query.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::CloseResultSet(std::shared_ptr<ResultSet> &resultSet)
{
    if (resultSet == nullptr) {
        ZLOGE("input is nullptr");
        return INVALID_ARGUMENT;
    }

    auto status = resultSet->Close();
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, StoreUtil::Anonymous(storeId_).c_str());
    }
    resultSet = nullptr;
    return status;
}

Status SingleStoreImpl::GetCount(const DataQuery &query, int &result) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBQuery dbQuery = convertor_.GetDBQuery(query);
    auto dbStatus = dbStore_->GetCount(dbQuery, result);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x query:%{public}s", status, StoreUtil::Anonymous(query.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetSecurityLevel(SecurityLevel &secLevel) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DistributedDB::SecurityOption option;
    auto dbStatus = dbStore_->GetSecurityOption(option);
    secLevel = static_cast<SecurityLevel>(StoreUtil::GetSecLevel(option));
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x security:[%{public}d]", status, option.securityLabel);
    }
    return status;
}

Status SingleStoreImpl::RemoveDeviceData(const std::string &device)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->RemoveDeviceData(DevManager::GetInstance().ToUUID(device));
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x device:%{public}s", status, StoreUtil::Anonymous(device).c_str());
    }
    return status;
}


int32_t SingleStoreImpl::AddRef()
{
    ref_++;
    return ref_;
}

int32_t SingleStoreImpl::Close(bool isForce)
{
    if (isForce) {
        ref_ = 1;
    }
    ref_--;
    if (ref_ != 0) {
        return ref_;
    }

    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    dbStore_ = nullptr;
    return ref_;
}

Status SingleStoreImpl::Backup(const std::string &file, const std::string &baseDir)
{
    return SERVER_UNAVAILABLE;
}

Status SingleStoreImpl::Restore(const std::string &file, const std::string &baseDir)
{
    return SERVER_UNAVAILABLE;
}

Status SingleStoreImpl::DeleteBackup(const std::vector<std::string> &files, const std::string &baseDir,
    std::map<std::string, DistributedKv::Status> &results)
{
    return SERVER_UNAVAILABLE;
}

Status SingleStoreImpl::GetResultSet(const DBQuery &query, std::shared_ptr<ResultSet> &resultSet) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DistributedDB::KvStoreResultSet *dbResultSet = nullptr;
    auto status = dbStore_->GetEntries(query, dbResultSet);
    if (dbResultSet == nullptr) {
        return StoreUtil::ConvertStatus(status);
    }
    resultSet = std::make_shared<StoreResultSet>(dbResultSet, dbStore_, convertor_);
    return SUCCESS;
}

Status SingleStoreImpl::GetEntries(const DBQuery &query, std::vector<Entry> &entries) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBEntry> dbEntries;
    std::string deviceId;
    auto dbStatus = dbStore_->GetEntries(query, dbEntries);
    entries.resize(dbEntries.size());
    auto it = entries.begin();
    for (auto &dbEntry : dbEntries) {
        auto &entry = *it;
        entry.key = convertor_.ToKey(std::move(dbEntry.key), deviceId);
        entry.value = std::move(dbEntry.value);
        ++it;
    }

    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status == NOT_FOUND) {
        status = SUCCESS;
    }
    return status;
}
Status SingleStoreImpl::Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t delay)
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
                             std::shared_ptr<SyncCallback> syncCallback)
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::RegisterSyncCallback(std::shared_ptr<SyncCallback> callback)
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::UnRegisterSyncCallback()
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::SetSyncParam(const KvSyncParam &syncParam)
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::GetSyncParam(KvSyncParam &syncParam)
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::SetCapabilityEnabled(bool enabled) const
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::SetCapabilityRange(const std::vector<std::string> &local,
                                           const std::vector<std::string> &remote) const
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query)
{
    return SERVER_UNAVAILABLE;
}
Status SingleStoreImpl::UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query)
{
    return SERVER_UNAVAILABLE;
}
} // namespace OHOS::DistributedKv