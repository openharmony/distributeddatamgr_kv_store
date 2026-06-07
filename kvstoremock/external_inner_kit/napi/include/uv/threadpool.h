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

/*
 * This file is private to libuv. It provides common functionality to both
 * Windows and Unix backends.
 */

#ifndef UV_THREADPOOL_H_
#define UV_THREADPOOL_H_


struct uv__work {
#ifdef USE_FFRT
  void (*work)(struct uv__work *w, int qos);
#else
  void (*work)(struct uv__work *w);
#endif
  void (*done)(struct uv__work *w, int status);
  struct uv_loop_s* loop;
  struct uv__queue wq;
};

#endif /* UV_THREADPOOL_H_ */
