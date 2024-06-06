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

#ifndef DISTRIBUTEDDATA_KVDB_IPC_INTERFACE_CODE_H
#define DISTRIBUTEDDATA_KVDB_IPC_INTERFACE_CODE_H

#include <cstdint>

/* SAID:1301 FeatureSystem:kvdb_service */
namespace OHOS::DistributedKv {
enum class KVDBServiceInterfaceCode : uint32_t {
    TRANS_HEAD = 0,
    TRANS_GET_STORE_IDS = TRANS_HEAD,
    TRANS_BEFORE_CREATE,
    TRANS_AFTER_CREATE,
    TRANS_DELETE,
    TRANS_SYNC,
    TRANS_REGISTER_NOTIFIER,
    TRANS_UNREGISTER_NOTIFIER,
    TRANS_SET_SYNC_PARAM,
    TRANS_GET_SYNC_PARAM,
    TRANS_ENABLE_CAP,
    TRANS_DISABLE_CAP,
    TRANS_SET_CAP,
    TRANS_ADD_SUB,
    TRANS_RMV_SUB,
    TRANS_SUB,
    TRANS_UNSUB,
    TRANS_GET_PASSWORD,
    TRANS_SYNC_EXT,
    TRANS_CLOUD_SYNC,
    TRANS_NOTIFY_DATA_CHANGE,
    TRANS_PUT_SWITCH,
    TRANS_GET_SWITCH,
    TRANS_SUBSCRIBE_SWITCH_DATA,
    TRANS_UNSUBSCRIBE_SWITCH_DATA,
    TRANS_CLOSE,
    TRANS_BUTT
};

enum class KVDBNotifierCode : uint32_t {
    TRANS_HEAD = 0,
    TRANS_SYNC_COMPLETED = TRANS_HEAD,
    TRANS_CLOUD_SYNC_COMPLETED,
    TRANS_ON_REMOTE_CHANGED,
    TRANS_ON_SWITCH_CHANGED,
    TRANS_BUTT
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATA_KVDB_IPC_INTERFACE_CODE_H