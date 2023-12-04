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

#ifndef PROCESS_SYSTEM_API_ADAPTER_IMPL_H
#define PROCESS_SYSTEM_API_ADAPTER_IMPL_H
#include <memory>
#include "iprocess_system_api_adapter.h"
#include "end_point.h"

namespace OHOS::DistributedKv {
class ProcessSystemApiAdapterImpl : public DistributedDB::IProcessSystemApiAdapter {
public:
    using AccessEventHanle = DistributedDB::OnAccessControlledEvent;
    using DBStatus = DistributedDB::DBStatus;
    using DBOption = DistributedDB::SecurityOption;
    API_EXPORT explicit ProcessSystemApiAdapterImpl(std::shared_ptr<Endpoint> endpoint);
    API_EXPORT ~ProcessSystemApiAdapterImpl();
    DBStatus RegOnAccessControlledEvent(const AccessEventHanle &callback) override;
    bool IsAccessControlled() const override;
    DBStatus SetSecurityOption(const std::string &filePath, const DBOption &option) override;
    DBStatus GetSecurityOption(const std::string &filePath, DBOption &option) const override;
    bool CheckDeviceSecurityAbility(const std::string &devId, const DBOption &option) const override;
private:
    std::shared_ptr<Endpoint> endpoint_;
};
}
#endif // PROCESS_SYSTEM_API_ADAPTER_IMPL_H
