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

#ifndef UV_SUNOS_H
#define UV_SUNOS_H

#include <sys/port.h>
#include <port.h>

/* For the sake of convenience and reduced #ifdef-ery in src/unix/sunos.c,
 * add the fs_event fields even when this version of SunOS doesn't support
 * file watching.
 */
#define UV_PLATFORM_LOOP_FIELDS                                               \
  uv__io_t fs_event_watcher;                                                  \
  int fs_fd;                                                                  \

#if defined(PORT_SOURCE_FILE)

# define UV_PLATFORM_FS_EVENT_FIELDS                                          \
  file_obj_t fo;                                                              \
  int fd;                                                                     \

#endif /* defined(PORT_SOURCE_FILE) */

#endif /* UV_SUNOS_H */
