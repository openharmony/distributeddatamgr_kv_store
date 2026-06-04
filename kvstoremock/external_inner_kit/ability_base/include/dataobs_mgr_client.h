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

#ifndef OHOS_ABILITY_RUNTIME_DATAOBS_MGR_CLIENT_H
#define OHOS_ABILITY_RUNTIME_DATAOBS_MGR_CLIENT_H

#include <mutex>

#include "data_ability_observer_interface.h"
#include "dataobs_mgr_errors.h"
#include "dataobs_mgr_interface.h"
#include "uri.h"

#include "iremote_object.h"

namespace OHOS {
namespace AAFwk {
/**
 * @class DataObsMgrClient
 * DataObsMgrClient is used to access dataobs manager services.
 */
class DataObsMgrClient {
public:
    DataObsMgrClient();
    virtual ~DataObsMgrClient();
    static std::shared_ptr<DataObsMgrClient> GetInstance();

    /**
     * Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterObserver(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
        int32_t userId = IDataObsMgr::DATAOBS_DEFAULT_CURRENT_USER,
        DataObsOption opt = DataObsOption());

    /**
     * Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterObserverFromExtension(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
        int32_t userId = IDataObsMgr::DATAOBS_DEFAULT_CURRENT_USER, DataObsOption opt = DataObsOption());

    /**
     * Deregisters an observer used for DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnregisterObserver(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
        int32_t userId = IDataObsMgr::DATAOBS_DEFAULT_CURRENT_USER, DataObsOption opt = DataObsOption());

    /**
     * Notifies the registered observers of a change to the data resource specified by Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode NotifyChange(const Uri &uri, int32_t userId = IDataObsMgr::DATAOBS_DEFAULT_CURRENT_USER,
        DataObsOption opt = DataObsOption());

    /**
     * Notifies the registered observers of a change to the data resource specified by Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode NotifyChangeFromExtension(const Uri &uri, int32_t userId = IDataObsMgr::DATAOBS_DEFAULT_CURRENT_USER,
        DataObsOption opt = DataObsOption());

    /**
     * Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    Status RegisterObserverExt(const Uri &uri, sptr<IDataAbilityObserver> dataObserver, bool isDescendants,
        DataObsOption opt = DataObsOption());

    /**
     * Deregisters an observer used for DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    Status UnregisterObserverExt(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
        DataObsOption opt = DataObsOption());

    /**
     * Deregisters observers used for DataObsMgr specified.
     *
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    Status UnregisterObserverExt(sptr<IDataAbilityObserver> dataObserver, DataObsOption opt = DataObsOption());

    /**
     * Notifies the registered observers of a change to the data resource specified by Uris.
     *
     * @param changeInfo Indicates the info of the data to operate.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    Status NotifyChangeExt(const ChangeInfo &changeInfo, DataObsOption opt = DataObsOption());

    /**
     * Notifies the process observer with the given progress key and cancel observer.
     *
     * @param key Identifies the progress of a specific task.

     * @param observer Observer for monitoring the ongoing process.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    Status NotifyProcessObserver(const std::string &key, const sptr<IRemoteObject> &observer,
        DataObsOption opt = DataObsOption());

private:
    /**
     * Connect dataobs manager service.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    std::pair<Status, sptr<IDataObsMgr>> GetObsMgr();

    void ResetService();
    void OnRemoteDied();
    void ReRegister();
    int32_t TryRegisterObserver(const Uri &uri, sptr<IDataAbilityObserver> key, int userId,
        bool isExtension, DataObsOption opt = {});

    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ServiceDeathRecipient(std::weak_ptr<DataObsMgrClient> owner) : owner_(owner) {}
        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            auto serviceClient = owner_.lock();
            if (serviceClient != nullptr && isFirst) {
                isFirst = false;
                serviceClient->OnRemoteDied();
            }
        }

    private:
        bool isFirst = true;
        std::weak_ptr<DataObsMgrClient> owner_;
    };

    static std::mutex mutex_;
    sptr<IDataObsMgr> dataObsManger_;

    struct ObserverInfo {
        Uri uri;
        int32_t userId;
        bool isExtension = false;
        uint32_t firstCallerTokenID = 0;
        ObserverInfo(Uri uri, int32_t userId) : uri(uri), userId(userId) {};
    };

    struct Param {
        Param(const Uri &uri, bool isDescendants) : uri(uri), isDescendants(isDescendants){};
        Uri uri;
        bool isDescendants;
    };
    sptr<ServiceDeathRecipient> deathRecipient_;
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_DATAOBS_MGR_CLIENT_H
