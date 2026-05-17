/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "DMSAdapter"

#include "dms_adapter.h"

#include "dms_handler.h"
#include "log_print.h"
#include "visibility.h"

using namespace OHOS::DistributedKv;

API_EXPORT std::shared_ptr<OHOS::DistributedKv::DMSAdapter> Create() asm("CreateDMSAdapterDelegate");

namespace OHOS::DistributedKv {
DMSAdapter::~DMSAdapter()
{}

class DMSAdapterImpl : public DMSAdapter {
public:
    ~DMSAdapterImpl() override = default;
    std::vector<EventNotify> GetDMSEventInfo() override;
};

std::vector<EventNotify> DMSAdapterImpl::GetDMSEventInfo()
{
    std::vector<DistributedSchedule::EventNotify> events;
    auto status = DistributedSchedule::DmsHandler::GetInstance().GetDSchedEventInfo(
        DistributedSchedule::DMS_COLLABORATION, events);
    if (status != 0) {
        ZLOGE("Get collaboration events failed, status:%{public}d", status);
        return {};
    }
    std::vector<EventNotify> notifyEvents;
    for (auto &event : events) {
        EventNotify eventNotify;
        eventNotify.srcBundleName_ = std::move(event.srcBundleName_);
        eventNotify.destBundleName_ = std::move(event.destBundleName_);
        eventNotify.dstNetworkId_ = std::move(event.dstNetworkId_);
        notifyEvents.push_back(eventNotify);
    }
    return notifyEvents;
}
} // namespace OHOS::DistributedKv

std::shared_ptr<OHOS::DistributedKv::DMSAdapter> Create()
{
    return std::make_shared<OHOS::DistributedKv::DMSAdapterImpl>();
}
