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

#ifndef GSPD_API_MANAGER_H
#define GSPD_API_MANAGER_H

#include <cstdint>

namespace DistributedDB {

typedef int32_t (*IsEntityDuplicate)(const char *queryJson, const char *dbJson, bool *isDuplicate);

struct GSPD_APIInfo {
    IsEntityDuplicate isEntityDuplicateApi = nullptr;
};

GSPD_APIInfo *GetApiInfo(void);
void GetApiInfoInstance(void);
void UnloadApiInfo(GSPD_APIInfo *gspdApiInfo);
bool CheckGSPDApi(void);

} // namespace DistributedDB

#endif // GSPD_API_MANAGER_H
