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

#ifndef UV_DARWIN_H
#define UV_DARWIN_H

#if defined(__APPLE__) && defined(__MACH__)
# include <mach/mach.h>
# include <mach/task.h>
# include <mach/semaphore.h>
# include <TargetConditionals.h>
# define UV_PLATFORM_SEM_T semaphore_t
#endif

#define UV_IO_PRIVATE_PLATFORM_FIELDS                                         \
  int rcount;                                                                 \
  int wcount;                                                                 \

#define UV_PLATFORM_LOOP_FIELDS                                               \
  uv_thread_t cf_thread;                                                      \
  void* _cf_reserved;                                                         \
  void* cf_state;                                                             \
  uv_mutex_t cf_mutex;                                                        \
  uv_sem_t cf_sem;                                                            \
  struct uv__queue cf_signals;                                                \

#define UV_PLATFORM_FS_EVENT_FIELDS                                           \
  uv__io_t event_watcher;                                                     \
  char* realpath;                                                             \
  int realpath_len;                                                           \
  int cf_flags;                                                               \
  uv_async_t* cf_cb;                                                          \
  struct uv__queue cf_events;                                                 \
  struct uv__queue cf_member;                                                 \
  int cf_error;                                                               \
  uv_mutex_t cf_mutex;                                                        \

#define UV_STREAM_PRIVATE_PLATFORM_FIELDS                                     \
  void* select;                                                               \

#define UV_HAVE_KQUEUE 1

#endif /* UV_DARWIN_H */
