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

#ifndef OHOS_DISTRIBUTED_DATA_INTERFACES_DISTRIBUTEDDATA_STORE_ERRNO_H
#define OHOS_DISTRIBUTED_DATA_INTERFACES_DISTRIBUTEDDATA_STORE_ERRNO_H
#include <errors.h>
namespace OHOS::DistributedKv {
constexpr ErrCode DISTRIBUTEDDATAMGR_ERR_OFFSET = ErrCodeOffset(SUBSYS_DISTRIBUTEDDATAMNG, 3);
/**
 * @brief Flags for kvstore operation.
 *
 * SUCCESS for success return, others for failure.
*/
enum Status : int32_t {
    /**
     * Executed success.
    */
    SUCCESS = ERR_OK,
    /**
     * Executed fail.
    */
    ERROR = DISTRIBUTEDDATAMGR_ERR_OFFSET,
    /**
     * Parameter is invalid.
    */
    INVALID_ARGUMENT = DISTRIBUTEDDATAMGR_ERR_OFFSET + 1,
    /**
     * Illegal state.
    */
    ILLEGAL_STATE = DISTRIBUTEDDATAMGR_ERR_OFFSET + 2,
    /**
     * The kvstore service is unavailable.
    */
    SERVER_UNAVAILABLE = DISTRIBUTEDDATAMGR_ERR_OFFSET + 3,
    /**
     * The kvstore is not open.
    */
    STORE_NOT_OPEN = DISTRIBUTEDDATAMGR_ERR_OFFSET + 4,
    /**
     * Subscribe repeatly.
    */
    STORE_ALREADY_SUBSCRIBE = DISTRIBUTEDDATAMGR_ERR_OFFSET + 5,
    /**
     * Not subscribe.
    */
    STORE_NOT_SUBSCRIBE = DISTRIBUTEDDATAMGR_ERR_OFFSET + 6,
    /**
     * Not found.
    */
    NOT_FOUND = DISTRIBUTEDDATAMGR_ERR_OFFSET + 7,
    /**
     * Store not found.
    */
    STORE_NOT_FOUND = NOT_FOUND,
    /**
     * Key not found.
    */
    KEY_NOT_FOUND = STORE_NOT_FOUND,
    /**
     * Device not found.
    */
    DEVICE_NOT_FOUND = STORE_NOT_FOUND,
    /**
     * Option parameter inconsistency.
    */
    STORE_META_CHANGED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 8,
    /**
     * Store upgrade fail.
    */
    STORE_UPGRADE_FAILED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 9,
    /**
     * Database error.
    */
    DB_ERROR = DISTRIBUTEDDATAMGR_ERR_OFFSET + 10,
    /**
     * Network error.
    */
    NETWORK_ERROR = DISTRIBUTEDDATAMGR_ERR_OFFSET + 11,
    /**
     * Permission denied.
    */
    PERMISSION_DENIED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 12,
    /**
     * IPC error.
    */
    IPC_ERROR = DISTRIBUTEDDATAMGR_ERR_OFFSET + 13,
    /**
     * Crypto error.
    */
    CRYPT_ERROR = DISTRIBUTEDDATAMGR_ERR_OFFSET + 14,
    /**
     * Time out.
    */
    TIME_OUT = DISTRIBUTEDDATAMGR_ERR_OFFSET + 15,
    /**
     * Not support operation.
    */
    NOT_SUPPORT = DISTRIBUTEDDATAMGR_ERR_OFFSET + 17,
    /**
     * Schema mismatch.
    */
    SCHEMA_MISMATCH = DISTRIBUTEDDATAMGR_ERR_OFFSET + 18,
    /**
     * Schema invalid.
    */
    INVALID_SCHEMA = DISTRIBUTEDDATAMGR_ERR_OFFSET + 19,
    /**
     * Read support only.
    */
    READ_ONLY = DISTRIBUTEDDATAMGR_ERR_OFFSET + 20,
    /**
     * Value fields invalid.
    */
    INVALID_VALUE_FIELDS = DISTRIBUTEDDATAMGR_ERR_OFFSET + 21,
    /**
     * Value field type invalid
    */
    INVALID_FIELD_TYPE = DISTRIBUTEDDATAMGR_ERR_OFFSET + 22,
    /**
     * Constrain violation.
    */
    CONSTRAIN_VIOLATION = DISTRIBUTEDDATAMGR_ERR_OFFSET + 23,
    /**
     * Invalid format.
    */
    INVALID_FORMAT = DISTRIBUTEDDATAMGR_ERR_OFFSET + 24,
    /**
     * Invalid query format.
    */
    INVALID_QUERY_FORMAT = DISTRIBUTEDDATAMGR_ERR_OFFSET + 25,
    /**
     * invalid query field.
    */
    INVALID_QUERY_FIELD = DISTRIBUTEDDATAMGR_ERR_OFFSET + 26,
    /**
     * System account event processing.
    */
    SYSTEM_ACCOUNT_EVENT_PROCESSING = DISTRIBUTEDDATAMGR_ERR_OFFSET + 27,
    /**
     * Recover success.
    */
    RECOVER_SUCCESS = DISTRIBUTEDDATAMGR_ERR_OFFSET + 28,
    /**
     * Recover fail.
    */
    RECOVER_FAILED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 29,
    /**
     * Exceed max access rate.
    */
    EXCEED_MAX_ACCESS_RATE = DISTRIBUTEDDATAMGR_ERR_OFFSET + 31,
    /**
     * Security level error.
    */
    SECURITY_LEVEL_ERROR = DISTRIBUTEDDATAMGR_ERR_OFFSET + 32,
    /**
     * Over max limits for subscribe observer number.
    */
    OVER_MAX_LIMITS = DISTRIBUTEDDATAMGR_ERR_OFFSET + 33,
    /**
     * The kvstore already closed.
    */
    ALREADY_CLOSED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 34,
    /**
     * Ipc parcel error.
    */
    IPC_PARCEL_ERROR = DISTRIBUTEDDATAMGR_ERR_OFFSET + 35,
    /**
     * Data corrupted.
    */
    DATA_CORRUPTED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 36,
    /**
     * The wal file size over limit.
    */
    WAL_OVER_LIMITS = DISTRIBUTEDDATAMGR_ERR_OFFSET + 37,

    /**
     * Session is opening.
    */
    RATE_LIMIT = DISTRIBUTEDDATAMGR_ERR_OFFSET + 38,

    /**
     * Data is being synchronized.
    */
    SYNC_ACTIVATED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 39,

    /**
     * Not support broadcast.
    */
    NOT_SUPPORT_BROADCAST = DISTRIBUTEDDATAMGR_ERR_OFFSET + 40,

   /**
    * Cloud disabled.
    */
    CLOUD_DISABLED = DISTRIBUTEDDATAMGR_ERR_OFFSET + 41
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_INTERFACES_DISTRIBUTEDDATA_STORE_ERRNO_H
