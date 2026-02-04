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
static std::mutex g_apiInfoMutex;
#endif

namespace DistributedDB {

void GSPD_ApiInit(GsPD_APIInfo &gspdApiInfo)
{
#ifndef _WIN32
    gspdApiInfo.getDupHdlApi = (GetDuplicateHandle)dlsym(g_library, "GsPD_GetDuplicateHandle");
    if (gspdApiInfo.getDupHdlApi == nullptr) {
        gspdApiInfo = {nullptr, nullptr, nullptr, nullptr};
        return;
    }
    gspdApiInfo.isEntityDuplicateApi = (IsEntityDuplicate)dlsym(g_library, "GsPD_IsEntityDuplicate");
    if (gspdApiInfo.isEntityDuplicateApi == nullptr) {
        gspdApiInfo = {nullptr, nullptr, nullptr, nullptr};
        return;
    }
    gspdApiInfo.finalizeDupHdlApi = (FinalizeDuplicateHandle)dlsym(g_library, "GsPD_FinalizeDuplicateHandle");
    if (gspdApiInfo.finalizeDupHdlApi == nullptr) {
        gspdApiInfo = {nullptr, nullptr, nullptr, nullptr};
        return;
    }
#endif
}

static GsPD_APIInfo g_gspdApiInfo;

GsPD_APIInfo *GetGsPDApiInfo(void)
{
    return &g_gspdApiInfo;
}

void GetGsPDApiInfoInstance(void)
{
#ifndef _WIN32
    std::lock_guard<std::mutex> lock(g_apiInfoMutex);
    if (g_library != nullptr) {
        return;
    }
    std::string libPath = "libdataflow_engine.z.so";
    g_library = dlopen(libPath.c_str(), RTLD_LAZY);
    if (g_library == nullptr) {
        return;
    }
    GSPD_ApiInit(g_gspdApiInfo);
    if (g_gspdApiInfo.getDupHdlApi == nullptr) {
        dlclose(g_library);
        g_library = nullptr;
        return;
    }
    g_gspdApiInfo.dupHdl = g_gspdApiInfo.getDupHdlApi();
    if (g_gspdApiInfo.dupHdl == nullptr) {
        dlclose(g_library);
        g_gspdApiInfo = {nullptr, nullptr, nullptr, nullptr};
        g_library = nullptr;
        return;
    }
#endif
}

void UnloadGsPDApiInfo(GsPD_APIInfo *gspdApiInfo)
{
#ifndef _WIN32
    std::lock_guard<std::mutex> lock(g_apiInfoMutex);
    if (g_library == nullptr) {
        return;
    }
    if (gspdApiInfo->finalizeDupHdlApi != nullptr) {
        gspdApiInfo->finalizeDupHdlApi(gspdApiInfo->dupHdl);
    }
    dlclose(g_library);
    *gspdApiInfo = {nullptr, nullptr, nullptr, nullptr};
    g_library = nullptr;
#endif
}

bool CheckGSPDApi(void)
{
#ifndef _WIN32
    bool exist = true;
    if (g_library == nullptr) {
        GetGsPDApiInfoInstance();
        exist = (g_library != nullptr);
        UnloadGsPDApiInfo(&g_gspdApiInfo);
    }
    return exist;
#else
    return false;
#endif
}

} // namespace DistributedDB
