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
#ifndef GSPD_DATAFLOW_ERROR_H
#define GSPD_DATAFLOW_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/* External errno */

// General
#define GSPD_DATAFLOW_OK (0)

// Task requests
#define GSPD_DATAFLOW_NOT_SUPPORTED (-10001000)
#define GSPD_DATAFLOW_OVER_LIMIT (-10002000)
#define GSPD_DATAFLOW_INVALID_ARGS (-10003000)
#define GSPD_DATAFLOW_INSTANCE_ABNORMAL (-10004000)

// Data
#define GSPD_DATAFLOW_NO_DATA (-10010000)
#define GSPD_DATAFLOW_DATA_CONFLICT (-10011000)
#define GSPD_DATAFLOW_DATATYPE_MISMATCH (-10012000)
#define GSPD_DATAFLOW_DATA_CORRUPTED (-10015000)

// Task execution
#define GSPD_DATAFLOW_INTERNAL_ERROR (-10020000)
#define GSPD_DATAFLOW_INSUFFICIENT_SPACE (-10021000)
#define GSPD_DATAFLOW_FAILED_MEMORY_OPERATION (-10022000)
#define GSPD_DATAFLOW_PERMISSION_ERROR (-10023000)
#define GSPD_DATAFLOW_TIME_OUT (-10024000)

// Not supported
#define GSPD_DATAFLOW_FEATURE_NOT_SUPPORTED (-15001003)

// Invalid args
#define GSPD_DATAFLOW_INVALID_CONFIG_VALUE (-15003004)
#define GSPD_DATAFLOW_INVALID_VALUE (-15003010)
#define GSPD_DATAFLOW_NAME_TOO_LONG (-15003025)
#define GSPD_DATAFLOW_INVALID_FORMAT (-15003030)

// No data
#define GSPD_DATAFLOW_RECORD_NOT_FOUND (-15010001)
#define GSPD_DATAFLOW_FIELD_NOT_FOUND (-15010002)

// Data conflict
#define GSPD_DATAFLOW_KEY_CONFLICT (-15011001)
#define GSPD_DATAFLOW_FIELD_TYPE_CONFLICT (-15011002)

#ifdef __cplusplus
}
#endif

#endif // GSPD_DATAFLOW_ERROR_H
