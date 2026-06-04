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

#ifndef OHOS_ABILITY_RUNTIME_DATAOBS_MGR_INTERFACE_H
#define OHOS_ABILITY_RUNTIME_DATAOBS_MGR_INTERFACE_H

#include <vector>

#include <ipc_types.h>
#include <iremote_broker.h>

#include "data_ability_observer_interface.h"
#include "dataobs_mgr_errors.h"
#include "uri.h"

namespace OHOS {
namespace AAFwk {
using Uri = OHOS::Uri;
constexpr const char* DATAOBS_MANAGER_SERVICE_NAME = "DataObsMgrService";

struct DataObsOption {
private:
    bool isSystem = false;
    uint32_t firstCallerTokenID = 0;
    int32_t firstCallerPid = 0;
    bool isDataShare = false;
    // fullTokenID's high 32 bits for system permission check purpose
    uint64_t firstCallerFullTokenID_ = 0;
public:
    DataObsOption() {}
    DataObsOption(bool isSystem):isSystem(isSystem) {}
    DataObsOption(bool isSystem, bool isDataShare):isSystem(isSystem), isDataShare(isDataShare) {}
    bool IsSystem()
    {
        return isSystem;
    }
    uint32_t FirstCallerTokenID()
    {
        return firstCallerTokenID;
    }
    void SetFirstCallerTokenID(uint32_t token)
    {
        firstCallerTokenID = token;
    }
    int32_t FirstCallerPid()
    {
        return firstCallerPid;
    }
    void SetFirstCallerPid(int32_t pid)
    {
        firstCallerPid = pid;
    }
    bool IsDataShare()
    {
        return isDataShare;
    }
    void SetDataShare(bool flag)
    {
        isDataShare = flag;
    }
    void SetFirstCallerFullTokenID(uint64_t fulltoken)
    {
        firstCallerFullTokenID_ = fulltoken;
    }
    uint64_t FirstCallerFullTokenID()
    {
        return firstCallerFullTokenID_;
    }
};

/**
 * @class IDataObsMgr
 * IDataObsMgr interface is used to access dataobs manager services.
 */
class IDataObsMgr : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.aafwk.DataObsMgr")

    enum {
        TRANS_HEAD,
        REGISTER_OBSERVER = TRANS_HEAD,
        UNREGISTER_OBSERVER,
        NOTIFY_CHANGE,
        REGISTER_OBSERVER_EXT,
        UNREGISTER_OBSERVER_EXT,
        UNREGISTER_OBSERVER_ALL_EXT,
        NOTIFY_CHANGE_EXT,
        NOTIFY_PROCESS,
        REGISTER_OBSERVER_FROM_EXTENSION,
        NOTIFY_CHANGE_FROM_EXTENSION,
        TRANS_BUTT,
    };

    static constexpr int DATAOBS_DEFAULT_CURRENT_USER = -1;

    /**
     * Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int RegisterObserver(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
        int32_t userId = DATAOBS_DEFAULT_CURRENT_USER,
        DataObsOption opt = DataObsOption()) = 0;

    /**
     * Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int RegisterObserverFromExtension(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
        int32_t userId = DATAOBS_DEFAULT_CURRENT_USER, DataObsOption opt = DataObsOption()) = 0;

    /**
     * Deregisters an observer used for DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int UnregisterObserver(const Uri &uri,
        sptr<IDataAbilityObserver> dataObserver, int32_t userId = DATAOBS_DEFAULT_CURRENT_USER,
        DataObsOption opt = DataObsOption()) = 0;

    /**
     * Notifies the registered observers of a change to the data resource specified by Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int NotifyChange(const Uri &uri, int32_t userId = DATAOBS_DEFAULT_CURRENT_USER,
        DataObsOption opt = DataObsOption()) = 0;

    /**
     * Notifies the registered observers of a change to the data resource specified by Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int NotifyChangeFromExtension(const Uri &uri, int32_t userId = DATAOBS_DEFAULT_CURRENT_USER,
        DataObsOption opt = DataObsOption()) = 0;

    /**
     * Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     * @param isDescendants, Indicates the Whether to note the change of descendants.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    virtual Status RegisterObserverExt(const Uri &uri, sptr<IDataAbilityObserver> dataObserver, bool isDescendants,
        DataObsOption opt = DataObsOption()) = 0;

    /**
     * Deregisters an observer used for DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    virtual Status UnregisterObserverExt(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
        DataObsOption opt = DataObsOption()) = 0;

    /**
     * Deregisters dataObserver used for DataObsMgr specified
     *
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    virtual Status UnregisterObserverExt(sptr<IDataAbilityObserver> dataObserver,
        DataObsOption opt = DataObsOption()) = 0;

    /**
     * Notifies the registered observers of a change to the data resource specified by Uris.
     *
     * @param changeInfo Indicates the info of the data to operate.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    virtual Status NotifyChangeExt(const ChangeInfo &changeInfo, DataObsOption opt = DataObsOption()) = 0;

    /**
     * Notifies the process observer with the given progress key and cancel observer.
     *
     * @param key Identifies the progress of a specific task.

     * @param observer Observer for monitoring the ongoing process.
     *
     * @return Returns SUCCESS on success, others on failure.
     */
    virtual Status NotifyProcessObserver(const std::string &key, const sptr<IRemoteObject> &observer,
        DataObsOption opt = DataObsOption()) = 0;
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_DATAOBS_MGR_INTERFACE_H
