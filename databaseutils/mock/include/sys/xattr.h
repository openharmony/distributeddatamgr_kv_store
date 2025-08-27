/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_XATTR_H
#define MOCK_XATTR_H

ssize_t getxattr(const char *path, const char *name, void *value, size_t size)
{
    return 0;
}

int setxattr(const char *path, const char *name, const void *value, size_t size, int flags)
{
    return 0;
}
#endif // MOCK_XATTR_H