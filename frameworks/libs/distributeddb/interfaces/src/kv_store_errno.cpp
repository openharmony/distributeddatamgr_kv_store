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

#include "kv_store_errno.h"
#include "db_errno.h"

namespace DistributedDB {
struct DBErrnoPair {
    int errCode;
    DBStatus status;
};

namespace {
    const DBErrnoPair ERRNO_MAP[] = {
        { E_OK, OK },
        { -E_BUSY, BUSY },
        { -E_NOT_FOUND, NOT_FOUND },
        { -E_INVALID_ARGS, INVALID_ARGS },
        { -E_TIMEOUT, TIME_OUT },
        { -E_NOT_SUPPORT, NOT_SUPPORT },
        { -E_INVALID_PASSWD_OR_CORRUPTED_DB, INVALID_PASSWD_OR_CORRUPTED_DB },
        { -E_MAX_LIMITS, OVER_MAX_LIMITS },
        { -E_INVALID_FILE, INVALID_FILE },
        { -E_INVALID_PATH, NO_PERMISSION },
        { -E_READ_ONLY, READ_ONLY },
        { -E_INVALID_SCHEMA, INVALID_SCHEMA },
        { -E_SCHEMA_MISMATCH, SCHEMA_MISMATCH },
        { -E_SCHEMA_VIOLATE_VALUE, SCHEMA_VIOLATE_VALUE },
        { -E_VALUE_MISMATCH_FEILD_COUNT, INVALID_VALUE_FIELDS },
        { -E_VALUE_MISMATCH_FEILD_TYPE, INVALID_FIELD_TYPE },
        { -E_VALUE_MISMATCH_CONSTRAINT, CONSTRAIN_VIOLATION },
        { -E_INVALID_FORMAT, INVALID_FORMAT },
        { -E_STALE, STALE },
        { -E_LOCAL_DELETED, LOCAL_DELETED },
        { -E_LOCAL_DEFEAT, LOCAL_DEFEAT },
        { -E_LOCAL_COVERED, LOCAL_COVERED },
        { -E_INVALID_QUERY_FORMAT, INVALID_QUERY_FORMAT },
        { -E_INVALID_QUERY_FIELD, INVALID_QUERY_FIELD },
        { -E_ALREADY_SET, ALREADY_SET },
        { -E_EKEYREVOKED, EKEYREVOKED_ERROR },
        { -E_SECURITY_OPTION_CHECK_ERROR, SECURITY_OPTION_CHECK_ERROR },
        { -E_INTERCEPT_DATA_FAIL, INTERCEPT_DATA_FAIL },
        { -E_LOG_OVER_LIMITS, LOG_OVER_LIMITS },
        { -E_DISTRIBUTED_SCHEMA_NOT_FOUND, DISTRIBUTED_SCHEMA_NOT_FOUND },
        { -E_DISTRIBUTED_SCHEMA_CHANGED, DISTRIBUTED_SCHEMA_CHANGED },
        { -E_MODE_MISMATCH, MODE_MISMATCH },
        { -E_NO_NEED_ACTIVE, NOT_ACTIVE },
        { -E_NONEXISTENT, NONEXISTENT },
        { -E_TYPE_MISMATCH, TYPE_MISMATCH },
        { -E_DENIED_SQL, NO_PERMISSION },
        { -E_USER_CHANGE, USER_CHANGED },
        { -E_PERIPHERAL_INTERFACE_FAIL, COMM_FAILURE },
        { -E_FEEDBACK_COMMUNICATOR_NOT_FOUND, COMM_FAILURE },
        { -E_FEEDBACK_UNKNOWN_MESSAGE, NOT_SUPPORT },
        { -E_NOT_PERMIT, PERMISSION_CHECK_FORBID_SYNC },
        { -E_REMOTE_OVER_SIZE, OVER_MAX_LIMITS },
        { -E_CONSTRAINT, CONSTRAINT },
        { -E_CLOUD_ERROR, CLOUD_ERROR },
        { -E_DB_CLOSED, DB_CLOSED },
        { -E_NOT_SET, UNSET_ERROR },
        { -E_CLOUD_NETWORK_ERROR, CLOUD_NETWORK_ERROR },
        { -E_CLOUD_SYNC_UNSET, CLOUD_SYNC_UNSET },
        { -E_CLOUD_FULL_RECORDS, CLOUD_FULL_RECORDS },
        { -E_CLOUD_LOCK_ERROR, CLOUD_LOCK_ERROR },
        { -E_CLOUD_ASSET_SPACE_INSUFFICIENT, CLOUD_ASSET_SPACE_INSUFFICIENT },
        { -E_TABLE_REFERENCE_CHANGED, PROPERTY_CHANGED },
        { -E_CLOUD_VERSION_CONFLICT, CLOUD_VERSION_CONFLICT },
        { -E_CLOUD_RECORD_EXIST_CONFLICT, CLOUD_RECORD_EXIST_CONFLICT },
        { -E_REMOVE_ASSETS_FAILED, REMOVE_ASSETS_FAIL },
        { -E_WITH_INVENTORY_DATA, WITH_INVENTORY_DATA },
        { -E_WAIT_COMPENSATED_SYNC, WAIT_COMPENSATED_SYNC },
        { -E_CLOUD_SYNC_TASK_MERGED, CLOUD_SYNC_TASK_MERGED },
        { -E_SQLITE_CANT_OPEN, SQLITE_CANT_OPEN },
        { -E_LOCAL_ASSET_NOT_FOUND, LOCAL_ASSET_NOT_FOUND },
        { -E_ASSET_NOT_FOUND_FOR_DOWN_ONLY, ASSET_NOT_FOUND_FOR_DOWN_ONLY },
        { -E_CLOUD_DISABLED, CLOUD_DISABLED },
        { -E_DISTRIBUTED_FIELD_DECREASE, DISTRIBUTED_FIELD_DECREASE },
    };
}

DBStatus TransferDBErrno(int err, bool isPass)
{
    for (const auto &item : ERRNO_MAP) {
        if (item.errCode == err) {
            return item.status;
        }
    }
    if (isPass) {
        return static_cast<DBStatus>(err);
    }
    return DB_ERROR;
}

int TransferDBStatusToErr(DBStatus dbStatus)
{
    for (const auto &item : ERRNO_MAP) {
        if (item.status == dbStatus) {
            return item.errCode;
        }
    }

    return dbStatus;
}
};
