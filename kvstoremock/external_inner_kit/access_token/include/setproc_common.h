/**
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
#ifndef SETPROC_COMMON_H
#define SETPROC_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif

#define ACCESS_TOKEN_OK 0
#define ACCESS_TOKEN_PARAM_INVALID (-1)
#define ACCESS_TOKEN_OPEN_ERROR (-2)
#define ACCESS_TOKEN_ID_IOCTL_BASE 'A'
#define TOKENID_DEVNODE "/dev/access_token_id"

enum {
    GET_TOKEN_ID = 1,
    SET_TOKEN_ID,
    GET_FTOKEN_ID,
    SET_FTOKEN_ID,
    ADD_PERMISSIONS,
    REMOVE_PERMISSIONS,
    GET_PERMISSION,
    SET_PERMISSION,
    ACCESS_TOKENID_MAX_NR,
};
#ifdef __cplusplus
}
#endif
#endif // SETPROC_COMMON_H