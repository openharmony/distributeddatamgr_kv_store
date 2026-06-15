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
#define LOG_TAG "KvStoreDataServiceMgr"
#include "kvstore_data_service_mgr.h"
#include "log_print.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "system_ability_load_callback_stub.h"

namespace OHOS::DistributedKv {
namespace {
class ServiceProxyLoadCallback : public SystemAbilityLoadCallbackStub {
public:
    ServiceProxyLoadCallback() = default;
    virtual ~ServiceProxyLoadCallback() = default;

    void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject) override;
    void OnLoadSystemAbilityFail(int32_t systemAbilityId) override;
};

void ServiceProxyLoadCallback::OnLoadSystemAbilitySuccess(
    int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject)
{
    if (systemAbilityId != DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID) {
        ZLOGE("Incorrect SA Id: %{public}d", systemAbilityId);
        return;
    }
    if (remoteObject == nullptr) {
        ZLOGE("remote object is nullptr");
        return;
    }
}

void ServiceProxyLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    ZLOGE("Load SA: %{public}d failed", systemAbilityId);
}
}

int32_t KvStoreDataServiceMgr::ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex,
    int32_t tokenId)
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        ZLOGE("KvStoreDataServiceMgr get samgr fail.");
        return ERROR;
    }

    auto remote = samgr->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    if (remote == nullptr) {
        ZLOGE("Get distributed data manager CheckSystemAbility failed.");
        sptr<ServiceProxyLoadCallback> loadCallback = new (std::nothrow) ServiceProxyLoadCallback();
        if (loadCallback == nullptr) {
            ZLOGE("Create load callback failed.");
            return ERROR;
        }
        int32_t errCode = samgr->LoadSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, loadCallback);
        if (errCode != ERR_OK) {
            ZLOGE("Get distributed data manager LoadSystemAbility failed, err: %{public}d", errCode);
        }
        return ERROR;
    }

    sptr<IKvStoreDataService> ability = iface_cast<IKvStoreDataService>(remote);
    if (ability == nullptr) {
        ZLOGE("KvStoreDataServiceMgr initialize proxy failed.");
        return ERROR;
    }
    return ability->ClearAppStorage(bundleName, userId, appIndex, tokenId);
}
}
