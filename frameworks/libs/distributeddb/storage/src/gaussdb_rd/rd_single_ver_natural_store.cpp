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
#include "rd_single_ver_natural_store.h"

#include "rd_single_ver_natural_store_connection.h"
#include "rd_utils.h"
#include "sqlite_single_ver_storage_engine.h"

namespace DistributedDB {

RdSingleVerNaturalStore::RdSingleVerNaturalStore()
{
    LOGD("RdSingleVerNaturalStore Created");
}

RdSingleVerNaturalStore::~RdSingleVerNaturalStore()
{
}

GenericKvDBConnection *RdSingleVerNaturalStore::NewConnection(int &errCode)
{
    RdSingleVerNaturalStoreConnection *connection = new (std::nothrow) RdSingleVerNaturalStoreConnection(this);
    if (connection == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        return nullptr;
    }
    errCode = E_OK;
    return connection;
}

int RdSingleVerNaturalStore::GetAndInitStorageEngine(const KvDBProperties &kvDBProp)
{
    int errCode = E_OK;
    {
        std::unique_lock<std::shared_mutex> lock(engineMutex_);
        storageEngine_ =
            static_cast<RdSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(kvDBProp, errCode));
        if (storageEngine_ == nullptr) {
            return errCode;
        }
    }

    errCode = InitDatabaseContext(kvDBProp);
    if (errCode != E_OK) {
        LOGE("[RdSinStore][GetAndInitStorageEngine] Init database context fail! errCode = [%d]", errCode);
    }
    return errCode;
}

int RdSingleVerNaturalStore::RegisterNotification()
{
    static const std::vector<int> events {
        static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT),
    };
    for (auto event = events.begin(); event != events.end(); ++event) {
        int errCode = RegisterNotificationEventType(*event);
        if (errCode == E_OK) {
            continue;
        }
        LOGE("Register rd single version event %d failed:%d!", *event, errCode);
        for (auto iter = events.begin(); iter != event; ++iter) {
            UnRegisterNotificationEventType(*iter);
        }
        return errCode;
    }
    notificationEventsRegistered_ = true;
    return E_OK;
}

void RdSingleVerNaturalStore::ReleaseHandle(RdSingleVerStorageExecutor *&handle) const
{
    if (handle == nullptr) {
        return;
    }
    if (storageEngine_ != nullptr) {
        StorageExecutor *databaseHandle = handle;
        storageEngine_->Recycle(databaseHandle);
        handle = nullptr;
    }
    engineMutex_.unlock_shared(); // unlock after handle used up
}

int RdSingleVerNaturalStore::TransObserverTypeToRegisterFunctionType(int observerType, RegisterFuncType &type) const
{
    static constexpr TransPair transMap[] = {
        { static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT),
            RegisterFuncType::OBSERVER_SINGLE_VERSION_NS_PUT_EVENT },
    };
    auto funcType = GetFuncType(observerType, transMap, sizeof(transMap) / sizeof(TransPair));
    if (funcType == RegisterFuncType::REGISTER_FUNC_TYPE_MAX) {
        return -E_NOT_SUPPORT;
    }
    type = funcType;
    return E_OK;
}

int RdSingleVerNaturalStore::Open(const KvDBProperties &kvDBProp)
{
    // Currently, Design for the consistency of directory and file setting secOption
    int errCode = ClearIncompleteDatabase(kvDBProp);
    if (errCode != E_OK) {
        LOGE("Clear incomplete database failed in single version:%d", errCode);
        return errCode;
    }
    const std::string dataDir = kvDBProp.GetStringProp(KvDBProperties::DATA_DIR, "");
    const std::string identifierDir = kvDBProp.GetStringProp(KvDBProperties::IDENTIFIER_DIR, "");
    bool isCreate = kvDBProp.GetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, true);
    errCode = DBCommon::CreateStoreDirectory(dataDir, identifierDir, DBConstant::SINGLE_SUB_DIR, isCreate);
    if (errCode != E_OK) {
        LOGE("Create single version natural store directory failed:%d", errCode);
        return errCode;
    }
    LOGD("[RdSingleVerNaturalStore] Open RdSingleVerNaturalStore");
    errCode = GetAndInitStorageEngine(kvDBProp);
    if (errCode != E_OK) {
        ReleaseResources();
        return errCode;
    }
    errCode = RegisterNotification();
    if (errCode != E_OK) {
        LOGE("register notification failed:%d", errCode);
        ReleaseResources();
        return errCode;
    }
    MyProp() = kvDBProp;
    OnKill([this]() { ReleaseResources(); });
    return errCode;
}

void RdSingleVerNaturalStore::ReleaseResources()
{
    if (notificationEventsRegistered_) {
        UnRegisterNotificationEventType(
            static_cast<EventType>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
        notificationEventsRegistered_ = false;
    }
    {
        std::unique_lock<std::shared_mutex> lock(engineMutex_);
        if (storageEngine_ != nullptr) {
            (void)StorageEngineManager::ReleaseStorageEngine(storageEngine_);
            storageEngine_ = nullptr;
        }
    }
}

// Invoked automatically when connection count is zero
void RdSingleVerNaturalStore::Close()
{
    ReleaseResources();
}

// Get interface type of this kvdb.
int RdSingleVerNaturalStore::GetInterfaceType() const
{
    return -E_NOT_SUPPORT;
}

// Get the interface ref-count, in order to access asynchronously.
void RdSingleVerNaturalStore::IncRefCount()
{
    return;
}

// Drop the interface ref-count.
void RdSingleVerNaturalStore::DecRefCount()
{
    return;
}

// Get the identifier of this kvdb.
std::vector<uint8_t> RdSingleVerNaturalStore::GetIdentifier() const
{
    return {};
}

// Get interface for syncer.
IKvDBSyncInterface *RdSingleVerNaturalStore::GetSyncInterface()
{
    return nullptr;
}

int RdSingleVerNaturalStore::GetMetaData(const Key &key, Value &value) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStore::PutMetaData(const Key &key, const Value &value, bool isInTransaction)
{
    return -E_NOT_SUPPORT;
}

// Delete multiple meta data records in a transaction.
int RdSingleVerNaturalStore::DeleteMetaData(const std::vector<Key> &keys)
{
    return -E_NOT_SUPPORT;
}

// Delete multiple meta data records with key prefix in a transaction.
int RdSingleVerNaturalStore::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStore::GetAllMetaKeys(std::vector<Key> &keys) const
{
    return -E_NOT_SUPPORT;
}

void RdSingleVerNaturalStore::GetMaxTimestamp(Timestamp &stamp) const
{
    return;
}

void RdSingleVerNaturalStore::SetMaxTimestamp(Timestamp timestamp)
{
    return;
}

int RdSingleVerNaturalStore::Rekey(const CipherPassword &passwd)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStore::Export(const std::string &filePath, const CipherPassword &passwd)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStore::Import(const std::string &filePath, const CipherPassword &passwd)
{
    return -E_NOT_SUPPORT;
}

RdSingleVerStorageExecutor *RdSingleVerNaturalStore::GetHandle(bool isWrite, int &errCode,
    OperatePerm perm) const
{
    engineMutex_.lock_shared();
    if (storageEngine_ == nullptr) {
        errCode = -E_INVALID_DB;
        engineMutex_.unlock_shared(); // unlock when get handle failed.
        return nullptr;
    }
    auto handle = storageEngine_->FindExecutor(isWrite, perm, errCode);
    if (handle == nullptr) {
        LOGD("Find null storage engine");
        engineMutex_.unlock_shared(); // unlock when get handle failed.
    }
    return static_cast<RdSingleVerStorageExecutor *>(handle);
}

SchemaObject RdSingleVerNaturalStore::GetSchemaInfo() const
{
    return MyProp().GetSchema();
}

bool RdSingleVerNaturalStore::CheckCompatible(const std::string &schema, uint8_t type) const
{
    return true;
}

Timestamp RdSingleVerNaturalStore::GetCurrentTimestamp()
{
    return 0;
}

SchemaObject RdSingleVerNaturalStore::GetSchemaObject() const
{
    return MyProp().GetSchema();
}

const SchemaObject &RdSingleVerNaturalStore::GetSchemaObjectConstRef() const
{
    return MyProp().GetSchemaConstRef();
}

const KvDBProperties &RdSingleVerNaturalStore::GetDbProperties() const
{
    return GetMyProperties();
}

int RdSingleVerNaturalStore::GetKvDBSize(const KvDBProperties &properties, uint64_t &size) const
{
    return -E_NOT_SUPPORT;
}

KvDBProperties &RdSingleVerNaturalStore::GetDbPropertyForUpdate()
{
    return MyProp();
}

int RdSingleVerNaturalStore::InitDatabaseContext(const KvDBProperties &kvDBProp)
{
    OpenDbProperties option;
    InitDataBaseOption(kvDBProp, option);

    StorageEngineAttr poolSize = {1, 1, 1, 16}; // at most 1 write 16 read.

    storageEngine_->SetNotifiedCallback(
        [&](int eventType, KvDBCommitNotifyFilterAbleData *committedData) {
            auto commitData = static_cast<SingleVerNaturalStoreCommitNotifyData *>(committedData);
            this->CommitAndReleaseNotifyData(commitData, true, eventType);
        }
    );

    std::string identifier = kvDBProp.GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    int errCode = storageEngine_->InitRdStorageEngine(poolSize, option, identifier);
    if (errCode != E_OK) {
        LOGE("Init the rd storage engine failed:%d", errCode);
    }
    return errCode;
}

int RdSingleVerNaturalStore::RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStore::SetAutoLifeCycleTime(uint32_t time)
{
    return -E_NOT_SUPPORT;
}

bool RdSingleVerNaturalStore::IsDataMigrating() const
{
    return false;
}

int RdSingleVerNaturalStore::TriggerToMigrateData() const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStore::CheckReadDataControlled() const
{
    return E_OK;
}

bool RdSingleVerNaturalStore::IsCacheDBMode() const
{
    return false;
}

void RdSingleVerNaturalStore::IncreaseCacheRecordVersion() const
{
    return;
}

uint64_t RdSingleVerNaturalStore::GetCacheRecordVersion() const
{
    return 0;
}

uint64_t RdSingleVerNaturalStore::GetAndIncreaseCacheRecordVersion() const
{
    return 0;
}

int RdSingleVerNaturalStore::CheckIntegrity() const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStore::GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const
{
    return -E_NOT_SUPPORT;
}

void RdSingleVerNaturalStore::SetDataInterceptor(const PushDataInterceptor &interceptor)
{
    return;
}

int RdSingleVerNaturalStore::SetMaxLogSize(uint64_t limit)
{
    return -E_NOT_SUPPORT;
}

uint64_t RdSingleVerNaturalStore::GetMaxLogSize() const
{
    return 0;
}

void RdSingleVerNaturalStore::Dump(int fd)
{
    return;
}

void RdSingleVerNaturalStore::WakeUpSyncer()
{
    LOGD("Not support syncer yet");
    return;
}

void RdSingleVerNaturalStore::CommitNotify(int notifyEvent, KvDBCommitNotifyFilterAbleData *data)
{
    GenericKvDB::CommitNotify(notifyEvent, data);
}

void RdSingleVerNaturalStore::InitDataBaseOption(const KvDBProperties &kvDBProp, OpenDbProperties &option)
{
    option.uri = GetDatabasePath(kvDBProp);
    option.subdir = GetSubDirPath(kvDBProp);
    SecurityOption securityOpt;
    if (RuntimeContext::GetInstance()->IsProcessSystemApiAdapterValid()) {
        securityOpt.securityLabel = kvDBProp.GetSecLabel();
        securityOpt.securityFlag = kvDBProp.GetSecFlag();
    }
    option.createIfNecessary = kvDBProp.GetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, true);
    option.createDirByStoreIdOnly = kvDBProp.GetBoolProp(KvDBProperties::CREATE_DIR_BY_STORE_ID_ONLY, false);
    option.readOnly = kvDBProp.GetBoolProp(KvDBProperties::READ_ONLY_MODE, false);
    option.isNeedIntegrityCheck = kvDBProp.GetBoolProp(KvDBProperties::CHECK_INTEGRITY, false);
    option.isNeedRmCorruptedDb = kvDBProp.GetBoolProp(KvDBProperties::RM_CORRUPTED_DB, false);
    bool isSharedMode = kvDBProp.GetBoolProp(KvDBProperties::SHARED_MODE, false);
    option.isHashTable = (IndexType)kvDBProp.GetIntProp(KvDBProperties::INDEX_TYPE, BTREE) == HASH;
    int pageSize = kvDBProp.GetIntProp(KvDBProperties::PAGE_SIZE, 32);
    int cacheSize = kvDBProp.GetIntProp(KvDBProperties::CACHE_SIZE, 2048);

    std::string config = "{";
    config += InitRdConfig() + R"(, )";
    config += R"("pageSize":)" + std::to_string(pageSize) + R"(, )";
    config += R"("bufferPoolSize":)" + std::to_string(cacheSize) + R"(, )";
    config += R"("redoPubBufSize":)" + std::to_string(cacheSize) + R"(, )";
    config += isSharedMode ? R"("sharedModeEnable": 1)" : R"("sharedModeEnable": 0)";
    config += "}";
    option.rdConfig = config;
}

} // namespace DistributedDB
