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

#include <unistd.h>

#include "fdsan_mock.h"

uint64_t fdsan_create_owner_tag(fdsan_owner_type type, uint64_t tag)
{
    (void)type;
    (void)tag;
    return 0;
}

void fdsan_exchange_owner_tag(int fd, uint64_t expected_tag, uint64_t new_tag)
{
    (void)fd;
    (void)expected_tag;
    (void)new_tag;
}

int fdsan_close_with_tag(int fd, uint64_t tag)
{
    (void)tag;
    return close(fd);
}
