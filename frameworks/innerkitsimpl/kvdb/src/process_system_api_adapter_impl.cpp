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
#define LOG_TAG "ProcessSystemApiAdapterImpl"
#include "process_system_api_adapter_impl.h"
#include <regex>
#include <sys/stat.h>
#include <unistd.h>
#include "log_print.h"
#include "security_label.h"

namespace OHOS::DistributedKv {
using Label = DistributedDB::SecurityLabel;
using Flag = DistributedDB::SecurityFlag;
using SecurityLabel = OHOS::FileManagement::ModuleSecurityLabel::SecurityLabel;
constexpr int32_t HEAD_SIZE = 3;
constexpr int32_t END_SIZE = 3;
constexpr const char *REPLACE_CHAIN = "***";
ProcessSystemApiAdapterImpl::ProcessSystemApiAdapterImpl(std::shared_ptr<Endpoint> endpoint)
    : endpoint_(endpoint)
{
}

ProcessSystemApiAdapterImpl::~ProcessSystemApiAdapterImpl()
{
}

ProcessSystemApiAdapterImpl::DBStatus ProcessSystemApiAdapterImpl::RegOnAccessControlledEvent(
    const AccessEventHanle &callback)
{
    return DBStatus::NOT_SUPPORT;
}

bool ProcessSystemApiAdapterImpl::IsAccessControlled() const
{
    return false;
}

ProcessSystemApiAdapterImpl::DBStatus ProcessSystemApiAdapterImpl::SetSecurityOption(const std::string &filePath,
    const DBOption &option)
{
    if (filePath.empty() || option.securityLabel < Label::NOT_SET || option.securityLabel > Label::S4) {
        return DBStatus::INVALID_ARGS;
    }

    struct stat curStat;
    stat(filePath.c_str(), &curStat);
    if (S_ISDIR(curStat.st_mode)) {
        return DBStatus::NOT_SUPPORT;
    }

    if (access(filePath.c_str(), F_OK) != 0) {
        return DBStatus::INVALID_ARGS;
    }

    if (option.securityLabel == Label::NOT_SET) {
        return DBStatus::OK;
    }

    auto secLevel = std::string("s") + std::to_string(option.securityLabel - 1);
    bool result = SecurityLabel::SetSecurityLabel(filePath, secLevel);
    if (!result) {
        auto fPath = filePath.substr(0, HEAD_SIZE) + REPLACE_CHAIN +
            filePath.substr(filePath.length() - END_SIZE, END_SIZE);
        ZLOGE("set label failed! level:%{public}s, file:%{public}s", secLevel.c_str(), fPath.c_str());
        return DBStatus::DB_ERROR;
    }
    return DBStatus::OK;
}

ProcessSystemApiAdapterImpl::DBStatus ProcessSystemApiAdapterImpl::GetSecurityOption(const std::string &filePath,
    DBOption &option) const
{
    if (filePath.empty()) {
        return DBStatus::INVALID_ARGS;
    }

    struct stat curStat;
    stat(filePath.c_str(), &curStat);
    if (S_ISDIR(curStat.st_mode)) {
        return DBStatus::NOT_SUPPORT;
    }

    if (access(filePath.c_str(), F_OK) != 0) {
        option = {Label::NOT_SET, Flag::ECE};
        return DBStatus::OK;
    }

    std::string value = SecurityLabel::GetSecurityLabel(filePath);
    if (!std::regex_match(value, std::regex("s([01234])"))) {
        option = {Label::NOT_SET, Flag::ECE};
        return DBStatus::OK;
    }
    option = { (value[1] - '0') + 1, value[1] == '3' ? Flag::SECE : Flag::ECE};
    return DBStatus::OK;
}

bool ProcessSystemApiAdapterImpl::CheckDeviceSecurityAbility(const std::string &devId, const DBOption &option) const
{
    auto securityLabel = option.securityLabel;
    return endpoint_->IsSaferThanDevice(securityLabel, devId);
}
} // namespace OHOS::DistributedKv