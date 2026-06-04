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


#ifndef INTERFACES_INNERKITS_SAMGR_INCLUDE_SYSTEM_ABILITY_MANAGER_PROXY_H
#define INTERFACES_INNERKITS_SAMGR_INCLUDE_SYSTEM_ABILITY_MANAGER_PROXY_H

#include <string>
#include <set>
#include "dynamic_cache.h"
#include "if_system_ability_manager.h"
#include "system_ability_on_demand_event.h"
#include "system_ability_load_callback_stub.h"

namespace OHOS {
class SystemAbilityManagerProxy :
    public DynamicCache, public IRemoteProxy<ISystemAbilityManager> {
public:
    explicit SystemAbilityManagerProxy(const sptr<IRemoteObject>& impl)
        : IRemoteProxy<ISystemAbilityManager>(impl) {}
    ~SystemAbilityManagerProxy() = default;
    /**
     * ListSystemAbilities, Return list of all existing abilities.
     *
     * @param dumpFlags, dump all
     * @return Returns the sa where the current samgr exists.
     */
    std::vector<std::u16string> ListSystemAbilities(unsigned int dumpFlags) override;

    /**
     * GetSystemAbility, Retrieve an existing ability, retrying and blocking for a few seconds if it doesn't exist.
     *
     * @param systemAbilityId, Need to obtain the said of sa.
     * @return nullptr indicates acquisition failure.
     */
    sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId) override;

    /**
     * CheckSystemAbility, Retrieve an existing ability, no-blocking.
     *
     * @param systemAbilityId, Need to obtain the said of sa.
     * @return nullptr indicates acquisition failure.
     */
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) override;

    /**
     * RemoveSystemAbility, Remove an ability.
     *
     * @param systemAbilityId, Need to remove the said of sa.
     * @return ERR_OK indicates remove success.
     */
    int32_t RemoveSystemAbility(int32_t systemAbilityId) override;

    /**
     * SubscribeSystemAbility, Subscribe a system ability status, and inherit from ISystemAbilityStatusChange class.
     *
     * @param systemAbilityId, Need to subscribe the said of sa.
     * @param listener, Need to implement OnAddSystemAbility, OnRemoveSystemAbility.
     * @return ERR_OK indicates SubscribeSystemAbility success.
     */
    int32_t SubscribeSystemAbility(int32_t systemAbilityId, const sptr<ISystemAbilityStatusChange>& listener) override;

    /**
     * UnSubscribeSystemAbility, UnSubscribe a system ability status, and inherit from ISystemAbilityStatusChange class.
     *
     * @param systemAbilityId, Need to UnSubscribe the said of sa.
     * @param listener, Need to implement OnAddSystemAbility, OnRemoveSystemAbility.
     * @return ERR_OK indicates SubscribeSystemAbility success.
     */
    int32_t UnSubscribeSystemAbility(int32_t systemAbilityId,
        const sptr<ISystemAbilityStatusChange> &listener) override;

    /**
     * GetSystemAbility, Retrieve an existing ability, blocking for a few seconds if it doesn't exist.
     *
     * @param systemAbilityId, Need to get the said of sa.
     * @param deviceId, If the device id is empty, it indicates that it is a local get.
     * @return nullptr indicates acquisition failure.
     */
    sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;

    /**
     * CheckSystemAbility, Retrieve an existing ability, no-blocking.
     *
     * @param systemAbilityId, Need to get the said of sa.
     * @param deviceId, If the device id is empty, it indicates that it is a local get.
     * @return nullptr indicates acquisition failure.
     */
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;

    /**
     * AddOnDemandSystemAbilityInfo, Add ondemand ability info.
     *
     * @param systemAbilityId, Need to add info the said of sa.
     * @param localAbilityManagerName, Process Name.
     * @return ERR_OK indicates AddOnDemandSystemAbilityInfo success.
     */
    int32_t AddOnDemandSystemAbilityInfo(int32_t systemAbilityId,
        const std::u16string& localAbilityManagerName) override;

    /**
     * CheckSystemAbility, Retrieve an ability, no-blocking.
     *
     * @param systemAbilityId, Need to check the said of sa.
     * @param isExist, Issue parameters, and a result of true indicates success.
     * @return nullptr indicates acquisition failure.
     */
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId, bool& isExist) override;

    /**
     * AddSystemAbility, add an ability to samgr
     *
     * @param systemAbilityId, Need to add the said of sa.
     * @param ability, SA to be added.
     * @param extraProp, Additional parameters for sa, such as whether it is distributed.
     * @return ERR_OK indicates successful add.
     */
    int32_t AddSystemAbility(int32_t systemAbilityId, const sptr<IRemoteObject>& ability,
        const SAExtraProp& extraProp) override;

    /**
     * AddSystemProcess, add an process.
     *
     * @param procName, Need to add the procName of process.
     * @param procObject, Remoteobject of procName.
     * @return ERR_OK indicates successful add.
     */
    int32_t AddSystemProcess(const std::u16string& procName, const sptr<IRemoteObject>& procObject) override;

    /**
     * LoadSystemAbility, Load sa.
     *
     * @param systemAbilityId, Need to load the said of sa.
     * @param timeout, OnLoadSystemAbilityFail and OnLoadSystemAbilitySuccess need be rewritten.
     * @return return is not nullptr means that the load was successful.
     */
    sptr<IRemoteObject> LoadSystemAbility(int32_t systemAbilityId, int32_t timeout) override;

    /**
     * LoadSystemAbility, Load sa.
     *
     * @param systemAbilityId, Need to load the said of sa.
     * @param callback, OnLoadSystemAbilityFail and OnLoadSystemAbilitySuccess need be rewritten.
     * @return ERR_OK It does not mean that the load was successful, but a callback function is.
     required to confirm whether it was successful.
     */
    int32_t LoadSystemAbility(int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback>& callback) override;

    /**
     * LoadSystemAbility, Load sa.
     *
     * @param systemAbilityId, Need to load the said of sa.
     * @param deviceId, if deviceId is empty, it indicates local load.
     * @param callback, OnLoadSystemAbilityFail and OnLoadSystemAbilitySuccess need be rewritten.
     * @return ERR_OK It does not mean that the load was successful
     */
    int32_t LoadSystemAbility(int32_t systemAbilityId, const std::string& deviceId,
        const sptr<ISystemAbilityLoadCallback>& callback) override;

    /**
     * UnloadSystemAbility, UnLoad sa.
     *
     * @param systemAbilityId, Need to UnLoad the said of sa.
     * @return ERR_OK It does not mean that the unload was successful.
     */
    int32_t UnloadSystemAbility(int32_t systemAbilityId) override;

    /**
     * CancelUnloadSystemAbility, CancelUnload sa.
     *
     * @param systemAbilityId, Need to CancelUnload the said of sa.
     * @return ERR_OK indicates that the uninstall was canceled successfully.
     */
    int32_t CancelUnloadSystemAbility(int32_t systemAbilityId) override;

    /**
     * UnloadAllIdleSystemAbility, unload all idle sa.
     * only support for memmgrservice
     *
     * @return ERR_OK It means unload all idle sa success.
     */
    int32_t UnloadAllIdleSystemAbility() override;

    /**
     * UnloadProcess, unload process by process name list.
     * only support for memmgrservice
     *
     * @return ERR_OK It means unload all process in list.
     */
    virtual int32_t UnloadProcess(const std::vector<std::u16string>& processList) override;

    /**
     * GetSystemProcessInfo, Get process info by said.
     * only support for memmgrservice
     *
     * @param processList, Issue a parameter and return it as a result.
     * @return ERR_OK indicates that the get successfully.
     */
    virtual int32_t GetLruIdleSystemAbilityProc(std::vector<IdleProcessInfo>& processInfos) override;

    /**
     * OnStartSystemAbilityFail, Notify samgr OnStart fail.
     *
     * @param systemAbilityId, Need the said of sa which wants to notify samgr.
     * @param errCode, OnStart error code.
     * @return ERR_OK indicates that the notify successfully.
     */
    virtual int32_t OnStartSystemAbilityFail(int32_t systemAbilityId, int32_t errCode) override;

    /**
     * GetSystemProcessInfo, Get process info by said.
     *
     * @param systemAbilityId, Need the said of sa which wants to get process info.
     * @param systemProcessInfo, Issue a parameter and return it as a result.
     * @return ERR_OK indicates that the get successfully.
     */
    int32_t GetSystemProcessInfo(int32_t systemAbilityId, SystemProcessInfo& systemProcessInfo) override;

    /**
     * GetRunningSystemProcess, Get all processes currently running.
     *
     * @param systemProcessInfos, Issue a parameter and return it as a result.
     * @return ERR_OK indicates that the get successfully.
     */
    int32_t GetRunningSystemProcess(std::list<SystemProcessInfo>& systemProcessInfos) override;

    /**
     * SubscribeSystemProcess, Subscribe the status of process.
     *
     * @param listener, callback.
     * @return ERR_OK indicates that the Subscribe successfully.
     */
    int32_t SubscribeSystemProcess(const sptr<ISystemProcessStatusChange>& listener) override;

    /**
     * SubscribeLowMemSystemProcess, Subscribe the status of process.
     *
     * @param listener, callback.
     * @return ERR_OK indicates that the Subscribe successfully.
     */
    int32_t SubscribeLowMemSystemProcess(const sptr<ISystemProcessStatusChange>& listener) override;

    /**
     * UnSubscribeLowMemSystemProcess, UnSubscribe the status of low-mem process.
     *
     * @param listener, callback.
     * @return ERR_OK indicates that the UnSubscribe successfully.
     */
    int32_t UnSubscribeLowMemSystemProcess(const sptr<ISystemProcessStatusChange>& listener) override;

    /**
     * SendStrategy, Send strategy to SA.
     *
     * @param type, type is a certain device status type.
     * @param systemAbilityIds, Need the vector of said which wants to send strategy.
     * @param level, level is level of a certain device status type.
     * @param action, action is scheduling strategy.
     * @return ERR_OK indicates that the Subscribe successfully.
     */
    int32_t SendStrategy(int32_t type, std::vector<int32_t>& systemAbilityIds,
        int32_t level, std::string& action) override;

    /**
     * UnSubscribeSystemProcess, UnSubscribe the status of process.
     *
     * @param listener, callback.
     * @return ERR_OK indicates that the UnSubscribe successfully.
     */
    int32_t UnSubscribeSystemProcess(const sptr<ISystemProcessStatusChange>& listener) override;

    /**
     * GetExtensionSaIds, Return list of saId that match extension.
     *
     * @param extension, extension, match with profile extension.
     * @param saIds, list of saId that match extension
     * @return ERR_OK indicates that the list of saId that match extension success.
     */
    int32_t GetExtensionSaIds(const std::string& extension, std::vector<int32_t> &saIds) override;

    /**
     * GetExtensionRunningSaList, Return started list of hanlde that match extension.
     *
     * @param extension, extension, match with profile extension.
     * @param saList, started list of remote obj that match extension
     * @return ERR_OK indicates that the list of hanlde that match extension success.
     */
    int32_t GetExtensionRunningSaList(const std::string& extension, std::vector<sptr<IRemoteObject>>& saList) override;

    /**
     * GetLocalAbilityManagerProxy, Return local ability manager proxy.
     *
     * @param systemAbilityId, need to obtain the said of sa.
     * @return nullptr indicates acquistion failure.
     */
    sptr<IRemoteObject> GetLocalAbilityManagerProxy(int32_t systemAbilityId) override;

    int32_t GetRunningSaExtensionInfoList(const std::string& extension,
        std::vector<SaExtensionInfo>& infoList) override;
    int32_t GetCommonEventExtraDataIdlist(int32_t saId, std::vector<int64_t>& extraDataIdList,
        const std::string& eventName = "") override;
    int32_t GetOnDemandReasonExtraData(int64_t extraDataId, MessageParcel& extraDataParcel) override;
    int32_t GetOnDemandPolicy(int32_t systemAbilityId, OnDemandPolicyType type,
        std::vector<SystemAbilityOnDemandEvent>& abilityOnDemandEvents) override;
    int32_t UpdateOnDemandPolicy(int32_t systemAbilityId, OnDemandPolicyType type,
        const std::vector<SystemAbilityOnDemandEvent>& sabilityOnDemandEvents) override;
    sptr<IRemoteObject> Recompute(int32_t systemAbilityId, int32_t code) override;
    int32_t GetOnDemandSystemAbilityIds(std::vector<int32_t>& systemAbilityIds) override;
    int32_t SubscribeSystemAbilityInImage(int32_t systemAbilityId,
        const sptr<ISystemAbilityStatusChange>& listener) override;
    int32_t OnUserStateChanged(int32_t userId, SamgrUserState userState) override;
    int32_t SetSamgrIpcPrior(bool enable) override;
private:
    sptr<IRemoteObject> GetSystemAbilityWrapper(int32_t systemAbilityId, const std::string& deviceId = "");
    sptr<IRemoteObject> CheckSystemAbilityWrapper(int32_t code, MessageParcel& data);
    sptr<IRemoteObject> CheckSystemAbilityWrapper(int32_t code, MessageParcel& data, int32_t& errCode);
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId, const std::string& deviceId, int32_t& errCode);
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId, bool& isExist, int32_t& errCode);
    int32_t MarshalSAExtraProp(const SAExtraProp& extraProp, MessageParcel& data) const;
    int32_t AddSystemAbilityWrapper(int32_t code, MessageParcel& data);
    int32_t RemoveSystemAbilityWrapper(int32_t code, MessageParcel& data);
    int32_t ReadSystemProcessFromParcel(MessageParcel& reply, std::list<SystemProcessInfo>& systemProcessInfos);
    int32_t ReadProcessInfoFromParcel(MessageParcel& reply, SystemProcessInfo& systemProcessInfo);
    int32_t ReadIdleProcessInfoFromParcel(MessageParcel& reply, std::vector<IdleProcessInfo>& procInfos);
    sptr<IRemoteObject> CheckSystemAbilityTransaction(int32_t systemAbilityId);
    bool IsOnDemandSystemAbility(int32_t systemAbilityId);
    int32_t ListExtensionSendReq(const std::string& extension,
        SamgrInterfaceCode cmd, MessageParcel& reply, MessageOption& option);
    int32_t SubscribeSystemAbilityInner(int32_t systemAbilityId,
        const sptr<ISystemAbilityStatusChange>& listener, MessageOption& option);
private:
    static inline BrokerDelegator<SystemAbilityManagerProxy> delegator_;
    std::set<int32_t> onDemandSystemAbilityIdsSet_;
    std::mutex onDemandSaLock_;
};

class SystemAbilityProxyCallback : public SystemAbilityLoadCallbackStub {
public:
    void OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
        const sptr<IRemoteObject> &remoteObject) override;
    void OnLoadSystemAbilityFail(int32_t systemAbilityId) override;
    std::mutex callbackLock_;
    std::condition_variable cv_;
    sptr<IRemoteObject> loadproxy_;
};
} // namespace OHOS

#endif // !defined(INTERFACES_INNERKITS_SAMGR_INCLUDE_SYSTEM_ABILITY_MANAGER_PROXY_H)
