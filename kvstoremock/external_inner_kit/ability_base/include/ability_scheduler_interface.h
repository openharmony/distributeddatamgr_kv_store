/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_SCHEDULER_INTERFACE_H
#define OHOS_ABILITY_RUNTIME_ABILITY_SCHEDULER_INTERFACE_H

#include <iremote_broker.h>
#include "lifecycle_state_info.h"
#include "pac_map.h"
#include "ui_extension_window_command.h"
#include "uri.h"
#include "want.h"

namespace OHOS {
namespace NativeRdb {
class AbsSharedResultSet;
class DataAbilityPredicates;
class ValuesBucket;
}
namespace AppExecFwk {
class DataAbilityResult;
class DataAbilityOperation;
}
namespace AAFwk {
using OHOS::AppExecFwk::PacMap;
class IDataAbilityObserver;
class SessionInfo;

/**
 * @class IAbilityScheduler
 * IAbilityScheduler is used to schedule ability kit lifecycle.
 */
class IAbilityScheduler : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.aafwk.AbilityScheduler");

    /*
     * ScheduleAbilityTransaction,  schedule ability to transform life state.
     *
     * @param Want, Special Want for service type's ability.
     * @param targetState, The lifecycle state to be transformed
     * @param sessionInfo, The session info
     */
    virtual bool ScheduleAbilityTransaction(const Want &want, const LifeCycleStateInfo &targetState,
        sptr<SessionInfo> sessionInfo = nullptr) = 0;

    /*
     * ScheduleShareData,  schedule ability to share data.
     *
     * @param uniqueId, Indicates the uniqueId returned after the ability is started.
     */
    virtual void ScheduleShareData(const int32_t &uniqueId) = 0;

    /*
     * SendResult, Send result to app when ability is terminated with result want.
     *
     * @param requestCode, the requestCode of the ability to start.
     * @param resultCode, the resultCode of the ability to terminate.
     * @param resultWant, the want of the ability to terminate.
     */
    virtual void SendResult(int requestCode, int resultCode, const Want &resultWant) = 0;

    /*
     * ScheduleConnectAbility,  schedule service ability to connect.
     *
     * @param Want, Special Want for service type's ability.
     */
    virtual void ScheduleConnectAbility(const Want &want) = 0;

    /*
     * ScheduleDisconnectAbility, schedule service ability to disconnect.
     */
    virtual void ScheduleDisconnectAbility(const Want &want) = 0;

    /*
     * ScheduleCommandAbility, schedule service ability to command.
     */
    virtual void ScheduleCommandAbility(const Want &want, bool restart, int startId) = 0;

    virtual void ScheduleCommandAbilityWindow(const Want &want, const sptr<SessionInfo> &sessionInfo,
        WindowCommand winCmd) = 0;

    /**
     * SchedulePrepareTerminateAbility, schedule ability to prepare terminate.
     */
    virtual bool SchedulePrepareTerminateAbility() = 0;

    /*
     * ScheduleSaveAbilityState, scheduling save ability state.
     */
    virtual void ScheduleSaveAbilityState() = 0;

    /*
     * ScheduleRestoreAbilityState, scheduling restore ability state.
     */
    virtual void ScheduleRestoreAbilityState(const PacMap &inState) = 0;

    /**
     * @brief Obtains the MIME types of files supported.
     *
     * @param uri Indicates the path of the files to obtain.
     * @param mimeTypeFilter Indicates the MIME types of the files to obtain. This parameter cannot be null.
     *
     * @return Returns the matched MIME types. If there is no match, null is returned.
     */
    virtual std::vector<std::string> GetFileTypes(const Uri &uri, const std::string &mimeTypeFilter) = 0;

    /**
     * @brief Opens a file in a specified remote path.
     *
     * @param uri Indicates the path of the file to open.
     * @param mode Indicates the file open mode, which can be "r" for read-only access, "w" for write-only access
     * (erasing whatever data is currently in the file), "wt" for write access that truncates any existing file,
     * "wa" for write-only access to append to any existing data, "rw" for read and write access on any existing data,
     *  or "rwt" for read and write access that truncates any existing file.
     *
     * @return Returns the file descriptor.
     */
    virtual int OpenFile(const Uri &uri, const std::string &mode) = 0;

    /**
     * @brief This is like openFile, open a file that need to be able to return sub-sections of files，often assets
     * inside of their .hap.
     *
     * @param uri Indicates the path of the file to open.
     * @param mode Indicates the file open mode, which can be "r" for read-only access, "w" for write-only access
     * (erasing whatever data is currently in the file), "wt" for write access that truncates any existing file,
     * "wa" for write-only access to append to any existing data, "rw" for read and write access on any existing
     * data, or "rwt" for read and write access that truncates any existing file.
     *
     * @return Returns the RawFileDescriptor object containing file descriptor.
     */
    virtual int OpenRawFile(const Uri &uri, const std::string &mode) = 0;

    /**
     * @brief Inserts a single data record into the database.
     *
     * @param uri Indicates the path of the data to operate.
     * @param value  Indicates the data record to insert. If this parameter is null, a blank row will be inserted.
     *
     * @return Returns the index of the inserted data record.
     */
    virtual int Insert(const Uri &uri, const NativeRdb::ValuesBucket &value) = 0;

    /**
     * @brief Updates data records in the database.
     *
     * @param uri Indicates the path of data to update.
     * @param value Indicates the data to update. This parameter can be null.
     * @param predicates Indicates filter criteria. You should define the processing logic when this parameter is null.
     *
     * @return Returns the number of data records updated.
     */
    virtual int Update(const Uri &uri, const NativeRdb::ValuesBucket &value,
        const NativeRdb::DataAbilityPredicates &predicates) = 0;

    /**
     * @brief Deletes one or more data records from the database.
     *
     * @param uri Indicates the path of the data to operate.
     * @param predicates Indicates filter criteria. You should define the processing logic when this parameter is null.
     *
     * @return Returns the number of data records deleted.
     */
    virtual int Delete(const Uri &uri, const NativeRdb::DataAbilityPredicates &predicates) = 0;

    /**
     * @brief Calls the method of the Data ability.
     *
     * @param uri Indicates the Data ability of the method to call.
     * @param method Indicates the method to call.
     * @param arg Indicates the parameter of the String type.
     * @param pacMap Defines a PacMap object for storing a series of values.
     *
     * @return Returns the call result.
     */
    virtual std::shared_ptr<AppExecFwk::PacMap> Call(
        const Uri &uri, const std::string &method, const std::string &arg, const AppExecFwk::PacMap &pacMap) = 0;

    /**
     * @brief Deletes one or more data records from the database.
     *
     * @param uri Indicates the path of data to query.
     * @param columns Indicates the columns to query. If this parameter is null, all columns are queried.
     * @param predicates Indicates filter criteria. You should define the processing logic when this parameter is null.
     *
     * @return Returns the query result.
     */
    virtual std::shared_ptr<NativeRdb::AbsSharedResultSet> Query(
        const Uri &uri, std::vector<std::string> &columns, const NativeRdb::DataAbilityPredicates &predicates) = 0;

    /**
     * @brief Obtains the MIME type matching the data specified by the URI of the Data ability. This method should be
     * implemented by a Data ability. Data abilities supports general data types, including text, HTML, and JPEG.
     *
     * @param uri Indicates the URI of the data.
     *
     * @return Returns the MIME type that matches the data specified by uri.
     */
    virtual std::string GetType(const Uri &uri) = 0;

    /**
     * @brief Reloads data in the database.
     *
     * @param uri Indicates the position where the data is to reload. This parameter is mandatory.
     * @param extras Indicates the PacMap object containing the additional parameters to be passed in this call. This
     * parameter can be null. If a custom Sequenceable object is put in the PacMap object and will be transferred across
     * processes, you must call BasePacMap.setClassLoader(ClassLoader) to set a class loader for the custom object.
     *
     * @return Returns true if the data is successfully reloaded; returns false otherwise.
     */
    virtual bool Reload(const Uri &uri, const PacMap &extras) = 0;

    /**
     * @brief Inserts multiple data records into the database.
     *
     * @param uri Indicates the path of the data to operate.
     * @param values Indicates the data records to insert.
     *
     * @return Returns the number of data records inserted.
     */
    virtual int BatchInsert(const Uri &uri, const std::vector<NativeRdb::ValuesBucket> &values) = 0;

    /**
     * @brief Registers an observer to DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Return true if success. otherwise return false.
     */
    virtual bool ScheduleRegisterObserver(const Uri &uri, const sptr<IDataAbilityObserver> &dataObserver) = 0;

    /**
     * @brief Deregisters an observer used for DataObsMgr specified by the given Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     * @param dataObserver, Indicates the IDataAbilityObserver object.
     *
     * @return Return true if success. otherwise return false.
     */
    virtual bool ScheduleUnregisterObserver(const Uri &uri, const sptr<IDataAbilityObserver> &dataObserver) = 0;

    /**
     * @brief Notifies the registered observers of a change to the data resource specified by Uri.
     *
     * @param uri, Indicates the path of the data to operate.
     *
     * @return Return true if success. otherwise return false.
     */
    virtual bool ScheduleNotifyChange(const Uri &uri) = 0;
    /**
     * @brief Converts the given uri that refer to the Data ability into a normalized URI. A normalized URI can be used
     * across devices, persisted, backed up, and restored. It can refer to the same item in the Data ability even if the
     * context has changed. If you implement URI normalization for a Data ability, you must also implement
     * denormalizeUri(ohos.utils.net.Uri) to enable URI denormalization. After this feature is enabled, URIs passed to
     * any method that is called on the Data ability must require normalization verification and denormalization. The
     * default implementation of this method returns null, indicating that this Data ability does not support URI
     * normalization.
     *
     * @param uri Indicates the Uri object to normalize.
     *
     * @return Returns the normalized Uri object if the Data ability supports URI normalization; returns null otherwise.
     */
    virtual Uri NormalizeUri(const Uri &uri) = 0;

    /**
     * @brief Converts the given normalized uri generated by normalizeUri(ohos.utils.net.Uri) into a denormalized one.
     * The default implementation of this method returns the original URI passed to it.
     *
     * @param uri uri Indicates the Uri object to denormalize.
     *
     * @return Returns the denormalized Uri object if the denormalization is successful; returns the original Uri passed
     * to this method if there is nothing to do; returns null if the data identified by the original Uri cannot be found
     * in the current environment.
     */
    virtual Uri DenormalizeUri(const Uri &uri) = 0;
    virtual std::vector<std::shared_ptr<AppExecFwk::DataAbilityResult>> ExecuteBatch(
        const std::vector<std::shared_ptr<AppExecFwk::DataAbilityOperation>> &operations) = 0;
    virtual void ContinueAbility(const std::string& deviceId, uint32_t versionCode) = 0;
    virtual void NotifyContinuationResult(int32_t result) = 0;

    virtual void DumpAbilityInfo(const std::vector<std::string> &params, std::vector<std::string> &info) = 0;
    virtual int CreateModalUIExtension(const Want &want) = 0;

    virtual void OnExecuteIntent(const Want &want) = 0;

    /**
     * @brief Called when the UIAbility has been started by call.
     */
    virtual void CallRequest() = 0;

    /**
     * @brief Update sessionToken.
     * @param sessionToken The token of session.
     */
    virtual void UpdateSessionToken(sptr<IRemoteObject> sessionToken) = 0;

    virtual void ScheduleCollaborate(const Want &want) = 0;

    virtual void ScheduleAbilityRequestFailure(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message, int32_t resultCode = 0) = 0;

    virtual void ScheduleAbilityRequestSuccess(const std::string &requestId,
        const AppExecFwk::ElementName &element) = 0;

    virtual void ScheduleAbilitiesRequestDone(const std::string &requestKey,
        int32_t resultCode) = 0;

    /**
     * @brief Called when a skill needs to be executed on an already-started ability.
     * @param want The Want containing skill execution parameters.
     */
    virtual void ExecuteSkill(const Want &want) {}

    enum {
        // ipc id for scheduling ability to a state of life cycle
        SCHEDULE_ABILITY_TRANSACTION = 0,

        // ipc id for sending result to caller
        SEND_RESULT,

        // ipc id for scheduling service ability to connect
        SCHEDULE_ABILITY_CONNECT,

        // ipc id for scheduling service ability to disconnect
        SCHEDULE_ABILITY_DISCONNECT,

        // ipc id for scheduling service ability to command
        SCHEDULE_ABILITY_COMMAND,

        SCHEDULE_ABILITY_COMMAND_WINDOW,

        // ipc id for scheduling save ability state
        SCHEDULE_SAVE_ABILITY_STATE,

        // ipc id for scheduling restore ability state
        SCHEDULE_RESTORE_ABILITY_STATE,

        // ipc id for scheduling getFileTypes
        SCHEDULE_GETFILETYPES,

        // ipc id for scheduling openFile
        SCHEDULE_OPENFILE,

        // ipc id for scheduling openRawFile
        SCHEDULE_OPENRAWFILE,

        // ipc id for scheduling insert
        SCHEDULE_INSERT,

        // ipc id for scheduling update
        SCHEDULE_UPDATE,

        // ipc id for scheduling delete
        SCHEDULE_DELETE,

        // ipc id for scheduling query
        SCHEDULE_QUERY,

        // ipc id for scheduling getType
        SCHEDULE_GETTYPE,

        // ipc id for scheduling Reload
        SCHEDULE_RELOAD,

        // ipc id for scheduling BatchInsert​
        SCHEDULE_BATCHINSERT,

        // ipc id for dataAbilityObServer Register
        SCHEDULE_REGISTEROBSERVER,

        // ipc id for dataAbilityObServer UnReguster
        SCHEDULE_UNREGISTEROBSERVER,

        // ipc id for dataAbilityObServer change
        SCHEDULE_NOTIFYCHANGE,

        // ipc id for scheduling NormalizeUri
        SCHEDULE_NORMALIZEURI,

        // ipc id for scheduling DenormalizeUri
        SCHEDULE_DENORMALIZEURI,

        // ipc id for scheduling ExecuteBatch
        SCHEDULE_EXECUTEBATCH,

        // ipc id for notify continuation result
        NOTIFY_CONTINUATION_RESULT,

        // ipc id for scheduling call request
        REQUEST_CALL_REMOTE,

        // ipc id for continue ability
        CONTINUE_ABILITY,

        // ipc id for dump ability runner
        DUMP_ABILITY_RUNNER_INNER,

        SCHEDULE_CALL,

        SCHEDULE_SHARE_DATA,

        // ipc id for scheduling service ability to prepare terminate (30)
        SCHEDULE_ABILITY_PREPARE_TERMINATE,

        SCHEDULE_ONEXECUTE_INTENT,

        SCHEDULE_EXECUTE_SKILL,

        CREATE_MODAL_UI_EXTENSION,

        UPDATE_SESSION_TOKEN,

        SCHEDULE_COLLABORATE_DATA,

        SCHEDULE_ABILITY_REQUEST_FAILURE,

        SCHEDULE_ABILITY_REQUEST_SUCCESS,

        SCHEDULE_ABILITIES_REQUEST_DONE,
    };
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_ABILITY_SCHEDULER_INTERFACE_H
