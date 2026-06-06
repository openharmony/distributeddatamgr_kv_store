/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_IPC_INTERFACE_CODE_H
#define OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_IPC_INTERFACE_CODE_H

/* SAID:180 */
namespace OHOS {
namespace AAFwk {
enum class AbilityManagerInterfaceCode {
    // ipc id 1-1000 for kit
    // ipc id for terminating ability (1)
    TERMINATE_ABILITY = 1,

    // ipc id for attaching ability thread (2)
    ATTACH_ABILITY_THREAD = 2,

    // ipc id for ability transition done (3)
    ABILITY_TRANSITION_DONE = 3,

    // ipc id for connecting ability done (4)
    CONNECT_ABILITY_DONE = 4,

    // ipc id for disconnecting ability done (5)
    DISCONNECT_ABILITY_DONE = 5,

    // ipc id for add window token (6)
    ADD_WINDOW_INFO = 6,

    // ipc id for list stack info (8)
    LIST_STACK_INFO = 8,

    // ipc id for get recent mission (9)
    GET_RECENT_MISSION = 9,

    // ipc id for removing mission (10)
    REMOVE_MISSION = 10,

    // ipc id for removing mission (11)
    REMOVE_STACK = 11,

    // ipc id for removing mission (12)
    COMMAND_ABILITY_DONE = 12,

    // ipc id for get mission snapshot (13)
    GET_MISSION_SNAPSHOT = 13,

    // ipc id for acquire data ability (14)
    ACQUIRE_DATA_ABILITY = 14,

    // ipc id for release data ability (15)
    RELEASE_DATA_ABILITY = 15,

    // ipc id for move mission to top (16)
    MOVE_MISSION_TO_TOP = 16,

    // ipc id for kill process (17)
    KILL_PROCESS = 17,

    // ipc id for uninstall app (18)
    UNINSTALL_APP = 18,

    // ipc id for move mission to floating stack (20)
    MOVE_MISSION_TO_FLOATING_STACK = 20,

    // ipc id for move mission to floating stack (21)
    MOVE_MISSION_TO_SPLITSCREEN_STACK = 21,

    // ipc id for change focus ability (22)
    CHANGE_FOCUS_ABILITY = 22,

    // ipc id for Minimize MultiWindow (23)
    MINIMIZE_MULTI_WINDOW = 23,

    // ipc id for Maximize MultiWindow (24)
    MAXIMIZE_MULTI_WINDOW = 24,

    // ipc id for get floating missions (25)
    GET_FLOATING_MISSIONS = 25,

    // ipc id for get floating missions (26)
    CLOSE_MULTI_WINDOW = 26,

    // ipc id for move mission to end (27)
    MOVE_MISSION_TO_END = 27,

    // ipc id for compel verify permission (28)
    COMPEL_VERIFY_PERMISSION = 28,

    // ipc id for power off (29)
    POWER_OFF = 29,

    // ipc id for power off (30)
    POWER_ON = 30,

    // ipc id for luck mission (31)
    LUCK_MISSION = 31,

    // ipc id for unluck mission (32)
    UNLUCK_MISSION = 32,

    // ipc id for set mission info (33)
    SET_MISSION_INFO = 33,

    // ipc id for get mission lock mode state (34)
    GET_MISSION_LOCK_MODE_STATE = 34,

    // ipc id for minimize ability (35)
    MINIMIZE_ABILITY = 35,

    // ipc id for lock mission for cleanup operation (36)
    LOCK_MISSION_FOR_CLEANUP = 36,

    // ipc id for unlock mission for cleanup operation (37)
    UNLOCK_MISSION_FOR_CLEANUP = 37,

    // ipc id for register mission listener (38)
    REGISTER_MISSION_LISTENER = 38,

    // ipc id for unregister mission listener (39)
    UNREGISTER_MISSION_LISTENER = 39,

    // ipc id for get mission infos (40)
    GET_MISSION_INFOS = 40,

    // ipc id for get mission info by id (41)
    GET_MISSION_INFO_BY_ID = 41,

    // ipc id for clean mission (42)
    CLEAN_MISSION = 42,

    // ipc id for clean all missions (43)
    CLEAN_ALL_MISSIONS = 43,

    // ipc id for move mission to front (44)
    MOVE_MISSION_TO_FRONT = 44,

    // ipc id for get mission snap shot (45)
    GET_MISSION_SNAPSHOT_BY_ID = 45,

    // ipc id for move mission to front (46)
    START_USER = 46,

    // ipc id for move mission to front (47)
    STOP_USER = 47,

    // ipc id for set ability controller (48)
    SET_ABILITY_CONTROLLER = 48,

    // ipc id for get stability test flag (49)
    IS_USER_A_STABILITY_TEST = 49,

    // ipc id for set mission label (50)
    SET_MISSION_LABEL = 50,

    // ipc id for ability foreground (51)
    DO_ABILITY_FOREGROUND = 51,

    // ipc id for ability background (52)
    DO_ABILITY_BACKGROUND = 52,

    // ipc id for move mission to front by options (53)
    MOVE_MISSION_TO_FRONT_BY_OPTIONS = 53,

    // ipc for get mission id by ability token (54)
    GET_MISSION_ID_BY_ABILITY_TOKEN = 54,

    // ipc id for set mission icon (55)
    SET_MISSION_ICON = 55,

    // dump ability info done (56)
    DUMP_ABILITY_INFO_DONE = 56,

    // start extension ability (57)
    START_EXTENSION_ABILITY = 57,

    // stop extension ability (58)
    STOP_EXTENSION_ABILITY = 58,

    // ipc id for set rootSceneSession (61)
    SET_ROOT_SCENE_SESSION = 61,

    // prepare terminate ability (62)
    PREPARE_TERMINATE_ABILITY = 62,

    COMMAND_ABILITY_WINDOW_DONE = 63,

    // prepare terminate ability (64)
    CALL_ABILITY_BY_SCB = 64,

    MOVE_ABILITY_TO_BACKGROUND = 65,

    // ipc id for set mission continue state (66)
    SET_MISSION_CONTINUE_STATE = 66,

    // ipc id for set session locked state (67)
    SET_SESSION_LOCKED_STATE = 67,

    // Register the app debug mode listener (68)
    REGISTER_APP_DEBUG_LISTENER = 68,

    // Cancel register the app debug mode listener (69)
    UNREGISTER_APP_DEBUG_LISTENER = 69,

    // Attach app debug (70)
    ATTACH_APP_DEBUG = 70,

    // Deatch app debug (71)
    DETACH_APP_DEBUG = 71,

    // Execute intent (72)
    EXECUTE_INTENT = 72,

    // execute insight intent done with result (73)
    EXECUTE_INSIGHT_INTENT_DONE = 73,

    // ipc id for logout user (74)
    LOGOUT_USER = 74,

    // Get forgeround UI abilities(75)
    GET_FOREGROUND_UI_ABILITIES = 75,

    // Pop-up launch of full-screen atomic service(77)
    OPEN_ATOMIC_SERVICE = 77,

    // Querying whether to allow embedded startup of atomic service.
    IS_EMBEDDED_OPEN_ALLOWED = 78,

    // Starts a new ability by shortcut.
    START_SHORTCUT = 79,

    // Set resident process enable status.
    SET_RESIDENT_PROCESS_ENABLE = 80,

    // ipc id for ability window config transition done (81)
    ABILITY_WINDOW_CONFIG_TRANSITION_DONE = 81,

    // Back to caller.
    BACK_TO_CALLER_UIABILITY = 82,

    // ipc id 1001-2000 for DMS
    // ipc id for starting ability (1001)
    START_ABILITY = 1001,

    // ipc id for connecting ability (1002)
    CONNECT_ABILITY = 1002,

    // ipc id for disconnecting ability (1003)
    DISCONNECT_ABILITY = 1003,

    // ipc id for disconnecting ability (1004)
    STOP_SERVICE_ABILITY = 1004,

    // ipc id for starting ability by caller(1005)
    START_ABILITY_ADD_CALLER = 1005,

    GET_PENDING_WANT_SENDER = 1006,

    SEND_PENDING_WANT_SENDER = 1007,

    CANCEL_PENDING_WANT_SENDER = 1008,

    GET_PENDING_WANT_UID = 1009,

    GET_PENDING_WANT_BUNDLENAME = 1010,

    GET_PENDING_WANT_USERID = 1011,

    GET_PENDING_WANT_TYPE = 1012,

    GET_PENDING_WANT_CODE = 1013,

    REGISTER_CANCEL_LISTENER = 1014,

    UNREGISTER_CANCEL_LISTENER = 1015,

    GET_PENDING_REQUEST_WANT = 1016,

    GET_PENDING_WANT_SENDER_INFO = 1017,
    SET_SHOW_ON_LOCK_SCREEN = 1018,

    SEND_APP_NOT_RESPONSE_PROCESS_ID = 1019,

    // ipc id for starting ability by settings(1020)
    START_ABILITY_FOR_SETTINGS = 1020,

    GET_ABILITY_MISSION_SNAPSHOT = 1021,

    GET_APP_MEMORY_SIZE = 1022,

    IS_RAM_CONSTRAINED_DEVICE = 1023,

    GET_ABILITY_RUNNING_INFO = 1024,

    GET_EXTENSION_RUNNING_INFO = 1025,

    GET_PROCESS_RUNNING_INFO = 1026,

    START_ABILITY_FOR_OPTIONS = 1028,

    // ipc id for call ability
    START_CALL_ABILITY = 1032,

    RELEASE_CALL_ABILITY = 1033,

    CONNECT_ABILITY_WITH_TYPE = 1034,

    // start ui extension ability
    START_UI_EXTENSION_ABILITY = 1035,

    CALL_REQUEST_DONE = 1036,

    START_ABILITY_AS_CALLER_BY_TOKEN = 1037,

    START_ABILITY_AS_CALLER_FOR_OPTIONS = 1038,

    // ipc id for minimize ui extension ability
    MINIMIZE_UI_EXTENSION_ABILITY = 1039,

    // ipc id for terminating ui extension ability
    TERMINATE_UI_EXTENSION_ABILITY = 1040,

    // ipc id for connect ui extension ability
    CONNECT_UI_EXTENSION_ABILITY = 1041,

    CHECK_UI_EXTENSION_IS_FOCUSED = 1042,

    START_UI_ABILITY_BY_SCB = 1043,

    // ipc id for minimize ui ability by scb
    MINIMIZE_UI_ABILITY_BY_SCB = 1044,

    // ipc id for close ui ability by scb
    CLOSE_UI_ABILITY_BY_SCB = 1045,

    // ipc id for request dialog service
    REQUEST_DIALOG_SERVICE = 1046,

    // ipc id for start specified ability by scb
    START_SPECIFIED_ABILITY_BY_SCB = 1047,

    // ipc id for set sessionManagerService
    SET_SESSIONMANAGERSERVICE = 1048,

    // ipc id for report drawn completed
    REPORT_DRAWN_COMPLETED = 1049,

    // ipc id for prepare to terminate ability by scb
    PREPARE_TERMINATE_ABILITY_BY_SCB = 1050,

    // start ui session ability
    START_UI_SESSION_ABILITY_ADD_CALLER = 1051,

    START_UI_SESSION_ABILITY_FOR_OPTIONS = 1052,

    // start ability by insigt intent
    START_ABILITY_BY_INSIGHT_INTENT = 1053,

    // get dialog session info
    GET_DIALOG_SESSION_INFO = 1054,

    // send dialog result
    SEND_DIALOG_RESULT = 1055,

    // request modal UIExtension by want
    REQUEST_MODAL_UIEXTENSION = 1056,

    // get root host info of uiextension
    GET_UI_EXTENSION_ROOT_HOST_INFO = 1057,

    // change current ability visibility
    CHANGE_ABILITY_VISIBILITY = 1058,

    // change ui ability visibility by scb
    CHANGE_UI_ABILITY_VISIBILITY_BY_SCB = 1059,

    // ipc id for start ability for result as caller
    START_ABILITY_FOR_RESULT_AS_CALLER = 1060,

    // ipc id for start ability for result as caller
    START_ABILITY_FOR_RESULT_AS_CALLER_FOR_OPTIONS = 1061,

    // ipc id for preload UIExtension ability by want
    PRELOAD_UIEXTENSION_ABILITY = 1062,

    // ipc id for start UIExtension ability embedded
    START_UI_EXTENSION_ABILITY_EMBEDDED = 1063,

    // ipc id for start UIExtension ability constrained embedded
    START_UI_EXTENSION_CONSTRAINED_EMBEDDED = 1064,

    // get ui extension session info
    GET_UI_EXTENSION_SESSION_INFO = 1065,

    // ipc id for clean uiability from user
    CLEAN_UI_ABILITY_BY_SCB = 1066,

    // start ability only ui ability
    START_ABILITY_ONLY_UI_ABILITY = 1067,

    //terminate UIServiceExtensionAbility
    TERMINATE_UI_SERVICE_EXTENSION_ABILITY = 1068,

    // ipc id for close ui extension ability by scb
    CLOSE_UI_EXTENSION_ABILITY_BY_SCB = 1069,

    SEND_LOCAL_PENDING_WANT_SENDER = 1070,

    SET_ON_NEW_WANT_SKIP_SCENARIOS = 1071,

    NOTIFY_STARTUP_EXCEPTION_BY_SCB = 1072,

    // ipc id for start UI abilities
    START_UI_ABILITIES = 1073,

    START_UI_ABILITIES_IN_SPLIT_WINDOW_MODE = 1074,

    // ipc id for get want from proxy
    GET_PENDING_REQUEST_WANT_FROM_PROXY = 1075,

    START_UI_ABILITY_WITH_CALLBACK = 1076,

    // ipc id for continue ability(1101)
    START_CONTINUATION = 1101,

    NOTIFY_CONTINUATION_RESULT = 1102,

    NOTIFY_COMPLETE_CONTINUATION = 1103,

    CONTINUE_ABILITY = 1104,

    CONTINUE_MISSION = 1105,

    SEND_RESULT_TO_ABILITY = 1106,

    REGISTER_REMOTE_ON_LISTENER = 1107,

    REGISTER_REMOTE_OFF_LISTENER = 1108,

    CONTINUE_MISSION_OF_BUNDLENAME = 1109,

    // ipc id for mission manager(1110)
    REGISTER_REMOTE_MISSION_LISTENER = 1110,
    UNREGISTER_REMOTE_MISSION_LISTENER = 1111,
    START_SYNC_MISSIONS = 1112,
    STOP_SYNC_MISSIONS = 1113,
    REGISTER_SNAPSHOT_HANDLER = 1114,
    GET_MISSION_SNAPSHOT_INFO = 1115,
    MOVE_MISSIONS_TO_FOREGROUND = 1117,
    MOVE_MISSIONS_TO_BACKGROUND = 1118,
    UPDATE_MISSION_SNAPSHOT_FROM_WMS,

    // ipc id for user test(1120)
    START_USER_TEST = 1120,
    FINISH_USER_TEST = 1121,
    DELEGATOR_DO_ABILITY_FOREGROUND = 1122,
    DELEGATOR_DO_ABILITY_BACKGROUND = 1123,
    GET_TOP_ABILITY_TOKEN         = 1124,
    // ipc id for starting ability with specify token id(1125)
    START_ABILITY_WITH_SPECIFY_TOKENID = 1125,
    REGISTER_ABILITY_FIRST_FRAME_STATE_OBSERVER = 1126,
    UNREGISTER_ABILITY_FIRST_FRAME_STATE_OBSERVER = 1127,
    // ipc for get ability state by persistent id
    GET_ABILITY_STATE_BY_PERSISTENT_ID = 1128,
    TRANSFER_ABILITY_RESULT = 1129,
    // ipc for notify frozen process by RSS
    NOTIFY_FROZEN_PROCESS_BY_RSS = 1130,

    // ipc id for pre-start mission
    PRE_START_MISSION = 1135,

    // ipc for open link
    OPEN_LINK = 1140,

    //ipc for suspend extionsion ability
    SUSPEND_EXTENSION_ABILITY = 1141,
    // ipc for resume extionsion ability
    RESUME_EXTENSION_ABILITY = 1142,
    //ipc for prelaunch
    START_PRELAUNCH_ABILITY = 1143,
    // ipc for request modal UIExtension with account
    REQUEST_MODAL_UI_EXTENSION_WITH_ACCOUNT = 1144,

    EXECUTE_INTENT_FOR_DISTRIBUTED = 1145,

    // ipc id 2001-3000 for tools
    // ipc id for dumping state (2001)
    DUMP_STATE = 2001,
    DUMPSYS_STATE = 2002,
    FORCE_TIMEOUT,

    REGISTER_WMS_HANDLER = 2500,
    COMPLETEFIRSTFRAMEDRAWING = 2501,
    REGISTER_CONNECTION_OBSERVER = 2502,
    UNREGISTER_CONNECTION_OBSERVER = 2503,
    GET_DLP_CONNECTION_INFOS = 2504,
    GET_CONNECTION_DATA = 2505,
    COMPLETE_FIRST_FRAME_DRAWING_BY_SCB = 2506,

    //ipc id for foreground app connection observer
    REGISTER_FOREGROUND_APP_CONNECTION_OBSERVER = 2507,
    UNREGISTER_FOREGROUND_APP_CONNECTION_OBSERVER = 2508,

    GET_TOP_ABILITY = 3000,
    FREE_INSTALL_ABILITY_FROM_REMOTE = 3001,
    ADD_FREE_INSTALL_OBSERVER = 3002,
    GET_ELEMENT_NAME_BY_TOKEN = 3003,

    // ipc id for app recovery(3010)
    ABILITY_RECOVERY = 3010,
    ABILITY_RECOVERY_ENABLE = 3011,

    QUERY_MISSION_VAILD = 3012,

    VERIFY_PERMISSION = 3013,

    CLEAR_RECOVERY_PAGE_STACK = 3014,

    ABILITY_RECOVERY_SUBMITINFO = 3015,

    ACQUIRE_SHARE_DATA = 4001,
    SHARE_DATA_DONE = 4002,

    // ipc id for notify as result (notify to snadbox app)
    NOTIFY_SAVE_AS_RESULT = 4201,

    // ipc id for collborator
    REGISTER_COLLABORATOR = 4050,
    UNREGISTER_COLLABORATOR = 4051,
    GET_ABILITY_MANAGER_COLLABORATOR = 4052,

    IS_ABILITY_CONTROLLER_START = 4054,
    OPEN_FILE = 4055,

    GET_ABILITY_TOKEN = 5001,

    REGISTER_STATUS_BAR_DELEGATE = 5100,
    KILL_PROCESS_WITH_PREPARE_TERMINATE = 5101,

    KILL_PROCESS_WITH_REASON = 5200,

    KILL_PROCESS_FOR_PERMISSION_UPDATE = 5300,

    FORCE_EXIT_APP = 6001,
    RECORD_APP_EXIT_REASON = 6002,
    RECORD_PROCESS_EXIT_REASON = 6003,
    UPGRADE_APP = 6004,
    MOVE_UI_ABILITY_TO_BACKGROUND = 6005,
    RECORD_PROCESS_EXIT_REASON_PLUS = 6006,
    KILL_APP_WITH_REASON = 6007,
    KILL_BUNDLE_WITH_REASON = 6008,
    RECORD_APP_WITH_REASON = 6009,
    RECORD_APP_WITH_REASON_BY_USERID = 6010,

    // ipc id for register auto startup system callback
    REGISTER_AUTO_STARTUP_SYSTEM_CALLBACK = 6101,
    // ipc id for unregister auto startup system callback
    UNREGISTER_AUTO_STARTUP_SYSTEM_CALLBACK = 6102,
    // ipc id for set application auto startup
    SET_APPLICATION_AUTO_STARTUP = 6103,
    // ipc id for cancel application auto startup
    CANCEL_APPLICATION_AUTO_STARTUP = 6104,
    // ipc id for query all auto startup application
    QUERY_ALL_AUTO_STARTUP_APPLICATION = 6105,
    // ipc id for retrieves the auto startup status of the current application.
    GET_AUTO_STARTUP_STATUS_FOR_SELF = 6106,
    // ipc id for manual start auto startup apps.
    MANUAL_START_AUTO_STARTUP_APPS = 6107,
    // ipc id for query the caller's token id for anco
    QUERY_CALLER_TOKEN_ID_FOR_ANCO = 6108,
    // ipc id for launch game customized
    LAUNCH_GAME_CUSTOMIZED = 6109,
    // ipc id for set game prelaunch complete time
    SET_GAME_PRELAUNCH_COMPLETE_TIME = 6110,

    // ipc id for on auto starup on
    ON_AUTO_STARTUP_ON = 6111,
    // ipc id for on auto starup off
    ON_AUTO_STARTUP_OFF = 6112,

    // ipc id for register session handler
    REGISTER_SESSION_HANDLER = 6010,
    // ipc id for update session info
    UPDATE_SESSION_INFO = 6011,

    // ipc id for set application auto startup by EDM
    SET_APPLICATION_AUTO_STARTUP_BY_EDM = 6113,
    // ipc id for cancel application auto startup by EDM
    CANCEL_APPLICATION_AUTO_STARTUP_BY_EDM = 6114,

    // ipc id for restart app
    RESTART_APP = 6115,
    // ipc id for request to display assert fault dialog
    REQUEST_ASSERT_FAULT_DIALOG = 6116,
    // ipc id for notify the operation status of the user
    NOTIFY_DEBUG_ASSERT_RESULT = 6117,

    // ipc id for terminate mission
    TERMINATE_MISSION = 6118,

    // ipc for block all apps from starting
    BLOCK_ALL_APP_START = 6119,

    // update associate config list
    UPDATE_ASSOCIATE_CONFIG_LIST = 6120,

    // set application keep alive
    SET_APPLICATION_KEEP_ALLIVE = 6121,

    // get keep-alive applications
    GET_APPLICATIONS_KEEP_ALIVE = 6122,

    // set application keep alive by EDM
    SET_APPLICATION_KEEP_ALLIVE_BY_EDM = 6123,

    // get keep-alive applications by EDM
    GET_APPLICATIONS_KEEP_ALIVE_BY_EDM = 6124,

    GET_INTENT_EXEMPTION_INFO = 6125,

    // set app service extension keep alive
    SET_APP_SERVICE_EXTENSION_KEEP_ALIVE = 6126,

    // get keep-alive app service extensions
    GET_APP_SERVICE_EXTENSIONS_KEEP_ALIVE = 6127,

    // add query ERMS observer
    ADD_QUERY_ERMS_OBSERVER = 6130,

    // query atomic service startup rule
    QUERY_ATOMIC_SERVICE_STARTUP_RULE = 6131,

    // start self uiability
    NDK_START_SELF_UI_ABILITY = 6132,

    // prepare terminate done
    PREPARE_TERMINATE_ABILITY_DONE = 6133,

    // kill process with prepare terminate done
    KILL_PROCESS_WITH_PREPARE_TERMINATE_DONE = 6134,

    // register hidden start observer
    REGISTER_HIDDEN_START_OBSERVER = 6135,

    // Unregister hidden start observer
    UNREGISTER_HIDDEN_START_OBSERVER = 6136,

    // Query preload uiextension record
    QUERY_PRELOAD_UIEXTENSION_RECORD = 6137,

    // start self uiability with startOptions
    START_SELF_UI_ABILITY_WITH_START_OPTIONS = 6138,

    // revoke delegator
    REVOKE_DELEGATOR = 6139,

    // get all insight intent info
    GET_ALL_INSIGHT_INTENT_INFO = 6140,

    // get all insight intent info by bundle name
    GET_INSIGHT_INTENT_INFO_BY_BUNDLE_NAME = 6141,

    // get all insight intent info by intent name
    GET_INSIGHT_INTENT_INFO_BY_INTENT_NAME = 6142,

    // start ability with wait
    START_ABILITY_WITH_WAIT = 6143,

    // restart atomic service
    RESTART_SELF_ATOMIC_SERVICE = 6144,

    // update the list of applications allowed in kiosk mode
    UPDATE_KIOSK_APP_LIST = 6145,

    // enter kiosk mode
    ENTER_KIOSK_MODE = 6146,

    // exit kiosk mode
    EXIT_KIOSK_MODE = 6147,

    // get current kiosk mode information
    GET_KIOSK_INFO = 6148,

    // register sa interceptor
    REGISTER_SA_INTERCEPTOR = 6149,

    // ipc id for start UIExtension ability pre view embedded
    START_UI_EXTENSION_PRE_VIEW_EMBEDDED = 6150,

    // preload application
    PRELOAD_APPLICATION = 6151,

    // start self uiability with startOptions and receives the pid
    START_SELF_UI_ABILITY_WITH_PID_RESULT = 6152,

    // start self uiability in current process
    START_SELF_UI_ABILITY_IN_CURRENT_PROCESS = 6153,

    // restart app limit check
    IS_RESTART_APP_LIMIT = 6154,

    // ipc id for un preload UIExtension ability by want and receives the preload Id
    UN_PRELOAD_UI_EXTENSION_ABILITY = 6155,

    // ipc id for claer all preload UIExtension ability
    CLEAR_ALL_PRELOAD_UI_EXTENSION_ABILITY = 6156,

    // register preload UIExtension host client
    REGISTER_PRELOAD_UI_EXTENSION_HOST_CLIENT = 6157,

    // unregister preload UIExtension host client
    UNREGISTER_PRELOAD_UI_EXTENSION_HOST_CLIENT = 6158,

    // get user locked bundle list
    GET_USER_LOCKED_BUNDLE_LIST = 6159,

    // set app recovery flag
    SET_APP_RECOVERY_FLAG  = 6160,

    // start ability by oe extension
    START_ABILITY_BY_OE_EXT = 6161,

    // query self modular object Extension infos
    QUERY_SELF_MODULAR_OBJECT_EXTENSION_INFOS = 6162,

    // cancel game prelaunch
    CANCEL_GAME_PRELAUNCH = 6163,

    // complete game prelaunch
    COMPLETE_GAME_PRELAUNCH = 6164,

    // query entity
    INSIGHT_INTENT_QUERY_ENTITY = 6165,

    // execute in-app skill
    EXECUTE_IN_APP_SKILL = 6169,

    // query skill type (independent or in-app)
    QUERY_SKILL_TYPE = 6171,

    // start self ability with token
    START_SELF = 6166,

    // start self uiability with caller token
    START_SELF_UI_ABILITY_WITH_TOKEN = 6167,

    // start self uiability with start options and caller token
    START_SELF_UI_ABILITY_WITH_OPTIONS_AND_TOKEN = 6168,

    // start self uiability by application context in current process
    START_SELF_UI_ABILITY_BY_APP_CONTEXT = 6169,

    // execute skill done with token for identity verification
    EXECUTE_SKILL_DONE_WITH_TOKEN = 6172,

    // execute in-app skill with explicit caller tokenId
    EXECUTE_IN_APP_SKILL_WITH_TOKEN_ID = 6173,
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_IPC_INTERFACE_CODE_H
