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

#ifndef DATASHARE_ERRNO_H
#define DATASHARE_ERRNO_H

#include "errors.h"
namespace OHOS {
namespace DataShare {
/**
* @brief The error code in the data share error case.
*/
constexpr int DATA_SHARE_ERROR = -1;

/**
* @brief The error code in the correct case.
*/
constexpr int E_OK = 0;

/**
* @brief The base code of the exception error code.
*/
constexpr int E_BASE = 1000;

/**
* @brief The error code for common exceptions.
*/
constexpr int E_ERROR = (E_BASE + 1);

/**
* @brief The error code for register exceptions.
*/
constexpr int E_REGISTERED_REPEATED = (E_BASE + 2);


/**
* @brief The error code for register exceptions.
*/
constexpr int E_UNREGISTERED_EMPTY = (E_BASE + 3);

/**
* @brief The error code for invalid statement.
*/
constexpr int E_INVALID_STATEMENT = (E_BASE + 7);

/**
* @brief The error code for invalid column index.
*/
constexpr int E_INVALID_COLUMN_INDEX = (E_BASE + 8);

/**
* @brief The error code for invalid object type.
*/
constexpr int E_INVALID_OBJECT_TYPE = (E_BASE + 20);

/**
* @brief The error code for invalid parcel.
*/
constexpr int E_INVALID_PARCEL = (E_BASE + 42);

/**
* @brief The version is smaller than exist.
*/
constexpr int E_VERSION_NOT_NEWER = (E_BASE + 45);

/**
* @brief Cannot find the template
*/
constexpr int E_TEMPLATE_NOT_EXIST = (E_BASE + 46);

/**
* @brief Cannot find the subscriber
*/
constexpr int E_SUBSCRIBER_NOT_EXIST = (E_BASE + 47);

/**
* @brief Cannot find the uri
*/
constexpr int E_URI_NOT_EXIST = (E_BASE + 48);

/**
* @brief Cannot find the bundleName
*/
constexpr int E_BUNDLE_NAME_NOT_EXIST = (E_BASE + 49);

/**
* @brief BMS not ready
*/
constexpr int E_BMS_NOT_READY = (E_BASE + 50);

/**
* @brief metaData not exists
*/
constexpr int E_METADATA_NOT_EXISTS = (E_BASE + 51);

/**
* @brief silent proxy is disable
*/
constexpr int E_SILENT_PROXY_DISABLE = (E_BASE + 52);

/**
* @brief token is empty
*/
constexpr int E_TOKEN_EMPTY = (E_BASE + 53);

/**
* @brief ext uri is empty
*/
constexpr int E_EXT_URI_INVALID = (E_BASE + 54);

/**
* @brief DataShare not ready
*/
constexpr int E_DATA_SHARE_NOT_READY = (E_BASE + 55);

/**
* @brief The error code for db error.
*/
constexpr int E_DB_ERROR = (E_BASE + 56);

/**
* @brief The error code for data supplier error
*/
constexpr int E_DATA_SUPPLIER_ERROR = (E_BASE + 57);

/**
* @brief The error code for marshal error.
*/
constexpr int E_MARSHAL_ERROR = (E_BASE + 58);

/**
* @brief The error code for unmarshal error.
*/
constexpr int E_UNMARSHAL_ERROR = (E_BASE + 59);

/**
* @brief The error code for write interface token to data error.
*/
constexpr int E_WRITE_TO_PARCE_ERROR = (E_BASE + 60);

/**
* @brief The error code for resultSet busy error.
*/
constexpr int E_RESULTSET_BUSY = (E_BASE + 61);

/**
* @brief The error code for invalid appIndex error.
*/
constexpr int E_APPINDEX_INVALID = (E_BASE + 62);

/**
* @brief The error code for nullptr observer.
*/
constexpr int E_NULL_OBSERVER = (E_BASE + 63);

/**
* @brief The error code for reusing helper instance released before.
*/
constexpr int E_HELPER_DIED = (E_BASE + 64);

/**
* @brief The error code for failure to get dataobs client.
*/
constexpr int E_DATA_OBS_NOT_READY = (E_BASE + 65);

/**
* @brief The error code for failure to connect the provider.
*/
constexpr int E_PROVIDER_NOT_CONNECTED = (E_BASE + 66);

/**
* @brief The error code for failure to null connection.
*/
constexpr int E_PROVIDER_CONN_NULL = (E_BASE + 67);

/**
* @brief The error code for passing invalid form of user id.
*/
constexpr int E_INVALID_USER_ID = (E_BASE + 68);

/**
* @brief The error code for not system app.
*/
constexpr int E_NOT_SYSTEM_APP = (E_BASE + 69);

/**
* @brief The error code for register observer inner error.
*/
constexpr int E_REGISTER_ERROR = (E_BASE + 70);

/**
* @brief The error code for notify change inner error.
*/
constexpr int E_NOTIFYCHANGE_ERROR = (E_BASE + 71);

/**
* @brief The error code for timeout inner error.
*/
constexpr int E_TIMEOUT_ERROR = (E_BASE + 72);

/**
* @brief This error code indicates that the timeout interface is busy.
*/
constexpr int E_ERROR_OVER_LIMIT_TASK = (E_BASE + 73);

/**
* @brief The error code for pool is null.
*/
constexpr int E_EXECUTOR_POOL_IS_NULL = (E_BASE + 74);

/**
* @brief The error code for not hap.
*/
constexpr int E_NOT_HAP = (E_BASE + 75);

/**
* @brief The error code for get bundle info failed.
*/
constexpr int E_GET_BUNDLEINFO_FAILED = (E_BASE + 76);

/**
* @brief The error code for not datashare extension.
*/
constexpr int E_NOT_DATASHARE_EXTENSION = (E_BASE + 77);

/**
* @brief The error code for invalid uri.
*/
constexpr int E_DATASHARE_INVALID_URI = (E_BASE + 78);

/**
* @brief The error code for verify failed.
*/
constexpr int E_VERIFY_FAILED = (E_BASE + 79);

/**
* @brief The error code for permission denied.
*/
constexpr int E_DATASHARE_PERMISSION_DENIED = (E_BASE + 80);

/**
* @brief The error code for empty uri.
*/
constexpr int E_EMPTY_URI = (E_BASE + 81);

/**
* @brief The error code for uri not trust.
*/
constexpr int E_NOT_IN_TRUSTS = (E_BASE + 82);

/**
* @brief The error code for get caller failed.
*/
constexpr int E_GET_CALLER_NAME_FAILED = (E_BASE + 83);

/**
* @brief The error code for nullptr observer client.
*/
constexpr int E_NULL_OBSERVER_CLIENT = (E_BASE + 84);

/**
* @brief The virtual function is not yet implement.
*/
constexpr int E_UNIMPLEMENT = (E_BASE + 85);

/**
* @brief The error code for DataShare Silent/Non-Silent Type Error.
*/
constexpr int E_DATASHARE_TYPE = (E_BASE + 86);

/**
* @brief The error code for illegal predicate field.
*/
constexpr int E_FIELD_ILLEGAL = (E_BASE + 87);

/**
* @brief The error code for invalid predicate field.
*/
constexpr int E_FIELD_INVALID = (E_BASE + 88);

/**
* @brief The error code for system ability manager operate failed.
*/
constexpr int E_SYSTEM_ABILITY_OPERATE_FAILED = (E_BASE + 89);
} // namespace DataShare
} // namespace OHOS

#endif