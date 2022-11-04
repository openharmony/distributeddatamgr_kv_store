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
#define LOG_TAG "JSUtilMock"
#include "js_util.h"
#include "securec.h"

#define be32toh(data) data
#define be64toh(data) data
#define htobe32(data) data
#define htobe64(data) data
#define GetCurrentAbilityParam(env, param)    GetParam(env, param)

#ifdef _WIN32
#define mkdir(dir, mode)  mkdir(dir)
#endif

#ifndef _WIN32
#define memcpy_s(t, tLen, s, len) memcpy(t, s, std::min(tlen, slen))
#endif

static napi_status GetParam(napi_env env, OHOS::DistributedKVStore::ContextParam &param)                                        
{                                
#ifdef _WIN32                                                                      
    std::string baseDir = getenv("TEMP");                                              
    if (baseDir.empty()) {                                                             
        return napi_invalid_arg;                                                       
    }                                                                                
#else                                                                                  
    std::string baseDir = getenv("LOGNAME");                                           
    if (baseDir.empty()) {                                                             
        return napi_invalid_arg;                                                       
    }                                                                                  
    baseDir = "/Users/" + baseDir + "/Library/Caches";                         
#endif
    mkdir(param.baseDir.c_str(), MODE);                                         
    param.baseDir = baseDir + "/HuaweiDevEcoStudioDatabases";
    param.area = OHOS::DistributedKv::Area::EL1;                                         
    param.hapName = "com.example.myapplication";
    return napi_ok;               
}

#include "../../frameworks/jskitsimpl/distributedkvstore/src/js_util.cpp"

