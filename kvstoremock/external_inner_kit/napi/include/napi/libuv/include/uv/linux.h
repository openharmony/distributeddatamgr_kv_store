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

#ifndef UV_LINUX_H
#define UV_LINUX_H

#define UV_PLATFORM_LOOP_FIELDS                                               \
  uv__io_t inotify_read_watcher;                                              \
  void* inotify_watchers;                                                     \
  int inotify_fd;                                                             \

#define UV_PLATFORM_FS_EVENT_FIELDS                                           \
  struct uv__queue watchers;                                                  \
  int wd;                                                                     \

#endif /* UV_LINUX_H */
