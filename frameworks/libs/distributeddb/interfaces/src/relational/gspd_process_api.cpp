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
#include "gspd_process_api.h"

#ifndef _WIN32
#include <dlfcn.h>
#endif

#include "gspd_api_manager.h"
#include "gspd_dataflow_error.h"
#include "log_print.h"
#include "db_errno.h"

namespace DistributedDB {

struct GspdErrnoPair {
    int32_t gspdCode;
    int kvDbCode;
};

const GspdErrnoPair GSPD_ERRNO_MAP[] = {
    { GSPD_DATAFLOW_OK, E_OK },
    { GSPD_DATAFLOW_INVALID_ARGS, -E_INVALID_ARGS },
};

int TransferGspdErrno(int err)
{
    if (err > 0) {
        return err;
    }
    for (const auto &item : GSPD_ERRNO_MAP) {
        if (item.gspdCode == err) {
            return item.kvDbCode;
        }
    }
    return -E_INTERNAL_ERROR;
}

static GSPD_APIInfo *g_gspdApiInfo = GetApiInfo();

int32_t GSPD_IsEntityDuplicate(const char *queryJson, const char *dbJson, bool *isDuplicate)
{
    GetApiInfoInstance();
    if (g_gspdApiInfo->isEntityDuplicateApi == nullptr) {
        const char *error = dlerror();
        if (error != nullptr) {
            LOGE("dlsym error: %s\n", error);
        }
        return TransferGspdErrno(GSPD_DATAFLOW_INTERNAL_ERROR);
    }
    int32_t ret = g_gspdApiInfo->isEntityDuplicateApi(queryJson, dbJson, isDuplicate);
    if (ret != GSPD_DATAFLOW_OK) {
        LOGE("Fail to duplicate");
    }
    UnloadApiInfo(g_gspdApiInfo);
    return TransferGspdErrno(ret);
}

} // namespace DistributedDB
