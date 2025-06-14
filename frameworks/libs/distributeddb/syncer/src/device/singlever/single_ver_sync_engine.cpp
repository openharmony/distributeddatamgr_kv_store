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

#include "single_ver_sync_engine.h"
#include "db_common.h"
#include "log_print.h"
#include "single_ver_kv_sync_task_context.h"
#include "single_ver_relational_sync_task_context.h"
#include "single_ver_sync_task_context.h"

namespace DistributedDB {
ISyncTaskContext *SingleVerSyncEngine::CreateSyncTaskContext(const ISyncInterface &syncInterface)
{
    SingleVerSyncTaskContext *context = nullptr;
    switch (syncInterface.GetInterfaceType()) {
        case ISyncInterface::SYNC_SVD:
            context = new (std::nothrow) SingleVerKvSyncTaskContext();
            break;
#ifdef RELATIONAL_STORE
        case ISyncInterface::SYNC_RELATION:
            context = new (std::nothrow) SingleVerRelationalSyncTaskContext();
            break;
#endif
        default:
            break;
    }

    if (context == nullptr) {
        LOGE("[SingleVerSyncEngine][CreateSyncTaskContext] create failed, may be out of memory");
        return nullptr;
    }
    context->SetSyncRetry(GetSyncRetry());
    context->EnableClearRemoteStaleData(needClearRemoteStaleData_);
    context->SetSubscribeManager(subManager_);
    return context;
}

void SingleVerSyncEngine::EnableClearRemoteStaleData(bool enable)
{
    LOGI("[SingleVerSyncEngine][EnableClearRemoteStaleData] enabled %d", enable);
    needClearRemoteStaleData_ = enable;
    std::unique_lock<std::mutex> lock(contextMapLock_);
    for (auto &iter : syncTaskContextMap_) {
        auto context = static_cast<SingleVerSyncTaskContext *>(iter.second);
        if (context != nullptr) { // LCOV_EXCL_BR_LINE
            context->EnableClearRemoteStaleData(enable);
        }
    }
}

int SingleVerSyncEngine::StartAutoSubscribeTimer(const ISyncInterface &syncInterface)
{
    if (syncInterface.IsSupportSubscribe() == -E_NOT_SUPPORT) {
        LOGI("[StartAutoSubscribeTimer] no need start");
        return E_OK;
    }

    std::lock_guard<std::mutex> lockGuard(timerLock_);
    if (subscribeTimerId_ > 0) {
        LOGI("[SingleSyncEngine] subscribeTimerId is already set");
        return -E_INTERNAL_ERROR;
    }
    TimerId timerId = 0;
    TimerAction timeOutCallback = [this](TimerId id) { return SubscribeTimeOut(id); };
    int errCode = RuntimeContext::GetInstance()->SetTimer(SUBSCRIBE_TRIGGER_TIME_OUT, timeOutCallback, nullptr,
        timerId);
    if (errCode != E_OK) {
        return errCode;
    }
    subscribeTimerId_ = timerId;
    LOGI("[SingleSyncEngine] start auto subscribe timerId=%" PRIu64 " finished", timerId);
    return errCode;
}

void SingleVerSyncEngine::StopAutoSubscribeTimer()
{
    TimerId subscribeTimerId = 0u;
    {
        std::lock_guard<std::mutex> lockGuard(timerLock_);
        if (subscribeTimerId_ == 0) {
            return;
        }
        subscribeTimerId = subscribeTimerId_;
        subscribeTimerId_ = 0;
    }
    RuntimeContext::GetInstance()->RemoveTimer(subscribeTimerId, true);
    LOGI("[SingleSyncEngine] stop auto subscribe timerId=%" PRIu64 " finished", subscribeTimerId);
}

int SingleVerSyncEngine::SubscribeTimeOut(TimerId id)
{
    if (!queryAutoSyncCallback_) { // LCOV_EXCL_BR_LINE
        return E_OK;
    }
    std::lock_guard<std::mutex> lockGuard(timerLock_);
    std::map<std::string, std::vector<QuerySyncObject>> allSyncQueries;
    GetAllUnFinishSubQueries(allSyncQueries);
    if (allSyncQueries.empty()) { // LCOV_EXCL_BR_LINE
        return E_OK;
    }
    for (const auto &item : allSyncQueries) {
        for (const auto &query : item.second) {
            InternalSyncParma param;
            GetSubscribeSyncParam(item.first, query, param);
            queryAutoSyncCallback_(param);
        }
    }
    return E_OK;
}
DEFINE_OBJECT_TAG_FACILITIES(SingleVerSyncEngine);
} // namespace DistributedDB