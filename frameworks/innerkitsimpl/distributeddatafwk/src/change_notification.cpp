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

#define LOG_TAG "ChangeNotification"

#include "change_notification.h"

#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
ChangeNotification::ChangeNotification(std::vector<Entry> &&insertEntries, std::vector<Entry> &&updateEntries,
                                       std::vector<Entry> &&deleteEntries, const std::string &deviceId, bool isClear)
    : insertEntries_(std::move(insertEntries)), updateEntries_(std::move(updateEntries)),
    deleteEntries_(std::move(deleteEntries)), deviceId_(deviceId), isClear_(isClear)
{}

ChangeNotification::~ChangeNotification()
{}

const std::vector<Entry> &ChangeNotification::GetInsertEntries() const
{
    return insertEntries_;
}

const std::vector<Entry> &ChangeNotification::GetUpdateEntries() const
{
    return updateEntries_;
}

const std::vector<Entry> &ChangeNotification::GetDeleteEntries() const
{
    return deleteEntries_;
}

const std::string &ChangeNotification::GetDeviceId() const
{
    return deviceId_;
}

bool ChangeNotification::IsClear() const
{
    return isClear_;
}

}  // namespace DistributedKv
}  // namespace OHOS
