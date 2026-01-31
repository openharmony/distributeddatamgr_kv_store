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
#include "gspd_api_manager.h"

#include <string>
#include <mutex>

#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifndef _WIN32
static void *g_library = nullptr;
static int g_openCount = 0;
static std::mutex g_apiInfoMutex;
#endif

namespace DistributedDB {

void GSPD_ApiInit(GSPD_APIInfo &gspdApiInfo)
{
#ifndef _WIN32
    gspdApiInfo.isEntityDuplicateApi = (IsEntityDuplicate)dlsym(g_library, "GsPD_IsEntityDuplicate");
#endif
}

static GSPD_APIInfo g_gspdProcessApiInfo;

GSPD_APIInfo *GetApiInfo(void)
{
    return &g_gspdProcessApiInfo;
}

void GetApiInfoInstance(void)
{
#ifndef _WIN32
    std::lock_guard<std::mutex> lock(g_apiInfoMutex);
    g_openCount++;
    if (g_library != nullptr) {
        return;
    }
    std::string libPath = "libdataflow_engine.z.so";
    g_library = dlopen(libPath.c_str(), RTLD_LAZY);
    if (g_library == nullptr) {
        return;
    }
    GSPD_ApiInit(g_gspdProcessApiInfo);
#endif
}

void UnloadApiInfo(GSPD_APIInfo *gspdApiInfo)
{
#ifndef _WIN32
    std::lock_guard<std::mutex> lock(g_apiInfoMutex);
    g_openCount--;
    if (g_library == nullptr) {
        return;
    }
    if (g_openCount == 0) {
        dlclose(g_library);
        *gspdApiInfo = {nullptr};
        g_library = nullptr;
    }
#endif
}

bool CheckGSPDApi(void)
{
#ifndef _WIN32
    bool exist = true;
    if (g_library == nullptr) {
        GetApiInfoInstance();
        exist = (g_library != nullptr);
        UnloadApiInfo(&g_gspdProcessApiInfo);
    }
    return exist;
#else
    return false;
#endif
}

} // namespace DistributedDB
