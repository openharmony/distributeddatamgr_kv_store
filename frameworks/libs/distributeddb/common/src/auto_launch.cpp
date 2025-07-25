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

#include "auto_launch.h"

#include "cloud/cloud_db_constant.h"
#include "concurrent_adapter.h"
#include "db_common.h"
#include "db_dump_helper.h"
#include "db_dfx_adapter.h"
#include "db_errno.h"
#include "kv_store_changed_data_impl.h"
#include "kv_store_nb_conflict_data_impl.h"
#include "kvdb_manager.h"
#include "kvdb_pragma.h"
#include "log_print.h"
#include "param_check_utils.h"
#include "relational_store_instance.h"
#include "relational_store_changed_data_impl.h"
#include "res_finalizer.h"
#include "runtime_context.h"
#include "semaphore_utils.h"
#include "sync_able_kvdb_connection.h"

namespace DistributedDB {
namespace {
    constexpr int MAX_AUTO_LAUNCH_ITEM_NUM = 8;
}

void AutoLaunch::SetCommunicatorAggregator(ICommunicatorAggregator *aggregator)
{
    LOGI("[AutoLaunch] SetCommunicatorAggregator");
    std::lock_guard<std::mutex> autoLock(communicatorLock_);
    int errCode;
    if (communicatorAggregator_ != nullptr) {
        LOGI("[AutoLaunch] SetCommunicatorAggregator communicatorAggregator_ is not nullptr");
        errCode = communicatorAggregator_->RegOnConnectCallback(nullptr, nullptr);
        if (errCode != E_OK) {
            LOGW("[AutoLaunch] RegOnConnectCallback set nullptr failed, errCode:%d", errCode);
        }
        errCode = communicatorAggregator_->RegCommunicatorLackCallback(nullptr, nullptr);
        if (errCode != E_OK) {
            LOGW("[AutoLaunch] RegCommunicatorLackCallback set nullptr failed, errCode:%d", errCode);
        }
    }
    communicatorAggregator_ = aggregator;
    if (aggregator == nullptr) {
        LOGI("[AutoLaunch] SetCommunicatorAggregator aggregator is nullptr");
        return;
    }
    errCode = aggregator->RegOnConnectCallback(
        [this](const std::string &device, bool isConnect) { OnlineCallBack(device, isConnect); }, nullptr);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] RegOnConnectCallback errCode:%d", errCode);
    }
    errCode = aggregator->RegCommunicatorLackCallback(
        [this](const LabelType &label, const std::string &originalUserId) {
            return ReceiveUnknownIdentifierCallBack(label, originalUserId);
        }, nullptr);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] RegCommunicatorLackCallback errCode:%d", errCode);
    }
}

AutoLaunch::~AutoLaunch()
{
    {
        std::lock_guard<std::mutex> autoLock(communicatorLock_);
        LOGI("[AutoLaunch] ~AutoLaunch()");
        if (communicatorAggregator_ != nullptr) {
            communicatorAggregator_->RegOnConnectCallback(nullptr, nullptr);
            communicatorAggregator_->RegCommunicatorLackCallback(nullptr, nullptr);
            communicatorAggregator_ = nullptr;
        }
    }
    // {identifier, userId}
    std::set<std::pair<std::string, std::string>> inDisableSet;
    std::set<std::pair<std::string, std::string>> inWaitIdleSet;
    std::unique_lock<std::mutex> autoLock(dataLock_);
    for (auto &items : autoLaunchItemMap_) {
        for (auto &iter : items.second) {
            if (iter.second.isDisable) {
                inDisableSet.insert({ items.first, iter.first });
            } else if (iter.second.state == AutoLaunchItemState::IDLE && (!iter.second.inObserver)) {
                TryCloseConnection(iter.second);
            } else {
                inWaitIdleSet.insert({ items.first, iter.first });
                iter.second.isDisable = true;
            }
        }
    }
    for (const auto &identifierInfo : inDisableSet) {
        cv_.wait(autoLock, [identifierInfo, this] {
            return autoLaunchItemMap_.count(identifierInfo.first) == 0 ||
                autoLaunchItemMap_[identifierInfo.first].count(identifierInfo.second) == 0 ||
                (!autoLaunchItemMap_[identifierInfo.first][identifierInfo.second].isDisable);
        });
        if (autoLaunchItemMap_.count(identifierInfo.first) != 0 &&
            autoLaunchItemMap_[identifierInfo.first].count(identifierInfo.second)) { // LCOV_EXCL_BR_LINE
            TryCloseConnection(autoLaunchItemMap_[identifierInfo.first][identifierInfo.second]);
        }
    }
    for (const auto &info : inWaitIdleSet) {
        cv_.wait(autoLock, [info, this] {
            return (autoLaunchItemMap_[info.first][info.second].state == AutoLaunchItemState::IDLE) &&
                (!autoLaunchItemMap_[info.first][info.second].inObserver);
        });
        TryCloseConnection(autoLaunchItemMap_[info.first][info.second]);
    }
}

int AutoLaunch::EnableKvStoreAutoLaunchParmCheck(AutoLaunchItem &autoLaunchItem, const std::string &normalIdentifier,
    const std::string &dualTupleIdentifier, bool isDualTupleMode)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    std::string userId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::USER_ID, "");
    if (isDualTupleMode && autoLaunchItemMap_.count(normalIdentifier) != 0 &&
        autoLaunchItemMap_[normalIdentifier].count(userId) != 0) {
        LOGE("[AutoLaunch] EnableKvStoreAutoLaunchParmCheck identifier is already enabled in normal tuple mode");
        return -E_ALREADY_SET;
    }
    if (!isDualTupleMode && autoLaunchItemMap_.count(dualTupleIdentifier) != 0 &&
        autoLaunchItemMap_[dualTupleIdentifier].count(userId) != 0) {
        LOGE("[AutoLaunch] EnableKvStoreAutoLaunchParmCheck identifier is already enabled in dual tuple mode");
        return -E_ALREADY_SET;
    }
    std::string identifier = isDualTupleMode ? dualTupleIdentifier : normalIdentifier;
    if (identifier.empty()) {
        LOGE("[AutoLaunch] EnableKvStoreAutoLaunchParmCheck identifier is invalid");
        return -E_INVALID_ARGS;
    }
    if (autoLaunchItemMap_.count(identifier) != 0 && autoLaunchItemMap_[identifier].count(userId) != 0) {
        LOGE("[AutoLaunch] EnableKvStoreAutoLaunchParmCheck identifier is already enabled!");
        return -E_ALREADY_SET;
    }
    uint32_t autoLaunchItemSize = 0;
    for (const auto &item : autoLaunchItemMap_) {
        autoLaunchItemSize += item.second.size();
    }
    if (autoLaunchItemSize == MAX_AUTO_LAUNCH_ITEM_NUM) {
        LOGE("[AutoLaunch] EnableKvStoreAutoLaunchParmCheck size is max(8) now");
        return -E_MAX_LIMITS;
    }
    autoLaunchItem.state = AutoLaunchItemState::IN_ENABLE;
    autoLaunchItemMap_[identifier][userId] = autoLaunchItem;
    LOGI("[AutoLaunch] EnableKvStoreAutoLaunchParmCheck ok identifier=%.6s, isDual=%d",
        STR_TO_HEX(identifier), isDualTupleMode);
    return E_OK;
}

int AutoLaunch::EnableKvStoreAutoLaunch(const KvDBProperties &properties, AutoLaunchNotifier notifier,
    const AutoLaunchOption &option)
{
    LOGI("[AutoLaunch] EnableKvStoreAutoLaunch");
    bool isDualTupleMode = properties.GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE, false);
    std::string dualTupleIdentifier = properties.GetStringProp(KvDBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
    std::string identifier = properties.GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    std::string userId = properties.GetStringProp(KvDBProperties::USER_ID, "");
    std::shared_ptr<DBProperties> ptr = std::make_shared<KvDBProperties>(properties);
    AutoLaunchItem autoLaunchItem { ptr, notifier, option.observer, option.conflictType, option.notifier };
    autoLaunchItem.isAutoSync = option.isAutoSync;
    autoLaunchItem.type = DBTypeInner::DB_KV;
    int errCode = EnableKvStoreAutoLaunchParmCheck(autoLaunchItem, identifier, dualTupleIdentifier, isDualTupleMode);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] EnableKvStoreAutoLaunch failed errCode:%d", errCode);
        return errCode;
    }
    if (isDualTupleMode && !RuntimeContext::GetInstance()->IsSyncerNeedActive(properties)) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        std::string tmpIdentifier = isDualTupleMode ? dualTupleIdentifier : identifier;
        LOGI("[AutoLaunch] GetDoOpenMap identifier=%.6s no need to open", STR_TO_HEX(tmpIdentifier));
        autoLaunchItemMap_[tmpIdentifier][userId].state = AutoLaunchItemState::IDLE;
        return errCode;
    }
    errCode = GetKVConnectionInEnable(autoLaunchItem, isDualTupleMode ? dualTupleIdentifier : identifier);
    if (errCode == E_OK) {
        LOGI("[AutoLaunch] EnableKvStoreAutoLaunch ok");
    } else {
        LOGE("[AutoLaunch] EnableKvStoreAutoLaunch failed errCode:%d", errCode);
    }
    return errCode;
}

void AutoLaunch::GetKVConnectionInEnableInner(const AutoLaunchItem &autoLaunchItem, const std::string &identifier,
    const std::string &userId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
    autoLaunchItemMap_[identifier][userId].conn = autoLaunchItem.conn;
    autoLaunchItemMap_[identifier][userId].observerHandle = autoLaunchItem.observerHandle;
}

int AutoLaunch::GetKVConnectionInEnable(AutoLaunchItem &autoLaunchItem, const std::string &identifier)
{
    LOGI("[AutoLaunch] GetKVConnectionInEnable");
    int errCode;
    std::shared_ptr<KvDBProperties> properties = std::static_pointer_cast<KvDBProperties>(autoLaunchItem.propertiesPtr);
    if (properties == nullptr) {
        LOGE("[AutoLaunch] GetKVConnectionInEnable properties is nullptr");
        return -E_INTERNAL_ERROR;
    }
    std::string userId = properties->GetStringProp(KvDBProperties::USER_ID, "");
    autoLaunchItem.conn = KvDBManager::GetDatabaseConnection(*properties, errCode, false);
    if (errCode == -E_ALREADY_OPENED) {
        LOGI("[AutoLaunch] GetKVConnectionInEnable user already getkvstore by self");
        std::lock_guard<std::mutex> autoLock(dataLock_);
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
        return E_OK;
    }
    if (autoLaunchItem.conn == nullptr) {
        EraseAutoLaunchItem(identifier, userId);
        return errCode;
    }
    bool isEmpty = false;
    {
        std::lock_guard<std::mutex> onlineDevicesLock(dataLock_);
        isEmpty = onlineDevices_.empty();
    }
    if (isEmpty) {
        LOGI("[AutoLaunch] GetKVConnectionInEnable no online device, ReleaseDatabaseConnection");
        IKvDBConnection *kvConn = static_cast<IKvDBConnection *>(autoLaunchItem.conn);
        errCode = KvDBManager::ReleaseDatabaseConnection(kvConn);
        if (errCode != E_OK) {
            LOGE("[AutoLaunch] GetKVConnectionInEnable ReleaseDatabaseConnection failed errCode:%d", errCode);
            EraseAutoLaunchItem(identifier, userId);
            return errCode;
        }
        std::lock_guard<std::mutex> autoLock(dataLock_);
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
        return E_OK;
    }
    errCode = RegisterObserverAndLifeCycleCallback(autoLaunchItem, identifier, false);
    if (errCode == E_OK) {
        GetKVConnectionInEnableInner(autoLaunchItem, identifier, userId);
    } else {
        LOGE("[AutoLaunch] GetKVConnectionInEnable RegisterObserverAndLifeCycleCallback err, do CloseConnection");
        TryCloseConnection(autoLaunchItem); // do nothing if failed
        EraseAutoLaunchItem(identifier, userId);
    }
    return errCode;
}

// we will return errCode, if errCode != E_OK
int AutoLaunch::CloseConnectionStrict(AutoLaunchItem &autoLaunchItem)
{
    LOGI("[AutoLaunch] CloseConnectionStrict");
    if (autoLaunchItem.conn == nullptr) {
        LOGI("[AutoLaunch] CloseConnectionStrict conn is nullptr, do nothing");
        return E_OK;
    }
    IKvDBConnection *kvConn = static_cast<IKvDBConnection *>(autoLaunchItem.conn);
    int errCode = kvConn->RegisterLifeCycleCallback(nullptr);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] CloseConnectionStrict RegisterLifeCycleCallback failed errCode:%d", errCode);
        return errCode;
    }
    if (autoLaunchItem.observerHandle != nullptr) {
        errCode = kvConn->UnRegisterObserver(autoLaunchItem.observerHandle);
        if (errCode != E_OK) {
            LOGE("[AutoLaunch] CloseConnectionStrict UnRegisterObserver failed errCode:%d", errCode);
            return errCode;
        }
        autoLaunchItem.observerHandle = nullptr;
    }
    errCode = KvDBManager::ReleaseDatabaseConnection(kvConn);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] CloseConnectionStrict ReleaseDatabaseConnection failed errCode:%d", errCode);
    }
    return errCode;
}

// before ReleaseDatabaseConnection, if errCode != E_OK, we not return, we try close more
void AutoLaunch::TryCloseConnection(AutoLaunchItem &autoLaunchItem)
{
    LOGI("[AutoLaunch] TryCloseConnection");
    switch (autoLaunchItem.type) {
        case DBTypeInner::DB_KV:
            TryCloseKvConnection(autoLaunchItem);
            break;
        case DBTypeInner::DB_RELATION:
            TryCloseRelationConnection(autoLaunchItem);
            break;
        default:
            LOGD("[AutoLaunch] Unknown type[%d] when try to close connection", static_cast<int>(autoLaunchItem.type));
            break;
    }
}

int AutoLaunch::RegisterObserverAndLifeCycleCallback(AutoLaunchItem &autoLaunchItem, const std::string &identifier,
    bool isExt)
{
    int errCode = RegisterObserver(autoLaunchItem, identifier, isExt);
    if (errCode != E_OK) {
        return errCode;
    }
    LOGI("[AutoLaunch] RegisterObserver ok");

    errCode = RegisterLifeCycleCallback(autoLaunchItem, identifier, isExt);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch]  RegisterLifeCycleCallback failed, errCode:%d", errCode);
        return errCode;
    }
    LOGI("[AutoLaunch] RegisterLifeCycleCallback ok");

    errCode = SetConflictNotifier(autoLaunchItem);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch]  SetConflictNotifier failed, errCode:%d", errCode);
        return errCode;
    }

    return PragmaAutoSync(autoLaunchItem);
}

int AutoLaunch::RegisterObserver(AutoLaunchItem &autoLaunchItem, const std::string &identifier, bool isExt)
{
    if (autoLaunchItem.conn == nullptr) {
        LOGE("[AutoLaunch] autoLaunchItem.conn is nullptr");
        return -E_INTERNAL_ERROR;
    }
    LOGI("[AutoLaunch] RegisterObserver type=%d", static_cast<int>(autoLaunchItem.type));
    switch (autoLaunchItem.type) {
        case DBTypeInner::DB_RELATION:
            return RegisterRelationalObserver(autoLaunchItem, identifier, isExt);
        case DBTypeInner::DB_KV:
            return RegisterKvObserver(autoLaunchItem, identifier, isExt);
        default:
            return -E_INVALID_ARGS;
    }
}

void AutoLaunch::ObserverFunc(const KvDBCommitNotifyData &notifyData, const std::string &identifier,
    const std::string &userId)
{
    LOGD("[AutoLaunch] ObserverFunc identifier=%.6s", STR_TO_HEX(identifier));
    AutoLaunchItem autoLaunchItem;
    std::string appId;
    std::string storeId;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (autoLaunchItemMap_.count(identifier) == 0 || autoLaunchItemMap_[identifier].count(userId) == 0) {
            LOGE("[AutoLaunch] ObserverFunc err no this identifier in map");
            return;
        }
        if (autoLaunchItemMap_[identifier][userId].isDisable) {
            LOGI("[AutoLaunch] ObserverFunc isDisable, do nothing");
            return;
        }
        autoLaunchItemMap_[identifier][userId].inObserver = true;
        autoLaunchItem.observer = autoLaunchItemMap_[identifier][userId].observer;
        autoLaunchItem.isWriteOpenNotified = autoLaunchItemMap_[identifier][userId].isWriteOpenNotified;
        autoLaunchItem.notifier = autoLaunchItemMap_[identifier][userId].notifier;

        std::shared_ptr<KvDBProperties> properties =
            std::static_pointer_cast<KvDBProperties>(autoLaunchItemMap_[identifier][userId].propertiesPtr);
        appId = properties->GetStringProp(KvDBProperties::APP_ID, "");
        storeId = properties->GetStringProp(KvDBProperties::STORE_ID, "");
    }
    if (autoLaunchItem.observer != nullptr) {
        LOGI("[AutoLaunch] do user observer");
        KvStoreChangedDataImpl data(&notifyData);
        (autoLaunchItem.observer)->OnChange(data);
    }
    LOGI("[AutoLaunch] in observer autoLaunchItem.isWriteOpenNotified:%d", autoLaunchItem.isWriteOpenNotified);

    if (!autoLaunchItem.isWriteOpenNotified && autoLaunchItem.notifier != nullptr) {
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            autoLaunchItemMap_[identifier][userId].isWriteOpenNotified = true;
        }
        AutoLaunchNotifier notifier = autoLaunchItem.notifier;
        int retCode = RuntimeContext::GetInstance()->ScheduleTask([notifier, userId, appId, storeId] {
            LOGI("[AutoLaunch] notify the user auto opened event");
            notifier(userId, appId, storeId, AutoLaunchStatus::WRITE_OPENED);
        });
        if (retCode != E_OK) {
            LOGE("[AutoLaunch] ObserverFunc notifier ScheduleTask retCode:%d", retCode);
        }
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    autoLaunchItemMap_[identifier][userId].inObserver = false;
    cv_.notify_all();
}

int AutoLaunch::DisableKvStoreAutoLaunch(const std::string &normalIdentifier, const std::string &dualTupleIdentifier,
    const std::string &userId)
{
    std::string identifier;
    AutoLaunchItem autoLaunchItem;
    {
        std::unique_lock<std::mutex> autoLock(dataLock_);
        identifier = (autoLaunchItemMap_.count(normalIdentifier) == 0) ? dualTupleIdentifier : normalIdentifier;
        LOGI("[AutoLaunch] DisableKvStoreAutoLaunch identifier=%.6s", STR_TO_HEX(identifier));
        if (autoLaunchItemMap_.count(identifier) == 0 || autoLaunchItemMap_[identifier].count(userId) == 0) {
            LOGE("[AutoLaunch] DisableKvStoreAutoLaunch identifier is not exist!");
            return -E_NOT_FOUND;
        }
        if (autoLaunchItemMap_[identifier][userId].isDisable) {
            LOGI("[AutoLaunch] DisableKvStoreAutoLaunch already disabling in another thread, do nothing here");
            return -E_BUSY;
        }
        if (autoLaunchItemMap_[identifier][userId].state == AutoLaunchItemState::IN_ENABLE) {
            LOGE("[AutoLaunch] DisableKvStoreAutoLaunch enable not return, do not disable!");
            return -E_BUSY;
        }
        autoLaunchItemMap_[identifier][userId].isDisable = true;
        if (autoLaunchItemMap_[identifier][userId].state != AutoLaunchItemState::IDLE) {
            LOGI("[AutoLaunch] DisableKvStoreAutoLaunch wait idle");
            cv_.wait(autoLock, [identifier, userId, this] {
                return (autoLaunchItemMap_[identifier][userId].state == AutoLaunchItemState::IDLE) &&
                    (!autoLaunchItemMap_[identifier][userId].inObserver);
            });
            LOGI("[AutoLaunch] DisableKvStoreAutoLaunch wait idle ok");
        }
        autoLaunchItem = autoLaunchItemMap_[identifier][userId];
    }

    int errCode = CloseConnectionStrict(autoLaunchItem);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] DisableKvStoreAutoLaunch CloseConnection failed errCode:%d", errCode);
        std::lock_guard<std::mutex> autoLock(dataLock_);
        autoLaunchItemMap_[identifier][userId].isDisable = false;
        autoLaunchItemMap_[identifier][userId].observerHandle = autoLaunchItem.observerHandle;
        cv_.notify_all();
        return errCode;
    }

    EraseAutoLaunchItem(identifier, userId);
    cv_.notify_all();
    if (autoLaunchItem.isWriteOpenNotified && autoLaunchItem.notifier) {
        RuntimeContext::GetInstance()->ScheduleTask([autoLaunchItem] { CloseNotifier(autoLaunchItem); });
    }
    LOGI("[AutoLaunch] DisableKvStoreAutoLaunch ok");
    return E_OK;
}

void AutoLaunch::GetAutoLaunchSyncDevices(const std::string &identifier, std::vector<std::string> &devices) const
{
    devices.clear();
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (autoLaunchItemMap_.count(identifier) == 0) {
        LOGD("[AutoLaunch] GetSyncDevices identifier is not exist!");
        return;
    }
    for (const auto &device : onlineDevices_) {
        devices.push_back(device);
    }
    devices.shrink_to_fit();
}

void AutoLaunch::CloseNotifier(const AutoLaunchItem &autoLaunchItem)
{
    if (autoLaunchItem.notifier) {
        std::string userId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::USER_ID, "");
        std::string appId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::APP_ID, "");
        std::string storeId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::STORE_ID, "");
        LOGI("[AutoLaunch] CloseNotifier do autoLaunchItem.notifier");
        autoLaunchItem.notifier(userId, appId, storeId, AutoLaunchStatus::WRITE_CLOSED);
        LOGI("[AutoLaunch] CloseNotifier do autoLaunchItem.notifier finished");
    } else {
        LOGI("[AutoLaunch] CloseNotifier autoLaunchItem.notifier is nullptr");
    }
}

void AutoLaunch::ConnectionLifeCycleCallbackTask(const std::string &identifier, const std::string &userId)
{
    LOGI("[AutoLaunch] ConnectionLifeCycleCallbackTask identifier=%.6s", STR_TO_HEX(identifier));
    AutoLaunchItem autoLaunchItem;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (autoLaunchItemMap_.count(identifier) == 0 || autoLaunchItemMap_[identifier].count(userId) == 0) {
            LOGE("[AutoLaunch] ConnectionLifeCycleCallback identifier is not exist!");
            return;
        }
        if (autoLaunchItemMap_[identifier][userId].isDisable) {
            LOGI("[AutoLaunch] ConnectionLifeCycleCallback isDisable, do nothing");
            return;
        }
        if (autoLaunchItemMap_[identifier][userId].state != AutoLaunchItemState::IDLE) {
            LOGI("[AutoLaunch] ConnectionLifeCycleCallback state:%d is not idle, do nothing",
                static_cast<int>(autoLaunchItemMap_[identifier][userId].state));
            return;
        }
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IN_LIFE_CYCLE_CALL_BACK;
        autoLaunchItem = autoLaunchItemMap_[identifier][userId];
    }
    LOGI("[AutoLaunch] ConnectionLifeCycleCallbackTask do CloseConnection");
    TryCloseConnection(autoLaunchItem); // do nothing if failed
    LOGI("[AutoLaunch] ConnectionLifeCycleCallback do CloseConnection finished");
    {
        std::lock_guard<std::mutex> lock(dataLock_);
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
        autoLaunchItemMap_[identifier][userId].conn = nullptr;
        autoLaunchItemMap_[identifier][userId].isWriteOpenNotified = false;
        cv_.notify_all();
        LOGI("[AutoLaunch] ConnectionLifeCycleCallback notify_all");
    }
    if (autoLaunchItem.isWriteOpenNotified) {
        CloseNotifier(autoLaunchItem);
    }
}

void AutoLaunch::ConnectionLifeCycleCallback(const std::string &identifier, const std::string &userId)
{
    LOGI("[AutoLaunch] ConnectionLifeCycleCallback identifier=%.6s", STR_TO_HEX(identifier));
    int errCode = RuntimeContext::GetInstance()->ScheduleTask(
        [this, identifier, userId] { ConnectionLifeCycleCallbackTask(identifier, userId); });
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] ConnectionLifeCycleCallback ScheduleTask failed");
    }
}

int AutoLaunch::OpenOneConnection(AutoLaunchItem &autoLaunchItem)
{
    LOGI("[AutoLaunch] GetOneConnection");
    int errCode;
    switch (autoLaunchItem.type) {
        case DBTypeInner::DB_KV:
            errCode = OpenKvConnection(autoLaunchItem);
            break;
        case DBTypeInner::DB_RELATION:
            errCode = OpenRelationalConnection(autoLaunchItem);
            break;
        default:
            errCode = -E_INVALID_ARGS;
    }
    return errCode;
}

void AutoLaunch::OnlineCallBack(const std::string &device, bool isConnect)
{
    LOGI("[AutoLaunch] OnlineCallBack device:%s{private}, isConnect:%d", device.c_str(), isConnect);
    if (!isConnect) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        onlineDevices_.erase(device);
        return;
    }
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        onlineDevices_.insert(device);
    }

    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this] { OnlineCallBackTask(); });
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] OnlineCallBack ScheduleTask failed");
    }
}

void AutoLaunch::OnlineCallBackTask()
{
    LOGI("[AutoLaunch] OnlineCallBackTask");
    // <identifier, <userId, AutoLaunchItem>>
    std::map<std::string, std::map<std::string, AutoLaunchItem>> doOpenMap;
    GetDoOpenMap(doOpenMap);
    GetConnInDoOpenMap(doOpenMap);
    UpdateGlobalMap(doOpenMap);
}

void AutoLaunch::GetDoOpenMap(std::map<std::string, std::map<std::string, AutoLaunchItem>> &doOpenMap)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    LOGI("[AutoLaunch] GetDoOpenMap");
    for (auto &items : autoLaunchItemMap_) {
        for (auto &iter : items.second) {
            bool isDualTupleMode = iter.second.propertiesPtr->GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false);
            if (iter.second.isDisable) {
                LOGI("[AutoLaunch] GetDoOpenMap this item isDisable do nothing");
                continue;
            } else if (iter.second.state != AutoLaunchItemState::IDLE) {
                LOGI("[AutoLaunch] GetDoOpenMap this item state:%d is not idle do nothing",
                    static_cast<int>(iter.second.state));
                continue;
            } else if (iter.second.conn != nullptr) {
                LOGI("[AutoLaunch] GetDoOpenMap this item is opened");
                continue;
            } else if (isDualTupleMode &&
                !RuntimeContext::GetInstance()->IsSyncerNeedActive(*iter.second.propertiesPtr)) {
                LOGI("[AutoLaunch] GetDoOpenMap this item no need to open");
                continue;
            } else {
                doOpenMap[items.first][iter.first] = iter.second;
                iter.second.state = AutoLaunchItemState::IN_COMMUNICATOR_CALL_BACK;
                LOGI("[AutoLaunch] GetDoOpenMap this item in IN_COMMUNICATOR_CALL_BACK");
            }
        }
    }
}

void AutoLaunch::GetConnInDoOpenMapInner(std::pair<const std::string, std::map<std::string, AutoLaunchItem>> &items,
    SemaphoreUtils &sema)
{
    for (auto &iter : items.second) {
        int errCode = RuntimeContext::GetInstance()->ScheduleTask([&sema, &iter, &items, this] {
            int ret = OpenOneConnection(iter.second);
            LOGI("[AutoLaunch] GetConnInDoOpenMap GetOneConnection errCode:%d", ret);
            if (ret != E_OK) { // LCOV_EXCL_BR_LINE
                sema.SendSemaphore();
                LOGI("[AutoLaunch] GetConnInDoOpenMap in open thread finish SendSemaphore");
                return;
            }
            ret = RegisterObserverAndLifeCycleCallback(iter.second, items.first, false);
            if (ret != E_OK) { // LCOV_EXCL_BR_LINE
                LOGE("[AutoLaunch] GetConnInDoOpenMap failed, we do CloseConnection");
                TryCloseConnection(iter.second); // if here failed, do nothing
                iter.second.conn = nullptr;
            }
            sema.SendSemaphore();
            LOGI("[AutoLaunch] GetConnInDoOpenMap in open thread finish SendSemaphore");
        });
        if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            LOGE("[AutoLaunch] GetConnInDoOpenMap ScheduleTask failed, SendSemaphore");
            sema.SendSemaphore();
        }
    }
}

void AutoLaunch::GetConnInDoOpenMap(std::map<std::string, std::map<std::string, AutoLaunchItem>> &doOpenMap)
{
    LOGI("[AutoLaunch] GetConnInDoOpenMap doOpenMap.size():%zu", doOpenMap.size());
    if (doOpenMap.empty()) {
        return;
    }
    uint32_t totalSize = 0u;
    for (auto &items : doOpenMap) {
        totalSize += items.second.size();
    }
    SemaphoreUtils sema(1 - totalSize);
    for (auto &items : doOpenMap) {
        GetConnInDoOpenMapInner(items, sema);
    }
    LOGI("[AutoLaunch] GetConnInDoOpenMap WaitSemaphore");
    sema.WaitSemaphore();
    LOGI("[AutoLaunch] GetConnInDoOpenMap WaitSemaphore ok");
}

void AutoLaunch::UpdateGlobalMap(std::map<std::string, std::map<std::string, AutoLaunchItem>> &doOpenMap)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    LOGI("[AutoLaunch] UpdateGlobalMap");
    for (const auto &items : doOpenMap) {
        for (const auto &iter : items.second) {
            if (iter.second.conn != nullptr) {
                autoLaunchItemMap_[items.first][iter.first].conn = iter.second.conn;
                autoLaunchItemMap_[items.first][iter.first].observerHandle = iter.second.observerHandle;
                autoLaunchItemMap_[items.first][iter.first].isWriteOpenNotified = false;
                LOGI("[AutoLaunch] UpdateGlobalMap opened conn update map");
            }
            autoLaunchItemMap_[items.first][iter.first].state = AutoLaunchItemState::IDLE;
            LOGI("[AutoLaunch] UpdateGlobalMap opened conn set state IDLE");
        }
    }
    cv_.notify_all();
    LOGI("[AutoLaunch] UpdateGlobalMap finish notify_all");
}

void AutoLaunch::ReceiveUnknownIdentifierCallBackTask(const std::string &identifier, const std::string &userId)
{
    LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBackTask identifier=%.6s", STR_TO_HEX(identifier));
    AutoLaunchItem autoLaunchItem;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        autoLaunchItem = autoLaunchItemMap_[identifier][userId];
    }
    int errCode = OpenOneConnection(autoLaunchItem);
    LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBack GetOneConnection errCode:%d", errCode);
    if (autoLaunchItem.conn == nullptr) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
        cv_.notify_all();
        LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBackTask set state IDLE");
        return;
    }
    errCode = RegisterObserverAndLifeCycleCallback(autoLaunchItem, identifier, false);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] ReceiveUnknownIdentifierCallBackTask RegisterObserverAndLifeCycleCallback failed");
        LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBackTask do CloseConnection");
        TryCloseConnection(autoLaunchItem); // if here failed, do nothing
        std::lock_guard<std::mutex> autoLock(dataLock_);
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
        cv_.notify_all();
        LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBackTask set state IDLE");
        return;
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    autoLaunchItemMap_[identifier][userId].conn = autoLaunchItem.conn;
    autoLaunchItemMap_[identifier][userId].observerHandle = autoLaunchItem.observerHandle;
    autoLaunchItemMap_[identifier][userId].isWriteOpenNotified = false;
    autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
    cv_.notify_all();
    LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBackTask conn opened set state IDLE");
}

int AutoLaunch::ReceiveUnknownIdentifierCallBack(const LabelType &label, const std::string &originalUserId)
{
    const std::string identifier(label.begin(), label.end());
    LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBack identifier=%.6s", STR_TO_HEX(identifier));
    // originalUserId size maybe 0
    bool handleByCallback = false;
    std::string userId = GetAutoLaunchItemUid(identifier, originalUserId, handleByCallback);
    if (handleByCallback) {
        goto EXT;
    }
    int errCode;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (autoLaunchItemMap_.count(identifier) == 0 || autoLaunchItemMap_[identifier].count(userId) == 0) {
            LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBack not find identifier");
            goto EXT;
        } else if (autoLaunchItemMap_[identifier][userId].isDisable) {
            LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBack isDisable ,do nothing");
            return -E_NOT_FOUND; // not E_OK is ok for communicator
        } else if (autoLaunchItemMap_[identifier][userId].conn != nullptr) {
            LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBack conn is not nullptr");
            return E_OK;
        } else if (autoLaunchItemMap_[identifier][userId].state != AutoLaunchItemState::IDLE) {
            LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBack state:%d is not idle, do nothing",
                static_cast<int>(autoLaunchItemMap_[identifier][userId].state));
            return E_OK;
        }
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IN_COMMUNICATOR_CALL_BACK;
        LOGI("[AutoLaunch] ReceiveUnknownIdentifierCallBack set state IN_COMMUNICATOR_CALL_BACK");
    }

    errCode = RuntimeContext::GetInstance()->ScheduleTask(
        [this, identifier, userId] { ReceiveUnknownIdentifierCallBackTask(identifier, userId); });
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] ReceiveUnknownIdentifierCallBack ScheduleTask failed");
        std::lock_guard<std::mutex> autoLock(dataLock_);
        autoLaunchItemMap_[identifier][userId].state = AutoLaunchItemState::IDLE;
    }
    return errCode;
EXT:
    return AutoLaunchExt(identifier, userId);
}

void AutoLaunch::SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback, DBTypeInner type)
{
    LOGI("[AutoLaunch] SetAutoLaunchRequestCallback type[%d]", static_cast<int>(type));
    TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, callback, type] () mutable {
        ConcurrentAdapter::AdapterAutoLock(extLock_);
        ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
        if (callback) {
            autoLaunchRequestCallbackMap_[type] = callback;
        } else {
            autoLaunchRequestCallbackMap_.erase(type);
        }
    }, nullptr, &autoLaunchRequestCallbackMap_);
    ADAPTER_WAIT(handle);
}

int AutoLaunch::AutoLaunchExt(const std::string &identifier, const std::string &userId)
{
    AutoLaunchParam param;
    // for non dual tuple mode, userId is ""
    param.userId = userId;
    DBTypeInner openType = DBTypeInner::DB_INVALID;
    int errCode = ExtAutoLaunchRequestCallBack(identifier, param, openType);
    if (errCode != E_OK) {
        return errCode;  // not E_OK is ok for communicator
    }

    std::shared_ptr<DBProperties> ptr;
    errCode = AutoLaunch::GetAutoLaunchProperties(param, openType, false, ptr);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] AutoLaunchExt param check fail errCode:%d", errCode);
        if (!param.notifier) {
            return errCode;
        }
        int retCode = RuntimeContext::GetInstance()->ScheduleTask([param] {
            param.notifier(param.userId, param.appId, param.storeId, INVALID_PARAM);
        });
        if (retCode != E_OK) {
            LOGE("[AutoLaunch] AutoLaunchExt notifier ScheduleTask retCode:%d", retCode);
        }
        return errCode;
    }
    AutoLaunchItem autoLaunchItem{ptr, param.notifier, param.option.observer, param.option.conflictType,
        param.option.notifier};
    autoLaunchItem.isAutoSync = param.option.isAutoSync;
    autoLaunchItem.type = openType;
    autoLaunchItem.storeObserver = param.option.storeObserver;
    errCode = RuntimeContext::GetInstance()->ScheduleTask([this, identifier, userId = param.userId, autoLaunchItem] {
        AutoLaunchItem item = autoLaunchItem;
        AutoLaunchExtTask(identifier, userId, item);
    });
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] AutoLaunchExt ScheduleTask errCode:%d", errCode);
    }
    return errCode;
}

void AutoLaunch::AutoLaunchExtTask(const std::string &identifier, const std::string &userId,
    AutoLaunchItem &autoLaunchItem)
{
    {
        bool isReturn = false;
        TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &userId,
            &autoLaunchItem, &isReturn] () mutable {
            ConcurrentAdapter::AdapterAutoLock(extLock_);
            ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
            if (extItemMap_.count(identifier) != 0 && extItemMap_[identifier].count(userId) != 0) {
                LOGE("[AutoLaunch] extItemMap has this identifier");
                isReturn = true;
                return;
            }
            extItemMap_[identifier][userId] = autoLaunchItem;
        }, nullptr, &extItemMap_);
        ADAPTER_WAIT(handle);
        if (isReturn) {
            return;
        }
    }
    bool abort = ChkAutoLaunchAbort(identifier, autoLaunchItem);
    if (abort) {
        TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &userId] () mutable {
            ConcurrentAdapter::AdapterAutoLock(extLock_);
            ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
            extItemMap_[identifier].erase(userId);
            if (extItemMap_[identifier].empty()) {
                extItemMap_.erase(identifier);
            }
        }, nullptr, &extItemMap_);
        ADAPTER_WAIT(handle);
        return;
    }
    TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &userId, &autoLaunchItem] () mutable {
        ConcurrentAdapter::AdapterAutoLock(extLock_);
        ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
        extItemMap_[identifier][userId] = autoLaunchItem; // Reassign item to prevent it from being erased
        extItemMap_[identifier][userId].isWriteOpenNotified = false;
        LOGI("[AutoLaunch] AutoLaunchExtTask ok");
    }, nullptr, &extItemMap_);
    ADAPTER_WAIT(handle);
}

void AutoLaunch::ExtObserverFunc(const KvDBCommitNotifyData &notifyData, const std::string &identifier,
    const std::string &userId)
{
    LOGD("[AutoLaunch] ExtObserverFunc identifier=%.6s", STR_TO_HEX(identifier));
    AutoLaunchItem autoLaunchItem;
    AutoLaunchNotifier notifier;
    {
        bool isReturn = false;
        TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &userId,
            &autoLaunchItem, &isReturn] () mutable {
            ConcurrentAdapter::AdapterAutoLock(extLock_);
            ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
            if (extItemMap_.count(identifier) == 0 || extItemMap_[identifier].count(userId) == 0) {
                LOGE("[AutoLaunch] ExtObserverFunc this identifier not in map");
                isReturn = true;
                return;
            }
            autoLaunchItem = extItemMap_[identifier][userId];
        }, &extItemMap_, nullptr);
        ADAPTER_WAIT(handle);
        if (isReturn) {
            return;
        }
    }
    if (autoLaunchItem.observer != nullptr) {
        LOGD("[AutoLaunch] do user observer");
        KvStoreChangedDataImpl data(&notifyData);
        autoLaunchItem.observer->OnChange(data);
    }

    {
        bool isReturn = false;
        TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &userId, &autoLaunchItem,
            &notifier, &isReturn] () mutable {
            ConcurrentAdapter::AdapterAutoLock(extLock_);
            ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
            if (extItemMap_.count(identifier) != 0 && extItemMap_[identifier].count(userId) != 0 &&
                !extItemMap_[identifier][userId].isWriteOpenNotified &&
                autoLaunchItem.notifier != nullptr) {
                extItemMap_[identifier][userId].isWriteOpenNotified = true;
                notifier = autoLaunchItem.notifier;
            } else {
                isReturn = true;
            }
        }, nullptr, &extItemMap_);
        ADAPTER_WAIT(handle);
        if (isReturn) {
            return;
        }
    }

    NotifyAutoLaunch(userId, autoLaunchItem, notifier);
}

void AutoLaunch::ExtConnectionLifeCycleCallback(const std::string &identifier, const std::string &userId)
{
    LOGI("[AutoLaunch] ExtConnectionLifeCycleCallback identifier=%.6s", STR_TO_HEX(identifier));
    int errCode = RuntimeContext::GetInstance()->ScheduleTask(
        [this, identifier, userId] { ExtConnectionLifeCycleCallbackTask(identifier, userId); });
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] ExtConnectionLifeCycleCallback ScheduleTask failed");
    }
}

void AutoLaunch::ExtConnectionLifeCycleCallbackTask(const std::string &identifier, const std::string &userId)
{
    LOGI("[AutoLaunch] ExtConnectionLifeCycleCallbackTask identifier=%.6s", STR_TO_HEX(identifier));
    AutoLaunchItem autoLaunchItem;
    {
        bool isReturn = false;
        TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &userId,
            &autoLaunchItem, &isReturn] () mutable {
            ConcurrentAdapter::AdapterAutoLock(extLock_);
            ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
            if (extItemMap_.count(identifier) == 0 || extItemMap_[identifier].count(userId) == 0) {
                LOGE("[AutoLaunch] ExtConnectionLifeCycleCallbackTask identifier is not exist!");
                isReturn = true;
                return;
            }
            autoLaunchItem = extItemMap_[identifier][userId];
            extItemMap_[identifier].erase(userId);
            if (extItemMap_[identifier].empty()) {
                extItemMap_.erase(identifier);
            }
        }, nullptr, &extItemMap_);
        ADAPTER_WAIT(handle);
        if (isReturn) {
            return;
        }
    }
    LOGI("[AutoLaunch] ExtConnectionLifeCycleCallbackTask do CloseConnection");
    TryCloseConnection(autoLaunchItem); // do nothing if failed
    if (autoLaunchItem.isWriteOpenNotified) {
        CloseNotifier(autoLaunchItem);
    }
}

int AutoLaunch::SetConflictNotifier(AutoLaunchItem &autoLaunchItem)
{
    if (autoLaunchItem.type != DBTypeInner::DB_KV) {
        LOGD("[AutoLaunch] Current Type[%d] Not Support ConflictNotifier Now", static_cast<int>(autoLaunchItem.type));
        return E_OK;
    }

    IKvDBConnection *kvConn = static_cast<IKvDBConnection *>(autoLaunchItem.conn);
    int conflictType = autoLaunchItem.conflictType;
    const KvStoreNbConflictNotifier &notifier = autoLaunchItem.conflictNotifier;
    if (conflictType == 0) {
        return E_OK;
    }
    int errCode;
    if (!notifier) {
        errCode = kvConn->SetConflictNotifier(conflictType, nullptr);
        goto END;
    }

    errCode = kvConn->SetConflictNotifier(conflictType,
        [conflictType, notifier](const KvDBCommitNotifyData &data) {
            int resultCode;
            const std::list<KvDBConflictEntry> entries = data.GetCommitConflicts(resultCode);
            if (resultCode != E_OK) {
                LOGE("Get commit conflicted entries failed:%d!", resultCode);
                return;
            }

            for (const auto &entry : entries) {
                // Prohibit signed numbers to perform bit operations
                uint32_t entryType = static_cast<uint32_t>(entry.type);
                uint32_t type = static_cast<uint32_t>(conflictType);
                if ((entryType & type) != 0) {
                    KvStoreNbConflictDataImpl dataImpl;
                    dataImpl.SetConflictData(entry);
                    notifier(dataImpl);
                }
            }
        });

END:
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Register conflict failed:%d!", errCode);
    }
    return errCode;
}

int AutoLaunch::GetAutoLaunchProperties(const AutoLaunchParam &param, const DBTypeInner &openType, bool checkDir,
    std::shared_ptr<DBProperties> &propertiesPtr)
{
    int errCode = E_OK;
    switch (openType) {
        case DBTypeInner::DB_KV: {
            propertiesPtr = std::make_shared<KvDBProperties>();
            std::shared_ptr<KvDBProperties> kvPtr = std::static_pointer_cast<KvDBProperties>(propertiesPtr);
            errCode = GetAutoLaunchKVProperties(param, kvPtr, checkDir);
            break;
        }
        case DBTypeInner::DB_RELATION: {
            propertiesPtr = std::make_shared<RelationalDBProperties>();
            std::shared_ptr<RelationalDBProperties> rdbPtr =
                std::static_pointer_cast<RelationalDBProperties>(propertiesPtr);
            errCode = GetAutoLaunchRelationProperties(param, rdbPtr);
            break;
        }
        default:
            return -E_INVALID_ARGS;
    }
    propertiesPtr->SetIntProp(DBProperties::AUTO_LAUNCH_ID,
        static_cast<int>(RuntimeContext::GetInstance()->GenerateSessionId()));
    return errCode;
}

int AutoLaunch::GetAutoLaunchKVProperties(const AutoLaunchParam &param,
    const std::shared_ptr<KvDBProperties> &propertiesPtr, bool checkDir)
{
    SchemaObject schemaObject;
    std::string canonicalDir;
    int errCode = ParamCheckUtils::CheckAndTransferAutoLaunchParam(param, checkDir, schemaObject, canonicalDir);
    if (errCode != E_OK) {
        return errCode;
    }

    if (param.option.isEncryptedDb) {
        propertiesPtr->SetPassword(param.option.cipher, param.option.passwd);
    }
    propertiesPtr->SetStringProp(KvDBProperties::DATA_DIR, canonicalDir);
    propertiesPtr->SetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, param.option.createIfNecessary);
    propertiesPtr->SetBoolProp(KvDBProperties::CREATE_DIR_BY_STORE_ID_ONLY, param.option.createDirByStoreIdOnly);
    propertiesPtr->SetBoolProp(KvDBProperties::MEMORY_MODE, false);
    propertiesPtr->SetBoolProp(KvDBProperties::ENCRYPTED_MODE, param.option.isEncryptedDb);
    propertiesPtr->SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
    propertiesPtr->SetIntProp(KvDBProperties::CONFLICT_RESOLVE_POLICY, param.option.conflictResolvePolicy);
    propertiesPtr->SetSchema(schemaObject);
    if (RuntimeContext::GetInstance()->IsProcessSystemApiAdapterValid()) {
        propertiesPtr->SetIntProp(KvDBProperties::SECURITY_LABEL, param.option.secOption.securityLabel);
        propertiesPtr->SetIntProp(KvDBProperties::SECURITY_FLAG, param.option.secOption.securityFlag);
    }
    propertiesPtr->SetBoolProp(DBProperties::COMPRESS_ON_SYNC, param.option.isNeedCompressOnSync);
    if (param.option.isNeedCompressOnSync) {
        propertiesPtr->SetIntProp(DBProperties::COMPRESSION_RATE,
            ParamCheckUtils::GetValidCompressionRate(param.option.compressionRate));
    }
    propertiesPtr->SetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE, param.option.syncDualTupleMode);
    propertiesPtr->SetBoolProp(KvDBProperties::READ_ONLY_MODE, false);
    propertiesPtr->SetBoolProp(KvDBProperties::SHARED_MODE, false);
    DbIdParam dbIdParam = { param.appId, param.userId, param.storeId, param.subUser };
    DBCommon::SetDatabaseIds(*propertiesPtr, dbIdParam);
    return E_OK;
}

int AutoLaunch::GetAutoLaunchRelationProperties(const AutoLaunchParam &param,
    const std::shared_ptr<RelationalDBProperties> &propertiesPtr)
{
    if (!ParamCheckUtils::CheckStoreParameter({param.userId, param.appId, param.storeId},
        false, param.subUser, true)) {
        LOGE("[AutoLaunch] CheckStoreParameter is invalid.");
        return -E_INVALID_ARGS;
    }
    propertiesPtr->SetStringProp(RelationalDBProperties::DATA_DIR, param.path);
    propertiesPtr->SetIdentifier(param.userId, param.appId, param.storeId, param.subUser);
    propertiesPtr->SetBoolProp(RelationalDBProperties::SYNC_DUAL_TUPLE_MODE, param.option.syncDualTupleMode);
    propertiesPtr->SetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE, static_cast<int>(param.option.tableMode));
    if (param.option.isEncryptedDb) {
        if (!ParamCheckUtils::CheckEncryptedParameter(param.option.cipher, param.option.passwd) ||
            param.option.iterateTimes == 0) {
            return -E_INVALID_ARGS;
        }
        propertiesPtr->SetCipherArgs(param.option.cipher, param.option.passwd, param.option.iterateTimes);
    }
    propertiesPtr->SetBoolProp(DBProperties::COMPRESS_ON_SYNC, param.option.isNeedCompressOnSync);
    if (param.option.isNeedCompressOnSync) {
        propertiesPtr->SetIntProp(DBProperties::COMPRESSION_RATE,
            ParamCheckUtils::GetValidCompressionRate(param.option.compressionRate));
    }
    return E_OK;
}

int AutoLaunch::ExtAutoLaunchRequestCallBack(const std::string &identifier, AutoLaunchParam &param,
    DBTypeInner &openType)
{
    std::map<DBTypeInner, AutoLaunchRequestCallback> callbackMap;
    TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &callbackMap] () mutable {
        ConcurrentAdapter::AdapterAutoLock(extLock_);
        ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
        callbackMap = autoLaunchRequestCallbackMap_;
        return E_OK;
    }, &autoLaunchRequestCallbackMap_);
    ADAPTER_WAIT(handle);
    if (callbackMap.empty()) {
        LOGI("[AutoLaunch] autoLaunchRequestCallbackMap_ is empty");
        return -E_NOT_FOUND;
    }
    bool needOpen = false;
    for (const auto &[type, callBack] : callbackMap) {
        needOpen = callBack(identifier, param);
        if (needOpen) {
            openType = type;
            break;
        }
    }

    if (!needOpen) {
        LOGI("[AutoLaunch] autoLaunchRequestCallback is not need open");
        return -E_NOT_FOUND; // not E_OK is ok for communicator
    }
    return E_OK;
}

int AutoLaunch::OpenKvConnection(AutoLaunchItem &autoLaunchItem)
{
    std::shared_ptr<KvDBProperties> properties =
        std::static_pointer_cast<KvDBProperties>(autoLaunchItem.propertiesPtr);
    int errCode = E_OK;
    IKvDBConnection *conn = KvDBManager::GetDatabaseConnection(*properties, errCode, false);
    if (errCode == -E_ALREADY_OPENED) {
        LOGI("[AutoLaunch] GetOneConnection user already getkvstore by self");
    } else if (conn == nullptr) {
        LOGE("[AutoLaunch] GetOneConnection GetDatabaseConnection failed errCode:%d", errCode);
    }
    autoLaunchItem.conn = conn;
    return errCode;
}

int AutoLaunch::OpenRelationalConnection(AutoLaunchItem &autoLaunchItem)
{
    std::shared_ptr<RelationalDBProperties> properties =
        std::static_pointer_cast<RelationalDBProperties>(autoLaunchItem.propertiesPtr);
    int errCode = E_OK;
    auto conn = RelationalStoreInstance::GetDatabaseConnection(*properties, errCode, false);
    if (errCode == -E_ALREADY_OPENED) {
        LOGI("[AutoLaunch] GetOneConnection user already openstore by self");
    } else if (conn == nullptr) {
        LOGE("[AutoLaunch] GetOneConnection GetDatabaseConnection failed errCode:%d", errCode);
    }
    autoLaunchItem.conn = conn;
    return errCode;
}

int AutoLaunch::RegisterLifeCycleCallback(AutoLaunchItem &autoLaunchItem, const std::string &identifier,
    bool isExt)
{
    int errCode = E_OK;
    DatabaseLifeCycleNotifier notifier;
    if (isExt) {
        notifier = [this](const std::string &identifierExt, const std::string &userId) {
            ExtConnectionLifeCycleCallback(identifierExt, userId);
        };
    } else {
        notifier = [this](const std::string &identifierExt, const std::string &userId) {
            ConnectionLifeCycleCallback(identifierExt, userId);
        };
    }
    switch (autoLaunchItem.type) {
        case DBTypeInner::DB_KV:
            errCode = static_cast<IKvDBConnection *>(autoLaunchItem.conn)->RegisterLifeCycleCallback(notifier);
            break;
        case DBTypeInner::DB_RELATION:
            errCode =
                static_cast<RelationalStoreConnection *>(autoLaunchItem.conn)->RegisterLifeCycleCallback(notifier);
            break;
        default:
            LOGD("[AutoLaunch] Unknown Type[%d]", static_cast<int>(autoLaunchItem.type));
            break;
    }
    return errCode;
}

int AutoLaunch::PragmaAutoSync(AutoLaunchItem &autoLaunchItem)
{
    int errCode = E_OK;
    if (autoLaunchItem.type != DBTypeInner::DB_KV) {
        LOGD("[AutoLaunch] Current Type[%d] Not Support AutoSync Now", static_cast<int>(autoLaunchItem.type));
        return errCode;
    }

    bool enAutoSync = autoLaunchItem.isAutoSync;
    errCode = static_cast<SyncAbleKvDBConnection *>(autoLaunchItem.conn)->Pragma(PRAGMA_AUTO_SYNC,
        static_cast<void *>(&enAutoSync));
    if (errCode != E_OK) {
        LOGE("[AutoLaunch]  PRAGMA_AUTO_SYNC failed, errCode:%d", errCode);
        return errCode;
    }
    LOGI("[AutoLaunch] set PRAGMA_AUTO_SYNC ok, enAutoSync=%d", enAutoSync);
    return errCode;
}

void AutoLaunch::TryCloseKvConnection(AutoLaunchItem &autoLaunchItem)
{
    LOGI("[AutoLaunch] TryCloseKvConnection");
    if (autoLaunchItem.conn == nullptr) {
        LOGI("[AutoLaunch] TryCloseKvConnection conn is nullptr, do nothing");
        return;
    }
    IKvDBConnection *kvConn = static_cast<IKvDBConnection *>(autoLaunchItem.conn);
    int errCode = kvConn->RegisterLifeCycleCallback(nullptr);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] TryCloseKvConnection RegisterLifeCycleCallback failed errCode:%d", errCode);
    }
    if (autoLaunchItem.observerHandle != nullptr) {
        errCode = kvConn->UnRegisterObserver(autoLaunchItem.observerHandle);
        if (errCode != E_OK) {
            LOGE("[AutoLaunch] TryCloseKvConnection UnRegisterObserver failed errCode:%d", errCode);
        }
        autoLaunchItem.observerHandle = nullptr;
    }
    errCode = KvDBManager::ReleaseDatabaseConnection(kvConn);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] TryCloseKvConnection ReleaseDatabaseConnection failed errCode:%d", errCode);
    }
}

void AutoLaunch::TryCloseRelationConnection(AutoLaunchItem &autoLaunchItem)
{
    LOGI("[AutoLaunch] TryCloseRelationConnection");
    if (autoLaunchItem.conn == nullptr) {
        LOGI("[AutoLaunch] TryCloseRelationConnection conn is nullptr, do nothing");
        return;
    }
    RelationalStoreConnection *rdbConn = static_cast<RelationalStoreConnection *>(autoLaunchItem.conn);
    int errCode = rdbConn->RegisterLifeCycleCallback(nullptr);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] TryCloseRelationConnection RegisterLifeCycleCallback failed errCode:%d", errCode);
    }
    errCode = rdbConn->Close();
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] TryCloseRelationConnection close connection failed errCode:%d", errCode);
    }
}

void AutoLaunch::EraseAutoLaunchItem(const std::string &identifier, const std::string &userId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    autoLaunchItemMap_[identifier].erase(userId);
    if (autoLaunchItemMap_[identifier].empty()) {
        autoLaunchItemMap_.erase(identifier);
    }
}

void AutoLaunch::NotifyInvalidParam(const AutoLaunchItem &autoLaunchItem)
{
    if (!autoLaunchItem.notifier) {
        return;
    }
    int retCode = RuntimeContext::GetInstance()->ScheduleTask([autoLaunchItem] {
        std::string userId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::USER_ID, "");
        std::string appId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::APP_ID, "");
        std::string storeId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::STORE_ID, "");
        autoLaunchItem.notifier(userId, appId, storeId, INVALID_PARAM);
    });
    if (retCode != E_OK) {
        LOGE("[AutoLaunch] AutoLaunchExt notifier ScheduleTask retCode:%d", retCode);
    }
}

int AutoLaunch::CheckAutoLaunchRealPath(const AutoLaunchItem &autoLaunchItem)
{
    std::string canonicalDir;
    std::string dataDir = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::DATA_DIR, "");
    if (!ParamCheckUtils::CheckDataDir(dataDir, canonicalDir)) {
        LOGE("[AutoLaunch] CheckDataDir is invalid Auto Launch failed.");
        NotifyInvalidParam(autoLaunchItem);
        return -E_INVALID_ARGS;
    }
    autoLaunchItem.propertiesPtr->SetStringProp(DBProperties::DATA_DIR, canonicalDir);
    return E_OK;
}

void AutoLaunch::Dump(int fd)
{
    std::lock_guard<std::mutex> lock(dataLock_);
    DBDumpHelper::Dump(fd, "\tenableAutoLaunch info [\n");
    for (const auto &[label, userItem] : autoLaunchItemMap_) {
        DBDumpHelper::Dump(fd, "\t\tlabel = %s, userId = [\n", DBCommon::TransferStringToHex(label).c_str());
        for (const auto &entry : userItem) {
            DBDumpHelper::Dump(fd, "\t\t\t%s\n", entry.first.c_str());
        }
        DBDumpHelper::Dump(fd, "\t\t]\n");
    }
    DBDumpHelper::Dump(fd, "\t]\n");
}

int AutoLaunch::RegisterKvObserver(AutoLaunchItem &autoLaunchItem, const std::string &identifier, bool isExt)
{
    std::shared_ptr<KvDBProperties> properties =
        std::static_pointer_cast<KvDBProperties>(autoLaunchItem.propertiesPtr);
    std::string userId = properties->GetStringProp(KvDBProperties::USER_ID, "");
    int errCode;
    Key key;
    KvDBObserverHandle *observerHandle = nullptr;
    IKvDBConnection *kvConn = static_cast<IKvDBConnection *>(autoLaunchItem.conn);
    observerHandle = kvConn->RegisterObserver(OBSERVER_CHANGES_FOREIGN, key,
        [this, isExt, identifier, userId](const KvDBCommitNotifyData &notifyData) {
            if (isExt) {
                ExtObserverFunc(notifyData, identifier, userId);
            } else {
                ObserverFunc(notifyData, identifier, userId);
            }
        }, errCode);
    if (errCode != E_OK) {
        LOGE("[AutoLaunch] RegisterObserver failed:%d!", errCode);
        return errCode;
    }
    autoLaunchItem.observerHandle = observerHandle;
    return errCode;
}

int AutoLaunch::RegisterRelationalObserver(AutoLaunchItem &autoLaunchItem, const std::string &identifier, bool isExt)
{
    if (autoLaunchItem.storeObserver == nullptr) {
        return E_OK;
    }
    RelationalStoreConnection *conn = static_cast<RelationalStoreConnection *>(autoLaunchItem.conn);
    (void)conn->RegisterObserverAction(autoLaunchItem.storeObserver, [this, autoLaunchItem, identifier](
        const std::string &changedDevice, ChangedData &&changedData, bool isChangedData, Origin origin) {
        if (isChangedData && autoLaunchItem.storeObserver) {
            LOGD("begin to observer on changed data");
            autoLaunchItem.storeObserver->OnChange(origin, changedDevice, std::move(changedData));
            return;
        }
        std::string userId;
        std::string appId;
        std::string storeId;
        AutoLaunchOnChange(changedDevice, userId, appId, storeId, autoLaunchItem);
        bool isWriteOpenNotified = false;
        AutoLaunchNotifier notifier = nullptr;
        {
            bool isReturn = false;
            TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &userId,
                &notifier, &isWriteOpenNotified, &isReturn] () mutable {
                ConcurrentAdapter::AdapterAutoLock(extLock_);
                ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
                if (extItemMap_.find(identifier) == extItemMap_.end() ||
                    extItemMap_[identifier].find(userId) == extItemMap_[identifier].end()) {
                    LOGE("[AutoLaunch] ExtObserverFunc this identifier not in map");
                    isReturn = true;
                    return;
                }
                notifier = extItemMap_[identifier][userId].notifier;
                isWriteOpenNotified = extItemMap_[identifier][userId].isWriteOpenNotified;
                extItemMap_[identifier][userId].isWriteOpenNotified = true;
            }, nullptr, &extItemMap_);
            ADAPTER_WAIT(handle);
            if (isReturn) {
                return;
            }
        }
        if (!isWriteOpenNotified && notifier != nullptr) {
            notifier(userId, appId, storeId, WRITE_OPENED);
        }
    });
    return E_OK;
}

void AutoLaunch::CloseConnection(DBTypeInner type, const DBProperties &properties)
{
    std::string identifier = properties.GetStringProp(DBProperties::IDENTIFIER_DATA, "");
    int closeId = properties.GetIntProp(DBProperties::AUTO_LAUNCH_ID, 0);
    TaskHandle handle = ConcurrentAdapter::ScheduleTaskH([this, &identifier, &properties,
        closeId, type] () mutable {
        ConcurrentAdapter::AdapterAutoLock(extLock_);
        ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
        auto itemMapIter = extItemMap_.find(identifier);
        if (itemMapIter == extItemMap_.end()) {
            std::string dualIdentifier = properties.GetStringProp(DBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
            itemMapIter = extItemMap_.find(dualIdentifier); // Try find conn in dual tuple mode
            if (itemMapIter == extItemMap_.end()) {
                LOGD("[AutoLaunch] Abort close because not found id");
                return;
            }
            identifier = dualIdentifier;
        }
        std::string userId = properties.GetStringProp(DBProperties::USER_ID, "");
        auto itemIter = itemMapIter->second.find(userId);
        if (itemIter == itemMapIter->second.end()) {
            LOGD("[AutoLaunch] Abort close because not found user id");
            return;
        }
        if (itemIter->second.propertiesPtr == nullptr) {
            LOGD("[AutoLaunch] Abort close because properties is invalid");
            return;
        }
        int targetId = itemIter->second.propertiesPtr->GetIntProp(DBProperties::AUTO_LAUNCH_ID, 0);
        if (closeId != 0 && closeId != targetId) {
            LOGD("[AutoLaunch] Abort close because connection has been closed");
            return;
        }
        if (itemIter->second.type != type) {
            LOGE("[AutoLaunch] Not same DB type for close connection");
            return;
        }
        LOGI("[AutoLaunch] Force close connection");
        TryCloseConnection(itemIter->second);
        if (itemIter->second.isWriteOpenNotified) {
            CloseNotifier(itemIter->second);
        }
        LOGI("[AutoLaunch] Force close connection finished");
        extItemMap_[identifier].erase(userId);
        if (extItemMap_[identifier].size() == 0) {
            extItemMap_.erase(identifier);
        }
    }, nullptr, &extItemMap_);
    ADAPTER_WAIT(handle);
}

std::string AutoLaunch::GetAutoLaunchItemUid(const std::string &identifier, const std::string &originalUserId,
    bool &handleByCallback)
{
    std::string userId = originalUserId;
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (autoLaunchItemMap_.find(identifier) == autoLaunchItemMap_.end()) {
        LOGE("[AutoLaunch] miss match in white list");
        handleByCallback = true;
        return userId;
    }
    if (originalUserId.empty() && autoLaunchItemMap_[identifier].size() > 1) {
        LOGE("[AutoLaunch] normal tuple mode userId larger than one userId");
        handleByCallback = true;
        return userId;
    }
    if (originalUserId.empty() && autoLaunchItemMap_[identifier].size() == 1) {
        // normal tuple mode
        userId = autoLaunchItemMap_[identifier].begin()->first;
        return userId;
    }
    for (const auto &[uid, item]: autoLaunchItemMap_[identifier]) {
        if (item.propertiesPtr == nullptr) {
            continue;
        }
        if (!item.propertiesPtr->GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false)) {
            userId = uid;
            break;
        }
    }
    return userId;
}

bool AutoLaunch::ChkAutoLaunchAbort(const std::string &identifier, AutoLaunchItem &autoLaunchItem)
{
    bool abort = false;
    do {
        int errCode = CheckAutoLaunchRealPath(autoLaunchItem);
        if (errCode != E_OK) {
            abort = true;
            break;
        }
        errCode = OpenOneConnection(autoLaunchItem);
        LOGI("[AutoLaunch] AutoLaunchExtTask GetOneConnection errCode:%d", errCode);
        if (autoLaunchItem.conn == nullptr) {
            abort = true;
            break;
        }
        errCode = RegisterObserverAndLifeCycleCallback(autoLaunchItem, identifier, true);
        if (errCode != E_OK) {
            LOGE("[AutoLaunch] AutoLaunchExtTask RegisterObserverAndLifeCycleCallback failed");
            TryCloseConnection(autoLaunchItem); // if here failed, do nothing
            abort = true;
        }
    } while (false);
    return abort;
}

void AutoLaunch::NotifyAutoLaunch(const std::string &userId, AutoLaunchItem &autoLaunchItem,
    AutoLaunchNotifier &notifier)
{
    std::string appId = autoLaunchItem.propertiesPtr->GetStringProp(KvDBProperties::APP_ID, "");
    std::string storeId = autoLaunchItem.propertiesPtr->GetStringProp(KvDBProperties::STORE_ID, "");
    int retCode = RuntimeContext::GetInstance()->ScheduleTask([notifier, userId, appId, storeId] {
        LOGI("[AutoLaunch] ExtObserverFunc do user notifier WRITE_OPENED");
        notifier(userId, appId, storeId, AutoLaunchStatus::WRITE_OPENED);
    });
    if (retCode != E_OK) {
        LOGE("[AutoLaunch] ExtObserverFunc notifier ScheduleTask retCode:%d", retCode);
    }
}

void AutoLaunch::AutoLaunchOnChange(const std::string &changedDevice, std::string &userId, std::string &appId,
    std::string &storeId, AutoLaunchItem autoLaunchItem)
{
    RelationalStoreChangedDataImpl data(changedDevice);
    if (autoLaunchItem.propertiesPtr != nullptr) {
        userId = autoLaunchItem.propertiesPtr->GetStringProp(KvDBProperties::USER_ID, "");
        appId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::APP_ID, "");
        storeId = autoLaunchItem.propertiesPtr->GetStringProp(DBProperties::STORE_ID, "");
        data.SetStoreProperty({ userId, appId, storeId });
    }
    if (autoLaunchItem.storeObserver) {
        LOGD("begin to observer onchange, changedDevice=%s", STR_MASK(changedDevice));
        autoLaunchItem.storeObserver->OnChange(data);
    }
}
} // namespace DistributedDB
