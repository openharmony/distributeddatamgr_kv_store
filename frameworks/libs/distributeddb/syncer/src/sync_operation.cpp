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

#include "sync_operation.h"
#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "performance_analysis.h"

namespace DistributedDB {
SyncOperation::SyncOperation(uint32_t syncId, const std::vector<std::string> &devices,
    int mode, const UserCallback &userCallback, bool isBlockSync)
    : devices_(devices),
      syncId_(syncId),
      mode_(mode),
      userCallback_(userCallback),
      isBlockSync_(isBlockSync),
      isAutoSync_(false),
      isFinished_(false),
      semaphore_(nullptr),
      query_(QuerySyncObject()),
      isQuerySync_(false),
      isAutoSubscribe_(false)
{
}

SyncOperation::~SyncOperation()
{
    RefObject::DecObjRef(context_);
    LOGD("SyncOperation::~SyncOperation()");
    Finalize();
}

int SyncOperation::Initialize()
{
    LOGD("[SyncOperation] Init SyncOperation id:%d.", syncId_);
    std::map<std::string, DeviceSyncProcess> tempSyncProcessMap;
    {
        AutoLock lockGuard(this);
        for (const std::string &deviceId : devices_) {
            statuses_.insert(std::pair<std::string, int>(deviceId, OP_WAITING));
            DeviceSyncProcess processInfo;
            processInfo.errCode = static_cast<DBStatus>(OP_WAITING);
            processInfo.syncId = syncId_;
            syncProcessMap_.insert(std::pair<std::string, DeviceSyncProcess>(deviceId, processInfo));
        }

        if (mode_ == AUTO_PUSH) {
            mode_ = PUSH;
            isAutoSync_ = true;
        } else if (mode_ == AUTO_PULL) {
            mode_ = PULL;
            isAutoSync_ = true;
        } else if (mode_ == AUTO_SUBSCRIBE_QUERY) {
            mode_ = SUBSCRIBE_QUERY;
            isAutoSubscribe_ = true;
        }
        if (isBlockSync_) {
            semaphore_ = std::make_unique<SemaphoreUtils>(0);
        }
        tempSyncProcessMap = syncProcessMap_;
    }
    if (userSyncProcessCallback_) {
        ExeSyncProcessCallFun(tempSyncProcessMap);
    }

    return E_OK;
}

void SyncOperation::SetOnSyncFinalize(const OnSyncFinalize &callback)
{
    onFinalize_ = callback;
}

void SyncOperation::SetOnSyncFinished(const OnSyncFinished &callback)
{
    onFinished_ = callback;
}

void SyncOperation::SetStatus(const std::string &deviceId, int status, int commErrCode)
{
    LOGD("[SyncOperation] SetStatus dev %s{private} status %d commErrCode %d", deviceId.c_str(), status, commErrCode);
    AutoLock lockGuard(this);
    if (IsKilled()) {
        LOGE("[SyncOperation] SetStatus failed, the SyncOperation has been killed!");
        return;
    }
    if (isFinished_) {
        LOGI("[SyncOperation] SetStatus already finished");
        return;
    }

    if (userSyncProcessCallback_) {
        if (syncProcessMap_[deviceId].errCode < static_cast<DBStatus>(OP_FINISHED_ALL)) {
            syncProcessMap_[deviceId].errCode = static_cast<DBStatus>(status);
        }
    }

    auto iter = statuses_.find(deviceId);
    if (iter != statuses_.end()) {
        if (iter->second >= OP_FINISHED_ALL) {
            return;
        }
        iter->second = status;
        if (((status != OP_COMM_ABNORMAL) && (status != OP_TIMEOUT)) || (commErrCode == E_OK)) {
            return;
        }
        commErrCodeMap_.insert(std::pair<std::string, int>(deviceId, commErrCode));
    }
}

void SyncOperation::SetUnfinishedDevStatus(int status)
{
    LOGD("[SyncOperation] SetUnfinishedDevStatus status %d", status);
    AutoLock lockGuard(this);
    if (IsKilled()) {
        LOGE("[SyncOperation] SetUnfinishedDevStatus failed, the SyncOperation has been killed!");
        return;
    }
    if (isFinished_) {
        LOGI("[SyncOperation] SetUnfinishedDevStatus already finished");
        return;
    }
    for (auto &item : statuses_) {
        if (item.second >= OP_FINISHED_ALL) {
            continue;
        }
        item.second = status;
    }
}

int SyncOperation::GetStatus(const std::string &deviceId) const
{
    AutoLock lockGuard(this);
    auto iter = statuses_.find(deviceId);
    if (iter != statuses_.end()) {
        return iter->second;
    }
    return -E_INVALID_ARGS;
}

uint32_t SyncOperation::GetSyncId() const
{
    return syncId_;
}

int SyncOperation::GetMode() const
{
    return mode_;
}

void SyncOperation::ReplaceCommErrCode(std::map<std::string, int> &finishStatus)
{
    for (auto &item : finishStatus) {
        if ((item.second != OP_COMM_ABNORMAL) && (item.second != OP_TIMEOUT)) {
            continue;
        }
        std::string deviceId = item.first;
        auto iter = commErrCodeMap_.find(deviceId);
        if (iter != commErrCodeMap_.end()) {
            item.second = iter->second;
        }
    }
}

void SyncOperation::Finished()
{
    std::map<std::string, int> tmpStatus;
    std::map<std::string, DeviceSyncProcess> tmpProcessMap;
    {
        AutoLock lockGuard(this);
        if (IsKilled() || isFinished_) {
            return;
        }
        isFinished_ = true;
        tmpStatus = statuses_;
        tmpProcessMap = syncProcessMap_;
        ReplaceCommErrCode(tmpStatus);
    }
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_ACK_RECV_TO_USER_CALL_BACK);
    }
    if (userCallback_) {
        std::string msg = GetFinishDetailMsg(tmpStatus);
        LOGI("[SyncOperation] SyncId=%d finished, %s", syncId_, msg.c_str());
        if (IsBlockSync()) {
            userCallback_(tmpStatus);
        } else {
            RefObject::IncObjRef(this);
            int errCode = RuntimeContext::GetInstance()->ScheduleQueuedTask(identifier_, [this, tmpStatus] {
                userCallback_(tmpStatus);
                RefObject::DecObjRef(this);
            });
            if (errCode != E_OK) {
                LOGE("[Finished] SyncOperation Finished userCallback_ retCode:%d", errCode);
                RefObject::DecObjRef(this);
            }
        }
    }

    if (userSyncProcessCallback_) {
        ExeSyncProcessCallFun(tmpProcessMap);
    }

    if (onFinished_) {
        LOGD("[SyncOperation] Sync %d finished call onFinished.", syncId_);
        onFinished_(syncId_);
    }
}

const std::vector<std::string> &SyncOperation::GetDevices() const
{
    return devices_;
}

void SyncOperation::WaitIfNeed()
{
    if (isBlockSync_ && (semaphore_ != nullptr)) {
        LOGD("[SyncOperation] Wait.");
        semaphore_->WaitSemaphore();
    }
}

void SyncOperation::NotifyIfNeed()
{
    if (isBlockSync_ && (semaphore_ != nullptr)) {
        LOGD("[SyncOperation] Notify.");
        semaphore_->SendSemaphore();
    }
}

bool SyncOperation::IsAutoSync() const
{
    return isAutoSync_;
}

bool SyncOperation::IsBlockSync() const
{
    return isBlockSync_;
}

bool SyncOperation::IsAutoControlCmd() const
{
    return isAutoSubscribe_;
}

void SyncOperation::SetSyncContext(RefObject *context)
{
    RefObject::DecObjRef(context_);
    context_ = context;
    RefObject::IncObjRef(context);
}

bool SyncOperation::CanCancel()
{
    return canCancel_;
}

void SyncOperation::SetSyncProcessCallFun(DeviceSyncProcessCallback callBack)
{
    if (callBack) {
        canCancel_ = true;
        this->userSyncProcessCallback_ = callBack;
    }
}

void SyncOperation::ExeSyncProcessCallFun(const std::map<std::string, DeviceSyncProcess> &syncProcessMap)
{
    if (IsBlockSync()) {
        userSyncProcessCallback_(syncProcessMap);
    } else {
        RefObject::IncObjRef(this);
        int errCode = RuntimeContext::GetInstance()->ScheduleQueuedTask(identifier_, [this, syncProcessMap] {
            userSyncProcessCallback_(syncProcessMap);
            RefObject::DecObjRef(this);
        });
        if (errCode != E_OK) {
            LOGE("[SyncOperation] ExeSyncProcessCallFun retCode:%d", errCode);
            RefObject::DecObjRef(this);
        }
    }
}

void SyncOperation::UpdateFinishedCount(const std::string &deviceId, uint32_t count)
{
    if (this->userSyncProcessCallback_) {
        std::map<std::string, DeviceSyncProcess> tmpMap;
        {
            AutoLock lockGuard(this);
            if (IsKilled()) {
                return;
            }
            LOGD("[UpdateFinishedCount] deviceId %s{private} count %u", deviceId.c_str(), count);
            this->syncProcessMap_[deviceId].pullInfo.finishedCount += count;
            tmpMap = this->syncProcessMap_;
        }
        ExeSyncProcessCallFun(tmpMap);
    }
}

void SyncOperation::SetSyncProcessTotal(const std::string &deviceId, uint32_t total)
{
    if (this->userSyncProcessCallback_) {
        {
            AutoLock lockGuard(this);
            if (IsKilled()) {
                return;
            }
            LOGD("[SetSyncProcessTotal] total=%u, syncId=%u, deviceId=%s{private}", total, syncId_, deviceId.c_str());
            this->syncProcessMap_[deviceId].pullInfo.total = total;
        }
    }
}

bool SyncOperation::CheckIsAllFinished() const
{
    AutoLock lockGuard(this);
    for (const auto &iter : statuses_) {
        if (iter.second < OP_FINISHED_ALL) {
            return false;
        }
    }
    return true;
}

void SyncOperation::Finalize()
{
    if ((syncId_ > 0) && onFinalize_) {
        LOGD("[SyncOperation] Callback SyncOperation onFinalize.");
        onFinalize_();
    }
}

void SyncOperation::SetQuery(const QuerySyncObject &query)
{
    std::lock_guard<std::mutex> lock(queryMutex_);
    query_ = query;
    isQuerySync_ = true;
    if (mode_ != SyncModeType::SUBSCRIBE_QUERY && mode_ != SyncModeType::UNSUBSCRIBE_QUERY) {
        mode_ += QUERY_SYNC_MODE_BASE;
    }
}

void SyncOperation::GetQuery(QuerySyncObject &targetObject) const
{
    std::lock_guard<std::mutex> lock(queryMutex_);
    targetObject = query_;
}

bool SyncOperation::IsQuerySync() const
{
    return isQuerySync_;
}

void SyncOperation::SetIdentifier(const std::vector<uint8_t> &identifier)
{
    identifier_.assign(identifier.begin(), identifier.end());
}

namespace {
struct SyncTypeNode {
    int mode = static_cast<int>(SyncModeType::INVALID_MODE);
    SyncType type = SyncType::INVALID_SYNC_TYPE;
};
struct SyncOperationStatusNode {
    int operationStatus = 0;
    DBStatus status = DBStatus::DB_ERROR;
};
struct SyncOperationProcessStatus {
    int operationStatus;
    ProcessStatus proStatus;
};
}

SyncType SyncOperation::GetSyncType(int mode)
{
    static const SyncTypeNode syncTypeNodes[] = {
        {static_cast<int>(SyncModeType::PUSH), SyncType::MANUAL_FULL_SYNC_TYPE},
        {static_cast<int>(SyncModeType::PULL), SyncType::MANUAL_FULL_SYNC_TYPE},
        {static_cast<int>(SyncModeType::PUSH_AND_PULL), SyncType::MANUAL_FULL_SYNC_TYPE},
        {static_cast<int>(SyncModeType::RESPONSE_PULL), SyncType::MANUAL_FULL_SYNC_TYPE},
        {static_cast<int>(SyncModeType::AUTO_PULL), SyncType::AUTO_SYNC_TYPE},
        {static_cast<int>(SyncModeType::AUTO_PUSH), SyncType::AUTO_SYNC_TYPE},
        {static_cast<int>(SyncModeType::QUERY_PUSH), SyncType::QUERY_SYNC_TYPE},
        {static_cast<int>(SyncModeType::QUERY_PULL), SyncType::QUERY_SYNC_TYPE},
        {static_cast<int>(SyncModeType::QUERY_PUSH_PULL), SyncType::QUERY_SYNC_TYPE}
    };
    const auto &result = std::find_if(std::begin(syncTypeNodes), std::end(syncTypeNodes), [mode](const auto &node) {
        return node.mode == mode;
    });
    return result == std::end(syncTypeNodes) ? SyncType::INVALID_SYNC_TYPE : result->type;
}

int SyncOperation::TransferSyncMode(int mode)
{
    // AUTO_PUSH and AUTO_PULL mode is used before sync, RESPONSE_PULL is regarded as push or query push mode.
    // so for the three mode, it is no need to transferred.
    if (mode >= SyncModeType::QUERY_PUSH && mode <= SyncModeType::QUERY_PUSH_PULL) {
        return (mode - QUERY_SYNC_MODE_BASE);
    }
    return mode;
}

std::string SyncOperation::GetQueryId() const
{
    std::lock_guard<std::mutex> lock(queryMutex_);
    return query_.GetIdentify();
}

DBStatus SyncOperation::DBStatusTrans(int operationStatus)
{
    static const SyncOperationStatusNode syncOperationStatusNodes[] = {
        { static_cast<int>(OP_FINISHED_ALL),                  OK },
        { static_cast<int>(OP_WAITING),                       OK },
        { static_cast<int>(OP_SYNCING),                       OK },
        { static_cast<int>(OP_SEND_FINISHED),                 OK },
        { static_cast<int>(OP_RECV_FINISHED),                 OK },
        { static_cast<int>(OP_TIMEOUT),                       TIME_OUT },
        { static_cast<int>(OP_PERMISSION_CHECK_FAILED),       PERMISSION_CHECK_FORBID_SYNC },
        { static_cast<int>(OP_COMM_ABNORMAL),                 COMM_FAILURE },
        { static_cast<int>(OP_SECURITY_OPTION_CHECK_FAILURE), SECURITY_OPTION_CHECK_ERROR },
        { static_cast<int>(OP_EKEYREVOKED_FAILURE),           EKEYREVOKED_ERROR },
        { static_cast<int>(OP_SCHEMA_INCOMPATIBLE),           SCHEMA_MISMATCH },
        { static_cast<int>(OP_BUSY_FAILURE),                  BUSY },
        { static_cast<int>(OP_QUERY_FORMAT_FAILURE),          INVALID_QUERY_FORMAT },
        { static_cast<int>(OP_QUERY_FIELD_FAILURE),           INVALID_QUERY_FIELD },
        { static_cast<int>(OP_NOT_SUPPORT),                   NOT_SUPPORT },
        { static_cast<int>(OP_INTERCEPT_DATA_FAIL),           INTERCEPT_DATA_FAIL },
        { static_cast<int>(OP_MAX_LIMITS),                    OVER_MAX_LIMITS },
        { static_cast<int>(OP_SCHEMA_CHANGED),                DISTRIBUTED_SCHEMA_CHANGED },
        { static_cast<int>(OP_INVALID_ARGS),                  INVALID_ARGS },
        { static_cast<int>(OP_USER_CHANGED),                  USER_CHANGED },
        { static_cast<int>(OP_DENIED_SQL),                    NO_PERMISSION },
        { static_cast<int>(OP_NOTADB_OR_CORRUPTED),           INVALID_PASSWD_OR_CORRUPTED_DB },
        { static_cast<int>(OP_FAILED),                        DB_ERROR },
    };
    const auto &result = std::find_if(std::begin(syncOperationStatusNodes), std::end(syncOperationStatusNodes),
        [operationStatus](const auto &node) {
            return node.operationStatus == operationStatus;
        });
    return result == std::end(syncOperationStatusNodes) ? static_cast<DBStatus>(operationStatus) : result->status;
}

ProcessStatus SyncOperation::DBStatusTransProcess(int operationStatus)
{
    static const SyncOperationProcessStatus syncOperationProcessStatus[] = {
        { static_cast<int>(OP_WAITING),                       PREPARED },
        { static_cast<int>(OP_SYNCING),                       PROCESSING },
        { static_cast<int>(OP_SEND_FINISHED),                 PROCESSING },
        { static_cast<int>(OP_RECV_FINISHED),                 PROCESSING },
        { static_cast<int>(OP_FINISHED_ALL),                  FINISHED },
        { static_cast<int>(OP_COMM_ABNORMAL),                 FINISHED },
    };
    const auto &result = std::find_if(std::begin(syncOperationProcessStatus), std::end(syncOperationProcessStatus),
        [operationStatus](const auto &node) {
            return node.operationStatus == operationStatus;
        });
    return result == std::end(syncOperationProcessStatus) ? FINISHED : result->proStatus;
}

std::string SyncOperation::GetFinishDetailMsg(const std::map<std::string, int> &finishStatus)
{
    std::string msg = "Sync detail is:";
    for (const auto &[dev, status]: finishStatus) {
        msg += "dev=" + DBCommon::StringMasking(dev);
        if ((status > static_cast<int>(OP_FINISHED_ALL)) || (status < E_OK)) {
            msg += " sync failed, reason is " + std::to_string(status);
        } else {
            msg += " sync success";
        }
        msg += " ";
    }
    msg.pop_back();
    return msg;
}
DEFINE_OBJECT_TAG_FACILITIES(SyncOperation)
} // namespace DistributedDB