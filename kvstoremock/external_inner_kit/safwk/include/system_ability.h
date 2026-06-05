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

#ifndef SYSTEM_ABILITY_H
#define SYSTEM_ABILITY_H

#include <string>
#include <unordered_map>
#include <vector>

#include "iremote_object.h"
#include "refbase.h"
#include "system_ability_ondemand_reason.h"

class CSystemAbilityInnerService;
namespace OHOS {
#define REGISTER_SYSTEM_ABILITY_BY_ID(abilityClassName, systemAbilityId, runOnCreate) \
    const bool abilityClassName##_##RegisterResult = \
    SystemAbility::MakeAndRegisterAbility(new abilityClassName(systemAbilityId, runOnCreate));

#define DECLEAR_SYSTEM_ABILITY(className) \
public: \
virtual std::string GetClassName() override { \
return #className; \
}

#define DECLEAR_BASE_SYSTEM_ABILITY(className) \
public: \
virtual std::string GetClassName() = 0

#define DECLARE_SYSTEM_ABILITY(className) \
public: \
virtual std::string GetClassName() override { \
return #className; \
}

#define DECLARE_BASE_SYSTEM_ABILITY(className) \
public: \
virtual std::string GetClassName() = 0

enum class SystemAbilityState {
    NOT_LOADED = 0,
    ACTIVE,
    IDLE,
};
/**
 * @class SystemAbility.
 * SystemAbility provides a base class for each SA.
 */
class SystemAbility {
    DECLARE_BASE_SYSTEM_ABILITY(SystemAbility);

public:
    /**
     * MakeAndRegisterAbility, Register SA into the localAbilityManager.
     *
     * @param systemAbility, SA required to register.
     * @return Returns true on success.
     */
    static bool MakeAndRegisterAbility(SystemAbility* systemAbility);

    /**
     * AddSystemAbilityListener, Provide the function of SA monitoring and waiting for other SA.
     *
     * @param systemAbilityId, said that needs to be monitored.
     * @return Returns true on success.
     */
    bool AddSystemAbilityListener(int32_t systemAbilityId);

    /**
     * RemoveSystemAbilityListener, Remove the SA you are listening to.
     *
     * @param systemAbilityId, The listening said that needs to be removed.
     * @return Returns true on success.
     */
    bool RemoveSystemAbilityListener(int32_t systemAbilityId);

protected:
    /**
     * OnDump, Dump sa.
     *
     * @return Returns void.
     */
    virtual void OnDump();

    /**
     * OnStart, The user needs to override onstart, initialize and publish SA.
     *
     * @return Returns void.
     */
    virtual void OnStart();

    /**
     * OnStart, The user needs to override onstart, initialize and publish SA.
     *
     * @param startReason, The reason for start SA.
     * @return Returns void.
     */
    virtual void OnStart(const SystemAbilityOnDemandReason& startReason);

    /**
     * OnIdle, The user needs to override OnIdle, OnIdle is a callback function when uninstalling SA.
     *
     * @param idleReason, The reason for unload SA.
     * @return Returns void.
     */
    virtual int32_t OnIdle(const SystemAbilityOnDemandReason& idleReason);

    /**
     * OnActive, The user needs to override OnActive, OnActive is a callback function when CancelIdle.
     *
     * @param activeReason, The reason for active SA.
     * @return Returns void.
     */
    virtual void OnActive(const SystemAbilityOnDemandReason& activeReason);

    /**
     * OnStop, The user needs to override OnStop, Onstop is called when the SA is uninstalled.
     *
     * @return Returns void.
     */
    virtual void OnStop();

    /**
     * OnStop, The user needs to override OnStop, Onstop is called when the SA is uninstalled.
     *
     * @param stopReason, The reason for stop SA.
     * @return Returns void.
     */
    virtual void OnStop(const SystemAbilityOnDemandReason& stopReason);

    /**
     * OnAddSystemAbility, OnAddSystemAbility will be called when the listening SA starts.
     *
     * @param systemAbilityId, The said being listened to.
     * @param deviceId, deviceId is empty.
     * @return void.
     */
    virtual void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId);

    /**
     * OnRemoveSystemAbility, OnRemoveSystemAbility will be called when the listening SA Uninstall.
     *
     * @param systemAbilityId, The said being listened to.
     * @param deviceId, deviceId is empty.
     * @return void.
     */
    virtual void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId);

    /**
     * OnSvcCmd, OnSvcCmd will be called when svc cmd is send
     *
     * @param fd, used to dump execution information.
     * @param args, command args.
     * @return int32_t, error code.
     */
    virtual int32_t OnSvcCmd(int32_t fd, const std::vector<std::u16string>& args);

    /**
     * GetSystemAbility, get systemability.
     *
     * @param systemAbilityId, The said number that needs to be obtained.
     * @return nullptr indicates that sa does not exist.
     */
    sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId);

    /**
     * Publish, Register functions for sa.
     *
     * @param systemAbility, SA that need to be registered.
     * @return True indicates successful registration.
     */
    bool Publish(sptr<IRemoteObject> systemAbility);

    /**
     * OnStartFail, Notifying the samgr of an onstart failure.
     *
     * @param errCode, OnStart exception reason.
     * @return True indicates Notify succeeded.
     */
    bool OnStartFail(int32_t errCode);

    /**
     * CancelIdle, Cancel Uninstall, Calling the function from idle to active.
     *
     * @return True indicates Cancel succeeded.
     */
    bool CancelIdle();

    /**
     * StopAbility, Remove sa from samgr.
     *
     * @param systemAbilityId, Said to be deleted.
     * @return void.
     */
    static void StopAbility(int32_t systemAbilityId);

    SystemAbility(bool runOnCreate = false);
    SystemAbility(int32_t systemAbilityId, bool runOnCreate = false);
    virtual ~SystemAbility();

    /**
     * GetAbilityState, Obtain status information of sa.
     *
     * @return status information of sa.
     */
    SystemAbilityState GetAbilityState();

    /**
     * OnDeviceLevelChanged, OnDeviceLevelChanged will be called when listen strategy send.
     *
     * @param type, type is a certain device status type.
     * @param level, level is level of a certain device status type.
     * @param action, action is scheduling strategy.
     * @return void.
     */
    virtual void OnDeviceLevelChanged(int32_t type, int32_t level, std::string& action);

    /**
     * OnExtension, OnExtension will be called when extension is send
     *
     * @param extension, the system ability extension name.
     * @param data, extension data.
     * @param reply, extension reply.
     * @return int32_t, error code.
     */
    virtual int32_t OnExtension(const std::string& extension, MessageParcel& data, MessageParcel& reply);

private:
    void Start();
    void Idle(SystemAbilityOnDemandReason& idleReason, int32_t& delayTime);
    void Active(SystemAbilityOnDemandReason& activeReason);
    void Stop();
    void SADump();
    int32_t GetSystemAbilitId() const;
    void SetLibPath(const std::string& libPath);
    const std::string& GetLibPath() const;
    void SetDependSa(const std::vector<int32_t>& dependSa);
    const std::vector<int32_t>& GetDependSa() const;
    void SetRunOnCreate(bool isRunOnCreate);
    bool IsRunOnCreate() const;
    void SetDistributed(bool isDistributed);
    bool GetDistributed() const;
    void SetDumpLevel(unsigned int dumpLevel);
    unsigned int GetDumpLevel() const;
    void SetDependTimeout(int dependTimeout);
    int GetDependTimeout() const;
    bool GetRunningStatus() const;
    void SetCapability(const std::u16string& capability);
    const std::u16string& GetCapability() const;
    void SetPermission(const std::u16string& defPerm);
    void GetOnDemandReasonExtraData(SystemAbilityOnDemandReason& onDemandStartReason);
    sptr<IRemoteObject> GetAbilityRemoteObject();

    friend class LocalAbilityManager;
    friend class ::CSystemAbilityInnerService;
    
private:
    int32_t saId_ = 0;
    std::string libPath_;
    std::vector<int32_t> dependSa_;
    bool isRunOnCreate_;
    bool isDistributed_;
    unsigned int dumpLevel_;
    int dependTimeout_;
    bool isRunning_;
    SystemAbilityState abilityState_;
    std::u16string capability_;
    sptr<IRemoteObject> publishObj_;
    std::u16string permission_;
    std::recursive_mutex abilityLock;
    std::mutex onStartLock_;
};
}

#endif
