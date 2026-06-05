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

#ifndef UV_MVS_H
#define UV_MVS_H

#define UV_PLATFORM_SEM_T long

#define UV_PLATFORM_LOOP_FIELDS                                               \
  void* ep;                                                                   \

#define UV_PLATFORM_FS_EVENT_FIELDS                                           \
  char rfis_rftok[8];                                                         \

#endif /* UV_MVS_H */
