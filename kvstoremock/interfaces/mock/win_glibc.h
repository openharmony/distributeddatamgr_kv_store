/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef WIN_GLIBC_H
#define WIN_GLIBC_H
#include <stdint.h>
__attribute__((visibility("default"))) int MAC_SetThreadName(const char *name);
using pthread_t = uintptr_t;
#ifndef pthread_setname_np
#define pthread_setname_np(tid, name) 0
#endif
#ifndef pthread_self
#define pthread_self() 0
#endif
#endif // WIN_GLIBC_H