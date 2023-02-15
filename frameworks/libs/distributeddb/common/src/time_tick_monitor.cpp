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

#include "time_tick_monitor.h"

#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
TimeTickMonitor::TimeTickMonitor()
    : timeChangedNotifier_(nullptr),
      runtimeCxt_(nullptr),
      monitorTimerId_(0),
      monitorCallback_(0),
      lastMonotonicTime_(0),
      lastSystemTime_(0),
      isStarted_(false)
{
}

TimeTickMonitor::~TimeTickMonitor()
{
    StopTimeTickMonitor();
    runtimeCxt_ = nullptr;
}

int TimeTickMonitor::StartTimeTickMonitor()
{
    if (isStarted_) {
        return E_OK;
    }

    int errCode = PrepareNotifierChain();
    if (errCode != E_OK) {
        return errCode;
    }

    lastMonotonicTime_ = GetMonotonicTime();
    lastSystemTime_ = GetSysCurrentTime();
    monitorCallback_ = std::bind(&TimeTickMonitor::TimeTick, this, std::placeholders::_1);
    runtimeCxt_ = RuntimeContext::GetInstance();
    monitorTimerId_ = 0;
    errCode = runtimeCxt_->SetTimer(MONITOR_INTERVAL, monitorCallback_, nullptr, monitorTimerId_);
    if (errCode != E_OK) {
        return errCode;
    }
    isStarted_ = true;
    return E_OK;
}

void TimeTickMonitor::StopTimeTickMonitor()
{
    if (!isStarted_) {
        return;
    }

    NotificationChain *notifier;
    {
        std::lock_guard<std::mutex> autoLock(timeTickMonitorLock_);
        notifier = timeChangedNotifier_;
        timeChangedNotifier_ = nullptr;
    }
    if (notifier != nullptr) {
        notifier->UnRegisterEventType(TIME_CHANGE_EVENT);
        RefObject::KillAndDecObjRef(notifier);
    }
    runtimeCxt_->RemoveTimer(monitorTimerId_, true);
    isStarted_ = false;
}

NotificationChain::Listener *TimeTickMonitor::RegisterTimeChangedLister(const TimeChangedAction &action, int &errCode)
{
    if (timeChangedNotifier_ == nullptr) {
        errCode = -E_NOT_INIT;
        return nullptr;
    }

    if (action == nullptr) {
        errCode = -E_INVALID_ARGS;
        return nullptr;
    }

    return timeChangedNotifier_->RegisterListener(TIME_CHANGE_EVENT, action, nullptr, errCode);
}

int TimeTickMonitor::PrepareNotifierChain()
{
    std::lock_guard<std::mutex> autoLock(timeTickMonitorLock_);
    if (timeChangedNotifier_ != nullptr) {
        return E_OK;
    }

    timeChangedNotifier_ = new (std::nothrow) NotificationChain();
    if (timeChangedNotifier_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    int errCode = timeChangedNotifier_->RegisterEventType(TIME_CHANGE_EVENT);
    if (errCode != E_OK) {
        RefObject::KillAndDecObjRef(timeChangedNotifier_);
        timeChangedNotifier_ = nullptr;
    }
    return errCode;
}

int TimeTickMonitor::TimeTick(TimerId timerId)
{
    if (timerId != monitorTimerId_) {
        return -E_INVALID_ARGS;
    }

    uint64_t monotonicTime = GetMonotonicTime();
    uint64_t systemTime = GetSysCurrentTime();
    int64_t monotonicOffset = static_cast<int64_t>(monotonicTime - lastMonotonicTime_);
    int64_t systemOffset = static_cast<int64_t>(systemTime - lastSystemTime_);
    lastMonotonicTime_ = monotonicTime;
    lastSystemTime_ = systemTime;
    int64_t changedOffset = systemOffset - monotonicOffset;
    if (std::abs(changedOffset) > MAX_NOISE) {
        LOGI("Local system time may be changed! changedOffset %ld", changedOffset);
        NotificationChain *notifier = nullptr;
        {
            std::lock_guard<std::mutex> autoLock(timeTickMonitorLock_);
            notifier = timeChangedNotifier_;
            RefObject::IncObjRef(notifier);
        }
        int ret = RuntimeContext::GetInstance()->ScheduleTask([notifier, changedOffset]() {
            if (notifier == nullptr) {
                return;
            }
            int64_t offset = changedOffset;
            notifier->NotifyEvent(TIME_CHANGE_EVENT, &offset);
            RefObject::DecObjRef(notifier);
        });
        if (ret != E_OK) {
            LOGE("TimeTickMonitor ScheduleTask failed %d", ret);
        }
    }
    return E_OK;
}

Timestamp TimeTickMonitor::GetSysCurrentTime()
{
    uint64_t curTime = 0;
    int errCode = OS::GetCurrentSysTimeInMicrosecond(curTime);
    if (errCode != E_OK) {
        LOGE("TimeTickMonitor:get system time failed!");
        return INVALID_TIMESTAMP;
    }
    return curTime;
}

Timestamp TimeTickMonitor::GetMonotonicTime()
{
    uint64_t time;
    int errCode = OS::GetMonotonicRelativeTimeInMicrosecond(time);
    if (errCode != E_OK) {
        LOGE("GetMonotonicTime ERR! err = %d", errCode);
        return INVALID_TIMESTAMP;
    }
    return time;
}

void TimeTickMonitor::NotifyTimeChange(TimeOffset offset) const
{
    std::lock_guard<std::mutex> lock(timeTickMonitorLock_);
    if (timeChangedNotifier_ == nullptr) {
        LOGD("NotifyTimeChange fail, timeChangedNotifier_ is null.");
        return;
    }
    timeChangedNotifier_->NotifyEvent(TIME_CHANGE_EVENT, static_cast<void *>(&offset));
}

bool TimeTickMonitor::EmptyListener() const
{
    std::lock_guard<std::mutex> lock(timeTickMonitorLock_);
    return timeChangedNotifier_->EmptyListener(TIME_CHANGE_EVENT);
}
} // namespace DistributedDB